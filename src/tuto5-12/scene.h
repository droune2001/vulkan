#ifndef _VULKAN_SCENE_2018_07_20_H_
#define _VULKAN_SCENE_2018_07_20_H_

#include <stdint.h> // uint33_t
#include "glm_usage.h"

#include <array>
#include <vector>
#include <unordered_map>

#define MAX_OBJECTS 1024
#define MAX_LIGHTS 8
#define MAX_CAMERAS 16

struct vulkan_context;

class Scene
{
public:
    Scene(vulkan_context *);
    ~Scene();

    using material_id_t = std::string;
    using texture_id_t = std::string;

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

        material_id_t material = "default";
        // hardcoded
        texture_id_t diffuse_texture;
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

    struct material_description_t
    {
        std::string id;
        std::string vertex_shader_path;
        std::string fragment_shader_path;
        // a completer
    };

    bool add_object(object_description_t od);
    bool add_light(light_description_t li);
    bool add_camera(camera_description_t ca);
    bool add_material(material_description_t ma);

    bool init(VkRenderPass rp);
    void de_init();
    void update(float dt);
    void draw(VkCommandBuffer cmd, VkViewport viewport, VkRect2D scissor_rect);

private:

    struct uniform_buffer_t
    {
        VkBuffer        buffer = VK_NULL_HANDLE;
        VkDeviceMemory  memory = VK_NULL_HANDLE;
    };

    // VBO/IBO to handle multiple objects.
    struct vertex_buffer_object_t
    {
        uint32_t        offset = 0; // first free byte offset.
        VkBuffer        buffer = VK_NULL_HANDLE;
        VkDeviceMemory  memory = VK_NULL_HANDLE;
    };

    void animate_object(float dt);
    void animate_camera(float dt);

    void draw_object(size_t object_index, VkCommandBuffer cmd);
    void draw_all_objects(VkCommandBuffer cmd);

    bool update_scene_ubo();
    bool update_all_objects_ubos();

    uniform_buffer_t &get_scene_ubo();
    bool create_scene_ubo();
    void destroy_scene_ubo();

    vertex_buffer_object_t & get_global_object_vbo();
    vertex_buffer_object_t & get_global_object_ibo();
    uniform_buffer_t       & get_global_object_ubo();

    bool create_global_object_buffers();
    void destroy_global_object_buffers();
    
    bool create_procedural_textures();
    void destroy_textures();

    bool create_shader_module(const std::string &file_path, VkShaderModule *shader_module);

    bool create_default_material(VkRenderPass rp);
    void destroy_materials();

    bool create_default_descriptor_set_layout();
    bool create_default_pipeline(VkRenderPass rp);

private:

    vulkan_context *_ctx = nullptr;

    //
    // OBJECTS
    //

    struct _object_t
    {
        uint32_t vertexCount = 0;
        uint32_t index_offset = 0;
        VkBuffer index_buffer = VK_NULL_HANDLE; // ref
        
        uint32_t indexCount = 0;
        uint32_t vertex_offset = 0;
        VkBuffer vertex_buffer = VK_NULL_HANDLE; // ref
        
        glm::mat4 model_matrix = glm::mat4(1);

        material_id_t material_id = "default";
        
        // hardcoded
        texture_id_t diffuse_texture;
    };

    std::vector<_object_t> _objects;
    uniform_buffer_t _global_object_ubo; // all objects model matrices in one buffer
    vertex_buffer_object_t _global_object_vbo; // all objects vertices in one buffer
    vertex_buffer_object_t _global_object_ibo; // all objects indices in one buffer
    bool _global_object_ubo_created = false; //
    bool _global_object_vbo_created = false; // lazy creation
    bool _global_object_ibo_created = false; //

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
        glm::mat4 v = glm::mat4(1); // view matrix
        glm::mat4 p = glm::mat4(1); // proj matrix
    };

    std::vector<_camera_t> _cameras;
    uniform_buffer_t _scene_ubo;
    bool _scene_ubo_created = false;

    //
    // MATERIALS
    //

    struct _texture_t
    {
        VkImage         texture_image = VK_NULL_HANDLE;
        VkDeviceMemory  texture_image_memory = {};
        VkImageView     texture_view = VK_NULL_HANDLE;
        VkSampler       sampler = VK_NULL_HANDLE;
    };
    
    std::unordered_map<texture_id_t, _texture_t> _textures;

    struct _material_t
    {
        VkShaderModule vs = VK_NULL_HANDLE;
        VkShaderModule fs = VK_NULL_HANDLE;

        VkDescriptorPool      descriptor_pool = VK_NULL_HANDLE;
        VkDescriptorSetLayout descriptor_set_layouts[2] = { VK_NULL_HANDLE, VK_NULL_HANDLE };
        VkDescriptorSet       descriptor_sets[2] = { VK_NULL_HANDLE, VK_NULL_HANDLE };

        VkPipeline       pipeline = {};
        VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    };

    std::unordered_map<material_id_t, _material_t> _materials;

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
