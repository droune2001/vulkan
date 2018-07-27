#include "build_options.h"
#include "platform.h"
#include "scene.h"
#include "Renderer.h"
#include "Shared.h"
#include "utils.h"

#include <array>
#include <string>

Scene::Scene(vulkan_context *c) : _ctx(c)
{

}

Scene::~Scene()
{
    de_init();
}

bool Scene::init()
{
    //Log("#    Init Uniform Buffer\n");
    //if (!InitUniformBuffer())
    //    return false;

    Log("#    Create Procedural Textures\n");
    if (!create_procedural_textures())
        return false;

    Log("#    Create Default Material\n");
    if (!create_default_material())
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

    Log("#   Destroy Uniform Buffer\n");
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

    _object_t obj = {};
    obj.model_matrix = od.model_matrix;
    obj.vertexCount = od.vertexCount;
    obj.indexCount = od.indexCount;
    obj.material_id = od.material;
    obj.diffuse_texture = od.diffuse_texture;

    Log(std::string("#    v: ") + std::to_string(od.vertexCount) + std::string(" i: ") + std::to_string(od.indexCount) + "\n");

    // with lazy init
    auto &global_vbo = get_global_object_vbo();
    auto &global_ibo = get_global_object_ibo();
    auto &global_ubo = get_global_object_ubo();

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
    
    //result = vkBindBufferMemory(device, global_vbo.buffer, global_vbo.memory, global_vbo.offset);
    //ErrorCheck(result);
    //if (result != VK_SUCCESS)
    //    return false;

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

    //result = vkBindBufferMemory(device, global_ibo.buffer, global_ibo.memory, global_ibo.offset);
    //ErrorCheck(result);
    //if (result != VK_SUCCESS)
    //    return false;

    global_ibo.offset += (uint32_t)index_data_size;



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
    // pipeline barrier pour flush ubo
#if 0
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelines[0]);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline_layout, 0, 1, &_descriptor_set, 0, nullptr);
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor_rect);

    draw_all_objects(cmd);
#endif
    // RENDER PASS END ---
}

// =====================================================

bool Scene::create_global_object_buffers()
{
    VkResult result;

    Log("#   * Create Global Object Vertex/Index/Uniform Buffers\n");

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
    buffer_create_infos[2].size = 4 * 1024 * 1024;// sizeof(_mvp); // size in bytes
    buffer_create_infos[2].usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;   // <-- UBO
    buffer_create_infos[2].sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // VBO
    result = vkCreateBuffer(_ctx->device, &buffer_create_infos[0], nullptr, &_global_object_vbo.buffer);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;
    // IBO
    result = vkCreateBuffer(_ctx->device, &buffer_create_infos[1], nullptr, &_global_object_ibo.buffer);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;
    // UBO
    result = vkCreateBuffer(_ctx->device, &buffer_create_infos[2], nullptr, &_global_object_ubo.buffer);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    Log("#   * Get Global Object V/I/U Buffer Memory Requirements(size and type)\n");
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
        VkMemoryPropertyFlags desired_memory_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT; // TODO: ON device, use staging buffer
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
    
    Log("#   * Allocate Global Object V/I/U Buffer Memory\n");
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

    Log("#   * Bind Global Object V/I/U Buffer Memory\n");
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
    Log("#   Free Global Object Buffers Memory\n");
    vkFreeMemory(_ctx->device, _global_object_vbo.memory, nullptr);
    vkFreeMemory(_ctx->device, _global_object_ibo.memory, nullptr);
    vkFreeMemory(_ctx->device, _global_object_ubo.memory, nullptr);

    Log("#   Destroy Global Object Buffers\n");
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
    // TODO
    return true;
}

void Scene::destroy_scene_ubo()
{
#if 0
    Log("#   Free Memory\n");
    vkFreeMemory(_ctx.device, _matrices_ubo.memory, nullptr);

    Log("#   Destroy Buffer\n");
    vkDestroyBuffer(_ctx.device, _matrices_ubo.buffer, nullptr);
#endif
}




