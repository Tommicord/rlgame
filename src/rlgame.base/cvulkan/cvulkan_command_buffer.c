#include "rlgame.base/cvulkan/cvulkan_command_buffer.h"

#include <stdint.h>
#include <inttypes.h>
#include <vulkan/vulkan.h>

enum R_CVulkan_Error
R_CVulkan_NewCommandBuffer (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkDevice                        device,
    VkCommandPool                   pool,
    VkCommandBufferLevel            level)
{
        R_CVULKAN_ASSERT (pCommandBuffer);
        R_CVULKAN_ASSERT (device != VK_NULL_HANDLE);
        R_CVULKAN_ASSERT (pool != VK_NULL_HANDLE);

#if defined(R_CVULKAN_DEBUG)
        if (!pCommandBuffer || device == VK_NULL_HANDLE || pool == VK_NULL_HANDLE)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }
#endif
        pCommandBuffer->handle = VK_NULL_HANDLE;
        pCommandBuffer->pool = pool;
        pCommandBuffer->device = device;
        pCommandBuffer->isRecording = 0;
#if defined(R_CVULKAN_DEBUG)
        pCommandBuffer->isInitialized = false;
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
        pCommandBuffer->isInitialized = true;
#endif
        return R_CVULKAN_ERROR_OK;
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
                vkFreeCommandBuffers (
                    pCommandBuffer->device,
                    pCommandBuffer->pool,
                    1,
                    &pCommandBuffer->handle);
                pCommandBuffer->handle = VK_NULL_HANDLE;
        }

        pCommandBuffer->isRecording = 0;
#if defined(R_CVULKAN_DEBUG)
        pCommandBuffer->isInitialized = false;
#endif
        pCommandBuffer->pool = VK_NULL_HANDLE;
        pCommandBuffer->device = VK_NULL_HANDLE;
}

enum R_CVulkan_Error
R_CVulkan_BeginCommandBuffer (
    struct R_CVulkan_CommandBuffer*       pCommandBuffer,
    VkCommandBufferUsageFlags             flags,
    const VkCommandBufferInheritanceInfo* pInheritanceInfo)
{
        R_CVULKAN_ASSERT (pCommandBuffer);

        if (!pCommandBuffer)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

#if defined(R_CVULKAN_DEBUG)
        if (!pCommandBuffer->isInitialized)
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }
#endif

        if (pCommandBuffer->isRecording)
        {
                return R_CVULKAN_ERROR_FAILED;
        }

        VkCommandBufferBeginInfo beginInfo = {0};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = flags;
        beginInfo.pInheritanceInfo = pInheritanceInfo;

        VkResult result = vkBeginCommandBuffer (pCommandBuffer->handle, &beginInfo);
        if (result != VK_SUCCESS)
        {
                return R_CVULKAN_ResultToError (result);
        }

        pCommandBuffer->isRecording = 1;
        return R_CVULKAN_ERROR_OK;
}

enum R_CVulkan_Error
R_CVulkan_EndCommandBuffer (struct R_CVulkan_CommandBuffer* pCommandBuffer)
{
        R_CVULKAN_ASSERT (pCommandBuffer);

        if (!pCommandBuffer)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

        if (!pCommandBuffer->isInitialized)
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }

        if (!pCommandBuffer->isRecording)
        {
                return R_CVULKAN_ERROR_FAILED;
        }

        VkResult result = vkEndCommandBuffer (pCommandBuffer->handle);
        if (result != VK_SUCCESS)
        {
                return R_CVULKAN_ResultToError (result);
        }

        pCommandBuffer->isRecording = 0;
        return R_CVULKAN_ERROR_OK;
}

enum R_CVulkan_Error
R_CVulkan_ResetCommandBuffer (struct R_CVulkan_CommandBuffer* pCommandBuffer, VkCommandBufferResetFlags flags)
{
        R_CVULKAN_ASSERT (pCommandBuffer);

        if (!pCommandBuffer)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

        if (!pCommandBuffer->isInitialized)
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }

        VkResult result = vkResetCommandBuffer (pCommandBuffer->handle, flags);
        if (result != VK_SUCCESS)
        {
                return R_CVULKAN_ResultToError (result);
        }

        pCommandBuffer->isRecording = 0;
        return R_CVULKAN_ERROR_OK;
}

