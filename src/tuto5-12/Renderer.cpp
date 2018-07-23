#include "build_options.h"

#include "platform.h"
#include "Renderer.h"
#include "Shared.h"
#include "window.h"
#include "scene.h"


#define GLM_FORCE_DEPTH_ZERO_TO_ONE 1 // clip space z [0..1] instead of [-1..1]
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> // glm::perspective
#include <glm/gtc/type_ptr.hpp> // glm::value_ptr

#include <cstdlib>
#include <assert.h>
#include <vector>
#include <array>
#include <iostream>
#include <sstream>

Renderer::Renderer(Window *w) : _w(w)
{
    
}

Renderer::~Renderer()
{
    //
    // SCENE
    //
    Log("#   Destroy Scene Vulkan Specifics\n");
    DeInitSceneVulkan();
    
    //
    //
    //
    if (_w)
    {
        Log("#   Destroy Vulkan Window Specifics\n");
        _w->DeInitVulkanWindowSpecifics(&_ctx);
    }

    //
    //
    //

    Log("#   Destroy Vma\n");
    DeInitVma();

    Log("#   Destroy Device\n");
    DeInitDevice();

    Log("#   Destroy Debug\n");
    DeInitDebug();

    Log("#   Destroy Instance\n");
    DeInitInstance();
}

bool Renderer::InitContext()
{
    // Manually load the dll, and grab the "vkGetInstanceProcAddr" symbol,
    // vkCreateInstance, and vkEnumerate extensions and layers
    Log("#   volkInitialize.\n");
    if (volkInitialize() != VK_SUCCESS)
        return false;

    // Setup debug callback structure.
    Log("#   Setup Debug\n");
    SetupDebug();

    // Fill the names of desired layers
    Log("#   Setup Layers\n");
    SetupLayers();

    // Fill the names of the desired extensions.
    Log("#   Setup Extensions\n");
    SetupExtensions();

    // Create the instance
    Log("#   Init Instance\n");
    if (!InitInstance())
        return false;

    // Loads all the symbols for that instance, beginning with vkCreateDevice.
    Log("#   Load instance related function ptrs (volkLoadInstance).\n");
    volkLoadInstance(_ctx.instance);

    // Install debug callback
    Log("#   Install debug callback\n");
    if (!InitDebug())
        return false;

    // Create device and get rendering queue.
    Log("#   Init device\n");
    if (!InitDevice())
        return false;

    // Load all the rest of the symbols, specifically for that device, bypassing
    // the loader dispatch,
    Log("#   Load device related function ptrs (volkLoadDevice).\n");
    volkLoadDevice(_ctx.device);

    Log("#   Init VMA\n");
    InitVma();

    //
    //
    //

    Log("#   Init Window Specific Vulkan\n");
    if (_w)
    {
        _w->InitVulkanWindowSpecifics(&_ctx);
        _global_viewport = _w->surface_size();
    }

    //
    // SCENE
    //
    Log("#   Init Scene Specific Vulkan\n");
    if (!InitSceneVulkan())
        return false;

    return true;
}

bool Renderer::InitSceneVulkan()
{
    Log("#    Init CommandBuffer\n");
    if (!InitCommandBuffer())
        return false;

    Log("#    Init Depth/Stencil\n");
    if (!InitDepthStencilImage())
        return false;

    Log("#    Init Render Pass\n");
    if (!InitRenderPass())
        return false;

    Log("#    Init FrameBuffers\n");
    if (!InitSwapChainFrameBuffers())
        return false;

    Log("#    Init Uniform Buffer\n");
    if (!InitUniformBuffer())
        return false;

    Log("#    Init FakeImage\n");
    if (!InitFakeImage())
        return false;

    Log("#    Init Descriptors\n");
    if (!InitDescriptors())
        return false;

    Log("#    Init Shaders\n");
    if (!InitShaders())
        return false;

    Log("#    Init Graphics Pipeline\n");
    if (!InitGraphicsPipeline())
        return false;

    return true;
}

void Renderer::DeInitSceneVulkan()
{
    vkQueueWaitIdle(_ctx.graphics.queue);

    Log("#   Destroy Graphics Pipeline\n");
    DeInitGraphicsPipeline();

    Log("#   Destroy Shaders\n");
    DeInitShaders();

    Log("#   Destroy Descriptors\n");
    DeInitDescriptors();

    Log("#   Destroy FakeImage\n");
    DeInitFakeImage();

    Log("#   Destroy Uniform Buffer\n");
    DeInitUniformBuffer();

    Log("#   Destroy FrameBuffers\n");
    DeInitSwapChainFrameBuffers();

    Log("#   Destroy Render Pass\n");
    DeInitRenderPass();

    Log("#   Destroy Depth/Stencil\n");
    DeInitDepthStencilImage();

    Log("#   Destroy Command Buffer\n");
    DeInitCommandBuffer();
}

bool Renderer::InitInstance()
{
    VkResult result;

    VkApplicationInfo application_info{};
    application_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.apiVersion         = VK_API_VERSION_1_0;//VK_MAKE_VERSION( 1, 1, 73 );
    application_info.applicationVersion = VK_MAKE_VERSION( 0, 0, 4 );
    application_info.pApplicationName   = "Vulkan Renderer";

    VkInstanceCreateInfo instance_create_info{};
    instance_create_info.sType                      = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pApplicationInfo           = &application_info;
    instance_create_info.enabledLayerCount          = (uint32_t)_ctx.instance_layers.size();
    instance_create_info.ppEnabledLayerNames        = _ctx.instance_layers.data();
    instance_create_info.enabledExtensionCount      = (uint32_t)_ctx.instance_extensions.size();
    instance_create_info.ppEnabledExtensionNames    = _ctx.instance_extensions.data();
    instance_create_info.pNext                      = &_ctx.debug_callback_create_info; // put it here to have debug info for the vkCreateInstance function, even if we have not given a debug callback yet.

    result = vkCreateInstance(&instance_create_info, nullptr, &_ctx.instance );

    // 0 = OK
    // positive error code = partial succes
    // negative = failure
    ErrorCheck( result );
    if (result != VK_SUCCESS)
        return false;

    return true;
}

void Renderer::DeInitInstance()
{
    vkDestroyInstance( _ctx.instance, nullptr );
    _ctx.instance = nullptr;
}

