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
 * replace _renderer->GetVulkanDevice() by _device, put in cache.



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

		Log("# Create Command Pool\n");
		VkCommandPool command_pool = VK_NULL_HANDLE;
		VkCommandPoolCreateInfo pool_create_info = {};
		pool_create_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		pool_create_info.queueFamilyIndex = r.GetVulkanGraphicsQueueFamilyIndex();
		pool_create_info.flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | // commands will be short lived, might be reset of freed often.
											VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // we are going to reset

		result = vkCreateCommandPool(r.GetVulkanDevice(), &pool_create_info, nullptr, &command_pool);
		ErrorCheck(result);

		Log("# Allocate Command Buffer\n");
		VkCommandBuffer command_buffer;
		VkCommandBufferAllocateInfo command_buffer_allocate_info{};
		command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		command_buffer_allocate_info.commandPool = command_pool;
		command_buffer_allocate_info.commandBufferCount = 1;
		command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // primary can be pushed to a queue manually, secondary cannot.

		result = vkAllocateCommandBuffers(r.GetVulkanDevice(), &command_buffer_allocate_info, &command_buffer);
		ErrorCheck(result);

		Log("# Create the \"render complete\" Semaphore\n");
		VkSemaphore render_complete_semaphore = VK_NULL_HANDLE;
		VkSemaphoreCreateInfo semaphore_create_info = {};
		semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		result = vkCreateSemaphore(r.GetVulkanDevice(), &semaphore_create_info, nullptr, &render_complete_semaphore);
		ErrorCheck(result);

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




			// Begin render (acquire image, wait for queue ready)
			w->BeginRender();

			// Record command buffer
			VkCommandBufferBeginInfo begin_info = {};
			begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			result = vkBeginCommandBuffer(command_buffer, &begin_info);
			ErrorCheck(result);
			{
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
					// clears the framebuffer
				}
				vkCmdEndRenderPass(command_buffer);
			}
			result = vkEndCommandBuffer(command_buffer); // compiles the command buffer
			ErrorCheck(result);

			// Submit command buffer
			VkSubmitInfo submit_info = {};
			submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submit_info.waitSemaphoreCount   = 0;
			submit_info.pWaitSemaphores      = nullptr;
			submit_info.pWaitDstStageMask    = nullptr;
			submit_info.commandBufferCount   = 1;
			submit_info.pCommandBuffers      = &command_buffer;
			submit_info.signalSemaphoreCount = 1; // signals this semaphore when the render is complete GPU side.
			submit_info.pSignalSemaphores    = &render_complete_semaphore;
			
			result = vkQueueSubmit(r.GetVulkanQueue(), 1, &submit_info, VK_NULL_HANDLE); // no fence	
			ErrorCheck(result);

			// <------------------------------------------------- Wait on semaphores before presenting

			// End render
			w->EndRender({render_complete_semaphore});
		}

		Log("# Wait Queue Idle\n");
		vkQueueWaitIdle(r.GetVulkanQueue());

		Log("# Destroy \"render complete\" Semaphore\n");
		vkDestroySemaphore(r.GetVulkanDevice(), render_complete_semaphore, nullptr);

		Log("# Destroy Command Pool\n");
		vkDestroyCommandPool(r.GetVulkanDevice(), command_pool, nullptr);
	}

	Log("### DONE - Press any key...\n");
	getchar();
	return 0;
}
