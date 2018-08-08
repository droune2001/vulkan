#ifndef _VULKAN_INITIALIZERS_2018_08_08_H_
#define _VULKAN_INITIALIZERS_2018_08_08_H_

namespace vk
{
namespace init
{

namespace image
{
    VkImageViewCreateInfo image_view_create_info();
}

namespace transfer
{
    VkBufferImageCopy buffer_image_copy();
} // transfer

namespace pipeline
{
    VkViewport viewport();
    VkPipelineRasterizationStateCreateInfo raster_state_create_info();
    VkStencilOpState stencil_op_state_NOP();
    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info();
    VkPipelineColorBlendAttachmentState color_blend_attachment_state_NO_BLEND();
    VkPipelineColorBlendStateCreateInfo color_blend_state_create_info();
    VkPipelineMultisampleStateCreateInfo multisample_state_create_info_NO_MSAA();
} // pipeline
} // init
} // vk

#endif // _VULKAN_INITIALIZERS_2018_08_08_H_
