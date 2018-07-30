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
	float light_radius = 5.0;
	float dist2 = dot(IN.to_light,IN.to_light);
	vec3 sky_color = vec3(0.39, 0.58, 0.92);
	
	float att = saturate(1.0 - dist2/(light_radius*light_radius));
	//att *= att;
	
    vec3 L = normalize( IN.to_light );
    vec3 V = normalize( IN.to_camera );
    vec3 N = normalize( IN.normal );
	vec3 R = normalize(reflect(L, N));
	
	float NdotUp = max( dot( N, vec3(0,1,0) ), 0.0 );
    float NdotL = max( dot( N, L ), 0.0f );
    float NdotV = max( dot( N, V ), 0.0f );
	float RdotV = max( dot( R, V ), 0.0f );
    vec3 diffuse_color = texture(diffuse_tex_sampler,IN.uv).rgb;
	vec3 diffuse = IN.vColor.xyz * NdotL * diffuse_color;
	vec3 sky = NdotUp * sky_color;
    float spec = pow(RdotV,128);
    uFragColor = vec4(saturate(0.2 * sky + att * IN.lColor.xyz * ( 0.7 * diffuse + 0.5 * spec )), IN.vColor.a);
}
