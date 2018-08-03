#version 450
#extension GL_ARB_separate_shader_objects : enable

layout ( location = 0 ) in struct fragment_in {
    vec4 vColor;
	vec4 lColor;
    vec3 normal;
    vec2 uv;
    vec3 to_camera;
    vec3 to_light;
	// TODO: add radius
} IN;

layout ( set = 0, binding = 1 ) uniform sampler2D tex_sampler;
//layout ( set = 2, binding = 2 ) uniform texture2D diffuse_tex;
//layout ( set = 2, binding = 2 ) uniform texture2D specular_tex;

layout (location = 0) out vec4 uFragColor;




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

void main() 
{
	float light_radius = 10.0;
	float dist2 = dot(IN.to_light,IN.to_light);
	float att = saturate(1.0 - dist2/(light_radius*light_radius));
	att *= att;
	vec3 light_color = att * sRGB_to_Linear(IN.lColor.xyz);
	
	vec3 sky_color = sRGB_to_Linear(vec3(0.39, 0.58, 0.92));
	
    vec3 L = normalize( IN.to_light );
    vec3 V = normalize( IN.to_camera );
	vec3 H = normalize( V + L );
    vec3 N = normalize( IN.normal );
	
    float NdotL = max( dot( N, L ), 0.0 );
    float NdotV = max( dot( N, V ), 0.001 );
	float NdotH = max( dot( N, H ), 0.001 );
	float HdotV = max( dot( H, V ), 0.001 );
	
	//vec3 sampled_diffuse = texture( sampler2D( tex_sampler, diffuse_tex), IN.uv).rgb;
	//vec3 sampled_specular = texture( sampler2D( tex_sampler, specular_tex), IN.uv).rgb;
	
	vec3 base = sRGB_to_Linear(texture(tex_sampler,IN.uv).rgb)* sRGB_to_Linear(IN.vColor.xyz);
	vec3 metallic = vec3(0.0); // TODO: sample from texture channel
	float roughness = 0.2; // TODO: sample from texture channel
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
