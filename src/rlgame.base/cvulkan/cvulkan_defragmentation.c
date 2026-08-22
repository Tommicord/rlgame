#include "rlgame.base/cvulkan/cvulkan_defragmentation.h"
#include "rlgame.base/cvulkan/cvulkan_memory_allocator.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"

#include <string.h>
#include <stdio.h>

#ifdef R_CVULKAN_DEFRAG_CUDA_ENABLED
#include <cuda_runtime.h>
#endif

#ifdef R_CVULKAN_DEFRAG_OPENCL_ENABLED
#include <CL/cl.h>
#endif

#ifdef R_CVULKAN_DEFRAG_OPENCL_ENABLED
extern const uint32_t cvulkanDefragmentationCl_size;
extern const uint32_t cvulkanDefragmentationCl_data[];
#endif

#define R_CVULKAN_DEFRAG_DEFAULT_MERGE_FACTOR 3
#define R_CVULKAN_DEFRAG_DEFAULT_MAX_BYTES_PER_PASS (64 * 1024 * 1024)
#define R_CVULKAN_DEFRAG_DEFAULT_MAX_PASSES 10
#define R_CVULKAN_DEFRAG_FRAGMENTATION_THRESHOLD 0.3f

enum R_CVulkan_DefragBackend;
struct R_CVulkan_DefragBlockMetadata;
struct R_CVulkan_DefragMove;
struct R_CVulkan_DefragConfig;
struct R_CVulkan_DefragContext;
struct R_CVulkan_DefragStats;

#ifdef R_CVULKAN_DEFRAG_CUDA_ENABLED
extern cudaError_t R_CVulkan_DefragLaunchAnalyzeBlocks (
    void*      blockMetadata,
    uint32_t   blockCount,
    uint32_t   mergeFactor,
    cudaStream_t stream);

extern cudaError_t R_CVulkan_DefragLaunchCreateMovePlan (
    void*      blockMetadata,
    void*      moves,
    uint32_t*  moveCount,
    uint32_t   blockCount,
    uint32_t   mergeFactor,
    uint64_t   maxBytesPerPass,
    cudaStream_t stream);

extern cudaError_t R_CVulkan_DefragCudaLaunchUpdateMetadata (
    void*      blockMetadata,
    void*      moves,
    uint32_t   moveCount,
    uint32_t   blockCount,
    cudaStream_t stream);
#endif

static enum R_CVulkanError R_CVulkan_DefragCollectBlockMetadata (
    R_CVulkan_DefragContext* pContext);

static enum R_CVulkanError R_CVulkan_DefragAnalyzeBlocksCUDA (
    R_CVulkan_DefragContext* pContext);

static enum R_CVulkanError R_CVulkan_DefragAnalyzeBlocksOpenCL (
    R_CVulkan_DefragContext* pContext);

static enum R_CVulkanError R_CVulkan_DefragAnalyzeBlocksCPU (
    R_CVulkan_DefragContext* pContext);

static enum R_CVulkanError R_CVulkan_DefragCreateMovePlanCUDA (
    R_CVulkan_DefragContext* pContext);

static enum R_CVulkanError R_CVulkan_DefragCreateMovePlanOpenCL (
    R_CVulkan_DefragContext* pContext);

static enum R_CVulkanError R_CVulkan_DefragCreateMovePlanCPU (
    R_CVulkan_DefragContext* pContext);

static enum R_CVulkanError R_CVulkan_DefragExecuteMovesCUDA (
    R_CVulkan_DefragContext* pContext,
    VkCommandBuffer           commandBuffer);

static enum R_CVulkanError R_CVulkan_DefragExecuteMovesOpenCL (
    R_CVulkan_DefragContext* pContext,
    VkCommandBuffer           commandBuffer);

static enum R_CVulkanError R_CVulkan_DefragExecuteMovesCPU (
    R_CVulkan_DefragContext* pContext,
    VkCommandBuffer           commandBuffer);

static enum R_CVulkanError R_CVulkan_DefragUpdateMetadataCUDA (
    R_CVulkan_DefragContext* pContext);

static enum R_CVulkanError R_CVulkan_DefragUpdateMetadataOpenCL (
    R_CVulkan_DefragContext* pContext);

static enum R_CVulkanError R_CVulkan_DefragUpdateMetadataCPU (
    R_CVulkan_DefragContext* pContext);

static enum R_CVulkanError R_CVulkan_DefragApplyMovesToAllocator (
    R_CVulkan_DefragContext* pContext);

static enum R_CVulkanError R_CVulkan_DefragEnsureCapacity (
    void**    ppBuffer,
    uint32_t* pCapacity,
    uint32_t  requiredCount,
    uint32_t  elementSize);

R_CVULKAN_API void
R_CVulkan_DefragSetDefaultConfig (struct R_CVulkan_DefragConfig* pConfig)
{
    if (!pConfig)
    {
        return;
    }
    
    pConfig->mergeFactor = R_CVULKAN_DEFRAG_DEFAULT_MERGE_FACTOR;
    pConfig->maxBytesPerPass = R_CVULKAN_DEFRAG_DEFAULT_MAX_BYTES_PER_PASS;
    pConfig->maxPasses = R_CVULKAN_DEFRAG_DEFAULT_MAX_PASSES;
    pConfig->preferredBackend = R_CVULKAN_DEFRAG_BACKEND_NONE;
}

