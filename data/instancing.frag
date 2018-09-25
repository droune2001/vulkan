#version 450
#extension GL_ARB_separate_shader_objects : enable
//#extension GL_KHR_vulkan_glsl : enable

//#define ANISOTROPY
//#define CLEAR_COAT
//#define OPTI_FOR_LOW_END

struct light_t
{
    vec4 position;
    vec4 color;
    vec4 direction;
    vec4 properties;
};

//
// SCENE/VIEW
// Common to VS and FS
layout( set = 0, binding = 0, std140 ) uniform subo
{
    mat4 view;
    mat4 proj;

    vec4 sky_color;

    light_t lights[8];

    // + light type, outer/inner cone angles?
    // + camera lens properties?
} scene;

//
// SHADER/PIPELINE
//
layout( set = 0, binding = 1 ) uniform sampler tex_sampler;

//
// MATERIAL INSTANCE - TODO: use 2 texture arrays and texture indices per instance.
//
layout( set = 1, binding = 0 ) uniform texture2D base_tex; // xyz = albedo or specular. a = alpha
layout( set = 1, binding = 1 ) uniform texture2D spec_tex; // x = roughness, y = metallic

//
// IN
//
layout( location = 0 ) in struct fragment_in 
{
    vec3 normal;
    vec2 uv;
    vec3 to_camera;
    vec3 world_pos;
    vec4 base; // instance data
    vec4 spec; // instance data
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

float D_Ashikhmin(float NoH, float linearRoughness)
{
    // Ashikhmin 2007, "Distribution-based BRDFs"
    float a2 = linearRoughness * linearRoughness;
    float cos2h = NoH * NoH;
    float sin2h = max(1.0 - cos2h, 0.0078125); // 2^(-14/2), so sin2h^2 > 0 in fp16
    float sin4h = sin2h * sin2h;
    float cot2 = -cos2h / (a2 * sin2h);
    return 1.0 / (PI * (4.0 * a2 + 1.0) * sin4h) * (4.0 * exp(cot2) + sin4h);
}

float D_Charlie(float NoH, float linearRoughness)
{
    // Estevez and Kulla 2017, "Production Friendly Microfacet Sheen BRDF"
    float invAlpha  = 1.0 / linearRoughness;
    float cos2h = NoH * NoH;
    float sin2h = max(1.0 - cos2h, 0.0078125); // 2^(-14/2), so sin2h^2 > 0 in fp16
    return (2.0 + invAlpha) * pow(sin2h, invAlpha * 0.5) / (2.0 * PI);
}

// product can be VdotH, NdotV or NdotH
vec3 F_Schlick(in float product, in vec3 f0)
{
    return f0 + (vec3(1) - f0) * pow(1.0 - product, 5.0);
    //return mix(f0, vec3(1), pow(1.01 - product, 5.0));
}

float F_Schlick(in float product, in float f0, in float f90)
{
    return f0 + (f90 - f0) * pow(1.0 - product, 5.0);
    //return mix(f0, vec3(1), pow(1.01 - product, 5.0));
}

float V_SmithGGXCorrelated(float NoV, float NoL, float a)
{
    float a2 = a * a;
    float GGXL = NoV * sqrt((-NoL * a2 + NoL) * NoL + a2);
    float GGXV = NoL * sqrt((-NoV * a2 + NoV) * NoV + a2);
    return 0.5 / (GGXV + GGXL);
}

float V_SmithGGXCorrelatedFast(float NdotV, float NdotL, float a)
{
    float GGXV = NdotL * (NdotV * (1.0 - a) + a);
    float GGXL = NdotV * (NdotL * (1.0 - a) + a);
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

// a = linear_roughness
float Fd_Burley(float NdotV, float NdotL, float LdotH, float a)
{
    float f90 = 0.5 + 2.0 * a * LdotH * LdotH;
    float lightScatter = F_Schlick(NdotL, 1.0, f90);
    float viewScatter = F_Schlick(NdotV, 1.0, f90);
    return lightScatter * viewScatter * (1.0 / PI);
}

//
// Cook Torrance Microfacet BSDF
//
vec3 CookTorranceBSDF(
        vec3 diffuse_color, vec3 f0, 
        float linear_roughness, float anisotropy, float clearCoatRoughness, float clearCoat,
        float NdotV, float NdotL, float NdotH, float VdotH, float LdotH,
        vec3 l, vec3 v, vec3 h, vec3 n
        )
{
#ifdef ANISOTROPY
    float at = max(linear_roughness * (1.0 + anisotropy), 0.001);
    float ab = max(linear_roughness * (1.0 - anisotropy), 0.001);
    //vec3 Q1 = dFdx(g_pos);
    //vec3 Q2 = dFdy(g_pos);
    //vec2 st1 = dFdx(IN.uv);
    //vec2 st2 = dFdy(IN.uv);
    //vec3 t = normalize(Q1*st2.t - Q2*st1.t);
    //vec3 b = normalize(-Q1*st2.s + Q2*st1.s);

    vec3 t = vec3(1,0,0); // tangent of tangent space
    vec3 b = vec3(0,1,0); // bitangent
    float D = D_GGX_Anisotropic(NdotH, h, t, b, at, ab);
#else
    float D = D_GGX(NdotH, linear_roughness);
    // CLOTH
    //float D = D_Ashikhmin(NdotH, linear_roughness);
    //float D = D_Charlie(NdotH, linear_roughness);
#endif

    vec3  F = F_Schlick(LdotH, f0);

#ifdef ANISOTROPY
    float TdotV = max( dot( t, v ), 0.0 );
    float BdotV = max( dot( b, v ), 0.0 );
    float TdotL = max( dot( t, l ), 0.0 );
    float BdotL = max( dot( b, l ), 0.0 );
    float V = V_SmithGGXCorrelated_Anisotropic(at, ab, TdotV, BdotV, TdotL, BdotL, NdotV, NdotL);
#else
#    ifdef OPTI_FOR_LOW_END
    float V = V_SmithGGXCorrelatedFast(NdotV, NdotL, linear_roughness);
#    else
    float V = V_SmithGGXCorrelated(NdotV, NdotL, linear_roughness);
#    endif
#endif

    // specular BRDF
    vec3 Fr = (D * V) * F;

    // diffuse BRDF
#ifdef OPTI_FOR_LOW_END
    vec3 Fd = diffuse_color * Fd_Lambert();
#else
    vec3 Fd = diffuse_color * Fd_Burley(NdotV, NdotL, LdotH, linear_roughness);
#endif
    
#ifdef CLEAR_COAT
    // remapping and linearization of clear coat roughness
    clearCoatRoughness = mix(0.045, 0.6, clearCoatRoughness);
    float clearCoatLinearRoughness = clearCoatRoughness * clearCoatRoughness;

    // clear coat BRDF - dielectric clearcoat with reflectance 4%
    float Dc = D_GGX(NdotH, clearCoatLinearRoughness);
    float Vc = V_Kelemen(LdotH);
    vec3  Fc = F_Schlick(LdotH, vec3(0.04)) * clearCoat; // clear coat strength
    vec3 Frc = (Dc * Vc) * Fc;

    // account for energy loss in the base layer
    return ((Fd + Fr * (1.0 - Fc)) * (1.0 - Fc) + Frc);
#else
    return Fd + Fr;//(Fd * (1-F) + Fr);
#endif
}

//
// LIGHTS
//
float square_falloff_attenuation(vec3 d, float lightInvRadius)
{
    float distanceSquare = dot(d, d);
    float factor = distanceSquare * lightInvRadius * lightInvRadius;
    float smoothFactor = max(1.0 - factor * factor, 0.0);
    return (smoothFactor * smoothFactor) / max(distanceSquare, 1e-4);
}

float spot_angle_attenuation(vec3 l, vec3 lightDir, float innerAngle, float outerAngle)
{
    // the scale and offset computations can be done CPU-side
    float cosOuter = cos(outerAngle);
    float spotScale = 1.0 / max(cos(innerAngle) - cosOuter, 1e-4);
    float spotOffset = -cosOuter * spotScale;

    float cd = dot(normalize(-lightDir), l);
    float attenuation = clamp(cd * spotScale + spotOffset, 0.0, 1.0);
    return attenuation * attenuation;
}

//
// MAIN
//

void main() 
{
    float anisotropy = 1.0; // [-1..1]
    float clearCoatRoughness = 0.0; // remapped later
    float clearCoat = 1.0; // fresnel multiplier

    vec4 sampled_base = texture(sampler2D(base_tex, tex_sampler), IN.uv);
    vec4 sampled_spec = texture(sampler2D(spec_tex, tex_sampler), IN.uv);

    vec3 base         = sRGB_to_Linear(IN.base.rgb) * sRGB_to_Linear(sampled_base.rgb);
    float roughness   = sampled_spec.r * IN.spec.x;
    float metallic    = sampled_spec.g < 1e-5 ? IN.spec.y : sampled_spec.g * IN.spec.y;
    float reflectance = sampled_spec.b * IN.spec.z;

    // remappings
    vec3 diffuse_color = mix(base, vec3(0), metallic); // (1-metallic)*base
    reflectance = 0.16 * reflectance * reflectance; // 0..4%..16% for dielectrics
    vec3 f0 = mix(vec3(reflectance), base, metallic); // metal use base as reflectance
    float linear_roughness = roughness * roughness;

    vec3 n = normalize( IN.normal );
    vec3 v = normalize( IN.to_camera );
    
    // double-sided
    float NdotV = dot( n, v );
    if(NdotV < 0)
    {
        n = -n;
        NdotV = -NdotV;
    }
    NdotV += 1e-5;
    //float NdotV = abs( dot( n, v ) ) + 1e-5;
    //float NdotV = ( dot( n, v ) ) + 1e-5;

    vec3 luminance = vec3(0);

    #if 0
    // FOR EACH LIGHT
    for (int i=0; i<8; i++)
    {
        light_t light = scene.lights[i];

        vec3 to_light = light.position.xyz - IN.world_pos;
        vec3 l = normalize( to_light );
        vec3 h = normalize( v + l );
    
        float NdotL = max( dot( n, l ), 0.0 );
        float NdotH = max( dot( n, h ), 0.0 );//1e-5 );
        float VdotH = max( dot( v, h ), 0.0 );//1e-5 );
        float LdotH = max( dot( l, h ), 0.0 );//1e-5 );

        vec3 BSDF = CookTorranceBSDF(
            diffuse_color, f0, 
            linear_roughness, anisotropy, clearCoatRoughness, clearCoat,
            NdotV, NdotL, NdotH, VdotH, LdotH,
            l, v, h, n
            );

        // apply lighting

        vec3 light_color = sRGB_to_Linear(light.color.rgb);
        float light_radius = light.properties.x;
        float light_intensity = light.properties.y;

        bool is_spot = light.direction.w > 0;
        vec3 spot_direction = light.direction.xyz;
        float inner_angle = light.properties.z;
        float outer_angle = light.properties.w;

        // directional
        //vec3 I = light_color; // light intensity, in lux
        //vec3 E = I * NdotL; // illuminance

        // point light
        float I = light_intensity; // = luminous_power / (4*PI)
        float attenuation = square_falloff_attenuation(to_light, 1.0 / light_radius);
        //for spot
        if (is_spot) 
        {
            attenuation *= spot_angle_attenuation(l, spot_direction, inner_angle, outer_angle);
        }
        float E = I * attenuation * NdotL;
        luminance += BSDF * E * light_color;
    }
    #endif

    const vec3 dir_light_dir[6] = {
        vec3(0,1,0),
        vec3(0,-1,0),
        vec3(1,0,0),
        vec3(-1,0,0),
        vec3(0,0,1),
        vec3(0,0,-1),
    };

    const vec4 dir_light_col[6] = {
        vec4(255.0/255.0, 12.0/255.0, 174.0/255.0,0.5), // pink
        vec4(233.0/255.0, 41.0/255.0, 0.0/255.0,0.5),
        vec4(1,0,1,0.5),
        vec4(0,1,0,0.5),
        vec4(0,1,1,0.5),
        vec4(40.0/255.0, 137.0/255.0, 255.0/255.0,0.5),
    };

    //vec3 sky_color = vec3(255.0/255.0, 12.0/255.0, 174.0/255.0); // pink
    //vec3 sky_color = vec3(40.0/255.0, 137.0/255.0, 255.0/255.0); // blue
    //vec3 sky_color = vec3(233.0/255.0, 41.0/255.0, 0.0/255.0); // orange-red

    for(int i=0; i<6; ++i)
    {
        // sky
        vec3 sky_color = sRGB_to_Linear(dir_light_col[i].rgb);
        float sky_intensity = dir_light_col[i].a;

        vec3 Ls = normalize(dir_light_dir[i]);
        vec3 Hs = normalize( v + Ls );
        float NdotLs = max( dot( n, Ls ), 0.0 );
        //float NdotLs = 0.5 * (dot( n, Ls ) + 1); // wrap
        float NdotHs = max( dot( n, Hs ), 0.0 );
        float VdotHs = max( dot( v, Hs ), 0.0 );
        float LsdotHs = max( dot( Ls, Hs ), 0.0 );

        vec3 BSDF_Sky = CookTorranceBSDF(
            diffuse_color, f0, 
            linear_roughness, anisotropy, clearCoatRoughness, clearCoat,
            NdotV, NdotLs, NdotHs, VdotHs, LsdotHs,
            Ls, v, Hs, n
            );
    
        // directional
        float Is = sky_intensity;
        float Es = Is * NdotLs;
        luminance += BSDF_Sky * Es * sky_color;
    }

    // fake envmap
    //vec3 r = reflect( -v, n );
//    float m = 2. * sqrt(pow( r.x, 2. ) + pow( r.y, 2. ) + pow( r.z + 1., 2.0));
//    vec2 vReflectionCoord = r.xy / m + .5;
//    //vReflectionCoord.y = -vReflectionCoord.y;
//
//    sky_color.r = vReflectionCoord.y < 0.5 ? 0.5 : 1.0;
//    sky_color.g = vReflectionCoord.y < 0.5 ? 1.0 : 1.0;
//    sky_color.b = vReflectionCoord.y < 0.5 ? 0.0 : 2.0 * (vReflectionCoord.y - 0.5);
//    float Is = sky_intensity;
//    float Es = Is;// * NdotLs;
//    luminance += BSDF_Sky * Es * sky_color;

    uFragColor = vec4(Linear_to_sRGB(luminance), 1);

    // DEBUG
    //uFragColor = vec4(0.5*(N+1),1);
    //uFragColor = vec4(0.5*(V+1),1);
    //uFragColor = vec4(0.5*(L+1),1);
    //uFragColor = vec4(0.5*(R+1),1);
    //uFragColor = vec4(NdotL, NdotL, NdotL, 1);
}
