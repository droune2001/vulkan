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

layout ( set = 0, binding = 1 ) uniform sampler2D diffuse_tex_sampler;
layout (location = 0) out vec4 uFragColor;

vec3 saturate(vec3 v)
{
	return max(vec3(0.0f), min(vec3(1.0f), v));
}

float saturate(float v)
{
	return max(0.0, min(1.0, v));
}

void main() 
{
	float light_radius = 10.0;
	float dist2 = dot(IN.to_light,IN.to_light);
	float att = saturate(1.0 - dist2/(light_radius*light_radius));
	//att *= att;
	
	
	
    vec3 L = normalize( IN.to_light );
    vec3 V = normalize( IN.to_camera );
    vec3 N = normalize( IN.normal );
	vec3 R = normalize(reflect(L, N));
	
    float NdotL = max( dot( N, L ), 0.0f );
    float NdotV = max( dot( N, V ), 0.0f );
	float RdotV = max( dot( R, V ), 0.0f );
    
	vec3 albedo = texture(diffuse_tex_sampler,IN.uv).rgb;
	
	vec3 diffuse = NdotL * albedo; // IN.vColor.xyz *
	
	float spec = NdotL * pow(RdotV,128);
	
	vec3 light_intensity = att * IN.lColor.xyz;
	
	float NdotUp = max( dot( N, vec3(0,1,0) ), 0.0 );
	vec3 sky_color = vec3(0.39, 0.58, 0.92);
	vec3 sky     = NdotUp * sky_color * albedo;
	
	vec3 ambient = vec3(1,1,1) * albedo;
	
    uFragColor = vec4(saturate(
		0.05 * ambient +
		0.2 * sky + 
		0.75 * light_intensity * ( 1.0 * diffuse + 1.0 * spec )), 
		IN.vColor.a);
}
