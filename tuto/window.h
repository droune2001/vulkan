#pragma once

#include <string>
#include <vector>

class Renderer;

class Window
{
public:
	Window(Renderer *renderer, uint32_t size_x, uint32_t size_y, const std::string & title);
	~Window();

	bool Init();
	void Close();
	bool Update();

	const VkSurfaceKHR GetSurface() const { return _surface; }

private:

	void InitOSWindow();
	void DeInitOSWindow();
	void UpdateOSWindow();

	bool InitOSSurface();
	bool InitSurface();
	void DeInitSurface();

	bool InitSwapChain();
	void DeInitSwapChain();

	bool InitSwapChainImages();
	void DeInitSwapChainImages();

	bool InitDepthStencilImage();
	void DeInitDepthStencilImage();

	bool InitRenderPass();
	void DeInitRenderPass();

private:

	Renderer * _renderer = nullptr;
	std::string _window_name;
	bool _window_should_run = true;

	uint32_t _surface_size_x = 512;
	uint32_t _surface_size_y = 512;
	uint32_t _swapchain_image_count = 2;
	VkSurfaceKHR _surface = VK_NULL_HANDLE;
	//VkSurfaceCapabilities2EXT ???
	//VkSurfaceCapabilities2KHR ???
	VkSurfaceCapabilitiesKHR _surface_caps = {};
	VkSurfaceFormatKHR _surface_format = {};
	VkSwapchainKHR _swapchain = VK_NULL_HANDLE;
	std::vector<VkImage> _swapchain_images;
	std::vector<VkImageView> _swapchain_image_views;

	VkImage _depth_stencil_image = {};
	VkDeviceMemory _depth_stencil_image_memory = VK_NULL_HANDLE;
	VkImageView _depth_stencil_image_view = VK_NULL_HANDLE;
	VkFormat _depth_stencil_format = VK_FORMAT_UNDEFINED;
	bool _stencil_available = false;

	VkRenderPass _render_pass = VK_NULL_HANDLE;

public:

#ifdef VK_USE_PLATFORM_WIN32_KHR
	HWND _win32_window = 0;
	HINSTANCE _win32_instance = 0;
	std::string _win32_class_name{};
	static uint64_t _win32_class_id_counter;
#endif
};
