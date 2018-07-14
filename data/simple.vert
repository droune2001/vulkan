#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout( std140, binding = 0 ) uniform buffer {
    mat4 model_matrix;
    mat4 view_matrix;
    mat4 proj_matrix;
} UBO;

layout( location = 0 ) in vec4 pos;
layout( location = 1 ) in vec3 normal;
layout( location = 2 ) in vec2 uv;

layout( location = 0 ) out struct vertex_out {
    vec4 vColor;
    vec3 normal;
    vec2 uv;
    vec3 camera;
} OUT;

void main() 
{
    mat4 modelView = UBO.model_matrix * UBO.view_matrix;

    gl_Position = pos * ( modelView * UBO.proj_matrix );

    OUT.vColor = vec4( 0, 0.5, 1.0, 1 );
    OUT.uv = uv;
    OUT.normal = (vec4( normal, 0.0 ) * inverse( modelView )).xyz;
    OUT.camera = vec3( UBO.view_matrix[3][1], UBO.view_matrix[3][2], UBO.view_matrix[3][3] );
}
