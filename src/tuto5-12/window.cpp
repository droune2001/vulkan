#include "build_options.h"
#include "platform.h"
#include "window.h"
#include "Renderer.h"
#include "Shared.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE 1 // clip space z [0..1] instead of [-1..1]
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> // glm::perspective
#include <glm/gtc/type_ptr.hpp> // glm::value_ptr

#include <assert.h>
#include <sstream>
#include <array>

Window::Window(Renderer *renderer, uint32_t size_x, uint32_t size_y, const std::string & title)
:    _renderer(renderer),
    _surface_size_x(size_x),
    _surface_size_y(size_y),
    _window_name(title)
{
}

bool Window::Init()
{
    Log("#  Init VMA\n");
    if (!InitVma())
        return false;

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
    if (!InitSwapChainFrameBuffers())
        return false;

    Log("#  Init Synchronizations\n");
    if (!InitSynchronizations())
        return false;

    Log("#  Init Uniform Buffer\n");
    if (!InitUniformBuffer())
        return false;
    
    Log("#  Init FakeImage\n");
    if (!InitFakeImage())
        return false;

    Log("#  Init Descriptors\n");
    if (!InitDescriptors())
        return false;
    
    Log("#  Init Vertex Buffer\n");
    if (!InitVertexBuffer())
        return false;

    Log("#  Init Shaders\n");
    if (!InitShaders())
        return false;

    Log("#  Init Graphics Pipeline\n");
    if (!InitGraphicsPipeline())
        return false;

    return true;
}

Window::~Window()
{
    vkQueueWaitIdle(_renderer->GetVulkanQueue());

    Log("#  Destroy Graphics Pipeline\n");
    DeInitGraphicsPipeline();

    Log("#  Destroy Shaders\n");
    DeInitShaders();

    Log("#  Destroy Vertex Buffer\n");
    DeInitVertexBuffer();

    Log("#  Destroy Descriptors\n");
    DeInitDescriptors();
    
    Log("#  Destroy FakeImage\n");
    DeInitFakeImage();

    Log("#  Destroy Uniform Buffer\n");
    DeInitUniformBuffer();

    Log("#  Destroy Synchronizations\n");
    DeInitSynchronizations();

    Log("#  Destroy FrameBuffers\n");
    DeInitSwapChainFrameBuffers();

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

    Log("#  Destroy Vma\n");
    DeInitVma();
}

