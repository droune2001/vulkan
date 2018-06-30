#pragma once

#include <vector>

class Window;

class Renderer
{
public:
    Renderer();
    ~Renderer();

	// TODO(nfauvet): put that elsewhere, does not belong here.
	Window *OpenWindow( uint32_t size_x, uint32_t size_y, const std::string &title);
	bool Run();
	bool Init();

    VkDevice device() { return _device; }
    uint32_t familyIndex() { return _graphics_family_index; }
    VkQueue queue() { return _queue; }

private:
	bool InitInstance();
    void DeInitInstance();

	bool InitDevice();
    void DeInitDevice();

    void SetupDebug();
	bool InitDebug();
    void DeInitDebug();

private:

	Window *_window = nullptr;

	VkInstance       _instance = VK_NULL_HANDLE; // ou nullptr, selon la version de vulkan
	VkPhysicalDevice _gpu = VK_NULL_HANDLE;
	VkDevice         _device = VK_NULL_HANDLE;
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
