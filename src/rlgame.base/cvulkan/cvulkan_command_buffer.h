#pragma once

#include <stdint.h>
#include <inttypes.h>
#include <vulkan/vulkan.h>

#include "rlgame.base/cvulkan/cvulkan_common.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"

/**
 * @brief Safe wrapper for VkCommandBuffer
 */
struct R_CVulkan_CommandBuffer
{
                VkCommandBuffer handle; /**< Raw Vulkan command buffer handle */
                VkCommandPool   pool; /**< Associated command pool */
                VkDevice        device; /**< Associated device */
                int             isRecording; /**< Whether currently recording */
                R_CVULKAN_DEBUG_FIELD
};

/**
 * @brief Initialize a command buffer
 * @param pCommandBuffer Pointer to command buffer to initialize
 * @param device Vulkan device
 * @param pool Command pool
 * @param level Command buffer level (primary or secondary)
 * @return R_CVULKAN_ERROR_OK on success, error code otherwise
 */
enum R_CVulkan_Error R_CVulkan_NewCommandBuffer (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkDevice                        device,
    VkCommandPool                   pool,
    VkCommandBufferLevel            level);

/**
 * @brief Deletes a command buffer
 * @param pCommandBuffer Pointer to command buffer to delete
 */
void R_CVulkan_DeleteCommandBuffer (struct R_CVulkan_CommandBuffer* pCommandBuffer);

/**
 * @brief Begin recording a command buffer
 * @param pCommandBuffer Pointer to command buffer
 * @param flags Command buffer usage flags
 * @param pInheritanceInfo Inheritance info for secondary command buffers (NULL for primary)
 * @return R_CVULKAN_ERROR_OK on success, error code otherwise
 */
enum R_CVulkan_Error R_CVulkan_BeginCommandBuffer (
    struct R_CVulkan_CommandBuffer*       pCommandBuffer,
    VkCommandBufferUsageFlags             flags,
    const VkCommandBufferInheritanceInfo* pInheritanceInfo);

/**
 * @brief End recording a command buffer
 * @param pCommandBuffer Pointer to command buffer
 * @return R_CVULKAN_ERROR_OK on success, error code otherwise
 */
enum R_CVulkan_Error R_CVulkan_EndCommandBuffer (struct R_CVulkan_CommandBuffer* pCommandBuffer);

/**
 * @brief Reset a command buffer
 * @param pCommandBuffer Pointer to command buffer
 * @param flags Reset flags
 * @return R_CVULKAN_ERROR_OK on success, error code otherwise
 */
enum R_CVulkan_Error R_CVulkan_ResetCommandBuffer (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkCommandBufferResetFlags       flags);

/* Command functions */

/**
 * @brief Bind a pipeline
 * @param pCommandBuffer Pointer to command buffer
 * @param pipelineBindPoint Pipeline bind point
 * @param pipeline Pipeline to bind
 * @return R_CVULKAN_ERROR_OK on success, error code otherwise
 */
enum R_CVulkan_Error R_CVulkan_CommandBufferBindPipeline (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkPipelineBindPoint             pipelineBindPoint,
    VkPipeline                      pipeline);

/**
 * @brief Bind vertex buffers
 * @param pCommandBuffer Pointer to command buffer
 * @param firstBinding First binding index
 * @param bindingCount Number of bindings
 * @param pBuffers Array of buffers
 * @param pOffsets Array of offsets
 * @return R_CVULKAN_ERROR_OK on success, error code otherwise
 */
enum R_CVulkan_Error R_CVulkan_CommandBufferBindVertexBuffer (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    uint32_t                        firstBinding,
    uint32_t                        bindingCount,
    const VkBuffer*                 pBuffers,
    const VkDeviceSize*             pOffsets);

/**
 * @brief Bind index buffer
 * @param pCommandBuffer Pointer to command buffer
 * @param buffer Index buffer
 * @param offset Offset in bytes
 * @param indexType Index type
 * @return R_CVULKAN_ERROR_OK on success, error code otherwise
 */
enum R_CVulkan_Error R_CVulkan_CommandBufferBindIndexBuffer (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkBuffer                        buffer,
    VkDeviceSize                    offset,
    VkIndexType                     indexType);