VkDevice Window::device()
{ 
    return _renderer->GetVulkanDevice(); 
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

    // version using semaphore
    //result = vkAcquireNextImageKHR(
    //    device(), _swapchain, UINT64_MAX, 
    //    wait_semaphore, VK_NULL_HANDLE, 
    //    &_active_swapchain_image_id);

    //// version using fence
    //result = vkAcquireNextImageKHR(
    //    device(), _swapchain, UINT64_MAX,
    //    VK_NULL_HANDLE, _swapchain_image_available_fence,
    //    &_active_swapchain_image_id);

    // version using BOTH!!!
    result = vkAcquireNextImageKHR(
        device(), _swapchain, UINT64_MAX,
        wait_semaphore, _swapchain_image_available_fence,
        &_active_swapchain_image_id);

    ErrorCheck(result);

    // <------------------------------------------------- FENCE wait for swap image

    result = vkWaitForFences(device(), 1, &_swapchain_image_available_fence, VK_TRUE, UINT64_MAX ); // wait forever
    ErrorCheck(result);

    result = vkResetFences(device(), 1, &_swapchain_image_available_fence);
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

bool Window::InitVma()
{
    VmaVulkanFunctions vulkan_functions = {};
    vulkan_functions.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
    vulkan_functions.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
    vulkan_functions.vkAllocateMemory = vkAllocateMemory;
    vulkan_functions.vkFreeMemory = vkFreeMemory;
    vulkan_functions.vkMapMemory = vkMapMemory;
    vulkan_functions.vkUnmapMemory = vkUnmapMemory;
    vulkan_functions.vkBindBufferMemory = vkBindBufferMemory;
    vulkan_functions.vkBindImageMemory = vkBindImageMemory;
    vulkan_functions.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
    vulkan_functions.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
    vulkan_functions.vkCreateBuffer = vkCreateBuffer;
    vulkan_functions.vkDestroyBuffer = vkDestroyBuffer;
    vulkan_functions.vkCreateImage = vkCreateImage;
    vulkan_functions.vkDestroyImage = vkDestroyImage;
    vulkan_functions.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2KHR;
    vulkan_functions.vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2KHR;

    VmaAllocatorCreateInfo allocator_info = {};
    allocator_info.physicalDevice = _renderer->GetVulkanPhysicalDevice();
    allocator_info.device = _renderer->GetVulkanDevice();
    allocator_info.pVulkanFunctions = &vulkan_functions;

    VkResult result = vmaCreateAllocator(&allocator_info, &_allocator);
    return( result == VK_SUCCESS );
    return true;
}

void Window::DeInitVma()
{
    vmaDestroyAllocator(_allocator);
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

    result = vkCreateSwapchainKHR(device(), &swapchain_create_info, nullptr, &_swapchain);
    ErrorCheck(result);

    return (result == VK_SUCCESS);
}

void Window::DeInitSwapChain()
{
    vkDestroySwapchainKHR(device(), _swapchain, nullptr);
}

bool Window::InitSwapChainImages()
{
    auto result = VK_SUCCESS;

    Log("#   Get SwapChain Images\n");
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
    result = vkCreateImage(device(), &image_create_info, nullptr, &_depth_stencil_image);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;
    
    Log("#   Get Image Memory Requirements\n");
    VkMemoryRequirements memory_requirements = {};
    vkGetImageMemoryRequirements(device(), _depth_stencil_image, &memory_requirements);

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
    result = vkAllocateMemory(device(), &memory_allocate_info, nullptr, &_depth_stencil_image_memory);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    Log("#   Bind Image Memory\n");
    result = vkBindImageMemory(device(), _depth_stencil_image, _depth_stencil_image_memory, 0);
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

    result = vkCreateImageView(device(), &image_view_create_info, nullptr, &_depth_stencil_image_view);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    return true;
}

void Window::DeInitDepthStencilImage()
{
    Log("#   Destroy Image View\n");
    vkDestroyImageView(device(), _depth_stencil_image_view, nullptr);

    Log("#   Free Memory\n");
    vkFreeMemory(device(), _depth_stencil_image_memory, nullptr);

    Log("#   Destroy Image\n");
    vkDestroyImage(device(), _depth_stencil_image, nullptr);
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
        attachements[ATTACH_INDEX_DEPTH].storeOp = VK_ATTACHMENT_STORE_OP_STORE; // VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachements[ATTACH_INDEX_DEPTH].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;// VK_ATTACHMENT_LOAD_OP_LOAD;
        attachements[ATTACH_INDEX_DEPTH].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachements[ATTACH_INDEX_DEPTH].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // format EXPECTED (render pass DOES NOT do it for you)
        attachements[ATTACH_INDEX_DEPTH].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; // renderpass DOES transform into it at the end.

        // color
        attachements[ATTACH_INDEX_COLOR].flags = 0;
        attachements[ATTACH_INDEX_COLOR].format = _surface_format.format; // bc we are rendering directly to the screen
        attachements[ATTACH_INDEX_COLOR].samples = VK_SAMPLE_COUNT_1_BIT; // needs to be the same for all attachements
        attachements[ATTACH_INDEX_COLOR].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachements[ATTACH_INDEX_COLOR].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        //attachements[ATTACH_INDEX_COLOR].stencilLoadOp  = ; // not used for color att
        //attachements[ATTACH_INDEX_COLOR].stencilStoreOp = ; // not used for color att
        attachements[ATTACH_INDEX_COLOR].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // UNDEFINED allows vulkan to throw the old content.
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

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL; // between the previous (acquire) command
    dependency.dstSubpass = 0;                   // and the first subpass
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                             //| VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    Log("#   Create Render Pass\n");

    VkRenderPassCreateInfo render_pass_create_info = {};
    render_pass_create_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.attachmentCount = (uint32_t)attachements.size();
    render_pass_create_info.pAttachments    = attachements.data();
    render_pass_create_info.subpassCount    = (uint32_t)subpasses.size();
    render_pass_create_info.pSubpasses      = subpasses.data();
    render_pass_create_info.dependencyCount = 1; // dependencies between subpasses, if one reads a buffer from another
    render_pass_create_info.pDependencies   = &dependency;

    result = vkCreateRenderPass(device(), &render_pass_create_info, nullptr, &_render_pass);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    return true;
}
  
void Window::DeInitRenderPass()
{
    vkDestroyRenderPass(device(), _render_pass, nullptr);
}

bool Window::InitSwapChainFrameBuffers()
{
    VkResult result;

    _swapchain_framebuffers.resize(_swapchain_image_count);
    
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

        result = vkCreateFramebuffer(device(), &frame_buffer_create_info, nullptr, &_swapchain_framebuffers[i]);
        ErrorCheck(result);
        if (result != VK_SUCCESS)
            return false;
    }
    return true;
}

void Window::DeInitSwapChainFrameBuffers()
{
    for (auto & framebuffer : _swapchain_framebuffers)
    {
        vkDestroyFramebuffer(device(), framebuffer, nullptr);
    }
}

bool Window::InitSynchronizations()
{
    VkResult result;

    VkFenceCreateInfo fence_create_info = {};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    result = vkCreateFence(device(), &fence_create_info, nullptr, &_swapchain_image_available_fence);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    return true;
}

void Window::DeInitSynchronizations()
{
    vkDestroyFence(device(), _swapchain_image_available_fence, nullptr);
}

