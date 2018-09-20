#include "build_options.h"

#include "platform.h"
#include "Renderer.h"
#include "Shared.h"
#include "window.h"
#include "scene.h"

#include "imgui.h"
#include "imgui_impl_vulkan.h"

#include "glm_usage.h"

#include <cstdlib>
#include <assert.h>
#include <vector>
#include <array>
#include <iostream>
#include <fstream>
#include <sstream>
#include <set>

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

    // Fill the needed features.
    Log("#   Setup Features\n");
    SetupFeatures();

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
    Log("#    Init Synchronization Primitives\n");
    if (!InitSynchronizations())
        return false;

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

    Log("#    Init Descriptor Pool\n");
    if (!InitDescriptorPool())
        return false;

    return true;
}

void Renderer::DeInitSceneVulkan()
{
    Log("#    Destroy DescriptorPool\n");
    DeInitDescriptorPool();

    Log("#    Destroy FrameBuffers\n");
    DeInitSwapChainFrameBuffers();

    Log("#    Destroy Render Pass\n");
    DeInitRenderPass();

    Log("#    Destroy Depth/Stencil\n");
    DeInitDepthStencilImage();

    Log("#    Destroy Command Buffer\n");
    DeInitCommandBuffer();

    Log("#    Destroy Synchronization Primitives\n");
    DeInitSynchronizations();
}

bool Renderer::InitInstance()
{
    VkResult result;

    VkApplicationInfo application_info = {};
    application_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.apiVersion         = VK_API_VERSION_1_0;//VK_MAKE_VERSION( 1, 1, 73 );
    application_info.applicationVersion = VK_MAKE_VERSION( 0, 0, 4 );
    application_info.pApplicationName   = "Vulkan Renderer";

    VkInstanceCreateInfo instance_create_info = {};
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

    // Get all physical devices and choose one
    Log("#    Choose Physical Device\n");
    if (!ChoosePhysicalDevice())
        return false;
    

    // Get the "Queue Family Properties" of the Device
    Log("#    Get Physical Device Queue Family Properties\n");
    if (!SelectQueueFamilyIndices())
        return false;
    
    if (_ctx.graphics.family_index == UINT32_MAX)
    {
        assert(!"Vulkan ERROR: Queue family supporting graphics not found.");
        return false;
    }

#if EXTRA_VERBOSE == 1
    // Instance/Device Layer Properties
    Log("#    Enumerate Instance Layer Properties:\n");
    EnumerateInstanceLayers();

    Log("#    Enumerate Device Layer Properties:\n");
    EnumerateDeviceLayers();

    Log("#    Enumerate Device Extensions Properties:\n");
    EnumerateDeviceExtensions();
#endif

    Log("#    Create Logical Device\n");
    if (!CreateLogicalDevice())
        return false;

    return true;
}

void Renderer::DeInitDevice()
{
    vkDestroyDevice( _ctx.device, nullptr );
    _ctx.device = nullptr;
}

bool Renderer::ChoosePhysicalDevice()
{
    VkResult result;

    Log("#     Enumerate Physical Device\n");
    // Call once to get the number
    uint32_t gpu_count = 0;
    result = vkEnumeratePhysicalDevices(_ctx.instance, &gpu_count, nullptr);
    ErrorCheck(result);
    if (gpu_count == 0)
    {
        assert(!"Vulkan ERROR: No GPU found.");
        return false;
    }

    Log(std::string("#     -> found ") + std::to_string(gpu_count) + std::string(" physical devices.\n"));

    // Call a second time to get the actual devices
    std::vector<VkPhysicalDevice> gpu_list(gpu_count);
    result = vkEnumeratePhysicalDevices(_ctx.instance, &gpu_count, gpu_list.data());
    ErrorCheck(result); // if it has passed the first time, it wont fail the second.
    if (result != VK_SUCCESS)
        return false;

    for (const auto &dev : gpu_list)
    {
        if (IsDeviceSuitable(dev))
        {
            Log("#     Found a suitable device.\n");
            _ctx.physical_device = dev;

            Log("#      Get Physical Device Properties\n");
            vkGetPhysicalDeviceProperties(_ctx.physical_device, &_ctx.physical_device_properties);

            Log("#      Get Physical Device Memory Properties\n");
            vkGetPhysicalDeviceMemoryProperties(_ctx.physical_device, &_ctx.physical_device_memory_properties);
            break;
        }
    }
    if (_ctx.physical_device == VK_NULL_HANDLE)
    {
        assert(!"Could not find a suitable physical device.");
        return false;
    }

    return true;
}

