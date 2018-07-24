#pragma once

#include "vk_mem_alloc_usage.h"

#include <vector>
#include <array>

class Window;
class Scene;

struct vulkan_queue
{
    VkQueue         queue = VK_NULL_HANDLE;
    uint32_t        family_index = UINT32_MAX;
    VkCommandPool   command_pool = VK_NULL_HANDLE;
    VkCommandBuffer command_buffer = VK_NULL_HANDLE; // maybe many
};

struct vulkan_context
{
    VkInstance       instance = VK_NULL_HANDLE; // ou nullptr, selon la version de vulkan
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    VkDevice         device = VK_NULL_HANDLE;
    
    VkPhysicalDeviceProperties       physical_device_properties = {};
    VkPhysicalDeviceMemoryProperties physical_device_memory_properties = {};

    vulkan_queue graphics = {};
    vulkan_queue compute  = {};
    vulkan_queue transfer = {};
    vulkan_queue present  = {};

    std::vector< const char * > instance_layers;
    std::vector< const char * > instance_extensions;
    std::vector< const char * > device_layers; // deprecated
    std::vector< const char * > device_extensions;

    VkDebugReportCallbackEXT    debug_report = VK_NULL_HANDLE;

    // keep it in here to be able to give it to VkCreateInstance
    VkDebugReportCallbackCreateInfoEXT debug_callback_create_info = {};
};

class Renderer
{
public:
    Renderer(Window *w);
    ~Renderer();

    // Inits vulkan context and window-vulkan specifics
    bool InitContext();
    bool InitSceneVulkan();
    void DeInitSceneVulkan();
    void AddScene(Scene *scene) { _scenes.push_back(scene); }
    void Draw(float dt);

    vulkan_context *context() { return &_ctx; };

private:
    bool InitInstance();
    void DeInitInstance();

    bool InitDevice();
        bool ChoosePhysicalDevice();
            bool IsDeviceSuitable(VkPhysicalDevice dev);
        bool SelectQueueFamilyIndices();
        bool EnumerateInstanceLayers();
        bool EnumerateDeviceLayers();
        bool EnumerateDeviceExtensions();
        bool CreateLogicalDevice();
    void DeInitDevice();

    void SetupDebug();
    bool InitDebug();
    void DeInitDebug();

    void SetupLayers();
    void SetupExtensions();

    bool InitVma();
    void DeInitVma();

    //
    // 
    //

    bool InitCommandBuffer();
    void DeInitCommandBuffer();

    bool InitDepthStencilImage();
    void DeInitDepthStencilImage();

    bool InitRenderPass();
    void DeInitRenderPass();

    bool InitSwapChainFrameBuffers();
    void DeInitSwapChainFrameBuffers();

    bool InitUniformBuffer();
    void DeInitUniformBuffer();

    bool InitDescriptors();
    void DeInitDescriptors();

    bool InitFakeImage();
    void DeInitFakeImage();

    bool InitShaders();
    void DeInitShaders();

    bool InitGraphicsPipeline();
    void DeInitGraphicsPipeline();

    //
    //
    //

    void set_object_position(float, float, float);
    void set_camera_position(float, float, float);
    void update_matrices_ubo();

private:

    vulkan_context _ctx;
    VmaAllocator   _allocator = VK_NULL_HANDLE;

    Window * _w = nullptr; // ref

    VkExtent2D _global_viewport = {512, 512};

    std::vector<Scene*> _scenes;


    std::vector<VkFramebuffer> _swapchain_framebuffers;

    VkImage _depth_stencil_image = {};
    VkDeviceMemory _depth_stencil_image_memory = VK_NULL_HANDLE;
    VkImageView _depth_stencil_image_view = VK_NULL_HANDLE;
    VkFormat _depth_stencil_format = VK_FORMAT_UNDEFINED;
    bool _stencil_available = false;

    VkRenderPass _render_pass = VK_NULL_HANDLE;

    VkDescriptorPool _descriptor_pool = VK_NULL_HANDLE;
    VkDescriptorSetLayout _descriptor_set_layout = VK_NULL_HANDLE;
    VkDescriptorSet _descriptor_set = VK_NULL_HANDLE;

    // ==== MATERIAL =======
    VkShaderModule _vertex_shader_module = VK_NULL_HANDLE;
    VkShaderModule _fragment_shader_module = VK_NULL_HANDLE;

    std::array<VkPipeline, 1> _pipelines = {};
    VkPipelineLayout _pipeline_layout = VK_NULL_HANDLE;
    // =====================

    struct matrices
    {
        float m[16];
        float v[16];
        float p[16];
    } _mvp;

    struct uniform_buffer
    {
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
    } _matrices_ubo;

    struct texture
    {
        VkImage         texture_image = VK_NULL_HANDLE;
        VkDeviceMemory  texture_image_memory = {};
        VkImageView     texture_view = VK_NULL_HANDLE;
        VkSampler       sampler = VK_NULL_HANDLE;
    } _checker_texture;

	// "Scene"
    struct camera
    {
        float cameraZ = 10.0f;
        float cameraZDir = -1.0f;
    } _camera;
};
