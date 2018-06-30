#pragma once

#ifdef _WIN32 // alwqys defined in windows, even on win64

#define VK_USE_PLATFORM_WIN32_KHR
#define VK_NO_PROTOTYPES
#include "volk.h"

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>

#else
#	error Platform not yet supported
#endif