R_CVULKAN_API enum R_CVulkanError
R_CVulkan_DefragGetAvailableBackend (struct R_CVulkan_DefragBackend* pBackend)
{
    if (!pBackend)
    {
        return R_CVULKAN_ERROR_NULL_POINTER;
    }
    
#ifdef R_CVULKAN_DEFRAG_CUDA_ENABLED
    int deviceCount = 0;
    cudaError_t cudaError = cudaGetDeviceCount (&deviceCount);
    if (cudaError == cudaSuccess && deviceCount > 0)
    {
        *pBackend = R_CVULKAN_DEFRAG_BACKEND_CUDA;
        return R_CVULKAN_OK;
    }
#endif
    
#ifdef R_CVULKAN_DEFRAG_OPENCL_ENABLED
    cl_uint platformCount = 0;
    cl_int clError = clGetPlatformIDs (0, NULL, &platformCount);
    if (clError == CL_SUCCESS && platformCount > 0)
    {
        *pBackend = R_CVULKAN_DEFRAG_BACKEND_OPENCL;
        return R_CVULKAN_OK;
    }
#endif
    
    *pBackend = R_CVULKAN_DEFRAG_BACKEND_CPU;
    return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkanError
R_CVulkan_DefragInitialize (
    R_CVulkan_DefragContext*       pContext,
    R_CVulkan_MemoryAllocator*     pAllocator,
    const R_CVulkan_DefragConfig* pConfig)
{
    enum R_CVulkanError result = R_CVULKAN_OK;
    
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
    }
    pContext->blockMetadataCapacity = 16;
    pContext->pBlockMetadata = R_CSTL_HeapAlloc (pContext->blockMetadataCapacity * sizeof (struct R_CVulkan_DefragBlockMetadata));
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
    
#ifdef R_CVULKAN_DEFRAG_OPENCL_ENABLED
    if (pContext->backend == R_CVULKAN_DEFRAG_BACKEND_OPENCL)
    {
        cl_platform_id platform = NULL;
        cl_uint platformCount = 0;
        cl_int clError = clGetPlatformIDs (1, &platform, &platformCount);
        if (clError != CL_SUCCESS || platformCount == 0)
        {
            pContext->backend = R_CVULKAN_DEFRAG_BACKEND_CPU;
        }
        else
        {
            cl_device_id device = NULL;
            cl_uint deviceCount = 0;
            clError = clGetDeviceIDs (platform, CL_DEVICE_TYPE_GPU, 1, &device, &deviceCount);
            if (clError != CL_SUCCESS || deviceCount == 0)
            {
                pContext->backend = R_CVULKAN_DEFRAG_BACKEND_CPU;
            }
            else
            {
                cl_context context = clCreateContext (NULL, 1, &device, NULL, NULL, &clError);
                if (clError != CL_SUCCESS || !context)
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
#endif
    
#ifdef R_CVULKAN_DEFRAG_CUDA_ENABLED
    if (pContext->backend == R_CVULKAN_DEFRAG_BACKEND_CUDA)
    {
        cudaError_t cudaError = cudaSetDevice (0);
        if (cudaError != cudaSuccess)
        {
            pContext->backend = R_CVULKAN_DEFRAG_BACKEND_CPU;
        }
    }
#endif
    
    pContext->isInitialized = 1;
    return R_CVULKAN_OK;
    
r_cvulkan_cleanup_moves:
    if (pContext->pMoves)
    {
        R_CSTL_HeapFree (pContext->pMoves);
        pContext->pMoves = NULL;
    }
r_cvulkan_cleanup_metadata:
    if (pContext->pBlockMetadata)
    {
        R_CSTL_HeapFree (pContext->pBlockMetadata);
        pContext->pBlockMetadata = NULL;
    }
r_cvulkan_cleanup:
    return result;
}

R_CVULKAN_API void
R_CVulkan_DefragCleanup (R_CVulkan_DefragContext* pContext)
{
    if (!pContext)
    {
        return;
    }
    
#ifdef R_CVULKAN_DEFRAG_OPENCL_ENABLED
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
    
    pContext->isInitialized = 0;
}

static enum R_CVulkanError
R_CVulkan_DefragCollectBlockMetadata (R_CVulkan_DefragContext* pContext)
{
    enum R_CVulkanError result = R_CVULKAN_OK;
        struct R_CVulkan_MemoryAllocator* pAllocator = pContext->pAllocator;
    
    if (pAllocator->blockCount > pContext->blockMetadataCapacity)
    {
        result = R_CVulkan_DefragEnsureCapacity (
            &pContext->pBlockMetadata,
            &pContext->blockMetadataCapacity,
            pAllocator->blockCount,
                sizeof (struct R_CVulkan_DefragBlockMetadata));
        if (result != R_CVULKAN_OK)
        {
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
            = 
            &((struct R_CVulkan_DefragBlockMetadata*)pContext->pBlockMetadata)[i];
        
        metadata->blockIndex = i;
        metadata->totalSize = block->size;
        metadata->usedSize = block->usedSize;
        metadata->freeSize = block->size - block->usedSize;
        metadata->fillLevel = (block->size > 0) 
            ? ((float)block->usedSize / (float)block->size) 
            : 0.0f;
        metadata->allocationCount = 0;
        metadata->isCandidate = 0;
    }
    
    return R_CVULKAN_OK;
}

static enum R_CVulkanError
R_CVulkan_DefragAnalyzeBlocksCUDA (R_CVulkan_DefragContext* pContext)
{
#ifdef R_CVULKAN_DEFRAG_CUDA_ENABLED
    enum R_CVulkanError result = R_CVULKAN_OK;
    cudaError_t cudaError = cudaSuccess;
    void* dBlockMetadata = NULL;
    
    cudaError = cudaMalloc (
        &dBlockMetadata,
        pContext->blockMetadataCount * sizeof (struct R_CVulkan_DefragBlockMetadata));
    if (cudaError != cudaSuccess)
    {
        result = R_CVULKAN_ERROR_OUT_OF_MEMORY;
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
            goto r_cvulkan_cleanup_device;
    }
    
    cudaError = cudaDeviceSynchronize ();
    if (cudaError != cudaSuccess)
    {
        result = R_CVULKAN_ERROR_FAILED;
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
            goto r_cvulkan_cleanup_device;
    }
    
r_cvulkan_cleanup_device:
    cudaFree (dBlockMetadata);
r_cvulkan_cleanup:
    return result;
#else
    return R_CVULKAN_ERROR_FAILED;
#endif
}

static enum R_CVulkanError
R_CVulkan_DefragAnalyzeBlocksOpenCL (R_CVulkan_DefragContext* pContext)
{
#ifdef R_CVULKAN_DEFRAG_OPENCL_ENABLED
    enum R_CVulkanError result = R_CVULKAN_OK;
    cl_int clError = CL_SUCCESS;
    cl_context context = (cl_context)pContext->pBackendContext;
    cl_command_queue queue = NULL;
    cl_mem dBlockMetadata = NULL;
    cl_program program = NULL;
    cl_kernel kernel = NULL;
    
    cl_device_id device = NULL;
    clError = clGetContextInfo (context, CL_CONTEXT_DEVICES, sizeof (cl_device_id), &device, NULL);
    if (clError != CL_SUCCESS)
    {
        result = R_CVULKAN_ERROR_FAILED;
        goto cleanup;
    }
    
    queue = clCreateCommandQueue (context, device, 0, &clError);
    if (clError != CL_SUCCESS || !queue)
    {
        result = R_CVULKAN_ERROR_FAILED;
        goto cleanup;
    }
    
    dBlockMetadata = clCreateBuffer (
        context,
        CL_MEM_READ_WRITE,
        pContext->blockMetadataCount * sizeof (struct R_CVulkan_DefragBlockMetadata),
        NULL,
        &clError);
    if (clError != CL_SUCCESS || !dBlockMetadata)
    {
        result = R_CVULKAN_ERROR_OUT_OF_MEMORY;
        goto cleanup_queue;
    }
    
    clError = clEnqueueWriteBuffer (
        queue,
        dBlockMetadata,
        CL_TRUE,
        0,
        pContext->blockMetadataCount * sizeof (struct R_CVulkan_DefragBlockMetadata),
        pContext->pBlockMetadata,
        0,
        NULL,
        NULL);
    if (clError != CL_SUCCESS)
    {
        result = R_CVULKAN_ERROR_FAILED;
        goto cleanup_buffer;
    }
    
    const char* source = (const char*)cvulkanDefragmentationCl_data;
    size_t sourceSize = cvulkanDefragmentationCl_size * sizeof (uint32_t);
    
    program = clCreateProgramWithBinary (context, 1, &device, &sourceSize, (const unsigned char**)&source, NULL, &clError);
    if (clError != CL_SUCCESS || !program)
    {
        result = R_CVULKAN_ERROR_FAILED;
        goto cleanup_buffer;
    }
    
    clError = clBuildProgram (program, 1, &device, NULL, NULL, NULL);
    if (clError != CL_SUCCESS)
    {
        result = R_CVULKAN_ERROR_FAILED;
        goto cleanup_program;
    }
    
    kernel = clCreateKernel (program, "R_CVulkan_DefragAnalyzeBlocksKernel", &clError);
    if (clError != CL_SUCCESS || !kernel)
    {
        result = R_CVULKAN_ERROR_FAILED;
        goto cleanup_program;
    }
    
    clError = clSetKernelArg (kernel, 0, sizeof (cl_mem), &dBlockMetadata);
    clError |= clSetKernelArg (kernel, 1, sizeof (cl_uint), &pContext->blockMetadataCount);
    clError |= clSetKernelArg (kernel, 2, sizeof (cl_uint), &pContext->config.mergeFactor);
    if (clError != CL_SUCCESS)
    {
        result = R_CVULKAN_ERROR_FAILED;
        goto cleanup_kernel;
    }
    
    size_t globalWorkSize = pContext->blockMetadataCount;
    clError = clEnqueueNDRangeKernel (queue, kernel, 1, NULL, &globalWorkSize, NULL, 0, NULL, NULL);
    if (clError != CL_SUCCESS)
    {
        result = R_CVULKAN_ERROR_FAILED;
        goto cleanup_kernel;
    }
    
    clError = clFinish (queue);
    if (clError != CL_SUCCESS)
    {
        result = R_CVULKAN_ERROR_FAILED;
        goto cleanup_kernel;
    }
    
    clError = clEnqueueReadBuffer (
        queue,
        dBlockMetadata,
        CL_TRUE,
        0,
        pContext->blockMetadataCount * sizeof (struct R_CVulkan_DefragBlockMetadata),
        pContext->pBlockMetadata,
        0,
        NULL,
        NULL);
    if (clError != CL_SUCCESS)
    {
        result = R_CVULKAN_ERROR_FAILED;
        goto cleanup_kernel;
    }
    
cleanup_kernel:
    if (kernel)
    {
        clReleaseKernel (kernel);
    }
cleanup_program:
    if (program)
    {
        clReleaseProgram (program);
    }
cleanup_buffer:
    if (dBlockMetadata)
    {
        clReleaseMemObject (dBlockMetadata);
    }
cleanup_queue:
    if (queue)
    {
        clReleaseCommandQueue (queue);
    }
cleanup:
    return result;
#else
    return R_CVULKAN_ERROR_FAILED;
#endif
}

static enum R_CVulkanError
R_CVulkan_DefragAnalyzeBlocksCPU (R_CVulkan_DefragContext* pContext)
{
    R_CVulkan_DefragBlockMetadata* metadataArray = 
        (R_CVulkan_DefragBlockMetadata*)pContext->pBlockMetadata;
    
    float threshold = (float)pContext->config.mergeFactor / 
                     (float)(pContext->config.mergeFactor + 1);
    
    for (uint32_t i = 0; i < pContext->blockMetadataCount; ++i)
    {
        R_CVulkan_DefragBlockMetadata* metadata = &metadataArray[i];
        
        metadata->fillLevel = (metadata->totalSize > 0) 
            ? ((float)metadata->usedSize / (float)metadata->totalSize) 
            : 0.0f;
        metadata->freeSize = metadata->totalSize - metadata->usedSize;
        
        metadata->isCandidate = (metadata->fillLevel <= threshold && metadata->usedSize > 0) ? 1 : 0;
    }
    
    return R_CVULKAN_OK;
}

static enum R_CVulkanError
R_CVulkan_DefragCreateMovePlanCUDA (R_CVulkan_DefragContext* pContext)
{
#ifdef R_CVULKAN_DEFRAG_CUDA_ENABLED
    enum R_CVulkanError result = R_CVULKAN_OK;
    cudaError_t cudaError = cudaSuccess;
    void* dBlockMetadata = NULL;
    void* dMoves = NULL;
    uint32_t* dMoveCount = NULL;
    
    uint32_t maxMoves = pContext->blockMetadataCount;
    if (maxMoves > pContext->moveCapacity)
    {
        result = R_CVulkan_DefragEnsureCapacity (
            &pContext->pMoves,
            &pContext->moveCapacity,
            maxMoves,
            sizeof (R_CVulkan_DefragMove));
        if (result != R_CVULKAN_OK)
        {
            return result;
        }
    }
    
    cudaError = cudaMalloc (
        &dBlockMetadata,
        pContext->blockMetadataCount * sizeof (struct R_CVulkan_DefragBlockMetadata));
    if (cudaError != cudaSuccess)
    {
        result = R_CVULKAN_ERROR_OUT_OF_MEMORY;
        goto cleanup;
    }
    
    cudaError = cudaMalloc (
        &dMoves, maxMoves * sizeof (struct R_CVulkan_DefragMove));
    if (cudaError != cudaSuccess)
    {
        result = R_CVULKAN_ERROR_OUT_OF_MEMORY;
        goto cleanup_metadata;
    }
    
    cudaError = cudaMalloc (&dMoveCount, sizeof (uint32_t));
    if (cudaError != cudaSuccess)
    {
        result = R_CVULKAN_ERROR_OUT_OF_MEMORY;
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
        goto cleanup_movecount;
    }
    
    cudaError = cudaDeviceSynchronize ();
    if (cudaError != cudaSuccess)
    {
        result = R_CVULKAN_ERROR_FAILED;
        goto cleanup_movecount;
    }
    
    cudaError = cudaMemcpy (
        &pContext->moveCount,
        dMoveCount,
        sizeof (uint32_t),
        cudaMemcpyDeviceToHost);
    if (cudaError != cudaSuccess)
    {
        result = R_CVULKAN_ERROR_FAILED;
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
            goto cleanup_movecount;
        }
    }
    
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
R_CVulkan_DefragCreateMovePlanOpenCL (R_CVulkan_DefragContext* pContext)
{
#ifdef R_CVULKAN_DEFRAG_OPENCL_ENABLED
    enum R_CVulkanError result = R_CVULKAN_OK;
    cl_int clError = CL_SUCCESS;
    cl_context context = (cl_context)pContext->pBackendContext;
    cl_command_queue queue = NULL;
    cl_mem dBlockMetadata = NULL;
    cl_mem dMoves = NULL;
    cl_mem dMoveCount = NULL;
    cl_program program = NULL;
    cl_kernel kernel = NULL;
    
    cl_device_id device = NULL;
    clError = clGetContextInfo (context, CL_CONTEXT_DEVICES, sizeof (cl_device_id), &device, NULL);
    if (clError != CL_SUCCESS)
    {
        result = R_CVULKAN_ERROR_FAILED;
        goto cleanup;
    }
    
    queue = clCreateCommandQueue (context, device, 0, &clError);
    if (clError != CL_SUCCESS || !queue)
    {
        result = R_CVULKAN_ERROR_FAILED;
        goto cleanup;
    }
    
    uint32_t maxMoves = pContext->blockMetadataCount;
    if (maxMoves > pContext->moveCapacity)
    {
        result = R_CVulkan_DefragEnsureCapacity (
            &pContext->pMoves,
            &pContext->moveCapacity,
            maxMoves,
            sizeof (R_CVulkan_DefragMove));
        if (result != R_CVULKAN_OK)
        {
            goto cleanup_queue;
        }
    }
    
    dBlockMetadata = clCreateBuffer (
        context,
        CL_MEM_READ_WRITE,
        pContext->blockMetadataCount * sizeof (struct R_CVulkan_DefragBlockMetadata),
        NULL,
        &clError);
    if (clError != CL_SUCCESS || !dBlockMetadata)
    {
        result = R_CVULKAN_ERROR_OUT_OF_MEMORY;
        goto cleanup_queue;
    }
    
    dMoves = clCreateBuffer (
        context,
        CL_MEM_READ_WRITE,
        maxMoves * sizeof (struct R_CVulkan_DefragMove),
        NULL,
        &clError);
    if (clError != CL_SUCCESS || !dMoves)
    {
        result = R_CVULKAN_ERROR_OUT_OF_MEMORY;
        goto cleanup_buffer_metadata;
    }
    
    dMoveCount = clCreateBuffer (context, CL_MEM_READ_WRITE, sizeof (uint32_t), NULL, &clError);
    if (clError != CL_SUCCESS || !dMoveCount)
    {
        result = R_CVULKAN_ERROR_OUT_OF_MEMORY;
        goto cleanup_buffer_moves;
    }
    
    clError = clEnqueueWriteBuffer (
        queue,
        dBlockMetadata,
        CL_TRUE,
        0,
        pContext->blockMetadataCount * sizeof (struct R_CVulkan_DefragBlockMetadata),
        pContext->pBlockMetadata,
        0,
        NULL,
        NULL);
    if (clError != CL_SUCCESS)
    {
        result = R_CVULKAN_ERROR_FAILED;
        goto cleanup_buffer_movecount;
    }
    
    const char* source = 
        "#include \"cvulkan_defragmentation.cl\"\n";
    
    program = clCreateProgramWithSource (context, 1, &source, NULL, &clError);
    if (clError != CL_SUCCESS || !program)
    {
        result = R_CVULKAN_ERROR_FAILED;
        goto cleanup_buffer_movecount;
    }
    
    clError = clBuildProgram (program, 1, &device, NULL, NULL, NULL);
    if (clError != CL_SUCCESS)
    {
        result = R_CVULKAN_ERROR_FAILED;
        goto cleanup_program;
    }
    
    kernel = clCreateKernel (program, "R_CVulkan_DefragCreateMovePlanKernel", &clError);
    if (clError != CL_SUCCESS || !kernel)
    {
        result = R_CVULKAN_ERROR_FAILED;
        goto cleanup_program;
    }
    
    clError = clSetKernelArg (kernel, 0, sizeof (cl_mem), &dBlockMetadata);
    clError |= clSetKernelArg (kernel, 1, sizeof (cl_mem), &dMoves);
    clError |= clSetKernelArg (kernel, 2, sizeof (cl_mem), &dMoveCount);
    clError |= clSetKernelArg (kernel, 3, sizeof (cl_uint), &pContext->blockMetadataCount);
    clError |= clSetKernelArg (kernel, 4, sizeof (cl_uint), &pContext->config.mergeFactor);
    clError |= clSetKernelArg (kernel, 5, sizeof (cl_ulong), &pContext->config.maxBytesPerPass);
    if (clError != CL_SUCCESS)
    {
        result = R_CVULKAN_ERROR_FAILED;
        goto cleanup_kernel;
    }
    
    size_t globalWorkSize = pContext->blockMetadataCount;
    clError = clEnqueueNDRangeKernel (queue, kernel, 1, NULL, &globalWorkSize, NULL, 0, NULL, NULL);
    if (clError != CL_SUCCESS)
    {
        result = R_CVULKAN_ERROR_FAILED;
        goto cleanup_kernel;
    }
    
    clError = clFinish (queue);
    if (clError != CL_SUCCESS)
    {
        result = R_CVULKAN_ERROR_FAILED;
        goto cleanup_kernel;
    }
    
    clError = clEnqueueReadBuffer (
        queue,
        dMoveCount,
        CL_TRUE,
        0,
        sizeof (uint32_t),
        &pContext->moveCount,
        0,
        NULL,
        NULL);
    if (clError != CL_SUCCESS)
    {
        result = R_CVULKAN_ERROR_FAILED;
        goto cleanup_kernel;
    }
    
    if (pContext->moveCount > 0)
    {
        clError = clEnqueueReadBuffer (
            queue,
            dMoves,
            CL_TRUE,
            0,
                pContext->moveCount * sizeof (struct R_CVulkan_DefragMove),
            pContext->pMoves,
            0,
            NULL,
            NULL);
        if (clError != CL_SUCCESS)
        {
            result = R_CVULKAN_ERROR_FAILED;
            goto cleanup_kernel;
        }
    }
    
cleanup_kernel:
    if (kernel)
    {
        clReleaseKernel (kernel);
    }
cleanup_program:
    if (program)
    {
        clReleaseProgram (program);
    }
cleanup_buffer_movecount:
    if (dMoveCount)
    {
        clReleaseMemObject (dMoveCount);
    }
cleanup_buffer_moves:
    if (dMoves)
    {
        clReleaseMemObject (dMoves);
    }
cleanup_buffer_metadata:
    if (dBlockMetadata)
    {
        clReleaseMemObject (dBlockMetadata);
    }
cleanup_queue:
    if (queue)
    {
        clReleaseCommandQueue (queue);
    }
cleanup:
    return result;
#else
    return R_CVULKAN_ERROR_FAILED;
#endif
}

static enum R_CVulkan_Error
R_CVulkan_DefragCreateMovePlanCPU (R_CVulkan_DefragContext* pContext)
{
        struct R_CVulkan_DefragBlockMetadata* metadataArray
            = 
        (struct R_CVulkan_DefragBlockMetadata*)pContext->pBlockMetadata;
        struct R_CVulkan_DefragMove* moves = (R_CVulkan_DefragMove*)pContext->pMoves;
    
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
        float bestFillLevel = 0.0f;
        
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
                        sizeof (R_CVulkan_DefragMove));
                    if (result != R_CVULKAN_OK)
                    {
                        return result;
                    }
                    moves = (R_CVulkan_DefragMove*)pContext->pMoves;
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
#ifdef R_CVULKAN_DEFRAG_CUDA_ENABLED
    enum R_CVulkanError result = R_CVULKAN_OK;
    cudaError_t cudaError = cudaSuccess;
    void* dBlockMetadata = NULL;
    void* dMoves = NULL;
    
    cudaError = cudaMalloc (
        &dBlockMetadata,
        pContext->blockMetadataCount * sizeof (R_CVulkan_DefragBlockMetadata));
    if (cudaError != cudaSuccess)
    {
        result = R_CVULKAN_ERROR_OUT_OF_MEMORY;
        goto cleanup;
    }
    
    cudaError = cudaMalloc (
        &dMoves,
        pContext->moveCount * sizeof (R_CVulkan_DefragMove));
    if (cudaError != cudaSuccess)
    {
        result = R_CVULKAN_ERROR_OUT_OF_MEMORY;
        goto cleanup_metadata;
    }
    
    cudaError = cudaMemcpy (
        dBlockMetadata,
        pContext->pBlockMetadata,
        pContext->blockMetadataCount * sizeof (R_CVulkan_DefragBlockMetadata),
        cudaMemcpyHostToDevice);
    if (cudaError != cudaSuccess)
    {
        result = R_CVULKAN_ERROR_FAILED;
        goto cleanup_moves;
    }
    
    cudaError = cudaMemcpy (
        dMoves,
        pContext->pMoves,
        pContext->moveCount * sizeof (R_CVulkan_DefragMove),
        cudaMemcpyHostToDevice);
    if (cudaError != cudaSuccess)
    {
        result = R_CVULKAN_ERROR_FAILED;
        goto cleanup_moves;
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
        goto cleanup_moves;
    }
    
    cudaError = cudaDeviceSynchronize ();
    if (cudaError != cudaSuccess)
    {
        result = R_CVULKAN_ERROR_FAILED;
        goto cleanup_moves;
    }
    
    cudaError = cudaMemcpy (
        pContext->pBlockMetadata,
        dBlockMetadata,
        pContext->blockMetadataCount * sizeof (struct R_CVulkan_DefragBlockMetadata),
        cudaMemcpyDeviceToHost);
    if (cudaError != cudaSuccess)
    {
        result = R_CVULKAN_ERROR_FAILED;
        goto cleanup_moves;
    }
    
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
R_CVulkan_DefragUpdateMetadataOpenCL (struct R_CVulkan_DefragContext* pContext)
{
#ifdef R_CVULKAN_DEFRAG_OPENCL_ENABLED
    enum R_CVulkanError result = R_CVULKAN_OK;
    cl_int clError = CL_SUCCESS;
    cl_context context = (cl_context)pContext->pBackendContext;
    cl_command_queue queue = NULL;
    cl_mem dBlockMetadata = NULL;
    cl_mem dMoves = NULL;
    cl_program program = NULL;
    cl_kernel kernel = NULL;
    
    cl_device_id device = NULL;
    clError = clGetContextInfo (context, CL_CONTEXT_DEVICES, sizeof (cl_device_id), &device, NULL);
    if (clError != CL_SUCCESS)
    {
        result = R_CVULKAN_ERROR_FAILED;
        goto cleanup;
    }
    
    queue = clCreateCommandQueue (context, device, 0, &clError);
    if (clError != CL_SUCCESS || !queue)
    {
        result = R_CVULKAN_ERROR_FAILED;
        goto cleanup;
    }
    
    dBlockMetadata = clCreateBuffer (
        context,
        CL_MEM_READ_WRITE,
        pContext->blockMetadataCount * sizeof (R_CVulkan_DefragBlockMetadata),
        NULL,
        &clError);
    if (clError != CL_SUCCESS || !dBlockMetadata)
    {
        result = R_CVULKAN_ERROR_OUT_OF_MEMORY;
        goto cleanup_queue;
    }
    
    dMoves = clCreateBuffer (
        context,
        CL_MEM_READ_WRITE,
        pContext->moveCount * sizeof (R_CVulkan_DefragMove),
        NULL,
        &clError);
    if (clError != CL_SUCCESS || !dMoves)
    {
        result = R_CVULKAN_ERROR_OUT_OF_MEMORY;
        goto cleanup_buffer_metadata;
    }
    
    clError = clEnqueueWriteBuffer (
        queue,
        dBlockMetadata,
        CL_TRUE,
        0,
        pContext->blockMetadataCount * sizeof (R_CVulkan_DefragBlockMetadata),
        pContext->pBlockMetadata,
        0,
        NULL,
        NULL);
    if (clError != CL_SUCCESS)
    {
        result = R_CVULKAN_ERROR_FAILED;
        goto cleanup_buffer_moves;
    }
    
    clError = clEnqueueWriteBuffer (
        queue,
        dMoves,
        CL_TRUE,
        0,
        pContext->moveCount * sizeof (R_CVulkan_DefragMove),
        pContext->pMoves,
        0,
        NULL,
        NULL);
    if (clError != CL_SUCCESS)
    {
        result = R_CVULKAN_ERROR_FAILED;
        goto cleanup_buffer_moves;
    }
    
    const char* source = 
        "#include \"cvulkan_defragmentation.cl\"\n";
    
    program = clCreateProgramWithSource (context, 1, &source, NULL, &clError);
    if (clError != CL_SUCCESS || !program)
    {
        result = R_CVULKAN_ERROR_FAILED;
        goto cleanup_buffer_moves;
    }
    
    clError = clBuildProgram (program, 1, &device, NULL, NULL, NULL);
    if (clError != CL_SUCCESS)
    {
        result = R_CVULKAN_ERROR_FAILED;
        goto cleanup_program;
    }
    
    kernel = clCreateKernel (program, "R_CVulkan_DefragUpdateMetadataKernel", &clError);
    if (clError != CL_SUCCESS || !kernel)
    {
        result = R_CVULKAN_ERROR_FAILED;
        goto cleanup_program;
    }
    
    clError = clSetKernelArg (kernel, 0, sizeof (cl_mem), &dBlockMetadata);
    clError |= clSetKernelArg (kernel, 1, sizeof (cl_mem), &dMoves);
    clError |= clSetKernelArg (kernel, 2, sizeof (cl_uint), &pContext->moveCount);
    clError |= clSetKernelArg (kernel, 3, sizeof (cl_uint), &pContext->blockMetadataCount);
    if (clError != CL_SUCCESS)
    {
        result = R_CVULKAN_ERROR_FAILED;
        goto cleanup_kernel;
    }
    
    size_t globalWorkSize = pContext->moveCount;
    clError = clEnqueueNDRangeKernel (queue, kernel, 1, NULL, &globalWorkSize, NULL, 0, NULL, NULL);
    if (clError != CL_SUCCESS)
    {
        result = R_CVULKAN_ERROR_FAILED;
        goto cleanup_kernel;
    }
    
    clError = clFinish (queue);
    if (clError != CL_SUCCESS)
    {
        result = R_CVULKAN_ERROR_FAILED;
        goto cleanup_kernel;
    }
    
    clError = clEnqueueReadBuffer (
        queue,
        dBlockMetadata,
        CL_TRUE,
        0,
        pContext->blockMetadataCount * sizeof (struct R_CVulkan_DefragBlockMetadata),
        pContext->pBlockMetadata,
        0,
        NULL,
        NULL);
    if (clError != CL_SUCCESS)
    {
        result = R_CVULKAN_ERROR_FAILED;
        goto cleanup_kernel;
    }
    
cleanup_kernel:
    if (kernel)
    {
        clReleaseKernel (kernel);
    }
cleanup_program:
    if (program)
    {
        clReleaseProgram (program);
    }
cleanup_buffer_moves:
    if (dMoves)
    {
        clReleaseMemObject (dMoves);
    }
cleanup_buffer_metadata:
    if (dBlockMetadata)
    {
        clReleaseMemObject (dBlockMetadata);
    }
cleanup_queue:
    if (queue)
    {
        clReleaseCommandQueue (queue);
    }
cleanup:
    return result;
#else
    return R_CVULKAN_ERROR_FAILED;
#endif
}

static enum R_CVulkanError
R_CVulkan_DefragUpdateMetadataCPU (struct R_CVulkan_DefragContext* pContext)
{
        struct R_CVulkan_DefragBlockMetadata* metadataArray
                = 
        (struct R_CVulkan_DefragBlockMetadata*)pContext->pBlockMetadata;
        struct R_CVulkan_DefragMove* moves = (struct R_CVulkan_DefragMove*)pContext->pMoves;
    
    for (uint32_t i = 0; i < pContext->moveCount; ++i)
    {
        R_CVulkan_DefragMove* move = &moves[i];
        
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
    
    return R_CVULKAN_OK;
}

static enum R_CVulkanError
R_CVulkan_DefragApplyMovesToAllocator (struct R_CVulkan_DefragContext* pContext)
{
    R_CVulkan_MemoryAllocator* pAllocator = pContext->pAllocator;
    R_CVulkan_DefragMove* moves = (R_CVulkan_DefragMove*)pContext->pMoves;
    
    for (uint32_t i = 0; i < pContext->moveCount; ++i)
    {
        R_CVulkan_DefragMove* move = &moves[i];
        
        if (move->operation != R_CVULKAN_DEFRAG_MOVE_OPERATION_MOVE)
        {
            continue;
        }
        
        if (move->srcBlockIndex >= pAllocator->blockCount || 
            move->dstBlockIndex >= pAllocator->blockCount)
        {
            continue;
        }
        
        R_CVulkan_MemoryBlock* srcBlock = pAllocator->ppBlocks[move->srcBlockIndex];
        R_CVulkan_MemoryBlock* dstBlock = pAllocator->ppBlocks[move->dstBlockIndex];
        
        if (!srcBlock || !dstBlock)
        {
            continue;
        }
        
        if (srcBlock->device != dstBlock->device || 
            srcBlock->properties != dstBlock->properties)
        {
            continue;
        }
        
        void* srcMapped = NULL;
        VkResult vkResult = vkMapMemory (
            srcBlock->device,
            srcBlock->memory,
            move->srcOffset,
            move->size,
            0,
            &srcMapped);
        if (vkResult != VK_SUCCESS)
        {
            continue;
        }
        
        void* dstMapped = NULL;
        vkResult = vkMapMemory (
            dstBlock->device,
            dstBlock->memory,
            move->dstOffset,
            move->size,
            0,
            &dstMapped);
        if (vkResult != VK_SUCCESS)
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
    
    if (!pContext || !pContext->isInitialized)
    {
        return R_CVULKAN_ERROR_NOT_INITIALIZED;
    }
    
    pContext->currentPass = 0;
    pContext->totalMoves = 0;
    pContext->totalBytesMoved = 0;
    pContext->moveCount = 0;
    
    result = R_CVulkan_DefragCollectBlockMetadata (pContext);
    if (result != R_CVULKAN_OK)
    {
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
    
    return result;
}

R_CVULKAN_API enum R_CVulkanError
R_CVulkan_DefragExecutePass (
    struct R_CVulkan_DefragContext* pContext,
    VkCommandBuffer                 commandBuffer)
{
    enum R_CVulkanError result = R_CVULKAN_OK;
    
    if (!pContext || !pContext->isInitialized)
    {
        return R_CVULKAN_ERROR_NOT_INITIALIZED;
    }
    
    if (pContext->currentPass >= pContext->config.maxPasses)
    {
        return R_CVULKAN_OK;
    }
    
    switch (pContext->backend)
    {
        case R_CVULKAN_DEFRAG_BACKEND_CUDA:
            result = R_CVulkan_DefragCreateMovePlanCUDA (pContext);
            if (result != R_CVULKAN_OK)
            {
                return result;
            }
            result = R_CVulkan_DefragExecuteMovesCUDA (pContext, commandBuffer);
            if (result != R_CVULKAN_OK)
            {
                return result;
            }
            result = R_CVulkan_DefragUpdateMetadataCUDA (pContext);
            break;
        case R_CVULKAN_DEFRAG_BACKEND_OPENCL:
            result = R_CVulkan_DefragCreateMovePlanOpenCL (pContext);
            if (result != R_CVULKAN_OK)
            {
                return result;
            }
            result = R_CVulkan_DefragExecuteMovesOpenCL (pContext, commandBuffer);
            if (result != R_CVULKAN_OK)
            {
                return result;
            }
            result = R_CVulkan_DefragUpdateMetadataOpenCL (pContext);
            break;
        case R_CVULKAN_DEFRAG_BACKEND_CPU:
        default:
            result = R_CVulkan_DefragCreateMovePlanCPU (pContext);
            if (result != R_CVULKAN_OK)
            {
                return result;
            }
            result = R_CVulkan_DefragExecuteMovesCPU (pContext, commandBuffer);
            if (result != R_CVULKAN_OK)
            {
                return result;
            }
            result = R_CVulkan_DefragUpdateMetadataCPU (pContext);
            break;
    }
    
    if (result != R_CVULKAN_OK)
    {
        return result;
    }
    
    pContext->currentPass++;
    
    if (pContext->moveCount == 0)
    {
        return R_CVULKAN_OK;
    }
    
    return R_CVULKAN_ERROR_INCOMPLETE;
}

R_CVULKAN_API enum R_CVulkanError
R_CVulkan_DefragEnd (struct R_CVulkan_DefragContext* pContext, struct R_CVulkan_DefragStats* pStats)
{
    if (!pContext || !pContext->isInitialized)
    {
        return R_CVULKAN_ERROR_NOT_INITIALIZED;
    }
    
    if (pStats)
    {
        memset (pStats, 0, sizeof (R_CVulkan_DefragStats));
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
    }
    
    return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkanError
R_CVulkan_DefragGetFragmentationLevel (
    const struct R_CVulkan_DefragContext* pContext,
    float*                        pFragmentation)
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
    
    R_CVulkan_DefragBlockMetadata* metadataArray = 
        (R_CVulkan_DefragBlockMetadata*)pContext->pBlockMetadata;
    
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
R_CVulkan_DefragIsNeeded (
    const R_CVulkan_DefragContext* pContext,
    int*                           pNeeded)
{
    enum R_CVulkanError result = R_CVULKAN_OK;
    float fragmentation = 0.0f;
    
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
