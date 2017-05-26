#include "Renderer.h"

int main( int argc, char **argv )
{
    Renderer r;

    VkCommandPool command_pool;
    VkCommandPoolCreateInfo pool_create_info{};
    pool_create_info.sType              = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_create_info.queueFamilyIndex   = r.familyIndex();
    pool_create_info.flags              = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    vkCreateCommandPool( r.device(), &pool_create_info, nullptr, &command_pool );


    VkCommandBuffer command_buffer;
    VkCommandBufferAllocateInfo command_buffer_allocate_info{};
    command_buffer_allocate_info.sType                  = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.commandPool            = command_pool;
    command_buffer_allocate_info.commandBufferCount     = 1;
    command_buffer_allocate_info.level                  = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    vkAllocateCommandBuffers( r.device(), &command_buffer_allocate_info, &command_buffer );

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer( command_buffer, &begin_info );
    {
        // ... fill with commands
        VkViewport viewport{};
        viewport.maxDepth = 1.0f;
        viewport.minDepth = 0.0f;
        viewport.width = 512;
        viewport.height = 512;
        viewport.x = 0;
        viewport.y = 0;
        vkCmdSetViewport( command_buffer, 0, 1, &viewport );
    }
    vkEndCommandBuffer( command_buffer ); // compiles the command buffer

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;
    vkQueueSubmit( r.queue(), 1, &submit_info, VK_NULL_HANDLE ); // no fence

    // fence = synchro between GPU submit and the CPU
    // TODO: tuto 4 26:15

    // poor man's synchro, wait until everything is complete in the queue.
    // But if we dont wait at all, it will crash because we are destroying
    // the queue when all its commands are not finished.
    vkQueueWaitIdle( r.queue() ); 

    // note: destroying the command pool destroys the commands.
    vkDestroyCommandPool( r.device(), command_pool, nullptr );

    return 0;
}
