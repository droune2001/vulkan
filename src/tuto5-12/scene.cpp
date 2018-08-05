#include "build_options.h"
#include "platform.h"
#include "scene.h"
#include "Renderer.h"
#include "Shared.h"
#include "utils.h"

#include <array>
#include <string>

#define MAX_NB_OBJECTS 1024

Scene::Scene(vulkan_context *c) : _ctx(c)
{

}

Scene::~Scene()
{
    de_init();
}

bool Scene::init(VkRenderPass rp)
{
    Log("#    Create Global Objects VBO/IBO/UBO\n");
    if (!create_global_object_buffers())
        return false;

    Log("#    Create Scene UBO\n");
    if (!create_scene_ubo())
        return false;
    
    Log("#    Create Procedural Textures\n");
    if (!create_procedural_textures())
        return false;

    Log("#    Create Texture Samplers\n");
    if (!create_texture_samplers())
        return false;

    Log("#    Create Default Material\n");
    if (!create_default_material(rp))
        return false;

    return true;
}

void Scene::de_init()
{
    vkQueueWaitIdle(_ctx->graphics.queue);

    Log("#   Destroy Materials\n");
    destroy_materials();

    Log("#   Destroy Procedural Textures\n");
    destroy_textures();

    Log("#   Destroy Uniform Buffers\n");
    if (_global_object_vbo_created || _global_object_ibo_created || _global_object_ubo_created)
    {
        destroy_global_object_buffers();
    }
    if (_scene_ubo_created) destroy_scene_ubo();
}

bool Scene::add_object(object_description_t od)
{
    Log("#   Add Object\n");

    VkResult result;
    VkDevice device = _ctx->device;

    // with lazy init
    auto &global_vbo = get_global_object_vbo();
    auto &global_ibo = get_global_object_ibo();
    auto &global_ubo = get_global_object_ubo();

    _object_t obj = {};
    obj.position        = od.position;
    obj.vertexCount     = od.vertexCount;
    obj.vertex_buffer   = global_vbo.buffer;
    obj.vertex_offset   = global_vbo.offset;
    obj.indexCount      = od.indexCount;
    obj.index_buffer    = global_ibo.buffer;
    obj.index_offset    = global_ibo.offset;
    obj.material_id     = od.material;
    obj.diffuse_texture = od.diffuse_texture;

    Log(std::string("#    v: ") + std::to_string(od.vertexCount) + std::string(" i: ") + std::to_string(od.indexCount) + "\n");

    void *mapped = nullptr;

    Log("#    Map Vertex Buffer\n");
    size_t vertex_data_size = od.vertexCount * sizeof(vertex_t);

    Log("#     offset: " + std::to_string(global_vbo.offset) + std::string(" size: ") + std::to_string(vertex_data_size) + "\n");
    result = vkMapMemory(device, global_vbo.memory, global_vbo.offset, vertex_data_size/*VK_WHOLE_SIZE*/, 0, &mapped);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    vertex_t *vertices = (vertex_t*)mapped;
    memcpy(vertices, od.vertices, vertex_data_size);

    Log("#    UnMap Vertex Buffer\n");
    vkUnmapMemory(device, global_vbo.memory);

    global_vbo.offset += (uint32_t)vertex_data_size;




    Log("#    Map Index Buffer\n");
    size_t index_data_size = od.indexCount * sizeof(index_t);

    Log("#     offset: " + std::to_string(global_ibo.offset) + std::string(" size: ") + std::to_string(index_data_size) + "\n");
    result = vkMapMemory(device, global_ibo.memory, global_ibo.offset, index_data_size/*VK_WHOLE_SIZE*/, 0, &mapped);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    index_t *indices = (index_t*)mapped;
    memcpy(indices, od.indices, index_data_size);

    Log("#    UnMap Index Buffer\n");
    vkUnmapMemory(device, global_ibo.memory);

    global_ibo.offset += (uint32_t)index_data_size;

    Log("#    Compute ModelMatrix and put it in the aligned buffer\n");
    glm::mat4* model_mat = (glm::mat4*)(((uint64_t)_model_matrices + (_objects.size() * _dynamic_alignment)));
    *model_mat = glm::translate(glm::mat4(1), od.position);
  
    _objects.push_back(obj);

    return true;
}

bool Scene::add_light(light_description_t li)
{
    _light_t light = {};
    light.color = glm::vec4(li.color, 1);
    light.position = glm::vec4(li.position, 1);
    
    _lights.push_back(light);

    return true;
}

bool Scene::add_camera(camera_description_t ca)
{
    _camera_t camera = {};
    camera.v = glm::lookAt(ca.position, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    camera.p = glm::perspective(ca.fovy, ca.aspect, ca.near_plane, ca.far_plane);
    camera.p[1][1] *= -1.0f;

    _cameras.push_back(camera);

    return true;
}

void Scene::update(float dt)
{   
    animate_object(dt);
    animate_camera(dt);
}

void Scene::draw(VkCommandBuffer cmd, VkViewport viewport, VkRect2D scissor_rect)
{
    // RENDER PASS BEGIN ---

    update_scene_ubo();
    update_all_objects_ubos();

    // pipeline barrier pour flush ubo ??

    auto default_material = _materials["default"];

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, default_material.pipeline);

    // scene, one time
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, default_material.pipeline_layout, 
        0, // bind to set #0
        1, &default_material.descriptor_sets[0], 0, nullptr);

    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor_rect);

    for (size_t i = 0; i < _objects.size(); ++i)
    {
        const _object_t &obj = _objects[i];

        // Bind Attribs Vertex/Index
        VkDeviceSize offsets = obj.vertex_offset;
        vkCmdBindVertexBuffers(cmd, 0, 1, &obj.vertex_buffer, &offsets);
        vkCmdBindIndexBuffer(cmd, obj.index_buffer, obj.index_offset, VK_INDEX_TYPE_UINT16);

        // ith object offset into dynamic ubo
        uint32_t dynamic_offset = static_cast<uint32_t>(i * _dynamic_alignment);

        // Bind Per-Object Uniforms
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, default_material.pipeline_layout, 
            1, // bind as set #1
            1, &default_material.descriptor_sets[1], 
            1, &dynamic_offset); // dynamic offsets

        vkCmdDrawIndexed(cmd, obj.indexCount, 1, 0, 0, 0);
    }

    

    // RENDER PASS END ---
}

