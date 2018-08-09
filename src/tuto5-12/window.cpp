#include "build_options.h"
#include "platform.h"
#include "window.h"
#include "Renderer.h"
#include "Shared.h"

#include "scene.h"

#include <assert.h>
#include <sstream>
#include <array>

Window::Window()
{
}

bool Window::OpenWindow(uint32_t size_x, uint32_t size_y, const std::string & title)
{
    _surface_size.width = size_x;
    _surface_size.height = size_y;
    _window_name = title;

    Log("#   Init OS Window\n");
    InitOSWindow();

    return true;
}

bool Window::InitVulkanWindowSpecifics(vulkan_context *ctx)
{
    _ctx = ctx;

    Log("#    Init Backbuffer Surface\n");
    if (!InitSurface())
        return false;

    Log("#    Init SwapChain\n");
    if (!InitSwapChain())
        return false;

    Log("#    InitSwapChain Images\n");
    if (!InitSwapChainImages())
        return false;

    return true;
}

void Window::DeleteWindow()
{
    Log("#  Destroy OS Window\n");
    DeInitOSWindow();
}

bool Window::DeInitVulkanWindowSpecifics(vulkan_context *ctx)
{
    Log("#  Destroy SwapChain Images\n");
    DeInitSwapChainImages();

    Log("#  Destroy SwapChain\n");
    DeInitSwapChain();

    Log("#  Destroy Backbuffer Surface\n");
    DeInitSurface();

    return true;
}

Window::~Window()
{
}

VkDevice Window::device()
{ 
    return _ctx->device;
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

void Window::BeginRender(VkSemaphore wait_semaphore)
{
    VkResult result;

    // Signal a semaphore when the presentation engine is done reading the swap
    // image we just acquired (happens later). This semaphore is used to wait
    // before writing (end of fragment shader) to the swap chain.
    result = vkAcquireNextImageKHR(
        device(), _swapchain, UINT64_MAX,
        wait_semaphore, VK_NULL_HANDLE,
        &_active_swapchain_image_id);

    ErrorCheck(result);
}

void Window::EndRender(std::vector<VkSemaphore> wait_semaphores)
{
    VkResult result;
    VkResult present_result = VkResult::VK_RESULT_MAX_ENUM;

    VkPresentInfoKHR present_info = {};
    present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = (uint32_t)wait_semaphores.size();
    present_info.pWaitSemaphores    = wait_semaphores.data();
    present_info.swapchainCount     = 1; // how many swapchains we want to update/present to
    present_info.pSwapchains        = &_swapchain;
    present_info.pImageIndices      = &_active_swapchain_image_id; // of size swapchainCount, indices into each swapchain
    present_info.pResults           = &present_result; // result for every swapchain

    result = vkQueuePresentKHR(_ctx->graphics.queue, &present_info);
    ErrorCheck(result);
    ErrorCheck(present_result);
}

bool Window::InitSurface()
{
    VkResult result = VK_SUCCESS;
    
    Log("#     Init OS Surface\n");
    if (!InitOSSurface()) 
        return false;

    auto gpu = _ctx->physical_device;

    Log("#     Test Device supports surface?\n");
    VkBool32 supportsPresent;
    result = vkGetPhysicalDeviceSurfaceSupportKHR(gpu, _ctx->graphics.family_index, _surface, &supportsPresent);
    ErrorCheck(result);
    if ((result != VK_SUCCESS) || !supportsPresent)
    {
        assert(!"Device does not support presentation on choosen surface.");
        return false;
    }

    //
    // SURFACE EXTENT
    //

    //vkGetPhysicalDeviceSurfaceCapabilities2EXT ???
    //vkGetPhysicalDeviceSurfaceCapabilities2KHR ???
    Log("#     Get Physical Device Surface Capabilities\n");
    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, _surface, &_surface_caps);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    _surface_size = ChooseSurfaceExtent();

    Log(std::string("#      width: ") + std::to_string(_surface_size.width) + std::string(" height: ") + std::to_string(_surface_size.height) + "\n");
    
    //
    // SURFACE FORMAT
    //

    // vkGetPhysicalDeviceSurfaceFormats2KHR ???
    Log("#      Get Physical Device Surface Formats\n");
    uint32_t surface_formats_count = 0;
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, _surface, &surface_formats_count, nullptr);
    ErrorCheck(result);
    if (result != VK_SUCCESS || surface_formats_count == 0)
    {
        assert(!"No surface formats");
        return false;
    }

    // VkSurfaceFormat2KHR ???
    std::vector<VkSurfaceFormatKHR> surface_formats;
    surface_formats.resize(surface_formats_count);
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, _surface, &surface_formats_count, surface_formats.data());
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    _surface_format = ChooseSurfaceFormat(surface_formats);
    
    return true;
}

