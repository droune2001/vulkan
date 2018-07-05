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
}

Window::~Window()
{
	DeInitDepthStencilImage();
	DeInitSwapChainImages();
	DeInitSwapChain();
	DeInitSurface();
	DeInitOSWindow();
}

bool Window::Init()
{
	bool ok = true;
	InitOSWindow();
	ok &= InitSurface();
	ok &= InitSwapChain();
	InitSwapChainImages();
	InitDepthStencilImage();

	return ok;
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

bool Window::InitSurface()
{
	VkResult result = VK_SUCCESS;
	bool ok = true;

	if (!InitOSSurface()) return false;

	auto gpu = _renderer->GetVulkanPhysicalDevice();

	VkBool32 supportsPresent;
	result = vkGetPhysicalDeviceSurfaceSupportKHR(gpu, _renderer->GetVulkanGraphicsQueueFamilyIndex(), _surface, &supportsPresent);
	ErrorCheck(result);
	if ((result != VK_SUCCESS) || !supportsPresent)
	{
		assert(!"device does not support presentation");
		return false;
	}

	//vkGetPhysicalDeviceSurfaceCapabilities2EXT ???
	//vkGetPhysicalDeviceSurfaceCapabilities2KHR ???
	result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, _surface, &_surface_caps);
	ErrorCheck(result);
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

	return ok;
}

void Window::DeInitSurface()
{
	vkDestroySurfaceKHR(_renderer->GetVulkanInstance(), _surface, nullptr);
}

bool Window::InitSwapChain()
{
	VkResult result;
	bool ok = true;

	// Test if we wanted more swap images than the system is capable of.
	if (_swapchain_image_count < _surface_caps.minImageCount + 1) 
		_swapchain_image_count = _surface_caps.minImageCount + 1;
	// if max is 0, then the implementation supports unlimited nb of swapchain.
	if (_surface_caps.maxImageCount > 0 && _swapchain_image_count > _surface_caps.maxImageCount) 
		_swapchain_image_count = _surface_caps.maxImageCount;

	VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR; // VK_PRESENT_MODE_FIFO_KHR always available
	{
		uint32_t present_mode_count = 0;
		result = vkGetPhysicalDeviceSurfacePresentModesKHR(_renderer->GetVulkanPhysicalDevice(), _surface, &present_mode_count, nullptr);
		ErrorCheck(result);
		ok &= (VK_SUCCESS == result);

		std::vector<VkPresentModeKHR> present_modes(present_mode_count);
		result = vkGetPhysicalDeviceSurfacePresentModesKHR(_renderer->GetVulkanPhysicalDevice(), _surface, &present_mode_count, present_modes.data());
		ErrorCheck(result);
		ok &= (VK_SUCCESS == result);
		
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

	result = vkCreateSwapchainKHR(_renderer->GetVulkanDevice(), &swapchain_create_info, nullptr, &_swapchain);
	ErrorCheck(result);
	ok &= (VK_SUCCESS == result);

	return ok;
}

void Window::DeInitSwapChain()
{
	vkDestroySwapchainKHR(_renderer->GetVulkanDevice(), _swapchain, nullptr);
}

void Window::InitSwapChainImages()
{
	auto result = VK_SUCCESS;

	_swapchain_image_count = 0;
	result = vkGetSwapchainImagesKHR(_renderer->GetVulkanDevice(), _swapchain, &_swapchain_image_count, nullptr);
	ErrorCheck(result);

	_swapchain_images.resize(_swapchain_image_count);
	_swapchain_image_views.resize(_swapchain_image_count);
	result = vkGetSwapchainImagesKHR(_renderer->GetVulkanDevice(), _swapchain, &_swapchain_image_count, _swapchain_images.data());
	ErrorCheck(result);

	for (uint32_t i = 0; i < _swapchain_image_count; ++i)
	{
		VkImageViewCreateInfo image_view_create_info = {};
		image_view_create_info.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		image_view_create_info.image    = _swapchain_images[i];
		image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		image_view_create_info.format   = _surface_format.format;
		image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_R; // or VK_COMPONENT_SWIZZLE_IDENTITY for all of them
		image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_G;
		image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_B;
		image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_A;
		image_view_create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		image_view_create_info.subresourceRange.baseMipLevel   = 0;
		image_view_create_info.subresourceRange.levelCount     = 1;
		image_view_create_info.subresourceRange.baseArrayLayer = 0;
		image_view_create_info.subresourceRange.layerCount     = 1;

		result = vkCreateImageView(_renderer->GetVulkanDevice(), &image_view_create_info, nullptr, &_swapchain_image_views[i]);
		ErrorCheck(result);
	}
}

void Window::DeInitSwapChainImages()
{
	for (uint32_t i = 0; i < _swapchain_image_count; ++i)
	{
		vkDestroyImageView(_renderer->GetVulkanDevice(), _swapchain_image_views[i], nullptr);
	}
}

void Window::InitDepthStencilImage()
{
	// TODO(nfauvet): TUTO 9 at 4:22
	VkImageCreateInfo image_create_info = {};
	image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_create_info.imageType = ;
	image_create_info.format = ;
	image_create_info.extent.width = ;
	image_create_info.extent.height = ;
	image_create_info.extent.depth = ;
	image_create_info.mipLevels = ;
	image_create_info.arrayLayers = ;
	image_create_info.samples = ;
	image_create_info.tiling = ;
	image_create_info.usage = ;
	image_create_info.sharingMode = ;
	image_create_info.queueFamilyIndexCount = ;
	image_create_info.pQueueFamilyIndices = ;
	image_create_info.initialLayout = ;

	auto result = vkCreateImage(_renderer->GetVulkanDevice(), &image_create_info, nullptr, &_depth_stencil_image);
}

void Window::DeInitDepthStencilImage()
{

}