enum R_CVulkan_Error
R_CVulkan_CommandBufferBindPipeline (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkPipelineBindPoint             pipelineBindPoint,
    VkPipeline                      pipeline)
{
        if (!pCommandBuffer)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

        if (!pCommandBuffer->isInitialized || !pCommandBuffer->isRecording)
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }

        vkCmdBindPipeline (pCommandBuffer->handle, pipelineBindPoint, pipeline);
        return R_CVULKAN_ERROR_OK;
}

enum R_CVulkan_Error
R_CVulkan_CommandBufferBindVertexBuffer (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    uint32_t                        firstBinding,
    uint32_t                        bindingCount,
    const VkBuffer*                 pBuffers,
    const VkDeviceSize*             pOffsets)
{
        if (!pCommandBuffer)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

        if (!pCommandBuffer->isInitialized || !pCommandBuffer->isRecording)
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }

        vkCmdBindVertexBuffers (pCommandBuffer->handle, firstBinding, bindingCount, pBuffers, pOffsets);
        return R_CVULKAN_ERROR_OK;
}

enum R_CVulkan_Error
R_CVulkan_CommandBufferBindIndexBuffer (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkBuffer                        buffer,
    VkDeviceSize                    offset,
    VkIndexType                     indexType)
{
        if (!pCommandBuffer)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

        if (!pCommandBuffer->isInitialized || !pCommandBuffer->isRecording)
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }

        vkCmdBindIndexBuffer (pCommandBuffer->handle, buffer, offset, indexType);
        return R_CVULKAN_ERROR_OK;
}

enum R_CVulkan_Error
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
        if (!pCommandBuffer)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

        if (!pCommandBuffer->isInitialized || !pCommandBuffer->isRecording)
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }

        vkCmdBindDescriptorSets (
            pCommandBuffer->handle,
            pipelineBindPoint,
            layout,
            firstSet,
            descriptorSetCount,
            pDescriptorSets,
            dynamicOffsetCount,
            pDynamicOffsets);
        return R_CVULKAN_ERROR_OK;
}

enum R_CVulkan_Error
R_CVulkan_CommandBufferDraw (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    uint32_t                        vertexCount,
    uint32_t                        instanceCount,
    uint32_t                        firstVertex,
    uint32_t                        firstInstance)
{
        if (!pCommandBuffer)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

        if (!pCommandBuffer->isInitialized || !pCommandBuffer->isRecording)
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }

        vkCmdDraw (pCommandBuffer->handle, vertexCount, instanceCount, firstVertex, firstInstance);
        return R_CVULKAN_ERROR_OK;
}

enum R_CVulkan_Error
R_CVulkan_CommandBufferDrawIndexed (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    uint32_t                        indexCount,
    uint32_t                        instanceCount,
    uint32_t                        firstIndex,
    int32_t                         vertexOffset,
    uint32_t                        firstInstance)
{
        if (!pCommandBuffer)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

        if (!pCommandBuffer->isInitialized || !pCommandBuffer->isRecording)
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }

        vkCmdDrawIndexed (
            pCommandBuffer->handle,
            indexCount,
            instanceCount,
            firstIndex,
            vertexOffset,
            firstInstance);
        return R_CVULKAN_ERROR_OK;
}

enum R_CVulkan_Error
R_CVulkan_CommandBuffer_SetViewport (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    uint32_t                        firstViewport,
    uint32_t                        viewportCount,
    const VkViewport*               pViewports)
{
        if (!pCommandBuffer)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

        if (!pCommandBuffer->isInitialized || !pCommandBuffer->isRecording)
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }

        vkCmdSetViewport (pCommandBuffer->handle, firstViewport, viewportCount, pViewports);
        return R_CVULKAN_ERROR_OK;
}

enum R_CVulkan_Error
R_CVulkan_CommandBufferSetScissor (
    const struct R_CVulkan_CommandBuffer* pCommandBuffer,
    uint32_t                              firstScissor,
    uint32_t                              scissorCount,
    const VkRect2D*                       pScissors)
{
        if (!pCommandBuffer)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

        if (!pCommandBuffer->isInitialized || !pCommandBuffer->isRecording)
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }

        vkCmdSetScissor (pCommandBuffer->handle, firstScissor, scissorCount, pScissors);
        return R_CVULKAN_ERROR_OK;
}

enum R_CVulkan_Error
R_CVulkan_CommandBufferSetBlendConstants (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    const float                     blendConstants[4])
{
        if (!pCommandBuffer)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

        if (!pCommandBuffer->isInitialized || !pCommandBuffer->isRecording)
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }

        vkCmdSetBlendConstants (pCommandBuffer->handle, blendConstants);
        return R_CVULKAN_ERROR_OK;
}

