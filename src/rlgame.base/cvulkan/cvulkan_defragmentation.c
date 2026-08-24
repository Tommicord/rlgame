#include "rlgame.base/cvulkan/cvulkan_defragmentation.h"
#include "rlgame.base/cvulkan/cvulkan_memory_allocator.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"
#include "rlgame.base/cstl/cstl_trace.h"
#include "rlgame.base/cstl/cstl_log.h"

#include <string.h>
#include <stdio.h>

#ifdef R_CUDA
#include <cuda_runtime.h>
#endif

#ifdef R_OPENCL
#include <CL/cl.h>
extern const uint32_t cvulkanDefragmentation_size;
extern const uint32_t cvulkanDefragmentation_data[];
#endif

#define R_CVULKAN_DEFRAG_DEFAULT_MERGE_FACTOR       3
#define R_CVULKAN_DEFRAG_DEFAULT_MAX_BYTES_PER_PASS (64 * 1024 * 1024)
#define R_CVULKAN_DEFRAG_DEFAULT_MAX_PASSES         10
#define R_CVULKAN_DEFRAG_FRAGMENTATION_THRESHOLD    0.325f

enum R_CVulkan_DefragBackend;
struct R_CVulkan_DefragBlockMetadata;
struct R_CVulkan_DefragMove;
struct R_CVulkan_DefragConfig;
struct R_CVulkan_DefragContext;
struct R_CVulkan_DefragStats;

#if defined(R_CUDA)
extern cudaError_t R_CVulkan_DefragLaunchAnalyzeBlocks (
    void*        blockMetadata,
    uint32_t     blockCount,
    uint32_t     mergeFactor,
    cudaStream_t stream);

extern cudaError_t R_CVulkan_DefragLaunchCreateMovePlan (
    void*        blockMetadata,
    void*        moves,
    uint32_t*    moveCount,
    uint32_t     blockCount,
    uint32_t     mergeFactor,
    uint64_t     maxBytesPerPass,
    cudaStream_t stream);

extern cudaError_t R_CVulkan_DefragCudaLaunchUpdateMetadata (
    void*        blockMetadata,
    void*        moves,
    uint32_t     moveCount,
    uint32_t     blockCount,
    cudaStream_t stream);
#endif

static enum R_CVulkanError R_CVulkan_DefragCollectBlockMetadata (struct R_CVulkan_DefragContext* pContext);

static enum R_CVulkanError R_CVulkan_DefragAnalyzeBlocksCUDA (struct R_CVulkan_DefragContext* pContext);

static enum R_CVulkanError R_CVulkan_DefragAnalyzeBlocksOpenCL (struct R_CVulkan_DefragContext* pContext);

static enum R_CVulkanError R_CVulkan_DefragAnalyzeBlocksCPU (struct R_CVulkan_DefragContext* pContext);

static enum R_CVulkanError R_CVulkan_DefragCreateMovePlanCUDA (struct R_CVulkan_DefragContext* pContext);

static enum R_CVulkanError R_CVulkan_DefragCreateMovePlanOpenCL (struct R_CVulkan_DefragContext* pContext);

static enum R_CVulkanError R_CVulkan_DefragCreateMovePlanCPU (struct R_CVulkan_DefragContext* pContext);

static enum R_CVulkanError
R_CVulkan_DefragExecuteMovesCUDA (struct R_CVulkan_DefragContext* pContext, VkCommandBuffer commandBuffer);

static enum R_CVulkanError
R_CVulkan_DefragExecuteMovesOpenCL (struct R_CVulkan_DefragContext* pContext, VkCommandBuffer commandBuffer);

static enum R_CVulkanError
R_CVulkan_DefragExecuteMovesCPU (struct R_CVulkan_DefragContext* pContext, VkCommandBuffer commandBuffer);

static enum R_CVulkanError R_CVulkan_DefragUpdateMetadataCUDA (struct R_CVulkan_DefragContext* pContext);

static enum R_CVulkanError R_CVulkan_DefragUpdateMetadataOpenCL (struct R_CVulkan_DefragContext* pContext);

static enum R_CVulkanError R_CVulkan_DefragUpdateMetadataCPU (struct R_CVulkan_DefragContext* pContext);

static enum R_CVulkanError R_CVulkan_DefragApplyMovesToAllocator (struct R_CVulkan_DefragContext* pContext);

static enum R_CVulkanError R_CVulkan_DefragEnsureCapacity (
    void**    ppBuffer,
    uint32_t* pCapacity,
    uint32_t  requiredCount,
    uint32_t  elementSize);

#ifdef R_OPENCL
static enum R_CVulkanError R_CVulkan_DefragExecuteKernel (
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
    size_t        globalWorkSize);
static enum R_CVulkanError R_CVulkan_DefragCreateProgram (
    cl_context   context,
    cl_device_id device,
    const void*  pBinaryData,
    size_t       binarySize,
    const char*  kernelName,
    cl_program*  outProgram,
    cl_kernel*   outKernel);
#endif

R_CVULKAN_API void
R_CVulkan_DefragSetDefaultConfig (struct R_CVulkan_DefragConfig* pConfig)
{
    R_CSTL_TRACE_FUNCTION ();

    if (!pConfig)
    {
        return;
    }
    pConfig->mergeFactor = R_CVULKAN_DEFRAG_DEFAULT_MERGE_FACTOR;
    pConfig->maxBytesPerPass = R_CVULKAN_DEFRAG_DEFAULT_MAX_BYTES_PER_PASS;
    pConfig->maxPasses = R_CVULKAN_DEFRAG_DEFAULT_MAX_PASSES;
    pConfig->preferredBackend = R_CVULKAN_DEFRAG_BACKEND_NONE;

    R_CSTL_TRACE_RETURN ();
}

