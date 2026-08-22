#pragma once

#include <vulkan/vulkan.h>
#include <stdint.h>
#include <inttypes.h>

#include "rlgame.base/cvulkan/cvulkan_platform.h"

/**
 * @brief Safe wrapper for VkSemaphore
 */
struct R_CVulkan_Semaphore
{
                VkSemaphore handle; /**< Raw Vulkan semaphore handle */
                VkDevice    device; /**< Associated device */
                R_CVULKAN_DEBUG_FIELD
};

/**
 * @brief Initialize a semaphore
 * @param semaphore Pointer to semaphore to initialize
 * @param device Vulkan device handle
 * @param timelineSemaphore Whether to create a timeline semaphore (requires extension)
 * @param initialValue Initial value for timeline semaphores (ignored for binary semaphores)
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkanError R_CVulkan_NewSemaphore (
    struct R_CVulkan_Semaphore*    pSemaphore,
    const struct R_CVulkan_Device* pDevice,
    int                            timelineSemaphore,
    uint64_t                       initialValue);

/**
 * @brief Deletes a semaphore and destroy the Vulkan object
 * @param semaphore Pointer to semaphore to delete
 */
R_CVULKAN_API void R_CVulkan_DeleteSemaphore (struct R_CVulkan_Semaphore* pSemaphore);

/**
 * @brief Signal a timeline semaphore
 * @param semaphore Pointer to semaphore
 * @param value Value to signal
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkanError
R_CVulkan_SemaphoreSignal (struct R_CVulkan_Semaphore* pSemaphore, uint64_t value);

/**
 * @brief Wait on a timeline semaphore
 * @param semaphore Pointer to semaphore
 * @param value Value to wait for
 * @param timeout Timeout in nanoseconds (UINT64_MAX for infinite wait)
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkanError
R_CVulkan_SemaphoreWait (struct R_CVulkan_Semaphore* pSemaphore, uint64_t value, uint64_t timeout);

/**
 * @brief Get the current value of a timeline semaphore
 * @param semaphore Pointer to semaphore
 * @param outValue Pointer to receive the current value
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkanError
R_CVulkan_SemaphoreGetValue (struct R_CVulkan_Semaphore* pSemaphore, uint64_t* pOutValue);

/**
 * @brief Get the raw Vulkan semaphore handle
 * @param semaphore Pointer to semaphore
 * @return Vulkan semaphore handle, or VK_NULL_HANDLE if not initialized
 */
R_CVULKAN_API VkSemaphore R_CVulkan_SemaphoreGetHandle (const struct R_CVulkan_Semaphore* pSemaphore);

/**
 * @brief Get the associated device
 * @param semaphore Pointer to semaphore
 * @return Vulkan device handle, or VK_NULL_HANDLE if not initialized
 */
R_CVULKAN_API VkDevice R_CVulkan_SemaphoreGetDevice (const struct R_CVulkan_Semaphore* pSemaphore);

/**
 * @brief Check if the semaphore is initialized
 * @param semaphore Pointer to semaphore
 * @return 1 if initialized, 0 otherwise
 */
R_CVULKAN_API int R_CVulkan_SemaphoreIsInitialized (const struct R_CVulkan_Semaphore* pSemaphore);
