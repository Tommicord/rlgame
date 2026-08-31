#include "rlgame.base/cvulkan/cvulkan_command_buffer.h"
#include "rlgame.base/cvulkan/cvulkan_device.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"

#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <vulkan/vulkan.h>

R_CVULKAN_API enum R_CVulkan_Error
r_cvulkan_new_command_buffer (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    const struct R_CVulkan_Device*  pDevice,
    VkCommandPool                   pool,
    VkCommandBufferLevel            level)
{
    R_CVULKAN_ASSERT (pCommandBuffer);
    R_CVULKAN_ASSERT (pDevice);
    R_CVULKAN_ASSERT (pool);

    const VkDevice device = r_cvulkan_device_get_logical_device (pDevice);
    pCommandBuffer->pool = pool;
    pCommandBuffer->device = device;

    VkCommandBufferAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = pool;
    allocInfo.level = level;
    allocInfo.commandBufferCount = 1;

    VkResult result = vkAllocateCommandBuffers (device, &allocInfo, &pCommandBuffer->handle);
    if (result != VK_SUCCESS)
    {
        return R_CVULKAN_ERROR_FAILED;
    }
    return R_CVULKAN_OK;
}

R_CVULKAN_API void
r_cvulkan_delete_command_buffer (struct R_CVulkan_CommandBuffer* pCommandBuffer)
{
    R_CVULKAN_ASSERT (pCommandBuffer);
    vkFreeCommandBuffers (pCommandBuffer->device, pCommandBuffer->pool, 1, &pCommandBuffer->handle);
#if defined(R_CVULKAN_DEBUG)
    pCommandBuffer->pool = VK_NULL_HANDLE;
    pCommandBuffer->device = VK_NULL_HANDLE;
#endif
}