void Scene::animate_object(float dt)
{
    static float obj_x = 0.0f;
    static float obj_y = 0.0f;
    static float obj_z = 0.0f;
    static float accum = 0.0f; // in seconds

    accum += dt;
    float speed = 3.0f; // radian/sec
    float radius = 2.0f;
    obj_x = radius * std::cos(speed * accum);
    obj_y = radius * std::sin(speed * accum);

    // Update first object position
    _objects[0].model_matrix = glm::translate(glm::mat4(1), glm::vec3(obj_x, obj_y, obj_z));
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
}

void Scene::update_scene_ubo()
{
    auto &scene_ubo = get_scene_ubo();

    // map
    //     copy view matrix
    //     copy proj matrix
    //     copy light color
    //     copy light position
    // unmap
}

void Scene::update_object_ubo(size_t i)
{
    const _object_t &obj = _objects[i];

    auto &global_object_vbo = get_global_object_ubo();

    // map
    //     copy model matrix
    // unmap
}

void Scene::draw_object(size_t object_index, VkCommandBuffer cmd)
{
    const _object_t &obj = _objects[object_index];

    // bind uniform buffer

    VkDeviceSize offsets = {};
    vkCmdBindVertexBuffers(cmd, 0, 1, &obj.vertex_buffer, &offsets);
    vkCmdBindIndexBuffer(cmd, obj.index_buffer, 0, VK_INDEX_TYPE_UINT16);
    vkCmdDrawIndexed(cmd, obj.indexCount, 1, 0, 0, 0);
}

void Scene::draw_all_objects(VkCommandBuffer cmd)
{
    for (size_t i = 0; i < _objects.size(); ++i)
    {
        update_object_ubo(i);
        draw_object(i, cmd);
    }
}

