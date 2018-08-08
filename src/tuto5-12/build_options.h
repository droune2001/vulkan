#pragma once

#ifndef NDEBUG
#   define BUILD_ENABLE_VULKAN_DEBUG           1
#   define BUILD_ENABLE_VULKAN_RUNTIME_DEBUG   1
#   define ENABLE_LOG                          1
#   define EXTRA_VERBOSE                       1
#else
#   define BUILD_ENABLE_VULKAN_DEBUG           0
#   define BUILD_ENABLE_VULKAN_RUNTIME_DEBUG   0
#   define ENABLE_LOG                          1
#   define EXTRA_VERBOSE                       0
#endif
