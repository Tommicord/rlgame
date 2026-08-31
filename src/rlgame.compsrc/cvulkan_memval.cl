#define R_CVULKAN_MEMVAL_FIXED_POINT_SCALE 256
#define R_CVULKAN_MEMVAL_ALIGNMENT 255

/**
 * @brief Region data structure for GPU processing
 */
struct r_cvulkan_mem_val_region
{
    ulong offset;
    ulong size;
};

/**
 * @brief Block data structure for GPU processing
 */
struct r_cvulkan_mem_val_block
{
    uint freeRegionCount;
    __global struct r_cvulkan_mem_val_region* pFreeRegions;
};

/**
 * @brief Statistics structure for GPU processing
 */
struct r_cvulkan_mem_val_statsGPU
{
    ulong totalFree;
    ulong largestFreeRegion;
    ulong alignedRegions;
    ulong misalignedRegions;
    ulong freeRegionCount;
};

/**
 * @brief OpenCL kernel for analyzing regions in parallel across all blocks
 * @param blocks Array of block data
 * @param blockCount Number of blocks
 * @param blockRegionOffsets Array of starting region indices for each block
 * @param totalRegionCount Total number of regions across all blocks
 * @param stats Output statistics (atomic accumulation)
 */
__kernel void r_cvulkan_mem_val_analyze_blocks_kernel (
    __global const struct r_cvulkan_mem_val_block* blocks,
    const uint blockCount,
    __global const uint* blockRegionOffsets,
    const uint totalRegionCount,
    __global struct r_cvulkan_mem_val_statsGPU* stats)
{
    uint globalRegionIndex = get_global_id (0);

    if (globalRegionIndex >= totalRegionCount)
    {
        return;
    }

    uint blockIndex = 0;
    uint low = 0;
    uint high = blockCount - 1;
    
    while (low <= high)
    {
        uint mid = (low + high) / 2;
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

    __global const struct r_cvulkan_mem_val_block* block = &blocks[blockIndex];
    uint regionIndex = globalRegionIndex - blockRegionOffsets[blockIndex];

    if (regionIndex >= block->freeRegionCount)
    {
        return;
    }

    __global const struct r_cvulkan_mem_val_region* region = &block->pFreeRegions[regionIndex];
    
    atomic_add (&stats->totalFree, region->size);
    atomic_max (&stats->largestFreeRegion, region->size);
    
    if ((region->offset & R_CVULKAN_MEMVAL_ALIGNMENT) == 0
        && (region->size & R_CVULKAN_MEMVAL_ALIGNMENT) == 0)
        atomic_add (&stats->alignedRegions, 1);
    else
        atomic_add (&stats->misalignedRegions, 1);
}

/**
 * @brief OpenCL kernel for calculating health metrics from statistics
 * @param stats Input statistics
 * @param lastFragmentationLevel Output fragmentation level (Q8.8)
 * @param health Output health metric (Q8.8)
 * @param defragmentationThreshold Output threshold (Q8.8)
 * @param defragmentationPending Output defragmentation needed flag
 * @param fragmentedAllocationFailures Number of fragmented allocation failures
 */
__kernel void r_cvulkan_mem_val_calculate_health_kernel (
    __global const struct r_cvulkan_mem_val_statsGPU* stats,
    __global ushort* lastFragmentationLevel,
    __global ushort* health,
    __global ushort* defragmentationThreshold,
    __global int* defragmentationPending,
    const ulong fragmentedAllocationFailures)
{
    // Single thread execution for this kernel
    if (get_global_id (0) != 0)
    {
        return;
    }

    // Q8.8 fixed-point: fragmentation = 1.0 - (largestFree / totalFree)
    if (stats->totalFree > 0)
    {
        ushort ratio = (ushort)((stats->largestFreeRegion * R_CVULKAN_MEMVAL_FIXED_POINT_SCALE) / stats->totalFree);
        *lastFragmentationLevel = R_CVULKAN_MEMVAL_FIXED_POINT_SCALE - ratio;
    }
    else
    {
        *lastFragmentationLevel = 0;
    }

    ulong regionCount = stats->alignedRegions + stats->misalignedRegions;
    
    // Q8.8 fixed-point: alignmentHealth = alignedRegions / regionCount
    ushort alignmentHealth = regionCount > 0 
        ? (ushort)((stats->alignedRegions * R_CVULKAN_MEMVAL_FIXED_POINT_SCALE) / regionCount) 
        : R_CVULKAN_MEMVAL_FIXED_POINT_SCALE;
    
    // Q8.8 fixed-point: failureHealth = 1.0 / (1.0 + failures)
    ushort failureHealth;
    ulong denominator = R_CVULKAN_MEMVAL_FIXED_POINT_SCALE + fragmentedAllocationFailures;
    if (denominator > 0)
        failureHealth = (ushort)((R_CVULKAN_MEMVAL_FIXED_POINT_SCALE * R_CVULKAN_MEMVAL_FIXED_POINT_SCALE) / denominator);
    else
        failureHealth = R_CVULKAN_MEMVAL_FIXED_POINT_SCALE;
    
    // Q8.8 fixed-point: health = alignmentHealth * 0.35 + (1.0 - fragmentation) * 0.45 + failureHealth * 0.20
    // 0.35 = 90/256, 0.45 = 115/256, 0.20 = 51/256
    ushort fragmentationComplement = R_CVULKAN_MEMVAL_FIXED_POINT_SCALE - *lastFragmentationLevel;
    uint healthCalc = (uint)alignmentHealth * 90 + (uint)fragmentationComplement * 115 + (uint)failureHealth * 51;
    *health = (ushort)(healthCalc / R_CVULKAN_MEMVAL_FIXED_POINT_SCALE);
    
    // Q8.8 fixed-point: defragmentationThreshold = 0.25 + health * 0.50
    // 0.25 = 64/256, 0.50 = 128/256
    uint thresholdCalc = 64 + ((uint)*health * 128) / R_CVULKAN_MEMVAL_FIXED_POINT_SCALE;
    *defragmentationThreshold = (ushort)thresholdCalc;
    
    // Integer comparison for threshold check
    *defragmentationPending = (*lastFragmentationLevel >= *defragmentationThreshold || fragmentedAllocationFailures > 0) ? 1 : 0;
}