bool Renderer::IsDeviceSuitable(VkPhysicalDevice dev)
{
    VkPhysicalDeviceProperties physical_device_properties = {};
    vkGetPhysicalDeviceProperties(dev, &physical_device_properties);

    VkPhysicalDeviceFeatures physical_device_features = {};
    vkGetPhysicalDeviceFeatures(dev, &physical_device_features);

#if EXTRA_VERBOSE == 1

    Log("Properties: \n");

    Log(std::string("   apiVersion ") + std::to_string(physical_device_properties.apiVersion) + "\n");
    Log(std::string("   driverVersion ") + std::to_string(physical_device_properties.driverVersion) + "\n");
    Log(std::string("   vendorID ") + std::to_string(physical_device_properties.vendorID) + "\n");
    Log(std::string("   deviceID ") + std::to_string(physical_device_properties.deviceID) + "\n");
    Log(std::string("   deviceType ") + std::to_string(physical_device_properties.deviceType) + "\n");
    Log(std::string("   deviceName ") + std::string(&physical_device_properties.deviceName[0]) + "\n");
    //uint8_t                             physical_device_properties.pipelineCacheUUID;
    //VkPhysicalDeviceLimits              physical_device_properties.limits;
    //VkPhysicalDeviceSparseProperties    physical_device_properties.sparseProperties;

    Log("Features: \n");

    Log(std::string("   robustBufferAccess ") + std::to_string(physical_device_features.robustBufferAccess) + "\n");
    Log(std::string("   fullDrawIndexUint32 ") + std::to_string(physical_device_features.fullDrawIndexUint32) + "\n");
    Log(std::string("   imageCubeArray ") + std::to_string(physical_device_features.imageCubeArray) + "\n");
    Log(std::string("   independentBlend ") + std::to_string(physical_device_features.independentBlend) + "\n");
    Log(std::string("   geometryShader ") + std::to_string(physical_device_features.geometryShader) + "\n");
    Log(std::string("   tessellationShader ") + std::to_string(physical_device_features.tessellationShader) + "\n");
    Log(std::string("   sampleRateShading ") + std::to_string(physical_device_features.sampleRateShading) + "\n");
    Log(std::string("   dualSrcBlend ") + std::to_string(physical_device_features.dualSrcBlend) + "\n");
    Log(std::string("   logicOp ") + std::to_string(physical_device_features.logicOp) + "\n");
    Log(std::string("   multiDrawIndirect ") + std::to_string(physical_device_features.multiDrawIndirect) + "\n");
    Log(std::string("   drawIndirectFirstInstance ") + std::to_string(physical_device_features.drawIndirectFirstInstance) + "\n");
    Log(std::string("   depthClamp ") + std::to_string(physical_device_features.depthClamp) + "\n");
    Log(std::string("   depthBiasClamp ") + std::to_string(physical_device_features.depthBiasClamp) + "\n");
    Log(std::string("   fillModeNonSolid ") + std::to_string(physical_device_features.fillModeNonSolid) + "\n");
    Log(std::string("   depthBounds ") + std::to_string(physical_device_features.depthBounds) + "\n");
    Log(std::string("   wideLines ") + std::to_string(physical_device_features.wideLines) + "\n");
    Log(std::string("   largePoints ") + std::to_string(physical_device_features.largePoints) + "\n");
    Log(std::string("   alphaToOne ") + std::to_string(physical_device_features.alphaToOne) + "\n");
    Log(std::string("   multiViewport ") + std::to_string(physical_device_features.multiViewport) + "\n");
    Log(std::string("   samplerAnisotropy ") + std::to_string(physical_device_features.samplerAnisotropy) + "\n");
    Log(std::string("   textureCompressionETC2 ") + std::to_string(physical_device_features.textureCompressionETC2) + "\n");
    Log(std::string("   textureCompressionASTC_LDR ") + std::to_string(physical_device_features.textureCompressionASTC_LDR) + "\n");
    Log(std::string("   textureCompressionBC ") + std::to_string(physical_device_features.textureCompressionBC) + "\n");
    Log(std::string("   occlusionQueryPrecise ") + std::to_string(physical_device_features.occlusionQueryPrecise) + "\n");
    Log(std::string("   pipelineStatisticsQuery ") + std::to_string(physical_device_features.pipelineStatisticsQuery) + "\n");
    Log(std::string("   vertexPipelineStoresAndAtomics ") + std::to_string(physical_device_features.vertexPipelineStoresAndAtomics) + "\n");
    Log(std::string("   fragmentStoresAndAtomics ") + std::to_string(physical_device_features.fragmentStoresAndAtomics) + "\n");
    Log(std::string("   shaderTessellationAndGeometryPointSize ") + std::to_string(physical_device_features.shaderTessellationAndGeometryPointSize) + "\n");
    Log(std::string("   shaderImageGatherExtended ") + std::to_string(physical_device_features.shaderImageGatherExtended) + "\n");
    Log(std::string("   shaderStorageImageExtendedFormats ") + std::to_string(physical_device_features.shaderStorageImageExtendedFormats) + "\n");
    Log(std::string("   shaderStorageImageMultisample ") + std::to_string(physical_device_features.shaderStorageImageMultisample) + "\n");
    Log(std::string("   shaderStorageImageReadWithoutFormat ") + std::to_string(physical_device_features.shaderStorageImageReadWithoutFormat) + "\n");
    Log(std::string("   shaderStorageImageWriteWithoutFormat ") + std::to_string(physical_device_features.shaderStorageImageWriteWithoutFormat) + "\n");
    Log(std::string("   shaderUniformBufferArrayDynamicIndexing ") + std::to_string(physical_device_features.shaderUniformBufferArrayDynamicIndexing) + "\n");
    Log(std::string("   shaderSampledImageArrayDynamicIndexing ") + std::to_string(physical_device_features.shaderSampledImageArrayDynamicIndexing) + "\n");
    Log(std::string("   shaderStorageBufferArrayDynamicIndexing ") + std::to_string(physical_device_features.shaderStorageBufferArrayDynamicIndexing) + "\n");
    Log(std::string("   shaderStorageImageArrayDynamicIndexing ") + std::to_string(physical_device_features.shaderStorageImageArrayDynamicIndexing) + "\n");
    Log(std::string("   shaderClipDistance ") + std::to_string(physical_device_features.shaderClipDistance) + "\n");
    Log(std::string("   shaderCullDistance ") + std::to_string(physical_device_features.shaderCullDistance) + "\n");
    Log(std::string("   shaderFloat64 ") + std::to_string(physical_device_features.shaderFloat64) + "\n");
    Log(std::string("   shaderInt64 ") + std::to_string(physical_device_features.shaderInt64) + "\n");
    Log(std::string("   shaderInt16 ") + std::to_string(physical_device_features.shaderInt16) + "\n");
    Log(std::string("   shaderResourceResidency ") + std::to_string(physical_device_features.shaderResourceResidency) + "\n");
    Log(std::string("   shaderResourceMinLod ") + std::to_string(physical_device_features.shaderResourceMinLod) + "\n");
    Log(std::string("   sparseBinding ") + std::to_string(physical_device_features.sparseBinding) + "\n");
    Log(std::string("   sparseResidencyBuffer ") + std::to_string(physical_device_features.sparseResidencyBuffer) + "\n");
    Log(std::string("   sparseResidencyImage2D ") + std::to_string(physical_device_features.sparseResidencyImage2D) + "\n");
    Log(std::string("   sparseResidencyImage3D ") + std::to_string(physical_device_features.sparseResidencyImage3D) + "\n");
    Log(std::string("   sparseResidency2Samples ") + std::to_string(physical_device_features.sparseResidency2Samples) + "\n");
    Log(std::string("   sparseResidency4Samples ") + std::to_string(physical_device_features.sparseResidency4Samples) + "\n");
    Log(std::string("   sparseResidency8Samples ") + std::to_string(physical_device_features.sparseResidency8Samples) + "\n");
    Log(std::string("   sparseResidency16Samples ") + std::to_string(physical_device_features.sparseResidency16Samples) + "\n");
    Log(std::string("   sparseResidencyAliased ") + std::to_string(physical_device_features.sparseResidencyAliased) + "\n");
    Log(std::string("   variableMultisampleRate ") + std::to_string(physical_device_features.variableMultisampleRate) + "\n");
    Log(std::string("   inheritedQueries ") + std::to_string(physical_device_features.inheritedQueries) + "\n");
#endif
    return physical_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
        && physical_device_features.geometryShader
        && physical_device_features.tessellationShader
        && physical_device_features.samplerAnisotropy
        && physical_device_features.fillModeNonSolid;
}