VkExtent2D Window::ChooseSurfaceExtent()
{
    // The size of the surface may not be exactly the one we asked for.
    if (_surface_caps.currentExtent.width < UINT32_MAX)
    {
        return _surface_caps.currentExtent;
    }
    else
    {
        // check our initial decision against min/max
        VkExtent2D actual_extent = _surface_size;
        
        actual_extent.width = std::max(
            _surface_caps.minImageExtent.width, 
            std::min(
                _surface_caps.maxImageExtent.width, 
                actual_extent.width
            )
        );
        
        actual_extent.height = std::max(
            _surface_caps.minImageExtent.height, 
            std::min(_surface_caps.maxImageExtent.height, 
                actual_extent.height
            )
        );

        return actual_extent;
    }
}

VkSurfaceFormatKHR Window::ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &surface_formats)
{
    if (surface_formats.size() == 1 && surface_formats[0].format == VK_FORMAT_UNDEFINED)
    {
        // if it does not care which format, lets choose one we want.
        VkSurfaceFormatKHR result = {};
        result.format = VK_FORMAT_B8G8R8A8_UNORM;
        result.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
        return result;
    }
    else
    {
        for (const auto& f : surface_formats) 
        { 
            if (f.format == VK_FORMAT_B8G8R8A8_UNORM && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) 
            { 
                return f;
            }
        }

        // recommanded to use the first one.
        return surface_formats[0];
    }
}

void Window::DeInitSurface()
{
    vkDestroySurfaceKHR(_ctx->instance, _surface, nullptr);
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

    Log("#     Get Physical Device Surface Present Modes.\n");
    uint32_t present_mode_count = 0;
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(_ctx->physical_device, _surface, &present_mode_count, nullptr);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    std::vector<VkPresentModeKHR> present_modes(present_mode_count);
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(_ctx->physical_device, _surface, &present_mode_count, present_modes.data());
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    VkPresentModeKHR present_mode = ChoosePresentMode(present_modes);

    Log( (present_mode == VK_PRESENT_MODE_MAILBOX_KHR) ? "#      -> VK_PRESENT_MODE_MAILBOX_KHR\n" : "#   -> VK_PRESENT_MODE_FIFO_KHR\n");

    std::ostringstream oss;
    oss << "#     Create SwapChain with " << _swapchain_image_count << " images.\n";
    Log(oss.str().c_str());

    VkSwapchainCreateInfoKHR swapchain_create_info = {};
    swapchain_create_info.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.surface               = _surface;
    swapchain_create_info.minImageCount         = _swapchain_image_count; // 2 = double buffering
    swapchain_create_info.imageFormat           = _surface_format.format;
    swapchain_create_info.imageColorSpace       = _surface_format.colorSpace;
    swapchain_create_info.imageExtent           = _surface_size;
    swapchain_create_info.imageArrayLayers      = 1; // except if we are doing stereo 3d
    swapchain_create_info.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // + transfer dst bit if not rendering directly.
    swapchain_create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_create_info.queueFamilyIndexCount = 0; // ignored if using exclusive sharing
    swapchain_create_info.pQueueFamilyIndices   = nullptr;
    swapchain_create_info.preTransform          = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR; // rotation, mirror...
    swapchain_create_info.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // for windows surface composition
    swapchain_create_info.presentMode           = present_mode; // vsync option
    swapchain_create_info.clipped               = VK_TRUE;
    swapchain_create_info.oldSwapchain          = VK_NULL_HANDLE; // used when reconstructing after having resized the window.

    result = vkCreateSwapchainKHR(device(), &swapchain_create_info, nullptr, &_swapchain);
    ErrorCheck(result);

    return (result == VK_SUCCESS);
}

void Window::DeInitSwapChain()
{
    vkDestroySwapchainKHR(device(), _swapchain, nullptr);
}

VkPresentModeKHR Window::ChoosePresentMode(const std::vector<VkPresentModeKHR> &present_modes)
{
    VkPresentModeKHR best_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (auto m : present_modes)
    {
        // look for MailBox which does vsync.
        if (m == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return m;
        }
        else if (m==VK_PRESENT_MODE_IMMEDIATE_KHR)
        {
            best_mode = m;
        }
    }

    return best_mode;
}

bool Window::InitSwapChainImages()
{
    auto result = VK_SUCCESS;

    Log("#     Get SwapChain Images\n");
    _swapchain_image_count = 0;
    result = vkGetSwapchainImagesKHR(device(), _swapchain, &_swapchain_image_count, nullptr);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    _swapchain_images.resize(_swapchain_image_count);
    _swapchain_image_views.resize(_swapchain_image_count);
    result = vkGetSwapchainImagesKHR(device(), _swapchain, &_swapchain_image_count, _swapchain_images.data());
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    for (uint32_t i = 0; i < _swapchain_image_count; ++i)
    {
        std::ostringstream oss;
        oss << "#     Create SwapChain Image View [" << i << "]\n";
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

        result = vkCreateImageView(device(), &image_view_create_info, nullptr, &_swapchain_image_views[i]);
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
        vkDestroyImageView(device(), _swapchain_image_views[i], nullptr);
    }
}

//
