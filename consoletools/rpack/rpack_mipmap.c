#include "rpack/rpack_mipmap.h"
#include "rpack/rpack_pipeline.h"

#include "rlgame.base/cstl/cstl_heap_allocator.h"
#include "rlgame.base/cstl/cstl_log.h"
#include "rlgame.base/cstl/cstl_trace.h"

#include <string.h>
#include <stdio.h>
#include <math.h>

#ifdef R_CUDA
#include <cuda_runtime.h>

extern cudaError_t r_pack_mipmap_launch_box_filter (
    const void*  pSource,
    void*        pDestination,
    uint32_t     sourceWidth,
    uint32_t     sourceHeight,
    uint32_t     destinationWidth,
    uint32_t     destinationHeight,
    cudaStream_t stream);

extern cudaError_t r_pack_mipmap_launch_gaussian_filter (
    const void*  pSource,
    void*        pDestination,
    uint32_t     sourceWidth,
    uint32_t     sourceHeight,
    uint32_t     destinationWidth,
    uint32_t     destinationHeight,
    float        sigma,
    cudaStream_t stream);
#endif

#ifdef R_OPENCL
#include <CL/cl.h>

extern const uint32_t rpackMipmap_size;
extern const uint32_t rpackMipmap_data[];
#endif

#define R_PACK_MIPMAP_BOX_FILTER_NAME      "r_pack_mipmap_box_filter_kernel"
#define R_PACK_MIPMAP_GAUSSIAN_FILTER_NAME "r_pack_mipmap_gaussian_filter_kernel"

enum r_pack_mipmap_backend
{
    R_PACK_MIPMAP_BACKEND_CPU = 0,
    R_PACK_MIPMAP_BACKEND_CUDA = 1,
    R_PACK_MIPMAP_BACKEND_OPENCL = 2,
    R_PACK_MIPMAP_BACKEND_NONE = 3
};

struct r_pack_mipmap_context
{
        enum r_pack_mipmap_backend backend;

#ifdef R_CUDA
        cudaStream_t cudaStream;
#endif

#ifdef R_OPENCL
        cl_context       openclContext;
        cl_device_id     openclDevice;
        cl_command_queue openclQueue;
        cl_program       openclProgram;
        cl_kernel        openclBoxKernel;
        cl_kernel        openclGaussianKernel;
#endif
};

static int r_pack_mipmap_resizeCPU (
    const struct r_pack_input_image* pSource,
    uint8_t*                         pDestination,
    uint32_t                         destWidth,
    uint32_t                         destHeight,
    enum r_pack_mipmap_filter        filter,
    float                            sigma);

#ifdef R_CUDA
static int r_pack_mipmap_resizeCUDA (
    struct r_pack_mipmap_context*    pContext,
    const struct r_pack_input_image* pSource,
    uint8_t*                         pDestination,
    uint32_t                         destWidth,
    uint32_t                         destHeight,
    enum r_pack_mipmap_filter        filter,
    float                            sigma);
#endif

#ifdef R_OPENCL
static int r_pack_mipmap_execute_kernel (
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
    size_t        globalWorkSize[2],
    size_t        localWorkSize[2]);

static int r_pack_mipmap_resize_openCL (
    struct r_pack_mipmap_context*    pContext,
    const struct r_pack_input_image* pSource,
    uint8_t*                         pDestination,
    uint32_t                         destWidth,
    uint32_t                         destHeight,
    enum r_pack_mipmap_filter        filter,
    float                            sigma);
#endif

