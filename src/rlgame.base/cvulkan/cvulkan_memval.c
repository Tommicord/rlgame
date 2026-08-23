#include "rlgame.base/cvulkan/cvulkan_memval.h"
#include "rlgame.base/cvulkan/cvulkan_memory_allocator.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"

#include <string.h>

#define R_CVULKAN_MEMVAL_ALIGNMENT 255u

struct R_CVulkan_MemValState
{
        struct R_CVulkan_MemValStats stats;
};

static struct R_CVulkan_MemValState*
R_CVulkan_MemValGetState (const struct R_CVulkan_MemoryAllocator* pAllocator)
{
        R_CVULKAN_ASSERT (pAllocator != NULL);
        return pAllocator ? pAllocator->pMemVal : NULL;
}

static void
R_CVulkan_MemValRefreshHealth (struct R_CVulkan_MemoryAllocator* pAllocator)
{
        struct R_CVulkan_MemValState* state = R_CVulkan_MemValGetState (pAllocator);
        R_CVULKAN_ASSERT (state != NULL);
        if (!state || !pAllocator) return;
        R_CVULKAN_ASSERT (pAllocator->blockCount == 0 || pAllocator->ppBlocks != NULL);
        if (pAllocator->blockCount > 0 && !pAllocator->ppBlocks) return;

        VkDeviceSize totalFree = 0;
        VkDeviceSize largestFreeRegion = 0;
        uint64_t     alignedRegions = 0;
        uint64_t     misalignedRegions = 0;

        state->stats.activeBlocks = pAllocator->blockCount;
        state->stats.freeRegionCount = 0;
        for (uint32_t blockIndex = 0; blockIndex < pAllocator->blockCount; ++blockIndex)
        {
                struct R_CVulkan_MemoryBlock* block = pAllocator->ppBlocks[blockIndex];
                R_CVULKAN_ASSERT (block != NULL);
                if (!block) continue;

                state->stats.freeRegionCount += block->freeRegionCount;
                for (uint32_t regionIndex = 0; regionIndex < block->freeRegionCount; ++regionIndex)
                {
                        const struct R_CVulkan_FreeRegion* region = &block->pFreeRegions[regionIndex];
                        totalFree += region->size;
                        if (region->size > largestFreeRegion) largestFreeRegion = region->size;
                        if ((region->offset & R_CVULKAN_MEMVAL_ALIGNMENT) == 0
                            && (region->size & R_CVULKAN_MEMVAL_ALIGNMENT) == 0)
                                alignedRegions++;
                        else
                                misalignedRegions++;
                }
        }

        state->stats.alignedRegions = alignedRegions;
        state->stats.misalignedRegions = misalignedRegions;
        state->stats.lastFragmentationLevel
            = totalFree > 0 ? 1.0f - ((float)largestFreeRegion / (float)totalFree) : 0.0f;
        if (state->stats.lastFragmentationLevel > state->stats.maxFragmentationLevel)
                state->stats.maxFragmentationLevel = state->stats.lastFragmentationLevel;

        uint64_t regionCount = alignedRegions + misalignedRegions;
        float alignmentHealth = regionCount > 0 ? (float)alignedRegions / (float)regionCount : 1.0f;
        float failureHealth = 1.0f / (1.0f + (float)state->stats.fragmentedAllocationFailures);
        state->stats.health
            = alignmentHealth * 0.35f + (1.0f - state->stats.lastFragmentationLevel) * 0.45f
              + failureHealth * 0.20f;
        state->stats.defragmentationThreshold = 0.25f + state->stats.health * 0.50f;
        state->stats.defragmentationPending
            = state->stats.lastFragmentationLevel >= state->stats.defragmentationThreshold
              || state->stats.fragmentedAllocationFailures > 0;
}

R_CVULKAN_API enum R_CVulkanError
R_CVulkan_MemValInitialize (struct R_CVulkan_MemoryAllocator* pAllocator)
{
        R_CVULKAN_ASSERT (pAllocator != NULL);
        if (!pAllocator) return R_CVULKAN_ERROR_NULL_POINTER;
        struct R_CVulkan_MemValState* state
            = (struct R_CVulkan_MemValState*)R_CSTL_HeapAlloc (sizeof (*state));
        if (!state) return R_CVULKAN_ERROR_OUT_OF_MEMORY;
        memset (state, 0, sizeof (*state));
        pAllocator->pMemVal = state;
        R_CVulkan_MemValRefreshHealth (pAllocator);
        return R_CVULKAN_OK;
}

R_CVULKAN_API void
R_CVulkan_MemValShutdown (struct R_CVulkan_MemoryAllocator* pAllocator)
{
        R_CVULKAN_ASSERT (pAllocator != NULL);
        if (!pAllocator || !pAllocator->pMemVal) return;
        R_CSTL_HeapFree (pAllocator->pMemVal);
        pAllocator->pMemVal = NULL;
}

R_CVULKAN_API void
R_CVulkan_MemValNotifyAllocation (
    const struct R_CVulkan_MemoryAllocator* pAllocator,
    const struct R_CVulkan_Suballocation*   allocation)
{
        struct R_CVulkan_MemValState* state = R_CVulkan_MemValGetState (pAllocator);
        R_CVULKAN_ASSERT (state != NULL);
        R_CVULKAN_ASSERT (allocation != NULL);
        if (!state || !allocation) return;
        state->stats.totalAllocations++;
        state->stats.activeAllocations++;
        state->stats.totalBytesAllocated += allocation->size;
        state->stats.activeBytes += allocation->size;
        R_CVulkan_MemValRefreshHealth ((struct R_CVulkan_MemoryAllocator*)pAllocator);
}

