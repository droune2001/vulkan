#include "build_options.h"
#include "platform.h"
#include "scene.h"
#include "Renderer.h"
#include "Shared.h"
#include "utils.h"
#include "initializers.h"

#include "imgui.h"
#include "imgui_impl_vulkan.h"

#include <array>
#include <string>

#define MAX_NB_OBJECTS 1024
#define USE_STAGING_FOR_INSTANCING 1

//
// VERTEX
//
uint32_t Scene::vertex_t::binding_description_count() { return 1; }
VkVertexInputBindingDescription *Scene::vertex_t::binding_descriptions()
{
    static VkVertexInputBindingDescription vertex_binding_description = {};
    vertex_binding_description.binding = 0;
    vertex_binding_description.stride = sizeof(Scene::vertex_t);
    vertex_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return &vertex_binding_description;
}

uint32_t Scene::vertex_t::attribute_description_count() { return 3; }
VkVertexInputAttributeDescription *Scene::vertex_t::attribute_descriptions()
{
    static std::array<VkVertexInputAttributeDescription, 3> vertex_attribute_description = {};
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

    return vertex_attribute_description.data();
}

//
// INSTANCE DATA
//
uint32_t Scene::instance_data_t::binding_description_count() { return 2; }
VkVertexInputBindingDescription *Scene::instance_data_t::binding_descriptions()
{
    static std::array<VkVertexInputBindingDescription, 2> vertex_binding_descriptions = {};
    vertex_binding_descriptions[0].binding = 0;
    vertex_binding_descriptions[0].stride = sizeof(Scene::vertex_t);
    vertex_binding_descriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    vertex_binding_descriptions[1].binding = 1;
    vertex_binding_descriptions[1].stride = sizeof(Scene::instance_data_t);
    vertex_binding_descriptions[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

    return vertex_binding_descriptions.data();
}

uint32_t Scene::instance_data_t::attribute_description_count() { return 3 + 7; }
VkVertexInputAttributeDescription *Scene::instance_data_t::attribute_descriptions()
{
    static std::array<VkVertexInputAttributeDescription, 3 + 7> vertex_attribute_descriptions = {};
    
    //
    // vertex_t
    //
    vertex_attribute_descriptions[0].location = 0;
    vertex_attribute_descriptions[0].binding = 0;
    vertex_attribute_descriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT; // position = 4 float
    vertex_attribute_descriptions[0].offset = offsetof(Scene::vertex_t, p);

    vertex_attribute_descriptions[1].location = 1;
    vertex_attribute_descriptions[1].binding = 0;
    vertex_attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // normal = 3 floats
    vertex_attribute_descriptions[1].offset = offsetof(Scene::vertex_t, n);

    vertex_attribute_descriptions[2].location = 2;
    vertex_attribute_descriptions[2].binding = 0;
    vertex_attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT; // uv = 2 floats
    vertex_attribute_descriptions[2].offset = offsetof(Scene::vertex_t, uv);

    //
    // instance_t
    //

    vertex_attribute_descriptions[3].location = 3;
    vertex_attribute_descriptions[3].binding = 1;
    vertex_attribute_descriptions[3].format = VK_FORMAT_R32G32B32A32_SFLOAT; // position = 4 float
    vertex_attribute_descriptions[3].offset = offsetof(Scene::instance_data_t, position);

    vertex_attribute_descriptions[4].location = 4;
    vertex_attribute_descriptions[4].binding = 1;
    vertex_attribute_descriptions[4].format = VK_FORMAT_R32G32B32A32_SFLOAT; // rotation = 4 float
    vertex_attribute_descriptions[4].offset = offsetof(Scene::instance_data_t, rotation);

    vertex_attribute_descriptions[5].location = 5;
    vertex_attribute_descriptions[5].binding = 1;
    vertex_attribute_descriptions[5].format = VK_FORMAT_R32G32B32A32_SFLOAT; // scale = 4 float
    vertex_attribute_descriptions[5].offset = offsetof(Scene::instance_data_t, scale);

    vertex_attribute_descriptions[6].location = 6;
    vertex_attribute_descriptions[6].binding = 1;
    vertex_attribute_descriptions[6].format = VK_FORMAT_R32G32B32A32_SFLOAT; // speed = 4 float
    vertex_attribute_descriptions[6].offset = offsetof(Scene::instance_data_t, speed);

    vertex_attribute_descriptions[7].location = 7;
    vertex_attribute_descriptions[7].binding = 1;
    vertex_attribute_descriptions[7].format = VK_FORMAT_R32G32B32A32_SFLOAT; // jitter = 4 floats
    vertex_attribute_descriptions[7].offset = offsetof(Scene::instance_data_t, jitter);

    vertex_attribute_descriptions[8].location = 8;
    vertex_attribute_descriptions[8].binding = 1;
    vertex_attribute_descriptions[8].format = VK_FORMAT_R32G32B32A32_SFLOAT; // base color = 4 floats
    vertex_attribute_descriptions[8].offset = offsetof(Scene::instance_data_t, base);

    vertex_attribute_descriptions[9].location = 9;
    vertex_attribute_descriptions[9].binding = 1;
    vertex_attribute_descriptions[9].format = VK_FORMAT_R32G32B32A32_SFLOAT; // specular = 4 floats
    vertex_attribute_descriptions[9].offset = offsetof(Scene::instance_data_t, spec);

    return vertex_attribute_descriptions.data();
}

//
// SCENE
//

Scene::Scene(vulkan_context *c) : _ctx(c)
{
    _view_t v;
    v.camera = "perspective";
    v.descriptor_set = VK_NULL_HANDLE;
    _views["perspective"] = v;
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

    Log("#    Upload ImGui Font\n");
    {
        auto cmd = begin_single_time_commands(_ctx->graphics);
        ImGui_ImplVulkan_CreateFontsTexture(cmd);
        end_single_time_commands(cmd, _ctx->graphics);
        ImGui_ImplVulkan_InvalidateFontUploadObjects();
    }

    Log("#    Create Procedural Textures\n");
    if (!create_procedural_textures())
        return false;

    Log("#    Create Texture Samplers\n");
    if (!create_texture_samplers())
        return false;

    Log("#    Create All Descriptor Set Layouts\n");
    if (!create_all_descriptor_set_layouts(_ctx->device, _descriptor_set_layouts.data()))
        return false;

    Log("#    Build All Shaders/Pipelines\n");
    if (!build_pipelines(rp))
        return false;

    return true;
}

void Scene::de_init()
{
    vkQueueWaitIdle(_ctx->graphics.queue);

    Log("#   Destroy Pipelines\n");
    destroy_pipelines();

    Log("#   Destroy Procedural Textures\n");
    destroy_textures();

    Log("#   Destroy Uniform Buffers\n");
    if (_global_object_vbo_created
        || _global_object_ibo_created
        || _global_object_matrices_ubo_created
        || _global_object_material_ubo_created)
    {
        destroy_global_object_buffers();
    }
    if (_scene_ubo_created) destroy_scene_ubo();
}

uint32_t Scene::_add_object(const object_description_t &desc )
{
    Log("#   Add Object\n");

    VkResult result;
    VkDevice device = _ctx->device;

    _object_t obj = {};

    // with lazy init
    auto &global_vbo = get_global_object_vbo();
    auto &global_ibo = get_global_object_ibo();
    auto &global_matrices_ubo = get_global_object_matrices_ubo();
    auto &global_material_ubo = get_global_object_material_ubo();
    auto &staging_buffer = get_global_staging_vbo();

    obj.position = desc.position;
    obj.vertexCount = desc.vertexCount;
    obj.vertex_buffer = global_vbo.buffer;
    obj.vertex_offset = global_vbo.offset;
    obj.indexCount = desc.indexCount;
    obj.index_buffer = global_ibo.buffer;
    obj.index_offset = global_ibo.offset;
    obj.material_ref = desc.material;
    obj.base_color = desc.base_color;
    obj.specular = desc.specular;

    Log(std::string("#    v: ") + std::to_string(desc.vertexCount) + std::string(" i: ") + std::to_string(desc.indexCount) + "\n");

    void *mapped = nullptr;

    {
        Log("#    Map (Staging) Vertex Buffer\n");
        size_t vertex_data_size = desc.vertexCount * sizeof(vertex_t);

        Log("#     offset: " + std::to_string(global_vbo.offset) + std::string(" size: ") + std::to_string(vertex_data_size) + "\n");
        result = vkMapMemory(device, staging_buffer.memory, 0, vertex_data_size, 0, &mapped);
        ErrorCheck(result);
        if (result != VK_SUCCESS)
            return false;

        vertex_t *vertices = (vertex_t*)mapped;
        memcpy(vertices, desc.vertices, vertex_data_size);

        Log("#    UnMap Vertex Buffer\n");
        vkUnmapMemory(device, staging_buffer.memory);

        copy_buffer_to_buffer(_global_staging_vbo.buffer, _global_object_vbo.buffer, vertex_data_size, 0, global_vbo.offset);

        global_vbo.offset += (uint32_t)vertex_data_size;
    }

    {
        Log("#    Map Index Buffer\n");
        size_t index_data_size = desc.indexCount * sizeof(index_t);

        Log("#     offset: " + std::to_string(global_ibo.offset) + std::string(" size: ") + std::to_string(index_data_size) + "\n");
        result = vkMapMemory(device, staging_buffer.memory, 0, index_data_size, 0, &mapped);
        ErrorCheck(result);
        if (result != VK_SUCCESS)
            return false;

        index_t *indices = (index_t*)mapped;
        memcpy(indices, desc.indices, index_data_size);

        Log("#    UnMap Index Buffer\n");
        vkUnmapMemory(device, staging_buffer.memory);

        copy_buffer_to_buffer(_global_staging_vbo.buffer, _global_object_ibo.buffer, index_data_size, 0, global_ibo.offset);

        global_ibo.offset += (uint32_t)index_data_size;
    }

    Log("#    Compute ModelMatrix and put it in the aligned buffer\n");
    glm::mat4* model_mat = (glm::mat4*)((uint64_t)global_matrices_ubo.host_data + (_objects.size() * global_matrices_ubo.alignment));
    *model_mat = glm::translate(glm::mat4(1), desc.position);

    // TODO: add normal matrix as a mat3 -> Matrices.uNormalMatrix = glm::inverseTranspose( glm::mat3( Matrices.uModelMatrix ) );

    Log("#    Fill Material Overrides into its aligned buffer\n");
    _material_override_t *materials = (_material_override_t*)((uint64_t)global_material_ubo.host_data + (_objects.size() * global_material_ubo.alignment));
    materials->base_color = desc.base_color;
    materials->specular = desc.specular;

    uint32_t index = (uint32_t)_objects.size();

    _objects.push_back(obj);

    return index;
}

bool Scene::add_object_to_global_instance_set(object_description_t desc)
{
    uint32_t index = _add_object(desc);

    _object_names.push_back(desc.name);
    _global_instance_set.push_back(index);

    return true;
}

bool Scene::add_instance_set(instance_set_description_t isd, uint32_t estimated_instance_count)
{
    auto &is = _instance_sets[isd.instance_set];
    is.model_index = _add_object(isd.object_desc);
    is.material_ref = isd.object_desc.material;

    if (estimated_instance_count > 0)
    {
        // NOTE(nfauvet): resize et pas reserve parce que je m'en sers comme un tableau tab[i], pas push_back
        is.instance_data.resize(MAX_INSTANCE_COUNT);
    }

    return true;
}

uint32_t Scene::add_object_to_instance_set(instanced_object_description_t o, instance_set_id_t id)
{
    auto &is = _instance_sets[id];
    uint32_t idx = is.instance_count;
    instance_data_t &data = is.instance_data[idx];
    data.position = glm::vec4(o.position,1);
    data.rotation = glm::vec4(o.rotation,0);
    data.scale = glm::vec4(o.scale,0);
    data.speed = glm::vec4(0,0,0,0);
    data.jitter = o.jitters;
    data.base = o.base_color;
    data.spec = o.specular;

    return is.instance_count++;
}

bool Scene::add_light(light_description_t li)
{
    _light_t &light = _lighting_block.lights[_lights.size()];

    light.position = glm::vec4(li.position, 1);
    light.color = glm::vec4(li.color, 1);
    light.direction = glm::vec4(li.direction, li.type == light_description_t::CONE_LIGHT_TYPE ? 1 : 0);
    light.properties = glm::vec4(li.radius, li.intensity, li.inner, li.outer);
    
    _lights.push_back(light);

    return true;
}

bool Scene::add_camera(camera_description_t ca)
{
    _camera_t camera = {};
    camera.v = glm::lookAt(ca.position, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    camera.p = glm::perspective(ca.fovy, ca.aspect, ca.near_plane, ca.far_plane);
    camera.p[1][1] *= -1.0f;

    _cameras[ca.camera_id] = camera;

    return true;
}

void Scene::update(float dt)
{
    show_property_sheet();

    if (_animate_object)
        animate_object(dt);

    //if (_animate_camera)
        animate_camera(dt);

    if (_animate_light)
        animate_light(dt);
}

void Scene::upload()
{
    update_scene_ubo();
    update_all_objects_ubos();
    update_all_instances_vbos();
}

void Scene::record_compute_commands(VkCommandBuffer cmd)
{
    VkResult result;

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    result = vkBeginCommandBuffer(cmd, &begin_info);
    ErrorCheck(result);
    {
        // TODO: for each instance set
        auto &is = _instance_sets["metal_spheres"];

        VkBufferMemoryBarrier storage_buffer_memory_barrier_before = {};
        storage_buffer_memory_barrier_before.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        storage_buffer_memory_barrier_before.srcAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
        storage_buffer_memory_barrier_before.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        storage_buffer_memory_barrier_before.buffer = is.instance_buffer.buffer; // <====== hardcoded 0
        storage_buffer_memory_barrier_before.size = VK_WHOLE_SIZE;
        storage_buffer_memory_barrier_before.srcQueueFamilyIndex = _ctx->graphics.family_index;
        storage_buffer_memory_barrier_before.dstQueueFamilyIndex = _ctx->compute.family_index;

        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            0,
            0, nullptr,
            1, &storage_buffer_memory_barrier_before,
            0, nullptr);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, compute_particles.pipe.pipeline);

        // bind storage buffer and uniform buffer
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, compute_particles.pipe.pipeline_layout,
            0, // bind to set #0
            1, &compute_particles.descriptor_set, 0, nullptr);

        vkCmdDispatch(cmd, 1 + _nb_instances / 256, 1, 1);

        VkBufferMemoryBarrier storage_buffer_memory_barrier_after = {};
        storage_buffer_memory_barrier_after.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        storage_buffer_memory_barrier_after.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        storage_buffer_memory_barrier_after.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
        storage_buffer_memory_barrier_after.buffer = is.instance_buffer.buffer;
        storage_buffer_memory_barrier_after.size = VK_WHOLE_SIZE;
        storage_buffer_memory_barrier_after.srcQueueFamilyIndex = _ctx->compute.family_index;
        storage_buffer_memory_barrier_after.dstQueueFamilyIndex = _ctx->graphics.family_index;

        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
            0,
            0, nullptr,
            1, &storage_buffer_memory_barrier_after,
            0, nullptr);
    }
    result = vkEndCommandBuffer(cmd); // compiles the command buffer
    ErrorCheck(result);
}

