#include "Renderer.h"
#include "Shared.h"

#include <cstdlib>
#include <assert.h>
#include <vector>
#include <array>
#include <iostream>
#include <sstream>

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>

Renderer::Renderer()
{
	
}

bool Renderer::Init()
{
	// Manually load the dll, and grab the "vkGetInstanceProcAddr" symbol,
	// vkCreateInstance, and vkEnumerate extensions and layers
	if (volkInitialize() != VK_SUCCESS)
		return false;

	// Setup debug callback structure and desired layers names.
	SetupDebug();

	// Create the instance
	if (!InitInstance())
		return false;

	// Loads all the symbols for that instance, beginning with vkCreateDevice.
	volkLoadInstance(_instance);

	// Install debug callback
	if (!InitDebug())
		return false;

	// Create device and get rendering queue.
	if (!InitDevice())
		return false;

	// Load all the rest of the symbols, specifically for that device, bypassing
	// the loader dispatch,
	volkLoadDevice(_device);

	return true;
}

Renderer::~Renderer()
{
    DeInitDevice();
    DeInitDebug();
    DeInitInstance();
}

bool Renderer::InitInstance()
{
    VkApplicationInfo application_info{};
    application_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.apiVersion         = VK_MAKE_VERSION( 1, 1, 73 ); //VK_API_VERSION_1_0;
    application_info.applicationVersion = VK_MAKE_VERSION( 0, 0, 4 );
    application_info.pApplicationName   = "Vulkan Tutorial 4";

    VkInstanceCreateInfo instance_create_info{};
    instance_create_info.sType                      = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pApplicationInfo           = &application_info;
    instance_create_info.enabledLayerCount          = (uint32_t)_instance_layers.size();
    instance_create_info.ppEnabledLayerNames        = _instance_layers.data();
    instance_create_info.enabledExtensionCount      = (uint32_t)_instance_extensions.size();
    instance_create_info.ppEnabledExtensionNames    = _instance_extensions.data();
    instance_create_info.pNext                      = &debug_callback_create_info; // put it here to have debug info for the vkCreateInstance function, even if we have not given a debug callback yet.
    auto err = vkCreateInstance( 
        &instance_create_info,
        nullptr, // no custom allocator
        &_instance );

    // 0 = OK
    // positive error code = partial succes
    // negative = failure
    ErrorCheck( err );

	return (VK_SUCCESS == err);
}

void Renderer::DeInitInstance()
{
    vkDestroyInstance( _instance, nullptr );
    _instance = nullptr;
}

// NOTE(nfauvet): a device in Vulkan is like the context in OpenGL
bool Renderer::InitDevice()
{
	bool ok = true;

    // Get all physical devices and choose one (the first here)
	{
		// Call once to get the number
		uint32_t gpu_count = 0;
		ok &= (VK_SUCCESS == vkEnumeratePhysicalDevices(_instance, &gpu_count, nullptr));

        // Call a second time to get the actual devices
        std::vector<VkPhysicalDevice> gpu_list( gpu_count );
		ok &= (VK_SUCCESS == vkEnumeratePhysicalDevices(_instance, &gpu_count, gpu_list.data()));

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
            assert( !"Vulkan ERROR: Queue family supporting graphics not found." );
            std::exit( -1 );
        }
    }

    // Instance Layer Properties
    {
        uint32_t layer_count = 0;
		ok &= (VK_SUCCESS == vkEnumerateInstanceLayerProperties(&layer_count, nullptr)); // first call = query number

        std::vector<VkLayerProperties> layer_property_list( layer_count );
		ok &= (VK_SUCCESS == vkEnumerateInstanceLayerProperties(&layer_count, layer_property_list.data())); // second call with allocated array

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
		ok &= (VK_SUCCESS == vkEnumerateDeviceLayerProperties(_gpu, &layer_count, nullptr)); // first call = query number

        std::vector<VkLayerProperties> layer_property_list( layer_count );
		ok &= (VK_SUCCESS == vkEnumerateDeviceLayerProperties(_gpu, &layer_count, layer_property_list.data())); // second call with allocated array

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
		&_device);

    ErrorCheck(err);

	ok &= (VK_SUCCESS == err);

    vkGetDeviceQueue( 
        _device, 
        _graphics_family_index, 
        0, // index in queueCount
        &_queue );

	return ok;
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
    std::ostringstream oss;
    oss << "VKDBG: ";
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

    std::cout << oss.str();

