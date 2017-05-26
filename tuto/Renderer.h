#pragma once

#include <vector>
#include <vulkan/vulkan.h>

class Renderer
{
public:
    Renderer();
    ~Renderer();

    VkDevice device() { return _device; }
    uint32_t familyIndex() { return _graphics_family_index; }
    VkQueue queue() { return _queue; }

private:
    void InitInstance();
    void DeInitInstance();

    void InitDevice();
    void DeInitDevice();

    void SetupDebug();
    void InitDebug();
    void DeInitDebug();

private:

    VkInstance       _instance = VK_NULL_HANDLE; // ou nullptr, selon la version de vulkan
    VkPhysicalDevice _gpu      = VK_NULL_HANDLE;
    VkDevice         _device   = VK_NULL_HANDLE;
    VkQueue          _queue = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties _gpu_properties = {};

    uint32_t _graphics_family_index = 0;

    std::vector< const char * > _instance_layers;
    std::vector< const char * > _instance_extensions;
    std::vector< const char * > _device_layers; // deprecated
    std::vector< const char * > _device_extensions; // deprecated

    VkDebugReportCallbackEXT    _debug_report = VK_NULL_HANDLE;

    // keep it in here to be able to give it to VkCreateInstance
    VkDebugReportCallbackCreateInfoEXT debug_callback_create_info = {};
};

