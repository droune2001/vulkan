#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout( std140, binding = 0 ) uniform buffer {
    mat4 model_matrix;
    mat4 view_matrix;
    mat4 proj_matrix;
} UBO;

layout (location = 0) in vec4 pos;

void main() 
{
    gl_Position = pos * (UBO.model_matrix * UBO.view_matrix * UBO.proj_matrix);
}
