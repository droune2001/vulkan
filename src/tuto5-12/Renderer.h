#pragma once

#include <vector>

class Window;

class Renderer
{
public:
    Renderer();
    ~Renderer();

	bool Init();

    Window *OpenWindow(uint32_t size_x, uint32_t size_y, const std::string &title);

    bool Run();
    
	void Draw();
	
    const VkInstance GetVulkanInstance() const { return _instance; }
    const VkPhysicalDevice GetVulkanPhysicalDevice() const { return _gpu; }
    const VkDevice GetVulkanDevice() const { return _device; }
    const VkQueue GetVulkanQueue() const { return _queue; }
    const uint32_t GetVulkanGraphicsQueueFamilyIndex() const { return _graphics_family_index; }
    const VkPhysicalDeviceProperties &GetVulkanPhysicalDeviceProperties() const { return _gpu_properties; }
    const VkPhysicalDeviceMemoryProperties &GetVulkanPhysicalDeviceMemoryProperties() const { return _gpu_memory_properties; }

	VkCommandBuffer &GetVulkanCommandBuffer() { return _command_buffer; }

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

    bool InitCommandBuffer();
    void DeInitCommandBuffer();

private:

    Window * _window = nullptr;

    VkInstance       _instance = VK_NULL_HANDLE; // ou nullptr, selon la version de vulkan
    VkPhysicalDevice _gpu = VK_NULL_HANDLE;
    VkDevice         _device = VK_NULL_HANDLE;
    VkQueue          _queue = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties _gpu_properties = {};
    VkPhysicalDeviceMemoryProperties _gpu_memory_properties = {};

    uint32_t _graphics_family_index = 0;

    std::vector< const char * > _instance_layers;
    std::vector< const char * > _instance_extensions;
    std::vector< const char * > _device_layers; // deprecated
    std::vector< const char * > _device_extensions; // deprecated

    VkDebugReportCallbackEXT    _debug_report = VK_NULL_HANDLE;

    // keep it in here to be able to give it to VkCreateInstance
    VkDebugReportCallbackCreateInfoEXT debug_callback_create_info = {};

	// "Scene"
	VkCommandPool _command_pool = VK_NULL_HANDLE;
	VkCommandBuffer _command_buffer = VK_NULL_HANDLE;
};
