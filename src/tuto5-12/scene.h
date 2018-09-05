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

#ifndef PI 
#   define PI 3.1415f
#   define PI_4 (PI/4.0f)
#   define PI_5 (PI/5.0f)
#endif

struct vulkan_context;
struct vulkan_queue;

class Scene
{
public:
    Scene(vulkan_context *);
    ~Scene();

    using object_id_t = std::string;
    using material_id_t = std::string;
    using material_instance_id_t = std::string;
    using texture_id_t = std::string;

    struct vertex_t
    {
        glm::vec4 p;
        glm::vec3 n;
        glm::vec2 uv;

        static uint32_t binding_description_count() { return 1; }
        static VkVertexInputBindingDescription * binding_descriptions()
        {
            static VkVertexInputBindingDescription vertex_binding_description = {};
            vertex_binding_description.binding = 0;
            vertex_binding_description.stride = sizeof(Scene::vertex_t);
            vertex_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return &vertex_binding_description;
        }

        static uint32_t attribute_description_count() { return 3; }
        static VkVertexInputAttributeDescription * attribute_descriptions()
        {
            static std::array<VkVertexInputAttributeDescription, 3> vertex_attribute_description = {};
            vertex_attribute_description[0].location = 0;
            vertex_attribute_description[0].binding = 0;
            vertex_attribute_description[0].format = VK_FORMAT_R32G32B32A32_SFLOAT; // position = 4 float
            vertex_attribute_description[0].offset = offsetof(Scene::vertex_t, p); //0;

            vertex_attribute_description[1].location = 1;
            vertex_attribute_description[1].binding = 0;
            vertex_attribute_description[1].format = VK_FORMAT_R32G32B32_SFLOAT; // normal = 3 floats
            vertex_attribute_description[1].offset = offsetof(Scene::vertex_t, n);//4 * sizeof(float); 

            vertex_attribute_description[2].location = 2;
            vertex_attribute_description[2].binding = 0;
            vertex_attribute_description[2].format = VK_FORMAT_R32G32_SFLOAT; // uv = 2 floats
            vertex_attribute_description[2].offset = offsetof(Scene::vertex_t, uv); // (4 + 3) * sizeof(float);

            return vertex_attribute_description.data();
        }
    };
    using index_t = uint16_t;

    struct object_description_t
    {
        object_id_t name = "";

        uint32_t indexCount = 0;
        uint16_t *indices = nullptr;
        uint32_t vertexCount = 0;
        vertex_t *vertices = nullptr;

        // for each instance
        glm::vec3 position = glm::vec3(0, 0, 0);

        material_instance_id_t material = "white_rough"; // material parameters set

                                                         // material overrides.
        glm::vec4 base_color = glm::vec4(0.5, 0.5, 0.5, 1.0);
        glm::vec4 specular = glm::vec4(0.5, 0.0, 0.0, 0.0); // roughness, metallic, 0, 0
    };

    struct light_description_t
    {
        enum { POINT_LIGHT_TYPE, CONE_LIGHT_TYPE } type = POINT_LIGHT_TYPE;
        glm::vec3 position = glm::vec3(0, 0, 0);
        glm::vec3 color = glm::vec3(1, 1, 1);
        glm::vec3 direction = glm::vec3(0, -1, 0);
        float     radius = 10.0f;
        float     intensity = 1.0f;
        float     inner = PI_5;
        float     outer = PI_4;
    };

    using view_id_t = std::string;
    using camera_id_t = std::string;
    struct camera_description_t
    {
        camera_id_t camera_id;
        glm::vec3 position = glm::vec3(10, 0, 0);
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

    // TODO: make it an abstract class and derive for each material
    struct material_instance_description_t
    {
        material_id_t material_id;
        material_instance_id_t instance_id;

        texture_id_t base_tex = "default"; // diffuse or specular, depending on metalness.
        texture_id_t specular_tex = "default_spec"; // x = roughness, y = metallic
        glm::vec3 diffuse_color = glm::vec3(1, 1, 1);
        float roughness = 0.1f;
        float metalness = 0.0f;
    };