bool Scene::create_procedural_textures()
{
    Log("#     Compute Procedural Texture\n");
    auto &checker_texture = _textures["checker"];

    struct loaded_image 
    {
        int width;
        int height;
        void *data;
    };

    loaded_image test_image;
    test_image.width = 512;
    test_image.height = 512;
    test_image.data = (void *) new float[test_image.width * test_image.height * 3];

    for (uint32_t x = 0; x < (uint32_t)test_image.width; ++x) 
    {
        for (uint32_t y = 0; y < (uint32_t)test_image.height; ++y) 
        {
            float g = 0.3f;
            if (x % 40 < 20 && y % 40 < 20) {
                g = 1;
            }
            if (x % 40 >= 20 && y % 40 >= 20) {
                g = 1;
            }

            float *pixel = ((float *)test_image.data) + (x * test_image.height * 3) + (y * 3);
            pixel[0] = g * 0.4f;
            pixel[1] = g * 0.5f;
            pixel[2] = g * 0.7f;
        }
    }

    VkResult result;
    auto device = _ctx->device;

    VkImageCreateInfo texture_create_info = {};
    texture_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    texture_create_info.imageType = VK_IMAGE_TYPE_2D;
    texture_create_info.format = VK_FORMAT_R32G32B32_SFLOAT;
    texture_create_info.extent = { (uint32_t)test_image.width, (uint32_t)test_image.height, 1 };
    texture_create_info.mipLevels = 1;
    texture_create_info.arrayLayers = 1;
    texture_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    texture_create_info.tiling = VK_IMAGE_TILING_LINEAR;
    texture_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    texture_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    texture_create_info.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED; // we will fill it so dont flush content when changing layout.

    Log("#     Create Image\n");
    result = vkCreateImage(device, &texture_create_info, nullptr, &checker_texture.texture_image);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    VkMemoryRequirements texture_memory_requirements = {};
    vkGetImageMemoryRequirements(device, checker_texture.texture_image, &texture_memory_requirements);

    VkMemoryAllocateInfo texture_image_allocate_info = {};
    texture_image_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    texture_image_allocate_info.allocationSize = texture_memory_requirements.size;

    uint32_t texture_memory_type_bits = texture_memory_requirements.memoryTypeBits;
    VkMemoryPropertyFlags tDesiredMemoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    for (uint32_t i = 0; i < 32; ++i) {
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
    result = vkAllocateMemory(device, &texture_image_allocate_info, nullptr, &checker_texture.texture_image_memory);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    result = vkBindImageMemory(device, checker_texture.texture_image, checker_texture.texture_image_memory, 0);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    Log("#     Map/Fill/Flush/UnMap\n");
    void *image_mapped;
    result = vkMapMemory(device, checker_texture.texture_image_memory, 0, VK_WHOLE_SIZE, 0, &image_mapped);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    memcpy(image_mapped, test_image.data, sizeof(float) * test_image.width * test_image.height * 3);

    VkMappedMemoryRange memory_range = {};
    memory_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    memory_range.memory = checker_texture.texture_image_memory;
    memory_range.offset = 0;
    memory_range.size = VK_WHOLE_SIZE;
    vkFlushMappedMemoryRanges(device, 1, &memory_range);

    vkUnmapMemory(device, checker_texture.texture_image_memory);

    // we can clear the image data:
    delete[] test_image.data;

    //
    // TRANSITION
    //

    VkFence submit_fence = {};
    VkFenceCreateInfo fence_create_info = {};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    vkCreateFence(device, &fence_create_info, nullptr, &submit_fence);

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    auto &cmd = _ctx->graphics.command_buffer;

    vkBeginCommandBuffer(cmd, &begin_info);

    VkImageMemoryBarrier layout_transition_barrier = {};
    layout_transition_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    layout_transition_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    layout_transition_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    layout_transition_barrier.oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    layout_transition_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    layout_transition_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    layout_transition_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    layout_transition_barrier.image = checker_texture.texture_image;
    layout_transition_barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

    Log("#     Transition\n");
    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_HOST_BIT, //VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,//VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &layout_transition_barrier);

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

    VkImageViewCreateInfo texture_image_view_create_info = {};
    texture_image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    texture_image_view_create_info.image = checker_texture.texture_image;
    texture_image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    texture_image_view_create_info.format = VK_FORMAT_R32G32B32_SFLOAT;
    texture_image_view_create_info.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
    texture_image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    texture_image_view_create_info.subresourceRange.baseMipLevel = 0;
    texture_image_view_create_info.subresourceRange.levelCount = 1;
    texture_image_view_create_info.subresourceRange.baseArrayLayer = 0;
    texture_image_view_create_info.subresourceRange.layerCount = 1;

    Log("#     Create Image View\n");
    result = vkCreateImageView(device, &texture_image_view_create_info, nullptr, &checker_texture.texture_view);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

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
    result = vkCreateSampler(device, &sampler_create_info, nullptr, &checker_texture.sampler);
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
        vkDestroySampler(_ctx->device, tex.sampler, nullptr);
        vkDestroyImageView(_ctx->device, tex.texture_view, nullptr);
        vkDestroyImage(_ctx->device, tex.texture_image, nullptr);
        vkFreeMemory(_ctx->device, tex.texture_image_memory, nullptr);
    }
}

