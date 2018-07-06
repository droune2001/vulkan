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
	InitOSWindow();

	if (!InitSurface())
		return false;

	if (!InitSwapChain())
		return false;

	if (!InitSwapChainImages())
		return false;

	if (!InitDepthStencilImage())
		return false;

	return true;
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
	
	if (!InitOSSurface()) 
		return false;

	auto gpu = _renderer->GetVulkanPhysicalDevice();

	VkBool32 supportsPresent;
	result = vkGetPhysicalDeviceSurfaceSupportKHR(gpu, _renderer->GetVulkanGraphicsQueueFamilyIndex(), _surface, &supportsPresent);
	ErrorCheck(result);
	if ((result != VK_SUCCESS) || !supportsPresent)
	{
		assert(!"Device does not support presentation on choosen surface.");
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
		ErrorCheck(result);
		if (surface_formats_count == 0)
		{
			assert(!"No surface formats");
			return false;
		}

		// VkSurfaceFormat2KHR ???
		std::vector<VkSurfaceFormatKHR> surface_formats;
		surface_formats.resize(surface_formats_count);
		result = vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, _surface, &surface_formats_count, surface_formats.data());
		ErrorCheck(result);
		if (surface_formats_count == 1 && surface_formats[0].format == VK_FORMAT_UNDEFINED)
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

	return true;
}

void Window::DeInitSurface()
{
	vkDestroySurfaceKHR(_renderer->GetVulkanInstance(), _surface, nullptr);
}

bool Window::InitSwapChain()
{
	VkResult result;
	
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
		if (result != VK_SUCCESS)
			return false;

		std::vector<VkPresentModeKHR> present_modes(present_mode_count);
		result = vkGetPhysicalDeviceSurfacePresentModesKHR(_renderer->GetVulkanPhysicalDevice(), _surface, &present_mode_count, present_modes.data());
		ErrorCheck(result);
		if (result != VK_SUCCESS)
			return false;

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

	return (result == VK_SUCCESS);
}

void Window::DeInitSwapChain()
{
	vkDestroySwapchainKHR(_renderer->GetVulkanDevice(), _swapchain, nullptr);
}

bool Window::InitSwapChainImages()
{
	auto result = VK_SUCCESS;

	_swapchain_image_count = 0;
	result = vkGetSwapchainImagesKHR(_renderer->GetVulkanDevice(), _swapchain, &_swapchain_image_count, nullptr);
	ErrorCheck(result);
	if (result != VK_SUCCESS)
		return false;

	_swapchain_images.resize(_swapchain_image_count);
	_swapchain_image_views.resize(_swapchain_image_count);
	result = vkGetSwapchainImagesKHR(_renderer->GetVulkanDevice(), _swapchain, &_swapchain_image_count, _swapchain_images.data());
	ErrorCheck(result);
	if (result != VK_SUCCESS)
		return false;

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
		if (result != VK_SUCCESS)
			return false;
	}

	return true;
}

void Window::DeInitSwapChainImages()
{
	for (uint32_t i = 0; i < _swapchain_image_count; ++i)
	{
		vkDestroyImageView(_renderer->GetVulkanDevice(), _swapchain_image_views[i], nullptr);
	}
}

