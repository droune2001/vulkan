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
