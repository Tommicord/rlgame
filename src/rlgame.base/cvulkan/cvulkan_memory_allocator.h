#pragma once

#include <vulkan/vulkan.h>
#include <stdint.h>
#include <inttypes.h>

#include "rlgame.base/cvulkan/cvulkan_platform.h"
#include "rlgame.base/cvulkan/cvulkan_defragmentation.h"

/**
 * @brief Suballocation representing an allocation within a memory block
 */
struct R_CVulkan_Suballocation
{
                VkDeviceSize   offset; /**< Offset within the memory block */
                VkDeviceSize   size; /**< Size of the allocation */
                VkBuffer       buffer; /**< Buffer handle */
                VkDeviceMemory memory; /**< Device memory handle */
                uint32_t       blockIndex; /**< Index of the memory block */
};

/**
 * @brief Free region within a memory block
 */
struct R_CVulkan_FreeRegion
{
                VkDeviceSize offset; /**< Offset of the free region */
                VkDeviceSize size; /**< Size of the free region */
};

/**
 * @brief Memory block that manages a single Vulkan buffer/memory
 */
struct R_CVulkan_MemoryBlock
{
                VkDevice                     device; /**< Vulkan device */
                VkPhysicalDevice             physicalDevice; /**< Physical device */
                VkDeviceSize                 size; /**< Total size of the block */
                VkBufferUsageFlags           usage; /**< Buffer usage flags */
                VkMemoryPropertyFlags        properties; /**< Memory property flags */
                VkBuffer                     buffer; /**< Buffer handle */
                VkDeviceMemory               memory; /**< Device memory handle */
                VkDeviceSize                 usedSize; /**< Currently used size */
                struct R_CVulkan_FreeRegion* pFreeRegions; /**< Array of free regions */
                uint32_t                     freeRegionCount; /**< Number of free regions */
                uint32_t                     freeRegionCapacity; /**< Capacity of free regions array */
};

/**
 * @brief Memory allocator that manages multiple memory blocks
 */
struct R_CVulkan_MemoryAllocator
{
                VkDevice                       device; /**< Vulkan device */
                VkPhysicalDevice               physicalDevice; /**< Physical device */
                struct R_CVulkan_MemoryBlock** ppBlocks; /**< Array of memory blocks */
                uint32_t                       blockCount; /**< Number of blocks */
                uint32_t                       blockCapacity; /**< Capacity of blocks array */
                VkDeviceSize                   minBlockSize; /**< Minimum block size (default: 256KB) */
                VkDeviceSize defaultMaxBlockSize; /**< Default maximum block size (default: 256MB) */
};

/**
 * @brief Initialize a memory allocator
 * @param pAllocator Pointer to allocator to initialize
 * @param device Vulkan device
 * @param physicalDevice Physical device
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkanError R_CVulkan_NewMemoryAllocator (
    struct R_CVulkan_MemoryAllocator* pAllocator,
    VkDevice                          device,
    VkPhysicalDevice                  physicalDevice);

/**
 * @brief Deletes a memory allocator and free all blocks
 * @param pAllocator Pointer to allocator to delete
 */
R_CVULKAN_API void R_CVulkan_DeleteMemoryAllocator (struct R_CVulkan_MemoryAllocator* pAllocator);

/**
 * @brief Configuration parameters for memory allocation
 */
struct R_CVulkan_MemoryAllocationInfo
{
                VkDeviceSize          size; /**< Size to allocate */
                VkDeviceSize          alignment; /**< Alignment requirement */
                VkBufferUsageFlags    usage; /**< Buffer usage flags */
                VkMemoryPropertyFlags properties; /**< Memory property flags */
};

/**
 * @brief Allocate memory with suballocation
 * @param pAllocator Pointer to allocator
 * @param pAllocInfo Allocation parameters
 * @param outAllocation Pointer to receive the suballocation
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkanError R_CVulkan_MemoryAllocatorAllocate (
    struct R_CVulkan_MemoryAllocator*            pAllocator,
    const struct R_CVulkan_MemoryAllocationInfo* pAllocInfo,
    struct R_CVulkan_Suballocation*              outAllocation);

/**
 * @brief Free a suballocation
 * @param pAllocator Pointer to allocator
 * @param pAllocation Suballocation to free
 */
R_CVULKAN_API void R_CVulkan_MemoryAllocatorFree (
    struct R_CVulkan_MemoryAllocator*     pAllocator,
    const struct R_CVulkan_Suballocation* pAllocation);

