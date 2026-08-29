#include "rlgame.base/cvulkan/cvulkan_command_buffer.h"
#include "rlgame.base/cvulkan/cvulkan_device.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"

#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <vulkan/vulkan.h>

enum R_CVulkanError
R_CVulkan_NewCommandBuffer (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    const struct R_CVulkan_Device*  pDevice,
    VkCommandPool                   pool,
    VkCommandBufferLevel            level)
{
    R_CVULKAN_ASSERT (pCommandBuffer);
    R_CVULKAN_ASSERT (pDevice);
    R_CVULKAN_ASSERT (pool != VK_NULL_HANDLE);

#if defined(R_CVULKAN_DEBUG)
    if (!pCommandBuffer || !pDevice || pool == VK_NULL_HANDLE)
    {
        return R_CVULKAN_ERROR_NULL_POINTER;
    }
    if (!R_CVulkan_DeviceIsInitialized (pDevice))
    {
        return R_CVULKAN_ERROR_NOT_INITIALIZED;
    }
#endif
    VkDevice device = R_CVulkan_DeviceGetLogicalDevice (pDevice);
    pCommandBuffer->handle = VK_NULL_HANDLE;
    pCommandBuffer->pool = pool;
    pCommandBuffer->device = device;
#if defined(R_CVULKAN_DEBUG)
    pCommandBuffer->record = 0;
    
#endif

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

#if defined(R_CVULKAN_DEBUG)
    
#endif
    return R_CVULKAN_OK;
}

void
R_CVulkan_DeleteCommandBuffer (struct R_CVulkan_CommandBuffer* pCommandBuffer)
{
    R_CVULKAN_ASSERT (pCommandBuffer);

#if defined(R_CVULKAN_DEBUG)
    if (!pCommandBuffer)
    {
        return;
    }
#endif

    if (pCommandBuffer->handle != VK_NULL_HANDLE && pCommandBuffer->pool != VK_NULL_HANDLE)
    {
        vkFreeCommandBuffers (pCommandBuffer->device, pCommandBuffer->pool, 1, &pCommandBuffer->handle);
        pCommandBuffer->handle = VK_NULL_HANDLE;
    }
#if defined(R_CVULKAN_DEBUG)
    pCommandBuffer->record = 0;
    
#endif
    pCommandBuffer->pool = VK_NULL_HANDLE;
    pCommandBuffer->device = VK_NULL_HANDLE;
}

enum R_CVulkanError
R_CVulkan_BeginCommandBuffer (
    struct R_CVulkan_CommandBuffer*       pCommandBuffer,
    VkCommandBufferUsageFlags             flags,
    const VkCommandBufferInheritanceInfo* pInheritanceInfo)
{
    R_CVULKAN_VALIDATE_PARAM (pCommandBuffer);
    R_CVULKAN_VALIDATE_PARAM_BOOTED (pCommandBuffer);
#if defined(R_CVULKAN_DEBUG)
    if (pCommandBuffer->record)
    {
        return R_CVULKAN_ERROR_FAILED;
    }
#endif

    VkCommandBufferBeginInfo beginInfo = {0};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = flags;
    beginInfo.pInheritanceInfo = pInheritanceInfo;

    VkResult result = vkBeginCommandBuffer (pCommandBuffer->handle, &beginInfo);
    if (result != VK_SUCCESS)
    {
        return R_CVulkan_ResultToError (result);
    }
#if defined(R_CVULKAN_DEBUG)
    pCommandBuffer->record = true;
#endif
    return R_CVULKAN_OK;
}

enum R_CVulkanError
R_CVulkan_EndCommandBuffer (struct R_CVulkan_CommandBuffer* pCommandBuffer)
{
    R_CVULKAN_VALIDATE_PARAM (pCommandBuffer);
    R_CVULKAN_VALIDATE_PARAM_BOOTED (pCommandBuffer);
#if defined(R_CVULKAN_DEBUG)
    if (!pCommandBuffer->record)
    {
        return R_CVULKAN_ERROR_FAILED;
    }
#endif
    VkResult result = vkEndCommandBuffer (pCommandBuffer->handle);
    if (result != VK_SUCCESS)
    {
        return R_CVulkan_ResultToError (result);
    }
#if defined(R_CVULKAN_DEBUG)
    pCommandBuffer->record = false;
#endif
    return R_CVULKAN_OK;
}

