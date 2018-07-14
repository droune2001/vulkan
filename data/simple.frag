#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout ( location = 0 ) in struct fragment_in {
    vec4 vColor;
    vec3 normal;
    vec2 uv;
    vec3 camera;
} IN;

layout (location = 0) out vec4 uFragColor;

void main() 
{
    vec3 L = normalize( IN.camera );
    float NdotL = max( dot( IN.normal, L ), 0.0f );
    uFragColor = NdotL * IN.vColor;
}