/**
 * @brief Find suitable memory type index
 * @param physicalDevice Physical device
 * @param memRequirements Memory requirements
 * @param properties Required memory properties
 * @param outTypeIndex Pointer to receive the memory type index
 * @return R_CVULKAN_OK on success, R_CVULKAN_ERROR_FAILED if not found
 */
R_CVULKAN_API enum R_CVulkanError R_CVulkan_FindMemoryType (
    VkPhysicalDevice            physicalDevice,
    const VkMemoryRequirements* memRequirements,
    VkMemoryPropertyFlags       properties,
    uint32_t*                   outTypeIndex);

/**
 * @brief Copy data to a mapped memory region
 * @param device Vulkan device
 * @param bufferMemory Device memory
 * @param offset Offset in memory
 * @param size Size to copy
 * @param data Pointer to source data
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkanError R_CVulkan_CopyDataToMemory (
    VkDevice       device,
    VkDeviceMemory bufferMemory,
    VkDeviceSize   offset,
    VkDeviceSize   size,
    const void*    data);

/**
 * @brief Get device from allocator
 * @param pAllocator Pointer to allocator
 * @return Vulkan device handle
 */
R_CVULKAN_API VkDevice
R_CVulkan_MemoryAllocatorGetDevice (const struct R_CVulkan_MemoryAllocator* pAllocator);

/**
 * @brief Get physical device from allocator
 * @param pAllocator Pointer to allocator
 * @return Physical device handle
 */
R_CVULKAN_API VkPhysicalDevice
R_CVulkan_MemoryAllocatorGetPhysicalDevice (const struct R_CVulkan_MemoryAllocator* pAllocator);

/**
 * @brief Get total allocated size from allocator
 * @param pAllocator Pointer to allocator
 * @return Total size of all blocks
 */
R_CVULKAN_API VkDeviceSize
R_CVulkan_MemoryAllocatorGetTotalSize (const struct R_CVulkan_MemoryAllocator* pAllocator);

/**
 * @brief Get total used size from allocator
 * @param pAllocator Pointer to allocator
 * @return Total used size across all blocks
 */
R_CVULKAN_API VkDeviceSize
R_CVulkan_MemoryAllocatorGetUsedSize (const struct R_CVulkan_MemoryAllocator* pAllocator);

/**
 * @brief Allocate memory for an image
 * @param device Vulkan device
 * @param physicalDevice Physical device
 * @param image Image to allocate memory for
 * @param properties Required memory properties
 * @param outMemory Pointer to receive the allocated memory handle
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkanError R_CVulkan_MemoryAllocatorAllocateImageMemory (
    VkDevice              device,
    VkPhysicalDevice      physicalDevice,
    VkImage               image,
    VkMemoryPropertyFlags properties,
    VkDeviceMemory*       outMemory);

/**
 * @brief Free image memory
 * @param device Vulkan device
 * @param memory Memory to free
 */
R_CVULKAN_API void R_CVulkan_MemoryAllocatorFreeImageMemory (VkDevice device, VkDeviceMemory memory);

/**
 * @brief Begin defragmentation for the memory allocator
 * @param pAllocator Pointer to allocator
 * @param pContext Pointer to receive defragmentation context
 * @param pConfig Configuration parameters (can be NULL for defaults)
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkanError R_CVulkan_MemoryAllocatorBeginDefragmentation (
    struct R_CVulkan_MemoryAllocator*     pAllocator,
    struct R_CVulkan_DefragContext**      ppContext,
    const struct R_CVulkan_DefragConfig*   pConfig);

/**
 * @brief Execute a defragmentation pass
 * @param pAllocator Pointer to allocator
 * @param pContext Defragmentation context
 * @param commandBuffer Vulkan command buffer (can be VK_NULL_HANDLE for CPU-only)
 * @return R_CVULKAN_OK on success, R_CVULKAN_ERROR_INCOMPLETE if more passes needed, error code otherwise
 */
R_CVULKAN_API enum R_CVulkanError R_CVulkan_MemoryAllocatorExecuteDefragPass (
    struct R_CVulkan_MemoryAllocator* pAllocator,
    struct R_CVulkan_DefragContext*    pContext,
    VkCommandBuffer                    commandBuffer);

/**
 * @brief End defragmentation and cleanup
 * @param pAllocator Pointer to allocator
 * @param pContext Defragmentation context
 * @param pStats Pointer to receive statistics (can be NULL)
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkanError R_CVulkan_MemoryAllocatorEndDefragmentation (
    struct R_CVulkan_MemoryAllocator* pAllocator,
    struct R_CVulkan_DefragContext*  pContext,
    struct R_CVulkan_DefragStats*     pStats);
