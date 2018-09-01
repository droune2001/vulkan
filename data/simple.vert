#version 450
#extension GL_ARB_separate_shader_objects : enable

struct light_t
{
    vec4 position;
    vec4 color;
    vec4 radius;
};

layout( set = 0, binding = 0 ) uniform scene_ubo
{
    mat4 view_matrix;
    mat4 proj_matrix;

    vec4 sky_color;

    light_t lights[];
} Scene_UBO;

layout( set = 2, binding = 0 ) uniform object_ubo
{
    mat4 model_matrix;
} Object_UBO;

layout( location = 0 ) in vec4 pos;
layout( location = 1 ) in vec3 normal;
layout( location = 2 ) in vec2 uv;

layout( location = 0 ) out struct vertex_out 
{
    // vec4 vColor;
    // vec4 lColor;
    vec3 normal;
    vec2 uv;
    vec3 to_camera;
    //vec3 to_light;
    vec3 world_pos;
} OUT;

void main() 
{
    vec4 world_pos = Object_UBO.model_matrix * pos;

    mat4 modelView = Scene_UBO.view_matrix * Object_UBO.model_matrix;

    gl_Position = Scene_UBO.proj_matrix * modelView * pos;

//    OUT.vColor = vec4( 1, 1, 1, 1 ); // TODO: add vertex color in attribs
//    OUT.lColor = Scene_UBO.light_color;
    OUT.uv = uv;
    OUT.normal = normal;//( inverse( transpose( modelView ) ) * vec4( normal, 0.0 )).xyz;

    vec4 camera_pos = inverse(Scene_UBO.view_matrix) * vec4(0,0,0,1);
    // use inv view matrix last column/row. 
    //vec3 camera_pos = vec3( -Scene_UBO.view_matrix[3][0], -Scene_UBO.view_matrix[3][1], -Scene_UBO.view_matrix[3][2] );

    // TODO: pass world_pos to fragment shader.
    OUT.to_camera = camera_pos.xyz - world_pos.xyz;
    //OUT.to_light = Scene_UBO.lights[0].position.xyz - world_pos.xyz;
    OUT.world_pos = world_pos.xyz;
}
