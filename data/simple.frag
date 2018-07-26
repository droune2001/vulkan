#version 450
#extension GL_ARB_separate_shader_objects : enable

layout ( location = 0 ) in struct fragment_in {
    vec4 vColor;
    vec3 normal;
    vec2 uv;
    vec3 to_camera;
    vec3 to_light;
} IN;

layout ( set = 0, binding = 1 ) uniform sampler2D diffuse_tex_sampler;
layout (location = 0) out vec4 uFragColor;

void main() 
{
    vec3 L = normalize( IN.to_light );
    vec3 V = normalize( IN.to_camera );
    vec3 N = normalize( IN.normal );

    float NdotL = max( dot( N, L ), 0.0f );
    float NdotV = max( dot( N, V ), 0.0f );
    vec4 Cd = vec4(texture(diffuse_tex_sampler,IN.uv).rgb, 1.0f);
    float spec = pow(NdotV,128);
    uFragColor = IN.vColor * NdotL * Cd + spec;
}
