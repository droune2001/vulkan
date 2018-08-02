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

void main() 
{
	float light_radius = 10.0;
	float dist2 = dot(IN.to_light,IN.to_light);
	float att = saturate(1.0 - dist2/(light_radius*light_radius));
	att *= att;
	vec3 light_diffuse_intensty = att * sRGB_to_Linear(IN.lColor.xyz);
	vec3 light_specular_intensty = light_diffuse_intensty;
	
    vec3 L = normalize( IN.to_light );
    vec3 V = normalize( IN.to_camera );
	vec3 H = normalize( V + L );
    vec3 N = normalize( IN.normal );
	vec3 R = normalize( reflect(L, N) );
	
    float NdotL = max( dot( N, L ), 0.0 );
    float NdotV = max( dot( N, V ), 0.001 );
	float NdotH = max( dot( N, H ), 0.001 );
	
	vec3 mat_diffuse_reflectance = sRGB_to_Linear(texture(diffuse_tex_sampler,IN.uv).rgb)* sRGB_to_Linear(IN.vColor.xyz);
	const float mat_shininess = 1024.0;
	const float energy_conservation = (8*mat_shininess)/(8*3.14159);
	vec3 mat_specular_reflectance = energy_conservation * vec3(1,1,1);
	
	vec3 diffuse = NdotL * mat_diffuse_reflectance * light_diffuse_intensty;
	vec3 specular = NdotL * pow(NdotH,mat_shininess) * mat_specular_reflectance * light_specular_intensty;
	
	float NdotUp = max( dot( N, vec3(0,1,0) ), 0.0 );
	vec3 sky_color = sRGB_to_Linear(vec3(0.39, 0.58, 0.92));
	vec3 sky = NdotUp * sky_color * mat_diffuse_reflectance;
	
	vec3 ambient = vec3(1,1,1) * mat_diffuse_reflectance;
	
	vec3 out_color = Linear_to_sRGB(saturate(
		//0.05 * ambient +
		0.2 * sky + 
		1.0 * diffuse + 
		1.0 * specular
		));
		
    uFragColor = vec4(out_color, IN.vColor.a);
	
	//uFragColor = vec4(0.5*(N+1),1);
	//uFragColor = vec4(0.5*(V+1),1);
	//uFragColor = vec4(0.5*(L+1),1);
	//uFragColor = vec4(0.5*(R+1),1);
	//uFragColor = vec4(NdotL, NdotL, NdotL, 1);
}