R_CVULKAN_API enum R_CVulkan_Error
r_cvulkan_begin_command_buffer (
    struct R_CVulkan_CommandBuffer*       pCommandBuffer,
    VkCommandBufferUsageFlags             flags,
    const VkCommandBufferInheritanceInfo* pInheritanceInfo)
{
    R_CVULKAN_ASSERT (pCommandBuffer);

    VkCommandBufferBeginInfo beginInfo = {0};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = flags;
    beginInfo.pInheritanceInfo = pInheritanceInfo;

    VkResult result = vkBeginCommandBuffer (pCommandBuffer->handle, &beginInfo);
    if (result != VK_SUCCESS)
    {
        return r_cvulkan_result_to_error (result);
    }
    return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkan_Error
r_cvulkan_end_command_buffer (struct R_CVulkan_CommandBuffer* pCommandBuffer)
{
    R_CVULKAN_ASSERT (pCommandBuffer);

    VkResult result = vkEndCommandBuffer (pCommandBuffer->handle);
    if (result != VK_SUCCESS)
    {
        return r_cvulkan_result_to_error (result);
    }
    return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkan_Error
r_cvulkan_reset_command_buffer (struct R_CVulkan_CommandBuffer* pCommandBuffer, VkCommandBufferResetFlags flags)
{
    R_CVULKAN_ASSERT (pCommandBuffer);
    VkResult result = vkResetCommandBuffer (pCommandBuffer->handle, flags);
    if (result != VK_SUCCESS)
    {
        return r_cvulkan_result_to_error (result);
    }
    return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkan_Error
r_cvulkan_command_buffer_bind_pipeline (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkPipelineBindPoint             pipelineBindPoint,
    VkPipeline                      pipeline)
{
    R_CVULKAN_ASSERT (pCommandBuffer);
    vkCmdBindPipeline (pCommandBuffer->handle, pipelineBindPoint, pipeline);
    return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkan_Error
r_cvulkan_command_buffer_bind_vertex_buffer (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    uint32_t                        firstBinding,
    uint32_t                        bindingCount,
    const VkBuffer*                 pBuffers,
    const VkDeviceSize*             pOffsets)
{
    R_CVULKAN_ASSERT (pCommandBuffer);
    vkCmdBindVertexBuffers (pCommandBuffer->handle, firstBinding, bindingCount, pBuffers, pOffsets);
    return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkan_Error
r_cvulkan_command_buffer_bind_index_buffer (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkBuffer                        buffer,
    VkDeviceSize                    offset,
    VkIndexType                     indexType)
{
    R_CVULKAN_ASSERT (pCommandBuffer);
    vkCmdBindIndexBuffer (pCommandBuffer->handle, buffer, offset, indexType);
    return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkan_Error
r_cvulkan_command_buffer_bind_descriptor_sets (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkPipelineBindPoint             pipelineBindPoint,
    VkPipelineLayout                layout,
    uint32_t                        firstSet,
    uint32_t                        descriptorSetCount,
    const VkDescriptorSet*          pDescriptorSets,
    uint32_t                        dynamicOffsetCount,
    const uint32_t*                 pDynamicOffsets)
{
    R_CVULKAN_ASSERT (pCommandBuffer);
    vkCmdBindDescriptorSets (
        pCommandBuffer->handle,
        pipelineBindPoint,
        layout,
        firstSet,
        descriptorSetCount,
        pDescriptorSets,
        dynamicOffsetCount,
        pDynamicOffsets);
    return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkan_Error
r_cvulkan_command_buffer_draw (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    uint32_t                        vertexCount,
    uint32_t                        instanceCount,
    uint32_t                        firstVertex,
    uint32_t                        firstInstance)
{
    R_CVULKAN_ASSERT (pCommandBuffer);
    vkCmdDraw (pCommandBuffer->handle, vertexCount, instanceCount, firstVertex, firstInstance);
    return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkan_Error
r_cvulkan_command_buffer_draw_indexed (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    uint32_t                        indexCount,
    uint32_t                        instanceCount,
    uint32_t                        firstIndex,
    int32_t                         vertexOffset,
    uint32_t                        firstInstance)
{
    R_CVULKAN_ASSERT (pCommandBuffer);
    vkCmdDrawIndexed (
        pCommandBuffer->handle,
        indexCount,
        instanceCount,
        firstIndex,
        vertexOffset,
        firstInstance);
    return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkan_Error
r_cvulkan_command_buffer_set_viewport (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    uint32_t                        firstViewport,
    uint32_t                        viewportCount,
    const VkViewport*               pViewports)
{
    R_CVULKAN_ASSERT (pCommandBuffer);
    vkCmdSetViewport (pCommandBuffer->handle, firstViewport, viewportCount, pViewports);
    return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkan_Error
r_cvulkan_command_buffer_set_scissor (
    const struct R_CVulkan_CommandBuffer* pCommandBuffer,
    uint32_t                              firstScissor,
    uint32_t                              scissorCount,
    const VkRect2D*                       pScissors)
{
    R_CVULKAN_ASSERT (pCommandBuffer);
    vkCmdSetScissor (pCommandBuffer->handle, firstScissor, scissorCount, pScissors);
    return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkan_Error
r_cvulkan_command_buffer_set_blend_constants (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    const float                     blendConstants[4])
{
    R_CVULKAN_ASSERT (pCommandBuffer);
    vkCmdSetBlendConstants (pCommandBuffer->handle, blendConstants);
    return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_CommandBuffer_SetDepthBounds (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    float                           minDepth,
    float                           maxDepth)
{
    R_CVULKAN_ASSERT (pCommandBuffer);
    vkCmdSetDepthBounds (pCommandBuffer->handle, minDepth, maxDepth);
    return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_CommandBuffer_SetStencilCompareMask (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkStencilFaceFlags              faceMask,
    uint32_t                        compareMask)
{
    R_CVULKAN_ASSERT (pCommandBuffer);
    vkCmdSetStencilCompareMask (pCommandBuffer->handle, faceMask, compareMask);
    return R_CVULKAN_OK;
}

enum R_CVulkan_Error
R_CVulkan_CommandBuffer_SetStencilWriteMask (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkStencilFaceFlags              faceMask,
    uint32_t                        writeMask)
{
    R_CVULKAN_ASSERT (pCommandBuffer);
    vkCmdSetStencilWriteMask (pCommandBuffer->handle, faceMask, writeMask);
    return R_CVULKAN_OK;
}

enum R_CVulkan_Error
r_cvulkan_command_buffer_set_stencil_reference (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkStencilFaceFlags              faceMask,
    uint32_t                        reference)
{
    R_CVULKAN_ASSERT (pCommandBuffer);
    vkCmdSetStencilReference (pCommandBuffer->handle, faceMask, reference);
    return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkan_Error
r_cvulkan_command_buffer_copy_buffer (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkBuffer                        srcBuffer,
    VkBuffer                        dstBuffer,
    const VkBufferCopy*             pRegion)
{
    R_CVULKAN_ASSERT (pCommandBuffer);
    vkCmdCopyBuffer (pCommandBuffer->handle, srcBuffer, dstBuffer, 1, pRegion);
    return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkan_Error
r_cvulkan_command_buffer_copy_buffer_to_image (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkBuffer                        srcBuffer,
    VkImage                         dstImage,
    VkImageLayout                   dstLayout,
    const VkBufferImageCopy*        pRegion)
{
    R_CVULKAN_ASSERT (pCommandBuffer);
    vkCmdCopyBufferToImage (pCommandBuffer->handle, srcBuffer, dstImage, dstLayout, 1, pRegion);
    return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkan_Error
r_cvulkan_command_buffer_copy_image_to_buffer (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkImage                         srcImage,
    VkImageLayout                   srcLayout,
    VkBuffer                        dstBuffer,
    const VkBufferImageCopy*        pRegion)
{
    R_CVULKAN_ASSERT (pCommandBuffer);
    vkCmdCopyImageToBuffer (pCommandBuffer->handle, srcImage, srcLayout, dstBuffer, 1, pRegion);
    return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkan_Error
r_cvulkan_command_buffer_pipeline_barrier (
    struct R_CVulkan_CommandBuffer*             pCommandBuffer,
    const struct r_cvulkan_pipeline_barrier_info* pBarrierInfo)
{
    R_CVULKAN_ASSERT (pBarrierInfo);
    vkCmdPipelineBarrier (
        pCommandBuffer->handle,
        pBarrierInfo->srcStageMask,
        pBarrierInfo->dstStageMask,
        pBarrierInfo->dependencyFlags,
        pBarrierInfo->memoryBarrierCount,
        pBarrierInfo->pMemoryBarriers,
        pBarrierInfo->bufferMemoryBarrierCount,
        pBarrierInfo->pBufferMemoryBarriers,
        pBarrierInfo->imageMemoryBarrierCount,
        pBarrierInfo->pImageMemoryBarriers);
    return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkan_Error
r_cvulkan_command_buffer_begin_render_pass (
    struct R_CVulkan_CommandBuffer*             pCommandBuffer,
    const struct r_cvulkan_render_pass_begin_info* pRenderPassInfo)
{
    R_CVULKAN_ASSERT (pRenderPassInfo);
    VkRenderPassBeginInfo beginInfo = {0};
    beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    beginInfo.renderPass = pRenderPassInfo->renderPass;
    beginInfo.framebuffer = pRenderPassInfo->framebuffer;
    beginInfo.renderArea = *pRenderPassInfo->pRenderArea;
    beginInfo.clearValueCount = pRenderPassInfo->clearValueCount;
    beginInfo.pClearValues = pRenderPassInfo->pClearValues;

    vkCmdBeginRenderPass (pCommandBuffer->handle, &beginInfo, pRenderPassInfo->contents);
    return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkan_Error
r_cvulkan_command_buffer_end_render_pass (struct R_CVulkan_CommandBuffer* pCommandBuffer)
{
    R_CVULKAN_ASSERT (pCommandBuffer);
    vkCmdEndRenderPass (pCommandBuffer->handle);
    return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkan_Error
r_cvulkan_command_buffer_begin_rendering (
    struct R_CVulkan_CommandBuffer*              pCommandBuffer,
    const struct r_cvulkan_dynamic_rendering_info* pRenderingInfo)
{
    R_CVULKAN_ASSERT (pCommandBuffer);
    R_CVULKAN_ASSERT (pRenderingInfo);
    VkRenderingAttachmentInfoKHR* pColorAttachments = NULL;
    if (pRenderingInfo->colorAttachmentCount > 0)
    {
        pColorAttachments = (VkRenderingAttachmentInfoKHR*)r_cstl_heap_alloc (
            sizeof (VkRenderingAttachmentInfoKHR) * pRenderingInfo->colorAttachmentCount);
        if (!pColorAttachments)
        {
            return R_CVULKAN_ERROR_OUT_OF_MEMORY;
        }
        memset (
            pColorAttachments,
            0,
            sizeof (VkRenderingAttachmentInfoKHR) * pRenderingInfo->colorAttachmentCount);

        for (uint32_t i = 0; i < pRenderingInfo->colorAttachmentCount; ++i)
        {
            const struct r_cvulkan_dynamic_rendering_attachment_info* pSrc
                = &pRenderingInfo->pColorAttachments[i];
            pColorAttachments[i].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
            pColorAttachments[i].imageView = pSrc->imageView;
            pColorAttachments[i].imageLayout = pSrc->imageLayout;
            pColorAttachments[i].resolveMode = pSrc->resolveMode;
            pColorAttachments[i].resolveImageView = pSrc->resolveImageView;
            pColorAttachments[i].resolveImageLayout = pSrc->resolveImageLayout;
            pColorAttachments[i].loadOp = pSrc->loadOp;
            pColorAttachments[i].storeOp = pSrc->storeOp;
            pColorAttachments[i].clearValue = pSrc->clearValue;
        }
    }

    VkRenderingAttachmentInfoKHR depthAttachment = {0};
    if (pRenderingInfo->pDepthAttachment)
    {
        depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        depthAttachment.imageView = pRenderingInfo->pDepthAttachment->imageView;
        depthAttachment.imageLayout = pRenderingInfo->pDepthAttachment->imageLayout;
        depthAttachment.resolveMode = pRenderingInfo->pDepthAttachment->resolveMode;
        depthAttachment.resolveImageView = pRenderingInfo->pDepthAttachment->resolveImageView;
        depthAttachment.resolveImageLayout = pRenderingInfo->pDepthAttachment->resolveImageLayout;
        depthAttachment.loadOp = pRenderingInfo->pDepthAttachment->loadOp;
        depthAttachment.storeOp = pRenderingInfo->pDepthAttachment->storeOp;
        depthAttachment.clearValue = pRenderingInfo->pDepthAttachment->clearValue;
    }

    VkRenderingAttachmentInfoKHR stencilAttachment = {0};
    if (pRenderingInfo->pStencilAttachment)
    {
        stencilAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        stencilAttachment.imageView = pRenderingInfo->pStencilAttachment->imageView;
        stencilAttachment.imageLayout = pRenderingInfo->pStencilAttachment->imageLayout;
        stencilAttachment.resolveMode = pRenderingInfo->pStencilAttachment->resolveMode;
        stencilAttachment.resolveImageView = pRenderingInfo->pStencilAttachment->resolveImageView;
        stencilAttachment.resolveImageLayout = pRenderingInfo->pStencilAttachment->resolveImageLayout;
        stencilAttachment.loadOp = pRenderingInfo->pStencilAttachment->loadOp;
        stencilAttachment.storeOp = pRenderingInfo->pStencilAttachment->storeOp;
        stencilAttachment.clearValue = pRenderingInfo->pStencilAttachment->clearValue;
    }

    VkRenderingInfoKHR renderingInfo = {0};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
    renderingInfo.flags = pRenderingInfo->flags;
    renderingInfo.renderArea = (VkRect2D){0, 0, 0, 0};
    renderingInfo.layerCount = 1;
    renderingInfo.viewMask = pRenderingInfo->viewMask;
    renderingInfo.colorAttachmentCount = pRenderingInfo->colorAttachmentCount;
    renderingInfo.pColorAttachments = pColorAttachments;
    renderingInfo.pDepthAttachment = pRenderingInfo->pDepthAttachment ? &depthAttachment : NULL;
    renderingInfo.pStencilAttachment = pRenderingInfo->pStencilAttachment ? &stencilAttachment : NULL;

    vkCmdBeginRendering (pCommandBuffer->handle, &renderingInfo);

    if (pColorAttachments)
    {
        r_cstl_heap_free (pColorAttachments);
    }

    return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkan_Error
r_cvulkan_command_buffer_end_rendering (struct R_CVulkan_CommandBuffer* pCommandBuffer)
{
    R_CVULKAN_ASSERT (pCommandBuffer);
    vkCmdEndRendering (pCommandBuffer->handle);
    return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkan_Error
r_cvulkan_command_buffer_next_subpass (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkSubpassContents               contents)
{
    R_CVULKAN_ASSERT (pCommandBuffer);
    vkCmdNextSubpass (pCommandBuffer->handle, contents);
    return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkan_Error
r_cvulkan_command_buffer_push_constants (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkPipelineLayout                layout,
    VkShaderStageFlags              stageFlags,
    uint32_t                        offset,
    uint32_t                        size,
    const void*                     pValues)
{
    R_CVULKAN_ASSERT (pCommandBuffer);
    vkCmdPushConstants (pCommandBuffer->handle, layout, stageFlags, offset, size, pValues);
    return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_CommandBuffer_Dispatch (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    uint32_t                        groupCountX,
    uint32_t                        groupCountY,
    uint32_t                        groupCountZ)
{
    R_CVULKAN_ASSERT (pCommandBuffer);
    vkCmdDispatch (pCommandBuffer->handle, groupCountX, groupCountY, groupCountZ);
    return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkan_Error
r_cvulkan_command_buffer_dispatch_indirect (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkBuffer                        buffer,
    VkDeviceSize                    offset)
{
    R_CVULKAN_ASSERT (pCommandBuffer);
    vkCmdDispatchIndirect (pCommandBuffer->handle, buffer, offset);
    return R_CVULKAN_OK;
}

R_CVULKAN_API VkCommandBuffer
r_cvulkan_command_buffer_get_handle (const struct R_CVulkan_CommandBuffer* pCommandBuffer)
{
    R_CVULKAN_ASSERT (pCommandBuffer);
    return pCommandBuffer->handle;
}

R_CVULKAN_API VkCommandPool
r_cvulkan_command_buffer_get_pool (const struct R_CVulkan_CommandBuffer* pCommandBuffer)
{
    R_CVULKAN_ASSERT (pCommandBuffer);
    return pCommandBuffer->pool;
}

R_CVULKAN_API VkDevice
r_cvulkan_command_buffer_get_device (const struct R_CVulkan_CommandBuffer* pCommandBuffer)
{
    R_CVULKAN_ASSERT (pCommandBuffer);
    return pCommandBuffer->device;
}