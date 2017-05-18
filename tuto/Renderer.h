#pragma once

#include <vector>
#include <vulkan/vulkan.h>

class Renderer
{
public:
    Renderer();
    ~Renderer();

private:
    void InitInstance();
    void DeInitInstance();

    void InitDevice();
    void DeInitDevice();

    void SetupDebug();
    void InitDebug();
    void DeInitDebug();

private:

    VkInstance          _instance = nullptr;
    VkPhysicalDevice    _gpu = nullptr;
    VkDevice            _device = nullptr;
    VkPhysicalDeviceProperties _gpu_properties = {};

    uint32_t     _graphics_family_index = 0;

    std::vector<const char *>   _instance_layers;
    std::vector<const char *>   _instance_extensions;
    std::vector<const char *>   _device_layers; // deprecated
    std::vector<const char *>   _device_extensions; // deprecated

    VkDebugReportCallbackEXT    _debug_report = nullptr;
    VkDebugReportCallbackCreateInfoEXT debug_callback_create_info = {};
};

