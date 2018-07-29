#version 450
#extension GL_ARB_separate_shader_objects : enable

layout( set = 0, binding = 0 ) uniform scene_ubo
{
    mat4 view_matrix;
    mat4 proj_matrix;
	vec4 light_position;
	vec4 light_color;
} Scene_UBO;

layout( set = 1, binding = 0 ) uniform object_ubo
{
    mat4 model_matrix;
} Object_UBO;

layout( location = 0 ) in vec4 pos;
layout( location = 1 ) in vec3 normal;
layout( location = 2 ) in vec2 uv;

layout( location = 0 ) out struct vertex_out {
    vec4 vColor;
	vec4 lColor;
    vec3 normal;
    vec2 uv;
    vec3 to_camera;
    vec3 to_light;
} OUT;

void main() 
{
    mat4 modelView = Scene_UBO.view_matrix * Object_UBO.model_matrix;

    gl_Position = Scene_UBO.proj_matrix * modelView * pos;

    OUT.vColor = vec4( 0, 0.5, 1.0, 1 );
	OUT.lColor = Scene_UBO.light_color;
    OUT.uv = uv;
    OUT.normal = (vec4( normal, 0.0 ) * inverse( modelView )).xyz;

    vec3 camera_pos = vec3( -Scene_UBO.view_matrix[3][0], -Scene_UBO.view_matrix[3][1], -Scene_UBO.view_matrix[3][2] );
    OUT.to_camera = camera_pos - pos.xyz;
    OUT.to_light = Scene_UBO.light_position.xyz - pos.xyz;
}
