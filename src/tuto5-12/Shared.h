#pragma once

#include "build_options.h"

#include <iostream>
#include <assert.h>

#if BUILD_ENABLE_VULKAN_RUNTIME_DEBUG

void _ErrorCheck( VkResult result );
#define ErrorCheck(e) _ErrorCheck(e)

#else

#define ErrorCheck(e)

#endif

void _LogCStr(const char *text);
void _LogStr(const std::string &str);

#if ENABLE_LOG == 1
#   define Log(a) _LogStr(a)
#else
#   define Log(a)
#endif

uint32_t FindMemoryTypeIndex(
    const VkPhysicalDeviceMemoryProperties *gpu_memory_properties, 
    const VkMemoryRequirements *memory_requirements, 
    const VkMemoryPropertyFlags required_memory_properties);