bool Scene::create_shader_module(const std::string &file_path, VkShaderModule *shader_module)
{
#if 0
    uint32_t codeSize;
    char *code = new char[10000];
    HANDLE fileHandle = nullptr;

    // load our vertex shader:
    fileHandle = ::CreateFile(file_path.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (fileHandle == INVALID_HANDLE_VALUE)
    {
        Log("!!!!!!!!! Failed to open shader file.\n");
        return false;
    }
    ::ReadFile((HANDLE)fileHandle, code, 10000, (LPDWORD)&codeSize, nullptr);
    ::CloseHandle(fileHandle);

    VkShaderModuleCreateInfo shader_creation_info = {};
    shader_creation_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_creation_info.codeSize = codeSize;
    shader_creation_info.pCode = (uint32_t *)code;
#else

    auto content = ReadFileContent(file_path);

    VkShaderModuleCreateInfo shader_creation_info = {};
    shader_creation_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_creation_info.codeSize = content.size();
    shader_creation_info.pCode = (uint32_t *)content.data();

#endif

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
    
    // layout( std140, binding = 0 ) uniform buffer { mat4 v; mat4 p; } Scene_UBO;
    std::array<VkDescriptorSetLayoutBinding, 3> bindings = {};
    bindings[0].binding = 0; // <---- value used in the shader itself
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = 1; // si j'ai 2 ubo, je mets 2 ou j'en fais 2??
    bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    bindings[0].pImmutableSamplers = nullptr;

    // layout( std140, binding = 1 ) uniform buffer { mat4 m; } Object_UBO;
    bindings[1].binding = 1; // <---- value used in the shader itself
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    bindings[1].pImmutableSamplers = nullptr;

    // layout ( set = 0, binding = 2 ) uniform sampler2D diffuse_tex_checker_texure._sampler;
    bindings[2].binding = 2; // <---- value used in the shader itself
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[2].descriptorCount = 1;
    bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[2].pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo set_layout_create_info = {};
    set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    set_layout_create_info.bindingCount = (uint32_t)bindings.size();
    set_layout_create_info.pBindings = bindings.data();

    Log("#      Create Default Descriptor Set Layout\n");
    result = vkCreateDescriptorSetLayout(_ctx->device, &set_layout_create_info, nullptr, &default_material.descriptor_set_layout);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    // allocate just enough for two uniform descriptor sets
    std::array<VkDescriptorPoolSize, 2> uniform_buffer_pool_sizes = {};
    uniform_buffer_pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniform_buffer_pool_sizes[0].descriptorCount = 2; // scene and object ubos

    uniform_buffer_pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    uniform_buffer_pool_sizes[1].descriptorCount = 1;

    VkDescriptorPoolCreateInfo pool_create_info = {};
    pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_create_info.maxSets = 1;
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
    descriptor_allocate_info.descriptorSetCount = 1;
    descriptor_allocate_info.pSetLayouts = &default_material.descriptor_set_layout;

    Log("#      Allocate Descriptor Set\n");
    result = vkAllocateDescriptorSets(_ctx->device, &descriptor_allocate_info, &default_material.descriptor_set);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    return true;
}

bool Scene::create_default_pipeline()
{
    _material_t &default_material = _materials["default"];


    return true;
}

bool Scene::create_default_material()
{
    _material_t &default_material = _materials["default"];

    Log("#     Create Default Vertex Shader\n");
    if (!create_shader_module("./vert.spv", &default_material.vs))
        return false;

    Log("#     Create Default Fragment Shader\n");
    if (!create_shader_module("./frag.spv", &default_material.fs))
        return false;

    Log("#     Create Default Descriptor Set Layout\n");
    if (!create_default_descriptor_set_layout())
        return false;

    Log("#     Create Default Pipeline\n");
    if (!create_default_pipeline())
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
        vkDestroyDescriptorSetLayout(_ctx->device, mat.descriptor_set_layout, nullptr);
#if 0
        Log("#    Destroy Pipeline\n");
        vkDestroyPipeline(_ctx->device, mat.pipeline, nullptr);

        Log("#    Destroy Pipeline Layout\n");
        vkDestroyPipelineLayout(_ctx->device, mat.pipeline_layout, nullptr);
#endif
    }
}

bool Scene::add_material(material_description_t ma)
{
    _material_t material = {};

    // TODO

    _materials[ma.id] = material;

    return true;
}



#if 0