// =====================================================

bool Scene::create_global_object_buffers()
{
    VkResult result;

    // Find the memory alignment for the object matrices;
    size_t min_ubo_alignment = _ctx->physical_device_properties.limits.minUniformBufferOffsetAlignment;
    _dynamic_alignment = sizeof(glm::mat4);
    if (min_ubo_alignment > 0) 
    {
        _dynamic_alignment = (_dynamic_alignment + min_ubo_alignment - 1) & ~(min_ubo_alignment - 1);
    }
     _dynamic_buffer_size = MAX_NB_OBJECTS * _dynamic_alignment;
    _model_matrices = (glm::mat4*)utils::aligned_alloc(_dynamic_buffer_size, _dynamic_alignment);
    for (size_t i = 0; i < MAX_NB_OBJECTS; ++i)
    {
        glm::mat4 *model_mat_for_obj_i = (glm::mat4*)((uint64_t)_model_matrices + (i * _dynamic_alignment));
        *model_mat_for_obj_i = glm::mat4(1);
    }

    std::array<VkBufferCreateInfo, 3> buffer_create_infos = {};
    // VBO
    buffer_create_infos[0].sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_infos[0].size = 4 * 1024 * 1024;// od.vertexCount * sizeof(vertex_t);
    buffer_create_infos[0].usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT; // <-- VBO
    buffer_create_infos[0].sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    // IBO
    buffer_create_infos[1].sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_infos[1].size = 4 * 1024 * 1024; // od.indexCount * sizeof(index_t);
    buffer_create_infos[1].usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT; // <-- IBO
    buffer_create_infos[1].sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    // UBO
    buffer_create_infos[2].sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_infos[2].size = _dynamic_buffer_size;// 1024 aligned matrices
    buffer_create_infos[2].usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;   // <-- UBO
    buffer_create_infos[2].sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // VBO
    Log("#     Create Buffer VBO\n");
    result = vkCreateBuffer(_ctx->device, &buffer_create_infos[0], nullptr, &_global_object_vbo.buffer);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;
    // IBO
    Log("#     Create Buffer IBO\n");
    result = vkCreateBuffer(_ctx->device, &buffer_create_infos[1], nullptr, &_global_object_ibo.buffer);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;
    // UBO
    Log("#     Create Buffer UBO\n");
    result = vkCreateBuffer(_ctx->device, &buffer_create_infos[2], nullptr, &_global_object_ubo.buffer);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    Log("#     Get Global Object V/I/U Buffer Memory Requirements(size and type)\n");
    std::array<VkMemoryRequirements, 3> buffer_memory_requirements = {};
    vkGetBufferMemoryRequirements(_ctx->device, _global_object_vbo.buffer, &buffer_memory_requirements[0]);
    vkGetBufferMemoryRequirements(_ctx->device, _global_object_ibo.buffer, &buffer_memory_requirements[1]);
    vkGetBufferMemoryRequirements(_ctx->device, _global_object_ubo.buffer, &buffer_memory_requirements[2]);

    std::array<VkMemoryAllocateInfo, 3> memory_allocate_infos = {};
    memory_allocate_infos[0].sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_allocate_infos[0].allocationSize = buffer_memory_requirements[0].size;

    memory_allocate_infos[1].sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_allocate_infos[1].allocationSize = buffer_memory_requirements[1].size;

    memory_allocate_infos[2].sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_allocate_infos[2].allocationSize = buffer_memory_requirements[2].size;

    for (size_t m = 0; m < buffer_memory_requirements.size(); ++m)
    {
        uint32_t memory_type_bits = buffer_memory_requirements[m].memoryTypeBits;
        VkMemoryPropertyFlags desired_memory_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT; // TODO: ON device, use staging buffer
        auto mem_props = _ctx->physical_device_memory_properties;
        for (uint32_t i = 0; i < mem_props.memoryTypeCount; ++i)
        {
            if (memory_type_bits & 1)
            {
                if ((mem_props.memoryTypes[i].propertyFlags & desired_memory_flags) == desired_memory_flags)
                {
                    memory_allocate_infos[m].memoryTypeIndex = i;
                    break;
                }
            }
            memory_type_bits = memory_type_bits >> 1;
        }
    }
    
    Log("#     Allocate Global Object V/I/U Buffer Memory\n");
    result = vkAllocateMemory(_ctx->device, &memory_allocate_infos[0], nullptr, &_global_object_vbo.memory);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    result = vkAllocateMemory(_ctx->device, &memory_allocate_infos[1], nullptr, &_global_object_ibo.memory);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    result = vkAllocateMemory(_ctx->device, &memory_allocate_infos[2], nullptr, &_global_object_ubo.memory);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    Log("#     Bind Global Object V/I/U Buffer Memory\n");
    result = vkBindBufferMemory(_ctx->device, _global_object_vbo.buffer, _global_object_vbo.memory, 0);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    result = vkBindBufferMemory(_ctx->device, _global_object_ibo.buffer, _global_object_ibo.memory, 0);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    result = vkBindBufferMemory(_ctx->device, _global_object_ubo.buffer, _global_object_ubo.memory, 0);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    _global_object_vbo_created = true;
    _global_object_ibo_created = true;
    _global_object_ubo_created = true;

    return true;
}