static int
r_pack_mipmap_resizeCPU (
    const struct r_pack_input_image* pSource,
    uint8_t*                         pDestination,
    uint32_t                         destWidth,
    uint32_t                         destHeight,
    enum r_pack_mipmap_filter        filter,
    float                            sigma)
{
    R_CSTL_TRACE_FUNCTION ();

    if (!pSource || !pDestination || destWidth == 0 || destHeight == 0)
    {
        R_CSTL_TRACE_RETURN ();
        return R_PACK_MIPMAP_ERROR_INVALID_ARGUMENT;
    }

    for (uint32_t y = 0; y < destHeight; ++y)
    {
        for (uint32_t x = 0; x < destWidth; ++x)
        {
            if (filter == R_PACK_MIPMAP_FILTER_BOX)
            {
                uint32_t srcX = (uint32_t)(((uint64_t)x * pSource->width) / destWidth);
                uint32_t srcY = (uint32_t)(((uint64_t)y * pSource->height) / destHeight);
                uint32_t sum[4] = {0, 0, 0, 0};
                uint32_t count = 0;

                for (uint32_t dy = 0; dy < 2 && srcY + dy < pSource->height; ++dy)
                {
                    for (uint32_t dx = 0; dx < 2 && srcX + dx < pSource->width; ++dx)
                    {
                        const uint8_t* pPixel
                            = pSource->pPixels + (srcY + dy) * pSource->stride + (srcX + dx) * 4;
                        for (uint32_t c = 0; c < 4; ++c)
                            sum[c] += pPixel[c];
                        count++;
                    }
                }

                uint8_t* pDestPixel = pDestination + (y * destWidth + x) * 4;
                for (uint32_t c = 0; c < 4; ++c)
                    pDestPixel[c] = (uint8_t)(sum[c] / count);
            }
            else
            {
                float srcX = ((float)x * pSource->width) / destWidth;
                float srcY = ((float)y * pSource->height) / destHeight;
                int   radius = (int)(3.0f * sigma);
                if (radius < 1) radius = 1;
                float sum[4] = {0.0f, 0.0f, 0.0f, 0.0f};
                float weightSum = 0.0f;

                for (int dy = -radius; dy <= radius; ++dy)
                {
                    for (int dx = -radius; dx <= radius; ++dx)
                    {
                        int sx = (int)(srcX + dx);
                        int sy = (int)(srcY + dy);

                        if (sx >= 0 && sx < (int)pSource->width && sy >= 0 && sy < (int)pSource->height)
                        {
                            float          distance = sqrtf ((float)(dx * dx + dy * dy));
                            float          weight = expf (-0.5f * (distance * distance) / (sigma * sigma));
                            const uint8_t* pPixel = pSource->pPixels + sy * pSource->stride + sx * 4;
                            for (uint32_t c = 0; c < 4; ++c)
                                sum[c] += pPixel[c] * weight;
                            weightSum += weight;
                        }
                    }
                }

                uint8_t* pDestPixel = pDestination + (y * destWidth + x) * 4;
                for (uint32_t c = 0; c < 4; ++c)
                    pDestPixel[c] = (uint8_t)(sum[c] / weightSum);
            }
        }
    }

    R_CSTL_TRACE_RETURN ();
    return R_PACK_MIPMAP_OK;
}