enum R_CVulkanError
R_CVulkan_ResetCommandBuffer (struct R_CVulkan_CommandBuffer* pCommandBuffer, VkCommandBufferResetFlags flags)
{
    R_CVULKAN_VALIDATE_PARAM (pCommandBuffer);
    R_CVULKAN_VALIDATE_PARAM_BOOTED (pCommandBuffer);
    VkResult result = vkResetCommandBuffer (pCommandBuffer->handle, flags);
    if (result != VK_SUCCESS)
    {
        return R_CVulkan_ResultToError (result);
    }
#if defined(R_CVULKAN_DEBUG)
    pCommandBuffer->record = false;
#endif
    return R_CVULKAN_OK;
}

enum R_CVulkanError
R_CVulkan_CommandBufferBindPipeline (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkPipelineBindPoint             pipelineBindPoint,
    VkPipeline                      pipeline)
{
    R_CVULKAN_VALIDATE_COMMAND_BUFFER (pCommandBuffer);
    vkCmdBindPipeline (pCommandBuffer->handle, pipelineBindPoint, pipeline);
    return R_CVULKAN_OK;
}

enum R_CVulkanError
R_CVulkan_CommandBufferBindVertexBuffer (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    uint32_t                        firstBinding,
    uint32_t                        bindingCount,
    const VkBuffer*                 pBuffers,
    const VkDeviceSize*             pOffsets)
{
    R_CVULKAN_VALIDATE_COMMAND_BUFFER (pCommandBuffer);
    vkCmdBindVertexBuffers (pCommandBuffer->handle, firstBinding, bindingCount, pBuffers, pOffsets);
    return R_CVULKAN_OK;
}

enum R_CVulkanError
R_CVulkan_CommandBufferBindIndexBuffer (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkBuffer                        buffer,
    VkDeviceSize                    offset,
    VkIndexType                     indexType)
{
    R_CVULKAN_VALIDATE_COMMAND_BUFFER (pCommandBuffer);
    vkCmdBindIndexBuffer (pCommandBuffer->handle, buffer, offset, indexType);
    return R_CVULKAN_OK;
}