void Scene::destroy_global_object_buffers()
{
    Log("#    Free Global Object Buffers Memory\n");
    vkFreeMemory(_ctx->device, _global_object_vbo.memory, nullptr);
    vkFreeMemory(_ctx->device, _global_object_ibo.memory, nullptr);
    vkFreeMemory(_ctx->device, _global_object_ubo.memory, nullptr);

    Log("#    Destroy Global Object Buffers\n");
    vkDestroyBuffer(_ctx->device, _global_object_vbo.buffer, nullptr);
    vkDestroyBuffer(_ctx->device, _global_object_ibo.buffer, nullptr);
    vkDestroyBuffer(_ctx->device, _global_object_ubo.buffer, nullptr);

    _global_object_vbo_created = false;
    _global_object_ibo_created = false;
    _global_object_ubo_created = false;
}

// lazy creation - can do it at the beginning.
Scene::vertex_buffer_object_t &Scene::get_global_object_vbo()
{
    if (!_global_object_vbo_created)
    {
        if (!create_global_object_buffers())
        {
            assert("could not create vbo");
        }
    }

    return _global_object_vbo;
}

Scene::vertex_buffer_object_t &Scene::get_global_object_ibo()
{
    if (!_global_object_ibo_created)
    {
        if (!create_global_object_buffers())
        {
            assert("could not create ibo");
        }
    }

    return _global_object_ibo;
}

Scene::uniform_buffer_t &Scene::get_global_object_ubo()
{
    if (!_global_object_ubo_created)
    {
        if (!create_global_object_buffers())
        {
            assert("could not create objects ubo");
        }
    }

    return _global_object_ubo;
}





Scene::uniform_buffer_t &Scene::get_scene_ubo()
{
    if (!_scene_ubo_created)
    {
        if (!create_scene_ubo())
        {
            assert("could not create scene ubo");
        }
    }

    return _scene_ubo;
}

bool Scene::create_scene_ubo()
{
    VkResult result;

    Log("#     Create Matrices Uniform Buffer\n");
    VkBufferCreateInfo uniform_buffer_create_info = {};
    uniform_buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    uniform_buffer_create_info.size = sizeof(_camera_t)+sizeof(_light_t); // one camera and one light
    uniform_buffer_create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;   // <-- UBO
    uniform_buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    result = vkCreateBuffer(_ctx->device, &uniform_buffer_create_info, nullptr, &_scene_ubo.buffer);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    Log("#     Get Uniform Buffer Memory Requirements(size and type)\n");
    VkMemoryRequirements uniform_buffer_memory_requirements = {};
    vkGetBufferMemoryRequirements(_ctx->device, _scene_ubo.buffer, &uniform_buffer_memory_requirements);

    VkMemoryAllocateInfo uniform_buffer_allocate_info = {};
    uniform_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    uniform_buffer_allocate_info.allocationSize = uniform_buffer_memory_requirements.size;

    uint32_t uniform_memory_type_bits = uniform_buffer_memory_requirements.memoryTypeBits;
    VkMemoryPropertyFlags uniform_desired_memory_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    auto mem_props = _ctx->physical_device_memory_properties;
    for (uint32_t i = 0; i < mem_props.memoryTypeCount; ++i)
    {
        VkMemoryType memory_type = mem_props.memoryTypes[i];
        if (uniform_memory_type_bits & 1)
        {
            if ((memory_type.propertyFlags & uniform_desired_memory_flags) == uniform_desired_memory_flags) {
                uniform_buffer_allocate_info.memoryTypeIndex = i;
                break;
            }
        }
        uniform_memory_type_bits = uniform_memory_type_bits >> 1;
    }

    Log("#     Allocate Uniform Buffer Memory\n");
    result = vkAllocateMemory(_ctx->device, &uniform_buffer_allocate_info, nullptr, &_scene_ubo.memory);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    Log("#     Bind memory to buffer\n");
    result = vkBindBufferMemory(_ctx->device, _scene_ubo.buffer, _scene_ubo.memory, 0);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    _scene_ubo_created = true;

    return true;
}

void Scene::destroy_scene_ubo()
{
    Log("#    Free Memory\n");
    vkFreeMemory(_ctx->device, _scene_ubo.memory, nullptr);

    Log("#    Destroy Buffer\n");
    vkDestroyBuffer(_ctx->device, _scene_ubo.buffer, nullptr);
}




void Scene::animate_object(float dt)
{
    static float obj_x = 0.0f;
    static float obj_y = 0.0f;
    static float obj_z = 0.0f;
    static float accum = 0.0f; // in seconds

    accum += dt;
    float speed = 3.0f; // radian/sec
    float radius = 1.5f;
    obj_x = radius * std::cos(speed * accum);
    obj_y = radius * std::sin(2.0f * speed * accum);
    obj_z = 0.5f * radius * std::cos(speed * accum);

    auto &obj_0 = _objects[0];
    auto &obj_1 = _objects[1];

    // Aligned offset
    glm::mat4* model_mat_obj_0 = (glm::mat4*)(((uint64_t)_model_matrices + (0 * _dynamic_alignment)));
    glm::mat4* model_mat_obj_1 = (glm::mat4*)(((uint64_t)_model_matrices + (1 * _dynamic_alignment)));

    *model_mat_obj_0 = glm::translate(glm::mat4(1), obj_0.position + glm::vec3(obj_x, obj_y, obj_z));
    *model_mat_obj_1 = glm::translate(glm::mat4(1), obj_1.position + glm::vec3(-obj_x, obj_y, -obj_z));
}

