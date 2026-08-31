#include "rlgame.base/cvulkan/cvulkan_memval.h"
#include "rlgame.base/cvulkan/cvulkan_memory_allocator.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"
#include "rlgame.base/cstl/cstl_log.h"

#include <string.h>

#ifdef R_CUDA
#include <cuda_runtime.h>
#endif

#ifdef R_OPENCL
#include <CL/cl.h>
extern const uint32_t cvulkanMemval_size;
extern const uint32_t cvulkanMemval_data[];
#endif

#ifdef R_CUDA
extern "C" cudaError_t R_CVulkan_MemValLaunchAnalyzeBlocks (
    const void* blocks,
    uint32_t    blockCount,
    const void* blockRegionOffsets,
    uint32_t    totalRegionCount,
    void*       stats,
    void*       stream);
extern "C" cudaError_t R_CVulkan_MemValLaunchGetHealth (
    const void* stats,
    void*       lastFragmentationLevel,
    void*       health,
    void*       defragmentationThreshold,
    void*       defragmentationPending,
    uint64_t    fragmentedAllocationFailures,
    void*       stream);
#endif

#ifdef R_OPENCL
extern cl_int R_CVulkan_MemValOpenCLLaunchAnalyzeBlocks (
    cl_command_queue queue,
    cl_mem           blocks,
    uint32_t         blockCount,
    cl_mem           blockRegionOffsets,
    uint32_t         totalRegionCount,
    cl_mem           stats);
extern cl_int R_CVulkan_MemValOpenCLLaunchGetHealth (
    cl_command_queue queue,
    cl_mem           stats,
    cl_mem           lastFragmentationLevel,
    cl_mem           health,
    cl_mem           defragmentationThreshold,
    cl_mem           defragmentationPending,
    uint64_t         fragmentedAllocationFailures);
#endif

#define R_CVULKAN_MEMVAL_ALIGNMENT         255u
#define R_CVULKAN_MEMVAL_FIXED_POINT_SCALE 256 // Q8.8 fixed-point scale

struct R_CVulkan_MemValRegion
{
        uint64_t offset;
        uint64_t size;
};

struct R_CVulkan_MemValBlock
{
        uint32_t                       freeRegionCount;
        struct R_CVulkan_MemValRegion* pFreeRegions;
};

struct R_CVulkan_MemValStatsGPU
{
        uint64_t totalFree;
        uint64_t largestFreeRegion;
        uint64_t alignedRegions;
        uint64_t misalignedRegions;
        uint64_t freeRegionCount;
};

struct R_CVulkan_MemValState
{
        struct R_CVulkan_MemValStats stats;
        enum R_CVulkan_MemValBackend preferredBackend;
        void*                        pBackendContext;
};

static struct R_CVulkan_MemValState*
R_CVulkan_MemValGetState (const struct R_CVulkan_MemoryAllocator* pAllocator)
{
    R_CVULKAN_ASSERT (pAllocator);
    return pAllocator ? pAllocator->pMemVal : NULL;
}

#ifdef R_OPENCL
static enum R_CVulkan_Error
R_CVulkan_MemValExecuteKernel (
    cl_context    context,
    cl_device_id  device,
    const char*   kernelName,
    const void*   pBinaryData,
    size_t        binarySize,
    const cl_mem* pBuffers,
    const size_t* pBufferSizes,
    const void**  pKernelArgs,
    const size_t* pArgSizes,
    uint32_t      argCount,
    size_t        globalWorkSize)
{
    enum R_CVulkan_Error result = R_CVULKAN_OK;
    cl_int               error = CL_SUCCESS;
    cl_program           program = NULL;
    cl_kernel            kernel = NULL;
    cl_command_queue     queue = NULL;

    queue = clCreateCommandQueue (context, device, 0, &error);
    if (error != CL_SUCCESS || !queue)
    {
        result = R_CVULKAN_ERROR_FAILED;
        R_CSTL_LOG_ERROR ("OpenCL create command queue failed: %d", error);
        goto r_cleanup;
    }

    program = clCreateProgramWithIL (context, pBinaryData, binarySize, &error);
    if (error != CL_SUCCESS || !program)
    {
        result = R_CVULKAN_ERROR_FAILED;
        R_CSTL_LOG_ERROR ("OpenCL create program failed: %d", error);
        goto r_cleanup_queue;
    }

    error = clBuildProgram (program, 1, &device, NULL, NULL, NULL);
    if (error != CL_SUCCESS)
    {
        result = R_CVULKAN_ERROR_FAILED;
        R_CSTL_LOG_ERROR ("OpenCL build program failed: %d", error);
        goto r_cleanup_program;
    }

    kernel = clCreateKernel (program, kernelName, &error);
    if (error != CL_SUCCESS || !kernel)
    {
        result = R_CVULKAN_ERROR_FAILED;
        R_CSTL_LOG_ERROR ("OpenCL create kernel failed: %d", error);
        goto r_cleanup_program;
    }

    for (uint32_t i = 0; i < argCount; ++i)
    {
        error = clSetKernelArg (kernel, i, pArgSizes[i], pKernelArgs[i]);
        if (error != CL_SUCCESS)
        {
            result = R_CVULKAN_ERROR_FAILED;
            R_CSTL_LOG_ERROR ("OpenCL set kernel arg %u failed: %d", i, error);
            goto r_cleanup_kernel;
        }
    }

    error = clEnqueueNDRangeKernel (queue, kernel, 1, NULL, &globalWorkSize, NULL, 0, NULL, NULL);
    if (error != CL_SUCCESS)
    {
        result = R_CVULKAN_ERROR_FAILED;
        R_CSTL_LOG_ERROR ("OpenCL enqueue kernel failed: %d", error);
        goto r_cleanup_kernel;
    }

    error = clFinish (queue);
    if (error != CL_SUCCESS)
    {
        result = R_CVULKAN_ERROR_FAILED;
        R_CSTL_LOG_ERROR ("OpenCL finish failed: %d", error);
        goto r_cleanup_kernel;
    }
r_cleanup_kernel:
    clReleaseKernel (kernel);
r_cleanup_program:
    clReleaseProgram (program);
r_cleanup_queue:
    clReleaseCommandQueue (queue);
r_cleanup:
    return result;
}
#endif

