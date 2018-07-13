#include "build_options.h"
#include "platform.h"
#include "Renderer.h"
#include "window.h"
#include "Shared.h"

#include <cstdio>
#include <array>
#include <chrono>
#include <sstream>

/* 
TODO:

*/

int main(int argc, char **argv)
{
    Log("### Main program starting.\n");
    {
        Log("# Creating Renderer\n");
        Renderer r;

        Log("# Init Renderer\n");
        if (!r.Init()) return -1;

        Log("# Creating Window\n");
        Window *w = r.OpenWindow(800, 600, "test");
        if (!w) return -1;

        VkResult result;
        VkDevice device = r.GetVulkanDevice();

        Log("# Create Command Pool\n");
        VkCommandPool command_pool = VK_NULL_HANDLE;
        VkCommandPoolCreateInfo pool_create_info = {};
        pool_create_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_create_info.queueFamilyIndex = r.GetVulkanGraphicsQueueFamilyIndex();
        pool_create_info.flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | // commands will be short lived, might be reset of freed often.
                                            VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // we are going to reset

        result = vkCreateCommandPool(device, &pool_create_info, nullptr, &command_pool);
        ErrorCheck(result);

        Log("# Allocate Command Buffer\n");
        VkCommandBuffer command_buffer;
        VkCommandBufferAllocateInfo command_buffer_allocate_info{};
        command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        command_buffer_allocate_info.commandPool = command_pool;
        command_buffer_allocate_info.commandBufferCount = 1;
        command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // primary can be pushed to a queue manually, secondary cannot.

        result = vkAllocateCommandBuffers(device, &command_buffer_allocate_info, &command_buffer);
        ErrorCheck(result);

        //Log("# Create the \"render complete\" and \"present complete\" semaphores\n");
        //VkSemaphore render_complete_semaphore = VK_NULL_HANDLE;
        //VkSemaphore present_complete_semaphore = VK_NULL_HANDLE;
        //VkSemaphoreCreateInfo semaphore_create_info = {};
        //semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        //result = vkCreateSemaphore(device, &semaphore_create_info, nullptr, &render_complete_semaphore);
        //ErrorCheck(result);
        //result = vkCreateSemaphore(device, &semaphore_create_info, nullptr, &present_complete_semaphore);
        //ErrorCheck(result);

        // FPS
        auto timer = std::chrono::steady_clock();
        auto last_time = timer.now();
        uint64_t frame_counter = 0;
        uint64_t fps = 0;

        Log("# Run...\n");
        while (r.Run())
        {
            // CPU Logic calculations
            ++frame_counter;
            if (last_time + std::chrono::seconds(1) < timer.now())
            {
                last_time = timer.now();
                fps = frame_counter;
                frame_counter = 0;

                std::ostringstream oss;
                oss << "*** FPS: " << fps << std::endl;
                Log(oss.str().c_str());
            }


            // Re create each time??? cant we reset them???
            //Log("# Create the \"render complete\" and \"present complete\" semaphores\n");
            VkSemaphore render_complete_semaphore = VK_NULL_HANDLE;
            VkSemaphore present_complete_semaphore = VK_NULL_HANDLE;
            VkSemaphoreCreateInfo semaphore_create_info = {};
            semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            result = vkCreateSemaphore(device, &semaphore_create_info, nullptr, &render_complete_semaphore);
            ErrorCheck(result);
            result = vkCreateSemaphore(device, &semaphore_create_info, nullptr, &present_complete_semaphore);
            ErrorCheck(result);

            // Begin render (acquire image, wait for queue ready)
            w->BeginRender(present_complete_semaphore);

            // Record command buffer
            VkCommandBufferBeginInfo begin_info = {};
            begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            result = vkBeginCommandBuffer(command_buffer, &begin_info);
            ErrorCheck(result);
            {
				// The render pass expects some layouts for render targets.
				// Does subpasses do it for us???? Using AttachmentReferences???
#if 1
                // Transition color image layout from VK_IMAGE_LAYOUT_PRESENT_SRC_KHR to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
                // Transition the depth/stencil image from UNDEFINED to OPTIMAL
                std::array<VkImageSubresourceRange, 2> resource_ranges = {};
                resource_ranges[0] = {VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1};// TODO: stencil only if available. Can we GET that?
                resource_ranges[1] = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
                
                std::array<VkImageMemoryBarrier, 2> layout_transition_barriers = {};
                layout_transition_barriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                //layout_transition_barriers[0].srcAccessMask = 0;
                //layout_transition_barriers[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                layout_transition_barriers[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                layout_transition_barriers[0].newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                layout_transition_barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                layout_transition_barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                layout_transition_barriers[0].image = w->GetVulkanDepthStencilImage();
                layout_transition_barriers[0].subresourceRange = resource_ranges[0];

                layout_transition_barriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                //layout_transition_barriers[1].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
                //layout_transition_barriers[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                layout_transition_barriers[1].oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                layout_transition_barriers[1].newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                layout_transition_barriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                layout_transition_barriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                layout_transition_barriers[1].image = w->GetVulkanActiveImage();
                layout_transition_barriers[1].subresourceRange = resource_ranges[1];

                vkCmdPipelineBarrier(
                    command_buffer,
                    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, // srcStageMask
                    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, // dstStageMask
                    0,
                    0, nullptr,
                    0, nullptr,
                    (uint32_t)layout_transition_barriers.size(), layout_transition_barriers.data());
#endif


                VkRect2D render_area = {};
                render_area.offset = { 0, 0 };
                render_area.extent = w->GetVulkanSurfaceSize();

                // NOTE: these values are used only if the attachment has the loadOp LOAD_OP_CLEAR
                std::array<VkClearValue, 2> clear_values = {};
                clear_values[0].depthStencil.depth = 0.0f;
                clear_values[0].depthStencil.stencil = 0;
                clear_values[1].color.float32[0] = 1.0f; // R // backbuffer is of type B8G8R8A8_UNORM
                clear_values[1].color.float32[1] = 0.0f; // G
                clear_values[1].color.float32[2] = 0.0f; // B
                clear_values[1].color.float32[3] = 1.0f; // A

                VkRenderPassBeginInfo render_pass_begin_info = {};
                render_pass_begin_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                render_pass_begin_info.renderPass      = w->GetVulkanRenderPass();
                render_pass_begin_info.framebuffer     = w->GetVulkanActiveFrameBuffer();
                render_pass_begin_info.renderArea      = render_area;
                render_pass_begin_info.clearValueCount = (uint32_t)clear_values.size();
                render_pass_begin_info.pClearValues    = clear_values.data();

                vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
                {
                    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, w->GetPipeline(0));

                    // take care of dynamic state:
                    VkExtent2D surface_size = w->GetVulkanSurfaceSize();

                    VkViewport viewport = {0, 0, (float)surface_size.width, (float)surface_size.height, 0, 1};
                    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

                    VkRect2D scissor = {0, 0, surface_size.width, surface_size.height};
                    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

                    VkDeviceSize offsets = {};
                    vkCmdBindVertexBuffers(command_buffer, 0, 1, w->GetVertexBufferPtr(), &offsets);

                    // DRAW TRIANGLE!!!!!!!!!!!!!!!!!!
                    vkCmdDraw(command_buffer,
                        3,   // vertex count
                        1,   // instance count
                        0,   // first vertex
                        0); // first instance
                }
                vkCmdEndRenderPass(command_buffer);

                // Transition color from OPTIMAL to PRESENT
                VkImageMemoryBarrier pre_present_layout_transition_barrier = {};
                pre_present_layout_transition_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                pre_present_layout_transition_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                pre_present_layout_transition_barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
                pre_present_layout_transition_barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                pre_present_layout_transition_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                pre_present_layout_transition_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                pre_present_layout_transition_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                pre_present_layout_transition_barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
                pre_present_layout_transition_barrier.image = w->GetVulkanActiveImage();

                vkCmdPipelineBarrier(command_buffer,
                    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                    0,
                    0, nullptr,
                    0, nullptr,
                    1, &pre_present_layout_transition_barrier);
            }
            result = vkEndCommandBuffer(command_buffer); // compiles the command buffer
            ErrorCheck(result);

            VkFence render_fence = {};
            VkFenceCreateInfo fence_create_info = {};
            fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            vkCreateFence(device, &fence_create_info, nullptr, &render_fence);

            // Submit command buffer
            VkPipelineStageFlags wait_stage_mask[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT}; // VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT ??
            VkSubmitInfo submit_info = {};
            submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.waitSemaphoreCount   = 1;
            submit_info.pWaitSemaphores      = &present_complete_semaphore;
            submit_info.pWaitDstStageMask    = wait_stage_mask;
            submit_info.commandBufferCount   = 1;
            submit_info.pCommandBuffers      = &command_buffer;
            submit_info.signalSemaphoreCount = 1; // signals this semaphore when the render is complete GPU side.
            submit_info.pSignalSemaphores    = &render_complete_semaphore;
            
            result = vkQueueSubmit(r.GetVulkanQueue(), 1, &submit_info, render_fence);
            ErrorCheck(result);

            // <------------------------------------------------- Wait on Fence

            vkWaitForFences(device, 1, &render_fence, VK_TRUE, UINT64_MAX);
            vkDestroyFence(device, render_fence, nullptr);

            // <------------------------------------------------- Wait on semaphores before presenting
            // End render
            w->EndRender({render_complete_semaphore});

            vkDestroySemaphore(device, render_complete_semaphore, nullptr);
            vkDestroySemaphore(device, present_complete_semaphore, nullptr);
        }

        Log("# Wait Queue Idle\n");
        vkQueueWaitIdle(r.GetVulkanQueue());

        //Log("# Destroy \"render complete\" Semaphore\n");
        //vkDestroySemaphore(device, render_complete_semaphore, nullptr);
        //vkDestroySemaphore(device, present_complete_semaphore, nullptr);

        Log("# Destroy Command Pool\n");
        vkDestroyCommandPool(device, command_pool, nullptr);
    }

    Log("### DONE - Press any key...\n");
    getchar();
    return 0;
}