// NOTE(nfauvet): a device in Vulkan is like the context in OpenGL
bool Renderer::InitDevice()
{
    VkResult result = VK_SUCCESS;

    // Get all physical devices and choose one (the first here)
    {
        Log("#    Enumerate Physical Device\n");
        // Call once to get the number
        uint32_t gpu_count = 0;
        result = vkEnumeratePhysicalDevices(_ctx.instance, &gpu_count, nullptr);
        ErrorCheck(result);
        if (gpu_count == 0)
        {
            assert(!"Vulkan ERROR: No GPU found.");
            return false;
        }

        Log(std::string("#    -> found ") + std::to_string(gpu_count) + std::string(" physical devices.\n"));

        // Call a second time to get the actual devices
        std::vector<VkPhysicalDevice> gpu_list( gpu_count );
        result = vkEnumeratePhysicalDevices(_ctx.instance, &gpu_count, gpu_list.data());
        ErrorCheck(result); // if it has passed the first time, it wont fail the second.
        if (result != VK_SUCCESS)
            return false;

        // Take the first
        _ctx.physical_device = gpu_list[0]; // pas forcement!

        Log("#    Get Physical Device Properties\n");
        vkGetPhysicalDeviceProperties(_ctx.physical_device, &_ctx.physical_device_properties);

        Log("#    Get Physical Device Memory Properties\n");
        vkGetPhysicalDeviceMemoryProperties(_ctx.physical_device, &_ctx.physical_device_memory_properties);
    }

    // Get the "Queue Family Properties" of the Device
    {
        Log("#    Get Physical Device Queue Family Properties\n");

        uint32_t family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(_ctx.physical_device, &family_count, nullptr);
        if (family_count == 0)
        {
            assert(!"Vulkan ERROR: No Queue family!!");
            return false;
        }
        Log(std::string("#     -> found ") + std::to_string(family_count) + std::string(" queue families.\n"));
        std::vector<VkQueueFamilyProperties> family_property_list(family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(_ctx.physical_device, &family_count, family_property_list.data());

        // Look for a queue family supporting graphics operations.
        bool found_graphics = false;
        bool found_compute = false; 
        bool found_transfer = false; 
        bool found_present = false;
        for ( uint32_t i = 0; i < family_count; ++i )
        {
            // to know if support for presentation on windows desktop, even not knowing about the surface.
            VkBool32 supportsPresentation = vkGetPhysicalDeviceWin32PresentationSupportKHR(_ctx.physical_device, i);
            
            if ( family_property_list[i].queueFlags & VK_QUEUE_GRAPHICS_BIT )
            {
                if (!found_graphics)
                {
                    Log(std::string("#     FOUND Graphics queue: ") + std::to_string(i) + std::string("\n"));
                    found_graphics = true;
                    _ctx.graphics.family_index = i;
                }
            }

            if (family_property_list[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
            {
                if (!found_compute)
                {
                    Log(std::string("#     FOUND Compute queue: ") + std::to_string(i) + std::string("\n"));
                    found_compute = true;
                    _ctx.compute.family_index = i;
                }
            }

            if (family_property_list[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
            {
                if (!found_transfer)
                {
                    Log(std::string("#     FOUND Transfer queue: ") + std::to_string(i) + std::string("\n"));
                    found_transfer = true;
                    _ctx.transfer.family_index = i;
                }
            }

            if (supportsPresentation)
            {
                if (!found_present)
                {
                    Log(std::string("#     FOUND Present queue: ") + std::to_string(i) + std::string("\n"));
                    found_present = true;
                    _ctx.present.family_index = i;
                }
            }
        }

        // TODO: faire un truc intelligent pour fusionner les queue, ou le contraire,
        // s'assurer qu'elles sont differentes pour faire les choses en parallele.
        if ( !found_graphics )
        {
            assert( !"Vulkan ERROR: Queue family supporting graphics not found." );
            return false;
        }
    }

    // Instance Layer Properties
    {
        Log("#    Enumerate Instance Layer Properties: (or not)\n");

        uint32_t layer_count = 0;
        result = vkEnumerateInstanceLayerProperties(&layer_count, nullptr); // first call = query number
        ErrorCheck(result);
        if (result != VK_SUCCESS)
            return false;

        std::vector<VkLayerProperties> layer_property_list( layer_count );
        result = vkEnumerateInstanceLayerProperties(&layer_count, layer_property_list.data()); // second call with allocated array
        ErrorCheck(result);
        if (result != VK_SUCCESS)
            return false;
#if 0
        Log("Instance layers: \n");
        for ( auto &i : layer_property_list )
        {
            std::ostringstream oss;
            oss << "#    " << i.layerName << "\t\t | " << i.description << std::endl;
            std::string oss_str = oss.str();
            Log(oss_str.c_str());
        }
        Log("\n");
#endif
    }


    // Device Layer Properties (deprecated)
    {
        Log("#    Enumerate Device Layer Properties: (or not)\n");

        uint32_t layer_count = 0;
        result = vkEnumerateDeviceLayerProperties(_ctx.physical_device, &layer_count, nullptr); // first call = query number
        ErrorCheck(result);
        if (result != VK_SUCCESS)
            return false;

        std::vector<VkLayerProperties> layer_property_list( layer_count );
        result = vkEnumerateDeviceLayerProperties(_ctx.physical_device, &layer_count, layer_property_list.data()); // second call with allocated array
        ErrorCheck(result);
        if (result != VK_SUCCESS)
            return false;
#if 0
        Log("Device layers: (deprecated)\n");
        for ( auto &i : layer_property_list )
        {
            std::ostringstream oss;
            oss << "#    " << i.layerName << "\t\t | " << i.description << std::endl;
            std::string oss_str = oss.str();
            Log(oss_str.c_str());
        }
        Log("\n");
#endif
    }

    // TODO: create as many queues as needed for compute, tranfer, present and graphics.

    float queue_priorities[]{ 1.0f }; // priorities are float from 0.0f to 1.0f
    VkDeviceQueueCreateInfo device_queue_create_info = {};
    device_queue_create_info.sType              = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    device_queue_create_info.queueFamilyIndex   = _ctx.graphics.family_index;
    device_queue_create_info.queueCount         = 1;
    device_queue_create_info.pQueuePriorities   = queue_priorities;

    // Create a logical "device" associated with the physical "device"
    VkDeviceCreateInfo device_create_info = {};
    device_create_info.sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.queueCreateInfoCount = 1;
    device_create_info.pQueueCreateInfos    = &device_queue_create_info;
    //device_create_info.enabledLayerCount = _ctx.device_layers.size(); // deprecated
    //device_create_info.ppEnabledLayerNames = _ctx.device_layers.data(); // deprecated
    device_create_info.enabledExtensionCount   = (uint32_t)_ctx.device_extensions.size();
    device_create_info.ppEnabledExtensionNames = _ctx.device_extensions.data();

    Log("#    Create Device\n");
    result = vkCreateDevice(_ctx.physical_device, &device_create_info, nullptr, &_ctx.device);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    // get first queue in family (index 0)
    // TODO: get N queues for N threads?
    Log("#    Get Graphics Queue\n");
    vkGetDeviceQueue(_ctx.device, _ctx.graphics.family_index, 0, &_ctx.graphics.queue );

    return true;
}

void Renderer::DeInitDevice()
{
    vkDestroyDevice( _ctx.device, nullptr );
    _ctx.device = nullptr;
}

#if BUILD_ENABLE_VULKAN_DEBUG

VKAPI_ATTR VkBool32 VKAPI_CALL
VulkanDebugCallback(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT obj_type,
    uint64_t src_obj, // pointer to object
    size_t location, // ?
    int32_t msg_code, // related to flags?
    const char *layer_prefix,
    const char *msg,
    void *user_data )
{
    std::ostringstream oss;
    oss << "# ";
    if ( flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT )
    {
        oss << "[INFO]:  ";
    }
    if ( flags & VK_DEBUG_REPORT_WARNING_BIT_EXT )
    {
        oss << "[WARN]:  ";
    }
    if ( flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT )
    {
        oss << "[PERF]:  ";
    }
    if ( flags & VK_DEBUG_REPORT_ERROR_BIT_EXT )
    {
        oss << "[ERROR]: ";
    }
    if ( flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT )
    {
        oss << "[DEBUG]: ";
    }

    oss << "@[" << layer_prefix << "]: ";
    oss << msg << std::endl;

    std::string s = oss.str();
    Log(s.c_str());

#ifdef _WIN32
    if ( flags & VK_DEBUG_REPORT_ERROR_BIT_EXT )
    {
        ::MessageBoxA( NULL, s.c_str(), "Vulkan Error!", 0 );
    }
#endif

    return false; // -> do not stop the command causing the error.
}

void Renderer::SetupLayers()
{
    //_ctx.instance_layers.push_back("VK_LAYER_LUNARG_api_dump" );
    //_ctx.instance_layers.push_back("VK_LAYER_LUNARG_assistant_layer");
    _ctx.instance_layers.push_back("VK_LAYER_LUNARG_core_validation");
    //_ctx.instance_layers.push_back("VK_LAYER_LUNARG_device_simulation");
    //_ctx.instance_layers.push_back("VK_LAYER_LUNARG_monitor" );
    _ctx.instance_layers.push_back("VK_LAYER_LUNARG_object_tracker");
    _ctx.instance_layers.push_back("VK_LAYER_LUNARG_parameter_validation");
    //_ctx.instance_layers.push_back("VK_LAYER_LUNARG_screenshot" );
    _ctx.instance_layers.push_back("VK_LAYER_LUNARG_standard_validation");
    //_ctx.instance_layers.push_back("VK_LAYER_LUNARG_swapchain" ); // pas sur mon portable. deprecated?
    _ctx.instance_layers.push_back("VK_LAYER_GOOGLE_threading");
    _ctx.instance_layers.push_back("VK_LAYER_GOOGLE_unique_objects");
    //_ctx.instance_layers.push_back("VK_LAYER_LUNARG_vktrace" );
    //_ctx.instance_layers.push_back("VK_LAYER_NV_optimus" );
    //_ctx.instance_layers.push_back("VK_LAYER_RENDERDOC_Capture" );
    //_ctx.instance_layers.push_back("VK_LAYER_VALVE_steam_overlay" );

    // DEPRECATED
    //_ctx.device_layers.push_back("VK_LAYER_NV_optimus"); // | NVIDIA Optimus layer
    //_ctx.device_layers.push_back("VK_LAYER_LUNARG_core_validation"); // | LunarG Validation Layer
    //_ctx.device_layers.push_back("VK_LAYER_LUNARG_object_tracker"); // | LunarG Validation Layer
    //_ctx.device_layers.push_back("VK_LAYER_LUNARG_parameter_validation"); // | LunarG Validation Layer
    //_ctx.device_layers.push_back("VK_LAYER_LUNARG_standard_validation"); // | LunarG Standard Validation
    //_ctx.device_layers.push_back("VK_LAYER_GOOGLE_threading"); // | Google Validation Layer
    //_ctx.device_layers.push_back("VK_LAYER_GOOGLE_unique_objects"); // | Google Validation Layer
}

void Renderer::SetupExtensions()
{
    _ctx.instance_extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    _ctx.instance_extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    _ctx.instance_extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);

    _ctx.device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}

void Renderer::SetupDebug()
{
    // moved as a member of class and created here to be able to pass it to instance_create_info
    // and be able to debug the VkCreateInstance function.
    //VkDebugReportCallbackCreateInfoEXT debug_callback_create_info{};
    _ctx.debug_callback_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    _ctx.debug_callback_create_info.pfnCallback = &VulkanDebugCallback;
    _ctx.debug_callback_create_info.flags =
        //VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
        VK_DEBUG_REPORT_WARNING_BIT_EXT |
        VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
        VK_DEBUG_REPORT_ERROR_BIT_EXT |
        //VK_DEBUG_REPORT_DEBUG_BIT_EXT
        0;
}

bool Renderer::InitDebug()
{
    auto result = vkCreateDebugReportCallbackEXT(_ctx.instance, &_ctx.debug_callback_create_info, nullptr, &_ctx.debug_report);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    return true;
}

void Renderer::DeInitDebug()
{
    vkDestroyDebugReportCallbackEXT( _ctx.instance, _ctx.debug_report, nullptr );
    _ctx.debug_report = nullptr;
}

#else

void Renderer::SetupDebug() {}
bool Renderer::InitDebug() { return true; }
void Renderer::DeInitDebug() {}

#endif // BUILD_ENABLE_VULKAN_DEBUG

bool Renderer::InitVma()
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
    allocator_info.physicalDevice = _ctx.physical_device;
    allocator_info.device = _ctx.device;
    allocator_info.pVulkanFunctions = &vulkan_functions;

    VkResult result = vmaCreateAllocator(&allocator_info, &_allocator);
    return(result == VK_SUCCESS);
}

void Renderer::DeInitVma()
{
    vmaDestroyAllocator(_allocator);
}

void Renderer::Draw(float dt)
{
    static float obj_x = 0.0f;
    static float obj_y = 0.0f;
    static float obj_z = 0.0f;
    static float accum = 0.0f; // in seconds

    accum += dt;
    float speed = 3.0f; // radian/sec
    float radius = 2.0f;
    obj_x = radius * std::cos(speed * accum);
    obj_y = radius * std::sin(speed * accum);

#if 0
    // animate camera
    if (_camera.cameraZ < 1)
    {
        _camera.cameraZ = 1;
        _camera.cameraZDir = 1;
    }
    else if (_camera.cameraZ > 10)
    {
        _camera.cameraZ = 10;
        _camera.cameraZDir = -1;
    }

    _camera.cameraZ += _camera.cameraZDir * 0.01f;
#else
    _camera.cameraZ = -10.0f;
#endif

    // Set and upload new matrices
    set_object_position(obj_x, obj_y, obj_z);
    //_window->set_camera_position(0.0f, 0.0f, _camera.cameraZ);
    update_matrices_ubo();

    VkResult result;

    // Re create each time??? cant we reset them???
    //Log("# Create the \"render complete\" and \"present complete\" semaphores\n");
    VkSemaphore render_complete_semaphore = VK_NULL_HANDLE;
    VkSemaphore present_complete_semaphore = VK_NULL_HANDLE;
    VkSemaphoreCreateInfo semaphore_create_info = {};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    result = vkCreateSemaphore(_ctx.device, &semaphore_create_info, nullptr, &render_complete_semaphore);
    ErrorCheck(result);
    result = vkCreateSemaphore(_ctx.device, &semaphore_create_info, nullptr, &present_complete_semaphore);
    ErrorCheck(result);

    // Begin render (acquire image, wait for queue ready)
    _w->BeginRender(present_complete_semaphore);

    auto &cmd = _ctx.graphics.command_buffer;

    // Record command buffer
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    result = vkBeginCommandBuffer(cmd, &begin_info);
    ErrorCheck(result);
    {
        // barrier for reading from uniform buffer after all writing is done:
        VkMemoryBarrier uniform_memory_barrier = {};
        uniform_memory_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        uniform_memory_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT; // the vkFlushMappedMemoryRanges is a "host" command.
        uniform_memory_barrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;

        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_HOST_BIT,
            VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,//VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            0,
            1, &uniform_memory_barrier,
            0, nullptr,
            0, nullptr);

        VkRect2D render_area = {};
        render_area.offset = { 0, 0 };
        render_area.extent = _w->surface_size();

        // NOTE: these values are used only if the attachment has the loadOp LOAD_OP_CLEAR
        std::array<VkClearValue, 2> clear_values = {};
        clear_values[0].depthStencil.depth = 1.0f;
        clear_values[0].depthStencil.stencil = 0;
        clear_values[1].color.float32[0] = 1.0f; // R // backbuffer is of type B8G8R8A8_UNORM
        clear_values[1].color.float32[1] = 0.0f; // G
        clear_values[1].color.float32[2] = 0.0f; // B
        clear_values[1].color.float32[3] = 1.0f; // A

        VkRenderPassBeginInfo render_pass_begin_info = {};
        render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_begin_info.renderPass = _render_pass;
        render_pass_begin_info.framebuffer = _swapchain_framebuffers[_w->active_swapchain_image_id()];
        render_pass_begin_info.renderArea = render_area;
        render_pass_begin_info.clearValueCount = (uint32_t)clear_values.size();
        render_pass_begin_info.pClearValues = clear_values.data();

        vkCmdBeginRenderPass(cmd, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
        {
            // TODO: put into window, too many get...
            // w->BindPipeline(command_buffer)
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelines[0]);

            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline_layout, 0, 1, &_descriptor_set, 0, nullptr);

            // take care of dynamic state:
            VkViewport viewport = { 0, 0, (float)_global_viewport.width, (float)_global_viewport.height, 0, 1 };
            vkCmdSetViewport(cmd, 0, 1, &viewport);

            VkRect2D scissor = { 0, 0, _global_viewport.width, _global_viewport.height };
            vkCmdSetScissor(cmd, 0, 1, &scissor);

            for (auto *s : _scenes)
            {
                s->draw_all_objects(cmd);
            }
        }
        vkCmdEndRenderPass(cmd);

#if 0 // NO NEED to transition at the end, if already specified in the render pass.
        // Transition color from OPTIMAL to PRESENT
        VkImageMemoryBarrier pre_present_layout_transition_barrier = {};
        pre_present_layout_transition_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        pre_present_layout_transition_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        pre_present_layout_transition_barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        pre_present_layout_transition_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED; //VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        pre_present_layout_transition_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        pre_present_layout_transition_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        pre_present_layout_transition_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        pre_present_layout_transition_barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        pre_present_layout_transition_barrier.image = w->GetVulkanActiveImage();

        vkCmdPipelineBarrier(command_buffer,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &pre_present_layout_transition_barrier);
#endif
    }
    result = vkEndCommandBuffer(cmd); // compiles the command buffer
    ErrorCheck(result);

    VkFence render_fence = {};
    VkFenceCreateInfo fence_create_info = {};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    vkCreateFence(_ctx.device, &fence_create_info, nullptr, &render_fence);

    // Submit command buffer
    VkPipelineStageFlags wait_stage_mask[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }; // VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT ??
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &present_complete_semaphore;
    submit_info.pWaitDstStageMask = wait_stage_mask;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd;
    submit_info.signalSemaphoreCount = 1; // signals this semaphore when the render is complete GPU side.
    submit_info.pSignalSemaphores = &render_complete_semaphore;

    result = vkQueueSubmit(_ctx.graphics.queue, 1, &submit_info, render_fence);
    ErrorCheck(result);

    // <------------------------------------------------- Wait on Fence

    vkWaitForFences(_ctx.device, 1, &render_fence, VK_TRUE, UINT64_MAX);
    vkDestroyFence(_ctx.device, render_fence, nullptr);

    // <------------------------------------------------- Wait on semaphores before presenting

    _w->EndRender({ render_complete_semaphore });

    vkDestroySemaphore(_ctx.device, render_complete_semaphore, nullptr);
    vkDestroySemaphore(_ctx.device, present_complete_semaphore, nullptr);
}





//
// SCENE
//





bool Renderer::InitCommandBuffer()
{
    VkResult result;

    Log("#  Create Graphics Command Pool\n");
    VkCommandPoolCreateInfo pool_create_info = {};
    pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_create_info.queueFamilyIndex = _ctx.graphics.family_index;
    pool_create_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | // commands will be short lived, might be reset of freed often.
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // we are going to reset

    result = vkCreateCommandPool(_ctx.device, &pool_create_info, nullptr, &_ctx.graphics.command_pool);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    Log("#  Allocate 1 Graphics Command Buffer\n");

    VkCommandBufferAllocateInfo command_buffer_allocate_info{};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.commandPool = _ctx.graphics.command_pool;
    command_buffer_allocate_info.commandBufferCount = 1;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // primary can be pushed to a queue manually, secondary cannot.

    result = vkAllocateCommandBuffers(_ctx.device, &command_buffer_allocate_info, &_ctx.graphics.command_buffer);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    return true;
}

void Renderer::DeInitCommandBuffer()
{
    Log("#  Destroy Graphics Command Pool\n");
    vkDestroyCommandPool(_ctx.device, _ctx.graphics.command_pool, nullptr);
}

bool Renderer::InitDepthStencilImage()
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

        Log("#     Scan Potential Formats Optimal Tiling... Get Physical Device Format Properties\n");
        for (auto f : potential_formats)
        {
            // VkFormatProperties2 ??? 
            VkFormatProperties format_properties = {};
            // vkGetPhysicalDeviceFormatProperties2 ???
            vkGetPhysicalDeviceFormatProperties(_ctx.physical_device, f, &format_properties);
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

    Log("#     Create Depth/Stencil Image\n");
    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = _depth_stencil_format;
    image_create_info.extent.width = _w->surface_size().width;
    image_create_info.extent.height = _w->surface_size().height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT; // of doing multi sampling, put the same here as in the swapchain.
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL; // use gpu tiling
    image_create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.queueFamilyIndexCount = VK_QUEUE_FAMILY_IGNORED;
    image_create_info.pQueueFamilyIndices = nullptr;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // will have to change layout after

                                                                 // NOTE(nfauvet): only create a handle to the image, like glGenTexture.
    result = vkCreateImage(_ctx.device, &image_create_info, nullptr, &_depth_stencil_image);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    Log("#     Get Image Memory Requirements\n");
    VkMemoryRequirements memory_requirements = {};
    vkGetImageMemoryRequirements(_ctx.device, _depth_stencil_image, &memory_requirements);

    Log("#     Find Memory Type Index\n");
    auto gpu_memory_properties = _ctx.physical_device_memory_properties;
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

    Log("#     Allocate Memory\n");
    result = vkAllocateMemory(_ctx.device, &memory_allocate_info, nullptr, &_depth_stencil_image_memory);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    Log("#     Bind Image Memory\n");
    result = vkBindImageMemory(_ctx.device, _depth_stencil_image, _depth_stencil_image_memory, 0);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    Log("#     Create Image View\n");
    VkImageViewCreateInfo image_view_create_info = {};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.image = _depth_stencil_image;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format = _depth_stencil_format;
    image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | (_stencil_available ? VK_IMAGE_ASPECT_STENCIL_BIT : 0);
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount = 1;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount = 1;

    result = vkCreateImageView(_ctx.device, &image_view_create_info, nullptr, &_depth_stencil_image_view);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    return true;
}

void Renderer::DeInitDepthStencilImage()
{
    Log("#   Destroy Image View\n");
    vkDestroyImageView(_ctx.device, _depth_stencil_image_view, nullptr);

    Log("#   Free Memory\n");
    vkFreeMemory(_ctx.device, _depth_stencil_image_memory, nullptr);

    Log("#   Destroy Image\n");
    vkDestroyImage(_ctx.device, _depth_stencil_image, nullptr);
}

#define ATTACH_INDEX_DEPTH 0
#define ATTACH_INDEX_COLOR 1

bool Renderer::InitRenderPass()
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
        attachements[ATTACH_INDEX_COLOR].format = _w->surface_format(); // bc we are rendering directly to the screen
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
    subpass_0_depth_attachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    std::array<VkAttachmentReference, 1> subpass_0_color_attachments = {};
    subpass_0_color_attachments[0].attachment = ATTACH_INDEX_COLOR;
    subpass_0_color_attachments[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    Log("#   Define SubPasses\n");

    std::array<VkSubpassDescription, 1> subpasses = {};
    {
        subpasses[0].flags = 0;
        subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // or compute
        subpasses[0].inputAttachmentCount = 0; // used for example if we are in the second subpass and it needs something from the first subpass
        subpasses[0].pInputAttachments = nullptr;
        subpasses[0].colorAttachmentCount = (uint32_t)subpass_0_color_attachments.size();
        subpasses[0].pColorAttachments = subpass_0_color_attachments.data();
        subpasses[0].pResolveAttachments = nullptr;
        subpasses[0].pDepthStencilAttachment = &subpass_0_depth_attachment;
        subpasses[0].preserveAttachmentCount = 0;
        subpasses[0].pPreserveAttachments = nullptr;
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
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.attachmentCount = (uint32_t)attachements.size();
    render_pass_create_info.pAttachments = attachements.data();
    render_pass_create_info.subpassCount = (uint32_t)subpasses.size();
    render_pass_create_info.pSubpasses = subpasses.data();
    render_pass_create_info.dependencyCount = 1; // dependencies between subpasses, if one reads a buffer from another
    render_pass_create_info.pDependencies = &dependency;

    result = vkCreateRenderPass(_ctx.device, &render_pass_create_info, nullptr, &_render_pass);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    return true;
}

void Renderer::DeInitRenderPass()
{
    vkDestroyRenderPass(_ctx.device, _render_pass, nullptr);
}

bool Renderer::InitSwapChainFrameBuffers()
{
    VkResult result;

    _swapchain_framebuffers.resize(_w->swapchain_image_count());

    for (uint32_t i = 0; i < _w->swapchain_image_count(); ++i)
    {
        std::array<VkImageView, 2> attachments = {};
        attachments[ATTACH_INDEX_DEPTH] = _depth_stencil_image_view; // shared between framebuffers
        attachments[ATTACH_INDEX_COLOR] = _w->swapchain_image_views(i);

        VkFramebufferCreateInfo frame_buffer_create_info = {};
        frame_buffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        frame_buffer_create_info.renderPass = _render_pass;
        frame_buffer_create_info.attachmentCount = (uint32_t)attachments.size(); // need to be compatible with the render pass attachments
        frame_buffer_create_info.pAttachments = attachments.data();
        frame_buffer_create_info.width = _w->surface_size().width;
        frame_buffer_create_info.height = _w->surface_size().height;
        frame_buffer_create_info.layers = 1;

        result = vkCreateFramebuffer(_ctx.device, &frame_buffer_create_info, nullptr, &_swapchain_framebuffers[i]);
        ErrorCheck(result);
        if (result != VK_SUCCESS)
            return false;
    }
    return true;
}

void Renderer::DeInitSwapChainFrameBuffers()
{
    for (auto & framebuffer : _swapchain_framebuffers)
    {
        vkDestroyFramebuffer(_ctx.device, framebuffer, nullptr);
    }
}

bool Renderer::InitUniformBuffer()
{
    VkResult result;

    float aspect_ratio = _global_viewport.width / (float)_global_viewport.height;
    float fov_degrees = 45.0f;
    float nearZ = 0.1f;
    float farZ = 1000.0f;

    glm::mat4 proj = glm::perspective(fov_degrees, aspect_ratio, nearZ, farZ);
    proj[1][1] *= -1.0f; // vulkan clip space with Y down.
    glm::mat4 view = glm::lookAt(glm::vec3(-3, -2, 10), glm::vec3(0), glm::vec3(0, 1, 0));
    glm::mat4 model = glm::mat4(1);

    memcpy(_mvp.m, glm::value_ptr(model), sizeof(model));
    memcpy(_mvp.v, glm::value_ptr(view), sizeof(view));
    memcpy(_mvp.p, glm::value_ptr(proj), sizeof(proj));

    Log("#   Create Matrices Uniform Buffer\n");
    VkBufferCreateInfo uniform_buffer_create_info = {};
    uniform_buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    uniform_buffer_create_info.size = sizeof(_mvp); // size in bytes
    uniform_buffer_create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;   // <-- UBO
    uniform_buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    result = vkCreateBuffer(_ctx.device, &uniform_buffer_create_info, nullptr, &_matrices_ubo.buffer);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    Log("#   Get Uniform Buffer Memory Requirements(size and type)\n");
    VkMemoryRequirements uniform_buffer_memory_requirements = {};
    vkGetBufferMemoryRequirements(_ctx.device, _matrices_ubo.buffer, &uniform_buffer_memory_requirements);

    VkMemoryAllocateInfo uniform_buffer_allocate_info = {};
    uniform_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    uniform_buffer_allocate_info.allocationSize = uniform_buffer_memory_requirements.size;

    uint32_t uniform_memory_type_bits = uniform_buffer_memory_requirements.memoryTypeBits;
    VkMemoryPropertyFlags uniform_desired_memory_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    auto mem_props = _ctx.physical_device_memory_properties;
    for (uint32_t i = 0; i < mem_props.memoryTypeCount; ++i)
    {
        VkMemoryType memory_type = mem_props.memoryTypes[i];
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
    result = vkAllocateMemory(_ctx.device, &uniform_buffer_allocate_info, nullptr, &_matrices_ubo.memory);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    Log("#   Bind memory to buffer\n");
    result = vkBindBufferMemory(_ctx.device, _matrices_ubo.buffer, _matrices_ubo.memory, 0);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    Log("#   Map Uniform Buffer\n");
    void *mapped = nullptr;
    result = vkMapMemory(_ctx.device, _matrices_ubo.memory, 0, VK_WHOLE_SIZE, 0, &mapped);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    Log("#   Copy matrices, first time.\n");
    memcpy(mapped, _mvp.m, sizeof(_mvp.m));
    memcpy(((float *)mapped + 16), _mvp.v, sizeof(_mvp.v));
    memcpy(((float *)mapped + 32), _mvp.p, sizeof(_mvp.p));

    Log("#   UnMap Uniform Buffer\n");
    vkUnmapMemory(_ctx.device, _matrices_ubo.memory);

    return true;
}

void Renderer::DeInitUniformBuffer()
{
    Log("#   Free Memory\n");
    vkFreeMemory(_ctx.device, _matrices_ubo.memory, nullptr);

    Log("#   Destroy Buffer\n");
    vkDestroyBuffer(_ctx.device, _matrices_ubo.buffer, nullptr);
}

bool Renderer::InitDescriptors()
{
    VkResult result;

    Log("#   Init bindings for the UBO of matrices and texture sampler\n");
    // layout( std140, binding = 0 ) uniform buffer { mat4 m; mat4 v; mat4 p; } UBO;
    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {};
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
    result = vkCreateDescriptorSetLayout(_ctx.device, &set_layout_create_info, nullptr, &_descriptor_set_layout);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    // allocate just enough for one uniform descriptor set
    std::array<VkDescriptorPoolSize, 2> uniform_buffer_pool_sizes = {};
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
    result = vkCreateDescriptorPool(_ctx.device, &pool_create_info, nullptr, &_descriptor_pool);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    VkDescriptorSetAllocateInfo descriptor_allocate_info = {};
    descriptor_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_allocate_info.descriptorPool = _descriptor_pool;
    descriptor_allocate_info.descriptorSetCount = 1;
    descriptor_allocate_info.pSetLayouts = &_descriptor_set_layout;

    Log("#   Allocate Descriptor Set\n");
    result = vkAllocateDescriptorSets(_ctx.device, &descriptor_allocate_info, &_descriptor_set);
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
    write_descriptor_for_uniform_buffer.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_for_uniform_buffer.dstSet = _descriptor_set;
    write_descriptor_for_uniform_buffer.dstBinding = 0;
    write_descriptor_for_uniform_buffer.dstArrayElement = 0;
    write_descriptor_for_uniform_buffer.descriptorCount = 1;
    write_descriptor_for_uniform_buffer.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write_descriptor_for_uniform_buffer.pImageInfo = nullptr;
    write_descriptor_for_uniform_buffer.pBufferInfo = &descriptor_uniform_buffer_info;
    write_descriptor_for_uniform_buffer.pTexelBufferView = nullptr;

    Log("#   Update Descriptor Set (UBO)\n");
    vkUpdateDescriptorSets(_ctx.device, 1, &write_descriptor_for_uniform_buffer, 0, nullptr);

    VkDescriptorImageInfo descriptor_image_info = {};
    descriptor_image_info.sampler = _checker_texture.sampler;
    descriptor_image_info.imageView = _checker_texture.texture_view;
    descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_PREINITIALIZED; // why ? we did change the layout manually!!
                                                                        // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL

    VkWriteDescriptorSet write_descriptor_for_checker_texture_sampler = {};
    write_descriptor_for_checker_texture_sampler.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_for_checker_texture_sampler.dstSet = _descriptor_set;
    write_descriptor_for_checker_texture_sampler.dstBinding = 1; // <----- beware
    write_descriptor_for_checker_texture_sampler.dstArrayElement = 0;
    write_descriptor_for_checker_texture_sampler.descriptorCount = 1;
    write_descriptor_for_checker_texture_sampler.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write_descriptor_for_checker_texture_sampler.pImageInfo = &descriptor_image_info;
    write_descriptor_for_checker_texture_sampler.pBufferInfo = nullptr;
    write_descriptor_for_checker_texture_sampler.pTexelBufferView = nullptr;

    Log("#   Update Descriptor Set (texture sampler)\n");
    vkUpdateDescriptorSets(_ctx.device, 1, &write_descriptor_for_checker_texture_sampler, 0, nullptr);

    return true;
}

void Renderer::DeInitDescriptors()
{
    Log("#   Destroy Descriptor Pool\n");
    vkDestroyDescriptorPool(_ctx.device, _descriptor_pool, nullptr);

    Log("#   Destroy Descriptor Set Layout\n");
    vkDestroyDescriptorSetLayout(_ctx.device, _descriptor_set_layout, nullptr);
}

bool Renderer::InitFakeImage()
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

    result = vkCreateImage(_ctx.device, &texture_create_info, nullptr, &_checker_texture.texture_image);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    VkMemoryRequirements texture_memory_requirements = {};
    vkGetImageMemoryRequirements(_ctx.device, _checker_texture.texture_image, &texture_memory_requirements);

    VkMemoryAllocateInfo texture_image_allocate_info = {};
    texture_image_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    texture_image_allocate_info.allocationSize = texture_memory_requirements.size;

    uint32_t texture_memory_type_bits = texture_memory_requirements.memoryTypeBits;
    VkMemoryPropertyFlags tDesiredMemoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    for (uint32_t i = 0; i < 32; ++i) {
        VkMemoryType memory_type = _ctx.physical_device_memory_properties.memoryTypes[i];
        if (texture_memory_type_bits & 1)
        {
            if ((memory_type.propertyFlags & tDesiredMemoryFlags) == tDesiredMemoryFlags)
            {
                texture_image_allocate_info.memoryTypeIndex = i;
                break;
            }
        }
        texture_memory_type_bits = texture_memory_type_bits >> 1;
    }

    result = vkAllocateMemory(_ctx.device, &texture_image_allocate_info, nullptr, &_checker_texture.texture_image_memory);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    result = vkBindImageMemory(_ctx.device, _checker_texture.texture_image, _checker_texture.texture_image_memory, 0);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    void *image_mapped;
    result = vkMapMemory(_ctx.device, _checker_texture.texture_image_memory, 0, VK_WHOLE_SIZE, 0, &image_mapped);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    memcpy(image_mapped, test_image.data, sizeof(float) * test_image.width * test_image.height * 3);

    VkMappedMemoryRange memory_range = {};
    memory_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    memory_range.memory = _checker_texture.texture_image_memory;
    memory_range.offset = 0;
    memory_range.size = VK_WHOLE_SIZE;
    vkFlushMappedMemoryRanges(_ctx.device, 1, &memory_range);

    vkUnmapMemory(_ctx.device, _checker_texture.texture_image_memory);

    // we can clear the image data:
    delete[] test_image.data;

    // TODO: transition

    VkFence submit_fence = {};
    VkFenceCreateInfo fence_create_info = {};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    vkCreateFence(_ctx.device, &fence_create_info, nullptr, &submit_fence);

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    auto &cmd = _ctx.graphics.command_buffer;

    vkBeginCommandBuffer(cmd, &begin_info);

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

    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_HOST_BIT, //VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,//VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &layout_transition_barrier);

    vkEndCommandBuffer(cmd);

    VkPipelineStageFlags waitStageMask[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = nullptr;
    submit_info.pWaitDstStageMask = waitStageMask;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = nullptr;
    result = vkQueueSubmit(_ctx.graphics.queue, 1, &submit_info, submit_fence);

    vkWaitForFences(_ctx.device, 1, &submit_fence, VK_TRUE, UINT64_MAX);
    vkResetFences(_ctx.device, 1, &submit_fence);
    vkResetCommandBuffer(cmd, 0);

    vkDestroyFence(_ctx.device, submit_fence, nullptr);

    // TODO: image view
    VkImageViewCreateInfo texture_image_view_create_info = {};
    texture_image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    texture_image_view_create_info.image = _checker_texture.texture_image;
    texture_image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    texture_image_view_create_info.format = VK_FORMAT_R32G32B32_SFLOAT;
    texture_image_view_create_info.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
    texture_image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    texture_image_view_create_info.subresourceRange.baseMipLevel = 0;
    texture_image_view_create_info.subresourceRange.levelCount = 1;
    texture_image_view_create_info.subresourceRange.baseArrayLayer = 0;
    texture_image_view_create_info.subresourceRange.layerCount = 1;

    result = vkCreateImageView(_ctx.device, &texture_image_view_create_info, nullptr, &_checker_texture.texture_view);
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

    result = vkCreateSampler(_ctx.device, &sampler_create_info, nullptr, &_checker_texture.sampler);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    return true;
}

void Renderer::DeInitFakeImage()
{
    vkDestroySampler(_ctx.device, _checker_texture.sampler, nullptr);
    vkDestroyImageView(_ctx.device, _checker_texture.texture_view, nullptr);
    vkDestroyImage(_ctx.device, _checker_texture.texture_image, nullptr);
    vkFreeMemory(_ctx.device, _checker_texture.texture_image_memory, nullptr);
}

bool Renderer::InitShaders()
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

    result = vkCreateShaderModule(_ctx.device, &vertex_shader_creation_info, nullptr, &_vertex_shader_module);
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

    result = vkCreateShaderModule(_ctx.device, &fragment_shader_creation_info, nullptr, &_fragment_shader_module);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    return true;
}

void Renderer::DeInitShaders()
{
    vkDestroyShaderModule(_ctx.device, _fragment_shader_module, nullptr);
    vkDestroyShaderModule(_ctx.device, _vertex_shader_module, nullptr);
}

bool Renderer::InitGraphicsPipeline()
{
    VkResult result;

    // use it later to define uniform buffer
    VkPipelineLayoutCreateInfo layout_create_info = {};
    layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_create_info.setLayoutCount = 1;
    layout_create_info.pSetLayouts = &_descriptor_set_layout;
    layout_create_info.pushConstantRangeCount = 0;
    layout_create_info.pPushConstantRanges = nullptr; // constant into shader for opti???

    Log("#   Fill Pipeline Layout... and everything else.\n");
    result = vkCreatePipelineLayout(_ctx.device, &layout_create_info, nullptr, &_pipeline_layout);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    std::array<VkPipelineShaderStageCreateInfo, 2> shader_stage_create_infos = {};
    shader_stage_create_infos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage_create_infos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shader_stage_create_infos[0].module = _vertex_shader_module;
    shader_stage_create_infos[0].pName = "main";        // shader entry point function name
    shader_stage_create_infos[0].pSpecializationInfo = nullptr;

    shader_stage_create_infos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage_create_infos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shader_stage_create_infos[1].module = _fragment_shader_module;
    shader_stage_create_infos[1].pName = "main";        // shader entry point function name
    shader_stage_create_infos[1].pSpecializationInfo = nullptr;

    VkVertexInputBindingDescription vertex_binding_description = {};
    vertex_binding_description.binding = 0;
    vertex_binding_description.stride = sizeof(Scene::vertex_t);
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
    vertex_attribute_description[1].offset = 4 * sizeof(float);

    vertex_attribute_description[2].location = 2;
    vertex_attribute_description[2].binding = 0;
    vertex_attribute_description[2].format = VK_FORMAT_R32G32_SFLOAT; // uv = 2 floats
    vertex_attribute_description[2].offset = (4 + 3) * sizeof(float);

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
    viewport.width = (float)_global_viewport.width;
    viewport.height = (float)_global_viewport.height;
    viewport.minDepth = 0;
    viewport.maxDepth = 1;

    VkRect2D scissors = {};
    scissors.offset = { 0, 0 };
    scissors.extent = _global_viewport;

    VkPipelineViewportStateCreateInfo viewport_state_create_info = {};
    viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_create_info.viewportCount = 1;
    viewport_state_create_info.pViewports = &viewport;
    viewport_state_create_info.scissorCount = 1;
    viewport_state_create_info.pScissors = &scissors;

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

    std::array<VkDynamicState, 2> dynamic_state = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {};
    dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_create_info.dynamicStateCount = (uint32_t)dynamic_state.size();
    dynamic_state_create_info.pDynamicStates = dynamic_state.data();

    std::array<VkGraphicsPipelineCreateInfo, 1> pipeline_create_infos = {};
    pipeline_create_infos[0].sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_create_infos[0].stageCount = (uint32_t)shader_stage_create_infos.size();
    pipeline_create_infos[0].pStages = shader_stage_create_infos.data();
    pipeline_create_infos[0].pVertexInputState = &vertex_input_state_create_info;
    pipeline_create_infos[0].pInputAssemblyState = &input_assembly_state_create_info;
    pipeline_create_infos[0].pTessellationState = nullptr;
    pipeline_create_infos[0].pViewportState = &viewport_state_create_info;
    pipeline_create_infos[0].pRasterizationState = &raster_state_create_info;
    pipeline_create_infos[0].pMultisampleState = &multisample_state_create_info;
    pipeline_create_infos[0].pDepthStencilState = &depth_stencil_state_create_info;
    pipeline_create_infos[0].pColorBlendState = &color_blend_state_create_info;
    pipeline_create_infos[0].pDynamicState = &dynamic_state_create_info;
    pipeline_create_infos[0].layout = _pipeline_layout;
    pipeline_create_infos[0].renderPass = _render_pass;
    pipeline_create_infos[0].subpass = 0;
    pipeline_create_infos[0].basePipelineHandle = VK_NULL_HANDLE; // only if VK_PIPELINE_CREATE_DERIVATIVE flag is set.
    pipeline_create_infos[0].basePipelineIndex = 0;

    Log("#   Create Pipeline\n");
    result = vkCreateGraphicsPipelines(
        _ctx.device,
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

void Renderer::DeInitGraphicsPipeline()
{
    for (auto & pipeline : _pipelines)
    {
        vkDestroyPipeline(_ctx.device, pipeline, nullptr);
    }
    vkDestroyPipelineLayout(_ctx.device, _pipeline_layout, nullptr);
}

void Renderer::set_object_position(float x, float y, float z)
{
    _mvp.m[12] = x;
    _mvp.m[13] = y;
    _mvp.m[14] = z;
}

void Renderer::set_camera_position(float x, float y, float z)
{
    _mvp.v[12] = -x;
    _mvp.v[13] = -y;
    _mvp.v[14] = -z;
}

void Renderer::update_matrices_ubo()
{
    void *matrix_mapped;
    vkMapMemory(_ctx.device, _matrices_ubo.memory, 0, VK_WHOLE_SIZE, 0, &matrix_mapped);

    memcpy(matrix_mapped, _mvp.m, sizeof(_mvp.m));
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
    vkFlushMappedMemoryRanges(_ctx.device, 1, &memory_range);

    vkUnmapMemory(_ctx.device, _matrices_ubo.memory);
}
//