static void
R_CVulkan_MemValRefreshHealthCPU (struct R_CVulkan_MemoryAllocator* pAllocator)
{
    R_CVULKAN_ASSERT (pAllocator);
    R_CVULKAN_ASSERT (pAllocator->ppBlocks);

    struct R_CVulkan_MemValState* state = R_CVulkan_MemValGetState (pAllocator);
    R_CVULKAN_ASSERT (state);
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
        R_CVULKAN_ASSERT (block);
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
            else misalignedRegions++;
        }
    }
    state->stats.alignedRegions = alignedRegions;
    state->stats.misalignedRegions = misalignedRegions;

    // Q8.8 fixed-point: fragmentation = 1.0 - (largestFree / totalFree)
    if (totalFree > 0)
    {
        uint16_t ratio = (uint16_t)((largestFreeRegion * R_CVULKAN_MEMVAL_FIXED_POINT_SCALE) / totalFree);
        state->stats.lastFragmentationLevel = R_CVULKAN_MEMVAL_FIXED_POINT_SCALE - ratio;
    }
    else
    {
        state->stats.lastFragmentationLevel = 0;
    }

    if (state->stats.lastFragmentationLevel > state->stats.maxFragmentationLevel)
        state->stats.maxFragmentationLevel = state->stats.lastFragmentationLevel;

    uint64_t regionCount = alignedRegions + misalignedRegions;

    // Q8.8 fixed-point: alignmentHealth = alignedRegions / regionCount
    uint16_t alignmentHealth
        = regionCount > 0 ? (uint16_t)((alignedRegions * R_CVULKAN_MEMVAL_FIXED_POINT_SCALE) / regionCount)
                          : R_CVULKAN_MEMVAL_FIXED_POINT_SCALE;

    // Q8.8 fixed-point: failureHealth = 1.0 / (1.0 + failures)
    // Using approximation: 256 / (256 + failures) to avoid division
    uint16_t failureHealth;
    uint64_t denominator = R_CVULKAN_MEMVAL_FIXED_POINT_SCALE + state->stats.fragmentedAllocationFailures;
    if (denominator > 0)
        failureHealth = (uint16_t)((R_CVULKAN_MEMVAL_FIXED_POINT_SCALE * R_CVULKAN_MEMVAL_FIXED_POINT_SCALE)
                                   / denominator);
    else failureHealth = R_CVULKAN_MEMVAL_FIXED_POINT_SCALE;

    // Q8.8 fixed-point: health = alignmentHealth * 0.35 + (1.0 - fragmentation) * 0.45 + failureHealth *
    // 0.20 0.35 = 90/256, 0.45 = 115/256, 0.20 = 51/256
    uint16_t fragmentationComplement
        = R_CVULKAN_MEMVAL_FIXED_POINT_SCALE - state->stats.lastFragmentationLevel;
    uint32_t healthCalc = (uint32_t)alignmentHealth * 90 + (uint32_t)fragmentationComplement * 115
                          + (uint32_t)failureHealth * 51;
    state->stats.health = (uint16_t)(healthCalc / R_CVULKAN_MEMVAL_FIXED_POINT_SCALE);

    // Q8.8 fixed-point: defragmentationThreshold = 0.25 + health * 0.50
    // 0.25 = 64/256, 0.50 = 128/256
    uint32_t thresholdCalc = 64 + ((uint32_t)state->stats.health * 128) / R_CVULKAN_MEMVAL_FIXED_POINT_SCALE;
    state->stats.defragmentationThreshold = (uint16_t)thresholdCalc;

    state->stats.defragmentationPending
        = state->stats.lastFragmentationLevel >= state->stats.defragmentationThreshold
          || state->stats.fragmentedAllocationFailures > 0;
}

