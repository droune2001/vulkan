#include "build_options.h"
#include "platform.h"
#include "window.h"
#include "Renderer.h"
#include "Shared.h"

#include <assert.h>
#include <sstream>
#include <array>

Window::Window(Renderer *renderer, uint32_t size_x, uint32_t size_y, const std::string & title)
:	_renderer(renderer),
	_surface_size_x(size_x),
	_surface_size_y(size_y),
	_window_name(title)
{
}

bool Window::Init()
{
	Log("#  Init OS Window\n");
	InitOSWindow();

	Log("#  Init Backbuffer Surface\n");
	if (!InitSurface())
		return false;

	Log("#  Init SwapChain\n");
	if (!InitSwapChain())
		return false;

	Log("#  InitSwapChain Images\n");
	if (!InitSwapChainImages())
		return false;

	Log("#  Init Depth/Stencil\n");
	if (!InitDepthStencilImage())
		return false;

	Log("#  Init Render Pass\n");
	if (!InitRenderPass())
		return false;

	Log("#  Init FrameBuffers\n");
	if (!InitFrameBuffers())
		return false;

	Log("#  Init Graphics Pipeline\n");
	if (!InitGraphicsPipeline())
		return false;

	Log("#  Init Synchronizations\n");
	if (!InitSynchronizations())
		return false;

	//Log("#  Init Graphics Pipeline\n");
	//if (!InitGraphicsPipeline())
	//	return false;

	return true;
}