void Scene::draw(VkCommandBuffer cmd, VkViewport viewport, VkRect2D scissor_rect)
{
#define DRAW_GLOBAL_INSTANCES 0
#define DRAW_INSTANCED_INSTANCES 1

    // RENDER PASS BEGIN ---

    auto default_pipeline = _pipelines["default"];
    auto default_view = _views["perspective"];

    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor_rect);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, default_pipeline.pipeline);

#if DRAW_GLOBAL_INSTANCES == 1

    //
    // SET 0
    // scene/view bindings, one time
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, default_pipeline.pipeline_layout,
        0, // bind to set #0
        1, &default_view.descriptor_set, 0, nullptr);

    for (const auto &m : _material_instances)
    {
        //
        // SET 1
        //
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, default_pipeline.pipeline_layout,
            1, 1, &m.second.descriptor_set, 0, nullptr);

        // TODO: loop in objects per material instances
        for (auto i : _global_instance_set)
        {
            const _object_t &obj = _objects[i];
            if (obj.material_ref != m.first)
                continue;

            // Bind Attribs Vertex/Index
            VkDeviceSize offsets = obj.vertex_offset;
            vkCmdBindVertexBuffers(cmd, 0, 1, &obj.vertex_buffer, &offsets);
            vkCmdBindIndexBuffer(cmd, obj.index_buffer, obj.index_offset, VK_INDEX_TYPE_UINT16);

            // ith object offset into dynamic ubo
            std::array<uint32_t, 2> dynamic_offsets = {
                static_cast<uint32_t>(i * _global_object_matrices_ubo.alignment),
                static_cast<uint32_t>(i * _global_object_material_ubo.alignment),
            };

            //
            // SET 2
            // Bind Per-Object Uniforms
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, default_pipeline.pipeline_layout,
                2, 1, &_global_objects_descriptor_set, //obj.descriptor_set,
                (uint32_t)dynamic_offsets.size(), dynamic_offsets.data()); // dynamic offsets

                                                                           // TODO: draw instanced for... instances.
            vkCmdDrawIndexed(cmd, obj.indexCount, 1, 0, 0, 0);
        }
    }
#endif

#if DRAW_INSTANCED_INSTANCES == 1
    //
    // Instanced Sets
    //
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _instance_pipe.pipeline);

    //
    // SET 0
    // scene/view bindings, one time
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _instance_pipe.pipeline_layout,
        0, // bind to set #0
        1, &default_view.descriptor_set, 0, nullptr);

    for (const auto &_is : _instance_sets)
    {
        const auto &is = _is.second;
        //const auto &is = _instance_sets["plastic_cubes"];
        //
        // SET 1
        //
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _instance_pipe.pipeline_layout,
            1, 1, &_material_instances[is.material_ref].descriptor_set , 0, nullptr);

        const auto &obj = _objects[is.model_index];

        // Bind Attribs Vertex/Index
        VkDeviceSize vertex_offsets = obj.vertex_offset;
        vkCmdBindVertexBuffers(cmd, 0, 1, &obj.vertex_buffer, &vertex_offsets); // bind point 0, per-vertex data
        VkDeviceSize instance_offsets = 0;
        vkCmdBindVertexBuffers(cmd, 1, 1, &is.instance_buffer.buffer, &instance_offsets); // bind point 1, per-instance data
        vkCmdBindIndexBuffer(cmd, obj.index_buffer, obj.index_offset, VK_INDEX_TYPE_UINT16);

        uint32_t instance_count = std::min(is.instance_count, (uint32_t)_nb_instances);
        vkCmdDrawIndexed(cmd, obj.indexCount, instance_count, 0, 0, 0);
    }
#endif
    // RENDER PASS END ---
}

// =====================================================

uint32_t Scene::find_memory_type(uint32_t memory_type_bits, VkMemoryPropertyFlags desired_memory_flags)
{
    auto mem_props = _ctx->physical_device_memory_properties;
    for (uint32_t i = 0; i < mem_props.memoryTypeCount; ++i)
    {
        if (memory_type_bits & 1)
        {
            if ((mem_props.memoryTypes[i].propertyFlags & desired_memory_flags) == desired_memory_flags)
            {
                return i;
            }
        }
        memory_type_bits = memory_type_bits >> 1;
    }

    return 0; // assert?
}

bool Scene::create_buffer(
    VkBuffer *pBuffer,                          // [out]
    VkDeviceMemory *pBufferMemory,              // [out]
    VkDeviceSize size,                          // [in]
    VkBufferUsageFlags usage_flags,             // [in]
    VkMemoryPropertyFlags memory_property_flags // [in]
)
{
    VkResult result;

    VkBufferCreateInfo buffer_create_info = {};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = size;
    buffer_create_info.usage = usage_flags;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    Log("#      Create Buffer\n");
    result = vkCreateBuffer(_ctx->device, &buffer_create_info, nullptr, pBuffer);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    VkMemoryRequirements buffer_memory_requirements = {};
    vkGetBufferMemoryRequirements(_ctx->device, *pBuffer, &buffer_memory_requirements);

    VkMemoryAllocateInfo memory_allocate_info = {};
    memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_allocate_info.allocationSize = buffer_memory_requirements.size;
    memory_allocate_info.memoryTypeIndex = find_memory_type(buffer_memory_requirements.memoryTypeBits, memory_property_flags);

    Log("#      Allocate Buffer Memory\n");
    result = vkAllocateMemory(_ctx->device, &memory_allocate_info, nullptr, pBufferMemory);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    Log("#      Bind Buffer Memory\n");
    result = vkBindBufferMemory(_ctx->device, *pBuffer, *pBufferMemory, 0);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    return true;
}

VkCommandBuffer Scene::begin_single_time_commands(const vulkan_queue &queue)
{
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    // TODO: or create a temp command buffer
    auto &cmd = queue.command_buffers[0];

    vkBeginCommandBuffer(cmd, &begin_info);

    return cmd;
}

void Scene::end_single_time_commands(VkCommandBuffer cmd, const vulkan_queue &queue)
{
    VkResult result;

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd;
    result = vkQueueSubmit(queue.queue, 1, &submit_info, VK_NULL_HANDLE);
    ErrorCheck(result);
    result = vkQueueWaitIdle(queue.queue);
    ErrorCheck(result);

    vkResetCommandBuffer(cmd, 0);

    // opt free temp command buffer.
}

