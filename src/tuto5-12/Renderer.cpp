#include "build_options.h"

#include "platform.h"
#include "Renderer.h"
#include "Shared.h"
#include "window.h"
#include "scene.h"

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
    // SCENE

    //Log("# Destroy Command Buffer\n");
    //DeInitCommandBuffer();

    if (_w)
    {
        Log("#   Destroy Vulkan Window Specifics\n");
        _w->DeInitVulkanWindowSpecifics(&_ctx);
    }

    Log("#   Destroy Vma\n");
    DeInitVma();

    Log("#   Destroy Device\n");
    DeInitDevice();

    Log("#   Destroy Debug\n");
    DeInitDebug();

    Log("#   Destroy Instance\n");
    DeInitInstance();
}

bool Renderer::Init()
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

    Log("#   Init Window Specific Vulkan\n");
    if (_w)
    {
        _w->InitVulkanWindowSpecifics(&_ctx);
    }

    // SCENE
    //Log("#  Init CommandBuffer\n");
    //if (!InitCommandBuffer())
    //    return false;

    return true;
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























//
// SCENE
//



void Renderer::Draw(float dt, Scene *scene)
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
    _w->set_object_position(obj_x, obj_y, obj_z);
    //_window->set_camera_position(0.0f, 0.0f, _camera.cameraZ);
    _w->update_matrices_ubo();

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
		render_pass_begin_info.renderPass = _w->render_pass();
		render_pass_begin_info.framebuffer = _w->active_swapchain_framebuffer();
		render_pass_begin_info.renderArea = render_area;
		render_pass_begin_info.clearValueCount = (uint32_t)clear_values.size();
		render_pass_begin_info.pClearValues = clear_values.data();

		vkCmdBeginRenderPass(cmd, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
		{
			// TODO: put into window, too many get...
			// w->BindPipeline(command_buffer)
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _w->pipeline(0));

			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _w->pipeline_layout(), 0, 1, _w->descriptor_set_ptr(), 0, nullptr);

			// take care of dynamic state:
			VkExtent2D surface_size = _w->surface_size();

			VkViewport viewport = { 0, 0, (float)surface_size.width, (float)surface_size.height, 0, 1 };
			vkCmdSetViewport(cmd, 0, 1, &viewport);

			VkRect2D scissor = { 0, 0, surface_size.width, surface_size.height };
			vkCmdSetScissor(cmd, 0, 1, &scissor);

            scene->draw_all_objects(cmd);
            
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
