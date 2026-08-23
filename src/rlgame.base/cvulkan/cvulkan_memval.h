
#pragma once

#include <vulkan/vulkan.h>
#include <stdint.h>

#include "rlgame.base/cvulkan/cvulkan_memory_allocator.h"
#include "rlgame.base/cvulkan/cvulkan_defragmentation.h"

struct R_CVulkan_MemValStats
{
	uint64_t totalAllocations;
	uint64_t totalFrees;
	uint64_t totalBytesAllocated;
	uint64_t totalBytesFreed;
	float    lastFragmentationLevel;
	float    maxFragmentationLevel;
};

R_CVULKAN_API enum R_CVulkanError R_CVulkan_MemValInitialize(struct R_CVulkan_MemoryAllocator* pAllocator);
R_CVULKAN_API void R_CVulkan_MemValShutdown(void);
R_CVULKAN_API void R_CVulkan_MemValNotifyAllocation(const struct R_CVulkan_MemoryAllocator* pAllocator, const struct R_CVulkan_Suballocation* allocation);
R_CVULKAN_API void R_CVulkan_MemValNotifyFree(const struct R_CVulkan_MemoryAllocator* pAllocator, const struct R_CVulkan_Suballocation* allocation);
R_CVULKAN_API void R_CVulkan_MemValGetStats(struct R_CVulkan_MemValStats* pStats);

