#include "utils.h"

#include <map>
#include <array>

namespace vk
{
    bool create_host_vertex_buffer(context *ctx, uint64_t size, buffer_t *dst)
    {
        return create_buffer(ctx, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, dst);
    }

    bool create_resident_host_vertex_buffer(context *ctx, uint64_t size, buffer_t *dst)
    {
        return create_buffer(ctx, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, dst);
    }

    bool create_buffer(context *ctx, uint64_t size_in_bytes, VkBufferUsageFlags usage, VkMemoryPropertyFlags desired_memory_flags, buffer_t *dst_buffer)
    {
        VkResult result;

        VkBufferCreateInfo vertex_buffer_create_info = {};
        vertex_buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        vertex_buffer_create_info.size = size_in_bytes;
        vertex_buffer_create_info.usage = usage;
        vertex_buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        result = vkCreateBuffer(ctx->device, &vertex_buffer_create_info, nullptr, &dst_buffer->buffer);
        if (result != VK_SUCCESS)
            return false;

        VkMemoryRequirements vertex_buffer_memory_requirements = {};
        vkGetBufferMemoryRequirements(ctx->device, dst_buffer->buffer, &vertex_buffer_memory_requirements);

        VkMemoryAllocateInfo vertex_buffer_allocate_info = {};
        vertex_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        vertex_buffer_allocate_info.allocationSize = vertex_buffer_memory_requirements.size;

        uint32_t vertex_memory_type_bits = vertex_buffer_memory_requirements.memoryTypeBits;
        for (uint32_t i = 0; i < ctx->physical_device_memory_properties.memoryTypeCount; ++i)
        {
            VkMemoryType memory_type = ctx->physical_device_memory_properties.memoryTypes[i];
            if (vertex_memory_type_bits & 1)
            {
                if ((memory_type.propertyFlags & desired_memory_flags) == desired_memory_flags) {
                    vertex_buffer_allocate_info.memoryTypeIndex = i;
                    break;
                }
            }
            vertex_memory_type_bits = vertex_memory_type_bits >> 1;
        }

        result = vkAllocateMemory(ctx->device, &vertex_buffer_allocate_info, nullptr, &dst_buffer->memory);
        if (result != VK_SUCCESS)
            return false;

        result = vkBindBufferMemory(ctx->device, dst_buffer->buffer, dst_buffer->memory, 0);
        if (result != VK_SUCCESS)
            return false;

        return true;
    }



    //
    // ICO SPHERE
    //

    using Lookup = std::map<std::pair<index_t, index_t>, index_t>;

    index_t vertex_for_edge(
        Lookup& lookup,
        VertexList& vertices, 
        index_t first, index_t second)
    {
        Lookup::key_type key(first, second);
        if (key.first > key.second)
            std::swap(key.first, key.second);

        auto inserted = lookup.insert({key, vertices.size()});
        if (inserted.second)
        {
            auto& edge0 = vertices[first];
            auto& edge1 = vertices[second];
            auto point = glm::normalize(edge0 + edge1);
            vertices.push_back(point);
        }

        return inserted.first->second;
    }

    TriangleList subdivide(
        VertexList& vertices,
        TriangleList triangles)
    {
        Lookup lookup;
        TriangleList result;

        for (auto&& each : triangles)
        {
            std::array<index_t, 3> mid;
            for (int edge = 0; edge < 3; ++edge)
            {
                mid[edge] = vertex_for_edge(lookup, vertices,
                    each.vertex[edge], each.vertex[(edge + 1) % 3]);
            }

            result.push_back({each.vertex[0], mid[0], mid[2]});
            result.push_back({each.vertex[1], mid[1], mid[0]});
            result.push_back({each.vertex[2], mid[2], mid[1]});
            result.push_back({mid[0], mid[1], mid[2]});
        }

        return result;
    }

    using IndexedMesh = std::pair<VertexList, TriangleList>;

    IndexedMesh make_icosphere(int subdivisions)
    {
        VertexList vertices = icosahedron::vertices;
        TriangleList triangles = icosahedron::triangles;

        for (int i = 0; i < subdivisions; ++i)
        {
            triangles = subdivide(vertices, triangles);
        }

        return{vertices, triangles};
    }

} // namespace vk
