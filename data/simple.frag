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
    vec4 spec; // x = roughness, y = metallic, z = reflectance
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



// a = alpha = linear_roughness = roughness * roughness
float D_GGX(in float NdotH, in float a)
{
    float a2 = a * a;
    float f = (NdotH * a2 - NdotH) * NdotH + 1.0;
    return a2 / (PI * f * f);
}

float sqr(float a) { return a*a;}

float D_GGX_Anisotropic(float NoH, const vec3 h, const vec3 t, const vec3 b, float at, float ab)
{
    float ToH = dot(t, h);
    float BoH = dot(b, h);
    float a2 = at * ab;
    vec3 v = vec3(ab * ToH, at * BoH, a2 * NoH);
    return a2 * sqr(a2 / dot(v, v)) * (1.0 / PI);
}

// product can be VdotH, NdotV or NdotH
vec3 F_Schlick(in float product, in vec3 f0) 
{
    return f0 + (vec3(1.0) - f0) * pow(1.0 - product, 5.0);
    //return mix(f0, vec3(1), pow(1.01 - product, 5.0));
}

float V_SmithGGXCorrelated(float NoV, float NoL, float a)
{
    float a2 = a * a;
    float GGXL = NoV * sqrt((-NoL * a2 + NoL) * NoL + a2);
    float GGXV = NoL * sqrt((-NoV * a2 + NoV) * NoV + a2);
    return 0.5 / (GGXV + GGXL);
}

float V_SmithGGXCorrelated_Anisotropic(float at, float ab, float ToV, float BoV, float ToL, float BoL, float NoV, float NoL)
{
    float lambdaV = NoL * length(vec3(at * ToV, ab * BoV, NoV));
    float lambdaL = NoV * length(vec3(at * ToL, ab * BoL, NoL));
    float v = 0.5 / (lambdaV + lambdaL);
    return saturate(v);
}

// cheap V for clear coat
float V_Kelemen(in float LdotH)
{
    return 0.25 / (LdotH * LdotH);
}

float Fd_Lambert()
{
    return 1.0 / PI;
}








float G_Schlick(in float a, in float NdotV, in float NdotL)
{
    float k = a * 0.5;
    float V = NdotV * (1.0 - k) + k;
    float L = NdotV * (1.0 - k) + k;
    return 0.25 / (V * L);
}

vec3 cooktorrance_specular( in float NdotL, in float NdotV, in float NdotH, in vec3 specular, in float a)
{
    float D = D_GGX(NdotH, a);
    float G = G_Schlick(a, NdotV, NdotL);

    return specular * G * D;
}


//
// MAIN
//

//#define ANISOTROPY
//#define CLEAT_COAT