bool Renderer::InitUniformBuffer()
{
    VkResult result;

    float aspect_ratio = _global_viewport.width / (float)_global_viewport.height;
    float fov_degrees = 45.0f;
    float nearZ = 0.1f;
    float farZ = 1000.0f;

    glm::mat4 proj = glm::perspective(fov_degrees, aspect_ratio, nearZ, farZ);
    proj[1][1] *= -1.0f; // vulkan clip space with Y down.
    glm::mat4 view = glm::lookAt(glm::vec3(-3, -2, 10), glm::vec3(0), glm::vec3(0, 1, 0));
    glm::mat4 model = glm::mat4(1);

    memcpy(_mvp.m, glm::value_ptr(model), sizeof(model));
    memcpy(_mvp.v, glm::value_ptr(view), sizeof(view));
    memcpy(_mvp.p, glm::value_ptr(proj), sizeof(proj));

    Log("#   Create Matrices Uniform Buffer\n");
    VkBufferCreateInfo uniform_buffer_create_info = {};
    uniform_buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    uniform_buffer_create_info.size = sizeof(_mvp); // size in bytes
    uniform_buffer_create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;   // <-- UBO
    uniform_buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    result = vkCreateBuffer(_ctx.device, &uniform_buffer_create_info, nullptr, &_matrices_ubo.buffer);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    Log("#   Get Uniform Buffer Memory Requirements(size and type)\n");
    VkMemoryRequirements uniform_buffer_memory_requirements = {};
    vkGetBufferMemoryRequirements(_ctx.device, _matrices_ubo.buffer, &uniform_buffer_memory_requirements);

    VkMemoryAllocateInfo uniform_buffer_allocate_info = {};
    uniform_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    uniform_buffer_allocate_info.allocationSize = uniform_buffer_memory_requirements.size;

    uint32_t uniform_memory_type_bits = uniform_buffer_memory_requirements.memoryTypeBits;
    VkMemoryPropertyFlags uniform_desired_memory_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    auto mem_props = _ctx.physical_device_memory_properties;
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

    Log("#   Allocate Uniform Buffer Memory\n");
    result = vkAllocateMemory(_ctx.device, &uniform_buffer_allocate_info, nullptr, &_matrices_ubo.memory);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    Log("#   Bind memory to buffer\n");
    result = vkBindBufferMemory(_ctx.device, _matrices_ubo.buffer, _matrices_ubo.memory, 0);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    Log("#   Map Uniform Buffer\n");
    void *mapped = nullptr;
    result = vkMapMemory(_ctx.device, _matrices_ubo.memory, 0, VK_WHOLE_SIZE, 0, &mapped);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    Log("#   Copy matrices, first time.\n");
    memcpy(mapped, _mvp.m, sizeof(_mvp.m));
    memcpy(((float *)mapped + 16), _mvp.v, sizeof(_mvp.v));
    memcpy(((float *)mapped + 32), _mvp.p, sizeof(_mvp.p));

    Log("#   UnMap Uniform Buffer\n");
    vkUnmapMemory(_ctx.device, _matrices_ubo.memory);

    return true;
}


bool Renderer::InitDescriptors()
{



    // "PER-OBJECT" at render time
    // Bind the UBO for this object, the texture of that object...


    // When a set is allocated all values are undefined and all 
    // descriptors are uninitialised. must init all statically used bindings:
    VkDescriptorBufferInfo descriptor_uniform_buffer_info = {};
    descriptor_uniform_buffer_info.buffer = _matrices_ubo.buffer;
    descriptor_uniform_buffer_info.offset = 0;
    descriptor_uniform_buffer_info.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet write_descriptor_for_uniform_buffer = {};
    write_descriptor_for_uniform_buffer.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_for_uniform_buffer.dstSet = _descriptor_set;
    write_descriptor_for_uniform_buffer.dstBinding = 0;
    write_descriptor_for_uniform_buffer.dstArrayElement = 0;
    write_descriptor_for_uniform_buffer.descriptorCount = 1;
    write_descriptor_for_uniform_buffer.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write_descriptor_for_uniform_buffer.pImageInfo = nullptr;
    write_descriptor_for_uniform_buffer.pBufferInfo = &descriptor_uniform_buffer_info;
    write_descriptor_for_uniform_buffer.pTexelBufferView = nullptr;

    Log("#   Update Descriptor Set (UBO)\n");
    vkUpdateDescriptorSets(_ctx.device, 1, &write_descriptor_for_uniform_buffer, 0, nullptr);

    VkDescriptorImageInfo descriptor_image_info = {};
    descriptor_image_info.sampler = _checker_texture.sampler;
    descriptor_image_info.imageView = _checker_texture.texture_view;
    descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_PREINITIALIZED; // why ? we did change the layout manually!!
                                                                        // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL

    VkWriteDescriptorSet write_descriptor_for_checker_texture_sampler = {};
    write_descriptor_for_checker_texture_sampler.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_for_checker_texture_sampler.dstSet = _descriptor_set;
    write_descriptor_for_checker_texture_sampler.dstBinding = 1; // <----- beware
    write_descriptor_for_checker_texture_sampler.dstArrayElement = 0;
    write_descriptor_for_checker_texture_sampler.descriptorCount = 1;
    write_descriptor_for_checker_texture_sampler.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write_descriptor_for_checker_texture_sampler.pImageInfo = &descriptor_image_info;
    write_descriptor_for_checker_texture_sampler.pBufferInfo = nullptr;
    write_descriptor_for_checker_texture_sampler.pTexelBufferView = nullptr;

    Log("#   Update Descriptor Set (texture sampler)\n");
    vkUpdateDescriptorSets(_ctx.device, 1, &write_descriptor_for_checker_texture_sampler, 0, nullptr);

    return true;
}

