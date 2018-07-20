#include "build_options.h"
#include "platform.h"
#include "scene.h"
#include "Renderer.h"
#include "Shared.h"

#include <array>

Scene::Scene(Renderer *r) : _r(r)
{

}

Scene::~Scene()
{
    VkDevice device = _r->GetVulkanDevice();

    // TODO: free buffers for each object
    for (auto o : _objects)
    {
        //Log("#   Free Memory\n");
        vkFreeMemory(device, o.vertex_buffer_memory, nullptr);
        vkFreeMemory(device, o.index_buffer_memory, nullptr);

        //Log("#   Destroy Buffer\n");
        vkDestroyBuffer(device, o.vertex_buffer, nullptr);
        vkDestroyBuffer(device, o.index_buffer, nullptr);
    }
}

bool Scene::add_object(object_description_t od)
{
    assert(_nb_objects <= MAX_OBJECTS);
    _object_t &obj = _objects[_nb_objects++];
    
    VkResult result;
    VkDevice device = _r->GetVulkanDevice();

    obj.vertexCount = od.vertexCount;
    obj.indexCount = od.indexCount;

    //Log("#   Create Vertex Buffer\n");
    std::array<VkBufferCreateInfo,2> buffer_create_infos = {};
    buffer_create_infos[0].sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_infos[0].size = od.vertexCount * sizeof(vertex_t);
    buffer_create_infos[0].usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    buffer_create_infos[0].sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    buffer_create_infos[1].sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_infos[1].size = od.indexCount * sizeof(index_t);
    buffer_create_infos[1].usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    buffer_create_infos[1].sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    result = vkCreateBuffer(device, &buffer_create_infos[0], nullptr, &obj.vertex_buffer);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    result = vkCreateBuffer(device, &buffer_create_infos[1], nullptr, &obj.index_buffer);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    //Log("#   Get Vertex Buffer Memory Requirements(size and type)\n");
    std::array<VkMemoryRequirements,2> buffer_memory_requirements = {};
    vkGetBufferMemoryRequirements(device, obj.vertex_buffer, &buffer_memory_requirements[0]);
    vkGetBufferMemoryRequirements(device, obj.index_buffer, &buffer_memory_requirements[1]);

    std::array<VkMemoryAllocateInfo,2> memory_allocate_infos = {};
    memory_allocate_infos[0].sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_allocate_infos[0].allocationSize = buffer_memory_requirements[0].size;

    memory_allocate_infos[1].sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_allocate_infos[1].allocationSize = buffer_memory_requirements[1].size;

    for (int m = 0; m < 2; ++m)
    {
        uint32_t memory_type_bits = buffer_memory_requirements[m].memoryTypeBits;
        VkMemoryPropertyFlags desired_memory_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        for (uint32_t i = 0; i < _r->GetVulkanPhysicalDeviceMemoryProperties().memoryTypeCount; ++i)
        {
            VkMemoryType memory_type = _r->GetVulkanPhysicalDeviceMemoryProperties().memoryTypes[i];
            if (memory_type_bits & 1)
            {
                if ((memory_type.propertyFlags & desired_memory_flags) == desired_memory_flags) 
                {
                    memory_allocate_infos[m].memoryTypeIndex = i;
                    break;
                }
            }
            memory_type_bits = memory_type_bits >> 1;
        }
    }

    //Log("#   Allocate Vertex Buffer Memory\n");
    result = vkAllocateMemory(device, &memory_allocate_infos[0], nullptr, &obj.vertex_buffer_memory);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    result = vkAllocateMemory(device, &memory_allocate_infos[1], nullptr, &obj.index_buffer_memory);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;


    //Log("#   Map Vertex Buffer\n");
    void *mapped = nullptr;

    result = vkMapMemory(device, obj.vertex_buffer_memory, 0, VK_WHOLE_SIZE, 0, &mapped);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    vertex_t *vertices = (vertex_t*)mapped;
    memcpy(vertices, od.vertices, od.vertexCount * sizeof(vertex_t));

    //Log("#   UnMap Vertex Buffer\n");
    vkUnmapMemory(device, obj.vertex_buffer_memory);

    result = vkBindBufferMemory(device, obj.vertex_buffer, obj.vertex_buffer_memory, 0);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;


    //Log("#   Map Vertex Buffer\n");
    //void *mapped = nullptr;

    result = vkMapMemory(device, obj.index_buffer_memory, 0, VK_WHOLE_SIZE, 0, &mapped);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    index_t *indices = (index_t*)mapped;
    memcpy(indices, od.indices, od.indexCount * sizeof(index_t));

    //Log("#   UnMap Vertex Buffer\n");
    vkUnmapMemory(device, obj.index_buffer_memory);

    result = vkBindBufferMemory(device, obj.index_buffer, obj.index_buffer_memory, 0);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    return true;
}

void Scene::draw_object(uint32_t object_index, VkCommandBuffer cmd)
{
    assert(object_index < _nb_objects);
    const _object_t &obj = _objects[object_index];

    VkDeviceSize offsets = {};
    vkCmdBindVertexBuffers(cmd, 0, 1, &obj.vertex_buffer, &offsets);
    vkCmdBindIndexBuffer(cmd, obj.index_buffer, 0, VK_INDEX_TYPE_UINT16);
    vkCmdDrawIndexed(cmd, obj.indexCount, 1, 0, 0, 0);
}

void Scene::draw_all_objects(VkCommandBuffer cmd)
{
    for (uint32_t i = 0; i < _nb_objects; ++i)
    {
        draw_object(i, cmd);
    }
}