enum R_CVulkan_Error
R_CVulkan_CommandBuffer_SetDepthBounds (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    float                           minDepth,
    float                           maxDepth)
{
        if (!pCommandBuffer)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

        if (!pCommandBuffer->isInitialized || !pCommandBuffer->isRecording)
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }

        vkCmdSetDepthBounds (pCommandBuffer->handle, minDepth, maxDepth);
        return R_CVULKAN_ERROR_OK;
}

enum R_CVulkan_Error
R_CVulkan_CommandBuffer_SetStencilCompareMask (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkStencilFaceFlags              faceMask,
    uint32_t                        compareMask)
{
        if (!pCommandBuffer)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

        if (!pCommandBuffer->isInitialized || !pCommandBuffer->isRecording)
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }

        vkCmdSetStencilCompareMask (pCommandBuffer->handle, faceMask, compareMask);
        return R_CVULKAN_ERROR_OK;
}

enum R_CVulkan_Error
R_CVulkan_CommandBuffer_SetStencilWriteMask (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkStencilFaceFlags              faceMask,
    uint32_t                        writeMask)
{
        if (!pCommandBuffer)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

        if (!pCommandBuffer->isInitialized || !pCommandBuffer->isRecording)
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }

        vkCmdSetStencilWriteMask (pCommandBuffer->handle, faceMask, writeMask);
        return R_CVULKAN_ERROR_OK;
}

enum R_CVulkan_Error
R_CVulkan_CommandBufferSetStencilReference (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkStencilFaceFlags              faceMask,
    uint32_t                        reference)
{
        if (!pCommandBuffer)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

        if (!pCommandBuffer->isInitialized || !pCommandBuffer->isRecording)
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }

        vkCmdSetStencilReference (pCommandBuffer->handle, faceMask, reference);
        return R_CVULKAN_ERROR_OK;
}

enum R_CVulkan_Error
R_CVulkan_CommandBufferCopyBuffer (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkBuffer                        srcBuffer,
    VkBuffer                        dstBuffer,
    const VkBufferCopy*             pRegion)
{
        if (!pCommandBuffer)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

        if (!pCommandBuffer->isInitialized || !pCommandBuffer->isRecording)
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }

        vkCmdCopyBuffer (pCommandBuffer->handle, srcBuffer, dstBuffer, 1, pRegion);
        return R_CVULKAN_ERROR_OK;
}

enum R_CVulkan_Error
R_CVulkan_CommandBufferCopyBufferToImage (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkBuffer                        srcBuffer,
    VkImage                         dstImage,
    VkImageLayout                   dstLayout,
    const VkBufferImageCopy*        pRegion)
{
        if (!pCommandBuffer)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

        if (!pCommandBuffer->isInitialized || !pCommandBuffer->isRecording)
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }

        vkCmdCopyBufferToImage (pCommandBuffer->handle, srcBuffer, dstImage, dstLayout, 1, pRegion);
        return R_CVULKAN_ERROR_OK;
}

enum R_CVulkan_Error
R_CVulkan_CommandBuffer_CopyImageToBuffer (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkImage                         srcImage,
    VkImageLayout                   srcLayout,
    VkBuffer                        dstBuffer,
    const VkBufferImageCopy*        pRegion)
{
        if (!pCommandBuffer)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

        if (!pCommandBuffer->isInitialized || !pCommandBuffer->isRecording)
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }

        vkCmdCopyImageToBuffer (pCommandBuffer->handle, srcImage, srcLayout, dstBuffer, 1, pRegion);
        return R_CVULKAN_ERROR_OK;
}

enum R_CVulkan_Error
R_CVulkan_CommandBufferPipelineBarrier (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkPipelineStageFlags            srcStageMask,
    VkPipelineStageFlags            dstStageMask,
    VkDependencyFlags               dependencyFlags,
    uint32_t                        memoryBarrierCount,
    const VkMemoryBarrier*          pMemoryBarriers,
    uint32_t                        bufferMemoryBarrierCount,
    const VkBufferMemoryBarrier*    pBufferMemoryBarriers,
    uint32_t                        imageMemoryBarrierCount,
    const VkImageMemoryBarrier*     pImageMemoryBarriers)
{
        if (!pCommandBuffer)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

        if (!pCommandBuffer->isInitialized || !pCommandBuffer->isRecording)
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }

        vkCmdPipelineBarrier (
            pCommandBuffer->handle,
            srcStageMask,
            dstStageMask,
            dependencyFlags,
            memoryBarrierCount,
            pMemoryBarriers,
            bufferMemoryBarrierCount,
            pBufferMemoryBarriers,
            imageMemoryBarrierCount,
            pImageMemoryBarriers);
        return R_CVULKAN_ERROR_OK;
}

