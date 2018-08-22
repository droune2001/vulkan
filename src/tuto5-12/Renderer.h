#pragma once

#include "vk_mem_alloc_usage.h"

#include <vector>
#include <array>

class Window;
class Scene;

constexpr uint32_t MAX_PARALLEL_FRAMES = 3;

struct vulkan_queue
{
    VkQueue         queue = VK_NULL_HANDLE;
    uint32_t        family_index = UINT32_MAX;
    VkCommandPool   command_pool = VK_NULL_HANDLE;
    std::array<VkCommandBuffer, MAX_PARALLEL_FRAMES> command_buffers = {}; // maybe many
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
    VkPhysicalDeviceFeatures    features = {};

    VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;

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
    void SetScene(Scene *scene) { _scene = scene; }
    void Draw(float dt);

    vulkan_context *context() { return &_ctx; };
    VkRenderPass render_pass() { return _render_pass; }

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
    void SetupFeatures();

    bool InitVma();
    void DeInitVma();

    //
    // SCENE from a global point of view
    // - acquire
    // - call render on scene
    // - blit the fbo into swapchain
    // - present swapchain
    //

    bool InitCommandBuffer();
    void DeInitCommandBuffer();

    bool InitDepthStencilImage();
    void DeInitDepthStencilImage();

    bool InitRenderPass();
    void DeInitRenderPass();

    bool InitSwapChainFrameBuffers();
    void DeInitSwapChainFrameBuffers();

    bool InitSynchronizations();
    void DeInitSynchronizations();

    bool InitDescriptorPool();
    void DeInitDescriptorPool();

private:

    vulkan_context _ctx;
    VmaAllocator   _allocator = VK_NULL_HANDLE;

    Window * _w = nullptr;
    Scene  * _scene = nullptr;

    VkExtent2D _global_viewport = {512, 512};

    std::vector<VkFramebuffer> _swapchain_framebuffers;

    VkImage _depth_stencil_image = {};
    VkDeviceMemory _depth_stencil_image_memory = VK_NULL_HANDLE;
    VkImageView _depth_stencil_image_view = VK_NULL_HANDLE;
    VkFormat _depth_stencil_format = VK_FORMAT_UNDEFINED;
    bool _stencil_available = false;

    VkRenderPass _render_pass = VK_NULL_HANDLE;

    uint32_t current_frame = 0;
    std::array<VkSemaphore, MAX_PARALLEL_FRAMES> _render_complete_semaphores = {};
    std::array<VkSemaphore, MAX_PARALLEL_FRAMES> _present_complete_semaphores = {};
    std::array<VkFence, MAX_PARALLEL_FRAMES>     _render_fences = {};
};
