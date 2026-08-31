#pragma once

#include <vulkan/vulkan.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include "rlgame.base/cvulkan/cvulkan_platform.h"

struct R_CVulkan_Device;

/**
 * @brief Safe wrapper for VkFence
 */
struct R_CVulkan_Fence
{
        VkFence  handle; /**< Raw Vulkan fence handle */
        VkDevice device; /**< Associated device */
};

/**
 * @brief Initialize a fence
 * @param fence Pointer to fence to initialize
 * @param device R_CVulkan device wrapper
 * @param signaled Whether the fence should be created in signaled state
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_NewFence (struct R_CVulkan_Fence* pFence, const struct R_CVulkan_Device* pDevice, bool signaled);

/**
 * @brief Deletes a fence and destroy the Vulkan object
 * @param fence Pointer to fence to delete
 */
R_CVULKAN_API void R_CVulkan_DeleteFence (struct R_CVulkan_Fence* pFence);

/**
 * @brief Wait for one or more fences to be signaled
 * @param device R_CVulkan device wrapper
 * @param fences Array of fences
 * @param fenceCount Number of fences
 * @param waitAll Wait for all fences (1) or any fence (0)
 * @param timeout Timeout in nanoseconds (UINT64_MAX for infinite wait)
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkan_Error R_CVulkan_FenceWait (
    const struct R_CVulkan_Device* pDevice,
    const struct R_CVulkan_Fence*  pFences,
    uint32_t                       fenceCount,
    int                            waitAll,
    uint64_t                       timeout);

/**
 * @brief Reset one or more fences to unsignaled state
 * @param pDevice R_CVulkan device wrapper
 * @param pFences Array of fences
 * @param fenceCount Number of fences
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkan_Error R_CVulkan_FenceReset (
    const struct R_CVulkan_Device* pDevice,
    const struct R_CVulkan_Fence*  pFences,
    uint32_t                       fenceCount);

/**
 * @brief Check if a fence is signaled
 * @param pDevice R_CVulkan device wrapper
 * @param pFence Pointer to fence
 * @param pOutSignaled Pointer to receive the signaled status (1 = signaled, 0 = unsignaled)
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkan_Error r_cvulkan_fence_get_status (
    const struct R_CVulkan_Device* pDevice,
    const struct R_CVulkan_Fence*  pFence,
    bool*                          pOutSignaled);

/**
 * @brief Get the raw Vulkan fence handle
 * @param fence Pointer to fence
 * @return Vulkan fence handle, or VK_NULL_HANDLE if not initialized
 */
R_CVULKAN_API VkFence r_cvulkan_fence_get_handle (const struct R_CVulkan_Fence* pFence);

/**
 * @brief Get the associated device
 * @param fence Pointer to fence
 * @return Vulkan device handle, or VK_NULL_HANDLE if not initialized
 */
R_CVULKAN_API VkDevice r_cvulkan_fence_get_device (const struct R_CVulkan_Fence* pFence);