#ifdef R_CUDA
static int
r_pack_mipmap_resizeCUDA (
    struct r_pack_mipmap_context*    pContext,
    const struct r_pack_input_image* pSource,
    uint8_t*                         pDestination,
    uint32_t                         destWidth,
    uint32_t                         destHeight,
    enum r_pack_mipmap_filter        filter,
    float                            sigma)
{
    R_CSTL_TRACE_FUNCTION ();

    int         result = R_PACK_MIPMAP_OK;
    void*       dSource = NULL;
    void*       dDestination = NULL;
    cudaError_t cudaError = cudaSuccess;
    size_t      sourceSize = 0;
    size_t      destSize = 0;

    if (!pContext || !pSource || !pDestination)
    {
        R_CSTL_TRACE_RETURN ();
        return R_PACK_MIPMAP_ERROR_INVALID_ARGUMENT;
    }

    sourceSize = pSource->width * pSource->height * 4;
    destSize = destWidth * destHeight * 4;

    cudaError = cudaMalloc (&dSource, sourceSize);
    if (cudaError != cudaSuccess)
    {
        R_CSTL_LOG_ERROR ("CUDA malloc failed for source: %s", cudaGetErrorString (cudaError));
        R_CSTL_TRACE_RETURN ();
        return R_PACK_MIPMAP_ERROR_DISPATCH;
    }

    cudaError = cudaMalloc (&dDestination, destSize);
    if (cudaError != cudaSuccess)
    {
        R_CSTL_LOG_ERROR ("CUDA malloc failed for destination: %s", cudaGetErrorString (cudaError));
        result = R_PACK_MIPMAP_ERROR_DISPATCH;
        goto r_cleanup_source;
    }

    cudaError = cudaMemcpy (dSource, pSource->pPixels, sourceSize, cudaMemcpyHostToDevice);
    if (cudaError != cudaSuccess)
    {
        R_CSTL_LOG_ERROR ("CUDA memcpy H2D failed: %s", cudaGetErrorString (cudaError));
        result = R_PACK_MIPMAP_ERROR_DISPATCH;
        goto r_cleanup_dest;
    }

    if (filter == R_PACK_MIPMAP_FILTER_GAUSSIAN)
        cudaError = r_pack_mipmap_launch_gaussian_filter (
            dSource,
            dDestination,
            pSource->width,
            pSource->height,
            destWidth,
            destHeight,
            sigma,
            pContext->cudaStream);
    else
        cudaError = r_pack_mipmap_launch_box_filter (
            dSource,
            dDestination,
            pSource->width,
            pSource->height,
            destWidth,
            destHeight,
            pContext->cudaStream);

    if (cudaError != cudaSuccess)
    {
        R_CSTL_LOG_ERROR ("CUDA kernel launch failed: %s", cudaGetErrorString (cudaError));
        result = R_PACK_MIPMAP_ERROR_DISPATCH;
        goto r_cleanup_dest;
    }

    cudaError = cudaStreamSynchronize (pContext->cudaStream);
    if (cudaError != cudaSuccess)
    {
        R_CSTL_LOG_ERROR ("CUDA stream sync failed: %s", cudaGetErrorString (cudaError));
        result = R_PACK_MIPMAP_ERROR_DISPATCH;
        goto r_cleanup_dest;
    }

    cudaError = cudaMemcpy (pDestination, dDestination, destSize, cudaMemcpyDeviceToHost);
    if (cudaError != cudaSuccess)
    {
        R_CSTL_LOG_ERROR ("CUDA memcpy D2H failed: %s", cudaGetErrorString (cudaError));
        result = R_PACK_MIPMAP_ERROR_DISPATCH;
        goto r_cleanup_dest;
    }

r_cleanup_dest:
    cudaFree (dDestination);
r_cleanup_source:
    cudaFree (dSource);
    R_CSTL_TRACE_RETURN ();
    return result;
}
#endif