bool Window::InitUniformBuffer()
{
    VkResult result;

    float aspect_ratio = _surface_size_x / (float)_surface_size_y;
    float fov_degrees = 45.0f;
    float nearZ = 0.1f;
    float farZ = 1000.0f;

    glm::mat4 proj = glm::perspective(fov_degrees, aspect_ratio, nearZ, farZ);
    proj[1][1] *= -1.0f; // vulkan clip space with Y down.
    glm::mat4 view  = glm::lookAt(glm::vec3(-3,-2, 10), glm::vec3(0), glm::vec3(0,1,0));
    glm::mat4 model = glm::mat4(1);

    memcpy(_mvp.m, glm::value_ptr(model), sizeof(model));
    memcpy(_mvp.v, glm::value_ptr(view),  sizeof(view));
    memcpy(_mvp.p, glm::value_ptr(proj),  sizeof(proj));

    Log("#   Create Matrices Uniform Buffer\n");
    VkBufferCreateInfo uniform_buffer_create_info = {};
    uniform_buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    uniform_buffer_create_info.size = sizeof(_mvp); // size in bytes
    uniform_buffer_create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;   // <-- UBO
    uniform_buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    result = vkCreateBuffer(device(), &uniform_buffer_create_info, nullptr, &_matrices_ubo.buffer);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    Log("#   Get Uniform Buffer Memory Requirements(size and type)\n");
    VkMemoryRequirements uniform_buffer_memory_requirements = {};
    vkGetBufferMemoryRequirements(device(), _matrices_ubo.buffer, &uniform_buffer_memory_requirements);

    VkMemoryAllocateInfo uniform_buffer_allocate_info = {};
    uniform_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    uniform_buffer_allocate_info.allocationSize = uniform_buffer_memory_requirements.size;

    uint32_t uniform_memory_type_bits = uniform_buffer_memory_requirements.memoryTypeBits;
    VkMemoryPropertyFlags uniform_desired_memory_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    for (uint32_t i = 0; i < _renderer->GetVulkanPhysicalDeviceMemoryProperties().memoryTypeCount; ++i)
    {
        VkMemoryType memory_type = _renderer->GetVulkanPhysicalDeviceMemoryProperties().memoryTypes[i];
        if (uniform_memory_type_bits & 1)
        {
            if ((memory_type.propertyFlags & uniform_desired_memory_flags) == uniform_desired_memory_flags) {
                uniform_buffer_allocate_info.memoryTypeIndex = i;
                break;
            }
        }
        uniform_memory_type_bits = uniform_memory_type_bits >> 1;
    }

    Log("#   Allocate Uniform Buffer Memory\n");
    result = vkAllocateMemory(device(), &uniform_buffer_allocate_info, nullptr, &_matrices_ubo.memory);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    Log("#   Bind memory to buffer\n");
    result = vkBindBufferMemory(device(), _matrices_ubo.buffer, _matrices_ubo.memory, 0);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    Log("#   Map Uniform Buffer\n");
    void *mapped = nullptr;
    result = vkMapMemory(device(), _matrices_ubo.memory, 0, VK_WHOLE_SIZE, 0, &mapped);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    Log("#   Copy matrices, first time.\n");
    memcpy(mapped,                 _mvp.m, sizeof(_mvp.m));
    memcpy(((float *)mapped + 16), _mvp.v, sizeof(_mvp.v));
    memcpy(((float *)mapped + 32), _mvp.p, sizeof(_mvp.p));

    Log("#   UnMap Uniform Buffer\n");
    vkUnmapMemory(device(), _matrices_ubo.memory);

    return true;
}

void Window::DeInitUniformBuffer()
{
    Log("#   Free Memory\n");
    vkFreeMemory(device(), _matrices_ubo.memory, nullptr);

    Log("#   Destroy Buffer\n");
    vkDestroyBuffer(device(), _matrices_ubo.buffer, nullptr);
}

