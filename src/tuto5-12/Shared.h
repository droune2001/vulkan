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

void Log(const char *text);
void Log(const std::string &str);

uint32_t FindMemoryTypeIndex(
    const VkPhysicalDeviceMemoryProperties *gpu_memory_properties, 
    const VkMemoryRequirements *memory_requirements, 
    const VkMemoryPropertyFlags required_memory_properties);
