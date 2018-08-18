#version 450
#extension GL_ARB_separate_shader_objects : enable
//#extension GL_KHR_vulkan_glsl : enable

//
// SCENE/VIEW
// Common to VS and FS
layout( set = 0, binding = 0 ) uniform scene_ubo
{
    mat4 view_matrix;
    mat4 proj_matrix;

    vec4 light_position;
    vec4 light_color;
    vec4 light_radius;
    vec4 sky_color;
    // + light type, outer/inner cone angles?
    // + camera lens properties?
} Scene_UBO;

//
// SHADER/PIPELINE
//
layout( set = 0, binding = 1 ) uniform sampler tex_sampler;

//
// MATERIAL INSTANCE
//
layout( set = 1, binding = 0 ) uniform texture2D base_tex; // xyz = albedo or specular. a = alpha
layout( set = 1, binding = 1 ) uniform texture2D spec_tex; // x = roughness, y = metallic

//
// OBJECT
//
layout( set = 2, binding = 1 ) uniform object_ubo
{
    vec4 base; // xyz = albedo or specular. a = alpha
    float roughness;
    float metallic;
} Object_UBO;

//
// IN
//
layout ( location = 0 ) in struct fragment_in {
    vec3 normal;
    vec2 uv;
    vec3 to_camera;
    vec3 to_light;
} IN;

//
// OUT
//
layout (location = 0) out vec4 uFragColor;

//
// UTILS
//

#define PI 3.1415926

vec3 saturate(vec3 v)
{
    return max(vec3(0.0f), min(vec3(1.0f), v));
}

float saturate(float v)
{
    return clamp(v, 0.0, 1.0);
}

vec3 sRGB_to_Linear(vec3 v)
{
    return vec3(pow(v.x, 2.2), pow(v.y, 2.2), pow(v.z, 2.2));
}

vec3 Linear_to_sRGB(vec3 v)
{
    const float i = 1.0/2.2;
    return vec3(pow(v.x, i), pow(v.y, i), pow(v.z, i));
}

float lamb_diffuse()
{
    return (1.0/PI);
}

// product = NdotV or NdotH
vec3 fresnel_factor(in vec3 f0, in float product)
{
    return mix(f0, vec3(1), pow(1.01 - product, 5.0));
}

float D_GGX(in float roughness, in float NdotH)
{
    float m = roughness * roughness;
    float m2 = m*m;
    float d = (NdotH * m2 - NdotH) * NdotH + 1.0;
    return m2 / (PI * d * d);
}

float G_schlick(in float roughness, in float NdotV, in float NdotL)
{
    float k = roughness * roughness * 0.5;
    float V = NdotV * (1.0 - k) + k;
    float L = NdotV * (1.0 - k) + k;
    return 0.25 / (V * L);
}

vec3 cooktorrance_specular( in float NdotL, in float NdotV, in float NdotH, in vec3 specular, in float roughness)
{
    float D = D_GGX(roughness, NdotH);
    float G = G_schlick(roughness, NdotV, NdotL);

    return specular * G * D;
}


//
// MAIN
//

void main() 
{
    vec3 sky_color     = sRGB_to_Linear(Scene_UBO.sky_color.rgb);
    vec3 light_color   = sRGB_to_Linear(Scene_UBO.light_color.rgb);
    float light_radius = Scene_UBO.light_radius.x;

    vec4 sampled_base = texture(sampler2D(base_tex, tex_sampler), IN.uv);
    vec4 sampled_spec = texture(sampler2D(spec_tex, tex_sampler), IN.uv);

    vec3 base       = sRGB_to_Linear(Object_UBO.base.rgb) * sRGB_to_Linear(sampled_base.rgb);
    float roughness = sampled_spec.r * Object_UBO.roughness;
    float metallic  = sampled_spec.g;// * Object_UBO.metallic;

    float dist2 = dot(IN.to_light,IN.to_light);
    float att = saturate(1.0 - dist2/(light_radius*light_radius));
    att *= att;
    light_color *= att;
    
    
    
    vec3 L = normalize( IN.to_light );
    vec3 V = normalize( IN.to_camera );
    vec3 H = normalize( V + L );
    vec3 N = normalize( IN.normal );
    
    float NdotL = max( dot( N, L ), 0.0 );
    float NdotV = max( dot( N, V ), 0.001 );
    float NdotH = max( dot( N, H ), 0.001 );
    float HdotV = max( dot( H, V ), 0.001 );
    
    
    vec3 specular = mix(vec3(0.04), base, metallic); // f0
    
    // COOK TORRANCE
    vec3 specfresnel = fresnel_factor(specular, HdotV);
    vec3 specref = cooktorrance_specular(NdotL, NdotV, NdotH, specfresnel, roughness);
    specref *= vec3(NdotL);
    
    // initial state
    vec3 diffuse_light   = vec3(0); // ambient
    vec3 reflected_light = vec3(0);
    
    // point light
    vec3 diffref = (vec3(1) - specfresnel) * lamb_diffuse() * NdotL;
    diffuse_light   += diffref * light_color;
    reflected_light += specref * light_color;
    
    // sky
    vec3 HSky = normalize( V + vec3(0,1,0) );
    float HSkydotV = max( dot( HSky, V ), 0.001 );
    float NdotHSky = max( dot( N, HSky ), 0.001 );
    float NdotUp = 0.5 * ( dot( N, vec3(0,1,0) ) + 1.0 ); // warp
    NdotUp *= NdotUp;
    vec3 specfresnel_sky = fresnel_factor(specular, HSkydotV);
    vec3 specref_sky = cooktorrance_specular(NdotUp, NdotV, NdotHSky, specfresnel_sky, roughness);
    specref_sky *= vec3(NdotUp);
    vec3 diffref_sky = (vec3(1) - specfresnel_sky) * lamb_diffuse() * NdotUp;
    diffuse_light += 0.3 * diffref_sky * sky_color;
    reflected_light += specref_sky * sky_color;
    
    vec3 result = 
          diffuse_light * mix(base, vec3(0), metallic)
        + reflected_light;
    
    uFragColor = vec4(Linear_to_sRGB(result), 1);
    
    // DEBUG
    //uFragColor = vec4(0.5*(N+1),1);
    //uFragColor = vec4(0.5*(V+1),1);
    //uFragColor = vec4(0.5*(L+1),1);
    //uFragColor = vec4(0.5*(R+1),1);
    //uFragColor = vec4(NdotL, NdotL, NdotL, 1);
}