bool Window::InitDescriptors()
{
    VkResult result;

    Log("#   Init bindings for the UBO of matrices and texture sampler\n");
    // layout( std140, binding = 0 ) uniform buffer { mat4 m; mat4 v; mat4 p; } UBO;
    std::array<VkDescriptorSetLayoutBinding,2> bindings = {};
    bindings[0].binding = 0; // <---- value used in the shader itself
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    bindings[0].pImmutableSamplers = nullptr;

    // layout ( set = 0, binding = 1 ) uniform sampler2D diffuse_tex_checker_texure._sampler;
    bindings[1].binding = 1; // <---- value used in the shader itself
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[1].pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo set_layout_create_info = {};
    set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    set_layout_create_info.bindingCount = (uint32_t)bindings.size();
    set_layout_create_info.pBindings = bindings.data();

    Log("#   Create Descriptor Set Layout\n");
    result = vkCreateDescriptorSetLayout(device(), &set_layout_create_info, nullptr, &_descriptor_set_layout);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    // allocate just enough for one uniform descriptor set
    std::array<VkDescriptorPoolSize,2> uniform_buffer_pool_sizes = {};
    uniform_buffer_pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniform_buffer_pool_sizes[0].descriptorCount = 1;

    uniform_buffer_pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    uniform_buffer_pool_sizes[1].descriptorCount = 1;

    VkDescriptorPoolCreateInfo pool_create_info = {}; 
    pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_create_info.maxSets = 1;
    pool_create_info.poolSizeCount = (uint32_t)uniform_buffer_pool_sizes.size();
    pool_create_info.pPoolSizes = uniform_buffer_pool_sizes.data();

    Log("#   Create Descriptor Set Pool\n");
    result = vkCreateDescriptorPool(device(), &pool_create_info, nullptr, &_descriptor_pool);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    VkDescriptorSetAllocateInfo descriptor_allocate_info = {};
    descriptor_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_allocate_info.descriptorPool = _descriptor_pool;
    descriptor_allocate_info.descriptorSetCount = 1;
    descriptor_allocate_info.pSetLayouts = &_descriptor_set_layout;

    Log("#   Allocate Descriptor Set\n");
    result = vkAllocateDescriptorSets(device(), &descriptor_allocate_info, &_descriptor_set);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    // When a set is allocated all values are undefined and all 
    // descriptors are uninitialised. must init all statically used bindings:
    VkDescriptorBufferInfo descriptor_uniform_buffer_info = {};
    descriptor_uniform_buffer_info.buffer = _matrices_ubo.buffer;
    descriptor_uniform_buffer_info.offset = 0;
    descriptor_uniform_buffer_info.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet write_descriptor_for_uniform_buffer = {};
    write_descriptor_for_uniform_buffer.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_for_uniform_buffer.dstSet           = _descriptor_set;
    write_descriptor_for_uniform_buffer.dstBinding       = 0;
    write_descriptor_for_uniform_buffer.dstArrayElement  = 0;
    write_descriptor_for_uniform_buffer.descriptorCount  = 1;
    write_descriptor_for_uniform_buffer.descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write_descriptor_for_uniform_buffer.pImageInfo       = nullptr;
    write_descriptor_for_uniform_buffer.pBufferInfo      = &descriptor_uniform_buffer_info;
    write_descriptor_for_uniform_buffer.pTexelBufferView = nullptr;

    Log("#   Update Descriptor Set (UBO)\n");
    vkUpdateDescriptorSets(device(), 1, &write_descriptor_for_uniform_buffer, 0, nullptr );

    VkDescriptorImageInfo descriptor_image_info = {};
    descriptor_image_info.sampler = _checker_texture.sampler;
    descriptor_image_info.imageView = _checker_texture.texture_view;
    descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_PREINITIALIZED; // why ? we did change the layout manually!!
    // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL

    VkWriteDescriptorSet write_descriptor_for_checker_texture_sampler = {};
    write_descriptor_for_checker_texture_sampler.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_for_checker_texture_sampler.dstSet           = _descriptor_set;
    write_descriptor_for_checker_texture_sampler.dstBinding       = 1; // <----- beware
    write_descriptor_for_checker_texture_sampler.dstArrayElement  = 0;
    write_descriptor_for_checker_texture_sampler.descriptorCount  = 1;
    write_descriptor_for_checker_texture_sampler.descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write_descriptor_for_checker_texture_sampler.pImageInfo       = &descriptor_image_info;
    write_descriptor_for_checker_texture_sampler.pBufferInfo      = nullptr;
    write_descriptor_for_checker_texture_sampler.pTexelBufferView = nullptr;

    Log("#   Update Descriptor Set (texture sampler)\n");
    vkUpdateDescriptorSets(device(), 1, &write_descriptor_for_checker_texture_sampler, 0, nullptr);

    return true;
}

void Window::DeInitDescriptors()
{
    Log("#   Destroy Descriptor Pool\n");
    vkDestroyDescriptorPool(device(), _descriptor_pool, nullptr);

    Log("#   Destroy Descriptor Set Layout\n");
    vkDestroyDescriptorSetLayout(device(), _descriptor_set_layout, nullptr);
}

bool Window::InitVertexBuffer()
{
    VkResult result;

    Log("#   Create Vertex Buffer\n");
    VkBufferCreateInfo vertex_buffer_create_info = {};
    vertex_buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vertex_buffer_create_info.size = sizeof(struct vertex) * 3; // size in Bytes
    vertex_buffer_create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    vertex_buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    result = vkCreateBuffer(device(), &vertex_buffer_create_info, nullptr, &_triangle_vbo.buffer);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    Log("#   Get Vertex Buffer Memory Requirements(size and type)\n");
    VkMemoryRequirements vertex_buffer_memory_requirements = {};
    vkGetBufferMemoryRequirements(device(), _triangle_vbo.buffer, &vertex_buffer_memory_requirements);

    VkMemoryAllocateInfo vertex_buffer_allocate_info = {};
    vertex_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    vertex_buffer_allocate_info.allocationSize = vertex_buffer_memory_requirements.size;

    uint32_t vertex_memory_type_bits = vertex_buffer_memory_requirements.memoryTypeBits;
    VkMemoryPropertyFlags vertex_desired_memory_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    for (uint32_t i = 0; i < _renderer->GetVulkanPhysicalDeviceMemoryProperties().memoryTypeCount; ++i)
    {
        VkMemoryType memory_type = _renderer->GetVulkanPhysicalDeviceMemoryProperties().memoryTypes[i];
        if (vertex_memory_type_bits & 1)
        {
            if ((memory_type.propertyFlags & vertex_desired_memory_flags) == vertex_desired_memory_flags) {
                vertex_buffer_allocate_info.memoryTypeIndex = i;
                break;
            }
        }
        vertex_memory_type_bits = vertex_memory_type_bits >> 1;
    }

    Log("#   Allocate Vertex Buffer Memory\n");
    result = vkAllocateMemory(device(), &vertex_buffer_allocate_info, nullptr, &_triangle_vbo.memory);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    Log("#   Map Vertex Buffer\n");
    void *mapped = nullptr;
    result = vkMapMemory(device(), _triangle_vbo.memory, 0, VK_WHOLE_SIZE, 0, &mapped);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    Log("#   Write to Vertex Buffer\n");
    vertex *triangle = (vertex *)mapped;
    // CCW triangle. pointy end at the top. at z = 0.
    vertex v1 = {
        -1.0f, -1.0f, 0.0f, 1.0f, // position
         -1.0f, -1.0f, 1.0f,       // normal
         0.0f,  0.0f              // uv
    };
    vertex v2 = {
        1.0f, -1.0f, -0.0f, 1.0f,  // position
        1.0f, -1.0f, 1.0f,        // normal
        1.0f,  0.0f               // uv
    };
    vertex v3 = {
        0.0f, 1.0f, 0.0f, 1.0f,  // position
        0.0f, 1.0f, 1.0f,        // normal
        0.5f, 1.0f               // uv
    };
    triangle[0] = v1;
    triangle[1] = v2;
    triangle[2] = v3;

    Log("#   UnMap Vertex Buffer\n");
    vkUnmapMemory(device(), _triangle_vbo.memory);

    // AFTER having written to it???
    result = vkBindBufferMemory(device(), _triangle_vbo.buffer, _triangle_vbo.memory, 0);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    return true;
}

