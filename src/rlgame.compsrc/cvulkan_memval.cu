#include <cuda_runtime.h>
#include <device_launch_parameters.h>

#define R_CVULKAN_MEMVAL_FIXED_POINT_SCALE 256
#define R_CVULKAN_MEMVAL_ALIGNMENT 255

/**
 * @brief Region data structure for GPU processing
 */
struct R_CVulkan_MemValRegion
{
    uint64_t offset;
    uint64_t size;
};

/**
 * @brief Block data structure for GPU processing
 */
struct R_CVulkan_MemValBlock
{
    uint32_t freeRegionCount;
    struct R_CVulkan_MemValRegion* pFreeRegions;
};

/**
 * @brief Statistics structure for GPU processing
 */
struct R_CVulkan_MemValStatsGPU
{
    uint64_t totalFree;
    uint64_t largestFreeRegion;
    uint64_t alignedRegions;
    uint64_t misalignedRegions;
    uint64_t freeRegionCount;
};

/**
 * @brief CUDA kernel for analyzing regions in parallel across all blocks
 * @param blocks Array of block data
 * @param blockCount Number of blocks
 * @param blockRegionOffsets Array of starting region indices for each block
 * @param totalRegionCount Total number of regions across all blocks
 * @param stats Output statistics (atomic accumulation)
 */
__global__ void R_CVulkan_MemValAnalyzeBlocksKernel (
    const struct R_CVulkan_MemValBlock* blocks,
    uint32_t blockCount,
    const uint32_t* blockRegionOffsets,
    uint32_t totalRegionCount,
    struct R_CVulkan_MemValStatsGPU* stats)
{
    uint32_t globalRegionIndex = blockIdx.x * blockDim.x + threadIdx.x;

    if (globalRegionIndex >= totalRegionCount)
    {
        return;
    }

    uint32_t blockIndex = 0;
    uint32_t low = 0;
    uint32_t high = blockCount - 1;
    
    while (low <= high)
    {
        uint32_t mid = (low + high) / 2;
        if (blockRegionOffsets[mid] <= globalRegionIndex)
        {
            blockIndex = mid;
            low = mid + 1;
        }
        else
        {
            high = mid - 1;
        }
    }

    const struct R_CVulkan_MemValBlock* block = &blocks[blockIndex];
    uint32_t regionIndex = globalRegionIndex - blockRegionOffsets[blockIndex];

    if (regionIndex >= block->freeRegionCount)
    {
        return;
    }

    const struct R_CVulkan_MemValRegion* region = &block->pFreeRegions[regionIndex];
    
    atomicAdd ((unsigned long long*)&stats->totalFree, region->size);
    atomicMax ((unsigned long long*)&stats->largestFreeRegion, region->size);
    
    if ((region->offset & R_CVULKAN_MEMVAL_ALIGNMENT) == 0
        && (region->size & R_CVULKAN_MEMVAL_ALIGNMENT) == 0)
        atomicAdd ((unsigned long long*)&stats->alignedRegions, 1);
    else
        atomicAdd ((unsigned long long*)&stats->misalignedRegions, 1);
}

/**
 * @brief CUDA kernel for calculating health metrics from statistics
 * @param stats Input statistics
 * @param lastFragmentationLevel Output fragmentation level (Q8.8)
 * @param health Output health metric (Q8.8)
 * @param defragmentationThreshold Output threshold (Q8.8)
 * @param defragmentationPending Output defragmentation needed flag
 * @param fragmentedAllocationFailures Number of fragmented allocation failures
 */