#ifdef R_CUDA
static void
R_CVulkan_MemValRefreshHealthCUDA (struct R_CVulkan_MemoryAllocator* pAllocator)
{
    R_CVULKAN_ASSERT (pAllocator);
    struct R_CVulkan_MemValState* state = R_CVulkan_MemValGetState (pAllocator);
    R_CVULKAN_ASSERT (state);
    if (pAllocator->blockCount == 0 || !pAllocator->ppBlocks) return;

    cudaError_t cudaError = cudaSuccess;
    void*       dBlocks = NULL;
    void*       dRegions = NULL;
    void*       dBlockRegionOffsets = NULL;
    void*       dStats = NULL;
    void*       dLastFragmentationLevel = NULL;
    void*       dHealth = NULL;
    void*       dDefragmentationThreshold = NULL;
    void*       dDefragmentationPending = NULL;

    uint32_t totalRegionCount = 0;
    for (uint32_t blockIndex = 0; blockIndex < pAllocator->blockCount; ++blockIndex)
    {
        struct R_CVulkan_MemoryBlock* block = pAllocator->ppBlocks[blockIndex];
        if (block) totalRegionCount += block->freeRegionCount;
    }

    if (totalRegionCount == 0)
    {
        R_CVulkan_MemValRefreshHealthCPU (pAllocator);
        return;
    }

    struct R_CVulkan_MemValBlock* hBlocks = (struct R_CVulkan_MemValBlock*)R_CSTL_HeapAlloc (
        pAllocator->blockCount * sizeof (struct R_CVulkan_MemValBlock));
    struct R_CVulkan_MemValRegion* hRegions = (struct R_CVulkan_MemValRegion*)R_CSTL_HeapAlloc (
        totalRegionCount * sizeof (struct R_CVulkan_MemValRegion));
    uint32_t* hBlockRegionOffsets = (uint32_t*)R_CSTL_HeapAlloc (pAllocator->blockCount * sizeof (uint32_t));

    if (!hBlocks || !hRegions || !hBlockRegionOffsets)
    {
        if (hBlocks) R_CSTL_HeapFree (hBlocks);
        if (hRegions) R_CSTL_HeapFree (hRegions);
        if (hBlockRegionOffsets) R_CSTL_HeapFree (hBlockRegionOffsets);
        R_CVulkan_MemValRefreshHealthCPU (pAllocator);
        return;
    }

    uint32_t regionOffset = 0;
    for (uint32_t blockIndex = 0; blockIndex < pAllocator->blockCount; ++blockIndex)
    {
        struct R_CVulkan_MemoryBlock* block = pAllocator->ppBlocks[blockIndex];
        if (!block)
        {
            hBlocks[blockIndex].freeRegionCount = 0;
            hBlocks[blockIndex].pFreeRegions = NULL;
            hBlockRegionOffsets[blockIndex] = regionOffset;
            continue;
        }

        hBlocks[blockIndex].freeRegionCount = block->freeRegionCount;
        hBlocks[blockIndex].pFreeRegions = (struct R_CVulkan_MemValRegion*)(uintptr_t)regionOffset;
        hBlockRegionOffsets[blockIndex] = regionOffset;

        for (uint32_t regionIndex = 0; regionIndex < block->freeRegionCount; ++regionIndex)
        {
            const struct R_CVulkan_FreeRegion* region = &block->pFreeRegions[regionIndex];
            hRegions[regionOffset].offset = region->offset;
            hRegions[regionOffset].size = region->size;
            regionOffset++;
        }
    }

    cudaError = cudaMalloc (&dBlocks, pAllocator->blockCount * sizeof (struct R_CVulkan_MemValBlock));
    if (cudaError != cudaSuccess) goto r_cleanup_host;

    cudaError = cudaMalloc (&dRegions, totalRegionCount * sizeof (struct R_CVulkan_MemValRegion));
    if (cudaError != cudaSuccess) goto r_cleanup_device_blocks;

    cudaError = cudaMalloc (&dBlockRegionOffsets, pAllocator->blockCount * sizeof (uint32_t));
    if (cudaError != cudaSuccess) goto r_cleanup_device_regions;

    cudaError = cudaMalloc (&dStats, sizeof (struct R_CVulkan_MemValStatsGPU));
    if (cudaError != cudaSuccess) goto r_cleanup_device_offsets;

    cudaError = cudaMalloc (&dLastFragmentationLevel, sizeof (uint16_t));
    if (cudaError != cudaSuccess) goto r_cleanup_device_stats;

    cudaError = cudaMalloc (&dHealth, sizeof (uint16_t));
    if (cudaError != cudaSuccess) goto r_cleanup_device_frag;

    cudaError = cudaMalloc (&dDefragmentationThreshold, sizeof (uint16_t));
    if (cudaError != cudaSuccess) goto r_cleanup_device_health;

    cudaError = cudaMalloc (&dDefragmentationPending, sizeof (int));
    if (cudaError != cudaSuccess) goto r_cleanup_device_threshold;

    cudaError = cudaMemcpy (
        dBlocks,
        hBlocks,
        pAllocator->blockCount * sizeof (struct R_CVulkan_MemValBlock),
        cudaMemcpyHostToDevice);
    if (cudaError != cudaSuccess) goto r_cleanup_device_pending;

    cudaError = cudaMemcpy (
        dRegions,
        hRegions,
        totalRegionCount * sizeof (struct R_CVulkan_MemValRegion),
        cudaMemcpyHostToDevice);
    if (cudaError != cudaSuccess) goto r_cleanup_device_pending;

    cudaError = cudaMemcpy (
        dBlockRegionOffsets,
        hBlockRegionOffsets,
        pAllocator->blockCount * sizeof (uint32_t),
        cudaMemcpyHostToDevice);
    if (cudaError != cudaSuccess) goto r_cleanup_device_pending;

    cudaMemset (dStats, 0, sizeof (struct R_CVulkan_MemValStatsGPU));

    cudaError = R_CVulkan_MemValLaunchAnalyzeBlocks (
        dBlocks,
        pAllocator->blockCount,
        dBlockRegionOffsets,
        totalRegionCount,
        dStats,
        0);
    if (cudaError != cudaSuccess) goto r_cleanup_device_pending;

    cudaError = cudaDeviceSynchronize ();
    if (cudaError != cudaSuccess) goto r_cleanup_device_pending;

    cudaError = R_CVulkan_MemValLaunchGetHealth (
        dStats,
        dLastFragmentationLevel,
        dHealth,
        dDefragmentationThreshold,
        dDefragmentationPending,
        state->stats.fragmentedAllocationFailures,
        0);
    if (cudaError != cudaSuccess) goto r_cleanup_device_pending;

    cudaError = cudaDeviceSynchronize ();
    if (cudaError != cudaSuccess) goto r_cleanup_device_pending;

    struct R_CVulkan_MemValStatsGPU hStats;
    cudaError
        = cudaMemcpy (&hStats, dStats, sizeof (struct R_CVulkan_MemValStatsGPU), cudaMemcpyDeviceToHost);
    if (cudaError != cudaSuccess) goto r_cleanup_device_pending;

    cudaError = cudaMemcpy (
        &state->stats.lastFragmentationLevel,
        dLastFragmentationLevel,
        sizeof (uint16_t),
        cudaMemcpyDeviceToHost);
    if (cudaError != cudaSuccess) goto r_cleanup_device_pending;

    cudaError = cudaMemcpy (&state->stats.health, dHealth, sizeof (uint16_t), cudaMemcpyDeviceToHost);
    if (cudaError != cudaSuccess) goto r_cleanup_device_pending;

    cudaError = cudaMemcpy (
        &state->stats.defragmentationThreshold,
        dDefragmentationThreshold,
        sizeof (uint16_t),
        cudaMemcpyDeviceToHost);
    if (cudaError != cudaSuccess) goto r_cleanup_device_pending;

    cudaError = cudaMemcpy (
        &state->stats.defragmentationPending,
        dDefragmentationPending,
        sizeof (int),
        cudaMemcpyDeviceToHost);
    if (cudaError != cudaSuccess) goto r_cleanup_device_pending;

    state->stats.activeBlocks = pAllocator->blockCount;
    state->stats.freeRegionCount = hStats.freeRegionCount;
    state->stats.alignedRegions = hStats.alignedRegions;
    state->stats.misalignedRegions = hStats.misalignedRegions;

    if (state->stats.lastFragmentationLevel > state->stats.maxFragmentationLevel)
        state->stats.maxFragmentationLevel = state->stats.lastFragmentationLevel;

    R_CSTL_LOG_INFO ("CUDA health refresh completed successfully");

r_cleanup_device_pending:
    cudaFree (dDefragmentationPending);
r_cleanup_device_threshold:
    cudaFree (dDefragmentationThreshold);
r_cleanup_device_health:
    cudaFree (dHealth);
r_cleanup_device_frag:
    cudaFree (dLastFragmentationLevel);
r_cleanup_device_stats:
    cudaFree (dStats);
r_cleanup_device_offsets:
    cudaFree (dBlockRegionOffsets);
r_cleanup_device_regions:
    cudaFree (dRegions);
r_cleanup_device_blocks:
    cudaFree (dBlocks);
r_cleanup_host:
    R_CSTL_HeapFree (hBlocks);
    R_CSTL_HeapFree (hRegions);
    R_CSTL_HeapFree (hBlockRegionOffsets);

    if (cudaError != cudaSuccess)
    {
        R_CSTL_LOG_ERROR ("CUDA health refresh failed: %d, falling back to CPU", cudaError);
        R_CVulkan_MemValRefreshHealthCPU (pAllocator);
    }
}
#endif

