#include "build_options.h"
#include "platform.h"

#include "initializers.h"


namespace vk
{
namespace init
{
//
// IMAGE
//
namespace image
{
VkImageViewCreateInfo image_view_create_info()
{
    VkImageViewCreateInfo texture_image_view_create_info = {};
    texture_image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    texture_image_view_create_info.image = VK_NULL_HANDLE; // <-- fill this
    texture_image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    texture_image_view_create_info.format = VK_FORMAT_UNDEFINED; // <-- fill this
    texture_image_view_create_info.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
    texture_image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    texture_image_view_create_info.subresourceRange.baseMipLevel = 0;
    texture_image_view_create_info.subresourceRange.levelCount = 1;
    texture_image_view_create_info.subresourceRange.baseArrayLayer = 0;
    texture_image_view_create_info.subresourceRange.layerCount = 1;

    return texture_image_view_create_info;
}
} // image

//
// TRANSFER
//
namespace transfer
{
VkBufferImageCopy buffer_image_copy()
{
    VkBufferImageCopy image_copy_region = {};
    image_copy_region.bufferOffset = 0;
    image_copy_region.bufferRowLength = 0;
    image_copy_region.bufferImageHeight = 0;
    image_copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_copy_region.imageSubresource.mipLevel = 0;
    image_copy_region.imageSubresource.baseArrayLayer = 0;
    image_copy_region.imageSubresource.layerCount = 1;
    image_copy_region.imageOffset = {0,0,0};
    image_copy_region.imageExtent = {0,0,0};

    return image_copy_region;
}
} // transfer

//
// PIPELINE
//
namespace pipeline
{

VkPipelineShaderStageCreateInfo shader_stage_create_info(VkShaderModule module, VkShaderStageFlagBits stage_flags)
{
    VkPipelineShaderStageCreateInfo info = {};

    info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.stage = stage_flags;
    info.module = module;
    info.pName = "main";
    info.pSpecializationInfo = nullptr;

    return info;
}

VkStencilOpState stencil_op_state_NOP()
{
    VkStencilOpState noOP_stencil_state = {};
    noOP_stencil_state.failOp = VK_STENCIL_OP_KEEP;
    noOP_stencil_state.passOp = VK_STENCIL_OP_KEEP;
    noOP_stencil_state.depthFailOp = VK_STENCIL_OP_KEEP;
    noOP_stencil_state.compareOp = VK_COMPARE_OP_ALWAYS;
    noOP_stencil_state.compareMask = 0;
    noOP_stencil_state.writeMask = 0;
    noOP_stencil_state.reference = 0;

    return noOP_stencil_state;
}

VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info()
{
    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info = {};
    depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state_create_info.depthTestEnable = VK_TRUE;
    depth_stencil_state_create_info.depthWriteEnable = VK_TRUE;
    depth_stencil_state_create_info.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state_create_info.stencilTestEnable = VK_FALSE;
    depth_stencil_state_create_info.front = stencil_op_state_NOP();
    depth_stencil_state_create_info.back = stencil_op_state_NOP();
    depth_stencil_state_create_info.minDepthBounds = 0;
    depth_stencil_state_create_info.maxDepthBounds = 0;

    return depth_stencil_state_create_info;
}

VkPipelineColorBlendAttachmentState color_blend_attachment_state_NO_BLEND()
{
    VkPipelineColorBlendAttachmentState color_blend_attachment_state = {};
    color_blend_attachment_state.blendEnable = VK_FALSE;
    color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
    color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
    color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment_state.colorWriteMask = 0xf; // all components.

    return color_blend_attachment_state;
}

VkPipelineColorBlendStateCreateInfo color_blend_state_create_info()
{
    VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = {};
    color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state_create_info.logicOpEnable = VK_FALSE;
    color_blend_state_create_info.logicOp = VK_LOGIC_OP_CLEAR;
    color_blend_state_create_info.attachmentCount = 1;
    color_blend_state_create_info.pAttachments = nullptr; // <-- fill this
    color_blend_state_create_info.blendConstants[0] = 0.0;
    color_blend_state_create_info.blendConstants[1] = 0.0;
    color_blend_state_create_info.blendConstants[2] = 0.0;
    color_blend_state_create_info.blendConstants[3] = 0.0;

    return color_blend_state_create_info;
}

VkPipelineMultisampleStateCreateInfo multisample_state_create_info_NO_MSAA()
{
    VkPipelineMultisampleStateCreateInfo multisample_state_create_info = {};
    multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample_state_create_info.sampleShadingEnable = VK_FALSE;
    multisample_state_create_info.minSampleShading = 0;
    multisample_state_create_info.pSampleMask = nullptr;
    multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
    multisample_state_create_info.alphaToOneEnable = VK_FALSE;

    return multisample_state_create_info;
}

VkPipelineRasterizationStateCreateInfo raster_state_create_info()
{
    VkPipelineRasterizationStateCreateInfo raster_state_create_info = {};
    raster_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster_state_create_info.depthClampEnable = VK_FALSE;
    raster_state_create_info.rasterizerDiscardEnable = VK_FALSE;
    raster_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
    raster_state_create_info.cullMode = VK_CULL_MODE_NONE;
    raster_state_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    raster_state_create_info.depthBiasEnable = VK_FALSE;
    raster_state_create_info.depthBiasConstantFactor = 0;
    raster_state_create_info.depthBiasClamp = 0;
    raster_state_create_info.depthBiasSlopeFactor = 0;
    raster_state_create_info.lineWidth = 1;

    return raster_state_create_info;
}

VkViewport viewport()
{
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = 512.0f;
    viewport.height = 512.0f;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    return viewport;
}

} // pipeline
} // init
} // vk

//
