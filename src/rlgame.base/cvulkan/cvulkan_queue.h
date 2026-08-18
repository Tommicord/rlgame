#pragma once

#include <inttypes.h>
#include <stdint.h>
#include <vulkan/vulkan.h>

#include "rlgame.base/cvulkan/cvulkan_common.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"

struct R_CVulkan_Device;
struct R_CVulkan_CommandBuffer;
struct R_CVulkan_Semaphore;
struct R_CVulkan_Fence;

/**
 * @brief Safe wrapper for VkQueue
 */
struct R_CVulkan_Queue
{
                VkQueue  handle; /**< Raw Vulkan queue handle */
                VkDevice device; /**< Associated device */
                uint32_t queueFamilyIndex; /**< Queue family index */
                uint32_t queueIndex; /**< Queue index within family */
                R_CVULKAN_DEBUG_FIELD
};

/**
 * @brief Initialize a queue
 * @param pQueue Pointer to queue to initialize
 * @param pDevice R_CVulkan device wrapper
 * @param queueFamilyIndex Queue family index
 * @param queueIndex Queue index within family
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkan_Error R_CVulkan_NewQueue (
    struct R_CVulkan_Queue*        pQueue,
    const struct R_CVulkan_Device* pDevice,
    uint32_t                       queueFamilyIndex,
    uint32_t                       queueIndex);

/**
 * @brief Deletes a queue
 * @param pQueue Pointer to queue to delete
 */
R_CVULKAN_API void R_CVulkan_DeleteQueue (struct R_CVulkan_Queue* pQueue);

/**
 * @brief Submit command buffers to the queue
 * @param queue Pointer to queue
 * @param pCommandBuffers Array of command buffers to submit
 * @param commandBufferCount Number of command buffers
 * @param pWaitSemaphores Array of semaphores to wait on
 * @param waitSemaphoreCount Number of wait semaphores
 * @param pWaitDstStageMask Array of pipeline stage flags to wait at
 * @param pSignalSemaphores Array of semaphores to signal
 * @param signalSemaphoreCount Number of signal semaphores
 * @param pFence Optional fence to signal when submission completes
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkan_Error R_CVulkan_QueueSubmit (
    struct R_CVulkan_Queue*               pQueue,
    const struct R_CVulkan_CommandBuffer* pCommandBuffers,
    uint32_t                              commandBufferCount,
    const struct R_CVulkan_Semaphore*     pWaitSemaphores,
    uint32_t                              waitSemaphoreCount,
    const VkPipelineStageFlags*           pWaitDstStageMask,
    const struct R_CVulkan_Semaphore*     pSignalSemaphores,
    uint32_t                              signalSemaphoreCount,
    const struct R_CVulkan_Fence*         pFence);

/**
 * @brief Present an image from a swapchain
 * @param pQueue Pointer to queue
 * @param pSwapchains Array of swapchains to present from (raw Vulkan handles)
 * @param swapchainCount Number of swapchains
 * @param pImageIndices Array of image indices to present
 * @param pWaitSemaphores Array of semaphores to wait on
 * @param waitSemaphoreCount Number of wait semaphores
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkan_Error R_CVulkan_QueuePresent (
    struct R_CVulkan_Queue*           pQueue,
    const VkSwapchainKHR*             pSwapchains,
    uint32_t                          swapchainCount,
    const uint32_t*                   pImageIndices,
    const struct R_CVulkan_Semaphore* pWaitSemaphores,
    uint32_t                          waitSemaphoreCount);

/**
 * @brief Wait for the queue to become idle
 * @param queue Pointer to queue
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkan_Error R_CVulkan_QueueWaitIdle (struct R_CVulkan_Queue* pQueue);

/**
 * @brief Get the raw Vulkan queue handle
 * @param queue Pointer to queue
 * @return Vulkan queue handle, or VK_NULL_HANDLE if not initialized
 */
R_CVULKAN_API VkQueue R_CVulkan_QueueGetHandle (const struct R_CVulkan_Queue* pQueue);

/**
 * @brief Get the associated device
 * @param queue Pointer to queue
 * @return Vulkan device handle, or VK_NULL_HANDLE if not initialized
 */
R_CVULKAN_API VkDevice R_CVulkan_QueueGetDevice (const struct R_CVulkan_Queue* pQueue);

/**
 * @brief Get the queue family index
 * @param queue Pointer to queue
 * @return Queue family index, or 0 if not initialized
 */
R_CVULKAN_API uint32_t R_CVulkan_QueueGetFamilyIndex (const struct R_CVulkan_Queue* pQueue);

/**
 * @brief Get the queue index
 * @param queue Pointer to queue
 * @return Queue index within family, or 0 if not initialized
 */
R_CVULKAN_API uint32_t R_CVulkan_QueueGetIndex (const struct R_CVulkan_Queue* pQueue);

/**
 * @brief Check if the queue is initialized
 * @param queue Pointer to queue
 * @return 1 if initialized, 0 otherwise
 */
R_CVULKAN_API int R_CVulkan_QueueIsInitialized (const struct R_CVulkan_Queue* pQueue);