#ifdef R_OPENCL
static void
R_CVulkan_MemValRefreshHealthOpenCL (struct R_CVulkan_MemoryAllocator* pAllocator)
{
    R_CVULKAN_ASSERT (pAllocator);
    struct R_CVulkan_MemValState* state = R_CVulkan_MemValGetState (pAllocator);
    R_CVULKAN_ASSERT (state);
    if (pAllocator->blockCount == 0 || !pAllocator->ppBlocks) return;

    cl_int           error = CL_SUCCESS;
    cl_context       context = (cl_context)state->pBackendContext;
    cl_command_queue queue = NULL;
    cl_mem           dBlocks = NULL;
    cl_mem           dRegions = NULL;
    cl_mem           dBlockRegionOffsets = NULL;
    cl_mem           dStats = NULL;
    cl_mem           dLastFragmentationLevel = NULL;
    cl_mem           dHealth = NULL;
    cl_mem           dDefragmentationThreshold = NULL;
    cl_mem           dDefragmentationPending = NULL;

    uint32_t totalRegionCount = 0;
    for (uint32_t blockIndex = 0; blockIndex < pAllocator->blockCount; ++blockIndex)
    {
        struct R_CVulkan_MemoryBlock* block = pAllocator->ppBlocks[blockIndex];
        if (block) totalRegionCount += block->freeRegionCount;
    }

    if (totalRegionCount == 0)
    {
        R_CVulkan_MemValRefreshHealthCPU (pAllocator);
        return;
    }

    cl_device_id device = NULL;
    error = clGetContextInfo (context, CL_CONTEXT_DEVICES, sizeof (cl_device_id), &device, NULL);
    if (error != CL_SUCCESS)
    {
        R_CSTL_LOG_ERROR ("OpenCL get context info failed: %d", error);
        R_CVulkan_MemValRefreshHealthCPU (pAllocator);
        return;
    }

    queue = clCreateCommandQueue (context, device, 0, &error);
    if (error != CL_SUCCESS || !queue)
    {
        R_CSTL_LOG_ERROR ("OpenCL create command queue failed: %d", error);
        R_CVulkan_MemValRefreshHealthCPU (pAllocator);
        return;
    }

    struct R_CVulkan_MemValBlock* hBlocks = (struct R_CVulkan_MemValBlock*)R_CSTL_HeapAlloc (
        pAllocator->blockCount * sizeof (struct R_CVulkan_MemValBlock));
    struct R_CVulkan_MemValRegion* hRegions = (struct R_CVulkan_MemValRegion*)R_CSTL_HeapAlloc (
        totalRegionCount * sizeof (struct R_CVulkan_MemValRegion));
    uint32_t* hBlockRegionOffsets = (uint32_t*)R_CSTL_HeapAlloc (pAllocator->blockCount * sizeof (uint32_t));

    if (!hBlocks || !hRegions || !hBlockRegionOffsets)
    {
        if (hBlocks) R_CSTL_HeapFree (hBlocks);
        if (hRegions) R_CSTL_HeapFree (hRegions);
        if (hBlockRegionOffsets) R_CSTL_HeapFree (hBlockRegionOffsets);
        clReleaseCommandQueue (queue);
        R_CVulkan_MemValRefreshHealthCPU (pAllocator);
        return;
    }

    uint32_t regionOffset = 0;
    for (uint32_t blockIndex = 0; blockIndex < pAllocator->blockCount; ++blockIndex)
    {
        struct R_CVulkan_MemoryBlock* block = pAllocator->ppBlocks[blockIndex];
        if (!block)
        {
            hBlocks[blockIndex].freeRegionCount = 0;
            hBlocks[blockIndex].pFreeRegions = NULL;
            hBlockRegionOffsets[blockIndex] = regionOffset;
            continue;
        }

        hBlocks[blockIndex].freeRegionCount = block->freeRegionCount;
        hBlocks[blockIndex].pFreeRegions = (struct R_CVulkan_MemValRegion*)(uintptr_t)regionOffset;
        hBlockRegionOffsets[blockIndex] = regionOffset;

        for (uint32_t regionIndex = 0; regionIndex < block->freeRegionCount; ++regionIndex)
        {
            const struct R_CVulkan_FreeRegion* region = &block->pFreeRegions[regionIndex];
            hRegions[regionOffset].offset = region->offset;
            hRegions[regionOffset].size = region->size;
            regionOffset++;
        }
    }

    dBlocks = clCreateBuffer (
        context,
        CL_MEM_READ_WRITE,
        pAllocator->blockCount * sizeof (struct R_CVulkan_MemValBlock),
        NULL,
        &error);
    if (error != CL_SUCCESS) goto r_cleanup_host;

    dRegions = clCreateBuffer (
        context,
        CL_MEM_READ_WRITE,
        totalRegionCount * sizeof (struct R_CVulkan_MemValRegion),
        NULL,
        &error);
    if (error != CL_SUCCESS) goto r_cleanup_blocks;

    dBlockRegionOffsets = clCreateBuffer (
        context,
        CL_MEM_READ_WRITE,
        pAllocator->blockCount * sizeof (uint32_t),
        NULL,
        &error);
    if (error != CL_SUCCESS) goto r_cleanup_regions;

    dStats
        = clCreateBuffer (context, CL_MEM_READ_WRITE, sizeof (struct R_CVulkan_MemValStatsGPU), NULL, &error);
    if (error != CL_SUCCESS) goto r_cleanup_offsets;

    dLastFragmentationLevel = clCreateBuffer (context, CL_MEM_READ_WRITE, sizeof (uint16_t), NULL, &error);
    if (error != CL_SUCCESS) goto r_cleanup_stats;

    dHealth = clCreateBuffer (context, CL_MEM_READ_WRITE, sizeof (uint16_t), NULL, &error);
    if (error != CL_SUCCESS) goto r_cleanup_frag;

    dDefragmentationThreshold = clCreateBuffer (context, CL_MEM_READ_WRITE, sizeof (uint16_t), NULL, &error);
    if (error != CL_SUCCESS) goto r_cleanup_health;

    dDefragmentationPending = clCreateBuffer (context, CL_MEM_READ_WRITE, sizeof (int), NULL, &error);
    if (error != CL_SUCCESS) goto r_cleanup_threshold;

    error = clEnqueueWriteBuffer (
        queue,
        dBlocks,
        CL_TRUE,
        0,
        pAllocator->blockCount * sizeof (struct R_CVulkan_MemValBlock),
        hBlocks,
        0,
        NULL,
        NULL);
    if (error != CL_SUCCESS) goto r_cleanup_pending;

    error = clEnqueueWriteBuffer (
        queue,
        dRegions,
        CL_TRUE,
        0,
        totalRegionCount * sizeof (struct R_CVulkan_MemValRegion),
        hRegions,
        0,
        NULL,
        NULL);
    if (error != CL_SUCCESS) goto r_cleanup_pending;

    error = clEnqueueWriteBuffer (
        queue,
        dBlockRegionOffsets,
        CL_TRUE,
        0,
        pAllocator->blockCount * sizeof (uint32_t),
        hBlockRegionOffsets,
        0,
        NULL,
        NULL);
    if (error != CL_SUCCESS) goto r_cleanup_pending;

    clEnqueueWriteBuffer (
        queue,
        dStats,
        CL_TRUE,
        0,
        sizeof (struct R_CVulkan_MemValStatsGPU),
        NULL,
        0,
        NULL,
        NULL);

    const void* binaryData = (const void*)cvulkanMemval_data;
    size_t      binarySize = cvulkanMemval_size * sizeof (uint32_t);

    cl_mem buffers[] = {dBlocks, dRegions, dBlockRegionOffsets, dStats};
    size_t bufferSizes[]
        = {pAllocator->blockCount * sizeof (struct R_CVulkan_MemValBlock),
           totalRegionCount * sizeof (struct R_CVulkan_MemValRegion),
           pAllocator->blockCount * sizeof (uint32_t),
           sizeof (struct R_CVulkan_MemValStatsGPU)};
    void* kernelArgs[]
        = {&dBlocks, &pAllocator->blockCount, &dBlockRegionOffsets, &totalRegionCount, &dStats};
    size_t argSizes[]
        = {sizeof (cl_mem), sizeof (cl_uint), sizeof (cl_mem), sizeof (cl_uint), sizeof (cl_mem)};

    enum R_CVulkan_Error result = R_CVulkan_MemValExecuteKernel (
        context,
        device,
        "R_CVulkan_MemValAnalyzeBlocksKernel",
        binaryData,
        binarySize,
        buffers,
        bufferSizes,
        (const void**)kernelArgs,
        argSizes,
        5,
        totalRegionCount);
    if (result != R_CVULKAN_OK) goto r_cleanup_pending;

    cl_mem healthBuffers[]
        = {dStats, dLastFragmentationLevel, dHealth, dDefragmentationThreshold, dDefragmentationPending};
    size_t healthBufferSizes[]
        = {sizeof (struct R_CVulkan_MemValStatsGPU),
           sizeof (uint16_t),
           sizeof (uint16_t),
           sizeof (uint16_t),
           sizeof (int)};
    void* healthKernelArgs[]
        = {&dStats,
           &dLastFragmentationLevel,
           &dHealth,
           &dDefragmentationThreshold,
           &dDefragmentationPending,
           &state->stats.fragmentedAllocationFailures};
    size_t healthArgSizes[]
        = {sizeof (cl_mem),
           sizeof (cl_mem),
           sizeof (cl_mem),
           sizeof (cl_mem),
           sizeof (cl_mem),
           sizeof (cl_ulong)};

    result = R_CVulkan_MemValExecuteKernel (
        context,
        device,
        "R_CVulkan_MemValGetHealthKernel",
        binaryData,
        binarySize,
        healthBuffers,
        healthBufferSizes,
        (const void**)healthKernelArgs,
        healthArgSizes,
        6,
        1);
    if (result != R_CVULKAN_OK) goto r_cleanup_pending;

    struct R_CVulkan_MemValStatsGPU hStats;
    error = clEnqueueReadBuffer (
        queue,
        dStats,
        CL_TRUE,
        0,
        sizeof (struct R_CVulkan_MemValStatsGPU),
        &hStats,
        0,
        NULL,
        NULL);
    if (error != CL_SUCCESS) goto r_cleanup_pending;

    error = clEnqueueReadBuffer (
        queue,
        dLastFragmentationLevel,
        CL_TRUE,
        0,
        sizeof (uint16_t),
        &state->stats.lastFragmentationLevel,
        0,
        NULL,
        NULL);
    if (error != CL_SUCCESS) goto r_cleanup_pending;

    error = clEnqueueReadBuffer (
        queue,
        dHealth,
        CL_TRUE,
        0,
        sizeof (uint16_t),
        &state->stats.health,
        0,
        NULL,
        NULL);
    if (error != CL_SUCCESS) goto r_cleanup_pending;

    error = clEnqueueReadBuffer (
        queue,
        dDefragmentationThreshold,
        CL_TRUE,
        0,
        sizeof (uint16_t),
        &state->stats.defragmentationThreshold,
        0,
        NULL,
        NULL);
    if (error != CL_SUCCESS) goto r_cleanup_pending;

    error = clEnqueueReadBuffer (
        queue,
        dDefragmentationPending,
        CL_TRUE,
        0,
        sizeof (int),
        &state->stats.defragmentationPending,
        0,
        NULL,
        NULL);
    if (error != CL_SUCCESS) goto r_cleanup_pending;

    state->stats.activeBlocks = pAllocator->blockCount;
    state->stats.freeRegionCount = hStats.freeRegionCount;
    state->stats.alignedRegions = hStats.alignedRegions;
    state->stats.misalignedRegions = hStats.misalignedRegions;

    if (state->stats.lastFragmentationLevel > state->stats.maxFragmentationLevel)
        state->stats.maxFragmentationLevel = state->stats.lastFragmentationLevel;

    R_CSTL_LOG_INFO ("OpenCL health refresh completed successfully");

r_cleanup_pending:
    clReleaseMemObject (dDefragmentationPending);
r_cleanup_threshold:
    clReleaseMemObject (dDefragmentationThreshold);
r_cleanup_health:
    clReleaseMemObject (dHealth);
r_cleanup_frag:
    clReleaseMemObject (dLastFragmentationLevel);
r_cleanup_stats:
    clReleaseMemObject (dStats);
r_cleanup_offsets:
    clReleaseMemObject (dBlockRegionOffsets);
r_cleanup_regions:
    clReleaseMemObject (dRegions);
r_cleanup_blocks:
    clReleaseMemObject (dBlocks);
r_cleanup_host:
    R_CSTL_HeapFree (hBlocks);
    R_CSTL_HeapFree (hRegions);
    R_CSTL_HeapFree (hBlockRegionOffsets);
    clReleaseCommandQueue (queue);

    if (error != CL_SUCCESS || result != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("OpenCL health refresh failed: %d, falling back to CPU", error);
        R_CVulkan_MemValRefreshHealthCPU (pAllocator);
    }
}
#endif

