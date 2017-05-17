#include "Renderer.h"

#include <cstdlib>
#include <assert.h>
#include <vector>
#include <iostream>

Renderer::Renderer()
{
    SetupDebug();
    InitInstance();
    InitDebug();
    InitDevice();
}

Renderer::~Renderer()
{
    DeInitDevice();
    DeInitDebug();
    DeInitInstance();
    // TUTORIAL 2: 41:57
}

void Renderer::InitInstance()
{
    VkApplicationInfo application_info{};
    application_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.apiVersion         = VK_MAKE_VERSION( 1, 0, 13 ); //VK_API_VERSION_1_0;
    application_info.applicationVersion = VK_MAKE_VERSION( 0, 1, 0 );
    application_info.pApplicationName   = "Vulkan Tutorial 1";

    VkInstanceCreateInfo instance_create_info{};
    instance_create_info.sType                      = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pApplicationInfo           = &application_info;
    instance_create_info.enabledLayerCount          = (uint32_t)_instance_layers.size();
    instance_create_info.ppEnabledLayerNames        = _instance_layers.data();
    instance_create_info.enabledExtensionCount      = (uint32_t)_instance_extensions.size();
    instance_create_info.ppEnabledExtensionNames    = _instance_extensions.data();
    auto err = vkCreateInstance( 
        &instance_create_info,
        nullptr, // no custom allocator
        &_instance );

    // NOTE(nfauvet): 
    // 0 = OK
    // positive error code = partial succes
    // negative = failure
    if ( VK_SUCCESS != err )
    {
        assert( !"Vulkan ERROR: Create instance failed.");
        std::exit( -1 );
    }
}

void Renderer::DeInitInstance()
{
    vkDestroyInstance( _instance, nullptr );
    _instance = nullptr;
}

// NOTE(nfauvet): a device in Vulkan is like the context in OpenGL
void Renderer::InitDevice()
{
    // Get all physical devices and choose one (the first here)
    {
        // Call once to get the number
        uint32_t gpu_count = 0;
        auto err1 = vkEnumeratePhysicalDevices( _instance, &gpu_count, nullptr );

        // Call a second time to get the actual devices
        std::vector<VkPhysicalDevice> gpu_list( gpu_count );
        auto err2 = vkEnumeratePhysicalDevices( _instance, &gpu_count, gpu_list.data() );

        // Take first physical device
        _gpu = gpu_list[0];

        vkGetPhysicalDeviceProperties( _gpu, &_gpu_properties );
    }

    // Get the "Queue Family Properties" of the Device
    {
        uint32_t family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties( _gpu, &family_count, nullptr );

        std::vector<VkQueueFamilyProperties> family_property_list( family_count );
        vkGetPhysicalDeviceQueueFamilyProperties( _gpu, &family_count, family_property_list.data() );

        // Look for a queue family supporting graphics operations.
        bool found = false;
        for ( uint32_t i = 0; i < family_count; ++i )
        {
            if ( family_property_list[i].queueFlags & VK_QUEUE_GRAPHICS_BIT )
            {
                found = true;
                _graphics_family_index = i;
                break;
            }
        }

        if ( !found )
        {
            assert( !"Vulkan ERROR: Queue family supporting grpahics not found." );
            std::exit( -1 );
        }
    }

    // Instance Layer Properties
    {
        uint32_t layer_count = 0;
        vkEnumerateInstanceLayerProperties( &layer_count, nullptr ); // first call = query number

        std::vector<VkLayerProperties> layer_property_list( layer_count );
        vkEnumerateInstanceLayerProperties( &layer_count, layer_property_list.data() ); // second call with allocated array

        std::cout << "Instance layers: \n";
        for ( auto &i : layer_property_list )
        {
            std::cout << "  " << i.layerName << "\t\t | " << i.description << std::endl;
        }
        std::cout << std::endl;
    }


    // Device Layer Properties (deprecated)
    {
        uint32_t layer_count = 0;
        vkEnumerateDeviceLayerProperties( _gpu, &layer_count, nullptr ); // first call = query number

        std::vector<VkLayerProperties> layer_property_list( layer_count );
        vkEnumerateDeviceLayerProperties( _gpu, &layer_count, layer_property_list.data() ); // second call with allocated array

        std::cout << "Device layers: (deprecated)\n";
        for ( auto &i : layer_property_list )
        {
            std::cout << "  " << i.layerName << "\t\t | " << i.description << std::endl;
        }
        std::cout << std::endl;
    }

    float queue_priorities[]{ 1.0f }; // priorities are float from 0.0f to 1.0f
    VkDeviceQueueCreateInfo device_queue_create_info{};
    device_queue_create_info.sType              = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    device_queue_create_info.queueFamilyIndex   = _graphics_family_index;
    device_queue_create_info.queueCount         = 1;
    device_queue_create_info.pQueuePriorities   = queue_priorities;

    // Create a logical "device" associated with the physical "device"
    VkDeviceCreateInfo device_create_info{};
    device_create_info.sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.queueCreateInfoCount = 1;
    device_create_info.pQueueCreateInfos    = &device_queue_create_info;
    //device_create_info.enabledLayerCount = _device_layers.size(); // deprecated
    //device_create_info.ppEnabledLayerNames = _device_layers.data(); // deprecated
    //device_create_info.enabledExtensionCount = _device_extensions.size(); // deprecated
    //device_create_info.ppEnabledExtensionNames = _device_extensions.data(); // deprecated

    auto err = vkCreateDevice( 
        _gpu,
        &device_create_info,
        nullptr, // allocator
        &_device );

    if ( VK_SUCCESS != err )
    {
        assert( !"Vulkan ERROR: Create Device failed." );
        std::exit( -1 );
    }
}

