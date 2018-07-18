#ifndef VMA_USAGE_H_
#define VMA_USAGE_H_

#define VMA_USE_STL_CONTAINERS 1
#define VMA_HEAVY_ASSERT(expr) assert(expr)

#pragma warning(push, 4)
#pragma warning(disable: 4127) // conditional expression is constant
#pragma warning(disable: 4100) // unreferenced formal parameter
#pragma warning(disable: 4189) // local variable is initialized but not referenced

#include "vk_mem_alloc.h"

#pragma warning(pop)

#endif
