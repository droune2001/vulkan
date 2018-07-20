#ifndef _VULKAN_UTILS_2018_07_18_H_
#define _VULKAN_UTILS_2018_07_18_H_

#include "build_options.h"
#include "platform.h"
#include "scene.h" // vertext_t, index_t

#ifndef GLM_SWIZZLE
#   define GLM_SWIZZLE
#endif
#include <glm/glm.hpp>

#include <vector>

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

} // namespace vk


//
// ICO SPHERE
//

struct triangle_t
{
    Scene::index_t vertex_index[3];
};

using TriangleList = std::vector<triangle_t>;
using IndexList = std::vector<Scene::index_t>;
using VertexList = std::vector<Scene::vertex_t>;

namespace icosahedron
{
    const float X = .525731112119133606f;
    const float Z = .850650808352039932f;
    const float N = 0.f;

    static const VertexList vertices =
    {
        {{-X, N, Z, 1},{-X, N, Z},{0,0}},
        {{X, N, Z, 1},{X, N, Z},{0,0}},
        {{-X, N,-Z, 1},{-X, N,-Z},{0,0}},
        {{X, N,-Z, 1},{X, N,-Z},{0,0}},
        {{N, Z, X, 1},{N, Z, X},{0,0}},
        {{N, Z,-X, 1},{N, Z,-X},{0,0}},
        {{N,-Z, X, 1},{N,-Z, X},{0,0}},
        {{N,-Z,-X, 1},{N,-Z,-X},{0,0}},
        {{Z, X, N, 1},{Z, X, N},{0,0}},
        {{-Z, X, N, 1},{-Z, X, N},{0,0}},
        {{Z,-X, N, 1},{Z,-X, N},{0,0}},
        {{-Z,-X, N, 1},{-Z,-X, N},{0,0}},
    };

    static const TriangleList triangles =
    {
        {0,4,1},{0,9,4},{9,5,4},{4,5,8},{4,8,1},
        {8,10,1},{8,3,10},{5,3,8},{5,2,3},{2,7,3},
        {7,10,3},{7,6,10},{7,11,6},{11,0,6},{0,1,6},
        {6,1,10},{9,0,11},{9,11,2},{9,2,5},{7,2,11}
    };
}

using IndexedMesh = std::pair<VertexList, IndexList>;
IndexedMesh make_icosphere(int subdivisions);


#endif // !_VULKAN_UTILS_2018_07_18_H_
