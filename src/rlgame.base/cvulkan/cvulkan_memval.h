
#pragma once

#include <vulkan/vulkan.h>
#include <stdint.h>

#include "rlgame.base/cvulkan/cvulkan_platform.h"

/**
 * @brief Backend type for GPU memory validation
 */
enum r_cvulkan_mem_val_backend
{
    R_CVULKAN_MEMVAL_BACKEND_NONE = 0,
    R_CVULKAN_MEMVAL_BACKEND_CUDA,
    R_CVULKAN_MEMVAL_BACKEND_OPENCL,
    R_CVULKAN_MEMVAL_BACKEND_CPU
};

struct R_CVulkan_MemoryAllocator;
struct R_CVulkan_Suballocation;

struct r_cvulkan_mem_val_stats
{
        uint64_t totalAllocations;
        uint64_t totalFrees;
        uint64_t totalBytesAllocated;
        uint64_t totalBytesFreed;
        uint64_t activeAllocations;
        uint64_t activeBytes;
        uint64_t totalBlocksReserved;
        uint64_t totalBlocksReleased;
        uint64_t activeBlocks;
        uint64_t freeRegionCount;
        uint16_t lastFragmentationLevel; // Q8.8 fixed-point (0-256 represents 0.0-1.0)
        uint16_t maxFragmentationLevel; // Q8.8 fixed-point (0-256 represents 0.0-1.0)
        int      defragmentationPending;
        uint64_t failedAllocations;
        uint64_t fragmentedAllocationFailures;
        uint64_t alignedRegions;
        uint64_t misalignedRegions;
        uint16_t health; // Q8.8 fixed-point (0-256 represents 0.0-1.0)
        uint16_t defragmentationThreshold; // Q8.8 fixed-point (0-256 represents 0.0-1.0)
};

R_CVULKAN_API enum R_CVulkan_Error r_cvulkan_mem_val_initialize (struct R_CVulkan_MemoryAllocator* pAllocator);
R_CVULKAN_API void                 r_cvulkan_mem_val_shutdown (struct R_CVulkan_MemoryAllocator* pAllocator);
R_CVULKAN_API void                 r_cvulkan_mem_val_notify_allocation (
                    const struct R_CVulkan_MemoryAllocator* pAllocator,
                    const struct R_CVulkan_Suballocation*   allocation);
R_CVULKAN_API void r_cvulkan_mem_val_notify_free (
    const struct R_CVulkan_MemoryAllocator* pAllocator,
    const struct R_CVulkan_Suballocation*   allocation);
R_CVULKAN_API void r_cvulkan_mem_val_notify_block_reserved (const struct R_CVulkan_MemoryAllocator* pAllocator);
R_CVULKAN_API void r_cvulkan_mem_val_notify_block_released (const struct R_CVulkan_MemoryAllocator* pAllocator);
R_CVULKAN_API void r_cvulkan_mem_val_notify_allocation_failure (
    const struct R_CVulkan_MemoryAllocator* pAllocator,
    VkDeviceSize                            size,
    VkDeviceSize                            alignment);
R_CVULKAN_API enum R_CVulkan_Error
r_cvulkan_mem_val_should_defragment (const struct R_CVulkan_MemoryAllocator* pAllocator, int* pNeeded);
R_CVULKAN_API void
r_cvulkan_mem_val_notify_defragmentation_complete (struct R_CVulkan_MemoryAllocator* pAllocator);
R_CVULKAN_API enum R_CVulkan_Error r_cvulkan_mem_val_get_stats (
    const struct R_CVulkan_MemoryAllocator* pAllocator,
    struct r_cvulkan_mem_val_stats*           pStats);