    bool add_object(object_description_t od);
    bool add_light(light_description_t li);
    bool add_camera(camera_description_t ca);
    bool add_material(material_description_t ma);
    bool add_material_instance(material_instance_description_t mi);

    bool init(VkRenderPass rp);
    void de_init();
    bool compile(); // create descriptor sets once all ythe scene is built.
    void update(float dt);
    void upload();
    void draw(VkCommandBuffer cmd, VkViewport viewport, VkRect2D scissor_rect);

private:

    struct staging_buffer_t
    {
        VkBuffer        buffer = VK_NULL_HANDLE;
        VkDeviceMemory  memory = VK_NULL_HANDLE;
        // TODO: store reserved size
    };

    struct uniform_buffer_t
    {
        VkBuffer        buffer = VK_NULL_HANDLE;
        VkDeviceMemory  memory = VK_NULL_HANDLE;
    };

    struct dynamic_uniform_buffer_t
    {
        void *          host_data = nullptr;
        size_t          alignment = 0;
        size_t          size = 0;
        VkBuffer        buffer = VK_NULL_HANDLE;
        VkDeviceMemory  memory = VK_NULL_HANDLE;
    };

    // VBO/IBO to handle multiple objects.
    struct vertex_buffer_object_t
    {
        uint32_t        offset = 0; // first free byte offset.
        VkBuffer        buffer = VK_NULL_HANDLE;
        VkDeviceMemory  memory = VK_NULL_HANDLE;
        // TODO: store reserved size
    };

    void show_property_sheet();
    void animate_camera(float dt);
    void animate_light(float dt);
    void animate_object(float dt);

    bool update_scene_ubo();
    bool update_all_objects_ubos();

    uniform_buffer_t &get_scene_ubo();
    bool create_scene_ubo();
    void destroy_scene_ubo();

    vertex_buffer_object_t & get_global_staging_vbo();
    vertex_buffer_object_t & get_global_object_vbo();
    vertex_buffer_object_t & get_global_object_ibo();
    dynamic_uniform_buffer_t & get_global_object_matrices_ubo();
    dynamic_uniform_buffer_t & get_global_object_material_ubo();

    bool create_global_object_buffers();
    void destroy_global_object_buffers();

    bool create_procedural_textures();
    bool create_texture_samplers();
    void destroy_textures();

    bool create_shader_module(const std::string &file_path, VkShaderModule *shader_module);

    bool build_pipelines(VkRenderPass rp);
    void destroy_materials();

    bool create_all_descriptor_set_layouts(VkDevice device, VkDescriptorSetLayout *layouts);
    bool create_all_descriptor_sets_pool();
    bool create_all_descriptor_sets();

    // utils

    uint32_t find_memory_type(uint32_t memory_type_bits, VkMemoryPropertyFlags desired_memory_flags);
    bool create_buffer(
        VkBuffer *pBuffer,                          // [out]
        VkDeviceMemory *pBufferMemory,              // [out]
        VkDeviceSize size,                          // [in]
        VkBufferUsageFlags usage_flags,             // [in]
        VkMemoryPropertyFlags memory_property_flags // [in]
    );
    bool copy_buffer_to_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size, VkDeviceSize src_offset = 0, VkDeviceSize dst_offset = 0);
    bool copy_buffer_to_image(VkBuffer src, VkImage dst, VkExtent3D extent);
    bool transition_texture(VkImage *pImage, VkImageLayout old_layout, VkImageLayout new_layout);
    bool copy_data_to_staging_buffer(staging_buffer_t buffer, void *data, VkDeviceSize size);

    VkCommandBuffer begin_single_time_commands(const vulkan_queue &queue);
    void end_single_time_commands(VkCommandBuffer cmd, const vulkan_queue &queue);