bool Scene::copy_buffer_to_image(VkBuffer src, VkImage dst, VkExtent3D extent)
{
    auto cmd = begin_single_time_commands(_ctx->transfer);
    {
        VkBufferImageCopy image_copy_region = vk::init::transfer::buffer_image_copy();
        image_copy_region.imageExtent = extent;

        vkCmdCopyBufferToImage(cmd, src, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_copy_region);
    }
    end_single_time_commands(cmd, _ctx->transfer);

    return true;
}

bool Scene::copy_buffer_to_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size, VkDeviceSize src_offset, VkDeviceSize dst_offset)
{
    auto cmd = begin_single_time_commands(_ctx->transfer);
    {
        VkBufferCopy copy_region = {};
        copy_region.srcOffset = src_offset;
        copy_region.dstOffset = dst_offset;
        copy_region.size = size;

        vkCmdCopyBuffer(cmd, src, dst, 1, &copy_region);
    }
    end_single_time_commands(cmd, _ctx->transfer);

    return true;
}

bool Scene::create_texture_2d(_texture_t *texture)
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
    texture_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    texture_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    texture_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // we will transfer the data from another buffer

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
    texture_image_allocate_info.memoryTypeIndex = find_memory_type(
        texture_memory_requirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT); // on the device

    Log("#     Allocate Memory\n");
    result = vkAllocateMemory(device, &texture_image_allocate_info, nullptr, &texture->image_memory);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    result = vkBindImageMemory(device, texture->image, texture->image_memory, 0);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    return true;
}

bool Scene::copy_data_to_staging_buffer(staging_buffer_t buffer, void *data, VkDeviceSize size, bool flush)
{
    VkResult result;
    auto device = _ctx->device;

    Log("#     Map/Fill/Flush/UnMap staging buffer.\n");
    void *image_mapped;
    result = vkMapMemory(device, buffer.memory, 0, size, 0, &image_mapped);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    memcpy(image_mapped, data, size);

    if (flush)
    {
        VkMappedMemoryRange memory_range = {};
        memory_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        memory_range.memory = buffer.memory;
        memory_range.offset = 0;
        memory_range.size = size;
        vkFlushMappedMemoryRanges(device, 1, &memory_range);
    }

    vkUnmapMemory(device, buffer.memory);

    return true;
}

bool Scene::transition_texture(VkImage *pImage, VkImageLayout old_layout, VkImageLayout new_layout)
{
    VkAccessFlags src_access_mask = VK_ACCESS_HOST_WRITE_BIT;
    VkAccessFlags dst_access_mask = VK_ACCESS_SHADER_READ_BIT;
    VkPipelineStageFlags src_stage_mask = VK_PIPELINE_STAGE_HOST_BIT;
    VkPipelineStageFlags dst_stage_mask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED
        && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        src_access_mask = 0;
        dst_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT;
        src_stage_mask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dst_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        src_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT;
        dst_access_mask = VK_ACCESS_SHADER_READ_BIT;
        src_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dst_stage_mask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        return false;
    }

    VkResult result;
    auto device = _ctx->device;

    VkFence submit_fence = {};
    VkFenceCreateInfo fence_create_info = {};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    vkCreateFence(device, &fence_create_info, nullptr, &submit_fence);

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    auto &cmd = _ctx->graphics.command_buffers[0];

    vkBeginCommandBuffer(cmd, &begin_info);

    VkImageMemoryBarrier layout_transition_barrier = {};
    layout_transition_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    layout_transition_barrier.srcAccessMask = src_access_mask;
    layout_transition_barrier.dstAccessMask = dst_access_mask;
    layout_transition_barrier.oldLayout = old_layout;
    layout_transition_barrier.newLayout = new_layout;
    layout_transition_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    layout_transition_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    layout_transition_barrier.image = *pImage;
    layout_transition_barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

    Log("#     Transition texture\n");
    vkCmdPipelineBarrier(cmd,
        src_stage_mask,
        dst_stage_mask,
        0,
        0, nullptr,
        0, nullptr,
        1,
        &layout_transition_barrier);

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

    auto &cmd = _ctx->graphics.command_buffers[0];

    vkBeginCommandBuffer(cmd, &begin_info);

    std::vector<VkImageMemoryBarrier> layout_transition_barriers = {};
    layout_transition_barriers.reserve(_textures.size());
    layout_transition_barriers.resize(_textures.size());
    size_t i = 0;
    for (auto &t : _textures)
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

// ===========================================================================