bool Window::InitDepthStencilImage()
{
	VkResult result;

	{
		// try these, pick the first supported
		std::vector<VkFormat> potential_formats = {
			VK_FORMAT_D32_SFLOAT_S8_UINT,
			VK_FORMAT_D24_UNORM_S8_UINT,
			VK_FORMAT_D16_UNORM_S8_UINT,
			VK_FORMAT_D32_SFLOAT,
			VK_FORMAT_D16_UNORM
		};

		for (auto f : potential_formats)
		{
			// VkFormatProperties2 ??? 
			VkFormatProperties format_properties = {};
			// vkGetPhysicalDeviceFormatProperties2 ???
			vkGetPhysicalDeviceFormatProperties(_renderer->GetVulkanPhysicalDevice(), f, &format_properties);
			if (format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
			{
				_depth_stencil_format = f;
				break;
			}
		}

		if (_depth_stencil_format == VK_FORMAT_UNDEFINED)
		{
			assert(!"No depth/stencil formats supported.");
			return false;
		}

		if (_depth_stencil_format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
			_depth_stencil_format == VK_FORMAT_D24_UNORM_S8_UINT ||
			_depth_stencil_format == VK_FORMAT_D16_UNORM_S8_UINT)
		{
			_stencil_available = true;
		}
	}

	VkImageCreateInfo image_create_info = {};
	image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_create_info.imageType = VK_IMAGE_TYPE_2D;
	image_create_info.format    = _depth_stencil_format;
	image_create_info.extent.width  = _surface_size_x;
	image_create_info.extent.height = _surface_size_y;
	image_create_info.extent.depth  = 1;
	image_create_info.mipLevels     = 1;
	image_create_info.arrayLayers   = 1;
	image_create_info.samples = VK_SAMPLE_COUNT_1_BIT; // of doing multi sampling, put the same here as in the swapchain.
	image_create_info.tiling  = VK_IMAGE_TILING_OPTIMAL; // use gpu tiling
	image_create_info.usage   = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	image_create_info.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
	image_create_info.queueFamilyIndexCount = VK_QUEUE_FAMILY_IGNORED;
	image_create_info.pQueueFamilyIndices   = nullptr;
	image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // will have to change layout after

	// NOTE(nfauvet): only create a handle to the image, like glGenTexture.
	result = vkCreateImage(_renderer->GetVulkanDevice(), &image_create_info, nullptr, &_depth_stencil_image);
	ErrorCheck(result);
	if (result != VK_SUCCESS)
		return false;
	
	VkMemoryRequirements memory_requirements = {};
	vkGetImageMemoryRequirements(_renderer->GetVulkanDevice(), _depth_stencil_image, &memory_requirements);

	auto gpu_memory_properties = _renderer->GetVulkanPhysicalDeviceMemoryProperties();
	uint32_t memory_index = FindMemoryTypeIndex(&gpu_memory_properties, &memory_requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	if (memory_index == UINT32_MAX)
	{
		assert(!"Memory index not found to allocated depth buffer");
		return false;
	}

	VkMemoryAllocateInfo memory_allocate_info = {};
	memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocate_info.allocationSize = memory_requirements.size;
	memory_allocate_info.memoryTypeIndex = memory_index;

	result = vkAllocateMemory(_renderer->GetVulkanDevice(), &memory_allocate_info, nullptr, &_depth_stencil_image_memory);
	ErrorCheck(result);
	if (result != VK_SUCCESS)
		return false;

	result = vkBindImageMemory(_renderer->GetVulkanDevice(), _depth_stencil_image, _depth_stencil_image_memory, 0);
	ErrorCheck(result);
	if (result != VK_SUCCESS)
		return false;

	VkImageViewCreateInfo image_view_create_info = {};
	image_view_create_info.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	image_view_create_info.image    = _depth_stencil_image;
	image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	image_view_create_info.format   = _depth_stencil_format;
	image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | (_stencil_available ? VK_IMAGE_ASPECT_STENCIL_BIT : 0);
	image_view_create_info.subresourceRange.baseMipLevel   = 0;
	image_view_create_info.subresourceRange.levelCount     = 1;
	image_view_create_info.subresourceRange.baseArrayLayer = 0;
	image_view_create_info.subresourceRange.layerCount     = 1;

	result = vkCreateImageView(_renderer->GetVulkanDevice(), &image_view_create_info, nullptr, &_depth_stencil_image_view);
	ErrorCheck(result);
	if (result != VK_SUCCESS)
		return false;

	return true;
}

void Window::DeInitDepthStencilImage()
{
	vkDestroyImageView(_renderer->GetVulkanDevice(), _depth_stencil_image_view, nullptr);
	vkFreeMemory(_renderer->GetVulkanDevice(), _depth_stencil_image_memory, nullptr);
	vkDestroyImage(_renderer->GetVulkanDevice(), _depth_stencil_image, nullptr);
}
