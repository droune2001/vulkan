#version 450
#extension GL_ARB_separate_shader_objects : enable

struct light_t
{
    vec4 position;
    vec4 color;
    vec4 direction;
    vec4 properties;
};

layout( set = 0, binding = 0 ) uniform scene_ubo
{
    mat4 view_matrix;
    mat4 proj_matrix;

    vec4 sky_color;

    light_t lights[3];
} Scene_UBO;

// Per-Vertex
layout( location = 0 ) in vec4 pos;
layout( location = 1 ) in vec3 normal;
layout( location = 2 ) in vec2 uv;

// Per-Instance
layout( location = 3 ) in mat4 model_matrix;
layout( location = 7 ) in vec4 base; // xyz = albedo or specular. a = alpha
layout( location = 8 ) in vec4 spec; // x = roughness, y = metallic, z = reflectance

// OUT
layout( location = 0 ) out struct vertex_out 
{
    vec3 normal;
    vec2 uv;
    vec3 to_camera;
    vec3 world_pos;
    vec4 base; // pass through instance data
    vec4 spec; // pass through instance data
} OUT;

void main() 
{
    vec4 world_pos = model_matrix * pos;
    mat4 modelView = Scene_UBO.view_matrix * model_matrix;
    vec4 camera_pos = inverse(Scene_UBO.view_matrix) * vec4(0,0,0,1);
    // use inv view matrix last column/row. 
    //vec3 camera_pos = vec3( -Scene_UBO.view_matrix[3][0], -Scene_UBO.view_matrix[3][1], -Scene_UBO.view_matrix[3][2] );

    gl_Position = Scene_UBO.proj_matrix * modelView * pos;

    OUT.uv = uv;
    OUT.normal = normal;//( inverse( transpose( modelView ) ) * vec4( normal, 0.0 )).xyz;
    OUT.to_camera = camera_pos.xyz - world_pos.xyz;
    OUT.world_pos = world_pos.xyz;
    OUT.base = base;
    OUT.spec = spec;
}