bool Renderer::SelectQueueFamilyIndices()
{
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
    for (uint32_t i = 0; i < family_count; ++i)
    {
        // to know if support for presentation on windows desktop, even not knowing about the surface.
        VkBool32 supportsPresentation = vkGetPhysicalDeviceWin32PresentationSupportKHR(_ctx.physical_device, i);

        if (family_property_list[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
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

    return true;
}

bool Renderer::EnumerateInstanceLayers()
{
    VkResult result;

    uint32_t layer_count = 0;
    result = vkEnumerateInstanceLayerProperties(&layer_count, nullptr); // first call = query number
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    std::vector<VkLayerProperties> layer_property_list(layer_count);
    result = vkEnumerateInstanceLayerProperties(&layer_count, layer_property_list.data()); // second call with allocated array
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    for (auto &i : layer_property_list)
    {
        std::ostringstream oss;
        oss << "#     " << i.layerName << " | " << i.description << std::endl;
        std::string oss_str = oss.str();
        Log(oss_str.c_str());
    }
    
    return true;
}


// Device Layer Properties (deprecated)
bool Renderer::EnumerateDeviceLayers()
{
    VkResult result;

    uint32_t layer_count = 0;
    result = vkEnumerateDeviceLayerProperties(_ctx.physical_device, &layer_count, nullptr); // first call = query number
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    std::vector<VkLayerProperties> layer_property_list(layer_count);
    result = vkEnumerateDeviceLayerProperties(_ctx.physical_device, &layer_count, layer_property_list.data()); // second call with allocated array
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;
    
    for (auto &i : layer_property_list)
    {
        std::ostringstream oss;
        oss << "#     " << i.layerName << " | " << i.description << std::endl;
        std::string oss_str = oss.str();
        Log(oss_str.c_str());
    }
    
    return true;
}

bool Renderer::EnumerateDeviceExtensions()
{
    VkResult result;

    uint32_t extension_count;
    result = vkEnumerateDeviceExtensionProperties(_ctx.physical_device, nullptr, &extension_count, nullptr);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    std::vector<VkExtensionProperties> available_extensions(extension_count);
    result = vkEnumerateDeviceExtensionProperties(_ctx.physical_device, nullptr, &extension_count, available_extensions.data());
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    for (auto &i : available_extensions)
    {
        std::ostringstream oss;
        oss << "#     " << i.extensionName << " | " << i.specVersion << std::endl;
        std::string oss_str = oss.str();
        Log(oss_str.c_str());
    }
    
    return true;
}

bool Renderer::CreateLogicalDevice()
{
    VkResult result;

    std::set<uint32_t> unique_families = { _ctx.graphics.family_index, _ctx.compute.family_index, _ctx.present.family_index, _ctx.transfer.family_index };
    size_t nb_unique = unique_families.size();

    float queue_priorities[] = { 1.0f }; // priorities are float from 0.0f to 1.0f

    std::vector<VkDeviceQueueCreateInfo> device_queue_create_infos(nb_unique);
    for (size_t i = 0; i < nb_unique; ++i)
    {
        device_queue_create_infos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        device_queue_create_infos[i].queueFamilyIndex = unique_families[i];
        device_queue_create_infos[i].queueCount = 1;
        device_queue_create_infos[i].pQueuePriorities = queue_priorities;
    }

    // Create a logical "device" associated with the physical "device"
    VkDeviceCreateInfo device_create_info = {};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.queueCreateInfoCount = (uint32_t)nb_unique;
    device_create_info.pQueueCreateInfos = device_queue_create_infos.data();
    //device_create_info.enabledLayerCount = _ctx.device_layers.size(); // deprecated
    //device_create_info.ppEnabledLayerNames = _ctx.device_layers.data(); // deprecated
    device_create_info.enabledExtensionCount = (uint32_t)_ctx.device_extensions.size();
    device_create_info.ppEnabledExtensionNames = _ctx.device_extensions.data();
    device_create_info.pEnabledFeatures = &_ctx.features;

    Log("#     Create Device\n");
    result = vkCreateDevice(_ctx.physical_device, &device_create_info, nullptr, &_ctx.device);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    // get first queue in family (index 0)
    Log("#     Get Graphics Queue\n");
    vkGetDeviceQueue(_ctx.device, _ctx.graphics.family_index, 0, &_ctx.graphics.queue);

    Log("#     Get Compute Queue\n");
    vkGetDeviceQueue(_ctx.device, _ctx.compute.family_index, 0, &_ctx.compute.queue);

    Log("#     Get Transfer Queue\n");
    vkGetDeviceQueue(_ctx.device, _ctx.transfer.family_index, 0, &_ctx.transfer.queue);

    Log("#     Get Present Queue\n");
    vkGetDeviceQueue(_ctx.device, _ctx.present.family_index, 0, &_ctx.present.queue);

    return true;
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
    _ctx.instance_layers.push_back("VK_LAYER_LUNARG_standard_validation");
    
    //_ctx.instance_layers.push_back("VK_LAYER_LUNARG_api_dump" );
    //_ctx.instance_layers.push_back("VK_LAYER_LUNARG_assistant_layer");
    //_ctx.instance_layers.push_back("VK_LAYER_LUNARG_core_validation"); // asp sur mon ordi maison
    //_ctx.instance_layers.push_back("VK_LAYER_LUNARG_device_simulation");
    //_ctx.instance_layers.push_back("VK_LAYER_LUNARG_monitor" );
    //_ctx.instance_layers.push_back("VK_LAYER_LUNARG_object_tracker");
    //_ctx.instance_layers.push_back("VK_LAYER_LUNARG_parameter_validation");
    //_ctx.instance_layers.push_back("VK_LAYER_LUNARG_screenshot" );
    //_ctx.instance_layers.push_back("VK_LAYER_LUNARG_standard_validation");
    //_ctx.instance_layers.push_back("VK_LAYER_LUNARG_swapchain" ); // pas sur mon portable. deprecated?
    //_ctx.instance_layers.push_back("VK_LAYER_GOOGLE_threading");
    //_ctx.instance_layers.push_back("VK_LAYER_GOOGLE_unique_objects");
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
void Renderer::SetupLayers() {}
void Renderer::SetupExtensions()
{
    _ctx.instance_extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    _ctx.instance_extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);

    _ctx.device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}
void Renderer::SetupDebug() {}
bool Renderer::InitDebug() { return true; }
void Renderer::DeInitDebug() {}

#endif // BUILD_ENABLE_VULKAN_DEBUG

void Renderer::SetupFeatures()
{
    _ctx.features.fillModeNonSolid = VK_TRUE;
    _ctx.features.samplerAnisotropy = VK_TRUE;
}

bool Renderer::InitVma()
{
#if USE_VMA == 1
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
#else
    return true;
#endif
}

void Renderer::DeInitVma()
{
#if USE_VMA == 1
    vmaDestroyAllocator(_allocator);
#endif
}

bool Renderer::InitSynchronizations()
{
    VkResult result;

    Log("#     Create two semaphores and a fence per parallel frame\n");
    for (uint32_t i = 0; i < MAX_PARALLEL_FRAMES; ++i)
    {
        VkSemaphoreCreateInfo semaphore_create_info = {};
        semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        result = vkCreateSemaphore(_ctx.device, &semaphore_create_info, nullptr, &_render_complete_semaphores[i]);
        ErrorCheck(result);
        if (result != VK_SUCCESS)
            return false;

        result = vkCreateSemaphore(_ctx.device, &semaphore_create_info, nullptr, &_present_complete_semaphores[i]);
        ErrorCheck(result);
        if (result != VK_SUCCESS)
            return false;

        VkFenceCreateInfo fence_create_info = {};
        fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT; // we are starting the rendering by a wait on a fence.
        result = vkCreateFence(_ctx.device, &fence_create_info, nullptr, &_render_fences[i]);
        ErrorCheck(result);
        if (result != VK_SUCCESS)
            return false;

        VkFenceCreateInfo compute_fence_create_info = {};
        fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT; // we are starting the rendering by a wait on a fence.
        result = vkCreateFence(_ctx.device, &compute_fence_create_info, nullptr, &_compute_fences[i]);
        ErrorCheck(result);
        if (result != VK_SUCCESS)
            return false;
    }

    return true;
}

void Renderer::DeInitSynchronizations()
{
    for (uint32_t i = 0; i < MAX_PARALLEL_FRAMES; ++i)
    {
        vkDestroyFence(_ctx.device, _render_fences[i], nullptr);
        vkDestroySemaphore(_ctx.device, _render_complete_semaphores[i], nullptr);
        vkDestroySemaphore(_ctx.device, _present_complete_semaphores[i], nullptr);
    }
}









//
// DRAW
//
void Renderer::Update(float dt)
{
    _scene->update(dt);
}

void Renderer::Draw(float dt)
{
    VkResult result;

    auto &compute_cmd = _ctx.compute.command_buffers[current_frame];
    _scene->record_compute_commands(compute_cmd);

    // CPU wait for the end of the previous same parallel frame.
    // If we want to render frame 1 of 2 parallel frames, wait for
    // the end of the previous frame 1.
    vkWaitForFences(_ctx.device, 1, &_render_fences[current_frame], VK_TRUE, UINT64_MAX);
    vkResetFences(_ctx.device, 1, &_render_fences[current_frame]);

    _scene->upload(); // upload uniforms for graphics and compute

    //
    // COMPUTE
    //
    {
        VkPipelineStageFlags wait_stage_mask[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.waitSemaphoreCount = 0;
        submit_info.pWaitSemaphores = nullptr;
        submit_info.pWaitDstStageMask = 0;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &compute_cmd;
        submit_info.signalSemaphoreCount = 0;
        submit_info.pSignalSemaphores = nullptr;

        result = vkQueueSubmit(_ctx.compute.queue, 1, &submit_info, _compute_fences[current_frame]);
        ErrorCheck(result);
    }

    // Begin render = acquire image and set semaphore to be signaled when presenting
    // engine is done reading that frame.
    _w->BeginRender(_present_complete_semaphores[current_frame]);

    auto &cmd = _ctx.graphics.command_buffers[current_frame];

    // Record command buffer
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    result = vkBeginCommandBuffer(cmd, &begin_info);
    ErrorCheck(result);
    {
#if 1
        // barrier for reading from uniform buffer after all writing is done:
        VkMemoryBarrier uniform_memory_barrier = {};
        uniform_memory_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        uniform_memory_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT; // the vkFlushMappedMemoryRanges is a "host" command.
        uniform_memory_barrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;

        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_HOST_BIT,
            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
            0,
            1, &uniform_memory_barrier,
            0, nullptr,
            0, nullptr);
#endif

        VkRect2D render_area = {};
        render_area.offset = { 0, 0 };
        render_area.extent = _w->surface_size();

        // NOTE: these values are used only if the attachment has the loadOp LOAD_OP_CLEAR
        std::array<VkClearValue, 2> clear_values = {};
        clear_values[0].depthStencil.depth = 1.0f;
        clear_values[0].depthStencil.stencil = 0;
        // cornflower blue #6495ED - 100 149 237
        // partly clouded sky 214 224 255
        glm::vec4 bg_color = _scene->bg_color();
        clear_values[1].color.float32[0] = bg_color.r; // R // backbuffer is of type B8G8R8A8_UNORM
        clear_values[1].color.float32[1] = bg_color.g; // G
        clear_values[1].color.float32[2] = bg_color.b; // B
        clear_values[1].color.float32[3] = bg_color.a; // A

        VkRenderPassBeginInfo render_pass_begin_info = {};
        render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_begin_info.renderPass = _render_pass;
        render_pass_begin_info.framebuffer = _swapchain_framebuffers[_w->active_swapchain_image_id()];
        render_pass_begin_info.renderArea = render_area;
        render_pass_begin_info.clearValueCount = (uint32_t)clear_values.size();
        render_pass_begin_info.pClearValues = clear_values.data();

        vkCmdBeginRenderPass(cmd, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
        {
            VkViewport viewport = { 0, 0, (float)_global_viewport.width, (float)_global_viewport.height, 0, 1 };
            VkRect2D scissor = { 0, 0, _global_viewport.width, _global_viewport.height };
            _scene->draw(cmd, viewport, scissor);
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
        }
        vkCmdEndRenderPass(cmd);

    // NO NEED to transition from OPTIMAL to PRESENT at the end, if already specified in the render pass.
    }
    result = vkEndCommandBuffer(cmd); // compiles the command buffer
    ErrorCheck(result);

    // Submit command buffer
    VkPipelineStageFlags wait_stage_mask[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 1;
    // the pipeline stage COLOR_ATTACH_OUTPUT has to wait for the semaphore saying
    //  that the FBO is available to write to = finished reading by the present engine.
    submit_info.pWaitSemaphores = &_present_complete_semaphores[current_frame];
    submit_info.pWaitDstStageMask = wait_stage_mask;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd;
    // signals this semaphore when the render is complete GPU side
    // so that the presentation can begin presenting.
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &_render_complete_semaphores[current_frame];

    result = vkQueueSubmit(_ctx.graphics.queue, 1, &submit_info, _render_fences[current_frame]);
    ErrorCheck(result);

    // Present the frame after having waited on the rendering to be finished.
    _w->EndRender({ _render_complete_semaphores[current_frame] });

    // Next parallel frame.
    current_frame = (current_frame + 1) % MAX_PARALLEL_FRAMES;
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
    command_buffer_allocate_info.commandBufferCount = MAX_PARALLEL_FRAMES;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // primary can be pushed to a queue manually, secondary cannot.

    result = vkAllocateCommandBuffers(_ctx.device, &command_buffer_allocate_info, _ctx.graphics.command_buffers.data());
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;




    Log("#  Create Transfer Command Pool\n");
    {
        VkCommandPoolCreateInfo pool_create_info = {};
        pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_create_info.queueFamilyIndex = _ctx.transfer.family_index;
        pool_create_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | // commands will be short lived, might be reset of freed often.
            VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // we are going to reset

        result = vkCreateCommandPool(_ctx.device, &pool_create_info, nullptr, &_ctx.transfer.command_pool);
        ErrorCheck(result);
        if (result != VK_SUCCESS)
            return false;
    }

    Log("#  Allocate 1 Transfer Command Buffer\n");
    {
        VkCommandBufferAllocateInfo command_buffer_allocate_info{};
        command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        command_buffer_allocate_info.commandPool = _ctx.transfer.command_pool;
        command_buffer_allocate_info.commandBufferCount = MAX_PARALLEL_FRAMES;
        command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // primary can be pushed to a queue manually, secondary cannot.

        result = vkAllocateCommandBuffers(_ctx.device, &command_buffer_allocate_info, _ctx.transfer.command_buffers.data());
        ErrorCheck(result);
        if (result != VK_SUCCESS)
            return false;
    }

    return true;
}

void Renderer::DeInitCommandBuffer()
{
    Log("#  Destroy Graphics Command Pool\n");
    vkDestroyCommandPool(_ctx.device, _ctx.graphics.command_pool, nullptr);

    Log("#  Destroy Transfer Command Pool\n");
    vkDestroyCommandPool(_ctx.device, _ctx.transfer.command_pool, nullptr);
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
    // Our first subpass will wait for the COLOR_ATTACH_OUTPUT to begin, so the
    // auto transition will happen after the swap chain image is ready to write,
    // because we put a semaphore wait on that same stage in the submit info.
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; 
    dependency.srcAccessMask = 0;
    // the operations waiting are read/write operations on the out color.
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

bool Renderer::InitDescriptorPool()
{
    VkResult result;

    VkDescriptorPoolSize pool_sizes[] =
    {
        {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}
    };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
    pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;

    result = vkCreateDescriptorPool(_ctx.device, &pool_info, nullptr, &_ctx.descriptor_pool);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    return true;
}

void Renderer::DeInitDescriptorPool()
{
    vkDestroyDescriptorPool(_ctx.device, _ctx.descriptor_pool, nullptr);
}

//
