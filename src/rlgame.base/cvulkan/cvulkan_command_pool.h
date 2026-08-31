#pragma once

#include <vulkan/vulkan.h>
#include <stdint.h>
#include <inttypes.h>

#include "rlgame.base/cvulkan/cvulkan_platform.h"

struct R_CVulkan_Device;

/**
 * @brief Safe wrapper for VkCommandPool
 */
struct R_CVulkan_CommandPool
{
        VkCommandPool handle; /**< Raw Vulkan command pool handle */
        VkDevice      device; /**< Associated device */
        uint32_t      queueFamilyIndex; /**< Queue family index */
};

/**
 * @brief Initialize a command pool
 * @param commandPool Pointer to command pool to initialize
 * @param device R_CVulkan device wrapper
 * @param queueFamilyIndex Queue family index
 * @param flags Command pool creation flags
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkan_Error r_cvulkan_new_command_pool (
    struct R_CVulkan_CommandPool*  pCommandPool,
    const struct R_CVulkan_Device* pDevice,
    uint32_t                       queueFamilyIndex,
    VkCommandPoolCreateFlags       flags);

/**
 * @brief Deletes a command pool and destroy the Vulkan object
 * @param commandPool Pointer to command pool to delete
 */
R_CVULKAN_API void r_cvulkan_delete_command_pool (struct R_CVulkan_CommandPool* pCommandPool);

/**
 * @brief Reset a command pool
 * @param commandPool Pointer to command pool
 * @param flags Reset flags
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkan_Error
r_cvulkan_command_pool_reset (struct R_CVulkan_CommandPool* pCommandPool, VkCommandPoolResetFlags flags);

/**
 * @brief Trim a command pool
 * @param commandPool Pointer to command pool
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkan_Error r_cvulkan_command_pool_trim (struct R_CVulkan_CommandPool* pCommandPool);

/**
 * @brief Get the raw Vulkan command pool handle
 * @param commandPool Pointer to command pool
 * @return Vulkan command pool handle, or VK_NULL_HANDLE if not initialized
 */
R_CVULKAN_API VkCommandPool r_cvulkan_command_pool_get_handle (const struct R_CVulkan_CommandPool* pCommandPool);

/**
 * @brief Get the associated device
 * @param commandPool Pointer to command pool
 * @return Vulkan device handle, or VK_NULL_HANDLE if not initialized
 */
R_CVULKAN_API VkDevice r_cvulkan_command_pool_get_device (const struct R_CVulkan_CommandPool* pCommandPool);

/**
 * @brief Get the queue family index
 * @param commandPool Pointer to command pool
 * @return Queue family index, or 0 if not initialized
 */
R_CVULKAN_API uint32_t
r_cvulkan_command_pool_get_queue_family_index (const struct R_CVulkan_CommandPool* pCommandPool);

/**
 * @brief Check if the command pool is initialized
 * @param commandPool Pointer to command pool
 * @return 1 if initialized, 0 otherwise
 */
R_CVULKAN_API int r_cvulkan_command_pool_is_initialized (const struct R_CVulkan_CommandPool* pCommandPool);
