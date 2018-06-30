#include "Renderer.h"

int main( int argc, char **argv )
{
    Renderer r;
	if (!r.Init())
		return -1;

	auto device = r.device();

	// use a fence to tell the CPU side that the GPU is ready.
	VkFence fence;
	VkFenceCreateInfo fence_create_info = {};
	fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	//fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT; // dont set to that unique possible value, it will block the fence.
	vkCreateFence(device, &fence_create_info, nullptr, &fence);

	// use a semaphore to tell the GPU that the GPU has finished a command.
	VkSemaphore semaphore;
	VkSemaphoreCreateInfo semaphore_create_info = {};
	semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	//semaphore_create_info.flags = ...; // do not exist yet, reserved for future use.
	vkCreateSemaphore(device, &semaphore_create_info, nullptr, &semaphore);


    VkCommandPool command_pool;
    VkCommandPoolCreateInfo pool_create_info{};
    pool_create_info.sType              = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_create_info.queueFamilyIndex   = r.familyIndex();
    pool_create_info.flags              = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    vkCreateCommandPool( r.device(), &pool_create_info, nullptr, &command_pool );


    VkCommandBuffer command_buffers[2]; // 2 command buffers to illustrate semaphores
    VkCommandBufferAllocateInfo command_buffer_allocate_info{};
    command_buffer_allocate_info.sType                  = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.commandPool            = command_pool;
    command_buffer_allocate_info.commandBufferCount     = 2;
    command_buffer_allocate_info.level                  = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    vkAllocateCommandBuffers( r.device(), &command_buffer_allocate_info, command_buffers );

	{
		VkCommandBufferBeginInfo begin_info = {};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		vkBeginCommandBuffer(command_buffers[0], &begin_info);
		{
			// jai pas tout compris, on verra plus tard :)
			//vkCmdPipelineBarrier(command_buffers[0],
			//	VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			//	VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			//	0,
			//	0, nullptr,
			//	0, nullptr, 
			//	0, nullptr );

			// ... fill with commands
			VkViewport viewport{};
			viewport.maxDepth = 1.0f;
			viewport.minDepth = 0.0f;
			viewport.width = 512;
			viewport.height = 512;
			viewport.x = 0;
			viewport.y = 0;
			vkCmdSetViewport(command_buffers[0], 0, 1, &viewport);
			// all commands begin with vkCmd
		}
		vkEndCommandBuffer(command_buffers[0]); // compiles the command buffer
	}

	{
		VkCommandBufferBeginInfo begin_info{};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		vkBeginCommandBuffer(command_buffers[1], &begin_info);
		{
			// ... fill with commands
			VkViewport viewport{};
			viewport.maxDepth = 1.0f;
			viewport.minDepth = 0.0f;
			viewport.width = 512;
			viewport.height = 512;
			viewport.x = 0;
			viewport.y = 0;
			vkCmdSetViewport(command_buffers[1], 0, 1, &viewport);
			// all commands begin with vkCmd
		}
		vkEndCommandBuffer(command_buffers[1]); // compiles the command buffer
	}

	// submit command buffer 0
	{
		VkSubmitInfo submit_info{};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &command_buffers[0];
		submit_info.signalSemaphoreCount = 1; // signals 1 semaphore at the end of the command buffer
		submit_info.pSignalSemaphores = &semaphore; // the semaphore to signal.
		//vkQueueSubmit(r.queue(), 1, &submit_info, fence); // signals the fence when finished
		vkQueueSubmit(r.queue(), 1, &submit_info, VK_NULL_HANDLE); // no fence	
	}

	// submit command buffer 1 - waits, at the fragment shader stage, for semaphore to be signaled
	{
		VkPipelineStageFlags flags[] = { VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT }; // use VK_PIPELINE_STAGE_ALL_COMMANDS_BIT to wait before executing any command.
		VkSubmitInfo submit_info{};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &command_buffers[1];
		submit_info.waitSemaphoreCount = 1;
		submit_info.pWaitSemaphores = &semaphore;
		submit_info.pWaitDstStageMask = flags; // wait at these pipeline stages
		//vkQueueSubmit(r.queue(), 1, &submit_info, fence); // signals the fence when finished
		vkQueueSubmit(r.queue(), 1, &submit_info, VK_NULL_HANDLE); // no fence	
	}


	//auto ret = vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX); // wait indefinitely for all fences
																		// can return VK_SUCCESS(0) or VK_TIMEOUT(2) when timeout is too small
	
    // poor man's synchro, wait until everything is complete in the queue.
    // But if we dont wait at all, it will crash because we are destroying
    // the queue when all its commands are not finished.
    vkQueueWaitIdle( r.queue() ); 
	



    // note: destroying the command pool destroys the commands.
    vkDestroyCommandPool( r.device(), command_pool, nullptr );
	vkDestroyFence(device, fence, nullptr);
	vkDestroySemaphore(device, semaphore, nullptr);

    return 0;
}