/**
 * @brief Bind descriptor sets
 * @param pCommandBuffer Pointer to command buffer
 * @param pipelineBindPoint Pipeline bind point
 * @param layout Pipeline layout
 * @param firstSet First descriptor set index
 * @param descriptorSetCount Number of descriptor sets
 * @param pDescriptorSets Array of descriptor sets
 * @param dynamicOffsetCount Number of dynamic offsets
 * @param pDynamicOffsets Array of dynamic offsets
 * @return R_CVULKAN_ERROR_OK on success, error code otherwise
 */
enum R_CVulkan_Error R_CVulkan_CommandBufferBindDescriptorSets (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkPipelineBindPoint             pipelineBindPoint,
    VkPipelineLayout                layout,
    uint32_t                        firstSet,
    uint32_t                        descriptorSetCount,
    const VkDescriptorSet*          pDescriptorSets,
    uint32_t                        dynamicOffsetCount,
    const uint32_t*                 pDynamicOffsets);

/**
 * @brief Draw
 * @param pCommandBuffer Pointer to command buffer
 * @param vertexCount Number of vertices to draw
 * @param instanceCount Number of instances to draw
 * @param firstVertex First vertex index
 * @param firstInstance First instance index
 * @return R_CVULKAN_ERROR_OK on success, error code otherwise
 */
enum R_CVulkan_Error R_CVulkan_CommandBufferDraw (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    uint32_t                        vertexCount,
    uint32_t                        instanceCount,
    uint32_t                        firstVertex,
    uint32_t                        firstInstance);

/**
 * @brief Draw indexed
 * @param pCommandBuffer Pointer to command buffer
 * @param indexCount Number of indices to draw
 * @param instanceCount Number of instances to draw
 * @param firstIndex First index
 * @param vertexOffset Vertex offset
 * @param firstInstance First instance index
 * @return R_CVULKAN_ERROR_OK on success, error code otherwise
 */
enum R_CVulkan_Error R_CVulkan_CommandBufferDrawIndexed (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    uint32_t                        indexCount,
    uint32_t                        instanceCount,
    uint32_t                        firstIndex,
    int32_t                         vertexOffset,
    uint32_t                        firstInstance);

/**
 * @brief Set viewport
 * @param pCommandBuffer Pointer to command buffer
 * @param firstViewport First viewport index
 * @param viewportCount Number of viewports
 * @param pViewports Array of viewports
 * @return R_CVULKAN_ERROR_OK on success, error code otherwise
 */
enum R_CVulkan_Error R_CVulkan_CommandBufferSetViewport (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    uint32_t                        firstViewport,
    uint32_t                        viewportCount,
    const VkViewport*               pViewports);

/**
 * @brief Set scissor
 * @param pCommandBuffer Pointer to command buffer
 * @param firstScissor First scissor index
 * @param scissorCount Number of scissors
 * @param pScissors Array of scissors
 * @return R_CVULKAN_ERROR_OK on success, error code otherwise
 */
enum R_CVulkan_Error R_CVulkan_CommandBufferSetScissor (
    const struct R_CVulkan_CommandBuffer* pCommandBuffer,
    uint32_t                        firstScissor,
    uint32_t                        scissorCount,
    const VkRect2D*                 pScissors);

/**
 * @brief Set blend constants
 * @param pCommandBuffer Pointer to command buffer
 * @param blendConstants Blend constants (4 floats)
 * @return R_CVULKAN_ERROR_OK on success, error code otherwise
 */
enum R_CVulkan_Error R_CVulkan_CommandBufferSetBlendConstants (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    const float                     blendConstants[4]);

/**
 * @brief Set depth bounds
 * @param pCommandBuffer Pointer to command buffer
 * @param minDepth Minimum depth bound
 * @param maxDepth Maximum depth bound
 * @return R_CVULKAN_ERROR_OK on success, error code otherwise
 */
enum R_CVulkan_Error R_CVulkan_CommandBufferSetDepthBounds (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    float                           minDepth,
    float                           maxDepth);

/**
 * @brief Set stencil compare mask
 * @param pCommandBuffer Pointer to command buffer
 * @param faceMask Face mask
 * @param compareMask Compare mask
 * @return R_CVULKAN_ERROR_OK on success, error code otherwise
 */
enum R_CVulkan_Error R_CVulkan_CommandBufferSetStencilCompareMask (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkStencilFaceFlags              faceMask,
    uint32_t                        compareMask);

