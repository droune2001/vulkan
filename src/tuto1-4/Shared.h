#pragma once

#include <iostream>
#include <assert.h>

#define VK_USE_PLATFORM_WIN32_KHR
#define VK_NO_PROTOTYPES
#include "volk.h"

void ErrorCheck( VkResult result );