bool Scene::create_global_object_buffers()
{
    Log("#     Create Global Matrices Object\'s UBO\n");
    {
        // Find the memory alignment for the object matrices;
        dynamic_uniform_buffer_t &mtx_ubo = _global_object_matrices_ubo;
        size_t min_ubo_alignment = _ctx->physical_device_properties.limits.minUniformBufferOffsetAlignment;
        mtx_ubo.alignment = sizeof(glm::mat4);
        if (min_ubo_alignment > 0)
        {
            mtx_ubo.alignment = (mtx_ubo.alignment + min_ubo_alignment - 1) & ~(min_ubo_alignment - 1);
        }
        mtx_ubo.size = MAX_NB_OBJECTS * mtx_ubo.alignment;
        mtx_ubo.host_data = utils::aligned_alloc(mtx_ubo.size, mtx_ubo.alignment);
        for (size_t i = 0; i < MAX_NB_OBJECTS; ++i)
        {
            glm::mat4 *model_mat_for_obj_i = (glm::mat4*)((uint64_t)mtx_ubo.host_data + (i * mtx_ubo.alignment));
            *model_mat_for_obj_i = glm::mat4(1);
        }


        if (!create_buffer(
            &mtx_ubo.buffer,
            &mtx_ubo.memory,
            mtx_ubo.size,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
            return false;

        _global_object_matrices_ubo_created = true;
    }

    Log("#     Create Global Materials Object\'s UBO\n");
    {
        // Find the memory alignment for the object material overrides
        dynamic_uniform_buffer_t &mtl_ubo = _global_object_material_ubo;
        size_t min_ubo_alignment = _ctx->physical_device_properties.limits.minUniformBufferOffsetAlignment;
        mtl_ubo.alignment = sizeof(_material_override_t);
        if (min_ubo_alignment > 0)
        {
            mtl_ubo.alignment = (mtl_ubo.alignment + min_ubo_alignment - 1) & ~(min_ubo_alignment - 1);
        }
        mtl_ubo.size = MAX_NB_OBJECTS * mtl_ubo.alignment;
        mtl_ubo.host_data = utils::aligned_alloc(mtl_ubo.size, mtl_ubo.alignment);
        for (size_t i = 0; i < MAX_NB_OBJECTS; ++i)
        {
            _material_override_t *model_mat_for_obj_i = (_material_override_t*)((uint64_t)mtl_ubo.host_data + (i * mtl_ubo.alignment));
            model_mat_for_obj_i->base_color = glm::vec4(0.5, 0.5, 0.5, 1.0);
            model_mat_for_obj_i->specular = glm::vec4(1, 1, 0, 0); // roughness, metallic, 0, 0
        }


        if (!create_buffer(
            &mtl_ubo.buffer,
            &mtl_ubo.memory,
            mtl_ubo.size,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
            return false;

        _global_object_material_ubo_created = true;
    }

    // VBO
    Log("#     Create Global Object\'s VBO\n");
    if (!create_buffer(
        &_global_object_vbo.buffer,
        &_global_object_vbo.memory,
        4 * 1024 * 1024,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
        return false;

    _global_object_vbo_created = true;

    // IBO
    Log("#     Create Global Object\'s IBO\n");
    if (!create_buffer(
        &_global_object_ibo.buffer,
        &_global_object_ibo.memory,
        4 * 1024 * 1024,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
        return false;

    _global_object_ibo_created = true;

    Log("#     Create Staging Buffer for VBO/IBO\n");
    if (!create_buffer(
        &_global_staging_vbo.buffer,
        &_global_staging_vbo.memory,
        8 * 1024 * 1024,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
        return false;

    _global_staging_vbo_created = true;

    return true;
}

void Scene::destroy_global_object_buffers()
{
    Log("#    Free Global Object Buffers Memory\n");
    vkFreeMemory(_ctx->device, _global_object_matrices_ubo.memory, nullptr);
    vkFreeMemory(_ctx->device, _global_object_material_ubo.memory, nullptr);
    vkFreeMemory(_ctx->device, _global_object_vbo.memory, nullptr);
    vkFreeMemory(_ctx->device, _global_object_ibo.memory, nullptr);
    vkFreeMemory(_ctx->device, _global_staging_vbo.memory, nullptr);

    Log("#    Destroy Global Object Buffers\n");
    vkDestroyBuffer(_ctx->device, _global_object_matrices_ubo.buffer, nullptr);
    vkDestroyBuffer(_ctx->device, _global_object_material_ubo.buffer, nullptr);
    vkDestroyBuffer(_ctx->device, _global_object_vbo.buffer, nullptr);
    vkDestroyBuffer(_ctx->device, _global_object_ibo.buffer, nullptr);
    vkDestroyBuffer(_ctx->device, _global_staging_vbo.buffer, nullptr);

    _global_object_matrices_ubo_created = false;
    _global_object_material_ubo_created = false;
    _global_object_vbo_created = false;
    _global_object_ibo_created = false;
    _global_staging_vbo_created = false;
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

Scene::dynamic_uniform_buffer_t & Scene::get_global_object_matrices_ubo()
{
    if (!_global_object_matrices_ubo_created)
    {
        if (!create_global_object_buffers())
        {
            assert("could not create objects matrices dynamic ubo");
        }
    }

    return _global_object_matrices_ubo;
}

Scene::dynamic_uniform_buffer_t & Scene::get_global_object_material_ubo()
{
    if (!_global_object_material_ubo_created)
    {
        if (!create_global_object_buffers())
        {
            assert("could not create objects material dynamic ubo");
        }
    }

    return _global_object_material_ubo;
}

Scene::vertex_buffer_object_t & Scene::get_global_staging_vbo()
{
    if (!_global_staging_vbo_created)
    {
        if (!create_global_object_buffers())
        {
            assert("could not create staging buffer");
        }
    }

    return _global_staging_vbo;
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
    Log("#     Create Matrices Uniform Buffer\n");
    if (!create_buffer(
        &_scene_ubo.buffer,
        &_scene_ubo.memory,
        sizeof(_camera_t) + sizeof(_lighting_block), // one camera and max lights
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
        return false;

    _scene_ubo_created = true;

    Log("#     Create Simulation Uniform Buffer\n");
    if (!create_buffer(
        &compute_particles.ubo.buffer,
        &compute_particles.ubo.memory,
        sizeof(_compute_particles_data_t::_simulation_data_t),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
        return false;

    return true;
}

void Scene::destroy_scene_ubo()
{
    Log("#    Free Memory\n");
    vkFreeMemory(_ctx->device, _scene_ubo.memory, nullptr);

    Log("#    Destroy Buffer\n");
    vkDestroyBuffer(_ctx->device, _scene_ubo.buffer, nullptr);
}


void *Scene::get_aligned(dynamic_uniform_buffer_t *buffer, uint32_t idx)
{
    return (void*)((uint64_t)buffer->host_data + (idx * buffer->alignment));
}

void Scene::animate_object(float dt)
{
#if 0
    static float obj_x = 0.0f;
    static float obj_y = 0.0f;
    static float obj_z = 0.0f;
    static float accum = 0.0f; // in seconds

    accum += dt;

#if 0
    float speed = 3.0f; // radian/sec
    float radius = 1.5f;
    obj_x = radius * std::cos(speed * accum);
    obj_y = radius * std::sin(2.0f * speed * accum);
    obj_z = 0.5f * radius * std::cos(speed * accum);
#endif
    auto &obj_0 = _objects[0];
    auto &obj_1 = _objects[1];

    // Aligned offset
    glm::mat4* model_mat_obj_0 = (glm::mat4*)get_aligned(&_global_object_matrices_ubo, 0);
    glm::mat4* model_mat_obj_1 = (glm::mat4*)get_aligned(&_global_object_matrices_ubo, 1);

    *model_mat_obj_0 = glm::translate(glm::mat4(1), obj_0.position + glm::vec3(obj_x, obj_y, obj_z));
    *model_mat_obj_1 = glm::translate(glm::mat4(1), obj_1.position + glm::vec3(-obj_x, obj_y, -obj_z));
#endif

    if (!_animate_instance_data)
        return;

    static float t = 0.0f; // in seconds
    t += dt;

#if 0 // no more cpu sim, too much RAM used on laptop.
    if (_simulate_cpu)
    {
        //
        // animate instanced objects
        //
        constexpr float TWO_PI = 6.2831853071f;
        constexpr float SQRT_2 = 1.414213562f;
#if USE_INSTANCE_SET_1 == 1
        {
            auto &is = _instance_sets["plastic_cubes"];
            for (uint32_t i = 0; i < is.instance_count; ++i)
            {
                uint32_t row = i / ROWS_COUNT;
                uint32_t col = i % COLS_COUNT;
                float norm_start_x = (0.5f + (col - (ROWS_COUNT / 2.0f))) / ((ROWS_COUNT / 2.0f) - 0.5f);
                float norm_start_z = (0.5f + (row - (COLS_COUNT / 2.0f))) / ((COLS_COUNT / 2.0f) - 0.5f);

                float d2 = norm_start_x * norm_start_x + norm_start_z * norm_start_z;
                float d = sqrtf((float)d2 / SQRT_2); // normalized distance

                constexpr float speed = 3.0f; // radian/sec
                float radius = _instances_layout_radius;// 20.0f; // dependant de rows/cols_count
                constexpr float height = 1.0f;
                is.positions[i].x = radius * norm_start_x;// radius * d * std::cos(speed * accum);
                is.positions[i].y = 1.0f + d * height * std::cos(d2 + 3.0f * speed * accum);
                is.positions[i].z = radius * norm_start_z;// radius * d * std::sin(speed * accum);

                constexpr float rotation_speed = 30.0f;
                is.rotations[i].x = glm::radians(d * rotation_speed * accum);
                is.rotations[i].y = 0.0f;
                is.rotations[i].z = 0.0f;

                is.scales[i].x = 1.0f;
                is.scales[i].y = 1.0f;
                is.scales[i].z = 1.0f;
            }
        }
#endif
#if USE_INSTANCE_SET_2 == 1
        {
            auto &is = _instance_sets["metal_spheres"];
            uint32_t instance_count = std::min(is.instance_count, (uint32_t)_nb_instances);
            for (uint32_t i = 0; i < instance_count; ++i)
            {
                float tt = _speed * t + i * _pdt; // delay each successive particle
                float rt = _rotation_speed * t + i * _pdt; // delay each successive particle

                auto J = is.jitters[i];
                // J.xyz = = offset sur position de depart, decale toute la track.
                // _e0 = facteur de jitter sur la position de depart 
                // _e1 = facteur de jitter sur taille des petits cercles.
                // _e2 = facteur de jitter sur rotatin speed.

                float global_pos_offset_x = _e0 * (2.0f * J.x - 1.0f);
                float global_pos_offset_y = _e0 * (2.0f * J.y - 1.0f);
                float global_pos_offset_z = _e0 * (2.0f * J.z - 1.0f);

                float local_pos_offset_x = _e1 * (2.0f * J.x - 1.0f);
                float local_pos_offset_y = _e1 * (2.0f * J.y - 1.0f);
                float local_pos_offset_z = _e1 * (2.0f * J.z - 1.0f);

                is.positions[i].x = _ax * std::cos(_bx*tt) + (_cx + local_pos_offset_x) * std::sin(_dx*tt) + global_pos_offset_x;
                is.positions[i].y = _ay * std::sin(_by*tt) + (_cy + local_pos_offset_y) * std::cos(_dy*tt) + global_pos_offset_y;
                is.positions[i].z = _az * std::sin(_bz*tt) + (_cz + local_pos_offset_z) * std::cos(_dz*tt) + global_pos_offset_z;

                is.rotations[i].x = glm::radians(_e2*J.x * _rsx * TWO_PI * rt);
                is.rotations[i].y = glm::radians(_e2*J.y * _rsy * TWO_PI * rt);
                is.rotations[i].z = glm::radians(_e2*J.z * _rsz * TWO_PI * rt);

                is.scales[i].x = _psx;
                is.scales[i].y = _psy;
                is.scales[i].z = _psz;
            }
        }
#endif

        // fill instance buffer data
        for (auto &_is : _instance_sets)
        {
            auto &is = _is.second;
            uint32_t instance_count = std::min(is.instance_count, (uint32_t)_nb_instances);
            for (uint32_t i = 0; i < instance_count; ++i)
            {
                glm::mat4 m(1);
                m = glm::translate(m, is.positions[i].xyz());
                m = glm::rotate(m, is.rotations[i].x, glm::vec3(1, 0, 0));
                m = glm::rotate(m, is.rotations[i].y, glm::vec3(0, 1, 0));
                m = glm::rotate(m, is.rotations[i].z, glm::vec3(0, 0, 1));
                m = glm::scale(m, is.scales[i]);

                is.instance_data[i].m = m;
                is.instance_data[i].b = is.base_colors[i];
                is.instance_data[i].s = is.speculars[i];
            }
        }
    } // simulate cpu
#endif

    // fill uniform data for the simulation compute shader.
    {
        uint32_t instance_count = std::min(_instance_sets["metal_spheres"].instance_count, (uint32_t)_nb_instances);
        compute_particles.data.data0 = glm::vec4(t, _speed, _rotation_speed, _pdt);
        compute_particles.data.data1 = glm::vec4(_e0, _e1, _e2, _e3);
        compute_particles.data.data2 = glm::vec4(_ax, _bx, _cx, _dx);
        compute_particles.data.data3 = glm::vec4(_ay, _by, _cy, _dy);
        compute_particles.data.data4 = glm::vec4(_az, _bz, _cz, _dz);
        compute_particles.data.data5 = glm::vec4(_psx, _psy, _psz, 0);
        compute_particles.data.data6 = glm::vec4(_rsx, _rsy, _rsz, 0);
        compute_particles.data.data7 = glm::vec4(0,0,0,0);
        compute_particles.data.instance_count = instance_count;
    }
}

void Scene::animate_camera(float dt)
{
    static float accum_dt = 0.0f;

    auto &camera = _cameras["perspective"];

    const float cam_as = 0.3f; // angular_speed, radians/sec
    if (_animate_camera)
    {
        accum_dt += dt;
        camera.pos.x = _camera_distance * std::cos(cam_as * accum_dt);
        camera.pos.y = _camera_elevation;
        camera.pos.z = _camera_distance * std::sin(cam_as * accum_dt);
    }
    else
    {
        camera.pos.x = _camera_distance;
        camera.pos.y = _camera_elevation;
        camera.pos.z = _camera_distance;
    }
    camera.v = glm::lookAt(camera.pos.xyz(), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
}

void Scene::animate_light(float dt)
{
    static float accum_dt = 0.0f;
    accum_dt += dt;

    const float BASE_Y_OFFSET = 2.0f;

    auto &lights = _lighting_block.lights;

    {
        const float r_x = 10.0f; // radius
        const float r_y = 0.5f; // radius
        const float r_z = 3.0f; // radius
        const float as = 0.2f;//1.4f; // angular_speed, radians/sec
        float lx = r_x * std::cos(3.0f * as * accum_dt);
        float ly = BASE_Y_OFFSET+1.5f + r_y * std::sin(as * accum_dt);
        float lz = r_z * std::cos(7.0f * as * accum_dt);
        lights[0].position = glm::vec4(lx, ly, lz, 1.0f);
    }

    {
        const float r_xz = 3.0f; // radius
        const float r_y = 1.2f; // radius
        const float o_y = 2.0f;
        const float as = 2.4f; // angular_speed, radians/sec
        float lx = r_xz * std::cos(as * accum_dt);
        float ly = BASE_Y_OFFSET+o_y;
        float lz = r_xz * std::cos(2.0f * as * accum_dt);
        lights[1].position = glm::vec4(lx, ly, lz, 1.0f);
    }

    for (int i = 2; i < 8; ++i)
    {
        float fi = (float)i;
        const float r_xz = 7.0f; // radius
        const float r_y = 1.2f; // radius
        const float as = 1.4f; // angular_speed, radians/sec
        float lx = r_xz * std::sin(as * accum_dt + fi);
        float ly = BASE_Y_OFFSET+2.0f + r_y * std::sin(as * accum_dt + fi);
        float lz = r_xz * std::cos(2.0f * as * accum_dt + fi);
        lights[i].position = glm::vec4(lx, ly, lz, 1.0f);
    }
    // TODO: vary color
}

bool Scene::update_scene_ubo()
{
    auto &scene_ubo = get_scene_ubo();
    auto camera = _cameras["perspective"];
    //auto light = _lights[_current_light];

    VkResult result;

    //Log("#   Map Uniform Buffer\n");
    void *mapped = nullptr;
    result = vkMapMemory(_ctx->device, _scene_ubo.memory, 0, VK_WHOLE_SIZE, 0, &mapped);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    //Log("#   Copy matrices, first time.\n");

    // TODO: use offsetof
    memcpy(mapped, glm::value_ptr(camera.v), sizeof(camera.v));
    memcpy(((float *)mapped + 16), glm::value_ptr(camera.p), sizeof(camera.p));
    memcpy(((float *)mapped + 32), &_lighting_block, sizeof(_lighting_block));
    
    // useless, COHERENT_BIT
    VkMappedMemoryRange memory_range = {};
    memory_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    memory_range.memory = _scene_ubo.memory;
    memory_range.offset = 0;
    memory_range.size = VK_WHOLE_SIZE;
    vkFlushMappedMemoryRanges(_ctx->device, 1, &memory_range);

    //Log("#   UnMap Uniform Buffer\n");
    vkUnmapMemory(_ctx->device, _scene_ubo.memory);


    //
    // COMPUTE UBO
    //
    {
        void *mapped = nullptr;
        result = vkMapMemory(_ctx->device, compute_particles.ubo.memory, 0, VK_WHOLE_SIZE, 0, &mapped);
        ErrorCheck(result);
        if (result != VK_SUCCESS)
            return false;

        memcpy(mapped, &compute_particles.data, sizeof(compute_particles.data));

        vkUnmapMemory(_ctx->device, compute_particles.ubo.memory);
    }

    return true;
}

bool Scene::update_all_objects_ubos()
{
    VkResult result;
    std::array<VkMappedMemoryRange, 2> memory_ranges = {};

    // TODO: update only modified(animated) matrices.

    {
        auto &ubo = get_global_object_matrices_ubo();

        void *mapped = nullptr;
        result = vkMapMemory(_ctx->device, ubo.memory, 0, VK_WHOLE_SIZE, 0, &mapped);
        ErrorCheck(result);
        if (result != VK_SUCCESS)
            return false;

        memcpy(mapped, ubo.host_data, _objects.size() * ubo.alignment);

        // TODO: find out when this is really needed.
        memory_ranges[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        memory_ranges[0].memory = ubo.memory;
        memory_ranges[0].offset = 0;
        memory_ranges[0].size = VK_WHOLE_SIZE;//_objects.size() * ubo.alignment;

        vkUnmapMemory(_ctx->device, ubo.memory);
    }

    {
        auto &ubo_material = get_global_object_material_ubo();

        void *mapped = nullptr;
        result = vkMapMemory(_ctx->device, ubo_material.memory, 0, VK_WHOLE_SIZE, 0, &mapped);
        ErrorCheck(result);
        if (result != VK_SUCCESS)
            return false;

        memcpy(mapped, ubo_material.host_data, _objects.size() * ubo_material.alignment);

        // TODO: find out when this is really needed.
        memory_ranges[1].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        memory_ranges[1].memory = ubo_material.memory;
        memory_ranges[1].offset = 0;
        memory_ranges[1].size = VK_WHOLE_SIZE;//_objects.size() * ubo_material.alignment;

        vkUnmapMemory(_ctx->device, ubo_material.memory);
    }

    vkFlushMappedMemoryRanges(_ctx->device, (uint32_t)memory_ranges.size(), memory_ranges.data());

    return true;
}

bool Scene::update_all_instances_vbos()
{
    if (_simulate_cpu)
    {
        auto &is = _instance_sets["metal_spheres"];
        uint32_t instance_count = std::min(is.instance_count, (uint32_t)_nb_instances);
        size_t instance_data_size = instance_count * sizeof(instance_data_t);
        copy_data_to_staging_buffer(is.staging_buffer, is.instance_data.data(), instance_data_size, false);
        copy_buffer_to_buffer(is.staging_buffer.buffer, is.instance_buffer.buffer, instance_data_size, 0, 0);
    }

#if 0
    if (_animate_instance_data)
    {
        for (auto _is : _instance_sets)
        {
            auto &is = _is.second;

            //Log("#    Map (Staging) Vertex Buffer\n");
            uint32_t instance_count = std::min(is.instance_count, (uint32_t)_nb_instances);
            size_t instance_data_size = instance_count * sizeof(instance_data_t);
#if USE_STAGING_FOR_INSTANCING == 1
            // TODO: map persistent, stored in instance_set
            copy_data_to_staging_buffer(is.staging_buffer, is.instance_data.data(), instance_data_size, false);
            copy_buffer_to_buffer(is.staging_buffer.buffer, is.instance_buffer.buffer, instance_data_size, 0, 0);
#else
            VkResult result;
            void *mapped = nullptr;
            result = vkMapMemory(_ctx->device, is.instance_buffer.memory, 0, instance_data_size, 0, &mapped);
            ErrorCheck(result);
            if (result != VK_SUCCESS)
                return false;

            memcpy(mapped, is.instance_data.data(), instance_data_size);

            vkUnmapMemory(_ctx->device, is.instance_buffer.memory);
#endif
        }
    }
#endif
    return true;
}

bool Scene::create_procedural_textures()
{
    Log("#     Create Texture Staging Buffer.\n");
    VkDeviceSize max_texture_size = 4096 * 4096 * 4 * sizeof(float); // 4K RGBA Float
    if (!create_buffer(&_texture_staging_buffer.buffer, &_texture_staging_buffer.memory, max_texture_size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
        return false;

    Log("#     Compute Procedural Texture\n");

    using create_func = void(*)(utils::loaded_image*);
    auto create_texture = [&](const std::string &name, VkFormat format, create_func f)
    {
        utils::loaded_image image;
        f(&image);

        auto &texture = _textures[name];
        texture.format = format;
        texture.extent = { image.width, image.height, 1 };

        create_texture_2d(&texture);
        copy_data_to_staging_buffer(_texture_staging_buffer, image.data, image.size);
        transition_texture(&texture.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copy_buffer_to_image(_texture_staging_buffer.buffer, texture.image, texture.extent);
        transition_texture(&texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        delete[] image.data;
    };


    create_texture("checker_base", VK_FORMAT_R32G32B32_SFLOAT, utils::create_checker_base_image);
    create_texture("checker_spec", VK_FORMAT_R32G32B32_SFLOAT, utils::create_checker_spec_image);

    create_texture("neutral_base", VK_FORMAT_R8G8B8A8_UNORM, utils::create_neutral_base_image);
    create_texture("neutral_metal_spec", VK_FORMAT_R32G32B32A32_SFLOAT, utils::create_neutral_metal_spec_image);
    create_texture("neutral_dielectric_spec", VK_FORMAT_R32G32B32A32_SFLOAT, utils::create_neutral_dielectric_spec_image);

    //
    // TEXTURE VIEWS
    //

    VkResult result;

    for (auto &t : _textures)
    {
        VkImageViewCreateInfo texture_image_view_create_info = vk::init::image::image_view_create_info();
        texture_image_view_create_info.image = t.second.image;
        texture_image_view_create_info.format = t.second.format;

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
    sampler_create_info.anisotropyEnable = VK_TRUE;
    sampler_create_info.maxAnisotropy = 16;
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

    // staging buffer
    vkDestroyBuffer(_ctx->device, _texture_staging_buffer.buffer, nullptr);
    vkFreeMemory(_ctx->device, _texture_staging_buffer.memory, nullptr);

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

bool Scene::create_all_descriptor_set_layouts(VkDevice device, VkDescriptorSetLayout *layouts)
{
    VkResult result;

    // 3 SETS
    //    set = 0 (SCENE)
    //        binding = 0 : camera matrices, light pos (UBO)(VS+FS)
    //        binding = 2 : texture sampler            (SMP)(FS)
    //    set = 1 (MATERIAL instance)
    //        binding = 0 : base texture               (TEX)(FS)
    //        binding = 1 : spec texture               (TEX)(FS)
    //    set = 2 (OBJECT instance)
    //        binding = 0 : model matrix               (Dyn UBO)(VS)
    //        binding = 1 : material overrides         (Dyn UBO)(FS) // same ubo?
    //    set = x (COMPUTE particles)
    //        binding = 0 : instance data              (SSBO)
    //        binding = 1 : simulation params          (UBO)

    //
    // PER-SCENE
    //
    {
        std::array<VkDescriptorSetLayoutBinding, 2> bindings = {};

        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[0].descriptorCount = 1; // use >1 for arrays bound to a single binding.
        bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[0].pImmutableSamplers = nullptr;

        bindings[1].binding = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        bindings[1].descriptorCount = 1;
        bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[1].pImmutableSamplers = nullptr; // TODO: set my sampler here, no need to bind afterwards

        VkDescriptorSetLayoutCreateInfo desc_set_layout_create_info = {};
        desc_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        desc_set_layout_create_info.bindingCount = (uint32_t)bindings.size();
        desc_set_layout_create_info.pBindings = bindings.data();

        Log("#      Create Default Descriptor Set Layout for Scene Uniforms\n");
        result = vkCreateDescriptorSetLayout(device, &desc_set_layout_create_info, nullptr, layouts + SCENE_DESCRIPTOR_SET_LAYOUT);
        ErrorCheck(result);
        if (result != VK_SUCCESS)
            return false;
    }

    //
    // PER-MATERIAL INSTANCE
    //
    {
        std::array<VkDescriptorSetLayoutBinding, 2> bindings = {};

        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[0].pImmutableSamplers = nullptr;

        bindings[1].binding = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        bindings[1].descriptorCount = 1;
        bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[1].pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo desc_set_layout_create_info = {};
        desc_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        desc_set_layout_create_info.bindingCount = (uint32_t)bindings.size();
        desc_set_layout_create_info.pBindings = bindings.data();

        Log("#      Create Default Descriptor Set Layout for Material instance Uniforms\n");
        result = vkCreateDescriptorSetLayout(device, &desc_set_layout_create_info, nullptr, layouts + MATERIAL_DESCRIPTOR_SET_LAYOUT);
        ErrorCheck(result);
        if (result != VK_SUCCESS)
            return false;
    }

    //
    // PER-OBJECT
    //
    {
        std::array<VkDescriptorSetLayoutBinding, 2> bindings = {};

        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        bindings[0].pImmutableSamplers = nullptr;

        bindings[1].binding = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        bindings[1].descriptorCount = 1;
        bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[1].pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo desc_set_layout_create_info = {};
        desc_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        desc_set_layout_create_info.bindingCount = (uint32_t)bindings.size();
        desc_set_layout_create_info.pBindings = bindings.data();

        Log("#      Create Default Descriptor Set Layout for Objects Uniforms\n");
        result = vkCreateDescriptorSetLayout(device, &desc_set_layout_create_info, nullptr, layouts + OBJECT_DESCRIPTOR_SET_LAYOUT);
        ErrorCheck(result);
        if (result != VK_SUCCESS)
            return false;
    }

    //
    // COMPUTE
    //
    {
        std::array<VkDescriptorSetLayoutBinding, 2> bindings = {};

        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[0].descriptorCount = 1; // use >1 for arrays bound to a single binding.
        bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        bindings[0].pImmutableSamplers = nullptr;

        bindings[1].binding = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[1].descriptorCount = 1;
        bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        bindings[1].pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo desc_set_layout_create_info = {};
        desc_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        desc_set_layout_create_info.bindingCount = (uint32_t)bindings.size();
        desc_set_layout_create_info.pBindings = bindings.data();

        Log("#      Create Descriptor Set Layout for Compute Particles (SSBO+UBO)\n");
        result = vkCreateDescriptorSetLayout(device, &desc_set_layout_create_info, nullptr, layouts + COMPUTE_DESCRIPTOR_SET_LAYOUT);
        ErrorCheck(result);
        if (result != VK_SUCCESS)
            return false;
    }

    return true;
}

bool Scene::create_all_descriptor_sets()
{
    _view_t &view = _views["perspective"]; // scene descriptor set is here

    VkResult result;

    VkDescriptorSetAllocateInfo descriptor_allocate_info = {};
    descriptor_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_allocate_info.descriptorPool = _ctx->descriptor_pool;// _descriptor_pool;
    descriptor_allocate_info.descriptorSetCount = 1;
    descriptor_allocate_info.pSetLayouts = &_descriptor_set_layouts[SCENE_DESCRIPTOR_SET_LAYOUT];

    Log("#      Allocate Scene/View Descriptor Set\n");
    result = vkAllocateDescriptorSets(_ctx->device, &descriptor_allocate_info, &view.descriptor_set);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    // TODO: create all material instances descriptor sets at once.
    descriptor_allocate_info.descriptorSetCount = 1;
    descriptor_allocate_info.pSetLayouts = &_descriptor_set_layouts[MATERIAL_DESCRIPTOR_SET_LAYOUT];
    for (auto &m : _material_instances)
    {
        Log("#      Allocate Material Instance[n] Descriptor Set\n");
        result = vkAllocateDescriptorSets(_ctx->device, &descriptor_allocate_info, &m.second.descriptor_set);
        ErrorCheck(result);
        if (result != VK_SUCCESS)
            return false;
    }

    Log("#      Allocate Object Instance Descriptor Sets\n");
    descriptor_allocate_info.descriptorSetCount = 1;
    descriptor_allocate_info.pSetLayouts = &_descriptor_set_layouts[OBJECT_DESCRIPTOR_SET_LAYOUT];
    result = vkAllocateDescriptorSets(_ctx->device, &descriptor_allocate_info, &_global_objects_descriptor_set);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    Log("#      Allocate Compute Descriptor Set\n");
    descriptor_allocate_info.descriptorSetCount = 1;
    descriptor_allocate_info.pSetLayouts = &_descriptor_set_layouts[COMPUTE_DESCRIPTOR_SET_LAYOUT];
    result = vkAllocateDescriptorSets(_ctx->device, &descriptor_allocate_info, &compute_particles.descriptor_set);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;

    //
    // CONFIGURE DESCRIPTOR SETS
    //
    // When a set is allocated all values are undefined and all 
    // descriptors are uninitialised. must init all statically used bindings:

    std::vector<VkWriteDescriptorSet> write_descriptor_sets = {};

    // 4 SETS
    //    set = 0 (SCENE)
    //        binding = 0 : camera matrices, light     (UBO)(VS+FS)
    //        binding = 1 : texture sampler            (SMP)(FS)
    //    set = 1 (MATERIAL instance)
    //        binding = 0 : base texture               (TEX)(FS)
    //        binding = 1 : spec texture               (TEX)(FS)
    //    set = 2 (OBJECT instance)
    //        binding = 0 : model matrix               (Dyn UBO)(VS)
    //        binding = 1 : material overrides         (Dyn UBO)(FS) // same ubo?
    //    set = x (compute)
    //        binding = 0 : per-instance data          (SSBO)
    //        binding = 1 : simulation data            (UBO)

    // SCENE UBO CAMERA = 0
    {
        Log("#      Update Descriptor Set (Scene CAMERA + LIGHT UBO)\n");

        VkDescriptorBufferInfo descriptor_buffer_info = {};
        descriptor_buffer_info.buffer = _scene_ubo.buffer;
        descriptor_buffer_info.offset = 0;
        descriptor_buffer_info.range = VK_WHOLE_SIZE;

        VkWriteDescriptorSet write_descriptor_set = {};
        write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_descriptor_set.dstSet = view.descriptor_set;
        write_descriptor_set.dstBinding = 0;
        write_descriptor_set.dstArrayElement = 0;
        write_descriptor_set.descriptorCount = 1;
        write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write_descriptor_set.pImageInfo = nullptr;
        write_descriptor_set.pBufferInfo = &descriptor_buffer_info;
        write_descriptor_set.pTexelBufferView = nullptr;

        //write_descriptor_sets.push_back(write_descriptor_set);
        vkUpdateDescriptorSets(_ctx->device, 1, &write_descriptor_set, 0, nullptr);
    }

    // SAMPLER = 1
    {
        Log("#      Update Descriptor Set (Scene Global Sampler)\n");

        VkDescriptorImageInfo descriptor_image_info = {};
        descriptor_image_info.sampler = _samplers[0];

        VkWriteDescriptorSet write_descriptor_set = {};
        write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_descriptor_set.dstSet = view.descriptor_set;
        write_descriptor_set.dstBinding = 1;
        write_descriptor_set.dstArrayElement = 0;
        write_descriptor_set.descriptorCount = 1;
        write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        write_descriptor_set.pImageInfo = &descriptor_image_info;
        write_descriptor_set.pBufferInfo = nullptr;
        write_descriptor_set.pTexelBufferView = nullptr;

        //write_descriptor_sets.push_back(write_descriptor_set);
        vkUpdateDescriptorSets(_ctx->device, 1, &write_descriptor_set, 0, nullptr);
    }

    //
    // MATERIAL INSTANCES, SET = 1
    //

    for (auto &m : _material_instances)
    {
        // BASE TEX = 0
        {
            VkDescriptorImageInfo descriptor_image_info = {};
            descriptor_image_info.imageView = _textures[m.second.base_tex].view;
            descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_PREINITIALIZED; // why ? we did change the layout manually!! VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL

            VkWriteDescriptorSet write_descriptor_set = {};
            write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write_descriptor_set.dstSet = m.second.descriptor_set;
            write_descriptor_set.dstBinding = 0;
            write_descriptor_set.dstArrayElement = 0;
            write_descriptor_set.descriptorCount = 1;
            write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            write_descriptor_set.pImageInfo = &descriptor_image_info;
            write_descriptor_set.pBufferInfo = nullptr;
            write_descriptor_set.pTexelBufferView = nullptr;

            Log("#      Update Descriptor Set for Material Instance [n] BASE TEX\n");
            //write_descriptor_sets.push_back(write_descriptor_set);
            vkUpdateDescriptorSets(_ctx->device, 1, &write_descriptor_set, 0, nullptr);
        }

        // SPEC TEX = 1
        {
            VkDescriptorImageInfo descriptor_image_info = {};
            descriptor_image_info.imageView = _textures[m.second.spec_tex].view;
            //descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_PREINITIALIZED; // why ? we did change the layout manually!! VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // why ? we did change the layout manually!! VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL

            VkWriteDescriptorSet write_descriptor_set = {};
            write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write_descriptor_set.dstSet = m.second.descriptor_set;
            write_descriptor_set.dstBinding = 1;
            write_descriptor_set.dstArrayElement = 0;
            write_descriptor_set.descriptorCount = 1;
            write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            write_descriptor_set.pImageInfo = &descriptor_image_info;
            write_descriptor_set.pBufferInfo = nullptr;
            write_descriptor_set.pTexelBufferView = nullptr;

            Log("#      Update Descriptor Set for Material Instance [n] SPEC TEX\n");
            //write_descriptor_sets.push_back(write_descriptor_set);
            vkUpdateDescriptorSets(_ctx->device, 1, &write_descriptor_set, 0, nullptr);
        }
    }

    //
    // DYNAMIC GLOBAL OBJECT UBOs, SET = 2
    //

    // MATRICES UBO = 0
    {
        VkDescriptorBufferInfo descriptor_buffer_info = {};
        descriptor_buffer_info.buffer = _global_object_matrices_ubo.buffer;
        descriptor_buffer_info.offset = 0;
        descriptor_buffer_info.range = VK_WHOLE_SIZE;

        VkWriteDescriptorSet write_descriptor_set = {};
        write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_descriptor_set.dstSet = _global_objects_descriptor_set;
        write_descriptor_set.dstBinding = 0;
        write_descriptor_set.dstArrayElement = 0;
        write_descriptor_set.descriptorCount = 1;
        write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        write_descriptor_set.pImageInfo = nullptr;
        write_descriptor_set.pBufferInfo = &descriptor_buffer_info;
        write_descriptor_set.pTexelBufferView = nullptr;

        Log("#      Update Descriptor Set (Object Matrices UBO)\n");
        //write_descriptor_sets.push_back(write_descriptor_set);
        vkUpdateDescriptorSets(_ctx->device, 1, &write_descriptor_set, 0, nullptr);
    }

    // MATERIALS UBO = 1
    {
        VkDescriptorBufferInfo descriptor_buffer_info = {};
        descriptor_buffer_info.buffer = _global_object_material_ubo.buffer;
        descriptor_buffer_info.offset = 0;
        descriptor_buffer_info.range = VK_WHOLE_SIZE;

        VkWriteDescriptorSet write_descriptor_set = {};
        write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_descriptor_set.dstSet = _global_objects_descriptor_set;
        write_descriptor_set.dstBinding = 1;
        write_descriptor_set.dstArrayElement = 0;
        write_descriptor_set.descriptorCount = 1;
        write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        write_descriptor_set.pImageInfo = nullptr;
        write_descriptor_set.pBufferInfo = &descriptor_buffer_info;
        write_descriptor_set.pTexelBufferView = nullptr;

        Log("#      Update Descriptor Set (Object Materials UBO)\n");
        //write_descriptor_sets.push_back(write_descriptor_set);
        vkUpdateDescriptorSets(_ctx->device, 1, &write_descriptor_set, 0, nullptr);
    }

    //
    // COMPUTE - PER INSTANCE SSBO = 0
    //
    {
        Log("#      Update Descriptor Set (Per-Instance SSBO)\n");

        VkDescriptorBufferInfo descriptor_buffer_info = {};
        descriptor_buffer_info.buffer = _instance_sets["metal_spheres"].instance_buffer.buffer;
        descriptor_buffer_info.offset = 0;
        descriptor_buffer_info.range = VK_WHOLE_SIZE;

        VkWriteDescriptorSet write_descriptor_set = {};
        write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_descriptor_set.dstSet = compute_particles.descriptor_set;
        write_descriptor_set.dstBinding = 0;
        write_descriptor_set.dstArrayElement = 0;
        write_descriptor_set.descriptorCount = 1;
        write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write_descriptor_set.pImageInfo = nullptr;
        write_descriptor_set.pBufferInfo = &descriptor_buffer_info;
        write_descriptor_set.pTexelBufferView = nullptr;

        //write_descriptor_sets.push_back(write_descriptor_set);
        vkUpdateDescriptorSets(_ctx->device, 1, &write_descriptor_set, 0, nullptr);
    }

    //
    // COMPUTE - SIMULATION DATA UBO = 1
    //
    {
        Log("#      Update Descriptor Set (Per-Instance SSBO)\n");

        VkDescriptorBufferInfo descriptor_buffer_info = {};
        descriptor_buffer_info.buffer = compute_particles.ubo.buffer;
        descriptor_buffer_info.offset = 0;
        descriptor_buffer_info.range = VK_WHOLE_SIZE;

        VkWriteDescriptorSet write_descriptor_set = {};
        write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_descriptor_set.dstSet = compute_particles.descriptor_set;
        write_descriptor_set.dstBinding = 1;
        write_descriptor_set.dstArrayElement = 0;
        write_descriptor_set.descriptorCount = 1;
        write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write_descriptor_set.pImageInfo = nullptr;
        write_descriptor_set.pBufferInfo = &descriptor_buffer_info;
        write_descriptor_set.pTexelBufferView = nullptr;

        vkUpdateDescriptorSets(_ctx->device, 1, &write_descriptor_set, 0, nullptr);
    }


    // UPDATE ALL AT ONCE
    //vkUpdateDescriptorSets(_ctx->device, (uint32_t)write_descriptor_sets.size(), write_descriptor_sets.data(), 0, nullptr);

    return true;
}

bool Scene::compile()
{
    auto &is = _instance_sets["metal_spheres"];

    Log("#     Create Instance Set SSBO/VBO\n");
    if (!create_buffer(
        &is.instance_buffer.buffer,
        &is.instance_buffer.memory,
        MAX_INSTANCE_COUNT * sizeof(instance_data_t),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
        return false;

    if (!create_buffer(
        &is.staging_buffer.buffer,
        &is.staging_buffer.memory,
        MAX_INSTANCE_COUNT * sizeof(instance_data_t),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
        return false;

    // initial fill of buffer
    uint32_t instance_count = MAX_INSTANCE_COUNT;
    size_t instance_data_size = instance_count * sizeof(instance_data_t);
    copy_data_to_staging_buffer(is.staging_buffer, is.instance_data.data(), instance_data_size, false);
    copy_buffer_to_buffer(is.staging_buffer.buffer, is.instance_buffer.buffer, instance_data_size, 0, 0);

    // clear simulation instance data.
    _instance_sets["metal_spheres"].instance_data.clear();


    // All descriptor sets, for all objects/instance_set
    Log("#     Create Scene and global object Descriptor Ses\n");
    if (!create_all_descriptor_sets())
        return false;


    return true;
}

bool Scene::build_pipelines(VkRenderPass rp)
{
    VkResult result;

    _pipeline_t &default_pipeline = _pipelines["default"];

    {
        std::array<VkDescriptorSetLayout, 3> pipeline_descriptor_set_layouts = {
            _descriptor_set_layouts[SCENE_DESCRIPTOR_SET_LAYOUT], // scene ubo
            _descriptor_set_layouts[MATERIAL_DESCRIPTOR_SET_LAYOUT], // sampler
            _descriptor_set_layouts[OBJECT_DESCRIPTOR_SET_LAYOUT]  // material override per object ubo
        };

        // use it later to define uniform buffer
        VkPipelineLayoutCreateInfo layout_create_info = {};
        layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layout_create_info.setLayoutCount = (uint32_t)pipeline_descriptor_set_layouts.size();
        layout_create_info.pSetLayouts = pipeline_descriptor_set_layouts.data();
        layout_create_info.pushConstantRangeCount = 0;
        layout_create_info.pPushConstantRanges = nullptr; // constant into shader for opti???

        Log("#     Create Default Pipeline Layout\n");
        result = vkCreatePipelineLayout(_ctx->device, &layout_create_info, nullptr, &default_pipeline.pipeline_layout);
        ErrorCheck(result);
        if (result != VK_SUCCESS)
            return false;
    }

    Log("#     Create Default Vertex Shader\n");
    if (!create_shader_module("./simple.vert.spv", &default_pipeline.vs))
        return false;

    Log("#     Create Default Fragment Shader\n");
    if (!create_shader_module("./simple.frag.spv", &default_pipeline.fs))
        return false;

    std::array<VkPipelineShaderStageCreateInfo, 2> shader_stage_create_infos = {
        vk::init::pipeline::shader_stage_create_info(default_pipeline.vs, VK_SHADER_STAGE_VERTEX_BIT),
        vk::init::pipeline::shader_stage_create_info(default_pipeline.fs, VK_SHADER_STAGE_FRAGMENT_BIT)
    };

    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {};
    vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state_create_info.vertexBindingDescriptionCount = vertex_t::binding_description_count();
    vertex_input_state_create_info.pVertexBindingDescriptions = vertex_t::binding_descriptions();
    vertex_input_state_create_info.vertexAttributeDescriptionCount = vertex_t::attribute_description_count();
    vertex_input_state_create_info.pVertexAttributeDescriptions = vertex_t::attribute_descriptions();

    // vertex topology config = triangles
    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {};
    input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = vk::init::pipeline::viewport(); // dynamic, will set the correct size later.

    VkRect2D scissors = {};
    scissors.offset = { 0, 0 };
    scissors.extent = { 512, 512 };

    VkPipelineViewportStateCreateInfo viewport_state_create_info = {};
    viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_create_info.viewportCount = 1;
    viewport_state_create_info.pViewports = &viewport;
    viewport_state_create_info.scissorCount = 1;
    viewport_state_create_info.pScissors = &scissors;

    VkPipelineRasterizationStateCreateInfo raster_state_create_info = vk::init::pipeline::raster_state_create_info();
    raster_state_create_info.polygonMode = VK_POLYGON_MODE_FILL; //VK_POLYGON_MODE_LINE;
    raster_state_create_info.cullMode = VK_CULL_MODE_NONE;// VK_CULL_MODE_BACK_BIT;
    raster_state_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisample_state_create_info = vk::init::pipeline::multisample_state_create_info_NO_MSAA();

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info = vk::init::pipeline::depth_stencil_state_create_info();

    VkPipelineColorBlendAttachmentState color_blend_attachment_state = vk::init::pipeline::color_blend_attachment_state_NO_BLEND();

    VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = vk::init::pipeline::color_blend_state_create_info();
    color_blend_state_create_info.attachmentCount = 1;
    color_blend_state_create_info.pAttachments = &color_blend_attachment_state;

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
    pipeline_create_info.layout = default_pipeline.pipeline_layout;
    pipeline_create_info.renderPass = rp; // TODO: create a render pass inside Scene
    pipeline_create_info.subpass = 0;
    pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE; // only if VK_PIPELINE_CREATE_DERIVATIVE flag is set.
    pipeline_create_info.basePipelineIndex = 0;

    Log("#     Create Default Pipeline\n");
    result = vkCreateGraphicsPipelines(
        _ctx->device,
        VK_NULL_HANDLE, // cache
        1,
        &pipeline_create_info,
        nullptr,
        &default_pipeline.pipeline);
    ErrorCheck(result);
    if (result != VK_SUCCESS)
        return false;


    //
    // Pipeline for instancing.
    //

    {
        std::array<VkDescriptorSetLayout, 2> pipeline_descriptor_set_layouts = {
            _descriptor_set_layouts[SCENE_DESCRIPTOR_SET_LAYOUT], // scene ubo
            _descriptor_set_layouts[MATERIAL_DESCRIPTOR_SET_LAYOUT]  // sampler
        };

        // use it later to define uniform buffer
        VkPipelineLayoutCreateInfo layout_create_info = {};
        layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layout_create_info.setLayoutCount = (uint32_t)pipeline_descriptor_set_layouts.size();
        layout_create_info.pSetLayouts = pipeline_descriptor_set_layouts.data();
        layout_create_info.pushConstantRangeCount = 0;
        layout_create_info.pPushConstantRanges = nullptr; // constant into shader for opti???

        Log("#     Create Instancing Pipeline Layout\n");
        result = vkCreatePipelineLayout(_ctx->device, &layout_create_info, nullptr, &_instance_pipe.pipeline_layout);
        ErrorCheck(result);
        if (result != VK_SUCCESS)
            return false;
    }

    Log("#     Create Instancing Vertex Shader\n");
    if (!create_shader_module("./instancing.vert.spv", &_instance_pipe.vs))
        return false;

    Log("#     Create Instancing Fragment Shader\n");
    if (!create_shader_module("./instancing.frag.spv", &_instance_pipe.fs))
        return false;

    shader_stage_create_infos[0].module = _instance_pipe.vs;
    shader_stage_create_infos[1].module = _instance_pipe.fs;

    {
        VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {};
        vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input_state_create_info.vertexBindingDescriptionCount = instance_data_t::binding_description_count();
        vertex_input_state_create_info.pVertexBindingDescriptions = instance_data_t::binding_descriptions();
        vertex_input_state_create_info.vertexAttributeDescriptionCount = instance_data_t::attribute_description_count();
        vertex_input_state_create_info.pVertexAttributeDescriptions = instance_data_t::attribute_descriptions();

        VkGraphicsPipelineCreateInfo pipeline_create_info = {};
        pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_create_info.stageCount = (uint32_t)shader_stage_create_infos.size();
        pipeline_create_info.pStages = shader_stage_create_infos.data(); // <-- instancing shaders
        pipeline_create_info.pVertexInputState = &vertex_input_state_create_info; // <--- instancing 2 vertex buffers (vertex and instance data)
        pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;
        pipeline_create_info.pTessellationState = nullptr;
        pipeline_create_info.pViewportState = &viewport_state_create_info;
        pipeline_create_info.pRasterizationState = &raster_state_create_info;
        pipeline_create_info.pMultisampleState = &multisample_state_create_info;
        pipeline_create_info.pDepthStencilState = &depth_stencil_state_create_info;
        pipeline_create_info.pColorBlendState = &color_blend_state_create_info;
        pipeline_create_info.pDynamicState = &dynamic_state_create_info;
        pipeline_create_info.layout = _instance_pipe.pipeline_layout; // <--- instancing pipe layout
        pipeline_create_info.renderPass = rp;
        pipeline_create_info.subpass = 0;
        pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
        pipeline_create_info.basePipelineIndex = 0;

        Log("#     Create Instancing Pipeline\n");
        result = vkCreateGraphicsPipelines(
            _ctx->device,
            VK_NULL_HANDLE, // cache
            1,
            &pipeline_create_info,
            nullptr,
            &_instance_pipe.pipeline);
        ErrorCheck(result);
        if (result != VK_SUCCESS)
            return false;
    }

    //
    // COMPUTE PARRTICLES
    //

    {
        VkDescriptorSetLayout compute_pipeline_descriptor_set_layout = 
            _descriptor_set_layouts[COMPUTE_DESCRIPTOR_SET_LAYOUT];

        VkPipelineLayoutCreateInfo layout_create_info = {};
        layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layout_create_info.setLayoutCount = 1;
        layout_create_info.pSetLayouts = &compute_pipeline_descriptor_set_layout;
        layout_create_info.pushConstantRangeCount = 0;
        layout_create_info.pPushConstantRanges = nullptr;

        Log("#     Create Compute Pipeline Layout\n");
        result = vkCreatePipelineLayout(_ctx->device, &layout_create_info, nullptr, &compute_particles.pipe.pipeline_layout);
        ErrorCheck(result);
        if (result != VK_SUCCESS)
            return false;

        Log("#     Create Particles Compute Shader\n");
        if (!create_shader_module("./particles.comp.spv", &compute_particles.pipe.cs))
            return false;

        VkPipelineShaderStageCreateInfo compute_shader_stage_create_info =
            vk::init::pipeline::shader_stage_create_info(compute_particles.pipe.cs, VK_SHADER_STAGE_COMPUTE_BIT);

        VkComputePipelineCreateInfo compute_pipeline_create_info = {};
        compute_pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        compute_pipeline_create_info.stage = compute_shader_stage_create_info;
        compute_pipeline_create_info.layout = compute_particles.pipe.pipeline_layout;
        compute_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
        compute_pipeline_create_info.basePipelineIndex = 0;

        Log("#     Create Instancing Pipeline\n");
        result = vkCreateComputePipelines(
            _ctx->device,
            VK_NULL_HANDLE, // cache
            1,
            &compute_pipeline_create_info,
            nullptr,
            &compute_particles.pipe.pipeline);
        ErrorCheck(result);
        if (result != VK_SUCCESS)
            return false;
    }

    return true;
}

void Scene::destroy_pipelines()
{
    //Log("#    Destroy Descriptor Pool\n");
    //vkDestroyDescriptorPool(_ctx->device, _descriptor_pool, nullptr);

    Log("#    Destroy Descriptor Set Layout\n");
    for (int i = 0; i < DESCRIPTOR_SET_LAYOUT_COUNT; ++i)
    {
        vkDestroyDescriptorSetLayout(_ctx->device, _descriptor_set_layouts[i], nullptr);
    }

    for (auto p : _pipelines)
    {
        auto pipe = p.second;

        Log("#    Destroy Shader Modules\n");
        vkDestroyShaderModule(_ctx->device, pipe.vs, nullptr);
        vkDestroyShaderModule(_ctx->device, pipe.fs, nullptr);

        Log("#    Destroy Pipeline\n");
        vkDestroyPipeline(_ctx->device, pipe.pipeline, nullptr);

        Log("#    Destroy Pipeline Layout\n");
        vkDestroyPipelineLayout(_ctx->device, pipe.pipeline_layout, nullptr);
    }

    // instancing pipeline
    Log("#    Destroy Shader Modules\n");
    vkDestroyShaderModule(_ctx->device, _instance_pipe.vs, nullptr);
    vkDestroyShaderModule(_ctx->device, _instance_pipe.fs, nullptr);

    Log("#    Destroy Pipeline\n");
    vkDestroyPipeline(_ctx->device, _instance_pipe.pipeline, nullptr);

    Log("#    Destroy Pipeline Layout\n");
    vkDestroyPipelineLayout(_ctx->device, _instance_pipe.pipeline_layout, nullptr);
}

bool Scene::add_pipeline(pipeline_description_t p)
{
    _pipeline_t pipe = {};

    // TODO:
    // load shaders, create descriptor layouts, create pipeline, ...

    _pipelines[p.id] = pipe;

    return true;
}

bool Scene::add_material_instance(material_instance_description_t mi)
{
    _material_instance_t material_instance = {};
    material_instance.base_tex = mi.base_tex;
    material_instance.spec_tex = mi.specular_tex;

    // TODO:
    // get layout descriptions from material_id
    // create descriptor set

    _material_instances[mi.instance_id] = material_instance;

    return true;
}

Scene::_material_override_t *Scene::get_object_material(int obj_idx)
{
    auto &global_material_ubo = get_global_object_material_ubo();
    return (_material_override_t*)((uint64_t)global_material_ubo.host_data + (obj_idx * global_material_ubo.alignment));
}

//
// GUI
//

void Scene::tmp_change_sphere_base_color(int idx, const glm::vec4 &base_color)
{
    _material_override_t *material = get_object_material(idx);
    material->base_color = base_color;
}

void Scene::tmp_change_sphere_spec_color(int idx, const glm::vec4 &spec_color)
{
    _material_override_t *material = get_object_material(idx);
    material->specular = spec_color;
}

glm::vec4 Scene::get_object_base_color(int idx)
{
    _material_override_t *material = get_object_material(idx);
    return material->base_color;
}

glm::vec4 Scene::get_object_spec_color(int idx)
{
    _material_override_t *material = get_object_material(idx);
    return material->specular;
}

void Scene::show_property_sheet()
{
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
    bool *pOpen = nullptr;

    ImGui::Begin("Properties", pOpen, window_flags);
    {
        if (ImGui::CollapsingHeader("Object/Light Material"))
        {
            if (!_object_names.empty())
            {
                ImGuiComboFlags combo_flags = ImGuiComboFlags_None;
                combo_flags |= ImGuiComboFlags_HeightLargest;
                if (ImGui::BeginCombo("Object", _object_names[_current_item_idx].c_str(), combo_flags))
                {
                    for (int n = 0; n < _object_names.size(); ++n)
                    {
                        bool is_selected = (_current_item_idx == n);
                        if (ImGui::Selectable(_object_names[n].c_str(), is_selected))
                            _current_item_idx = n;
                        if (is_selected)
                            ImGui::SetItemDefaultFocus();   // Set the initial focus when opening the combo (scrolling + for keyboard navigation support in the upcoming navigation branch)
                    }
                    ImGui::EndCombo();
                }
            }

            ImGui::Combo("Current Light", &_current_light, "Light_0\0Light_1\0Light_2\0\0");

            glm::vec4 base_color = get_object_base_color(_current_item_idx);
            if (ImGui::ColorEdit4("base_color", glm::value_ptr(base_color)))
            {
                tmp_change_sphere_base_color(_current_item_idx, base_color);
            }

            glm::vec4 spec_color = get_object_spec_color(_current_item_idx);
            float roughness = (spec_color.x - 0.045f) / 0.955f;
            if (ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f))
            {
                spec_color.x = 0.045f + 0.955f * roughness;
                tmp_change_sphere_spec_color(_current_item_idx, spec_color);
            }
            float metalness = spec_color.y;
            if (ImGui::SliderFloat("Metalness", &metalness, 0.0f, 1.0f))
            {
                spec_color.y = metalness;
                tmp_change_sphere_spec_color(_current_item_idx, spec_color);
            }
            ImGui::ColorEdit4("Sun Color", glm::value_ptr(_lighting_block.sky_color));
            ImGui::ColorEdit4("Background Color", glm::value_ptr(_bg_color));
        }

        if (ImGui::CollapsingHeader("Options"))
        {
            ImGui::Checkbox("Animate light", &_animate_light);
            ImGui::Checkbox("Animate object", &_animate_object);
            ImGui::Checkbox("Animate instances", &_animate_instance_data);
        }

        if (ImGui::CollapsingHeader("Camera"))
        {
            ImGui::Checkbox("Rotate camera", &_animate_camera);
            ImGui::SliderFloat("Radius", &_instances_layout_radius, 1.0f, 400.0f);
            ImGui::SliderFloat("Camera Distance", &_camera_distance, 1.0f, 1000.0f);
            ImGui::SliderFloat("Camera Elevation", &_camera_elevation, 0.0f, 500.0f);
        }

        if (ImGui::CollapsingHeader("Curve Shape"))
        {
            ImGui::SliderFloat("Ax(big radius)", &_ax, 1.0f, 20.0f);
            ImGui::SliderFloat("Ay(big radius)", &_ay, 1.0f, 20.0f);
            ImGui::SliderFloat("Az(big radius)", &_az, 1.0f, 20.0f);
            ImGui::SliderFloat("Bx(nb big circles)", &_bx, 1.0f, 10.0f);
            ImGui::SliderFloat("By(nb big circles)", &_by, 1.0f, 10.0f);
            ImGui::SliderFloat("Bz(nb big circles)", &_bz, 1.0f, 10.0f);
            ImGui::SliderFloat("Cx(small radius)", &_cx, 0.1f, 5.0f);
            ImGui::SliderFloat("Cy(small radius)", &_cy, 0.1f, 5.0f);
            ImGui::SliderFloat("Cz(small radius)", &_cz, 0.1f, 5.0f);
            ImGui::SliderFloat("Dx(nb small circ)", &_dx, 0.0f, 200.0f);
            ImGui::SliderFloat("Dy(nb small circ)", &_dy, 0.0f, 200.0f);
            ImGui::SliderFloat("Dz(nb small circ)", &_dz, 0.0f, 200.0f);
        }

        if (ImGui::CollapsingHeader("Jitter"))
        {
            ImGui::SliderFloat("E0", &_e0, 0.0f, 5.0f);
            ImGui::SliderFloat("E1", &_e1, 0.01f, 1.0f);
            ImGui::SliderFloat("E2", &_e2, 1.0f, 5.0f);
            ImGui::SliderFloat("E3", &_e3, 0.01f, 1.0f);
        }

        if (ImGui::CollapsingHeader("Misc"))
        {
            ImGui::SliderFloat("Rx (tour/s)", &_rsx, 0.0f, 5.0f);
            ImGui::SliderFloat("Ry", &_rsy, 0.0f, 5.0f);
            ImGui::SliderFloat("Rz", &_rsz, 0.0f, 5.0f);

            ImGui::SliderFloat("Psx", &_psx, 0.01f, 1.0f);
            ImGui::SliderFloat("Psy", &_psy, 0.01f, 1.0f);
            ImGui::SliderFloat("Psz", &_psz, 0.01f, 1.0f);

            ImGui::SliderFloat("Delta time", &_pdt, 0.0001f, 0.01f);
            ImGui::SliderFloat("Speed", &_speed, 0.001f, 1.0f);
            ImGui::SliderFloat("R. Speed", &_rotation_speed, 0.001f, 1.0f);

            ImGui::SliderInt("Instances", &_nb_instances, 1, MAX_INSTANCE_COUNT);
        }
    }
    ImGui::End();
}

//
