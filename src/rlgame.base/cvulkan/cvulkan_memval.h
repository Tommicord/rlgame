
#pragma once

#include <vulkan/vulkan.h>
#include <stdint.h>

#include "rlgame.base/cvulkan/cvulkan_platform.h"

/**
 * @brief Backend type for GPU memory validation
 */
enum R_CVulkan_MemValBackend
{
        R_CVULKAN_MEMVAL_BACKEND_NONE = 0,
        R_CVULKAN_MEMVAL_BACKEND_CUDA,
        R_CVULKAN_MEMVAL_BACKEND_OPENCL,
        R_CVULKAN_MEMVAL_BACKEND_CPU
};

struct R_CVulkan_MemoryAllocator;
struct R_CVulkan_Suballocation;

struct R_CVulkan_MemValStats
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
	uint16_t lastFragmentationLevel;  // Q8.8 fixed-point (0-256 represents 0.0-1.0)
	uint16_t maxFragmentationLevel;   // Q8.8 fixed-point (0-256 represents 0.0-1.0)
	int      defragmentationPending;
	uint64_t failedAllocations;
	uint64_t fragmentedAllocationFailures;
	uint64_t alignedRegions;
	uint64_t misalignedRegions;
	uint16_t health;                  // Q8.8 fixed-point (0-256 represents 0.0-1.0)
	uint16_t defragmentationThreshold; // Q8.8 fixed-point (0-256 represents 0.0-1.0)
};

R_CVULKAN_API enum R_CVulkanError R_CVulkan_MemValInitialize(struct R_CVulkan_MemoryAllocator* pAllocator);
R_CVULKAN_API void R_CVulkan_MemValShutdown(struct R_CVulkan_MemoryAllocator* pAllocator);
R_CVULKAN_API void R_CVulkan_MemValNotifyAllocation(const struct R_CVulkan_MemoryAllocator* pAllocator, const struct R_CVulkan_Suballocation* allocation);
R_CVULKAN_API void R_CVulkan_MemValNotifyFree(const struct R_CVulkan_MemoryAllocator* pAllocator, const struct R_CVulkan_Suballocation* allocation);
R_CVULKAN_API void R_CVulkan_MemValNotifyBlockReserved(const struct R_CVulkan_MemoryAllocator* pAllocator);
R_CVULKAN_API void R_CVulkan_MemValNotifyBlockReleased(const struct R_CVulkan_MemoryAllocator* pAllocator);
R_CVULKAN_API void R_CVulkan_MemValNotifyAllocationFailure (
	const struct R_CVulkan_MemoryAllocator* pAllocator,
	VkDeviceSize                            size,
	VkDeviceSize                            alignment);
R_CVULKAN_API enum R_CVulkanError R_CVulkan_MemValShouldDefragment (
	const struct R_CVulkan_MemoryAllocator* pAllocator,
	int*                                      pNeeded);
R_CVULKAN_API void R_CVulkan_MemValNotifyDefragmentationComplete (
	struct R_CVulkan_MemoryAllocator* pAllocator);
R_CVULKAN_API enum R_CVulkanError R_CVulkan_MemValGetStats (
	const struct R_CVulkan_MemoryAllocator* pAllocator,
	struct R_CVulkan_MemValStats*            pStats);

