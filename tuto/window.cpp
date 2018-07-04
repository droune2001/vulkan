#include "build_options.h"
#include "platform.h"
#include "window.h"
#include "Renderer.h"
#include "Shared.h"

#include <assert.h>

Window::Window(Renderer *renderer, uint32_t size_x, uint32_t size_y, const std::string & title)
:	_renderer(renderer),
	_surface_size_x(size_x),
	_surface_size_y(size_y),
	_window_name(title)
{
	InitOSWindow();
	InitSurface();
	InitSwapChain();
}

Window::~Window()
{
	DeInitSwapChain();
	DeInitSurface();
	DeInitOSWindow();
}

void Window::Close()
{
	_window_should_run = false;
}

bool Window::Update()
{
	UpdateOSWindow();
	return _window_should_run;
}

void Window::InitSurface()
{
	InitOSSurface();

	auto gpu = _renderer->GetVulkanPhysicalDevice();

	VkBool32 supportsPresent;
	vkGetPhysicalDeviceSurfaceSupportKHR(gpu, _renderer->GetVulkanGraphicsQueueFamilyIndex(), _surface, &supportsPresent);
	if (!supportsPresent)
	{
		assert(!"device does not support presentation");
		std::exit(-1);
	}

	//vkGetPhysicalDeviceSurfaceCapabilities2EXT ???
	//vkGetPhysicalDeviceSurfaceCapabilities2KHR ???
	auto result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, _surface, &_surface_caps);
	{
		// The size of the surface may not be exactly the one we asked for.
		if (_surface_caps.currentExtent.width < UINT32_MAX)
		{
			_surface_size_x = _surface_caps.currentExtent.width;
			_surface_size_y = _surface_caps.currentExtent.height;
		}

		// vkGetPhysicalDeviceSurfaceFormats2KHR ???
		uint32_t surface_formats_count = 0;
		result = vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, _surface, &surface_formats_count, nullptr);
		if (surface_formats_count == 0)
		{
			assert(!"no surface formats");
			std::exit(-1);
		}

		// VkSurfaceFormat2KHR ???
		std::vector<VkSurfaceFormatKHR> surface_formats;
		surface_formats.resize(surface_formats_count);
		result = vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, _surface, &surface_formats_count, surface_formats.data());
		if (surface_formats[0].format == VK_FORMAT_UNDEFINED) 
		{
			// if it does not care which format, lets choose one we want.
			_surface_format.format = VK_FORMAT_B8G8R8A8_UNORM;
			_surface_format.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
		}
		else
		{
			// recommanded to use the first one.
			_surface_format = surface_formats[0];
		}
	}
}

void Window::DeInitSurface()
{
	vkDestroySurfaceKHR(_renderer->GetVulkanInstance(), _surface, nullptr);
}

void Window::InitSwapChain()
{
	// Test if we wanted more swap images than the system is capable of.
	if (_swapchain_image_count > _surface_caps.maxImageCount) _swapchain_image_count = _surface_caps.maxImageCount;
	if (_swapchain_image_count < _surface_caps.minImageCount + 1) _swapchain_image_count = _surface_caps.minImageCount;

	VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR; // VK_PRESENT_MODE_FIFO_KHR always available
	{
		uint32_t present_mode_count = 0;
		ErrorCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(_renderer->GetVulkanPhysicalDevice(), _surface, &present_mode_count, nullptr));
		if (present_mode_count == 0)
		{
			assert(!"no present modes");
			std::exit(-1);
		}
		std::vector<VkPresentModeKHR> present_modes(present_mode_count);
		ErrorCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(_renderer->GetVulkanPhysicalDevice(), _surface, &present_mode_count, present_modes.data()));
		for (auto m : present_modes)
		{
			// look for MailBox which does vsync.
			if (m == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				present_mode = m;
			}
		}
	}

	VkSwapchainCreateInfoKHR swapchain_create_info = {};
	swapchain_create_info.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_create_info.surface               = _surface;
	swapchain_create_info.minImageCount         = _swapchain_image_count; // 2 = double buffering
	swapchain_create_info.imageFormat           = _surface_format.format;
	swapchain_create_info.imageColorSpace       = _surface_format.colorSpace;
	swapchain_create_info.imageExtent.width     = _surface_size_x;
	swapchain_create_info.imageExtent.height    = _surface_size_y;
	swapchain_create_info.imageArrayLayers      = 1;
	swapchain_create_info.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // + transfer dst bit if not rendering directly.
	swapchain_create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
	swapchain_create_info.queueFamilyIndexCount = 0; // ignored if using exclusive sharing
	swapchain_create_info.pQueueFamilyIndices   = nullptr;
	swapchain_create_info.preTransform          = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR; // rotation, mirror...
	swapchain_create_info.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // for windows surface composition
	swapchain_create_info.presentMode           = present_mode; // vsync option
	swapchain_create_info.clipped               = VK_TRUE;
	swapchain_create_info.oldSwapchain          = VK_NULL_HANDLE; // used when reconstructing after having resized the window.

	ErrorCheck(vkCreateSwapchainKHR(_renderer->GetVulkanDevice(), &swapchain_create_info, nullptr, &_swapchain));
	
	uint32_t swapchain_image_count = 0;
	ErrorCheck(vkGetSwapchainImagesKHR(_renderer->GetVulkanDevice(), _swapchain, &swapchain_image_count, nullptr));
	if (swapchain_image_count == 0)
	{
		assert(!"no swapchain images");
		std::exit(-1);
	}

	// TODO(nfauvet): tuto 8
	std::vector<VkImage> swapchain_images(swapchain_image_count);
	ErrorCheck(vkGetSwapchainImagesKHR(_renderer->GetVulkanDevice(), _swapchain, &swapchain_image_count, swapchain_images.data()));
}

void Window::DeInitSwapChain()
{
	vkDestroySwapchainKHR(_renderer->GetVulkanDevice(), _swapchain, nullptr);
}