static void
R_CVulkan_MemValRefreshHealth (struct R_CVulkan_MemoryAllocator* pAllocator)
{
    struct R_CVulkan_MemValState* state = R_CVulkan_MemValGetState (pAllocator);
    R_CVULKAN_ASSERT (state);
    if (!state) return;

#ifdef R_CUDA
    R_CVulkan_MemValRefreshHealthCUDA (pAllocator);
#endif
#ifdef R_OPENCL
    R_CVulkan_MemValRefreshHealthOpenCL (pAllocator);
#endif
    R_CVulkan_MemValRefreshHealthCPU (pAllocator);
}

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_MemValInitialize (struct R_CVulkan_MemoryAllocator* pAllocator)
{
    R_CVULKAN_ASSERT (pAllocator);
    if (!pAllocator) return R_CVULKAN_ERROR_NULL_POINTER;
    struct R_CVulkan_MemValState* state = (struct R_CVulkan_MemValState*)R_CSTL_HeapAlloc (sizeof (*state));
    if (!state) return R_CVULKAN_ERROR_OUT_OF_MEMORY;
    memset (state, 0, sizeof (*state));
    state->preferredBackend = R_CVULKAN_MEMVAL_BACKEND_CPU;
#if defined(R_CUDA)
    int         deviceCount = 0;
    cudaError_t cudaError = cudaGetDeviceCount (&deviceCount);
    if (cudaError == cudaSuccess && deviceCount > 0)
    {
        state->preferredBackend = R_CVULKAN_MEMVAL_BACKEND_CUDA;
        cudaError = cudaSetDevice (0);
        R_CSTL_LOG_INFO ("MemVal backend selected: CUDA (%d devices)", deviceCount);
    }
    else
    {
        R_CSTL_LOG_INFO ("MemVal backend: CUDA not available, falling back to CPU");
    }
#elif defined(R_OPENCL)
    cl_uint platformCount = 0;
    cl_int  clError = clGetPlatformIDs (0, NULL, &platformCount);
    if (clError == CL_SUCCESS && platformCount > 0)
    {
        cl_platform_id platform = NULL;
        clError = clGetPlatformIDs (1, &platform, &platformCount);
        if (clError == CL_SUCCESS && platformCount > 0)
        {
            cl_device_id device = NULL;
            cl_uint      deviceCount = 0;
            clError = clGetDeviceIDs (platform, CL_DEVICE_TYPE_GPU, 1, &device, &deviceCount);
            if (clError == CL_SUCCESS && deviceCount > 0)
            {
                cl_context context = clCreateContext (NULL, 1, &device, NULL, NULL, &clError);
                if (clError == CL_SUCCESS && context)
                {
                    state->preferredBackend = R_CVULKAN_MEMVAL_BACKEND_OPENCL;
                    state->pBackendContext = context;
                    R_CSTL_LOG_INFO ("MemVal backend selected: OpenCL (%u platforms)", platformCount);
                }
            }
        }
    }
    if (state->preferredBackend == R_CVULKAN_MEMVAL_BACKEND_CPU)
    {
        R_CSTL_LOG_INFO ("MemVal backend: OpenCL not available, falling back to CPU");
    }
#endif
    pAllocator->pMemVal = state;
    R_CVulkan_MemValRefreshHealth (pAllocator);
    return R_CVULKAN_OK;
}