#ifdef R_OPENCL
static int
r_pack_mipmap_execute_kernel (
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
    size_t        globalWorkSize[2],
    size_t        localWorkSize[2])
{
    R_CSTL_TRACE_FUNCTION ();

    int              result = R_PACK_MIPMAP_OK;
    cl_int           error = CL_SUCCESS;
    cl_program       program = NULL;
    cl_kernel        kernel = NULL;
    cl_command_queue queue = NULL;
    cl_mem           sourceBuffer = NULL;
    cl_mem           destBuffer = NULL;

    queue = clCreateCommandQueue (context, device, 0, &error);
    if (error != CL_SUCCESS || !queue)
    {
        result = R_PACK_MIPMAP_ERROR_DISPATCH;
        R_CSTL_LOG_ERROR ("OpenCL create command queue failed: %d", error);
        goto r_cleanup;
    }

    program = clCreateProgramWithIL (context, pBinaryData, binarySize, &error);
    if (error != CL_SUCCESS || !program)
    {
        result = R_PACK_MIPMAP_ERROR_DISPATCH;
        R_CSTL_LOG_ERROR ("OpenCL create program failed: %d", error);
        goto r_cleanup_queue;
    }

    error = clBuildProgram (program, 1, &device, NULL, NULL, NULL);
    if (error != CL_SUCCESS)
    {
        result = R_PACK_MIPMAP_ERROR_DISPATCH;
        R_CSTL_LOG_ERROR ("OpenCL build program failed: %d", error);
        goto r_cleanup_program;
    }

    kernel = clCreateKernel (program, kernelName, &error);
    if (error != CL_SUCCESS || !kernel)
    {
        result = R_PACK_MIPMAP_ERROR_DISPATCH;
        R_CSTL_LOG_ERROR ("OpenCL create kernel failed: %d", error);
        goto r_cleanup_program;
    }

    for (uint32_t i = 0; i < argCount; ++i)
    {
        error = clSetKernelArg (kernel, i, pArgSizes[i], pKernelArgs[i]);
        if (error != CL_SUCCESS)
        {
            result = R_PACK_MIPMAP_ERROR_DISPATCH;
            R_CSTL_LOG_ERROR ("OpenCL set kernel arg %u failed: %d", i, error);
            goto r_cleanup_kernel;
        }
    }

    error = clEnqueueNDRangeKernel (queue, kernel, 2, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL);
    if (error != CL_SUCCESS)
    {
        result = R_PACK_MIPMAP_ERROR_DISPATCH;
        R_CSTL_LOG_ERROR ("OpenCL enqueue kernel failed: %d", error);
        goto r_cleanup_kernel;
    }

    error = clFinish (queue);
    if (error != CL_SUCCESS)
    {
        result = R_PACK_MIPMAP_ERROR_DISPATCH;
        R_CSTL_LOG_ERROR ("OpenCL finish failed: %d", error);
        goto r_cleanup_kernel;
    }

r_cleanup_kernel:
    if (kernel) clReleaseKernel (kernel);
r_cleanup_program:
    if (program) clReleaseProgram (program);
r_cleanup_queue:
    if (queue) clReleaseCommandQueue (queue);
r_cleanup:
    R_CSTL_TRACE_RETURN ();
    return result;
}

static int
r_pack_mipmap_resize_openCL (
    struct r_pack_mipmap_context*    pContext,
    const struct r_pack_input_image* pSource,
    uint8_t*                         pDestination,
    uint32_t                         destWidth,
    uint32_t                         destHeight,
    enum r_pack_mipmap_filter        filter,
    float                            sigma)
{
    R_CSTL_TRACE_FUNCTION ();

    int         result = R_PACK_MIPMAP_OK;
    cl_int      error = CL_SUCCESS;
    cl_mem      sourceBuffer = NULL;
    cl_mem      destBuffer = NULL;
    const char* kernelName = NULL;
    size_t      globalWorkSize[2] = {destWidth, destHeight};
    size_t      localWorkSize[2] = {16, 16};
    const void* kernelArgs[7];
    size_t      argSizes[7];
    uint32_t    argCount = 0;

    if (!pContext || !pSource || !pDestination)
    {
        R_CSTL_TRACE_RETURN ();
        return R_PACK_MIPMAP_ERROR_INVALID_ARGUMENT;
    }

    sourceBuffer = clCreateBuffer (
        pContext->openclContext,
        CL_MEM_READ_ONLY,
        pSource->width * pSource->height * 4,
        NULL,
        &error);
    if (error != CL_SUCCESS)
    {
        result = R_PACK_MIPMAP_ERROR_DISPATCH;
        R_CSTL_LOG_ERROR ("OpenCL create source buffer failed: %d", error);
        goto r_cleanup;
    }

    destBuffer = clCreateBuffer (
        pContext->openclContext,
        CL_MEM_WRITE_ONLY,
        destWidth * destHeight * 4,
        NULL,
        &error);
    if (error != CL_SUCCESS)
    {
        result = R_PACK_MIPMAP_ERROR_DISPATCH;
        R_CSTL_LOG_ERROR ("OpenCL create dest buffer failed: %d", error);
        goto r_cleanup_source;
    }

    error = clEnqueueWriteBuffer (
        pContext->openclQueue,
        sourceBuffer,
        CL_TRUE,
        0,
        pSource->width * pSource->height * 4,
        pSource->pPixels,
        0,
        NULL,
        NULL);
    if (error != CL_SUCCESS)
    {
        result = R_PACK_MIPMAP_ERROR_DISPATCH;
        R_CSTL_LOG_ERROR ("OpenCL write buffer failed: %d", error);
        goto r_cleanup_dest;
    }

    kernelName = (filter == R_PACK_MIPMAP_FILTER_GAUSSIAN) ? R_PACK_MIPMAP_GAUSSIAN_FILTER_NAME
                                                           : R_PACK_MIPMAP_BOX_FILTER_NAME;
    kernelArgs[0] = &sourceBuffer;
    argSizes[0] = sizeof (cl_mem);
    kernelArgs[1] = &destBuffer;
    argSizes[1] = sizeof (cl_mem);
    kernelArgs[2] = &pSource->width;
    argSizes[2] = sizeof (cl_uint);
    kernelArgs[3] = &pSource->height;
    argSizes[3] = sizeof (cl_uint);
    kernelArgs[4] = &destWidth;
    argSizes[4] = sizeof (cl_uint);
    kernelArgs[5] = &destHeight;
    argSizes[5] = sizeof (cl_uint);
    argCount = 6;

    if (filter == R_PACK_MIPMAP_FILTER_GAUSSIAN)
    {
        kernelArgs[6] = &sigma;
        argSizes[6] = sizeof (cl_float);
        argCount = 7;
    }

    result = r_pack_mipmap_execute_kernel (
        pContext->openclContext,
        pContext->openclDevice,
        kernelName,
        rpackMipmap_data,
        rpackMipmap_size * sizeof (uint32_t),
        NULL,
        NULL,
        kernelArgs,
        argSizes,
        argCount,
        globalWorkSize,
        localWorkSize);
    if (result != R_PACK_MIPMAP_OK) goto r_cleanup_dest;

    error = clEnqueueReadBuffer (
        pContext->openclQueue,
        destBuffer,
        CL_TRUE,
        0,
        destWidth * destHeight * 4,
        pDestination,
        0,
        NULL,
        NULL);
    if (error != CL_SUCCESS)
    {
        result = R_PACK_MIPMAP_ERROR_DISPATCH;
        R_CSTL_LOG_ERROR ("OpenCL read buffer failed: %d", error);
        goto r_cleanup_dest;
    }

r_cleanup_dest:
    clReleaseMemObject (destBuffer);
r_cleanup_source:
    clReleaseMemObject (sourceBuffer);
r_cleanup:
    R_CSTL_TRACE_RETURN ();
    return result;
}
#endif

