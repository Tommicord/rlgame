#include "rpack_mipmap.h"

#if defined(R_OPENCL)
#include <CL/cl.h>
#include <math.h>
#include <stdlib.h>

struct R_Pack_MipmapContext
{
        cl_context       context;
        cl_command_queue queue;
        cl_program       program;
        cl_kernel        boxKernel;
        cl_kernel        gaussianKernel;
};

int
R_Pack_MipmapInitialize (
    void*                         pContext,
    void*                         pDevice,
    void*                         pQueue,
    const char*                   pKernelSource,
    size_t                        kernelSourceSize,
    struct R_Pack_MipmapContext** ppOutContext)
{
    R_RPACK_MIPMAP_VALIDATE (
        pContext != NULL && pDevice != NULL && pQueue != NULL,
        R_RPACK_MIPMAP_ERROR_INVALID_ARGUMENT);
    R_RPACK_MIPMAP_VALIDATE (
        pKernelSource != NULL && kernelSourceSize > 0,
        R_RPACK_MIPMAP_ERROR_INVALID_ARGUMENT);
    R_RPACK_MIPMAP_VALIDATE (ppOutContext != NULL, R_RPACK_MIPMAP_ERROR_INVALID_ARGUMENT);

    struct R_Pack_MipmapContext* pMipmap = calloc (1, sizeof (*pMipmap));
    if (!pMipmap) return R_RPACK_MIPMAP_ERROR_INITIALIZATION;

    cl_int error = CL_SUCCESS;
    pMipmap->context = (cl_context)pContext;
    pMipmap->queue = (cl_command_queue)pQueue;
    pMipmap->program
        = clCreateProgramWithSource (pMipmap->context, 1, &pKernelSource, &kernelSourceSize, &error);
    if (error == CL_SUCCESS)
        error = clBuildProgram (pMipmap->program, 1, (const cl_device_id*)&pDevice, NULL, NULL, NULL);
    if (error == CL_SUCCESS)
        pMipmap->boxKernel = clCreateKernel (pMipmap->program, "R_RPack_MipmapBoxFilterKernel", &error);
    if (error == CL_SUCCESS)
        pMipmap->gaussianKernel
            = clCreateKernel (pMipmap->program, "R_RPack_MipmapGaussianFilterKernel", &error);
    if (error != CL_SUCCESS)
    {
        R_Pack_MipmapShutdown (pMipmap);
        return R_RPACK_MIPMAP_ERROR_INITIALIZATION;
    }

    *ppOutContext = pMipmap;
    return R_RPACK_MIPMAP_OK;
}

void
R_Pack_MipmapShutdown (struct R_Pack_MipmapContext* pContext)
{
    if (!pContext) return;
    if (pContext->boxKernel) clReleaseKernel (pContext->boxKernel);
    if (pContext->gaussianKernel) clReleaseKernel (pContext->gaussianKernel);
    if (pContext->program) clReleaseProgram (pContext->program);
    free (pContext);
}

int
R_Pack_MipmapDispatch (
    struct R_Pack_MipmapContext* pContext,
    void*                        pSource,
    void*                        pDestination,
    uint32_t                     sourceWidth,
    uint32_t                     sourceHeight,
    uint32_t                     destinationWidth,
    uint32_t                     destinationHeight,
    enum R_Pack_MipmapFilter     filter,
    float                        sigma)
{
    R_RPACK_MIPMAP_VALIDATE (
        pContext != NULL && pSource != NULL && pDestination != NULL,
        R_RPACK_MIPMAP_ERROR_INVALID_ARGUMENT);
    R_RPACK_MIPMAP_VALIDATE (
        sourceWidth > 0 && sourceHeight > 0 && destinationWidth > 0 && destinationHeight > 0,
        R_RPACK_MIPMAP_ERROR_INVALID_ARGUMENT);
    R_RPACK_MIPMAP_VALIDATE (filter <= R_RPACK_MIPMAP_FILTER_GAUSSIAN, R_RPACK_MIPMAP_ERROR_INVALID_ARGUMENT);
    R_RPACK_MIPMAP_VALIDATE (
        filter == R_RPACK_MIPMAP_FILTER_BOX || (isfinite (sigma) && sigma > 0.0f),
        R_RPACK_MIPMAP_ERROR_INVALID_ARGUMENT);

    cl_kernel kernel
        = filter == R_RPACK_MIPMAP_FILTER_GAUSSIAN ? pContext->gaussianKernel : pContext->boxKernel;
    cl_mem source = (cl_mem)pSource;
    cl_mem destination = (cl_mem)pDestination;
    cl_int error = clSetKernelArg (kernel, 0, sizeof (source), &source);
    error |= clSetKernelArg (kernel, 1, sizeof (destination), &destination);
    error |= clSetKernelArg (kernel, 2, sizeof (sourceWidth), &sourceWidth);
    error |= clSetKernelArg (kernel, 3, sizeof (sourceHeight), &sourceHeight);
    error |= clSetKernelArg (kernel, 4, sizeof (destinationWidth), &destinationWidth);
    error |= clSetKernelArg (kernel, 5, sizeof (destinationHeight), &destinationHeight);
    if (filter == R_RPACK_MIPMAP_FILTER_GAUSSIAN) error |= clSetKernelArg (kernel, 6, sizeof (sigma), &sigma);
    if (error != CL_SUCCESS) return R_RPACK_MIPMAP_ERROR_DISPATCH;

    const size_t globalSize[2] = {destinationWidth, destinationHeight};
    return clEnqueueNDRangeKernel (pContext->queue, kernel, 2, NULL, globalSize, NULL, 0, NULL, NULL)
                   == CL_SUCCESS
               ? R_RPACK_MIPMAP_OK
               : R_RPACK_MIPMAP_ERROR_DISPATCH;
}

#elif !defined(R_CUDA)

struct R_Pack_MipmapContext
{
        int unused;
};

int
R_Pack_MipmapInitialize (
    void*                         pContext,
    void*                         pDevice,
    void*                         pQueue,
    const char*                   pKernelSource,
    size_t                        kernelSourceSize,
    struct R_Pack_MipmapContext** ppOutContext)
{
    (void)pContext;
    (void)pDevice;
    (void)pQueue;
    (void)pKernelSource;
    (void)kernelSourceSize;
    (void)ppOutContext;
    return R_RPACK_MIPMAP_ERROR_UNSUPPORTED;
}

void
R_Pack_MipmapShutdown (struct R_Pack_MipmapContext* pContext)
{
    (void)pContext;
}

int
R_Pack_MipmapDispatch (
    struct R_Pack_MipmapContext* pContext,
    void*                        pSource,
    void*                        pDestination,
    uint32_t                     sourceWidth,
    uint32_t                     sourceHeight,
    uint32_t                     destinationWidth,
    uint32_t                     destinationHeight,
    enum R_Pack_MipmapFilter     filter,
    float                        sigma)
{
    (void)pContext;
    (void)pSource;
    (void)pDestination;
    (void)sourceWidth;
    (void)sourceHeight;
    (void)destinationWidth;
    (void)destinationHeight;
    (void)filter;
    (void)sigma;
    return R_RPACK_MIPMAP_ERROR_UNSUPPORTED;
}

#endif