void Renderer::DeInitDescriptors()
{
    Log("#   Destroy Descriptor Pool\n");
    vkDestroyDescriptorPool(_ctx.device, _descriptor_pool, nullptr);

    Log("#   Destroy Descriptor Set Layout\n");
    vkDestroyDescriptorSetLayout(_ctx.device, _descriptor_set_layout, nullptr);
}

bool Scene::create_shader_module(const std::string &file_path, VkShaderModule *shader_module)
{
#if 0
    uint32_t codeSize;
    char *code = new char[10000];
    HANDLE fileHandle = nullptr;

    // load our vertex shader:
    fileHandle = ::CreateFile(file_path.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (fileHandle == INVALID_HANDLE_VALUE)
    {
        Log("!!!!!!!!! Failed to open shader file.\n");
        return false;
    }
    ::ReadFile((HANDLE)fileHandle, code, 10000, (LPDWORD)&codeSize, nullptr);
    ::CloseHandle(fileHandle);

    VkShaderModuleCreateInfo shader_creation_info = {};
    shader_creation_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_creation_info.codeSize = codeSize;
    shader_creation_info.pCode = (uint32_t *)code;
#else

    auto content = ReadFileContent(file_path);

    VkShaderModuleCreateInfo shader_creation_info = {};
    shader_creation_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_creation_info.codeSize = content.size();
    shader_creation_info.pCode = (uint32_t *)content.data();

#endif

    VkResult result;

    result = vkCreateShaderModule(_ctx.device, &shader_creation_info, nullptr, shader_module);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    return true;
}

bool Renderer::InitShaders()
{
    if (!CreateShaderModule("./vert.spv", &_vertex_shader_module))
        return false;

    if (!CreateShaderModule("./frag.spv", &_fragment_shader_module))
        return false;

    return true;
}

void Renderer::DeInitShaders()
{
    vkDestroyShaderModule(_ctx.device, _fragment_shader_module, nullptr);
    vkDestroyShaderModule(_ctx.device, _vertex_shader_module, nullptr);
}

bool Renderer::InitGraphicsPipeline()
{
    VkResult result;

    // use it later to define uniform buffer
    VkPipelineLayoutCreateInfo layout_create_info = {};
    layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_create_info.setLayoutCount = 1;
    layout_create_info.pSetLayouts = &_descriptor_set_layout;
    layout_create_info.pushConstantRangeCount = 0;
    layout_create_info.pPushConstantRanges = nullptr; // constant into shader for opti???

    Log("#   Fill Pipeline Layout... and everything else.\n");
    result = vkCreatePipelineLayout(_ctx.device, &layout_create_info, nullptr, &_pipeline_layout);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    std::array<VkPipelineShaderStageCreateInfo, 2> shader_stage_create_infos = {};
    shader_stage_create_infos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage_create_infos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shader_stage_create_infos[0].module = _vertex_shader_module;
    shader_stage_create_infos[0].pName = "main";        // shader entry point function name
    shader_stage_create_infos[0].pSpecializationInfo = nullptr;

    shader_stage_create_infos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage_create_infos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shader_stage_create_infos[1].module = _fragment_shader_module;
    shader_stage_create_infos[1].pName = "main";        // shader entry point function name
    shader_stage_create_infos[1].pSpecializationInfo = nullptr;

    VkVertexInputBindingDescription vertex_binding_description = {};
    vertex_binding_description.binding = 0;
    vertex_binding_description.stride = sizeof(Scene::vertex_t);
    vertex_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    // bind input to location=0, binding=0
    VkVertexInputAttributeDescription vertex_attribute_description[3] = {};
    vertex_attribute_description[0].location = 0;
    vertex_attribute_description[0].binding = 0;
    vertex_attribute_description[0].format = VK_FORMAT_R32G32B32A32_SFLOAT; // position = 4 float
    vertex_attribute_description[0].offset = 0;

    vertex_attribute_description[1].location = 1;
    vertex_attribute_description[1].binding = 0;
    vertex_attribute_description[1].format = VK_FORMAT_R32G32B32_SFLOAT; // normal = 3 floats
    vertex_attribute_description[1].offset = 4 * sizeof(float);

    vertex_attribute_description[2].location = 2;
    vertex_attribute_description[2].binding = 0;
    vertex_attribute_description[2].format = VK_FORMAT_R32G32_SFLOAT; // uv = 2 floats
    vertex_attribute_description[2].offset = (4 + 3) * sizeof(float);

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
    viewport.width = (float)_global_viewport.width;
    viewport.height = (float)_global_viewport.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissors = {};
    scissors.offset = { 0, 0 };
    scissors.extent = _global_viewport;

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
    raster_state_create_info.polygonMode = VK_POLYGON_MODE_LINE;// VK_POLYGON_MODE_FILL;
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

    std::array<VkGraphicsPipelineCreateInfo, 1> pipeline_create_infos = {};
    pipeline_create_infos[0].sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_create_infos[0].stageCount = (uint32_t)shader_stage_create_infos.size();
    pipeline_create_infos[0].pStages = shader_stage_create_infos.data();
    pipeline_create_infos[0].pVertexInputState = &vertex_input_state_create_info;
    pipeline_create_infos[0].pInputAssemblyState = &input_assembly_state_create_info;
    pipeline_create_infos[0].pTessellationState = nullptr;
    pipeline_create_infos[0].pViewportState = &viewport_state_create_info;
    pipeline_create_infos[0].pRasterizationState = &raster_state_create_info;
    pipeline_create_infos[0].pMultisampleState = &multisample_state_create_info;
    pipeline_create_infos[0].pDepthStencilState = &depth_stencil_state_create_info;
    pipeline_create_infos[0].pColorBlendState = &color_blend_state_create_info;
    pipeline_create_infos[0].pDynamicState = &dynamic_state_create_info;
    pipeline_create_infos[0].layout = _pipeline_layout;
    pipeline_create_infos[0].renderPass = _render_pass;
    pipeline_create_infos[0].subpass = 0;
    pipeline_create_infos[0].basePipelineHandle = VK_NULL_HANDLE; // only if VK_PIPELINE_CREATE_DERIVATIVE flag is set.
    pipeline_create_infos[0].basePipelineIndex = 0;

    Log("#   Create Pipeline\n");
    result = vkCreateGraphicsPipelines(
        _ctx.device,
        VK_NULL_HANDLE, // cache
        (uint32_t)pipeline_create_infos.size(),
        pipeline_create_infos.data(),
        nullptr,
        _pipelines.data());
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    return true;
}

void Renderer::DeInitGraphicsPipeline()
{
    for (auto & pipeline : _pipelines)
    {
        vkDestroyPipeline(_ctx.device, pipeline, nullptr);
    }
    vkDestroyPipelineLayout(_ctx.device, _pipeline_layout, nullptr);
}


void Renderer::update_matrices_ubo()
{
    void *matrix_mapped;
    vkMapMemory(_ctx.device, _matrices_ubo.memory, 0, VK_WHOLE_SIZE, 0, &matrix_mapped);

    memcpy(matrix_mapped, _mvp.m, sizeof(_mvp.m));
    memcpy(((float *)matrix_mapped + 16), _mvp.v, sizeof(_mvp.v));
    memcpy(((float *)matrix_mapped + 32), _mvp.p, sizeof(_mvp.p));

    /*
    memcpy(matrix_mapped + offsetof(_mvp, m), _mvp.m, sizeof(_mvp.m));
    memcpy(matrix_mapped + offsetof(_mvp, v), _mvp.v, sizeof(_mvp.v));
    memcpy(matrix_mapped + offsetof(_mvp, p), _mvp.p, sizeof(_mvp.p));
    */

    VkMappedMemoryRange memory_range = {};
    memory_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    memory_range.memory = _matrices_ubo.memory;
    memory_range.offset = 0;
    memory_range.size = VK_WHOLE_SIZE;
    vkFlushMappedMemoryRanges(_ctx.device, 1, &memory_range);

    vkUnmapMemory(_ctx.device, _matrices_ubo.memory);
}

#endif








//