static enum r_pack_mipmap_backend
r_pack_mipmap_select_backend (void)
{
    R_CSTL_TRACE_FUNCTION ();

#if defined(R_CUDA)
    int         deviceCount = 0;
    cudaError_t cudaError = cudaGetDeviceCount (&deviceCount);
    if (cudaError == cudaSuccess && deviceCount > 0)
    {
        R_CSTL_LOG_INFO ("Mipmap backend selected: CUDA (%d devices)", deviceCount);
        R_CSTL_TRACE_RETURN ();
        return R_PACK_MIPMAP_BACKEND_CUDA;
    }
#endif

#if defined(R_OPENCL)
    cl_uint platformCount = 0;
    cl_int  clError = clGetPlatformIDs (0, NULL, &platformCount);
    if (clError == CL_SUCCESS && platformCount > 0)
    {
        R_CSTL_LOG_INFO ("Mipmap backend selected: OpenCL (%u platforms)", platformCount);
        R_CSTL_TRACE_RETURN ();
        return R_PACK_MIPMAP_BACKEND_OPENCL;
    }
#endif

    R_CSTL_LOG_INFO ("Mipmap backend selected: CPU (fallback)");
    R_CSTL_TRACE_RETURN ();
    return R_PACK_MIPMAP_BACKEND_CPU;
}

R_PACK_API int
r_pack_mipmap_initialize (
    void*                          pContext,
    void*                          pDevice,
    void*                          pQueue,
    const char*                    pKernelSource,
    size_t                         kernelSourceSize,
    struct r_pack_mipmap_context** ppOutContext)
{
    R_CSTL_TRACE_FUNCTION ();