__global__ void R_CVulkan_MemValCalculateHealthKernel (
    const struct R_CVulkan_MemValStatsGPU* stats,
    uint16_t* lastFragmentationLevel,
    uint16_t* health,
    uint16_t* defragmentationThreshold,
    int* defragmentationPending,
    uint64_t fragmentedAllocationFailures)
{
    if (threadIdx.x != 0 || blockIdx.x != 0)
    {
        return;
    }

    // Q8.8 fixed-point: fragmentation = 1.0 - (largestFree / totalFree)
    if (stats->totalFree > 0)
    {
        uint16_t ratio = (uint16_t)((stats->largestFreeRegion * R_CVULKAN_MEMVAL_FIXED_POINT_SCALE) / stats->totalFree);
        *lastFragmentationLevel = R_CVULKAN_MEMVAL_FIXED_POINT_SCALE - ratio;
    }
    else
    {
        *lastFragmentationLevel = 0;
    }

    uint64_t regionCount = stats->alignedRegions + stats->misalignedRegions;
    
    // Q8.8 fixed-point: alignmentHealth = alignedRegions / regionCount
    uint16_t alignmentHealth = regionCount > 0 
        ? (uint16_t)((stats->alignedRegions * R_CVULKAN_MEMVAL_FIXED_POINT_SCALE) / regionCount) 
        : R_CVULKAN_MEMVAL_FIXED_POINT_SCALE;
    
    // Q8.8 fixed-point: failureHealth = 1.0 / (1.0 + failures)
    uint16_t failureHealth;
    uint64_t denominator = R_CVULKAN_MEMVAL_FIXED_POINT_SCALE + fragmentedAllocationFailures;
    if (denominator > 0)
        failureHealth = (uint16_t)((R_CVULKAN_MEMVAL_FIXED_POINT_SCALE * R_CVULKAN_MEMVAL_FIXED_POINT_SCALE) / denominator);
    else
        failureHealth = R_CVULKAN_MEMVAL_FIXED_POINT_SCALE;
    
    // Q8.8 fixed-point: health = alignmentHealth * 0.35 + (1.0 - fragmentation) * 0.45 + failureHealth * 0.20
    // 0.35 = 90/256, 0.45 = 115/256, 0.20 = 51/256
    uint16_t fragmentationComplement = R_CVULKAN_MEMVAL_FIXED_POINT_SCALE - *lastFragmentationLevel;
    uint32_t healthCalc = (uint32_t)alignmentHealth * 90 + (uint32_t)fragmentationComplement * 115 + (uint32_t)failureHealth * 51;
    *health = (uint16_t)(healthCalc / R_CVULKAN_MEMVAL_FIXED_POINT_SCALE);
    
    // Q8.8 fixed-point: defragmentationThreshold = 0.25 + health * 0.50
    // 0.25 = 64/256, 0.50 = 128/256
    uint32_t thresholdCalc = 64 + ((uint32_t)*health * 128) / R_CVULKAN_MEMVAL_FIXED_POINT_SCALE;
    *defragmentationThreshold = (uint16_t)thresholdCalc;
    
    // Integer comparison for threshold check
    *defragmentationPending = (*lastFragmentationLevel >= *defragmentationThreshold || fragmentedAllocationFailures > 0) ? 1 : 0;
}

/**
 * @brief Host function to launch block analysis kernel
 */
extern "C" cudaError_t R_CVulkan_MemValLaunchAnalyzeBlocks (
    const void* blocks,
    uint32_t blockCount,
    const void* blockRegionOffsets,
    uint32_t totalRegionCount,
    void* stats,
    cudaStream_t stream)
{
    int blockSize = 256;
    int gridSize = (totalRegionCount + blockSize - 1) / blockSize;

    R_CVulkan_MemValAnalyzeBlocksKernel<<<gridSize, blockSize, 0, stream>>> (
        (const struct R_CVulkan_MemValBlock*)blocks,
        blockCount,
        (const uint32_t*)blockRegionOffsets,
        totalRegionCount,
        (struct R_CVulkan_MemValStatsGPU*)stats);

    return cudaGetLastError ();
}

/**
 * @brief Host function to launch health calculation kernel
 */
extern "C" cudaError_t R_CVulkan_MemValLaunchCalculateHealth (
    const void* stats,
    void* lastFragmentationLevel,
    void* health,
    void* defragmentationThreshold,
    void* defragmentationPending,
    uint64_t fragmentedAllocationFailures,
    cudaStream_t stream)
{
    R_CVulkan_MemValCalculateHealthKernel<<<1, 1, 0, stream>>> (
        (const struct R_CVulkan_MemValStatsGPU*)stats,
        (uint16_t*)lastFragmentationLevel,
        (uint16_t*)health,
        (uint16_t*)defragmentationThreshold,
        (int*)defragmentationPending,
        fragmentedAllocationFailures);

    return cudaGetLastError ();
}
