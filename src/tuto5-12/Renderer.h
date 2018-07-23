#pragma once

#include "vk_mem_alloc_usage.h"

#include <vector>

class Window;
class Scene;

struct vulkan_queue
{
    VkQueue         queue = VK_NULL_HANDLE;
    uint32_t        family_index = 0;
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
    
    const vulkan_context &context() { return _ctx; };
    VkCommandBuffer graphics_command_buffer() { return _ctx.graphics.command_buffer; }

    //const VkInstance instance() const { return _ctx.instance; }
    //const VkPhysicalDevice physical_device() const { return _ctx.physical_device; }
    //const VkDevice device() const { return _ctx.device; }
    //const VkQueue graphics_queue() const { return _ctx.graphics_queue; }
    //const VkQueue compute_queue() const { return _ctx.compute_queue; }
    //const VkQueue transfer_queue() const { return _ctx.transfer_queue; }
    //const VkQueue present_queue() const { return _ctx.present_queue; }
    //const uint32_t graphics_family_index() const { return _ctx.graphics_family_index; }
    //const uint32_t compute_family_index() const { return _ctx.compute_family_index; }
    //const uint32_t transfer_family_index() const { return _ctx.transfer_family_index; }
    //const uint32_t present_family_index() const { return _ctx.present_family_index; }
    //const VkPhysicalDeviceProperties &physical_device_properties() const { return _ctx.physical_device_properties; }
    //const VkPhysicalDeviceMemoryProperties &physical_device_memory_properties() const { return _ctx.physical_device_memory_properties; }
    //VkCommandBuffer &GetVulkanCommandBuffer() { return _command_buffer; }

    Renderer(Window *w);
    ~Renderer();

    // inits vulkan context and window-vulkan specifics
    bool Init();
    
    void Draw(float dt, Scene *scene);
   

private:
    bool InitInstance();
    void DeInitInstance();

    bool InitDevice();
    void DeInitDevice();

    void SetupDebug();
    bool InitDebug();
    void DeInitDebug();

    void SetupLayers();
    void SetupExtensions();

    bool InitVma();
    void DeInitVma();

    bool InitCommandBuffer();
    void DeInitCommandBuffer();

private:

    vulkan_context _ctx;
    VmaAllocator   _allocator = VK_NULL_HANDLE;

    Window * _w = nullptr; // ref

    




	// "Scene"
    struct camera
    {
        float cameraZ = 10.0f;
        float cameraZDir = -1.0f;
    } _camera;
};