R_CVULKAN_API void
R_CVulkan_MemValShutdown (struct R_CVulkan_MemoryAllocator* pAllocator)
{
    R_CVULKAN_ASSERT (pAllocator);
    R_CVULKAN_ASSERT (pAllocator->pMemVal);
    struct R_CVulkan_MemValState* state = (struct R_CVulkan_MemValState*)pAllocator->pMemVal;

#if defined(R_OPENCL)
    clReleaseContext ((cl_context)state->pBackendContext);
#endif

    R_CSTL_HeapFree (pAllocator->pMemVal);
    pAllocator->pMemVal = NULL;
}

R_CVULKAN_API void
R_CVulkan_MemValNotifyAllocation (
    const struct R_CVulkan_MemoryAllocator* pAllocator,
    const struct R_CVulkan_Suballocation*   allocation)
{
    struct R_CVulkan_MemValState* state = R_CVulkan_MemValGetState (pAllocator);
    R_CVULKAN_ASSERT (state);
    R_CVULKAN_ASSERT (allocation);
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
    R_CVULKAN_ASSERT (state);
    R_CVULKAN_ASSERT (allocation);

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
    R_CVULKAN_ASSERT (state);
    state->stats.totalBlocksReserved++;
    R_CVulkan_MemValRefreshHealth ((struct R_CVulkan_MemoryAllocator*)pAllocator);
}