R_CVULKAN_API enum R_CVulkanError
R_CVulkan_DefragGetAvailableBackend (enum R_CVulkan_DefragBackend* pBackend)
{
    R_CSTL_TRACE_FUNCTION ();

    if (!pBackend)
    {
        return R_CVULKAN_ERROR_NULL_POINTER;
    }

#if defined(R_CUDA)
    int         deviceCount = 0;
    cudaError_t cudaError = cudaGetDeviceCount (&deviceCount);
    if (cudaError == cudaSuccess && deviceCount > 0)
    {
        *pBackend = R_CVULKAN_DEFRAG_BACKEND_CUDA;
        R_CSTL_LOG_INFO ("Defrag backend selected: CUDA (%d devices)", deviceCount);
        R_CSTL_TRACE_RETURN ();
        return R_CVULKAN_OK;
    }
#elif defined(R_OPENCL)
    cl_uint platformCount = 0;
    cl_int  clError = clGetPlatformIDs (0, NULL, &platformCount);
    if (clError == CL_SUCCESS && platformCount > 0)
    {
        *pBackend = R_CVULKAN_DEFRAG_BACKEND_OPENCL;
        R_CSTL_LOG_INFO ("Defrag backend selected: OpenCL (%u platforms)", platformCount);
        R_CSTL_TRACE_RETURN ();
        return R_CVULKAN_OK;
    }
#endif

    *pBackend = R_CVULKAN_DEFRAG_BACKEND_CPU;
    R_CSTL_LOG_INFO ("Defrag backend selected: CPU (fallback)");
    R_CSTL_TRACE_RETURN ();
    return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkanError
R_CVulkan_DefragInitialize (
    struct R_CVulkan_DefragContext*      pContext,
    struct R_CVulkan_MemoryAllocator*    pAllocator,
    const struct R_CVulkan_DefragConfig* pConfig)
{
    enum R_CVulkanError result = R_CVULKAN_OK;

    R_CSTL_TRACE_FUNCTION ();

    if (!pContext || !pAllocator)
    {
        return R_CVULKAN_ERROR_NULL_POINTER;
    }
    memset (pContext, 0, sizeof (struct R_CVulkan_DefragContext));

    pContext->pAllocator = pAllocator;

    if (pConfig)
    {
        pContext->config = *pConfig;
    }
    else
    {
        R_CVulkan_DefragSetDefaultConfig (&pContext->config);
    }

    R_CSTL_LOG_INFO (
        "Defrag config: mergeFactor=%u, maxBytesPerPass=%" PRIu64 ", maxPasses=%u",
        pContext->config.mergeFactor,
        pContext->config.maxBytesPerPass,
        pContext->config.maxPasses);

    if (pContext->config.preferredBackend == R_CVULKAN_DEFRAG_BACKEND_NONE)
    {
        result = R_CVulkan_DefragGetAvailableBackend (&pContext->backend);
        if (result != R_CVULKAN_OK)
        {
            goto r_cvulkan_cleanup;
        }
    }
    else
    {
        pContext->backend = pContext->config.preferredBackend;
        R_CSTL_LOG_INFO ("Defrag backend forced: %d", (int)pContext->backend);
    }
    pContext->blockMetadataCapacity = 16;
    pContext->pBlockMetadata
        = R_CSTL_HeapAlloc (pContext->blockMetadataCapacity * sizeof (struct R_CVulkan_DefragBlockMetadata));
    if (!pContext->pBlockMetadata)
    {
        result = R_CVULKAN_ERROR_OUT_OF_MEMORY;
        goto r_cvulkan_cleanup;
    }

    pContext->moveCapacity = 256;
    pContext->pMoves = R_CSTL_HeapAlloc (pContext->moveCapacity * sizeof (struct R_CVulkan_DefragMove));
    if (!pContext->pMoves)
    {
        result = R_CVULKAN_ERROR_OUT_OF_MEMORY;
        goto r_cvulkan_metadata;
    }

#if defined(R_OPENCL)
    if (pContext->backend == R_CVULKAN_DEFRAG_BACKEND_OPENCL)
    {
        cl_platform_id platform = NULL;
        cl_uint        platformCount = 0;
        cl_int         error = clGetPlatformIDs (1, &platform, &platformCount);
        if (error != CL_SUCCESS || platformCount == 0)
        {
            pContext->backend = R_CVULKAN_DEFRAG_BACKEND_CPU;
        }
        else
        {
            cl_device_id device = NULL;
            cl_uint      deviceCount = 0;
            error = clGetDeviceIDs (platform, CL_DEVICE_TYPE_GPU, 1, &device, &deviceCount);
            if (error != CL_SUCCESS || deviceCount == 0)
            {
                pContext->backend = R_CVULKAN_DEFRAG_BACKEND_CPU;
            }
            else
            {
                cl_context context = clCreateContext (NULL, 1, &device, NULL, NULL, &error);
                if (error != CL_SUCCESS || !context)
                {
                    pContext->backend = R_CVULKAN_DEFRAG_BACKEND_CPU;
                }
                else
                {
                    pContext->pBackendContext = context;
                }
            }
        }
    }
#elif defined(R_CUDA)
    if (pContext->backend == R_CVULKAN_DEFRAG_BACKEND_CUDA)
    {
        cudaError_t cudaError = cudaSetDevice (0);
        if (cudaError != cudaSuccess)
        {
            pContext->backend = R_CVULKAN_DEFRAG_BACKEND_CPU;
        }
    }
#endif
    pContext->booted = true;
    R_CSTL_LOG_INFO ("Defrag context initialized successfully");
    R_CSTL_TRACE_RETURN ();
    return R_CVULKAN_OK;

r_cvulkan_cleanup_moves:
    if (pContext->pMoves)
    {
        R_CSTL_HeapFree (pContext->pMoves);
        pContext->pMoves = NULL;
    }
r_cvulkan_metadata:
    if (pContext->pBlockMetadata)
    {
        R_CSTL_HeapFree (pContext->pBlockMetadata);
        pContext->pBlockMetadata = NULL;
    }
r_cvulkan_cleanup:
    return result;
}

R_CVULKAN_API void
R_CVulkan_DefragCleanup (struct R_CVulkan_DefragContext* pContext)
{
    R_CSTL_TRACE_FUNCTION ();

    if (!pContext)
    {
        return;
    }

#if defined(R_OPENCL)
    if (pContext->backend == R_CVULKAN_DEFRAG_BACKEND_OPENCL && pContext->pBackendContext)
    {
        clReleaseContext ((cl_context)pContext->pBackendContext);
        pContext->pBackendContext = NULL;
    }
#endif

    if (pContext->pBlockMetadata)
    {
        R_CSTL_HeapFree (pContext->pBlockMetadata);
        pContext->pBlockMetadata = NULL;
    }

    if (pContext->pMoves)
    {
        R_CSTL_HeapFree (pContext->pMoves);
        pContext->pMoves = NULL;
    }

    pContext->booted = false;
    R_CSTL_TRACE_RETURN ();
}

static enum R_CVulkanError
R_CVulkan_DefragCollectBlockMetadata (struct R_CVulkan_DefragContext* pContext)
{
    enum R_CVulkanError               result = R_CVULKAN_OK;
    struct R_CVulkan_MemoryAllocator* pAllocator = pContext->pAllocator;

    R_CSTL_TRACE_FUNCTION ();

    if (pAllocator->blockCount > pContext->blockMetadataCapacity)
    {
        result = R_CVulkan_DefragEnsureCapacity (
            &pContext->pBlockMetadata,
            &pContext->blockMetadataCapacity,
            pAllocator->blockCount,
            sizeof (struct R_CVulkan_DefragBlockMetadata));
        if (result != R_CVULKAN_OK)
        {
            R_CSTL_TRACE_RETURN ();
            return result;
        }
    }

    pContext->blockMetadataCount = pAllocator->blockCount;

    for (uint32_t i = 0; i < pAllocator->blockCount; ++i)
    {
        struct R_CVulkan_MemoryBlock* block = pAllocator->ppBlocks[i];
        if (!block)
        {
            continue;
        }

        struct R_CVulkan_DefragBlockMetadata* metadata
            = &((struct R_CVulkan_DefragBlockMetadata*)pContext->pBlockMetadata)[i];

        metadata->blockIndex = i;
        metadata->totalSize = block->size;
        metadata->usedSize = block->usedSize;
        metadata->freeSize = block->size - block->usedSize;
        metadata->fillLevel = (block->size > 0) ? ((float)block->usedSize / (float)block->size) : 0.0f;
        metadata->allocationCount = 0;
        metadata->isCandidate = 0;
    }

    R_CSTL_LOG_INFO ("Collected metadata for %u memory blocks", pContext->blockMetadataCount);
    R_CSTL_TRACE_RETURN ();
    return R_CVULKAN_OK;
}

static enum R_CVulkanError
R_CVulkan_DefragAnalyzeBlocksCUDA (struct R_CVulkan_DefragContext* pContext)
{
#if defined(R_CUDA)
    enum R_CVulkanError result = R_CVULKAN_OK;
    cudaError_t         cudaError = cudaSuccess;
    void*               dBlockMetadata = NULL;

    R_CSTL_TRACE_FUNCTION ();
    R_CSTL_LOG_INFO ("Analyzing %u blocks using CUDA backend", pContext->blockMetadataCount);

    cudaError = cudaMalloc (
        &dBlockMetadata,
        pContext->blockMetadataCount * sizeof (struct R_CVulkan_DefragBlockMetadata));
    if (cudaError != cudaSuccess)
    {
        result = R_CVULKAN_ERROR_OUT_OF_MEMORY;
        R_CSTL_LOG_ERROR ("CUDA malloc failed: %d", cudaError);
        goto r_cvulkan_cleanup;
    }

    cudaError = cudaMemcpy (
        dBlockMetadata,
        pContext->pBlockMetadata,
        pContext->blockMetadataCount * sizeof (struct R_CVulkan_DefragBlockMetadata),
        cudaMemcpyHostToDevice);
    if (cudaError != cudaSuccess)
    {
        result = R_CVULKAN_ERROR_FAILED;
        R_CSTL_LOG_ERROR ("CUDA memcpy H2D failed: %d", cudaError);
        goto cleanup_device;
    }

    cudaError = R_CVulkan_DefragLaunchAnalyzeBlocks (
        dBlockMetadata,
        pContext->blockMetadataCount,
        pContext->config.mergeFactor,
        0);
    if (cudaError != cudaSuccess)
    {
        result = R_CVULKAN_ERROR_FAILED;
        R_CSTL_LOG_ERROR ("CUDA kernel launch failed: %d", cudaError);
        goto r_cvulkan_cleanup_device;
    }

    cudaError = cudaDeviceSynchronize ();
    if (cudaError != cudaSuccess)
    {
        result = R_CVULKAN_ERROR_FAILED;
        R_CSTL_LOG_ERROR ("CUDA sync failed: %d", cudaError);
        goto r_cvulkan_cleanup_device;
    }

    cudaError = cudaMemcpy (
        pContext->pBlockMetadata,
        dBlockMetadata,
        pContext->blockMetadataCount * sizeof (struct R_CVulkan_DefragBlockMetadata),
        cudaMemcpyDeviceToHost);
    if (cudaError != cudaSuccess)
    {
        result = R_CVULKAN_ERROR_FAILED;
        R_CSTL_LOG_ERROR ("CUDA memcpy D2H failed: %d", cudaError);
        goto r_cvulkan_cleanup_device;
    }

    R_CSTL_LOG_INFO ("CUDA block analysis completed successfully");
    R_CSTL_TRACE_RETURN ();

r_cvulkan_cleanup_device:
    cudaFree (dBlockMetadata);
r_cvulkan_cleanup:
    return result;
#else
    return R_CVULKAN_ERROR_FAILED;
#endif
}

static enum R_CVulkanError
R_CVulkan_DefragAnalyzeBlocksOpenCL (struct R_CVulkan_DefragContext* pContext)
{
#ifdef R_OPENCL
    enum R_CVulkanError result = R_CVULKAN_OK;
    cl_int              error = CL_SUCCESS;
    cl_context          context = (cl_context)pContext->pBackendContext;
    cl_mem              dBlockMetadata = NULL;

    R_CSTL_TRACE_FUNCTION ();
    R_CSTL_LOG_INFO ("Analyzing %u blocks using OpenCL backend", pContext->blockMetadataCount);

    cl_device_id device = NULL;
    error = clGetContextInfo (context, CL_CONTEXT_DEVICES, sizeof (cl_device_id), &device, NULL);
    if (error != CL_SUCCESS)
    {
        result = R_CVULKAN_ERROR_FAILED;
        R_CSTL_LOG_ERROR ("OpenCL get context info failed: %d", error);
        goto r_cleanup;
    }

    dBlockMetadata = clCreateBuffer (
        context,
        CL_MEM_READ_WRITE,
        pContext->blockMetadataCount * sizeof (struct R_CVulkan_DefragBlockMetadata),
        NULL,
        &error);
    if (error != CL_SUCCESS || !dBlockMetadata)
    {
        result = R_CVULKAN_ERROR_OUT_OF_MEMORY;
        R_CSTL_LOG_ERROR ("OpenCL create buffer failed: %d", error);
        goto r_cleanup;
    }

    cl_command_queue queue = clCreateCommandQueue (context, device, 0, &error);
    if (error != CL_SUCCESS || !queue)
    {
        result = R_CVULKAN_ERROR_FAILED;
        R_CSTL_LOG_ERROR ("OpenCL create command queue failed: %d", error);
        goto r_cleanup_buffer;
    }

    error = clEnqueueWriteBuffer (
        queue,
        dBlockMetadata,
        CL_TRUE,
        0,
        pContext->blockMetadataCount * sizeof (struct R_CVulkan_DefragBlockMetadata),
        pContext->pBlockMetadata,
        0,
        NULL,
        NULL);
    if (error != CL_SUCCESS)
    {
        result = R_CVULKAN_ERROR_FAILED;
        R_CSTL_LOG_ERROR ("OpenCL write buffer failed: %d", error);
        clReleaseCommandQueue (queue);
        goto r_cleanup_buffer;
    }
    clReleaseCommandQueue (queue);

    const char* source = (const char*)cvulkanDefragmentation_data;
    size_t      sourceSize = cvulkanDefragmentation_size * sizeof (uint32_t);

    cl_mem buffers[] = {dBlockMetadata};
    size_t bufferSizes[] = {pContext->blockMetadataCount * sizeof (struct R_CVulkan_DefragBlockMetadata)};
    void*  kernelArgs[] = {&dBlockMetadata, &pContext->blockMetadataCount, &pContext->config.mergeFactor};
    size_t argSizes[] = {sizeof (cl_mem), sizeof (cl_uint), sizeof (cl_uint)};

    result = R_CVulkan_DefragOpenCLExecuteKernel (
        context,
        device,
        "R_CVulkan_DefragAnalyzeBlocksKernel",
        source,
        sourceSize,
        buffers,
        bufferSizes,
        (const void**)kernelArgs,
        argSizes,
        3,
        pContext->blockMetadataCount);
    if (result != R_CVULKAN_OK)
    {
        goto r_cleanup_buffer;
    }

    R_CSTL_LOG_INFO ("OpenCL block analysis completed successfully");
    R_CSTL_TRACE_RETURN ();

r_cleanup_buffer:
    if (dBlockMetadata)
    {
        clReleaseMemObject (dBlockMetadata);
    }
r_cleanup:
    return result;
#else
    return R_CVULKAN_ERROR_FAILED;
#endif
}

static enum R_CVulkanError
R_CVulkan_DefragAnalyzeBlocksCPU (struct R_CVulkan_DefragContext* pContext)
{
    struct R_CVulkan_DefragBlockMetadata* metadataArray
        = (struct R_CVulkan_DefragBlockMetadata*)pContext->pBlockMetadata;
    R_CSTL_TRACE_FUNCTION ();
    R_CSTL_LOG_INFO ("Analyzing %u blocks using CPU backend", pContext->blockMetadataCount);

    float threshold = (float)pContext->config.mergeFactor / (float)(pContext->config.mergeFactor + 1);

    uint32_t candidateCount = 0;
    for (uint32_t i = 0; i < pContext->blockMetadataCount; ++i)
    {
        struct R_CVulkan_DefragBlockMetadata* metadata = &metadataArray[i];

        metadata->fillLevel
            = (metadata->totalSize > 0) ? ((float)metadata->usedSize / (float)metadata->totalSize) : 0.0f;
        metadata->freeSize = metadata->totalSize - metadata->usedSize;

        metadata->isCandidate = (metadata->fillLevel <= threshold && metadata->usedSize > 0) ? 1 : 0;
        if (metadata->isCandidate)
        {
            candidateCount++;
        }
    }

    R_CSTL_LOG_INFO (
        "CPU block analysis completed: %u/%u blocks are candidates",
        candidateCount,
        pContext->blockMetadataCount);
    R_CSTL_TRACE_RETURN ();
    return R_CVULKAN_OK;
}

static enum R_CVulkanError
R_CVulkan_DefragCreateMovePlanCUDA (struct R_CVulkan_DefragContext* pContext)
{
#ifdef R_CUDA
    enum R_CVulkanError result = R_CVULKAN_OK;
    cudaError_t         cudaError = cudaSuccess;
    void*               dBlockMetadata = NULL;
    void*               dMoves = NULL;
    uint32_t*           dMoveCount = NULL;

    R_CSTL_TRACE_FUNCTION ();

    uint32_t maxMoves = pContext->blockMetadataCount;
    if (maxMoves > pContext->moveCapacity)
    {
        result = R_CVulkan_DefragEnsureCapacity (
            &pContext->pMoves,
            &pContext->moveCapacity,
            maxMoves,
            sizeof (struct R_CVulkan_DefragMove));
        if (result != R_CVULKAN_OK)
        {
            R_CSTL_TRACE_RETURN ();
            return result;
        }
    }

    cudaError = cudaMalloc (
        &dBlockMetadata,
        pContext->blockMetadataCount * sizeof (struct R_CVulkan_DefragBlockMetadata));
    if (cudaError != cudaSuccess)
    {
        result = R_CVULKAN_ERROR_OUT_OF_MEMORY;
        R_CSTL_LOG_ERROR ("CUDA malloc failed: %d", cudaError);
        goto cleanup;
    }

    cudaError = cudaMalloc (&dMoves, maxMoves * sizeof (struct R_CVulkan_DefragMove));
    if (cudaError != cudaSuccess)
    {
        result = R_CVULKAN_ERROR_OUT_OF_MEMORY;
        R_CSTL_LOG_ERROR ("CUDA malloc failed: %d", cudaError);
        goto cleanup_metadata;
    }

    cudaError = cudaMalloc (&dMoveCount, sizeof (uint32_t));
    if (cudaError != cudaSuccess)
    {
        result = R_CVULKAN_ERROR_OUT_OF_MEMORY;
        R_CSTL_LOG_ERROR ("CUDA malloc failed: %d", cudaError);
        goto cleanup_moves;
    }

    cudaError = cudaMemcpy (
        dBlockMetadata,
        pContext->pBlockMetadata,
        pContext->blockMetadataCount * sizeof (struct R_CVulkan_DefragBlockMetadata),
        cudaMemcpyHostToDevice);
    if (cudaError != cudaSuccess)
    {
        result = R_CVULKAN_ERROR_FAILED;
        R_CSTL_LOG_ERROR ("CUDA memcpy H2D failed: %d", cudaError);
        goto cleanup_movecount;
    }

    cudaError = R_CVulkan_DefragLaunchCreateMovePlan (
        dBlockMetadata,
        dMoves,
        dMoveCount,
        pContext->blockMetadataCount,
        pContext->config.mergeFactor,
        pContext->config.maxBytesPerPass,
        0);
    if (cudaError != cudaSuccess)
    {
        result = R_CVULKAN_ERROR_FAILED;
        R_CSTL_LOG_ERROR ("CUDA kernel launch failed: %d", cudaError);
        goto cleanup_movecount;
    }

    cudaError = cudaDeviceSynchronize ();
    if (cudaError != cudaSuccess)
    {
        result = R_CVULKAN_ERROR_FAILED;
        R_CSTL_LOG_ERROR ("CUDA sync failed: %d", cudaError);
        goto cleanup_movecount;
    }

    cudaError = cudaMemcpy (&pContext->moveCount, dMoveCount, sizeof (uint32_t), cudaMemcpyDeviceToHost);
    if (cudaError != cudaSuccess)
    {
        result = R_CVULKAN_ERROR_FAILED;
        R_CSTL_LOG_ERROR ("CUDA memcpy D2H failed: %d", cudaError);
        goto cleanup_movecount;
    }

    if (pContext->moveCount > 0)
    {
        cudaError = cudaMemcpy (
            pContext->pMoves,
            dMoves,
            pContext->moveCount * sizeof (struct R_CVulkan_DefragMove),
            cudaMemcpyDeviceToHost);
        if (cudaError != cudaSuccess)
        {
            result = R_CVULKAN_ERROR_FAILED;
            R_CSTL_LOG_ERROR ("CUDA memcpy D2H failed: %d", cudaError);
            goto cleanup_movecount;
        }
    }

    R_CSTL_LOG_INFO ("CUDA move plan created: %u moves planned", pContext->moveCount);
    R_CSTL_TRACE_RETURN ();

cleanup_movecount:
    cudaFree (dMoveCount);
cleanup_moves:
    cudaFree (dMoves);
cleanup_metadata:
    cudaFree (dBlockMetadata);
cleanup:
    return result;
#else
    return R_CVULKAN_ERROR_FAILED;
#endif
}

static enum R_CVulkanError
R_CVulkan_DefragCreateMovePlanOpenCL (struct R_CVulkan_DefragContext* pContext)
{
#if defined(R_OPENCL)
    enum R_CVulkanError result = R_CVULKAN_OK;
    cl_int              error = CL_SUCCESS;
    cl_context          context = (cl_context)pContext->pBackendContext;
    cl_command_queue    queue = NULL;
    cl_mem              dBlockMetadata = NULL;
    cl_mem              dMoves = NULL;
    cl_mem              dMoveCount = NULL;
    cl_program          program = NULL;
    cl_kernel           kernel = NULL;

    R_CSTL_TRACE_FUNCTION ();

    cl_device_id device = NULL;
    error = clGetContextInfo (context, CL_CONTEXT_DEVICES, sizeof (cl_device_id), &device, NULL);
    if (error != CL_SUCCESS)
    {
        result = R_CVULKAN_ERROR_FAILED;
        R_CSTL_LOG_ERROR ("OpenCL get context info failed: %d", error);
        goto r_cleanup;
    }

    queue = clCreateCommandQueue (context, device, 0, &error);
    if (error != CL_SUCCESS || !queue)
    {
        result = R_CVULKAN_ERROR_FAILED;
        R_CSTL_LOG_ERROR ("OpenCL create command queue failed: %d", error);
        goto r_cleanup;
    }

    uint32_t maxMoves = pContext->blockMetadataCount;
    if (maxMoves > pContext->moveCapacity)
    {
        result = R_CVulkan_DefragEnsureCapacity (
            &pContext->pMoves,
            &pContext->moveCapacity,
            maxMoves,
            sizeof (struct R_CVulkan_DefragMove));
        if (result != R_CVULKAN_OK)
        {
            goto r_cleanup_queue;
        }
    }

    dBlockMetadata = clCreateBuffer (
        context,
        CL_MEM_READ_WRITE,
        pContext->blockMetadataCount * sizeof (struct R_CVulkan_DefragBlockMetadata),
        NULL,
        &error);
    if (error != CL_SUCCESS || !dBlockMetadata)
    {
        result = R_CVULKAN_ERROR_OUT_OF_MEMORY;
        R_CSTL_LOG_ERROR ("OpenCL create buffer failed: %d", error);
        goto r_cleanup_queue;
    }

    dMoves = clCreateBuffer (
        context,
        CL_MEM_READ_WRITE,
        maxMoves * sizeof (struct R_CVulkan_DefragMove),
        NULL,
        &error);
    if (error != CL_SUCCESS || !dMoves)
    {
        result = R_CVULKAN_ERROR_OUT_OF_MEMORY;
        R_CSTL_LOG_ERROR ("OpenCL create buffer failed: %d", error);
        goto r_cleanup_buffer_metadata;
    }

    dMoveCount = clCreateBuffer (context, CL_MEM_READ_WRITE, sizeof (uint32_t), NULL, &error);
    if (error != CL_SUCCESS || !dMoveCount)
    {
        result = R_CVULKAN_ERROR_OUT_OF_MEMORY;
        R_CSTL_LOG_ERROR ("OpenCL create buffer failed: %d", error);
        goto r_cleanup_buffer_moves;
    }

    error = clEnqueueWriteBuffer (
        queue,
        dBlockMetadata,
        CL_TRUE,
        0,
        pContext->blockMetadataCount * sizeof (struct R_CVulkan_DefragBlockMetadata),
        pContext->pBlockMetadata,
        0,
        NULL,
        NULL);
    if (error != CL_SUCCESS)
    {
        result = R_CVULKAN_ERROR_FAILED;
        R_CSTL_LOG_ERROR ("OpenCL write buffer failed: %d", error);
        goto r_cleanup_buffer_movecount;
    }

    const void* pBinaryData = (const void*)cvulkanDefragmentation_data;
    size_t      binarySize = cvulkanDefragmentation_size * sizeof (uint32_t);

    result = R_CVulkan_DefragCreateProgram (
        context,
        device,
        pBinaryData,
        binarySize,
        "R_CVulkan_DefragCreateMovePlanKernel",
        &program,
        &kernel);
    if (result != R_CVULKAN_OK)
    {
        goto r_cleanup_buffer_movecount;
    }

    error = clSetKernelArg (kernel, 0, sizeof (cl_mem), &dBlockMetadata);
    error |= clSetKernelArg (kernel, 1, sizeof (cl_mem), &dMoves);
    error |= clSetKernelArg (kernel, 2, sizeof (cl_mem), &dMoveCount);
    error |= clSetKernelArg (kernel, 3, sizeof (cl_uint), &pContext->blockMetadataCount);
    error |= clSetKernelArg (kernel, 4, sizeof (cl_uint), &pContext->config.mergeFactor);
    error |= clSetKernelArg (kernel, 5, sizeof (cl_ulong), &pContext->config.maxBytesPerPass);
    if (error != CL_SUCCESS)
    {
        result = R_CVULKAN_ERROR_FAILED;
        R_CSTL_LOG_ERROR ("OpenCL set kernel args failed: %d", error);
        goto r_cleanup_kernel;
    }

    size_t globalWorkSize = pContext->blockMetadataCount;
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

    error = clEnqueueReadBuffer (
        queue,
        dMoveCount,
        CL_TRUE,
        0,
        sizeof (uint32_t),
        &pContext->moveCount,
        0,
        NULL,
        NULL);
    if (error != CL_SUCCESS)
    {
        result = R_CVULKAN_ERROR_FAILED;
        R_CSTL_LOG_ERROR ("OpenCL read buffer failed: %d", error);
        goto r_cleanup_kernel;
    }

    if (pContext->moveCount > 0)
    {
        error = clEnqueueReadBuffer (
            queue,
            dMoves,
            CL_TRUE,
            0,
            pContext->moveCount * sizeof (struct R_CVulkan_DefragMove),
            pContext->pMoves,
            0,
            NULL,
            NULL);
        if (error != CL_SUCCESS)
        {
            result = R_CVULKAN_ERROR_FAILED;
            R_CSTL_LOG_ERROR ("OpenCL read buffer failed: %d", error);
            goto r_cleanup_kernel;
        }
    }

    R_CSTL_LOG_INFO ("OpenCL move plan created: %u moves planned", pContext->moveCount);
    R_CSTL_TRACE_RETURN ();

r_cleanup_kernel:
    clReleaseKernel (kernel);
r_cleanup_program:
    clReleaseProgram (program);
r_cleanup_buffer_movecount:
    clReleaseMemObject (dMoveCount);
r_cleanup_buffer_moves:
    clReleaseMemObject (dMoves);
r_cleanup_buffer_metadata:
    clReleaseMemObject (dBlockMetadata);
r_cleanup_queue:
    clReleaseCommandQueue (queue);
r_cleanup:
    return result;
#else
    return R_CVULKAN_ERROR_FAILED;
#endif
}

static enum R_CVulkanError
R_CVulkan_DefragCreateMovePlanCPU (struct R_CVulkan_DefragContext* pContext)
{
    struct R_CVulkan_DefragBlockMetadata* metadataArray
        = (struct R_CVulkan_DefragBlockMetadata*)pContext->pBlockMetadata;
    struct R_CVulkan_DefragMove* moves = (struct R_CVulkan_DefragMove*)pContext->pMoves;

    R_CSTL_TRACE_FUNCTION ();

    pContext->moveCount = 0;
    uint64_t bytesSoFar = 0;

    for (uint32_t i = 0; i < pContext->blockMetadataCount; ++i)
    {
        struct R_CVulkan_DefragBlockMetadata* metadata = &metadataArray[i];

        if (!metadata->isCandidate)
        {
            continue;
        }

        uint32_t targetBlockIndex = 0;
        float    bestFillLevel = 0.0f;

        for (uint32_t j = 0; j < pContext->blockMetadataCount; ++j)
        {
            if (i == j)
            {
                continue;
            }

            if (!metadataArray[j].isCandidate)
            {
                continue;
            }

            if (metadataArray[j].fillLevel > bestFillLevel)
            {
                bestFillLevel = metadataArray[j].fillLevel;
                targetBlockIndex = j;
            }
        }

        if (bestFillLevel > 0.0f)
        {
            if (bytesSoFar + metadata->usedSize <= pContext->config.maxBytesPerPass)
            {
                if (pContext->moveCount >= pContext->moveCapacity)
                {
                    enum R_CVulkanError result = R_CVulkan_DefragEnsureCapacity (
                        &pContext->pMoves,
                        &pContext->moveCapacity,
                        pContext->moveCount + 1,
                        sizeof (struct R_CVulkan_DefragMove));
                    if (result != R_CVULKAN_OK)
                    {
                        R_CSTL_LOG_ERROR ("CPU move plan capacity expansion failed: %d", result);
                        R_CSTL_TRACE_RETURN ();
                        return result;
                    }
                    moves = (struct R_CVulkan_DefragMove*)pContext->pMoves;
                }

                moves[pContext->moveCount].srcBlockIndex = i;
                moves[pContext->moveCount].dstBlockIndex = targetBlockIndex;
                moves[pContext->moveCount].srcOffset = 0;
                moves[pContext->moveCount].dstOffset = metadataArray[targetBlockIndex].usedSize;
                moves[pContext->moveCount].size = metadata->usedSize;
                moves[pContext->moveCount].operation = R_CVULKAN_DEFRAG_MOVE_OPERATION_MOVE;

                bytesSoFar += metadata->usedSize;
                pContext->moveCount++;
            }
        }
    }

    R_CSTL_LOG_INFO ("CPU move plan created: %u moves planned", pContext->moveCount);
    R_CSTL_TRACE_RETURN ();
    return R_CVULKAN_OK;
}

static enum R_CVulkanError
R_CVulkan_DefragExecuteMovesCUDA (struct R_CVulkan_DefragContext* pContext, VkCommandBuffer commandBuffer)
{
    (void)pContext;
    (void)commandBuffer;
    return R_CVULKAN_OK;
}

static enum R_CVulkanError
R_CVulkan_DefragExecuteMovesOpenCL (struct R_CVulkan_DefragContext* pContext, VkCommandBuffer commandBuffer)
{
    (void)pContext;
    (void)commandBuffer;
    return R_CVULKAN_OK;
}

static enum R_CVulkanError
R_CVulkan_DefragExecuteMovesCPU (struct R_CVulkan_DefragContext* pContext, VkCommandBuffer commandBuffer)
{
    (void)commandBuffer;
    return R_CVulkan_DefragApplyMovesToAllocator (pContext);
}

static enum R_CVulkanError
R_CVulkan_DefragUpdateMetadataCUDA (struct R_CVulkan_DefragContext* pContext)
{
#ifdef R_CUDA
    enum R_CVulkanError result = R_CVULKAN_OK;
    cudaError_t         cudaError = cudaSuccess;
    void*               dBlockMetadata = NULL;
    void*               dMoves = NULL;

    R_CSTL_TRACE_FUNCTION ();

    cudaError = cudaMalloc (
        &dBlockMetadata,
        pContext->blockMetadataCount * sizeof (struct R_CVulkan_DefragBlockMetadata));
    if (cudaError != cudaSuccess)
    {
        result = R_CVULKAN_ERROR_OUT_OF_MEMORY;
        R_CSTL_LOG_ERROR ("CUDA malloc failed: %d", cudaError);
        goto r_cleanup;
    }
    cudaError = cudaMalloc (&dMoves, pContext->moveCount * sizeof (struct R_CVulkan_DefragMove));
    if (cudaError != cudaSuccess)
    {
        result = R_CVULKAN_ERROR_OUT_OF_MEMORY;
        R_CSTL_LOG_ERROR ("CUDA malloc failed: %d", cudaError);
        goto r_cleanup_metadata;
    }

    cudaError = cudaMemcpy (
        dBlockMetadata,
        pContext->pBlockMetadata,
        pContext->blockMetadataCount * sizeof (struct R_CVulkan_DefragBlockMetadata),
        cudaMemcpyHostToDevice);
    if (cudaError != cudaSuccess)
    {
        result = R_CVULKAN_ERROR_FAILED;
        R_CSTL_LOG_ERROR ("CUDA memcpy H2D failed: %d", cudaError);
        goto r_cleanup_moves;
    }

    cudaError = cudaMemcpy (
        dMoves,
        pContext->pMoves,
        pContext->moveCount * sizeof (struct R_CVulkan_DefragMove),
        cudaMemcpyHostToDevice);
    if (cudaError != cudaSuccess)
    {
        result = R_CVULKAN_ERROR_FAILED;
        R_CSTL_LOG_ERROR ("CUDA memcpy H2D failed: %d", cudaError);
        goto r_cleanup_moves;
    }

    cudaError = R_CVulkan_DefragCudaLaunchUpdateMetadata (
        dBlockMetadata,
        dMoves,
        pContext->moveCount,
        pContext->blockMetadataCount,
        0);
    if (cudaError != cudaSuccess)
    {
        result = R_CVULKAN_ERROR_FAILED;
        R_CSTL_LOG_ERROR ("CUDA kernel launch failed: %d", cudaError);
        goto r_cleanup_moves;
    }

    cudaError = cudaDeviceSynchronize ();
    if (cudaError != cudaSuccess)
    {
        result = R_CVULKAN_ERROR_FAILED;
        R_CSTL_LOG_ERROR ("CUDA sync failed: %d", cudaError);
        goto r_cleanup_moves;
    }

    cudaError = cudaMemcpy (
        pContext->pBlockMetadata,
        dBlockMetadata,
        pContext->blockMetadataCount * sizeof (struct R_CVulkan_DefragBlockMetadata),
        cudaMemcpyDeviceToHost);
    if (cudaError != cudaSuccess)
    {
        result = R_CVULKAN_ERROR_FAILED;
        R_CSTL_LOG_ERROR ("CUDA memcpy D2H failed: %d", cudaError);
        goto r_cleanup_moves;
    }

    R_CSTL_LOG_INFO ("CUDA metadata update completed successfully");
    R_CSTL_TRACE_RETURN ();

r_cleanup_moves:
    cudaFree (dMoves);
r_cleanup_metadata:
    cudaFree (dBlockMetadata);
r_cleanup:
    return result;
#else
    return R_CVULKAN_ERROR_FAILED;
#endif
}

static enum R_CVulkanError
R_CVulkan_DefragUpdateMetadataOpenCL (struct R_CVulkan_DefragContext* pContext)
{
#ifdef R_OPENCL
    enum R_CVulkanError result = R_CVULKAN_OK;
    cl_int              error = CL_SUCCESS;
    cl_context          context = (cl_context)pContext->pBackendContext;
    cl_command_queue    queue = NULL;
    cl_mem              dBlockMetadata = NULL;
    cl_mem              dMoves = NULL;
    cl_program          program = NULL;
    cl_kernel           kernel = NULL;

    R_CSTL_TRACE_FUNCTION ();

    cl_device_id device = NULL;
    error = clGetContextInfo (context, CL_CONTEXT_DEVICES, sizeof (cl_device_id), &device, NULL);
    if (error != CL_SUCCESS)
    {
        result = R_CVULKAN_ERROR_FAILED;
        R_CSTL_LOG_ERROR ("OpenCL get context info failed: %d", error);
        goto r_cleanup;
    }
    queue = clCreateCommandQueue (context, device, 0, &error);
    if (error != CL_SUCCESS || !queue)
    {
        result = R_CVULKAN_ERROR_FAILED;
        R_CSTL_LOG_ERROR ("OpenCL create command queue failed: %d", error);
        goto r_cleanup;
    }
    dBlockMetadata = clCreateBuffer (
        context,
        CL_MEM_READ_WRITE,
        pContext->blockMetadataCount * sizeof (struct R_CVulkan_DefragBlockMetadata),
        NULL,
        &error);
    if (error != CL_SUCCESS || !dBlockMetadata)
    {
        result = R_CVULKAN_ERROR_OUT_OF_MEMORY;
        R_CSTL_LOG_ERROR ("OpenCL create buffer failed: %d", error);
        goto r_cleanup_queue;
    }
    dMoves = clCreateBuffer (
        context,
        CL_MEM_READ_WRITE,
        pContext->moveCount * sizeof (struct R_CVulkan_DefragMove),
        NULL,
        &error);
    if (error != CL_SUCCESS || !dMoves)
    {
        result = R_CVULKAN_ERROR_OUT_OF_MEMORY;
        R_CSTL_LOG_ERROR ("OpenCL create buffer failed: %d", error);
        goto r_cleanup_buffer_metadata;
    }
    error = clEnqueueWriteBuffer (
        queue,
        dBlockMetadata,
        CL_TRUE,
        0,
        pContext->blockMetadataCount * sizeof (struct R_CVulkan_DefragBlockMetadata),
        pContext->pBlockMetadata,
        0,
        NULL,
        NULL);
    if (error != CL_SUCCESS)
    {
        result = R_CVULKAN_ERROR_FAILED;
        R_CSTL_LOG_ERROR ("OpenCL write buffer failed: %d", error);
        goto r_cleanup_buffer_moves;
    }

    error = clEnqueueWriteBuffer (
        queue,
        dMoves,
        CL_TRUE,
        0,
        pContext->moveCount * sizeof (struct R_CVulkan_DefragMove),
        pContext->pMoves,
        0,
        NULL,
        NULL);
    if (error != CL_SUCCESS)
    {
        result = R_CVULKAN_ERROR_FAILED;
        R_CSTL_LOG_ERROR ("OpenCL write buffer failed: %d", error);
        goto r_cleanup_buffer_moves;
    }

    const char* pBinaryData = (const void*)cvulkanDefragmentation_data;
    size_t      binarySize = cvulkanDefragmentation_size * sizeof (uint32_t);

    result = R_CVulkan_DefragCreateProgram (
        context,
        device,
        pBinaryData,
        binarySize,
        "R_CVulkan_DefragUpdateMetadataKernel",
        &program,
        &kernel);
    if (result != R_CVULKAN_OK)
    {
        goto r_cleanup_buffer_moves;
    }

    error = clSetKernelArg (kernel, 0, sizeof (cl_mem), &dBlockMetadata);
    error |= clSetKernelArg (kernel, 1, sizeof (cl_mem), &dMoves);
    error |= clSetKernelArg (kernel, 2, sizeof (cl_uint), &pContext->moveCount);
    error |= clSetKernelArg (kernel, 3, sizeof (cl_uint), &pContext->blockMetadataCount);
    if (error != CL_SUCCESS)
    {
        result = R_CVULKAN_ERROR_FAILED;
        R_CSTL_LOG_ERROR ("OpenCL set kernel args failed: %d", error);
        goto r_cleanup_kernel;
    }

    size_t globalWorkSize = pContext->moveCount;
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

    error = clEnqueueReadBuffer (
        queue,
        dBlockMetadata,
        CL_TRUE,
        0,
        pContext->blockMetadataCount * sizeof (struct R_CVulkan_DefragBlockMetadata),
        pContext->pBlockMetadata,
        0,
        NULL,
        NULL);
    if (error != CL_SUCCESS)
    {
        result = R_CVULKAN_ERROR_FAILED;
        R_CSTL_LOG_ERROR ("OpenCL read buffer failed: %d", error);
        goto r_cleanup_kernel;
    }

    R_CSTL_LOG_INFO ("OpenCL metadata update completed successfully");
    R_CSTL_TRACE_RETURN ();

r_cleanup_kernel:
    if (kernel)
    {
        clReleaseKernel (kernel);
    }
r_cleanup_program:
    if (program)
    {
        clReleaseProgram (program);
    }
r_cleanup_buffer_moves:
    if (dMoves)
    {
        clReleaseMemObject (dMoves);
    }
r_cleanup_buffer_metadata:
    if (dBlockMetadata)
    {
        clReleaseMemObject (dBlockMetadata);
    }
r_cleanup_queue:
    if (queue)
    {
        clReleaseCommandQueue (queue);
    }
r_cleanup:
    return result;
#else
    return R_CVULKAN_ERROR_FAILED;
#endif
}

static enum R_CVulkanError
R_CVulkan_DefragUpdateMetadataCPU (struct R_CVulkan_DefragContext* pContext)
{
    struct R_CVulkan_DefragBlockMetadata* metadataArray
        = (struct R_CVulkan_DefragBlockMetadata*)pContext->pBlockMetadata;
    struct R_CVulkan_DefragMove* moves = (struct R_CVulkan_DefragMove*)pContext->pMoves;

    R_CSTL_TRACE_FUNCTION ();

    for (uint32_t i = 0; i < pContext->moveCount; ++i)
    {
        struct R_CVulkan_DefragMove* move = &moves[i];

        if (move->operation != R_CVULKAN_DEFRAG_MOVE_OPERATION_MOVE)
        {
            continue;
        }

        if (move->srcBlockIndex < pContext->blockMetadataCount)
        {
            metadataArray[move->srcBlockIndex].usedSize -= move->size;
            metadataArray[move->srcBlockIndex].freeSize += move->size;
        }

        if (move->dstBlockIndex < pContext->blockMetadataCount)
        {
            metadataArray[move->dstBlockIndex].usedSize += move->size;
            metadataArray[move->dstBlockIndex].freeSize -= move->size;
        }
    }
    R_CSTL_LOG_INFO ("CPU metadata update completed successfully");
    R_CSTL_TRACE_RETURN ();
    return R_CVULKAN_OK;
}

static enum R_CVulkanError
R_CVulkan_DefragApplyMovesToAllocator (struct R_CVulkan_DefragContext* pContext)
{
    struct R_CVulkan_MemoryAllocator* pAllocator = pContext->pAllocator;
    struct R_CVulkan_DefragMove*      moves = (struct R_CVulkan_DefragMove*)pContext->pMoves;

    for (uint32_t i = 0; i < pContext->moveCount; ++i)
    {
        struct R_CVulkan_DefragMove* move = &moves[i];

        if (move->operation != R_CVULKAN_DEFRAG_MOVE_OPERATION_MOVE)
        {
            continue;
        }

        if (move->srcBlockIndex >= pAllocator->blockCount || move->dstBlockIndex >= pAllocator->blockCount)
        {
            continue;
        }
        struct R_CVulkan_MemoryBlock* srcBlock = pAllocator->ppBlocks[move->srcBlockIndex];
        struct R_CVulkan_MemoryBlock* dstBlock = pAllocator->ppBlocks[move->dstBlockIndex];

        if (!srcBlock || !dstBlock)
        {
            continue;
        }

        if (srcBlock->device != dstBlock->device || srcBlock->properties != dstBlock->properties)
        {
            continue;
        }

        void*    srcMapped = NULL;
        VkResult result
            = vkMapMemory (srcBlock->device, srcBlock->memory, move->srcOffset, move->size, 0, &srcMapped);
        if (result != VK_SUCCESS)
        {
            continue;
        }

        void* dstMapped = NULL;
        result = vkMapMemory (dstBlock->device, dstBlock->memory, move->dstOffset, move->size, 0, &dstMapped);
        if (result != VK_SUCCESS)
        {
            vkUnmapMemory (srcBlock->device, srcBlock->memory);
            continue;
        }

        memcpy (dstMapped, srcMapped, (size_t)move->size);

        vkUnmapMemory (dstBlock->device, dstBlock->memory);
        vkUnmapMemory (srcBlock->device, srcBlock->memory);

        srcBlock->usedSize -= move->size;
        dstBlock->usedSize += move->size;

        pContext->totalBytesMoved += move->size;
        pContext->totalMoves++;
    }

    return R_CVULKAN_OK;
}

#ifdef R_OPENCL
static enum R_CVulkanError
R_CVulkan_DefragOpenCLExecuteKernel (
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
    enum R_CVulkanError result = R_CVULKAN_OK;
    cl_int              error = CL_SUCCESS;
    cl_command_queue    queue = NULL;
    cl_program          program = NULL;
    cl_kernel           kernel = NULL;

    queue = clCreateCommandQueue (context, device, 0, &error);
    if (error != CL_SUCCESS || !queue)
    {
        result = R_CVULKAN_ERROR_FAILED;
        R_CSTL_LOG_ERROR ("OpenCL create command queue failed: %d", error);
        goto r_cleanup;
    }

    result = R_CVulkan_DefragCreateProgram (
        context,
        device,
        pBinaryData,
        binarySize,
        kernelName,
        &program,
        &kernel);
    if (result != R_CVULKAN_OK)
    {
        goto r_cleanup_queue;
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

    for (uint32_t i = 0; i < argCount; ++i)
    {
        if (pBuffers[i])
        {
            error = clEnqueueReadBuffer (
                queue,
                pBuffers[i],
                CL_TRUE,
                0,
                pBufferSizes[i],
                (void*)pKernelArgs[i],
                0,
                NULL,
                NULL);
            if (error != CL_SUCCESS)
            {
                result = R_CVULKAN_ERROR_FAILED;
                R_CSTL_LOG_ERROR ("OpenCL read buffer %u failed: %d", i, error);
                goto r_cleanup_kernel;
            }
        }
    }

r_cleanup_kernel:
    if (kernel)
    {
        clReleaseKernel (kernel);
    }
r_cleanup_program:
    if (program)
    {
        clReleaseProgram (program);
    }
r_cleanup_queue:
    if (queue)
    {
        clReleaseCommandQueue (queue);
    }
r_cleanup:
    return result;
}
#endif

#ifdef R_OPENCL
static enum R_CVulkanError
R_CVulkan_DefragCreateProgram (
    cl_context   context,
    cl_device_id device,
    const void*  pBinaryData,
    size_t       binarySize,
    const char*  kernelName,
    cl_program*  outProgram,
    cl_kernel*   outKernel)
{
    enum R_CVulkanError result = R_CVULKAN_OK;
    cl_int              error = CL_SUCCESS;
    cl_program          program = NULL;
    cl_kernel           kernel = NULL;

    program = clCreateProgramWithIL (context, pBinaryData, binarySize, &error);
    if (error != CL_SUCCESS || !program)
    {
        result = R_CVULKAN_ERROR_FAILED;
        R_CSTL_LOG_ERROR ("OpenCL create program failed: %d", error);
        goto r_cleanup;
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

    *outProgram = program;
    *outKernel = kernel;
    return R_CVULKAN_OK;

r_cleanup_program:
    clReleaseProgram (program);
r_cleanup:
    return result;
}
#endif

static enum R_CVulkanError
R_CVulkan_DefragEnsureCapacity (
    void**    ppBuffer,
    uint32_t* pCapacity,
    uint32_t  requiredCount,
    uint32_t  elementSize)
{
    if (requiredCount <= *pCapacity)
    {
        return R_CVULKAN_OK;
    }

    uint32_t newCapacity = *pCapacity == 0 ? 16 : *pCapacity * 2;
    while (newCapacity < requiredCount)
    {
        newCapacity *= 2;
    }

    void* newBuffer = R_CSTL_HeapRealloc (*ppBuffer, newCapacity * elementSize);
    if (!newBuffer)
    {
        return R_CVULKAN_ERROR_OUT_OF_MEMORY;
    }

    *ppBuffer = newBuffer;
    *pCapacity = newCapacity;

    return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkanError
R_CVulkan_DefragBegin (struct R_CVulkan_DefragContext* pContext)
{
    enum R_CVulkanError result = R_CVULKAN_OK;

    R_CSTL_TRACE_FUNCTION ();

    if (!pContext || !pContext->booted)
    {
        return R_CVULKAN_ERROR_NOT_INITIALIZED;
    }

    pContext->currentPass = 0;
    pContext->totalMoves = 0;
    pContext->totalBytesMoved = 0;
    pContext->moveCount = 0;

    R_CSTL_LOG_INFO ("Beginning defragmentation with backend %d", (int)pContext->backend);

    result = R_CVulkan_DefragCollectBlockMetadata (pContext);
    if (result != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("Failed to collect block metadata: %d", result);
        R_CSTL_TRACE_RETURN ();
        return result;
    }

    switch (pContext->backend)
    {
    case R_CVULKAN_DEFRAG_BACKEND_CUDA:
        result = R_CVulkan_DefragAnalyzeBlocksCUDA (pContext);
        break;
    case R_CVULKAN_DEFRAG_BACKEND_OPENCL:
        result = R_CVulkan_DefragAnalyzeBlocksOpenCL (pContext);
        break;
    case R_CVULKAN_DEFRAG_BACKEND_CPU:
    default:
        result = R_CVulkan_DefragAnalyzeBlocksCPU (pContext);
        break;
    }
    if (result == R_CVULKAN_OK)
    {
        R_CSTL_LOG_INFO ("Defragmentation begin completed successfully");
    }
    else
    {
        R_CSTL_LOG_ERROR ("Block analysis failed: %d", result);
    }

    R_CSTL_TRACE_RETURN ();
    return result;
}

R_CVULKAN_API enum R_CVulkanError
R_CVulkan_DefragExecutePass (struct R_CVulkan_DefragContext* pContext, VkCommandBuffer commandBuffer)
{
    enum R_CVulkanError result = R_CVULKAN_OK;

    R_CSTL_TRACE_FUNCTION ();

    if (!pContext || !pContext->booted)
    {
        return R_CVULKAN_ERROR_NOT_INITIALIZED;
    }

    if (pContext->currentPass >= pContext->config.maxPasses)
    {
        R_CSTL_LOG_INFO ("Max passes (%u) reached, defrag complete", pContext->config.maxPasses);
        R_CSTL_TRACE_RETURN ();
        return R_CVULKAN_OK;
    }

    R_CSTL_LOG_INFO ("Executing defrag pass %u/%u", pContext->currentPass + 1, pContext->config.maxPasses);

    switch (pContext->backend)
    {
    case R_CVULKAN_DEFRAG_BACKEND_CUDA:
        result = R_CVulkan_DefragCreateMovePlanCUDA (pContext);
        if (result != R_CVULKAN_OK)
        {
            R_CSTL_LOG_ERROR ("CUDA move plan creation failed: %d", result);
            R_CSTL_TRACE_RETURN ();
            return result;
        }
        result = R_CVulkan_DefragExecuteMovesCUDA (pContext, commandBuffer);
        if (result != R_CVULKAN_OK)
        {
            R_CSTL_LOG_ERROR ("CUDA move execution failed: %d", result);
            R_CSTL_TRACE_RETURN ();
            return result;
        }
        result = R_CVulkan_DefragUpdateMetadataCUDA (pContext);
        break;
    case R_CVULKAN_DEFRAG_BACKEND_OPENCL:
        result = R_CVulkan_DefragCreateMovePlanOpenCL (pContext);
        if (result != R_CVULKAN_OK)
        {
            R_CSTL_LOG_ERROR ("OpenCL move plan creation failed: %d", result);
            R_CSTL_TRACE_RETURN ();
            return result;
        }
        result = R_CVulkan_DefragExecuteMovesOpenCL (pContext, commandBuffer);
        if (result != R_CVULKAN_OK)
        {
            R_CSTL_LOG_ERROR ("OpenCL move execution failed: %d", result);
            R_CSTL_TRACE_RETURN ();
            return result;
        }
        result = R_CVulkan_DefragUpdateMetadataOpenCL (pContext);
        break;
    case R_CVULKAN_DEFRAG_BACKEND_CPU:
    default:
        result = R_CVulkan_DefragCreateMovePlanCPU (pContext);
        if (result != R_CVULKAN_OK)
        {
            R_CSTL_LOG_ERROR ("CPU move plan creation failed: %d", result);
            R_CSTL_TRACE_RETURN ();
            return result;
        }
        result = R_CVulkan_DefragExecuteMovesCPU (pContext, commandBuffer);
        if (result != R_CVULKAN_OK)
        {
            R_CSTL_LOG_ERROR ("CPU move execution failed: %d", result);
            R_CSTL_TRACE_RETURN ();
            return result;
        }
        result = R_CVulkan_DefragUpdateMetadataCPU (pContext);
        break;
    }

    if (result != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("Metadata update failed: %d", result);
        R_CSTL_TRACE_RETURN ();
        return result;
    }

    pContext->currentPass++;

    R_CSTL_LOG_INFO (
        "Pass %u completed: %u moves, %" PRIu64 " bytes moved this pass",
        pContext->currentPass,
        pContext->moveCount,
        pContext->totalBytesMoved);

    if (pContext->moveCount == 0)
    {
        R_CSTL_LOG_INFO ("No more moves needed, defrag complete");
        R_CSTL_TRACE_RETURN ();
        return R_CVULKAN_OK;
    }

    R_CSTL_LOG_INFO ("More passes needed");
    R_CSTL_TRACE_RETURN ();
    return R_CVULKAN_ERROR_INCOMPLETE;
}

R_CVULKAN_API enum R_CVulkanError
R_CVulkan_DefragEnd (struct R_CVulkan_DefragContext* pContext, struct R_CVulkan_DefragStats* pStats)
{
    R_CSTL_TRACE_FUNCTION ();

    if (!pContext || !pContext->booted)
    {
        return R_CVULKAN_ERROR_NOT_INITIALIZED;
    }

    if (pStats)
    {
        memset (pStats, 0, sizeof (struct R_CVulkan_DefragStats));
        pStats->passesCompleted = pContext->currentPass;
        pStats->totalMoves = pContext->totalMoves;
        pStats->totalBytesMoved = pContext->totalBytesMoved;

        float fragmentationBefore = 0.0f;
        R_CVulkan_DefragGetFragmentationLevel (pContext, &fragmentationBefore);
        pStats->fragmentationBefore = fragmentationBefore;

        float fragmentationAfter = 0.0f;
        R_CVulkan_DefragCollectBlockMetadata (pContext);
        R_CVulkan_DefragGetFragmentationLevel (pContext, &fragmentationAfter);
        pStats->fragmentationAfter = fragmentationAfter;

        R_CSTL_LOG_INFO (
            "Defrag statistics: passes=%u, moves=%u, bytesMoved=%" PRIu64
            ", fragBefore=%.4f%%, fragAfter=%.4f%%",
            pStats->passesCompleted,
            pStats->totalMoves,
            pStats->totalBytesMoved,
            pStats->fragmentationBefore,
            pStats->fragmentationAfter);
    }
    else
    {
        R_CSTL_LOG_INFO (
            "Defrag completed: %u passes, %u moves, %" PRIu64 " bytes moved",
            pContext->currentPass,
            pContext->totalMoves,
            pContext->totalBytesMoved);
    }

    R_CSTL_TRACE_RETURN ();
    return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkanError
R_CVulkan_DefragGetFragmentationLevel (const struct R_CVulkan_DefragContext* pContext, float* pFragmentation)
{
    if (!pContext || !pFragmentation)
    {
        return R_CVULKAN_ERROR_NULL_POINTER;
    }

    if (pContext->blockMetadataCount == 0)
    {
        *pFragmentation = 0.0f;
        return R_CVULKAN_OK;
    }

    struct R_CVulkan_DefragBlockMetadata* metadataArray
        = (struct R_CVulkan_DefragBlockMetadata*)pContext->pBlockMetadata;

    uint64_t totalFree = 0;
    uint64_t totalSize = 0;

    for (uint32_t i = 0; i < pContext->blockMetadataCount; ++i)
    {
        totalFree += metadataArray[i].freeSize;
        totalSize += metadataArray[i].totalSize;
    }

    *pFragmentation = (totalSize > 0) ? ((float)totalFree / (float)totalSize) : 0.0f;

    return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkanError
R_CVulkan_DefragIsNeeded (const struct R_CVulkan_DefragContext* pContext, int* pNeeded)
{
    enum R_CVulkanError result = R_CVULKAN_OK;
    float               fragmentation = 0.0f;

    if (!pContext || !pNeeded)
    {
        return R_CVULKAN_ERROR_NULL_POINTER;
    }

    result = R_CVulkan_DefragGetFragmentationLevel (pContext, &fragmentation);
    if (result != R_CVULKAN_OK)
    {
        return result;
    }

    *pNeeded = (fragmentation > R_CVULKAN_DEFRAG_FRAGMENTATION_THRESHOLD) ? 1 : 0;

    return R_CVULKAN_OK;
}