R_CVULKAN_API void
R_CVulkan_MemValNotifyFree (
    const struct R_CVulkan_MemoryAllocator* pAllocator,
    const struct R_CVulkan_Suballocation*   allocation)
{
        struct R_CVulkan_MemValState* state = R_CVulkan_MemValGetState (pAllocator);
        R_CVULKAN_ASSERT (state != NULL);
        R_CVULKAN_ASSERT (allocation != NULL);
        if (!state || !allocation) return;
        state->stats.totalFrees++;
        state->stats.totalBytesFreed += allocation->size;
        R_CVULKAN_ASSERT (state->stats.activeAllocations > 0);
        R_CVULKAN_ASSERT (state->stats.activeBytes >= allocation->size);
        if (state->stats.activeAllocations > 0) state->stats.activeAllocations--;
        if (state->stats.activeBytes >= allocation->size) state->stats.activeBytes -= allocation->size;
        R_CVulkan_MemValRefreshHealth ((struct R_CVulkan_MemoryAllocator*)pAllocator);
}

R_CVULKAN_API void
R_CVulkan_MemValNotifyBlockReserved (const struct R_CVulkan_MemoryAllocator* pAllocator)
{
        struct R_CVulkan_MemValState* state = R_CVulkan_MemValGetState (pAllocator);
        R_CVULKAN_ASSERT (state != NULL);
        if (!state) return;
        state->stats.totalBlocksReserved++;
        R_CVulkan_MemValRefreshHealth ((struct R_CVulkan_MemoryAllocator*)pAllocator);
}

R_CVULKAN_API void
R_CVulkan_MemValNotifyBlockReleased (const struct R_CVulkan_MemoryAllocator* pAllocator)
{
        struct R_CVulkan_MemValState* state = R_CVulkan_MemValGetState (pAllocator);
        R_CVULKAN_ASSERT (state != NULL);
        if (!state) return;
        state->stats.totalBlocksReleased++;
        R_CVulkan_MemValRefreshHealth ((struct R_CVulkan_MemoryAllocator*)pAllocator);
}

R_CVULKAN_API void
R_CVulkan_MemValNotifyAllocationFailure (
    const struct R_CVulkan_MemoryAllocator* pAllocator,
    VkDeviceSize                            size,
    VkDeviceSize                            alignment)
{
        struct R_CVulkan_MemValState* state = R_CVulkan_MemValGetState (pAllocator);
        R_CVULKAN_ASSERT (state != NULL);
        R_CVULKAN_ASSERT (size > 0);
        R_CVULKAN_ASSERT (alignment > 0);
        if (!state || size == 0 || alignment == 0) return;
        state->stats.failedAllocations++;
        for (uint32_t blockIndex = 0; blockIndex < pAllocator->blockCount; ++blockIndex)
        {
                const struct R_CVulkan_MemoryBlock* block = pAllocator->ppBlocks[blockIndex];
                if (!block) continue;
                VkDeviceSize freeBytes = 0;
                for (uint32_t regionIndex = 0; regionIndex < block->freeRegionCount; ++regionIndex)
                        freeBytes += block->pFreeRegions[regionIndex].size;
                if (freeBytes >= size)
                {
                        state->stats.fragmentedAllocationFailures++;
                        break;
                }
        }
        R_CVulkan_MemValRefreshHealth ((struct R_CVulkan_MemoryAllocator*)pAllocator);
}

R_CVULKAN_API enum R_CVulkanError
R_CVulkan_MemValShouldDefragment (const struct R_CVulkan_MemoryAllocator* pAllocator, int* pNeeded)
{
        R_CVULKAN_ASSERT (pAllocator != NULL);
        R_CVULKAN_ASSERT (pNeeded != NULL);
        if (!pAllocator || !pNeeded) return R_CVULKAN_ERROR_NULL_POINTER;
        if (!pAllocator->pMemVal) return R_CVULKAN_ERROR_NOT_INITIALIZED;
        *pNeeded = pAllocator->pMemVal->stats.defragmentationPending;
        return R_CVULKAN_OK;
}

R_CVULKAN_API void
R_CVulkan_MemValNotifyDefragmentationComplete (struct R_CVulkan_MemoryAllocator* pAllocator)
{
        struct R_CVulkan_MemValState* state = R_CVulkan_MemValGetState (pAllocator);
        R_CVULKAN_ASSERT (state != NULL);
        if (!state) return;
        state->stats.fragmentedAllocationFailures = 0;
        R_CVulkan_MemValRefreshHealth (pAllocator);
}

R_CVULKAN_API enum R_CVulkanError
R_CVulkan_MemValGetStats (
    const struct R_CVulkan_MemoryAllocator* pAllocator,
    struct R_CVulkan_MemValStats*            pStats)
{
        R_CVULKAN_ASSERT (pAllocator != NULL);
        R_CVULKAN_ASSERT (pStats != NULL);
        if (!pAllocator || !pStats) return R_CVULKAN_ERROR_NULL_POINTER;
        if (!pAllocator->pMemVal) return R_CVULKAN_ERROR_NOT_INITIALIZED;
        *pStats = pAllocator->pMemVal->stats;
        return R_CVULKAN_OK;
}