void Window::DeInitVertexBuffer()
{
    Log("#   Free Memory\n");
    vkFreeMemory(device(), _triangle_vbo.memory, nullptr);

    Log("#   Destroy Buffer\n");
    vkDestroyBuffer(device(), _triangle_vbo.buffer, nullptr);
}

bool Window::InitFakeImage()
{
    struct loaded_image {
        int width;
        int height;
        void *data;
    };

    loaded_image test_image;
    test_image.width = 800;
    test_image.height = 600;
    test_image.data = (void *) new float[test_image.width * test_image.height * 3];

    for (uint32_t x = 0; x < (uint32_t)test_image.width; ++x) {
        for (uint32_t y = 0; y < (uint32_t)test_image.height; ++y) {
            float g = 0.3f;
            if (x % 40 < 20 && y % 40 < 20) {
                g = 1;
            }
            if (x % 40 >= 20 && y % 40 >= 20) {
                g = 1;
            }

            float *pixel = ((float *)test_image.data) + (x * test_image.height * 3) + (y * 3);
            pixel[0] = g * 0.4f;
            pixel[1] = g * 0.5f;
            pixel[2] = g * 0.7f;
        }
    }

    VkResult result;

    VkImageCreateInfo texture_create_info = {};
    texture_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    texture_create_info.imageType = VK_IMAGE_TYPE_2D;
    texture_create_info.format = VK_FORMAT_R32G32B32_SFLOAT;
    texture_create_info.extent = { (uint32_t)test_image.width, (uint32_t)test_image.height, 1 };
    texture_create_info.mipLevels = 1;
    texture_create_info.arrayLayers = 1;
    texture_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    texture_create_info.tiling = VK_IMAGE_TILING_LINEAR;
    texture_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    texture_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    texture_create_info.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED; // we will fill it so dont flush content when changing layout.

    result = vkCreateImage(device(), &texture_create_info, nullptr, &_checker_texture.texture_image);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    VkMemoryRequirements texture_memory_requirements = {};
    vkGetImageMemoryRequirements(device(), _checker_texture.texture_image, &texture_memory_requirements);

    VkMemoryAllocateInfo texture_image_allocate_info = {};
    texture_image_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    texture_image_allocate_info.allocationSize = texture_memory_requirements.size;

    uint32_t texture_memory_type_bits = texture_memory_requirements.memoryTypeBits;
    VkMemoryPropertyFlags tDesiredMemoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    for (uint32_t i = 0; i < 32; ++i) {
        VkMemoryType memory_type = _renderer->GetVulkanPhysicalDeviceMemoryProperties().memoryTypes[i];
        if (texture_memory_type_bits & 1) {
            if ((memory_type.propertyFlags & tDesiredMemoryFlags) == tDesiredMemoryFlags) {
                texture_image_allocate_info.memoryTypeIndex = i;
                break;
            }
        }
        texture_memory_type_bits = texture_memory_type_bits >> 1;
    }

    result = vkAllocateMemory(device(), &texture_image_allocate_info, nullptr, &_checker_texture.texture_image_memory);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    result = vkBindImageMemory(device(), _checker_texture.texture_image, _checker_texture.texture_image_memory, 0);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    void *image_mapped;
    result = vkMapMemory(device(), _checker_texture.texture_image_memory, 0, VK_WHOLE_SIZE, 0, &image_mapped);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    memcpy(image_mapped, test_image.data, sizeof(float) * test_image.width * test_image.height * 3);

    VkMappedMemoryRange memory_range = {};
    memory_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    memory_range.memory = _checker_texture.texture_image_memory;
    memory_range.offset = 0;
    memory_range.size = VK_WHOLE_SIZE;
    vkFlushMappedMemoryRanges(device(), 1, &memory_range);

    vkUnmapMemory(device(), _checker_texture.texture_image_memory);

    // we can clear the image data:
    delete[] test_image.data;

    // TODO: transition

    VkFence submit_fence = {};
    VkFenceCreateInfo fence_create_info = {};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    vkCreateFence(device(), &fence_create_info, nullptr, &submit_fence);

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(_renderer->GetVulkanCommandBuffer(), &begin_info);

    VkImageMemoryBarrier layout_transition_barrier = {};
    layout_transition_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    layout_transition_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    layout_transition_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    layout_transition_barrier.oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    layout_transition_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    layout_transition_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    layout_transition_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    layout_transition_barrier.image = _checker_texture.texture_image;
    layout_transition_barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

    vkCmdPipelineBarrier(_renderer->GetVulkanCommandBuffer(),
        VK_PIPELINE_STAGE_HOST_BIT, //VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,//VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &layout_transition_barrier);

    vkEndCommandBuffer(_renderer->GetVulkanCommandBuffer());

    VkPipelineStageFlags waitStageMask[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = nullptr;
    submit_info.pWaitDstStageMask = waitStageMask;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &_renderer->GetVulkanCommandBuffer();
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = nullptr;
    result = vkQueueSubmit(_renderer->GetVulkanQueue(), 1, &submit_info, submit_fence);

    vkWaitForFences(device(), 1, &submit_fence, VK_TRUE, UINT64_MAX);
    vkResetFences(device(), 1, &submit_fence);
    vkResetCommandBuffer(_renderer->GetVulkanCommandBuffer(), 0);

    vkDestroyFence(device(), submit_fence, nullptr);

    // TODO: image view
    VkImageViewCreateInfo texture_image_view_create_info = {};
    texture_image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    texture_image_view_create_info.image = _checker_texture.texture_image;
    texture_image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    texture_image_view_create_info.format = VK_FORMAT_R32G32B32_SFLOAT;
    texture_image_view_create_info.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
    texture_image_view_create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    texture_image_view_create_info.subresourceRange.baseMipLevel   = 0;
    texture_image_view_create_info.subresourceRange.levelCount     = 1;
    texture_image_view_create_info.subresourceRange.baseArrayLayer = 0;
    texture_image_view_create_info.subresourceRange.layerCount     = 1;

    result = vkCreateImageView(device(), &texture_image_view_create_info, nullptr, &_checker_texture.texture_view);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    VkSamplerCreateInfo sampler_create_info = {};
    sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_create_info.magFilter = VK_FILTER_LINEAR;
    sampler_create_info.minFilter = VK_FILTER_LINEAR;
    sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_create_info.mipLodBias = 0;
    sampler_create_info.anisotropyEnable = VK_FALSE;
    sampler_create_info.minLod = 0;
    sampler_create_info.maxLod = 5;
    sampler_create_info.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    sampler_create_info.unnormalizedCoordinates = VK_FALSE;

    result = vkCreateSampler(device(), &sampler_create_info, nullptr, &_checker_texture.sampler);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    return true;
}

void Window::DeInitFakeImage()
{
    vkDestroySampler(device(), _checker_texture.sampler, nullptr);
    vkDestroyImageView(device(), _checker_texture.texture_view, nullptr);
    vkDestroyImage(device(), _checker_texture.texture_image, nullptr);
    vkFreeMemory(device(), _checker_texture.texture_image_memory, nullptr);
}

bool Window::InitShaders()
{
    uint32_t codeSize;
    char *code = new char[10000];
    HANDLE fileHandle = nullptr;

    // load our vertex shader:
    fileHandle = ::CreateFile("vert.spv", GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (fileHandle == INVALID_HANDLE_VALUE) {
        Log("!!!!!!!!! Failed to open shader file.\n");
        return false;
    }
    ::ReadFile((HANDLE)fileHandle, code, 10000, (LPDWORD)&codeSize, nullptr);
    ::CloseHandle(fileHandle);

    VkResult result;

    VkShaderModuleCreateInfo vertex_shader_creation_info = {};
    vertex_shader_creation_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertex_shader_creation_info.codeSize = codeSize;
    vertex_shader_creation_info.pCode = (uint32_t *)code;

    result = vkCreateShaderModule(device(), &vertex_shader_creation_info, nullptr, &_vertex_shader_module);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    // load our fragment shader:
    fileHandle = ::CreateFile("frag.spv", GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (fileHandle == INVALID_HANDLE_VALUE) {
        Log("!!!!!!!!! Failed to open shader file.\n"); 
        return false;
    }
    ::ReadFile((HANDLE)fileHandle, code, 10000, (LPDWORD)&codeSize, nullptr);
    ::CloseHandle(fileHandle);

    VkShaderModuleCreateInfo fragment_shader_creation_info = {};
    fragment_shader_creation_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragment_shader_creation_info.codeSize = codeSize;
    fragment_shader_creation_info.pCode = (uint32_t *)code;

    result = vkCreateShaderModule(device(), &fragment_shader_creation_info, nullptr, &_fragment_shader_module);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    return true;
}

void Window::DeInitShaders()
{
    vkDestroyShaderModule(device(), _fragment_shader_module, nullptr);
    vkDestroyShaderModule(device(), _vertex_shader_module, nullptr);
}

bool Window::InitGraphicsPipeline()
{
    VkResult result;

    // use it later to define uniform buffer
    VkPipelineLayoutCreateInfo layout_create_info = {};
    layout_create_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_create_info.setLayoutCount         = 1;
    layout_create_info.pSetLayouts            = &_descriptor_set_layout;
    layout_create_info.pushConstantRangeCount = 0;
    layout_create_info.pPushConstantRanges    = nullptr; // constant into shader for opti???

    Log("#   Fill Pipeline Layout... and everything else.\n");
    result = vkCreatePipelineLayout(device(), &layout_create_info, nullptr, &_pipeline_layout);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    std::array<VkPipelineShaderStageCreateInfo,2> shader_stage_create_infos = {};
    shader_stage_create_infos[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage_create_infos[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
    shader_stage_create_infos[0].module = _vertex_shader_module;
    shader_stage_create_infos[0].pName  = "main";        // shader entry point function name
    shader_stage_create_infos[0].pSpecializationInfo = nullptr;

    shader_stage_create_infos[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage_create_infos[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    shader_stage_create_infos[1].module = _fragment_shader_module;
    shader_stage_create_infos[1].pName  = "main";        // shader entry point function name
    shader_stage_create_infos[1].pSpecializationInfo = nullptr;

    VkVertexInputBindingDescription vertex_binding_description = {};
    vertex_binding_description.binding = 0;
    vertex_binding_description.stride = sizeof(vertex);
    vertex_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    // bind input to location=0, binding=0
    VkVertexInputAttributeDescription vertex_attribute_description[3] = {};
    vertex_attribute_description[0].location = 0;
    vertex_attribute_description[0].binding = 0;
    vertex_attribute_description[0].format = VK_FORMAT_R32G32B32A32_SFLOAT; // position = 4 float
    vertex_attribute_description[0].offset = 0;

    vertex_attribute_description[1].location = 1;
    vertex_attribute_description[1].binding = 0;
    vertex_attribute_description[1].format = VK_FORMAT_R32G32B32_SFLOAT; // normal = 3 floats
    vertex_attribute_description[1].offset = 4*sizeof(float);

    vertex_attribute_description[2].location = 2;
    vertex_attribute_description[2].binding = 0;
    vertex_attribute_description[2].format = VK_FORMAT_R32G32_SFLOAT; // uv = 2 floats
    vertex_attribute_description[2].offset = (4+3)*sizeof(float);

    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {};
    vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
    vertex_input_state_create_info.pVertexBindingDescriptions = &vertex_binding_description;
    vertex_input_state_create_info.vertexAttributeDescriptionCount = 3;
    vertex_input_state_create_info.pVertexAttributeDescriptions = vertex_attribute_description;

    // vertex topology config = triangles
    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {};
    input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = (float)_surface_size_x;
    viewport.height = (float)_surface_size_y;
    viewport.minDepth = 0;
    viewport.maxDepth = 1;

    VkRect2D scissors = {};
    scissors.offset = { 0, 0 };
    scissors.extent = { _surface_size_x, _surface_size_y };

    VkPipelineViewportStateCreateInfo viewport_state_create_info = {};
    viewport_state_create_info.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_create_info.viewportCount = 1;
    viewport_state_create_info.pViewports    = &viewport;
    viewport_state_create_info.scissorCount  = 1;
    viewport_state_create_info.pScissors     = &scissors;

    VkPipelineRasterizationStateCreateInfo raster_state_create_info = {};
    raster_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster_state_create_info.depthClampEnable = VK_FALSE;
    raster_state_create_info.rasterizerDiscardEnable = VK_FALSE;
    raster_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
    raster_state_create_info.cullMode = VK_CULL_MODE_NONE;
    raster_state_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;// VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster_state_create_info.depthBiasEnable = VK_FALSE;
    raster_state_create_info.depthBiasConstantFactor = 0;
    raster_state_create_info.depthBiasClamp = 0;
    raster_state_create_info.depthBiasSlopeFactor = 0;
    raster_state_create_info.lineWidth = 1;

    VkPipelineMultisampleStateCreateInfo multisample_state_create_info = {};
    multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample_state_create_info.sampleShadingEnable = VK_FALSE;
    multisample_state_create_info.minSampleShading = 0;
    multisample_state_create_info.pSampleMask = nullptr;
    multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
    multisample_state_create_info.alphaToOneEnable = VK_FALSE;

    VkStencilOpState noOP_stencil_state = {};
    noOP_stencil_state.failOp = VK_STENCIL_OP_KEEP;
    noOP_stencil_state.passOp = VK_STENCIL_OP_KEEP;
    noOP_stencil_state.depthFailOp = VK_STENCIL_OP_KEEP;
    noOP_stencil_state.compareOp = VK_COMPARE_OP_ALWAYS;
    noOP_stencil_state.compareMask = 0;
    noOP_stencil_state.writeMask = 0;
    noOP_stencil_state.reference = 0;

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info = {};
    depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state_create_info.depthTestEnable = VK_TRUE;//VK_FALSE;// VK_TRUE;
    depth_stencil_state_create_info.depthWriteEnable = VK_TRUE;//VK_FALSE;// VK_TRUE;
    depth_stencil_state_create_info.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state_create_info.stencilTestEnable = VK_FALSE;
    depth_stencil_state_create_info.front = noOP_stencil_state;
    depth_stencil_state_create_info.back = noOP_stencil_state;
    depth_stencil_state_create_info.minDepthBounds = 0;
    depth_stencil_state_create_info.maxDepthBounds = 0;

    VkPipelineColorBlendAttachmentState color_blend_attachment_state = {};
    color_blend_attachment_state.blendEnable = VK_FALSE;
    color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
    color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
    color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment_state.colorWriteMask = 0xf; // all components.

    VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = {};
    color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state_create_info.logicOpEnable = VK_FALSE;
    color_blend_state_create_info.logicOp = VK_LOGIC_OP_CLEAR;
    color_blend_state_create_info.attachmentCount = 1;
    color_blend_state_create_info.pAttachments = &color_blend_attachment_state;
    color_blend_state_create_info.blendConstants[0] = 0.0;
    color_blend_state_create_info.blendConstants[1] = 0.0;
    color_blend_state_create_info.blendConstants[2] = 0.0;
    color_blend_state_create_info.blendConstants[3] = 0.0;

    std::array<VkDynamicState,2> dynamic_state = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {};
    dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_create_info.dynamicStateCount = (uint32_t)dynamic_state.size();
    dynamic_state_create_info.pDynamicStates = dynamic_state.data();

    std::array<VkGraphicsPipelineCreateInfo, 1> pipeline_create_infos = {};
    pipeline_create_infos[0].sType                 = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_create_infos[0].stageCount            = (uint32_t)shader_stage_create_infos.size();
    pipeline_create_infos[0].pStages               = shader_stage_create_infos.data();
    pipeline_create_infos[0].pVertexInputState     = &vertex_input_state_create_info;
    pipeline_create_infos[0].pInputAssemblyState   = &input_assembly_state_create_info;
    pipeline_create_infos[0].pTessellationState    = nullptr;
    pipeline_create_infos[0].pViewportState        = &viewport_state_create_info;
    pipeline_create_infos[0].pRasterizationState   = &raster_state_create_info;
    pipeline_create_infos[0].pMultisampleState     = &multisample_state_create_info;
    pipeline_create_infos[0].pDepthStencilState    = &depth_stencil_state_create_info;
    pipeline_create_infos[0].pColorBlendState      = &color_blend_state_create_info;
    pipeline_create_infos[0].pDynamicState         = &dynamic_state_create_info;
    pipeline_create_infos[0].layout                = _pipeline_layout;
    pipeline_create_infos[0].renderPass            = _render_pass;
    pipeline_create_infos[0].subpass               = 0;
    pipeline_create_infos[0].basePipelineHandle    = VK_NULL_HANDLE; // only if VK_PIPELINE_CREATE_DERIVATIVE flag is set.
    pipeline_create_infos[0].basePipelineIndex     = 0;

    Log("#   Create Pipeline\n");
    result = vkCreateGraphicsPipelines(
        device(),
        VK_NULL_HANDLE, // cache
        (uint32_t)pipeline_create_infos.size(),
        pipeline_create_infos.data(),
        nullptr,
        _pipelines.data());
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    return true;
}

void Window::DeInitGraphicsPipeline()
{
    for (auto & pipeline : _pipelines)
    {
        vkDestroyPipeline(device(), pipeline, nullptr);
    }
    vkDestroyPipelineLayout(device(), _pipeline_layout, nullptr);
}

void Window::set_object_position(float x, float y, float z)
{
    _mvp.m[12]  = x;
    _mvp.m[13]  = y;
    _mvp.m[14] = z;
}

void Window::set_camera_position(float x, float y, float z)
{
    _mvp.v[12]  = -x;
    _mvp.v[13]  = -y;
    _mvp.v[14] = -z;
}

void Window::update_matrices_ubo()
{
    void *matrix_mapped;
    vkMapMemory(device(), _matrices_ubo.memory, 0, VK_WHOLE_SIZE, 0, &matrix_mapped);

    memcpy(matrix_mapped,                 _mvp.m, sizeof(_mvp.m));
    memcpy(((float *)matrix_mapped + 16), _mvp.v, sizeof(_mvp.v));
    memcpy(((float *)matrix_mapped + 32), _mvp.p, sizeof(_mvp.p));

    /*
    memcpy(matrix_mapped + offsetof(_mvp, m), _mvp.m, sizeof(_mvp.m));
    memcpy(matrix_mapped + offsetof(_mvp, v), _mvp.v, sizeof(_mvp.v));
    memcpy(matrix_mapped + offsetof(_mvp, p), _mvp.p, sizeof(_mvp.p));
    */

    VkMappedMemoryRange memory_range = {};
    memory_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    memory_range.memory = _matrices_ubo.memory;
    memory_range.offset = 0;
    memory_range.size = VK_WHOLE_SIZE;
    vkFlushMappedMemoryRanges(device(), 1, &memory_range);

    vkUnmapMemory(device(), _matrices_ubo.memory);
}