    int                           result = R_PACK_MIPMAP_OK;
    struct r_pack_mipmap_context* pMipmap = NULL;
    (void)pContext;
    (void)pDevice;
    (void)pQueue;
    (void)pKernelSource;
    (void)kernelSourceSize;

    if (!ppOutContext)
    {
        R_CSTL_TRACE_RETURN ();
        return R_PACK_MIPMAP_ERROR_INVALID_ARGUMENT;
    }

    pMipmap = (struct r_pack_mipmap_context*)r_cstl_heap_alloc (sizeof (*pMipmap));
    if (!pMipmap)
    {
        R_CSTL_TRACE_RETURN ();
        return R_PACK_MIPMAP_ERROR_INITIALIZATION;
    }

    memset (pMipmap, 0, sizeof (*pMipmap));
    pMipmap->backend = r_pack_mipmap_select_backend ();

    if (pMipmap->backend == R_PACK_MIPMAP_BACKEND_CUDA)
    {
#ifdef R_CUDA
        cudaError_t cudaError = cudaStreamCreate (&pMipmap->cudaStream);
        if (cudaError != cudaSuccess)
        {
            R_CSTL_LOG_ERROR ("Failed to create CUDA stream: %s", cudaGetErrorString (cudaError));
            pMipmap->backend = R_PACK_MIPMAP_BACKEND_CPU;
        }
#else
        pMipmap->backend = R_PACK_MIPMAP_BACKEND_CPU;
#endif
    }
    else if (pMipmap->backend == R_PACK_MIPMAP_BACKEND_OPENCL)
    {
#ifdef R_OPENCL
        cl_int         error = CL_SUCCESS;
        cl_platform_id platform = NULL;
        cl_uint        platformCount = 0;
        cl_device_id   device = NULL;
        cl_uint        deviceCount = 0;
        const char*    source = NULL;
        size_t         sourceSize = 0;

        error = clGetPlatformIDs (1, &platform, &platformCount);
        if (error != CL_SUCCESS || platformCount == 0)
        {
            R_CSTL_LOG_ERROR ("Failed to get OpenCL platforms");
            pMipmap->backend = R_PACK_MIPMAP_BACKEND_CPU;
            goto r_exit;
        }
        error = clGetDeviceIDs (platform, CL_DEVICE_TYPE_GPU, 1, &device, &deviceCount);
        if (error != CL_SUCCESS || deviceCount == 0)
        {
            R_CSTL_LOG_ERROR ("Failed to get OpenCL GPU devices");
            pMipmap->backend = R_PACK_MIPMAP_BACKEND_CPU;
            goto r_exit;
        }

        pMipmap->openclDevice = device;
        pMipmap->openclContext = clCreateContext (NULL, 1, &device, NULL, NULL, &error);
        if (error != CL_SUCCESS || !pMipmap->openclContext)
        {
            R_CSTL_LOG_ERROR ("Failed to create OpenCL context");
            pMipmap->backend = R_PACK_MIPMAP_BACKEND_CPU;
            goto r_exit;
        }

        pMipmap->openclQueue = clCreateCommandQueue (pMipmap->openclContext, device, 0, &error);
        if (error != CL_SUCCESS || !pMipmap->openclQueue)
        {
            R_CSTL_LOG_ERROR ("Failed to create OpenCL command queue");
            result = R_PACK_MIPMAP_ERROR_INITIALIZATION;
            goto r_cleanup_context;
        }
        source = (const char*)rpackMipmap_data;
        sourceSize = rpackMipmap_size * sizeof (uint32_t);

        pMipmap->openclProgram
            = clCreateProgramWithIL (pMipmap->openclContext, (const void*)&source, sourceSize, &error);
        if (error != CL_SUCCESS || !pMipmap->openclProgram)
        {
            R_CSTL_LOG_ERROR ("Failed to create OpenCL program from binary");
            result = R_PACK_MIPMAP_ERROR_INITIALIZATION;
            goto r_cleanup_queue;
        }

        error = clBuildProgram (pMipmap->openclProgram, 1, &device, NULL, NULL, NULL);
        if (error != CL_SUCCESS)
        {
            R_CSTL_LOG_ERROR ("Failed to build OpenCL program");
            result = R_PACK_MIPMAP_ERROR_INITIALIZATION;
            goto r_cleanup_program;
        }

        pMipmap->openclBoxKernel
            = clCreateKernel (pMipmap->openclProgram, R_PACK_MIPMAP_BOX_FILTER_NAME, &error);
        pMipmap->openclGaussianKernel
            = clCreateKernel (pMipmap->openclProgram, R_PACK_MIPMAP_GAUSSIAN_FILTER_NAME, &error);
        if (error != CL_SUCCESS || !pMipmap->openclBoxKernel || !pMipmap->openclGaussianKernel)
        {
            R_CSTL_LOG_ERROR ("Failed to create OpenCL kernels");
            result = R_PACK_MIPMAP_ERROR_INITIALIZATION;
            goto r_cleanup_kernels;
        }
#else
        pMipmap->backend = R_PACK_MIPMAP_BACKEND_CPU;
#endif
    }