    // tmp
    void tmp_change_sphere_base_color(int idx, const glm::vec4 &base_color);
    void tmp_change_sphere_spec_color(int idx, const glm::vec4 &spec_color);

private:

    vulkan_context * _ctx = nullptr;

    bool _animate_camera = true;
    bool _animate_light = true;
    bool _animate_object = true;
    int _current_item_idx = 0;
    int _current_light = 0;

    //
    // OBJECTS
    //

    struct _material_override_t
    {
        glm::vec4 base_color = glm::vec4(0.5, 0.5, 0.5, 1.0);
        glm::vec4 specular = glm::vec4(1, 1, 0, 0); // roughness, metallic, 0, 0
    };

    _material_override_t *get_object_material(int obj_idx);
    glm::vec4 get_object_base_color(int idx);
    glm::vec4 get_object_spec_color(int idx);

    struct _object_t
    {
        uint32_t vertexCount = 0;
        uint32_t index_offset = 0;
        VkBuffer index_buffer = VK_NULL_HANDLE; // ref

        uint32_t indexCount = 0;
        uint32_t vertex_offset = 0;
        VkBuffer vertex_buffer = VK_NULL_HANDLE; // ref

                                                 // for animation
        glm::vec3 position = glm::vec3(0, 0, 0);
        glm::vec4 base_color = glm::vec4(0.5, 0.5, 0.5, 1.0);
        glm::vec4 specular = glm::vec4(1, 1, 0, 0); // roughness, metallic, 0, 0

        material_instance_id_t material_ref;
    };

    // set #2 binding #0 model_matrix       : VS
    //        binding #1 material overrides : FS
    VkDescriptorSet _global_objects_descriptor_set = VK_NULL_HANDLE;

    // TODO: map, with name and index in global buffers.
    std::vector<_object_t> _objects;
    std::vector<object_id_t> _object_names = {};

    struct _instanced_object_t
    {
        uint32_t vertexCount = 0;
        uint32_t index_offset = 0;
        VkBuffer index_buffer = VK_NULL_HANDLE; // ref

        uint32_t indexCount = 0;
        uint32_t vertex_offset = 0;
        VkBuffer vertex_buffer = VK_NULL_HANDLE; // ref

        VkBuffer instance_buffer = VK_NULL_HANDLE;

        uint32_t instance_count = 0;

        #define MAX_INSTANCE_COUNT 256

        // for animation
        std::array<glm::vec3, MAX_INSTANCE_COUNT> positions = {};
        std::array<glm::vec4, MAX_INSTANCE_COUNT> base_colors = {}; // glm::vec4(0.5, 0.5, 0.5, 1.0);
        std::array<glm::vec4, MAX_INSTANCE_COUNT> speculars = {}; // glm::vec4(1, 1, 0, 0); // roughness, metallic, 0, 0

        material_instance_id_t material_ref; // same material for all objects in the instance set.
    };

    std::vector<_instanced_object_t> _instanced_objects;


    void *get_aligned(dynamic_uniform_buffer_t *buffer, uint32_t idx);
    dynamic_uniform_buffer_t _global_object_matrices_ubo; // all objects model matrices in one buffer
    dynamic_uniform_buffer_t _global_object_material_ubo; // all objects material overrides in one buffer
    vertex_buffer_object_t _global_object_vbo; // all objects vertices in one buffer
    vertex_buffer_object_t _global_object_ibo; // all objects indices in one buffer
    vertex_buffer_object_t _global_staging_vbo; // used for transfer.
    bool _global_object_matrices_ubo_created = false; //
    bool _global_object_material_ubo_created = false; //
    bool _global_object_vbo_created = false;          // for lazy creation
    bool _global_object_ibo_created = false;          //
    bool _global_staging_vbo_created = false;         //

    //
    // LIGHTS 
    //