void main() 
{
#ifdef ANISOTROPY
    float anisotropy = 1.0; // [-1..1]
#endif

    float clearCoatRoughness = 0.0;
    float clearCoat = 1.0;

    vec4 sampled_base = texture(sampler2D(base_tex, tex_sampler), IN.uv);
    vec4 sampled_spec = texture(sampler2D(spec_tex, tex_sampler), IN.uv);

    vec3 base         = sRGB_to_Linear(Object_UBO.base.rgb) * sRGB_to_Linear(sampled_base.rgb);
    float roughness   = sampled_spec.r * Object_UBO.spec.x;
    float metallic    = sampled_spec.g * Object_UBO.spec.y;
    float reflectance = sampled_spec.b * Object_UBO.spec.z;

    // remappings
    vec3 diffuse_color = mix(base, vec3(0), metallic); // (1-metallic)*base
    reflectance = 0.16 * reflectance * reflectance; // 0..4%..16% for dielectrics
    vec3 f0 = mix(vec3(reflectance), base, metallic); // metal use base as reflectance
    float linear_roughness = roughness * roughness;

#ifdef ANISOTROPY
    float at = max(linear_roughness * (1.0 + anisotropy), 0.001);
    float ab = max(linear_roughness * (1.0 - anisotropy), 0.001);
#endif

    vec3 l = normalize( IN.to_light );
    vec3 v = normalize( IN.to_camera );
    vec3 h = normalize( v + l );
    vec3 n = normalize( IN.normal );
    
    float NdotL = max( dot( n, l ), 0.0 );
    float NdotV = abs( dot( n, v ) ) + 1e-5;
    float NdotH = max( dot( n, h ), 1e-5 );
    float VdotH = max( dot( v, h ), 1e-5 );
    float LdotH = max( dot( l, h ), 1e-5 );

    // Cook-Torrance Specular Microfacet
#ifdef ANISOTROPY
    vec3 t = vec3(1,0,0); // tangent of tangent space
    vec3 b = vec3(0,1,0); // bitangent
    float D = D_GGX_Anisotropic(NdotH, h, t, b, at, ab);
#else
    float D = D_GGX(NdotH, linear_roughness);
#endif

    vec3  F = F_Schlick(LdotH, f0);

#ifdef ANISOTROPY
    float TdotV = max( dot( t, v ), 0.0 );
    float BdotV = max( dot( b, v ), 0.0 );
    float TdotL = max( dot( t, l ), 0.0 );
    float BdotL = max( dot( b, l ), 0.0 );
    float V = V_SmithGGXCorrelated_Anisotropic(at, ab, TdotV, BdotV, TdotL, BdotL, NdotV, NdotL);
#else
    float V = V_SmithGGXCorrelated(NdotV, NdotL, linear_roughness);
#endif
    // specular BRDF
    vec3 Fr = (D * V) * F;

    // diffuse BRDF
    vec3 Fd = diffuse_color * Fd_Lambert();
    
#if CLEAR_COAT
    // remapping and linearization of clear coat roughness
    clearCoatRoughness = mix(0.089, 0.6, clearCoatRoughness);
    float clearCoatLinearRoughness = clearCoatRoughness * clearCoatRoughness;

    // clear coat BRDF - dielectric clearcoat with reflectance 4%
    float Dc = D_GGX(NdotH, clearCoatLinearRoughness);
    float Vc = V_Kelemen(LdotH);
    vec3  Fc = F_Schlick(LdotH, vec3(0.04)) * clearCoat; // clear coat strength
    vec3 Frc = (Dc * Vc) * Fc;

    // account for energy loss in the base layer
    //return color * ((Fd + Fr * (1.0 - Fc)) * (1.0 - Fc) + Frc);
#endif

    // apply lighting

    vec3 sky_color     = sRGB_to_Linear(Scene_UBO.sky_color.rgb);
    vec3 light_color   = sRGB_to_Linear(Scene_UBO.light_color.rgb);
    float light_radius = Scene_UBO.light_radius.x;

    float dist2 = dot(IN.to_light,IN.to_light);
    float att = saturate(1.0 - dist2/(light_radius*light_radius));
    att *= att;
    light_color *= att;


    
#if CLEAR_COAT
    vec3 result = light_color * NdotL * ((Fd + Fr * (1.0 - Fc)) * (1.0 - Fc) + Frc);
#else
    vec3 result = light_color * NdotL *(Fd * (1-F) + Fr);
#endif

    // COOK TORRANCE
    //vec3 specfresnel = F_Schlick(LdotH, f0);
    //vec3 specref = cooktorrance_specular(NdotL, NdotV, NdotH, specfresnel, linear_roughness);
    //specref *= vec3(NdotL);
    
    // initial state
    //vec3 diffuse_light   = vec3(0); // ambient
    //vec3 reflected_light = vec3(0);
    
    // point light
    //vec3 diffref = (vec3(1) - specfresnel) * Fd_Lambert() * NdotL;
    //diffuse_light   += diffref * light_color;
    //reflected_light += specref * light_color;
    
    // sky
    /*
    vec3 HSky = normalize( V + vec3(0,1,0) );
    float HSkydotV = max( dot( HSky, V ), 0.001 );
    float NdotHSky = max( dot( N, HSky ), 0.001 );
    float NdotUp = 0.5 * ( dot( N, vec3(0,1,0) ) + 1.0 ); // warp
    NdotUp *= NdotUp;
    vec3 specfresnel_sky = F_Schlick(HSkydotV, f0);
    vec3 specref_sky = cooktorrance_specular(NdotUp, NdotV, NdotHSky, specfresnel_sky, linear_roughness);
    specref_sky *= vec3(NdotUp);
    vec3 diffref_sky = (vec3(1) - specfresnel_sky) * Fd_Lambert() * NdotUp;
    diffuse_light += 0.3 * diffref_sky * sky_color;
    reflected_light += specref_sky * sky_color;

    vec3 result = 
          diffuse_light * diffuse_color
        + reflected_light;
    */

    uFragColor = vec4(Linear_to_sRGB(result), 1);
    
    // DEBUG
    //uFragColor = vec4(0.5*(N+1),1);
    //uFragColor = vec4(0.5*(V+1),1);
    //uFragColor = vec4(0.5*(L+1),1);
    //uFragColor = vec4(0.5*(R+1),1);
    //uFragColor = vec4(NdotL, NdotL, NdotL, 1);
}
