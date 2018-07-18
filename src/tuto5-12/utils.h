#ifndef _VULKAN_UTILS_2018_07_18_H_
#define _VULKAN_UTILS_2018_07_18_H_

#include "build_options.h"
#include "platform.h"

namespace vk {

    struct context
    {
        VkInstance                       instance;
        VkPhysicalDevice                 physical_device;
        VkDevice                         device;
        VkQueue                          graphics_queue;
        VkQueue                          compute_queue;
        VkQueue                          transfer_queue;
        uint32_t                         graphics_family_index;
        uint32_t                         compute_family_index;
        uint32_t                         transfer_family_index;
        VkPhysicalDeviceProperties       physical_device_properties;
        VkPhysicalDeviceMemoryProperties physical_device_memory_properties;
    };

    struct buffer_t
    {
        VkBuffer buffer;
        VkDeviceMemory memory;
    };

    /*
    * usage:
    *  create_buffer(my_device, sizeof(struct vertex) * 3, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &buffer);
    */
    bool create_buffer(context *ctx, uint64_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags desired_memory_flags, buffer_t *dst);
    bool create_host_vertex_buffer(context *ctx, uint64_t size, buffer_t *dst);
    bool create_resident_vertex_buffer(context *ctx, uint64_t size, buffer_t *dst);

    // sub-section of a buffer
    struct vertex_buffer_t
    {
        uint32_t offset; // offset inside the buffer
        uint32_t size;   // size in bytes of the data in the buffer
        buffer_t buffer; // ref to the buffer in which the data is
    };
}

#endif // !_VULKAN_UTILS_2018_07_18_H_
