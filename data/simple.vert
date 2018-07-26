#version 420
#extension GL_ARB_separate_shader_objects : enable

// mot cle "buffer" ne semble plus exister apres la version 420 de glsl
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
    vec3 to_camera;
    vec3 to_light;
} OUT;

void main() 
{
    vec3 light_pos = vec3(1,1,1);

    mat4 modelView = UBO.view_matrix * UBO.model_matrix;

    gl_Position = UBO.proj_matrix * modelView * pos;

    OUT.vColor = vec4( 0, 0.5, 1.0, 1 );
    OUT.uv = uv;
    OUT.normal = (vec4( normal, 0.0 ) * inverse( modelView )).xyz;


    vec3 camera_pos = vec3( -UBO.view_matrix[3][0], -UBO.view_matrix[3][1], -UBO.view_matrix[3][2] );
    OUT.to_camera = camera_pos - pos.xyz;
    OUT.to_light = light_pos - pos.xyz;
}