void Scene::animate_camera(float dt)
{
    const float camera_speed = 1.0f; // unit per second

    if (_camera_anim.cameraZ < 5)
    {
        _camera_anim.cameraZ = 5;
        _camera_anim.cameraZDir = 1;
    }
    else if (_camera_anim.cameraZ > 10)
    {
        _camera_anim.cameraZ = 10;
        _camera_anim.cameraZDir = -1;
    }

    _camera_anim.cameraZ += _camera_anim.cameraZDir * camera_speed * dt;

    // Update first camera position
    _cameras[0].v = glm::lookAt(glm::vec3(0,0,_camera_anim.cameraZ), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

    //
    // LIGHT anim
    //
    static float accum_dt = 0.0f;
    accum_dt += dt;
    const float r = 4.0f; // radius
    const float as = 0.8f; // angular_speed, radians/sec
    float lx = r * std::cos(as * accum_dt);
    float ly = r * std::sin(as * accum_dt);
    float lz = r * std::cos(2.0f * as * accum_dt);
    _lights[0].position = glm::vec4(lx, ly, lz, 1.0f);
}

bool Scene::update_scene_ubo()
{
    auto &scene_ubo = get_scene_ubo();
    auto camera = _cameras[0];
    auto light = _lights[0];

    VkResult result;

    //Log("#   Map Uniform Buffer\n");
    void *mapped = nullptr;
    result = vkMapMemory(_ctx->device, _scene_ubo.memory, 0, VK_WHOLE_SIZE, 0, &mapped);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    //Log("#   Copy matrices, first time.\n");
    memcpy(mapped, glm::value_ptr(camera.v), sizeof(camera.v));
    memcpy(((float *)mapped + 16), glm::value_ptr(camera.p), sizeof(camera.p));
    
    memcpy(((float *)mapped + 32), glm::value_ptr(light.color), sizeof(light.color));
    memcpy(((float *)mapped + 32 + 4), glm::value_ptr(light.position), sizeof(light.position));

    VkMappedMemoryRange memory_range = {};
    memory_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    memory_range.memory = _scene_ubo.memory;
    memory_range.offset = 0;
    memory_range.size = VK_WHOLE_SIZE;
    vkFlushMappedMemoryRanges(_ctx->device, 1, &memory_range);

    //Log("#   UnMap Uniform Buffer\n");
    vkUnmapMemory(_ctx->device, _scene_ubo.memory);

    return true;
}

bool Scene::update_all_objects_ubos()
{
    // TODO: update only modified(animated) matrices.

    auto &ubo = get_global_object_ubo();
    
    VkResult result;
    
    void *mapped = nullptr;
    result = vkMapMemory(_ctx->device, ubo.memory, 0, VK_WHOLE_SIZE, 0, &mapped);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    memcpy(mapped, _model_matrices, _objects.size() * _dynamic_alignment);

    // TODO: find out when this is really needed.
    VkMappedMemoryRange memory_range = {};
    memory_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    memory_range.memory = ubo.memory;
    memory_range.offset = 0;
    memory_range.size = VK_WHOLE_SIZE;
    vkFlushMappedMemoryRanges(_ctx->device, 1, &memory_range);

    vkUnmapMemory(_ctx->device, ubo.memory);

    return true;
}

bool Scene::create_texture_2d(void *data, size_t size, _texture_t *texture)
{
    VkResult result;
    auto device = _ctx->device;

    VkImageCreateInfo texture_create_info = {};
    texture_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    texture_create_info.imageType = VK_IMAGE_TYPE_2D;
    texture_create_info.format = texture->format;
    texture_create_info.extent = texture->extent;
    texture_create_info.mipLevels = 1;
    texture_create_info.arrayLayers = 1;
    texture_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    texture_create_info.tiling = VK_IMAGE_TILING_LINEAR;
    texture_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    texture_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    texture_create_info.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED; // we will fill it so dont flush content when changing layout.

    Log("#     Create Image\n");
    result = vkCreateImage(device, &texture_create_info, nullptr, &texture->image);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    VkMemoryRequirements texture_memory_requirements = {};
    vkGetImageMemoryRequirements(device, texture->image, &texture_memory_requirements);

    VkMemoryAllocateInfo texture_image_allocate_info = {};
    texture_image_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    texture_image_allocate_info.allocationSize = texture_memory_requirements.size;

    uint32_t texture_memory_type_bits = texture_memory_requirements.memoryTypeBits;
    VkMemoryPropertyFlags tDesiredMemoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT; // TODO: use staging buffer
    for (uint32_t i = 0; i < 32; ++i) 
    {
        VkMemoryType memory_type = _ctx->physical_device_memory_properties.memoryTypes[i];
        if (texture_memory_type_bits & 1)
        {
            if ((memory_type.propertyFlags & tDesiredMemoryFlags) == tDesiredMemoryFlags)
            {
                texture_image_allocate_info.memoryTypeIndex = i;
                break;
            }
        }
        texture_memory_type_bits = texture_memory_type_bits >> 1;
    }

    Log("#     Allocate Memory\n");
    result = vkAllocateMemory(device, &texture_image_allocate_info, nullptr, &texture->image_memory);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    result = vkBindImageMemory(device, texture->image, texture->image_memory, 0);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    Log("#     Map/Fill/Flush/UnMap\n");
    void *image_mapped;
    result = vkMapMemory(device, texture->image_memory, 0, VK_WHOLE_SIZE, 0, &image_mapped);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    memcpy(image_mapped, data, size);

    VkMappedMemoryRange memory_range = {};
    memory_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    memory_range.memory = texture->image_memory;
    memory_range.offset = 0;
    memory_range.size = VK_WHOLE_SIZE;
    vkFlushMappedMemoryRanges(device, 1, &memory_range);

    vkUnmapMemory(device, texture->image_memory);

    return true;
}

bool Scene::transition_textures()
{
    VkResult result;
    auto device = _ctx->device;

    VkFence submit_fence = {};
    VkFenceCreateInfo fence_create_info = {};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    vkCreateFence(device, &fence_create_info, nullptr, &submit_fence);

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    auto &cmd = _ctx->graphics.command_buffer;

    vkBeginCommandBuffer(cmd, &begin_info);

    std::vector<VkImageMemoryBarrier> layout_transition_barriers = {};
    layout_transition_barriers.reserve(_textures.size());
    layout_transition_barriers.resize(_textures.size());
    size_t i = 0;
    for ( auto &t : _textures)
    {
        layout_transition_barriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        layout_transition_barriers[i].srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        layout_transition_barriers[i].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        layout_transition_barriers[i].oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
        layout_transition_barriers[i].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        layout_transition_barriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        layout_transition_barriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        layout_transition_barriers[i].image = t.second.image;
        layout_transition_barriers[i].subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        ++i;
    }

    Log("#     Transition all textures\n");
    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_HOST_BIT,
        VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
        0,
        0, nullptr,
        0, nullptr,
        (uint32_t)layout_transition_barriers.size(), 
        layout_transition_barriers.data());

    vkEndCommandBuffer(cmd);

    VkPipelineStageFlags wait_stage_mask[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = nullptr;
    submit_info.pWaitDstStageMask = wait_stage_mask;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = nullptr;
    result = vkQueueSubmit(_ctx->graphics.queue, 1, &submit_info, submit_fence);

    vkWaitForFences(device, 1, &submit_fence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &submit_fence);
    vkResetCommandBuffer(cmd, 0);

    vkDestroyFence(device, submit_fence, nullptr);

    return true;
}

bool Scene::create_procedural_textures()
{
    Log("#     Compute Procedural Texture\n");
    
    utils::loaded_image checker_image;
    utils::create_checker_image(&checker_image);

    auto &checker_texture = _textures["checker"];
    checker_texture.format = VK_FORMAT_R32G32B32_SFLOAT;
    checker_texture.extent = { checker_image.width, checker_image.height, 1};
    if (!create_texture_2d(checker_image.data, checker_image.size, &checker_texture))
        return false;
    
    delete[] checker_image.data;


    utils::loaded_image default_image;
    utils::create_default_image(&default_image);

    auto &default_texture = _textures["default"];
    default_texture.format = VK_FORMAT_R8G8B8A8_UNORM;// VK_FORMAT_R8G8B8_UINT;
    default_texture.extent = { default_image.width, default_image.height, 1 };
    if (!create_texture_2d(default_image.data, default_image.size, &default_texture))
        return false;

    delete[] default_image.data;

    //
    // TRANSITION
    //

    if (!transition_textures())
        return false;

    //
    // TEXTURE VIEWS
    //

    VkResult result;

    for (auto &t : _textures)
    {
        VkImageViewCreateInfo texture_image_view_create_info = {};
        texture_image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        texture_image_view_create_info.image = t.second.image;
        texture_image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        texture_image_view_create_info.format = t.second.format;
        texture_image_view_create_info.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
        texture_image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        texture_image_view_create_info.subresourceRange.baseMipLevel = 0;
        texture_image_view_create_info.subresourceRange.levelCount = 1;
        texture_image_view_create_info.subresourceRange.baseArrayLayer = 0;
        texture_image_view_create_info.subresourceRange.layerCount = 1;

        Log("#     Create Image View\n");
        result = vkCreateImageView(_ctx->device, &texture_image_view_create_info, nullptr, &t.second.view);
        ErrorCheck(result);
        if (result != VK_SUCCESS)
            return false;
    }
    
    return true;
}

bool Scene::create_texture_samplers()
{
    VkResult result;

    VkSamplerCreateInfo sampler_create_info = {};
    sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_create_info.magFilter = VK_FILTER_LINEAR;
    sampler_create_info.minFilter = VK_FILTER_LINEAR;
    sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_create_info.mipLodBias = 0;
    sampler_create_info.anisotropyEnable = VK_FALSE;
    sampler_create_info.minLod = 0;
    sampler_create_info.maxLod = 5;
    sampler_create_info.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    sampler_create_info.unnormalizedCoordinates = VK_FALSE;

    Log("#     Create Sampler\n");
    result = vkCreateSampler(_ctx->device, &sampler_create_info, nullptr, &_samplers[0]);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    return true;
}

void Scene::destroy_textures()
{
    for (auto t : _textures)
    {
        auto tex = t.second;
        vkDestroyImageView(_ctx->device, tex.view, nullptr);
        vkDestroyImage(_ctx->device, tex.image, nullptr);
        vkFreeMemory(_ctx->device, tex.image_memory, nullptr);
    }

    for (auto s : _samplers)
    {
        vkDestroySampler(_ctx->device, s, nullptr);
    }
}

bool Scene::create_shader_module(const std::string &file_path, VkShaderModule *shader_module)
{
    auto content = utils::read_file_content(file_path);

    VkShaderModuleCreateInfo shader_creation_info = {};
    shader_creation_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_creation_info.codeSize = content.size();
    shader_creation_info.pCode = (uint32_t *)content.data();

    VkResult result;

    Log("#      Create Shader Module\n");
    result = vkCreateShaderModule(_ctx->device, &shader_creation_info, nullptr, shader_module);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    return true;
}

bool Scene::create_default_descriptor_set_layout()
{
    _material_t &default_material = _materials["default"];

    VkResult result;
    
    // 2 Descriptor sets
    // set = 0 for scene uniforms (camera matrices, lights)
    // set = 1 for object uniforms (model matrix)

    //
    // PER-SCENE
    //

    std::array<VkDescriptorSetLayoutBinding, 2> scene_bindings = {};

    // layout( set = 0, binding = 0 ) uniform buffer { mat4 v; mat4 p; vec4 light_pos; vec4 light_color; } Scene_UBO;
    scene_bindings[0].binding = 0; // <---- value used in the shader itself
    scene_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    scene_bindings[0].descriptorCount = 1; // si j'ai 2 ubo, je mets 2 ou j'en fais 2??
    scene_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    scene_bindings[0].pImmutableSamplers = nullptr;

    // layout ( set = 0, binding = 1 ) uniform sampler2D diffuse_tex_checker_texure._sampler;
    scene_bindings[1].binding = 1; // <---- value used in the shader itself
    scene_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    scene_bindings[1].descriptorCount = 1;
    scene_bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    scene_bindings[1].pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo scene_set_layout_create_info = {};
    scene_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    scene_set_layout_create_info.bindingCount = (uint32_t)scene_bindings.size();
    scene_set_layout_create_info.pBindings = scene_bindings.data();

    Log("#      Create Default Descriptor Set Layout for Scene Uniforms\n");
    result = vkCreateDescriptorSetLayout(_ctx->device, &scene_set_layout_create_info, nullptr, &default_material.descriptor_set_layouts[0]);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    // NOTE: on doit pouvoir mettre les 3 bindings dans 1 seul descriptor_layout

    //
    // PER-OBJECT
    //

    std::array<VkDescriptorSetLayoutBinding, 1> object_bindings = {};
    // layout( set = 1, binding = 0 ) uniform buffer { mat4 m; } Object_UBO;
    object_bindings[0].binding = 0; // <---- value used in the shader itself
    object_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    object_bindings[0].descriptorCount = 1;
    object_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    object_bindings[0].pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo object_set_layout_create_info = {};
    object_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    object_set_layout_create_info.bindingCount = (uint32_t)object_bindings.size();
    object_set_layout_create_info.pBindings = object_bindings.data();

    Log("#      Create Default Descriptor Set Layout for Objects Uniforms\n");
    result = vkCreateDescriptorSetLayout(_ctx->device, &object_set_layout_create_info, nullptr, &default_material.descriptor_set_layouts[1]);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;




    // allocate just enough for two uniform descriptor sets
    std::array<VkDescriptorPoolSize, 3> uniform_buffer_pool_sizes = {};
    uniform_buffer_pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniform_buffer_pool_sizes[0].descriptorCount = 1; // scene ubo

    uniform_buffer_pool_sizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    uniform_buffer_pool_sizes[1].descriptorCount = 1; // dynamic object ubo

    uniform_buffer_pool_sizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    uniform_buffer_pool_sizes[2].descriptorCount = 1; // texture sampler;

    VkDescriptorPoolCreateInfo pool_create_info = {};
    pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_create_info.maxSets = 2;
    pool_create_info.poolSizeCount = (uint32_t)uniform_buffer_pool_sizes.size();
    pool_create_info.pPoolSizes = uniform_buffer_pool_sizes.data();

    Log("#      Create Default Descriptor Set Pool\n");
    result = vkCreateDescriptorPool(_ctx->device, &pool_create_info, nullptr, &default_material.descriptor_pool);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    VkDescriptorSetAllocateInfo descriptor_allocate_info = {};
    descriptor_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_allocate_info.descriptorPool = default_material.descriptor_pool;
    descriptor_allocate_info.descriptorSetCount = 2; // scene and obj sets
    descriptor_allocate_info.pSetLayouts = default_material.descriptor_set_layouts; // [0] [1]

    Log("#      Allocate 2 Descriptor Sets\n");
    result = vkAllocateDescriptorSets(_ctx->device, &descriptor_allocate_info, default_material.descriptor_sets); // [0] [1]
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    //
    // CONFIGURE DESCRIPTOR SETS
    //
    // When a set is allocated all values are undefined and all 
    // descriptors are uninitialised. must init all statically used bindings:


    VkDescriptorBufferInfo scene_ubo_descriptor_buffer_info = {};
    scene_ubo_descriptor_buffer_info.buffer = _scene_ubo.buffer;
    scene_ubo_descriptor_buffer_info.offset = 0;
    scene_ubo_descriptor_buffer_info.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet scene_ubo_write_descriptor_set = {};
    scene_ubo_write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    scene_ubo_write_descriptor_set.dstSet = default_material.descriptor_sets[0]; // <-- scene descriptor set
    scene_ubo_write_descriptor_set.dstBinding = 0; // binding = 0 in shaders
    scene_ubo_write_descriptor_set.dstArrayElement = 0;
    scene_ubo_write_descriptor_set.descriptorCount = 1;
    scene_ubo_write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    scene_ubo_write_descriptor_set.pImageInfo = nullptr;
    scene_ubo_write_descriptor_set.pBufferInfo = &scene_ubo_descriptor_buffer_info;
    scene_ubo_write_descriptor_set.pTexelBufferView = nullptr;

    Log("#      Update Descriptor Set (Scene UBO)\n");
    vkUpdateDescriptorSets(_ctx->device, 1, &scene_ubo_write_descriptor_set, 0, nullptr);

    VkDescriptorImageInfo descriptor_image_info = {};
    descriptor_image_info.sampler = _samplers[0];
    descriptor_image_info.imageView = _textures["checker"].view; //_textures["checker"].view;
    descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_PREINITIALIZED; // why ? we did change the layout manually!! VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL

    VkWriteDescriptorSet write_descriptor_for_checker_texture_sampler = {};
    write_descriptor_for_checker_texture_sampler.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_for_checker_texture_sampler.dstSet = default_material.descriptor_sets[0]; // <-- scene descriptor set
    write_descriptor_for_checker_texture_sampler.dstBinding = 1; // binding = 1 on shaders
    write_descriptor_for_checker_texture_sampler.dstArrayElement = 0;
    write_descriptor_for_checker_texture_sampler.descriptorCount = 1;
    write_descriptor_for_checker_texture_sampler.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    //VK_DESCRIPTOR_TYPE_SAMPLER
    //VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
    // TODO: find how to pass a fixed sampler, and change image view for each object.
    write_descriptor_for_checker_texture_sampler.pImageInfo = &descriptor_image_info;
    write_descriptor_for_checker_texture_sampler.pBufferInfo = nullptr;
    write_descriptor_for_checker_texture_sampler.pTexelBufferView = nullptr;

    Log("#      Update Descriptor Set (Scene texture sampler)\n");
    vkUpdateDescriptorSets(_ctx->device, 1, &write_descriptor_for_checker_texture_sampler, 0, nullptr);

    VkDescriptorBufferInfo object_ubo_descriptor_buffer_info = {};
    object_ubo_descriptor_buffer_info.buffer = _global_object_ubo.buffer;
    object_ubo_descriptor_buffer_info.offset = 0;
    object_ubo_descriptor_buffer_info.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet object_ubo_write_descriptor_set = {};
    object_ubo_write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    object_ubo_write_descriptor_set.dstSet = default_material.descriptor_sets[1]; // <-- object descriptor set
    object_ubo_write_descriptor_set.dstBinding = 0; // binding = 0 in shaders
    object_ubo_write_descriptor_set.dstArrayElement = 0;
    object_ubo_write_descriptor_set.descriptorCount = 1;
    object_ubo_write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    object_ubo_write_descriptor_set.pImageInfo = nullptr;
    object_ubo_write_descriptor_set.pBufferInfo = &object_ubo_descriptor_buffer_info;
    object_ubo_write_descriptor_set.pTexelBufferView = nullptr;

    Log("#      Update Descriptor Set (Object UBO)\n");
    vkUpdateDescriptorSets(_ctx->device, 1, &object_ubo_write_descriptor_set, 0, nullptr);

    return true;
}

bool Scene::create_default_pipeline(VkRenderPass rp)
{
    _material_t &default_material = _materials["default"];

    VkResult result;

    // use it later to define uniform buffer
    VkPipelineLayoutCreateInfo layout_create_info = {};
    layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_create_info.setLayoutCount = 2; // <--- 2 sets, scene and object
    layout_create_info.pSetLayouts = default_material.descriptor_set_layouts;
    layout_create_info.pushConstantRangeCount = 0;
    layout_create_info.pPushConstantRanges = nullptr; // constant into shader for opti???

    Log("#      Fill Pipeline Layout (with 2 sets)\n");
    result = vkCreatePipelineLayout(_ctx->device, &layout_create_info, nullptr, &default_material.pipeline_layout);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    std::array<VkPipelineShaderStageCreateInfo, 2> shader_stage_create_infos = {};
    shader_stage_create_infos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage_create_infos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shader_stage_create_infos[0].module = default_material.vs;
    shader_stage_create_infos[0].pName = "main";        // shader entry point function name
    shader_stage_create_infos[0].pSpecializationInfo = nullptr;

    shader_stage_create_infos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage_create_infos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shader_stage_create_infos[1].module = default_material.fs;
    shader_stage_create_infos[1].pName = "main";        // shader entry point function name
    shader_stage_create_infos[1].pSpecializationInfo = nullptr;


    // TODO: put in model

    VkVertexInputBindingDescription vertex_binding_description = {};
    vertex_binding_description.binding = 0;
    vertex_binding_description.stride = sizeof(Scene::vertex_t);
    vertex_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    // bind input to location=0/1/2, binding=0
    VkVertexInputAttributeDescription vertex_attribute_description[3] = {};
    vertex_attribute_description[0].location = 0;
    vertex_attribute_description[0].binding = 0;
    vertex_attribute_description[0].format = VK_FORMAT_R32G32B32A32_SFLOAT; // position = 4 float
    vertex_attribute_description[0].offset = offsetof(Scene::vertex_t, p); //0;

    vertex_attribute_description[1].location = 1;
    vertex_attribute_description[1].binding = 0;
    vertex_attribute_description[1].format = VK_FORMAT_R32G32B32_SFLOAT; // normal = 3 floats
    vertex_attribute_description[1].offset = offsetof(Scene::vertex_t, n);//4 * sizeof(float); 

    vertex_attribute_description[2].location = 2;
    vertex_attribute_description[2].binding = 0;
    vertex_attribute_description[2].format = VK_FORMAT_R32G32_SFLOAT; // uv = 2 floats
    vertex_attribute_description[2].offset = offsetof(Scene::vertex_t, uv); // (4 + 3) * sizeof(float);

    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {};
    vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
    vertex_input_state_create_info.pVertexBindingDescriptions = &vertex_binding_description;
    vertex_input_state_create_info.vertexAttributeDescriptionCount = 3;
    vertex_input_state_create_info.pVertexAttributeDescriptions = vertex_attribute_description;

    // vertex topology config = triangles
    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {};
    input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = 512.0f; // dynamic, will set the correct size later.
    viewport.height = 512.0f;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissors = {};
    scissors.offset = { 0, 0 };
    scissors.extent = { 512, 512 };

    VkPipelineViewportStateCreateInfo viewport_state_create_info = {};
    viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_create_info.viewportCount = 1;
    viewport_state_create_info.pViewports = &viewport;
    viewport_state_create_info.scissorCount = 1;
    viewport_state_create_info.pScissors = &scissors;

    VkPipelineRasterizationStateCreateInfo raster_state_create_info = {};
    raster_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster_state_create_info.depthClampEnable = VK_FALSE;
    raster_state_create_info.rasterizerDiscardEnable = VK_FALSE;
    raster_state_create_info.polygonMode = VK_POLYGON_MODE_FILL; //VK_POLYGON_MODE_LINE;// VK_POLYGON_MODE_FILL;
    raster_state_create_info.cullMode = VK_CULL_MODE_NONE;
    raster_state_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;// VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster_state_create_info.depthBiasEnable = VK_FALSE;
    raster_state_create_info.depthBiasConstantFactor = 0;
    raster_state_create_info.depthBiasClamp = 0;
    raster_state_create_info.depthBiasSlopeFactor = 0;
    raster_state_create_info.lineWidth = 1;

    VkPipelineMultisampleStateCreateInfo multisample_state_create_info = {};
    multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample_state_create_info.sampleShadingEnable = VK_FALSE;
    multisample_state_create_info.minSampleShading = 0;
    multisample_state_create_info.pSampleMask = nullptr;
    multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
    multisample_state_create_info.alphaToOneEnable = VK_FALSE;

    VkStencilOpState noOP_stencil_state = {};
    noOP_stencil_state.failOp = VK_STENCIL_OP_KEEP;
    noOP_stencil_state.passOp = VK_STENCIL_OP_KEEP;
    noOP_stencil_state.depthFailOp = VK_STENCIL_OP_KEEP;
    noOP_stencil_state.compareOp = VK_COMPARE_OP_ALWAYS;
    noOP_stencil_state.compareMask = 0;
    noOP_stencil_state.writeMask = 0;
    noOP_stencil_state.reference = 0;

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info = {};
    depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state_create_info.depthTestEnable = VK_TRUE;
    depth_stencil_state_create_info.depthWriteEnable = VK_TRUE;
    depth_stencil_state_create_info.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state_create_info.stencilTestEnable = VK_FALSE;
    depth_stencil_state_create_info.front = noOP_stencil_state;
    depth_stencil_state_create_info.back = noOP_stencil_state;
    depth_stencil_state_create_info.minDepthBounds = 0;
    depth_stencil_state_create_info.maxDepthBounds = 0;

    VkPipelineColorBlendAttachmentState color_blend_attachment_state = {};
    color_blend_attachment_state.blendEnable = VK_FALSE;
    color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
    color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
    color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment_state.colorWriteMask = 0xf; // all components.

    VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = {};
    color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state_create_info.logicOpEnable = VK_FALSE;
    color_blend_state_create_info.logicOp = VK_LOGIC_OP_CLEAR;
    color_blend_state_create_info.attachmentCount = 1;
    color_blend_state_create_info.pAttachments = &color_blend_attachment_state;
    color_blend_state_create_info.blendConstants[0] = 0.0;
    color_blend_state_create_info.blendConstants[1] = 0.0;
    color_blend_state_create_info.blendConstants[2] = 0.0;
    color_blend_state_create_info.blendConstants[3] = 0.0;

    std::array<VkDynamicState, 2> dynamic_state = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {};
    dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_create_info.dynamicStateCount = (uint32_t)dynamic_state.size();
    dynamic_state_create_info.pDynamicStates = dynamic_state.data();

    VkGraphicsPipelineCreateInfo pipeline_create_info = {};
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_create_info.stageCount = (uint32_t)shader_stage_create_infos.size();
    pipeline_create_info.pStages = shader_stage_create_infos.data();
    pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;
    pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;
    pipeline_create_info.pTessellationState = nullptr;
    pipeline_create_info.pViewportState = &viewport_state_create_info;
    pipeline_create_info.pRasterizationState = &raster_state_create_info;
    pipeline_create_info.pMultisampleState = &multisample_state_create_info;
    pipeline_create_info.pDepthStencilState = &depth_stencil_state_create_info;
    pipeline_create_info.pColorBlendState = &color_blend_state_create_info;
    pipeline_create_info.pDynamicState = &dynamic_state_create_info;
    pipeline_create_info.layout = default_material.pipeline_layout;
    pipeline_create_info.renderPass = rp; // TODO: create a render pass inside Scene
    pipeline_create_info.subpass = 0;
    pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE; // only if VK_PIPELINE_CREATE_DERIVATIVE flag is set.
    pipeline_create_info.basePipelineIndex = 0;

    Log("#      Create Pipeline\n");
    result = vkCreateGraphicsPipelines(
        _ctx->device,
        VK_NULL_HANDLE, // cache
        1,
        &pipeline_create_info,
        nullptr,
        &default_material.pipeline);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    return true;
}

bool Scene::create_default_material(VkRenderPass rp)
{
    _material_t &default_material = _materials["default"];

    Log("#     Create Default Vertex Shader\n");
    if (!create_shader_module("./simple.vert.spv", &default_material.vs))
        return false;

    Log("#     Create Default Fragment Shader\n");
    if (!create_shader_module("./simple.frag.spv", &default_material.fs))
        return false;

    Log("#     Create Default Descriptor Set Layout\n");
    if (!create_default_descriptor_set_layout())
        return false;

    Log("#     Create Default Pipeline\n");
    if (!create_default_pipeline(rp))
        return false;

    return true;
}

void Scene::destroy_materials()
{
    for (auto m : _materials)
    {
        auto mat = m.second;

        Log("#    Destroy Shader Modules\n");
        vkDestroyShaderModule(_ctx->device, mat.vs, nullptr);
        vkDestroyShaderModule(_ctx->device, mat.fs, nullptr);

        Log("#    Destroy Descriptor Pool\n");
        vkDestroyDescriptorPool(_ctx->device, mat.descriptor_pool, nullptr);

        Log("#    Destroy Descriptor Set Layout\n");
        for (int i = 0; i < 2; ++i)
        {
            vkDestroyDescriptorSetLayout(_ctx->device, mat.descriptor_set_layouts[i], nullptr);
        }
        
        Log("#    Destroy Pipeline\n");
        vkDestroyPipeline(_ctx->device, mat.pipeline, nullptr);

        Log("#    Destroy Pipeline Layout\n");
        vkDestroyPipelineLayout(_ctx->device, mat.pipeline_layout, nullptr);
    }
}

bool Scene::add_material(material_description_t ma)
{
    _material_t material = {};

    // TODO

    _materials[ma.id] = material;

    return true;
}

//