#ifdef _WIN32
    std::string s = oss.str();
    if ( flags & VK_DEBUG_REPORT_ERROR_BIT_EXT )
    {
        ::MessageBoxA( NULL, s.c_str(), "Vulkan Error!", 0 );
    }
    OutputDebugStringA( s.c_str() );
#endif

    return false; // -> do not stop the command causing the error.
}

void Renderer::SetupDebug()
{
    // moved as a member of class and created here to be able to pass it to instance_create_info
    // and be able to debug the VkCreateInstance function.
    //VkDebugReportCallbackCreateInfoEXT debug_callback_create_info{};
    debug_callback_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    debug_callback_create_info.pfnCallback = &VulkanDebugCallback;
    debug_callback_create_info.flags =
        //VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
        VK_DEBUG_REPORT_WARNING_BIT_EXT |
        VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
        VK_DEBUG_REPORT_ERROR_BIT_EXT |
        //VK_DEBUG_REPORT_DEBUG_BIT_EXT
        0;

    //_instance_layers.push_back("VK_LAYER_LUNARG_api_dump" );
	//_instance_layers.push_back("VK_LAYER_LUNARG_assistant_layer");
    _instance_layers.push_back("VK_LAYER_LUNARG_core_validation" );
	//_instance_layers.push_back("VK_LAYER_LUNARG_device_simulation");
    //_instance_layers.push_back("VK_LAYER_LUNARG_monitor" );
    _instance_layers.push_back("VK_LAYER_LUNARG_object_tracker" );
    _instance_layers.push_back("VK_LAYER_LUNARG_parameter_validation" );
    //_instance_layers.push_back("VK_LAYER_LUNARG_screenshot" );
	_instance_layers.push_back("VK_LAYER_LUNARG_standard_validation" );
    //_instance_layers.push_back("VK_LAYER_LUNARG_swapchain" ); // pas sur mon portable. deprecated?
    _instance_layers.push_back("VK_LAYER_GOOGLE_threading" );
    _instance_layers.push_back("VK_LAYER_GOOGLE_unique_objects" );
    //_instance_layers.push_back("VK_LAYER_LUNARG_vktrace" );
    //_instance_layers.push_back("VK_LAYER_NV_optimus" );
    //_instance_layers.push_back("VK_LAYER_RENDERDOC_Capture" );
    //_instance_layers.push_back("VK_LAYER_VALVE_steam_overlay" );

	_instance_extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	_instance_extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	_instance_extensions.push_back("VK_KHR_win32_surface");

	//_device_layers.push_back( "VK_LAYER_LUNARG_core_validation" ); // deprecated
	//_device_layers.push_back( "VK_LAYER_LUNARG_parameter_validation" ); // deprecated
    //_device_layers.push_back( "VK_LAYER_LUNARG_standard_validation" ); // deprecated

}

PFN_vkCreateDebugReportCallbackEXT fvkCreateDebugReportCallbackEXT = nullptr;
PFN_vkDestroyDebugReportCallbackEXT fvkDestroyDebugReportCallbackEXT = nullptr;

bool Renderer::InitDebug()
{
	// @[Loader Message]: Layer "VK_LAYER_RENDERDOC_Capture" using deprecated 'vkGetInstanceProcAddr' tag which was deprecated starting with JSON file version 1.1.0.
	// Instead, use the new vkNegotiateLayerInterfaceVersion function to return the GetInstanceProcAddr function for this layer.
    
	
	// already done by volk
	//fvkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr( _instance, "vkCreateDebugReportCallbackEXT" );
    //fvkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr( _instance, "vkDestroyDebugReportCallbackEXT" );

 /*   if ( !fvkCreateDebugReportCallbackEXT || !fvkDestroyDebugReportCallbackEXT )
    {
        assert(!"GetProcAddr failed");
        std::exit( -1 );
    }

    fvkCreateDebugReportCallbackEXT( _instance, &debug_callback_create_info, nullptr, &_debug_report );*/

	auto err = vkCreateDebugReportCallbackEXT(_instance, &debug_callback_create_info, nullptr, &_debug_report);
	return (VK_SUCCESS == err);
}

void Renderer::DeInitDebug()
{
    vkDestroyDebugReportCallbackEXT( _instance, _debug_report, nullptr );
    _debug_report = nullptr;
}

