#include "platform.h"
#include "Shared.h"
#include "build_options.h"

#if BUILD_ENABLE_VULKAN_RUNTIME_DEBUG

void _ErrorCheck( VkResult result )
{
    if ( result < 0 )
    {
        switch ( result )
        {
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            Log("VK_ERROR_OUT_OF_HOST_MEMORY\n");
            break;
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            Log("VK_ERROR_OUT_OF_DEVICE_MEMORY\n");
            break;
        case VK_ERROR_INITIALIZATION_FAILED:
            Log("VK_ERROR_INITIALIZATION_FAILED\n");
            break;
        case VK_ERROR_DEVICE_LOST:
            Log("VK_ERROR_DEVICE_LOST\n");
            break;
        case VK_ERROR_MEMORY_MAP_FAILED:
            Log("VK_ERROR_MEMORY_MAP_FAILED\n");
            break;
        case VK_ERROR_LAYER_NOT_PRESENT:
            Log("VK_ERROR_LAYER_NOT_PRESENT\n");
            break;
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            Log("VK_ERROR_EXTENSION_NOT_PRESENT\n");
            break;
        case VK_ERROR_FEATURE_NOT_PRESENT:
            Log("VK_ERROR_FEATURE_NOT_PRESENT\n");
            break;
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            Log("VK_ERROR_INCOMPATIBLE_DRIVER\n");
            break;
        case VK_ERROR_TOO_MANY_OBJECTS:
            Log("VK_ERROR_TOO_MANY_OBJECTS\n");
            break;
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            Log("VK_ERROR_FORMAT_NOT_SUPPORTED\n");
            break;
        case VK_ERROR_FRAGMENTED_POOL:
            Log("VK_ERROR_FRAGMENTED_POOL\n");
            break;
        case VK_ERROR_SURFACE_LOST_KHR:
            Log("VK_ERROR_SURFACE_LOST_KHR\n");
            break;
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            Log("VK_ERROR_NATIVE_WINDOW_IN_USE_KHR\n");
            break;
        case VK_SUBOPTIMAL_KHR:
            Log("VK_SUBOPTIMAL_KHR\n");
            break;
        case VK_ERROR_OUT_OF_DATE_KHR:
            Log("VK_ERROR_OUT_OF_DATE_KHR\n");
            break;
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
            Log("VK_ERROR_INCOMPATIBLE_DISPLAY_KHR\n");
            break;
        case VK_ERROR_VALIDATION_FAILED_EXT:
            Log("VK_ERROR_VALIDATION_FAILED_EXT\n");
            break;
        case VK_ERROR_INVALID_SHADER_NV:
            Log("VK_ERROR_INVALID_SHADER_NV\n");
            break;
        case VK_ERROR_OUT_OF_POOL_MEMORY_KHR:
            Log("VK_ERROR_OUT_OF_POOL_MEMORY_KHR\n");
            break;
        case VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR:
            Log("VK_ERROR_INVALID_EXTERNAL_HANDLE_KHX\n");
            break;
        default:
            break;
        }
        assert( !"Vulkan runtime error!" );
    }
}

#endif // BUILD_ENABLE_VULKAN_RUNTIME_DEBUG

void Log(const char *text)
{
    std::cout << text;
    ::OutputDebugStringA(text);
}

void Log(const std::string &str)
{
    const char *cstr = str.c_str();
    Log(cstr);
}

uint32_t FindMemoryTypeIndex(
    const VkPhysicalDeviceMemoryProperties *gpu_memory_properties,
    const VkMemoryRequirements *memory_requirements,
    const VkMemoryPropertyFlags required_memory_properties)
{
    for (uint32_t i = 0; i < gpu_memory_properties->memoryTypeCount; ++i) 
    {
        if (memory_requirements->memoryTypeBits & (1 << i)) 
        {
            if ((gpu_memory_properties->memoryTypes[i].propertyFlags & required_memory_properties) == required_memory_properties)
            {
                return i;
            }
        }
    }
    assert(!"Failed at finding required memory type.");
    return UINT32_MAX;
}
