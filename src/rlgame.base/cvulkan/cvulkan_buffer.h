#pragma once

#include <inttypes.h>
#include <stdint.h>
#include <vulkan/vulkan.h>

#include "rlgame.base/cvulkan/cvulkan_common.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"

struct R_CVulkan_Device;

/**
 * @brief Safe wrapper for VkBuffer
 */
struct R_CVulkan_Buffer
{
                VkBuffer                     handle; /**< Raw Vulkan buffer handle */
                VkDeviceMemory               memory; /**< Device memory handle */
                VkDevice                     device; /**< Associated device */
                R_CVulkanDeviceSize          size; /**< Buffer size in bytes */
                R_CVulkanBufferUsageFlags    usage; /**< Buffer usage flags */
                R_CVulkanMemoryPropertyFlags properties; /**< Memory property flags */
                void*                        pMapped; /**< Mapped memory pointer (if mapped) */
                int                          isMapped; /**< Whether memory is currently mapped */
                R_CVULKAN_DEBUG_FIELD
};

/**
 * @brief Initialize a buffer
 * @param pBuffer Pointer to buffer to initialize
 * @param device R_CVulkan device wrapper
 * @param physicalDevice Physical device
 * @param size Buffer size in bytes
 * @param usage Buffer usage flags
 * @param properties Memory property flags
 * @return R_CVULKAN_ERROR_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkan_Error R_CVulkan_NewBuffer (
    struct R_CVulkan_Buffer*       pBuffer,
    const struct R_CVulkan_Device* device,
    VkPhysicalDevice               physicalDevice,
    R_CVulkanDeviceSize            size,
    R_CVulkanBufferUsageFlags      usage,
    R_CVulkanMemoryPropertyFlags   properties);

/**
 * @brief Deletes a buffer and destroy the Vulkan objects
 * @param pBuffer Pointer to buffer to delete
 */
R_CVULKAN_API void R_CVulkan_DeleteBuffer (struct R_CVulkan_Buffer* pBuffer);

/**
 * @brief Map buffer memory to host memory
 * @param pBuffer Pointer to buffer
 * @param offset Offset in bytes to map from
 * @param size Size in bytes to map
 * @param ppOutData Pointer to receive the mapped data pointer
 * @return R_CVULKAN_ERROR_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkan_Error R_CVulkan_BufferMap (
    struct R_CVulkan_Buffer* pBuffer,
    R_CVulkanDeviceSize      offset,
    R_CVulkanDeviceSize      size,
    void**                   ppOutData);

/**
 * @brief Unmap buffer memory
 * @param pBuffer Pointer to buffer
 * @return R_CVULKAN_ERROR_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkan_Error R_CVulkan_BufferUnmap (struct R_CVulkan_Buffer* pBuffer);

/**
 * @brief Copy data to buffer memory (maps, copies, unmaps)
 * @param pBuffer Pointer to buffer
 * @param offset Offset in bytes to copy to
 * @param size Size in bytes to copy
 * @param data Pointer to source data
 * @return R_CVULKAN_ERROR_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkan_Error R_CVulkan_BufferCopyData (
    struct R_CVulkan_Buffer* pBuffer,
    R_CVulkanDeviceSize      offset,
    R_CVulkanDeviceSize      size,
    const void*              data);

/**
 * @brief Invalidate buffer memory ranges (for host-coherent memory)
 * @param pBuffer Pointer to buffer
 * @param offset Offset in bytes
 * @param size Size in bytes
 * @return R_CVULKAN_ERROR_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkan_Error R_CVulkan_BufferInvalidate (
    struct R_CVulkan_Buffer* pBuffer,
    R_CVulkanDeviceSize      offset,
    R_CVulkanDeviceSize      size);

/**
 * @brief Flush buffer memory ranges (for host-coherent memory)
 * @param pBuffer Pointer to buffer
 * @param offset Offset in bytes
 * @param size Size in bytes
 * @return R_CVULKAN_ERROR_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkan_Error R_CVulkan_BufferFlush (
    struct R_CVulkan_Buffer* pBuffer,
    R_CVulkanDeviceSize      offset,
    R_CVulkanDeviceSize      size);

/**
 * @brief Get the raw Vulkan buffer handle
 * @param pBuffer Pointer to buffer
 * @return Vulkan buffer handle, or VK_NULL_HANDLE if not initialized
 */
R_CVULKAN_API VkBuffer R_CVulkan_BufferGetHandle (const struct R_CVulkan_Buffer* pBuffer);

/**
 * @brief Get the device memory handle
 * @param pBuffer Pointer to buffer
 * @return Device memory handle, or VK_NULL_HANDLE if not initialized
 */
R_CVULKAN_API VkDeviceMemory R_CVulkan_BufferGetMemory (const struct R_CVulkan_Buffer* pBuffer);

/**
 * @brief Get the associated device
 * @param pBuffer Pointer to buffer
 * @return Vulkan device handle, or VK_NULL_HANDLE if not initialized
 */
R_CVULKAN_API VkDevice R_CVulkan_BufferGetDevice (const struct R_CVulkan_Buffer* pBuffer);

/**
 * @brief Get the buffer size
 * @param pBuffer Pointer to buffer
 * @return Buffer size in bytes, or 0 if not initialized
 */
R_CVULKAN_API R_CVulkanDeviceSize R_CVulkan_BufferGetSize (const struct R_CVulkan_Buffer* pBuffer);

/**
 * @brief Get the buffer usage flags
 * @param pBuffer Pointer to buffer
 * @return Buffer usage flags, or 0 if not initialized
 */
R_CVULKAN_API R_CVulkanBufferUsageFlags R_CVulkan_BufferGetUsage (const struct R_CVulkan_Buffer* pBuffer);

/**
 * @brief Get the memory property flags
 * @param pBuffer Pointer to buffer
 * @return Memory property flags, or 0 if not initialized
 */
R_CVULKAN_API R_CVulkanMemoryPropertyFlags R_CVulkan_BufferGetProperties (const struct R_CVulkan_Buffer* pBuffer);

/**
 * @brief Get the mapped memory pointer
 * @param pBuffer Pointer to buffer
 * @return Mapped memory pointer, or NULL if not mapped
 */
R_CVULKAN_API void* R_CVulkan_BufferGetMapped (const struct R_CVulkan_Buffer* pBuffer);

/**
 * @brief Check if the buffer is currently mapped
 * @param pBuffer Pointer to buffer
 * @return 1 if mapped, 0 otherwise
 */
R_CVULKAN_API int R_CVulkan_BufferIsMapped (const struct R_CVulkan_Buffer* pBuffer);

/**
 * @brief Check if the buffer is initialized
 * @param pBuffer Pointer to buffer
 * @return 1 if initialized, 0 otherwise
 */
R_CVULKAN_API int R_CVulkan_BufferIsInitialized (const struct R_CVulkan_Buffer* pBuffer);
