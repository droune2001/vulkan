#version 450
#extension GL_ARB_separate_shader_objects : enable

struct light_t
{
    vec4 position;
    vec4 color;
    vec4 direction;
    vec4 properties;
};

layout( set = 0, binding = 0 ) uniform subo
{
    mat4 view;
    mat4 proj;

    vec4 sky_color;

    light_t lights[8];
} scene;

// Per-Vertex
layout( location = 0 ) in vec4 v_pos;
layout( location = 1 ) in vec3 normal;
layout( location = 2 ) in vec2 uv;

// Per-Instance
layout( location = 3 ) in vec4 i_position;
layout( location = 4 ) in vec4 i_rotation;
layout( location = 5 ) in vec4 i_scale;
layout( location = 6 ) in vec4 i_speed;
layout( location = 7 ) in vec4 i_jitter;
layout( location = 8 ) in vec4 i_base; // xyz = albedo or specular. a = alpha
layout( location = 9 ) in vec4 i_spec; // x = roughness, y = metallic, z = reflectance

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

mat4 rebuild_matrix(vec4 p, vec3 r, vec3 s)
{
    mat4 m = mat4(1.0);

    // position
    m[3] = p;

    float cx = cos(r.x);
    float sx = sin(r.x);
    float cy = cos(r.y);
    float sy = sin(r.y);
    float cz = cos(r.z);
    float sz = sin(r.z);

    mat3 rot_matrix_x = mat3(
        vec3(1,0,0), 
        vec3(0,cx,sx), 
        vec3(0,-sx,cx)
    );

    mat3 rot_matrix_y = mat3(
        vec3(cy,0,-sy), 
        vec3(0,1,0), 
        vec3(sy,0,cy)
    );

    mat3 rot_matrix_z = mat3(
        vec3(cz,sz,0), 
        vec3(-sz,cz,0), 
        vec3(0,0,1)
    );

    mat4 rot_mat = mat4(mat3(rot_matrix_z * rot_matrix_y * rot_matrix_x));

    // rotation
    m *= rot_mat;

    // scale
    m[0][0] *= s.x;
    m[1][1] *= s.y;
    m[2][2] *= s.z;

    return m;
}

void main() 
{
    mat4 model_matrix = rebuild_matrix(i_position, i_rotation.xyz, i_scale.xyz);
    vec4 world_pos = model_matrix * v_pos;
    mat4 model_view = scene.view * model_matrix;
    vec4 camera_pos = inverse(scene.view) * vec4(0,0,0,1);

    gl_Position = scene.proj * model_view * v_pos;

    OUT.uv = uv;
    OUT.normal = (inverse(transpose(model_view)) * vec4(normal, 0.0)).xyz;
    OUT.to_camera = camera_pos.xyz - world_pos.xyz;
    OUT.world_pos = world_pos.xyz;
    OUT.base = i_base;
    OUT.spec = i_spec;
}
