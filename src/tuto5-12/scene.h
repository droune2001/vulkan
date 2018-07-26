#ifndef _VULKAN_SCENE_2018_07_20_H_
#define _VULKAN_SCENE_2018_07_20_H_

#include <stdint.h> // uint33_t
#include "glm_usage.h"

#include <array>
#include <vector>

#define MAX_OBJECTS 1024
#define MAX_LIGHTS 8
#define MAX_CAMERAS 16

struct vulkan_context;

class Scene
{
public:
    Scene(vulkan_context *);
    ~Scene();

    struct vertex_t
    {
        glm::vec4 p;
        glm::vec3 n;
        glm::vec2 uv;
    };
    using index_t = uint16_t;

    struct object_description_t
    {
        uint32_t indexCount = 0;
        uint16_t *indices = nullptr;
        uint32_t vertexCount = 0;
        vertex_t *vertices = nullptr;
        glm::mat4 model_matrix;
    };

    struct light_description_t
    {
        glm::vec3 color    = glm::vec3(1,1,1);
        glm::vec3 position = glm::vec3(0,0,0);
    };

    struct camera_description_t
    {
        glm::vec3 position = glm::vec3(10,0,0);
        float fovy = 90.0f;
        float aspect = 4.0f / 3.0f;
        float near_plane = 0.1f;
        float far_plane = 1000.0f;
    };

    bool add_object(object_description_t od);
    bool add_light(light_description_t li);
    bool add_camera(camera_description_t ca);
    
    bool init(); // tmp
    void de_init();
    void update(float dt);
    void draw(VkCommandBuffer cmd, VkViewport viewport, VkRect2D scissor_rect);

private:

    void animate_object(float dt);
    void animate_camera(float dt);

    void draw_object(size_t object_index, VkCommandBuffer cmd);
    void draw_all_objects(VkCommandBuffer cmd);

    void update_scene_ubo();
    void update_object_ubo(size_t i);

    bool create_scene_ubo();
    bool create_object_ubo();



    // SCENE ======================================================
    bool InitUniformBuffer();
    void DeInitUniformBuffer();

    bool InitDescriptors();
    void DeInitDescriptors();

    bool InitFakeImage();
    void DeInitFakeImage();

    bool InitShaders();
    bool CreateShaderModule(const std::string &file_path, VkShaderModule *shader_module);
    void DeInitShaders();

    bool InitGraphicsPipeline();
    void DeInitGraphicsPipeline();

    void update_matrices_ubo();
    // ==============================================================

private:

    vulkan_context *_ctx = nullptr;

    struct uniform_buffer_t
    {
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
    };

    VkDescriptorPool _descriptor_pool = VK_NULL_HANDLE;
    VkDescriptorSetLayout _descriptor_set_layout = VK_NULL_HANDLE;
    VkDescriptorSet _descriptor_set = VK_NULL_HANDLE;

    //
    // OBJECTS
    //

    struct _object_t
    {
        uint32_t vertexCount = 0;
        VkBuffer index_buffer = VK_NULL_HANDLE;
        VkDeviceMemory index_buffer_memory = VK_NULL_HANDLE;

        uint32_t indexCount = 0;
        VkBuffer vertex_buffer = VK_NULL_HANDLE;
        VkDeviceMemory vertex_buffer_memory = VK_NULL_HANDLE;

        glm::mat4 model_matrix = glm::mat4(1);
    };

    std::vector<_object_t> _objects;
    uniform_buffer_t _objects_ubo;
    bool _object_ubo_created = false;

    //
    // LIGHTS 
    //

    struct _light_t
    {
        glm::vec4 color    = glm::vec4(1, 1, 1, 1);
        glm::vec4 position = glm::vec4(0, 0, 0, 1);
    };

    std::vector<_light_t> _lights;
    uniform_buffer_t _lights_ubo;

    //
    // CAMERAS
    //

    struct _camera_t
    {
        glm::mat4 v = glm::mat4(1);; // view matrix
        glm::mat4 p = glm::mat4(1);; // proj matrix
    };

    std::vector<_camera_t> _cameras;
    uniform_buffer_t _cameras_ubo;

    bool _scene_ubo_created = false;

    //
    // MATERIALS
    //

    struct texture
    {
        VkImage         texture_image = VK_NULL_HANDLE;
        VkDeviceMemory  texture_image_memory = {};
        VkImageView     texture_view = VK_NULL_HANDLE;
        VkSampler       sampler = VK_NULL_HANDLE;
    } _checker_texture;

    VkShaderModule _vertex_shader_module = VK_NULL_HANDLE;
    VkShaderModule _fragment_shader_module = VK_NULL_HANDLE;

    std::array<VkPipeline, 1> _pipelines = {};
    VkPipelineLayout _pipeline_layout = VK_NULL_HANDLE;

    //
    // ANIMATION
    //

    struct camera_animation_t
    {
        float cameraZ = 6.0f;
        float cameraZDir = -1.0f;
    } _camera_anim;
};

#endif // _VULKAN_SCENE_2018_07_20_H_