    struct _light_t
    {
        // TODO: do I still need to pass this to the VS?
        // VS
        glm::vec4 position = glm::vec4(0, 0, 0, 1);
        // FS
        glm::vec4 color = glm::vec4(1, 1, 1, 1);
        glm::vec4 direction = glm::vec4(0, -1, 0, 0); // cone direction, w == 0 if point light, 1 if cone
        glm::vec4 properties = glm::vec4(10.0f, 0.0f, PI_5, PI_4); // x = radius, y = intensity (0 if inactive), z = inner angle, w = outer angle
    };

    #define MAX_LIGHTS_PER_SHADER 8
    struct _lighting_block_t
    {
        glm::vec4 sky_color = glm::vec4(214 / 255.0f, 224 / 255.0f, 255 / 255.0f, 0.3f); // RGB: color A:intensity
        //glm::vec4 sky_color    = glm::vec4(0.39, 0.58, 0.92, 1);

        std::array<_light_t, MAX_LIGHTS_PER_SHADER> lights;
    } _lighting_block;

    std::vector<_light_t> _lights;
    //uniform_buffer_t _lights_ubo;

    //
    // CAMERAS
    //

    struct _camera_t
    {
        glm::mat4 v = glm::mat4(1); // view matrix
        glm::mat4 p = glm::mat4(1); // proj matrix
    };
    std::unordered_map<camera_id_t, _camera_t> _cameras;

    //
    // VIEW / SCENE
    //

    struct _view_t
    {
        camera_id_t camera = "perspective";

        // for the moment, it is the "scene"
        // set = 0
        // binding = 0: camera position, light position : VS
        // binding = 1: light color, sky_color          : FS
        // binding = 2: tex sampler                     : FS
        VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
    };

    std::unordered_map<view_id_t, _view_t> _views;
    uniform_buffer_t _scene_ubo;
    bool _scene_ubo_created = false;

    //
    // MATERIALS
    //

    struct _texture_t
    {
        VkImage         image = VK_NULL_HANDLE;
        VkDeviceMemory  image_memory = {};
        VkImageView     view = VK_NULL_HANDLE;
        VkFormat        format = VK_FORMAT_UNDEFINED;
        VkExtent3D      extent = { 0,0,0 };
        // NOTE: we can put a descriptor in each texture.
        // atm, we chose to put it in material instances,
        // because we group together base+spec textures in
        // a single set with predefined bindings.
    };

    bool create_texture_2d(_texture_t *texture);
    bool transition_textures();

    std::unordered_map<texture_id_t, _texture_t> _textures;
    staging_buffer_t _texture_staging_buffer;

    std::array<VkSampler, 1> _samplers;

    // for all descriptors, 1000 of each type.
    VkDescriptorPool _descriptor_pool = VK_NULL_HANDLE;

    // All descriptor sets layouts
    enum
    {
        SCENE_DESCRIPTOR_SET_LAYOUT = 0,
        MATERIAL_DESCRIPTOR_SET_LAYOUT,
        OBJECT_DESCRIPTOR_SET_LAYOUT,

        DESCRIPTOR_SET_LAYOUT_COUNT
    };
    std::array<VkDescriptorSetLayout, DESCRIPTOR_SET_LAYOUT_COUNT> _descriptor_set_layouts = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE };

    VkPipeline       _instance_pipeline = VK_NULL_HANDLE;
    VkPipelineLayout _instance_pipeline_layout = VK_NULL_HANDLE;

    struct _material_t
    {
        VkShaderModule vs = VK_NULL_HANDLE;
        VkShaderModule fs = VK_NULL_HANDLE;

        VkPipeline       pipeline = VK_NULL_HANDLE;
        VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    };

    std::unordered_map<material_id_t, _material_t> _materials;


    struct _material_instance_t
    {
        texture_id_t base_tex;
        texture_id_t spec_tex;
        VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
        // set = 1 binding = 0 texture2d base;
        //         binding = 1 texture2d spec;
    };
    std::unordered_map<material_instance_id_t, _material_instance_t> _material_instances;
};

#endif // _VULKAN_SCENE_2018_07_20_H_