    goto r_exit;

#ifdef R_OPENCL
r_cleanup_kernels:
    clReleaseKernel (pMipmap->openclBoxKernel);
    clReleaseKernel (pMipmap->openclGaussianKernel);
r_cleanup_program:
    clReleaseProgram (pMipmap->openclProgram);
r_cleanup_queue:
    clReleaseCommandQueue (pMipmap->openclQueue);
r_cleanup_context:
    clReleaseContext (pMipmap->openclContext);
    pMipmap->backend = R_PACK_MIPMAP_BACKEND_CPU;
#endif

r_exit:
    *ppOutContext = pMipmap;
    R_CSTL_TRACE_RETURN ();
    return R_PACK_MIPMAP_OK;
}

R_PACK_API void
r_pack_mipmap_shutdown (struct r_pack_mipmap_context* pContext)
{
    R_CSTL_TRACE_FUNCTION ();

    if (!pContext)
    {
        R_CSTL_TRACE_RETURN ();
        return;
    }

#ifdef R_CUDA
    if (pContext->backend == R_PACK_MIPMAP_BACKEND_CUDA && pContext->cudaStream)
        cudaStreamDestroy (pContext->cudaStream);
#endif

#ifdef R_OPENCL
    if (pContext->backend == R_PACK_MIPMAP_BACKEND_OPENCL)
    {
        clReleaseKernel (pContext->openclBoxKernel);
        clReleaseKernel (pContext->openclGaussianKernel);
        clReleaseProgram (pContext->openclProgram);
        clReleaseCommandQueue (pContext->openclQueue);
        clReleaseContext (pContext->openclContext);
    }
#endif

    r_cstl_heap_free (pContext);
    R_CSTL_TRACE_RETURN ();
}

R_PACK_API int
r_pack_mipmap_dispatch (
    struct r_pack_mipmap_context* pContext,
    void*                         pSource,
    void*                         pDestination,
    uint32_t                      sourceWidth,
    uint32_t                      sourceHeight,
    uint32_t                      destinationWidth,
    uint32_t                      destinationHeight,
    enum r_pack_mipmap_filter     filter,
    float                         sigma)
{
    R_CSTL_TRACE_FUNCTION ();

    struct r_pack_input_image sourceImage
        = {.pPixels = (const uint8_t*)pSource,
           .width = sourceWidth,
           .height = sourceHeight,
           .stride = sourceWidth * 4,
           .pName = NULL};
#if defined(R_CUDA)
    return r_pack_mipmap_resizeCUDA (
        pContext,
        &sourceImage,
        (uint8_t*)pDestination,
        destinationWidth,
        destinationHeight,
        filter,
        sigma);
#elif defined(R_OPENCL)
    return r_pack_mipmap_resize_openCL (
        pContext,
        &sourceImage,
        (uint8_t*)pDestination,
        destinationWidth,
        destinationHeight,
        filter,
        sigma);
#endif
    return r_pack_mipmap_resizeCPU (
        &sourceImage,
        (uint8_t*)pDestination,
        destinationWidth,
        destinationHeight,
        filter,
        sigma);
}