Window::~Window()
{
	vkQueueWaitIdle(_renderer->GetVulkanQueue());

	//Log("#  Destroy Graphics Pipeline\n");
	//DeInitGraphicsPipeline();

	Log("#  Destroy Synchronizations\n");
	DeInitSynchronizations();

	Log("#  Destroy FrameBuffers\n");
	DeInitFrameBuffers();

	Log("#  Destroy Render Pass\n");
	DeInitRenderPass();

	Log("#  Destroy Depth/Stencil\n");
	DeInitDepthStencilImage();

	Log("#  Destroy SwapChain Images\n");
	DeInitSwapChainImages();

	Log("#  Destroy SwapChain\n");
	DeInitSwapChain();

	Log("#  Destroy Backbuffer Surface\n");
	DeInitSurface();

	Log("#  Destroy OS Window\n");
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

void Window::BeginRender()
{
	VkResult result;

	result = vkAcquireNextImageKHR(
		_renderer->GetVulkanDevice(), _swapchain, UINT64_MAX, 
		VK_NULL_HANDLE, _swapchain_image_available_fence, 
		&_active_swapchain_image_id);
	ErrorCheck(result);

	// <------------------------------------------------- FENCE wait for swap image

	result = vkWaitForFences(_renderer->GetVulkanDevice(), 1, &_swapchain_image_available_fence, VK_TRUE, UINT64_MAX ); // wait forever
	ErrorCheck(result);

	result = vkResetFences(_renderer->GetVulkanDevice(), 1, &_swapchain_image_available_fence);
	ErrorCheck(result);

	// <------------------------------------------------- WAIT for queue

	result = vkQueueWaitIdle(_renderer->GetVulkanQueue());
	ErrorCheck(result);
}

void Window::EndRender(std::vector<VkSemaphore> wait_semaphores)
{
	VkResult result;

	VkResult present_result = VkResult::VK_RESULT_MAX_ENUM;

	// <------------------------------------------------- Wait on semaphores before presenting

	VkPresentInfoKHR present_info = {};
	present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = (uint32_t)wait_semaphores.size();
	present_info.pWaitSemaphores    = wait_semaphores.data();
	present_info.swapchainCount     = 1; // how many swapchains we want to update/present to
	present_info.pSwapchains        = &_swapchain;
	present_info.pImageIndices      = &_active_swapchain_image_id; // of size swapchainCount, indices into each swapchain
	present_info.pResults           = &present_result; // result for every swapchain

	result = vkQueuePresentKHR(_renderer->GetVulkanQueue(), &present_info);
	ErrorCheck(result);
	ErrorCheck(present_result);
}

bool Window::InitSurface()
{
	VkResult result = VK_SUCCESS;
	
	Log("#   Init OS Surface\n");
	if (!InitOSSurface()) 
		return false;

	auto gpu = _renderer->GetVulkanPhysicalDevice();

	Log("#   Test Device supports surface?\n");
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
	Log("#   Get Physical Device Surface Capabilities\n");
	result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, _surface, &_surface_caps);
	ErrorCheck(result);
	{
		// The size of the surface may not be exactly the one we asked for.
		if (_surface_caps.currentExtent.width < UINT32_MAX)
		{
			_surface_size_x = _surface_caps.currentExtent.width;
			_surface_size_y = _surface_caps.currentExtent.height;

			std::ostringstream oss;
			oss << "#    width: " << _surface_size_x << " height: " << _surface_size_y << std::endl;
			Log(oss.str().c_str());
		}

		{
			// vkGetPhysicalDeviceSurfaceFormats2KHR ???
			Log("#   Get Physical Device Surface Formats\n");
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

	Log("#   Get Physical Device Surface Present Modes.\n");
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
	Log( (present_mode == VK_PRESENT_MODE_MAILBOX_KHR) ? "#   -> VK_PRESENT_MODE_MAILBOX_KHR\n" : "#   -> VK_PRESENT_MODE_FIFO_KHR\n");

	std::ostringstream oss;
	oss << "#   Create SwapChain:\n"
		<< "#   -> image count: " << _swapchain_image_count << "\n";
	Log(oss.str().c_str());

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

	Log("#   Get SwapChain Images\n");
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
		std::ostringstream oss;
		oss << "#   Create SwapChain Image View [" << i << "]\n";
		Log(oss.str().c_str());

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

		Log("#   Scan Potential Formats Optimal Tiling... Get Physical Device Format Properties\n");
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

	Log("#   Create Image\n");
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
	
	Log("#   Get Image Memory Requirements\n");
	VkMemoryRequirements memory_requirements = {};
	vkGetImageMemoryRequirements(_renderer->GetVulkanDevice(), _depth_stencil_image, &memory_requirements);

	Log("#   Find Memory Type Index\n");
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

	Log("#   Allocate Memory\n");
	result = vkAllocateMemory(_renderer->GetVulkanDevice(), &memory_allocate_info, nullptr, &_depth_stencil_image_memory);
	ErrorCheck(result);
	if (result != VK_SUCCESS)
		return false;

	Log("#   Bind Image Memory\n");
	result = vkBindImageMemory(_renderer->GetVulkanDevice(), _depth_stencil_image, _depth_stencil_image_memory, 0);
	ErrorCheck(result);
	if (result != VK_SUCCESS)
		return false;

	Log("#   Create Image View\n");
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
	Log("#   Destroy Image View\n");
	vkDestroyImageView(_renderer->GetVulkanDevice(), _depth_stencil_image_view, nullptr);

	Log("#   Free Memory\n");
	vkFreeMemory(_renderer->GetVulkanDevice(), _depth_stencil_image_memory, nullptr);

	Log("#   Destroy Image\n");
	vkDestroyImage(_renderer->GetVulkanDevice(), _depth_stencil_image, nullptr);
}

#define ATTACH_INDEX_DEPTH 0
#define ATTACH_INDEX_COLOR 1

bool Window::InitRenderPass()
{
	VkResult result;

	Log("#   Define Attachements\n");
	std::array<VkAttachmentDescription, 2> attachements = {};
	{   // depth/stencil
		attachements[ATTACH_INDEX_DEPTH].flags = 0;
		attachements[ATTACH_INDEX_DEPTH].format = _depth_stencil_format;
		attachements[ATTACH_INDEX_DEPTH].samples = VK_SAMPLE_COUNT_1_BIT;
		attachements[ATTACH_INDEX_DEPTH].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachements[ATTACH_INDEX_DEPTH].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachements[ATTACH_INDEX_DEPTH].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;// VK_ATTACHMENT_LOAD_OP_LOAD;
		attachements[ATTACH_INDEX_DEPTH].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachements[ATTACH_INDEX_DEPTH].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachements[ATTACH_INDEX_DEPTH].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		// color
		attachements[ATTACH_INDEX_COLOR].flags = 0;
		attachements[ATTACH_INDEX_COLOR].format = _surface_format.format; // bc we are rendering directly to the screen
		attachements[ATTACH_INDEX_COLOR].samples = VK_SAMPLE_COUNT_1_BIT; // needs to be the same for all attachements
		attachements[ATTACH_INDEX_COLOR].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachements[ATTACH_INDEX_COLOR].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		//attachements[ATTACH_INDEX_COLOR].stencilLoadOp  = ; // not used for color att
		//attachements[ATTACH_INDEX_COLOR].stencilStoreOp = ; // not used for color att
		attachements[ATTACH_INDEX_COLOR].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachements[ATTACH_INDEX_COLOR].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // ready to present
	}

	Log("#   Define Attachment References\n");

	VkAttachmentReference subpass_0_depth_attachment = {};
	subpass_0_depth_attachment.attachment = ATTACH_INDEX_DEPTH;
	subpass_0_depth_attachment.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	std::array<VkAttachmentReference, 1> subpass_0_color_attachments = {};
	subpass_0_color_attachments[0].attachment = ATTACH_INDEX_COLOR;
	subpass_0_color_attachments[0].layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	Log("#   Define SubPasses\n");

	std::array<VkSubpassDescription, 1> subpasses = {};
	{
		subpasses[0].flags                   = 0;
		subpasses[0].pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS; // or compute
		subpasses[0].inputAttachmentCount    = 0; // used for example if we are in the second subpass and it needs something from the first subpass
		subpasses[0].pInputAttachments       = nullptr;
		subpasses[0].colorAttachmentCount    = (uint32_t)subpass_0_color_attachments.size();
		subpasses[0].pColorAttachments       = subpass_0_color_attachments.data();
		subpasses[0].pResolveAttachments     = nullptr;
		subpasses[0].pDepthStencilAttachment = &subpass_0_depth_attachment;
		subpasses[0].preserveAttachmentCount = 0;
		subpasses[0].pPreserveAttachments    = nullptr;
	}

	Log("#   Create Render Pass\n");

	VkRenderPassCreateInfo render_pass_create_info = {};
	render_pass_create_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_create_info.attachmentCount = (uint32_t)attachements.size();
	render_pass_create_info.pAttachments    = attachements.data();
	render_pass_create_info.subpassCount    = (uint32_t)subpasses.size();
	render_pass_create_info.pSubpasses      = subpasses.data();
	render_pass_create_info.dependencyCount = 0; // dependencies between subpasses, if one reads a buffer from another
	render_pass_create_info.pDependencies   = nullptr;

	result = vkCreateRenderPass(_renderer->GetVulkanDevice(), &render_pass_create_info, nullptr, &_render_pass);
	ErrorCheck(result);
	if (result != VK_SUCCESS)
		return false;

	return true;
}
  
void Window::DeInitRenderPass()
{
	vkDestroyRenderPass(_renderer->GetVulkanDevice(), _render_pass, nullptr);
}

bool Window::InitFrameBuffers()
{
	VkResult result;

	_framebuffers.resize(_swapchain_image_count);
	
	for (uint32_t i = 0; i < _swapchain_image_count; ++i)
	{
		std::array<VkImageView, 2> attachments = {};
		attachments[ATTACH_INDEX_DEPTH] = _depth_stencil_image_view; // shared between framebuffers
		attachments[ATTACH_INDEX_COLOR] = _swapchain_image_views[i];

		VkFramebufferCreateInfo frame_buffer_create_info = {};
		frame_buffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frame_buffer_create_info.renderPass      = _render_pass;
		frame_buffer_create_info.attachmentCount = (uint32_t)attachments.size(); // need to be compatible with the render pass attachments
		frame_buffer_create_info.pAttachments    = attachments.data();
		frame_buffer_create_info.width           = _surface_size_x;
		frame_buffer_create_info.height          = _surface_size_y;
		frame_buffer_create_info.layers          = 1;

		result = vkCreateFramebuffer(_renderer->GetVulkanDevice(), &frame_buffer_create_info, nullptr, &_framebuffers[i]);
		ErrorCheck(result);
		if (result != VK_SUCCESS)
			return false;
	}
	return true;
}

void Window::DeInitFrameBuffers()
{
	for (size_t i = 0; i < _framebuffers.size(); ++i)
	{
		vkDestroyFramebuffer(_renderer->GetVulkanDevice(), _framebuffers[i], nullptr);
	}
}

bool Window::InitSynchronizations()
{
	VkResult result;

	VkFenceCreateInfo fence_create_info = {};
	fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

	result = vkCreateFence(_renderer->GetVulkanDevice(), &fence_create_info, nullptr, &_swapchain_image_available_fence);
	ErrorCheck(result);
	if (result != VK_SUCCESS)
		return false;

	return true;
}

void Window::DeInitSynchronizations()
{
	vkDestroyFence(_renderer->GetVulkanDevice(), _swapchain_image_available_fence, nullptr);
}













bool Window::InitGraphicsPipeline()
{
#if 0
	VkResult result;

	// TODO...
	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages = {};
	VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {};
	VkPipelineInputAssemblyStateCreateInfo assembly_state_create_info = {};
	VkPipelineViewportStateCreateInfo viewport_state_create_info = {};
	VkPipelineRasterizationStateCreateInfo raster_state_create_info = {};
	VkPipelineMultisampleStateCreateInfo multisample_state_create_info = {};
	VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info = {};
	VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = {};

	std::array<VkGraphicsPipelineCreateInfo, 1> pipeline_create_infos = {};
	pipeline_create_infos[0].sType                 = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_create_infos[0].stageCount            = (uint32_t)shader_stages.size();
	pipeline_create_infos[0].pStages               = shader_stages.data();
	pipeline_create_infos[0].pVertexInputState     = &vertex_input_state_create_info;
	pipeline_create_infos[0].pInputAssemblyState   = &assembly_state_create_info;
	pipeline_create_infos[0].pTessellationState    = nullptr;
	pipeline_create_infos[0].pViewportState        = &viewport_state_create_info;
	pipeline_create_infos[0].pRasterizationState   = &raster_state_create_info;
	pipeline_create_infos[0].pMultisampleState     = &multisample_state_create_info;
	pipeline_create_infos[0].pDepthStencilState    = &depth_stencil_state_create_info;
	pipeline_create_infos[0].pColorBlendState      = &color_blend_state_create_info;
	pipeline_create_infos[0].pDynamicState         = nullptr;
	pipeline_create_infos[0].layout                = _pipeline_layout;
	pipeline_create_infos[0].renderPass            = _render_pass;
	pipeline_create_infos[0].subpass               = 0;
	pipeline_create_infos[0].basePipelineHandle    = VK_NULL_HANDLE; // only if VK_PIPELINE_CREATE_DERIVATIVE flag is set.
	pipeline_create_infos[0].basePipelineIndex     = -1;

	result = vkCreateGraphicsPipelines(
		_renderer->GetVulkanDevice(),
		VK_NULL_HANDLE, // cache
		(uint32_t)pipeline_create_infos.size(),
		pipeline_create_infos.data(),
		nullptr,
		_pipelines.data());
	ErrorCheck(result);
	if (result != VK_SUCCESS)
		return false;
#endif
	return true;
}

void Window::DeInitGraphicsPipeline()
{
	//for (size_t i = 0; i < _pipelines.size(); ++i)
	//{
	//	vkDestroyPipeline(_renderer->GetVulkanDevice(), _pipelines[i], nullptr);
	//}
}