enum R_CVulkan_Error
R_CVulkan_CommandBufferBeginRenderPass (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkRenderPass                    renderPass,
    VkFramebuffer                   framebuffer,
    const VkRect2D*                 pRenderArea,
    VkSubpassContents               contents,
    uint32_t                        clearValueCount,
    const VkClearValue*             pClearValues)
{
        if (!pCommandBuffer)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

        if (!pCommandBuffer->isInitialized || !pCommandBuffer->isRecording)
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }

        VkRenderPassBeginInfo beginInfo = {0};
        beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        beginInfo.renderPass = renderPass;
        beginInfo.framebuffer = framebuffer;
        beginInfo.renderArea = *pRenderArea;
        beginInfo.clearValueCount = clearValueCount;
        beginInfo.pClearValues = pClearValues;

        vkCmdBeginRenderPass (pCommandBuffer->handle, &beginInfo, contents);
        return R_CVULKAN_ERROR_OK;
}

enum R_CVulkan_Error
R_CVulkan_CommandBufferEndRenderPass (struct R_CVulkan_CommandBuffer* pCommandBuffer)
{
        if (!pCommandBuffer)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

        if (!pCommandBuffer->isInitialized || !pCommandBuffer->isRecording)
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }

        vkCmdEndRenderPass (pCommandBuffer->handle);
        return R_CVULKAN_ERROR_OK;
}

enum R_CVulkan_Error
R_CVulkan_CommandBufferNextSubpass (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkSubpassContents               contents)
{
        if (!pCommandBuffer)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

        if (!pCommandBuffer->isInitialized || !pCommandBuffer->isRecording)
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }

        vkCmdNextSubpass (pCommandBuffer->handle, contents);
        return R_CVULKAN_ERROR_OK;
}

enum R_CVulkan_Error
R_CVulkan_CommandBufferPushConstants (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkPipelineLayout                layout,
    VkShaderStageFlags              stageFlags,
    uint32_t                        offset,
    uint32_t                        size,
    const void*                     pValues)
{
        if (!pCommandBuffer)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

        if (!pCommandBuffer->isInitialized || !pCommandBuffer->isRecording)
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }

        vkCmdPushConstants (pCommandBuffer->handle, layout, stageFlags, offset, size, pValues);
        return R_CVULKAN_ERROR_OK;
}

enum R_CVulkan_Error
R_CVulkan_CommandBuffer_Dispatch (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    uint32_t                        groupCountX,
    uint32_t                        groupCountY,
    uint32_t                        groupCountZ)
{
        if (!pCommandBuffer)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

        if (!pCommandBuffer->isInitialized || !pCommandBuffer->isRecording)
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }

        vkCmdDispatch (pCommandBuffer->handle, groupCountX, groupCountY, groupCountZ);
        return R_CVULKAN_ERROR_OK;
}

enum R_CVulkan_Error
R_CVulkan_CommandBufferDispatchIndirect (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkBuffer                        buffer,
    VkDeviceSize                    offset)
{
        if (!pCommandBuffer)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

        if (!pCommandBuffer->isInitialized || !pCommandBuffer->isRecording)
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }

        vkCmdDispatchIndirect (pCommandBuffer->handle, buffer, offset);
        return R_CVULKAN_ERROR_OK;
}

VkCommandBuffer
R_CVulkan_CommandBufferGetHandle (const struct R_CVulkan_CommandBuffer* pCommandBuffer)
{
        return pCommandBuffer->handle;
}

VkCommandPool
R_CVulkan_CommandBufferGetPool (const struct R_CVulkan_CommandBuffer* pCommandBuffer)
{
        return pCommandBuffer->pool;
}

VkDevice
R_CVulkan_CommandBufferGetDevice (const struct R_CVulkan_CommandBuffer* pCommandBuffer)
{
        return pCommandBuffer->device;
}

int
R_CVulkan_CommandBufferIsRecording (const struct R_CVulkan_CommandBuffer* pCommandBuffer)
{
        return pCommandBuffer->isRecording;
}

int
R_CVulkan_CommandBufferIsInitialized (const struct R_CVulkan_CommandBuffer* pCommandBuffer)
{
        return pCommandBuffer->isInitialized;
}