void Renderer::DeInitDevice()
{
    vkDestroyDevice( _device, nullptr );
    _device = nullptr;
}

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
    std::cout << "VKDBG: ";
    if ( flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT )
    {
        std::cout << "[INFO]:  ";
    }
    if ( flags & VK_DEBUG_REPORT_WARNING_BIT_EXT )
    {
        std::cout << "[WARN]:  ";
    }
    if ( flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT )
    {
        std::cout << "[PERF]:  ";
    }
    if ( flags & VK_DEBUG_REPORT_ERROR_BIT_EXT )
    {
        std::cout << "[ERROR]: ";
    }
    if ( flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT )
    {
        std::cout << "[DEBUG]: ";
    }

    std::cout << "@[" << layer_prefix << "]: ";
    std::cout << msg << std::endl;

    return false; // -> do not stop the command causing the error.
}

void Renderer::SetupDebug()
{
    //_instance_layers.push_back( "VK_LAYER_LUNARG_standard_validation" );
    //_instance_layers.push_back( "VK_LAYER_LUNARG_api_dump" );
    _instance_layers.push_back( "VK_LAYER_LUNARG_core_validation" );
    _instance_layers.push_back( "VK_LAYER_LUNARG_monitor" );
    _instance_layers.push_back( "VK_LAYER_LUNARG_object_tracker" );
    _instance_layers.push_back( "VK_LAYER_LUNARG_parameter_validation" );
    _instance_layers.push_back( "VK_LAYER_LUNARG_screenshot" );
    _instance_layers.push_back( "VK_LAYER_LUNARG_swapchain" );
    _instance_layers.push_back( "VK_LAYER_GOOGLE_threading" );
    _instance_layers.push_back( "VK_LAYER_GOOGLE_unique_objects" );
    //_instance_layers.push_back( "VK_LAYER_LUNARG_vktrace" );
    //_instance_layers.push_back( "VK_LAYER_NV_optimus" );
    //_instance_layers.push_back( "VK_LAYER_RENDERDOC_Capture" );
    //_instance_layers.push_back( "VK_LAYER_VALVE_steam_overlay" );
    _instance_layers.push_back( "VK_LAYER_LUNARG_standard_validation" );


    _instance_extensions.push_back( VK_EXT_DEBUG_REPORT_EXTENSION_NAME );

    //_device_layers.push_back( "VK_LAYER_LUNARG_standard_validation" ); // deprecated
}

PFN_vkCreateDebugReportCallbackEXT fvkCreateDebugReportCallbackEXT = nullptr;
PFN_vkDestroyDebugReportCallbackEXT fvkDestroyDebugReportCallbackEXT = nullptr;

void Renderer::InitDebug()
{
    fvkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr( _instance, "vkCreateDebugReportCallbackEXT" );
    fvkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr( _instance, "vkDestroyDebugReportCallbackEXT" );

    if ( !fvkCreateDebugReportCallbackEXT || !fvkDestroyDebugReportCallbackEXT )
    {
        assert(!"GetProcAddr failed");
        std::exit( -1 );
    }

    VkDebugReportCallbackCreateInfoEXT debug_callback_create_info{};
    debug_callback_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    debug_callback_create_info.pfnCallback = &VulkanDebugCallback;
    debug_callback_create_info.flags = 
        VK_DEBUG_REPORT_INFORMATION_BIT_EXT | 
        VK_DEBUG_REPORT_WARNING_BIT_EXT | 
        VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | 
        VK_DEBUG_REPORT_ERROR_BIT_EXT | 
        VK_DEBUG_REPORT_DEBUG_BIT_EXT;
    fvkCreateDebugReportCallbackEXT( _instance, &debug_callback_create_info, nullptr, &_debug_report );
}

void Renderer::DeInitDebug()
{
    fvkDestroyDebugReportCallbackEXT( _instance, _debug_report, nullptr );
    _debug_report = nullptr;
}