R_CVULKAN_API void
R_CVulkan_MemValNotifyBlockReleased (const struct R_CVulkan_MemoryAllocator* pAllocator)
{
    struct R_CVulkan_MemValState* state = R_CVulkan_MemValGetState (pAllocator);
    R_CVULKAN_ASSERT (state);
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
    R_CVULKAN_ASSERT (state);
    R_CVULKAN_ASSERT (size > 0);
    R_CVULKAN_ASSERT (alignment > 0);

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

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_MemValShouldDefragment (const struct R_CVulkan_MemoryAllocator* pAllocator, int* pNeeded)
{
#if defined(R_CVULKAN_DEBUG)
    if (!pAllocator || !pNeeded) return R_CVULKAN_ERROR_NULL_POINTER;
    if (!pAllocator->pMemVal) return R_CVULKAN_ERROR_NOT_INITIALIZED;
#endif
    *pNeeded = pAllocator->pMemVal->stats.defragmentationPending;
    return R_CVULKAN_OK;
}

R_CVULKAN_API void
R_CVulkan_MemValNotifyDefragmentationComplete (struct R_CVulkan_MemoryAllocator* pAllocator)
{
    struct R_CVulkan_MemValState* state = R_CVulkan_MemValGetState (pAllocator);
    R_CVULKAN_ASSERT (state);
    state->stats.fragmentedAllocationFailures = 0;
    R_CVulkan_MemValRefreshHealth (pAllocator);
}

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_MemValGetStats (
    const struct R_CVulkan_MemoryAllocator* pAllocator,
    struct R_CVulkan_MemValStats*           pStats)
{
#if defined(R_CVULKAN_DEBUG)
    if (!pAllocator || !pStats) return R_CVULKAN_ERROR_NULL_POINTER;
    if (!pAllocator->pMemVal) return R_CVULKAN_ERROR_NOT_INITIALIZED;
#endif
    *pStats = pAllocator->pMemVal->stats;
    return R_CVULKAN_OK;
}