/**
 * @brief Set stencil write mask
 * @param pCommandBuffer Pointer to command buffer
 * @param faceMask Face mask
 * @param writeMask Write mask
 * @return R_CVULKAN_ERROR_OK on success, error code otherwise
 */
enum R_CVulkan_Error R_CVulkan_CommandBufferSetStencilWriteMask (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkStencilFaceFlags              faceMask,
    uint32_t                        writeMask);

/**
 * @brief Set stencil reference
 * @param pCommandBuffer Pointer to command buffer
 * @param faceMask Face mask
 * @param reference Reference value
 * @return R_CVULKAN_ERROR_OK on success, error code otherwise
 */
enum R_CVulkan_Error R_CVulkan_CommandBufferSetStencilReference (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkStencilFaceFlags              faceMask,
    uint32_t                        reference);

/**
 * @brief Copy buffer
 * @param pCommandBuffer Pointer to command buffer
 * @param srcBuffer Source buffer
 * @param dstBuffer Destination buffer
 * @param pRegion Copy region
 * @return R_CVULKAN_ERROR_OK on success, error code otherwise
 */
enum R_CVulkan_Error R_CVulkan_CommandBufferCopyBuffer (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkBuffer                        srcBuffer,
    VkBuffer                        dstBuffer,
    const VkBufferCopy*             pRegion);

/**
 * @brief Copy buffer to image
 * @param pCommandBuffer Pointer to command buffer
 * @param srcBuffer Source buffer
 * @param dstImage Destination image
 * @param dstLayout Destination image layout
 * @param pRegion Copy region
 * @return R_CVULKAN_ERROR_OK on success, error code otherwise
 */
enum R_CVulkan_Error R_CVulkan_CommandBufferCopyBufferToImage (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkBuffer                        srcBuffer,
    VkImage                         dstImage,
    VkImageLayout                   dstLayout,
    const VkBufferImageCopy*        pRegion);

/**
 * @brief Copy image to buffer
 * @param pCommandBuffer Pointer to command buffer
 * @param srcImage Source image
 * @param srcLayout Source image layout
 * @param dstBuffer Destination buffer
 * @param region Copy region
 * @return R_CVULKAN_ERROR_OK on success, error code otherwise
 */
enum R_CVulkan_Error R_CVulkan_CommandBufferCopyImageToBuffer (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkImage                         srcImage,
    VkImageLayout                   srcLayout,
    VkBuffer                        dstBuffer,
    const VkBufferImageCopy*        region);

/**
 * @brief Pipeline barrier
 * @param pCommandBuffer Pointer to command buffer
 * @param srcStageMask Source stage mask
 * @param dstStageMask Destination stage mask
 * @param srcAccessMask Source access mask
 * @param dstAccessMask Destination access mask
 * @param dependencyFlags Dependency flags
 * @param memoryBarrierCount Number of memory barriers
 * @param pMemoryBarriers Array of memory barriers
 * @param bufferMemoryBarrierCount Number of buffer memory barriers
 * @param pBufferMemoryBarriers Array of buffer memory barriers
 * @param imageMemoryBarrierCount Number of image memory barriers
 * @param pImageMemoryBarriers Array of image memory barriers
 * @return R_CVULKAN_ERROR_OK on success, error code otherwise
 */
enum R_CVulkan_Error R_CVulkan_CommandBufferPipelineBarrier (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkPipelineStageFlags            srcStageMask,
    VkPipelineStageFlags            dstStageMask,
    VkDependencyFlags               dependencyFlags,
    uint32_t                        memoryBarrierCount,
    const VkMemoryBarrier*          pMemoryBarriers,
    uint32_t                        bufferMemoryBarrierCount,
    const VkBufferMemoryBarrier*    pBufferMemoryBarriers,
    uint32_t                        imageMemoryBarrierCount,
    const VkImageMemoryBarrier*     pImageMemoryBarriers);

/**
 * @brief Begin render pass
 * @param pCommandBuffer Pointer to command buffer
 * @param renderPass Render pass
 * @param framebuffer Framebuffer
 * @param pRenderArea Render area
 * @param contents Render pass contents
 * @param clearValueCount Number of clear values
 * @param pClearValues Array of clear values
 * @return R_CVULKAN_ERROR_OK on success, error code otherwise
 */