enum R_CVulkanError
R_CVulkan_CommandBufferBindDescriptorSets (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkPipelineBindPoint             pipelineBindPoint,
    VkPipelineLayout                layout,
    uint32_t                        firstSet,
    uint32_t                        descriptorSetCount,
    const VkDescriptorSet*          pDescriptorSets,
    uint32_t                        dynamicOffsetCount,
    const uint32_t*                 pDynamicOffsets)
{
    R_CVULKAN_VALIDATE_COMMAND_BUFFER (pCommandBuffer);
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

enum R_CVulkanError
R_CVulkan_CommandBufferDraw (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    uint32_t                        vertexCount,
    uint32_t                        instanceCount,
    uint32_t                        firstVertex,
    uint32_t                        firstInstance)
{
    R_CVULKAN_VALIDATE_COMMAND_BUFFER (pCommandBuffer);
    vkCmdDraw (pCommandBuffer->handle, vertexCount, instanceCount, firstVertex, firstInstance);
    return R_CVULKAN_OK;
}

enum R_CVulkanError
R_CVulkan_CommandBufferDrawIndexed (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    uint32_t                        indexCount,
    uint32_t                        instanceCount,
    uint32_t                        firstIndex,
    int32_t                         vertexOffset,
    uint32_t                        firstInstance)
{
    R_CVULKAN_VALIDATE_COMMAND_BUFFER (pCommandBuffer);
    vkCmdDrawIndexed (
        pCommandBuffer->handle,
        indexCount,
        instanceCount,
        firstIndex,
        vertexOffset,
        firstInstance);
    return R_CVULKAN_OK;
}

enum R_CVulkanError
R_CVulkan_CommandBufferSetViewport (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    uint32_t                        firstViewport,
    uint32_t                        viewportCount,
    const VkViewport*               pViewports)
{
    R_CVULKAN_VALIDATE_COMMAND_BUFFER (pCommandBuffer);
    vkCmdSetViewport (pCommandBuffer->handle, firstViewport, viewportCount, pViewports);
    return R_CVULKAN_OK;
}

enum R_CVulkanError
R_CVulkan_CommandBufferSetScissor (
    const struct R_CVulkan_CommandBuffer* pCommandBuffer,
    uint32_t                              firstScissor,
    uint32_t                              scissorCount,
    const VkRect2D*                       pScissors)
{
    R_CVULKAN_VALIDATE_COMMAND_BUFFER (pCommandBuffer);
    vkCmdSetScissor (pCommandBuffer->handle, firstScissor, scissorCount, pScissors);
    return R_CVULKAN_OK;
}

enum R_CVulkanError
R_CVulkan_CommandBufferSetBlendConstants (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    const float                     blendConstants[4])
{
    R_CVULKAN_VALIDATE_COMMAND_BUFFER (pCommandBuffer);
    vkCmdSetBlendConstants (pCommandBuffer->handle, blendConstants);
    return R_CVULKAN_OK;
}

enum R_CVulkanError
R_CVulkan_CommandBuffer_SetDepthBounds (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    float                           minDepth,
    float                           maxDepth)
{
    R_CVULKAN_VALIDATE_COMMAND_BUFFER (pCommandBuffer);
    vkCmdSetDepthBounds (pCommandBuffer->handle, minDepth, maxDepth);
    return R_CVULKAN_OK;
}

enum R_CVulkanError
R_CVulkan_CommandBuffer_SetStencilCompareMask (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkStencilFaceFlags              faceMask,
    uint32_t                        compareMask)
{
    R_CVULKAN_VALIDATE_COMMAND_BUFFER (pCommandBuffer);
    vkCmdSetStencilCompareMask (pCommandBuffer->handle, faceMask, compareMask);
    return R_CVULKAN_OK;
}

enum R_CVulkanError
R_CVulkan_CommandBuffer_SetStencilWriteMask (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkStencilFaceFlags              faceMask,
    uint32_t                        writeMask)
{
    R_CVULKAN_VALIDATE_COMMAND_BUFFER (pCommandBuffer);
    vkCmdSetStencilWriteMask (pCommandBuffer->handle, faceMask, writeMask);
    return R_CVULKAN_OK;
}

enum R_CVulkanError
R_CVulkan_CommandBufferSetStencilReference (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkStencilFaceFlags              faceMask,
    uint32_t                        reference)
{
    R_CVULKAN_VALIDATE_COMMAND_BUFFER (pCommandBuffer);
    vkCmdSetStencilReference (pCommandBuffer->handle, faceMask, reference);
    return R_CVULKAN_OK;
}

enum R_CVulkanError
R_CVulkan_CommandBufferCopyBuffer (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkBuffer                        srcBuffer,
    VkBuffer                        dstBuffer,
    const VkBufferCopy*             pRegion)
{
    R_CVULKAN_VALIDATE_COMMAND_BUFFER (pCommandBuffer);
    vkCmdCopyBuffer (pCommandBuffer->handle, srcBuffer, dstBuffer, 1, pRegion);
    return R_CVULKAN_OK;
}

enum R_CVulkanError
R_CVulkan_CommandBufferCopyBufferToImage (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkBuffer                        srcBuffer,
    VkImage                         dstImage,
    VkImageLayout                   dstLayout,
    const VkBufferImageCopy*        pRegion)
{
    R_CVULKAN_VALIDATE_COMMAND_BUFFER (pCommandBuffer);
    vkCmdCopyBufferToImage (pCommandBuffer->handle, srcBuffer, dstImage, dstLayout, 1, pRegion);
    return R_CVULKAN_OK;
}

enum R_CVulkanError
R_CVulkan_CommandBuffer_CopyImageToBuffer (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkImage                         srcImage,
    VkImageLayout                   srcLayout,
    VkBuffer                        dstBuffer,
    const VkBufferImageCopy*        pRegion)
{
    R_CVULKAN_VALIDATE_COMMAND_BUFFER (pCommandBuffer);

    vkCmdCopyImageToBuffer (pCommandBuffer->handle, srcImage, srcLayout, dstBuffer, 1, pRegion);
    return R_CVULKAN_OK;
}

enum R_CVulkanError
R_CVulkan_CommandBufferPipelineBarrier (
    struct R_CVulkan_CommandBuffer*             pCommandBuffer,
    const struct R_CVulkan_PipelineBarrierInfo* pBarrierInfo)
{
    R_CVULKAN_VALIDATE_PARAM (pBarrierInfo);
    R_CVULKAN_VALIDATE_COMMAND_BUFFER (pCommandBuffer);
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

enum R_CVulkanError
R_CVulkan_CommandBufferBeginRenderPass (
    struct R_CVulkan_CommandBuffer*             pCommandBuffer,
    const struct R_CVulkan_RenderPassBeginInfo* pRenderPassInfo)
{
    R_CVULKAN_VALIDATE_PARAM (pRenderPassInfo);
    R_CVULKAN_VALIDATE_COMMAND_BUFFER (pCommandBuffer);
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

enum R_CVulkanError
R_CVulkan_CommandBufferEndRenderPass (struct R_CVulkan_CommandBuffer* pCommandBuffer)
{
    R_CVULKAN_VALIDATE_COMMAND_BUFFER (pCommandBuffer);
    vkCmdEndRenderPass (pCommandBuffer->handle);
    return R_CVULKAN_OK;
}

enum R_CVulkanError
R_CVulkan_CommandBufferBeginRendering (
    struct R_CVulkan_CommandBuffer*              pCommandBuffer,
    const struct R_CVulkan_DynamicRenderingInfo* pRenderingInfo)
{
    R_CVULKAN_VALIDATE_PARAM (pRenderingInfo);
    R_CVULKAN_VALIDATE_COMMAND_BUFFER (pCommandBuffer);
    VkRenderingAttachmentInfoKHR* pColorAttachments = NULL;
    if (pRenderingInfo->colorAttachmentCount > 0)
    {
        pColorAttachments = (VkRenderingAttachmentInfoKHR*)R_CSTL_HeapAlloc (
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
            const struct R_CVulkan_DynamicRenderingAttachmentInfo* pSrc
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
        R_CSTL_HeapFree (pColorAttachments);
    }

    return R_CVULKAN_OK;
}

enum R_CVulkanError
R_CVulkan_CommandBufferEndRendering (struct R_CVulkan_CommandBuffer* pCommandBuffer)
{
    R_CVULKAN_VALIDATE_COMMAND_BUFFER (pCommandBuffer);
    vkCmdEndRendering (pCommandBuffer->handle);
    return R_CVULKAN_OK;
}

enum R_CVulkanError
R_CVulkan_CommandBufferNextSubpass (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkSubpassContents               contents)
{
    R_CVULKAN_VALIDATE_COMMAND_BUFFER (pCommandBuffer);
    vkCmdNextSubpass (pCommandBuffer->handle, contents);
    return R_CVULKAN_OK;
}

enum R_CVulkanError
R_CVulkan_CommandBufferPushConstants (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkPipelineLayout                layout,
    VkShaderStageFlags              stageFlags,
    uint32_t                        offset,
    uint32_t                        size,
    const void*                     pValues)
{
    R_CVULKAN_VALIDATE_COMMAND_BUFFER (pCommandBuffer);
    vkCmdPushConstants (pCommandBuffer->handle, layout, stageFlags, offset, size, pValues);
    return R_CVULKAN_OK;
}

enum R_CVulkanError
R_CVulkan_CommandBuffer_Dispatch (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    uint32_t                        groupCountX,
    uint32_t                        groupCountY,
    uint32_t                        groupCountZ)
{
    R_CVULKAN_VALIDATE_COMMAND_BUFFER (pCommandBuffer);
    vkCmdDispatch (pCommandBuffer->handle, groupCountX, groupCountY, groupCountZ);
    return R_CVULKAN_OK;
}

enum R_CVulkanError
R_CVulkan_CommandBufferDispatchIndirect (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkBuffer                        buffer,
    VkDeviceSize                    offset)
{
    R_CVULKAN_VALIDATE_COMMAND_BUFFER (pCommandBuffer);
    vkCmdDispatchIndirect (pCommandBuffer->handle, buffer, offset);
    return R_CVULKAN_OK;
}

VkCommandBuffer
R_CVulkan_CommandBufferGetHandle (const struct R_CVulkan_CommandBuffer* pCommandBuffer)
{
    R_CVULKAN_VALIDATE_GETTER (pCommandBuffer);
    return pCommandBuffer->handle;
}

VkCommandPool
R_CVulkan_CommandBufferGetPool (const struct R_CVulkan_CommandBuffer* pCommandBuffer)
{
    R_CVULKAN_VALIDATE_GETTER (pCommandBuffer);
    return pCommandBuffer->pool;
}

VkDevice
R_CVulkan_CommandBufferGetDevice (const struct R_CVulkan_CommandBuffer* pCommandBuffer)
{
    R_CVULKAN_VALIDATE_GETTER (pCommandBuffer);
    return pCommandBuffer->device;
}

int
R_CVulkan_CommandBufferIsRecording (const struct R_CVulkan_CommandBuffer* pCommandBuffer)
{
#if defined(R_CVULKAN_DEBUG)
    return pCommandBuffer->record;
#else
    return false;
#endif
}

int
R_CVulkan_CommandBufferIsInitialized (const struct R_CVulkan_CommandBuffer* pCommandBuffer)
{
#if defined(R_CVULKAN_DEBUG)
    return 1;
#else
    return true;
#endif
}