enum R_CVulkan_Error R_CVulkan_CommandBufferBeginRenderPass (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkRenderPass                    renderPass,
    VkFramebuffer                   framebuffer,
    const VkRect2D*                 pRenderArea,
    VkSubpassContents               contents,
    uint32_t                        clearValueCount,
    const VkClearValue*             pClearValues);

/**
 * @brief End render pass
 * @param pCommandBuffer Pointer to command buffer
 * @return R_CVULKAN_ERROR_OK on success, error code otherwise
 */
enum R_CVulkan_Error R_CVulkan_CommandBufferEndRenderPass (struct R_CVulkan_CommandBuffer* pCommandBuffer);

/**
 * @brief Next subpass
 * @param pCommandBuffer Pointer to command buffer
 * @param contents Subpass contents
 * @return R_CVULKAN_ERROR_OK on success, error code otherwise
 */
enum R_CVulkan_Error R_CVulkan_CommandBufferNextSubpass (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkSubpassContents               contents);

/**
 * @brief Push constants
 * @param pCommandBuffer Pointer to command buffer
 * @param layout Pipeline layout
 * @param stageFlags Shader stage flags
 * @param offset Offset in push constants
 * @param size Size of data
 * @param pValues Pointer to data
 * @return R_CVULKAN_ERROR_OK on success, error code otherwise
 */
enum R_CVulkan_Error R_CVulkan_CommandBufferPushConstants (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkPipelineLayout                layout,
    VkShaderStageFlags              stageFlags,
    uint32_t                        offset,
    uint32_t                        size,
    const void*                     pValues);

/**
 * @brief Dispatch compute
 * @param pCommandBuffer Pointer to command buffer
 * @param groupCountX Number of work groups in X dimension
 * @param groupCountY Number of work groups in Y dimension
 * @param groupCountZ Number of work groups in Z dimension
 * @return R_CVULKAN_ERROR_OK on success, error code otherwise
 */
enum R_CVulkan_Error R_CVulkan_CommandBufferDispatch (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    uint32_t                        groupCountX,
    uint32_t                        groupCountY,
    uint32_t                        groupCountZ);

/**
 * @brief Dispatch indirect
 * @param pCommandBuffer Pointer to command buffer
 * @param buffer Buffer containing dispatch parameters
 * @param offset Offset in buffer
 * @return R_CVULKAN_ERROR_OK on success, error code otherwise
 */
enum R_CVulkan_Error R_CVulkan_CommandBufferDispatchIndirect (
    struct R_CVulkan_CommandBuffer* pCommandBuffer,
    VkBuffer                        buffer,
    VkDeviceSize                    offset);

/**
 * @brief Get the raw Vulkan command buffer handle
 * @param pCommandBuffer Pointer to command buffer
 * @return Vulkan command buffer handle, or VK_NULL_HANDLE if not initialized
 */
VkCommandBuffer R_CVulkan_CommandBufferGetHandle (const struct R_CVulkan_CommandBuffer* pCommandBuffer);

/**
 * @brief Get the associated command pool
 * @param pCommandBuffer Pointer to command buffer
 * @return Command pool handle, or VK_NULL_HANDLE if not initialized
 */
VkCommandPool R_CVulkan_CommandBufferGetPool (const struct R_CVulkan_CommandBuffer* pCommandBuffer);

/**
 * @brief Get the associated device
 * @param pCommandBuffer Pointer to command buffer
 * @return Vulkan device handle, or VK_NULL_HANDLE if not initialized
 */
VkDevice R_CVulkan_CommandBufferGetDevice (const struct R_CVulkan_CommandBuffer* pCommandBuffer);

/**
 * @brief Check if currently recording
 * @param pCommandBuffer Pointer to command buffer
 * @return 1 if recording, 0 otherwise
 */
int R_CVulkan_CommandBufferIsRecording (const struct R_CVulkan_CommandBuffer* pCommandBuffer);

/**
 * @brief Check if the command buffer is initialized
 * @param pCommandBuffer Pointer to command buffer
 * @return 1 if initialized, 0 otherwise
 */
int R_CVulkan_CommandBufferIsInitialized (const struct R_CVulkan_CommandBuffer* pCommandBuffer);
