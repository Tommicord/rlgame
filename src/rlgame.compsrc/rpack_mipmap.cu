#include "rpack/rpack_mipmap.h"

#include <cuda_runtime.h>
#include <stdlib.h>

struct r_pack_mipmap_context
{
    cudaStream_t stream;
};

#include "rpack/rpack_mipmap.h"

#include <cuda_runtime.h>
#include <math.h>
#include <stdlib.h>

struct r_pack_mipmap_context
{
    cudaStream_t stream;
};

/**
 * @brief CUDA kernel for mipmap generation using box filter
 * @param srcPixels Source image pixels (RGBA)
 * @param dstPixels Destination mipmap level pixels (RGBA)
 * @param srcWidth Source width
 * @param srcHeight Source height
 * @param dstWidth Destination width
 * @param dstHeight Destination height
 */
__global__ void
r_pack_mipmap_box_filter_kernel (
    const uchar4* srcPixels,
    uchar4*       dstPixels,
    uint32_t      srcWidth,
    uint32_t      srcHeight,
    uint32_t      dstWidth,
    uint32_t      dstHeight)
{
    uint32_t x = blockIdx.x * blockDim.x + threadIdx.x;
    uint32_t y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x >= dstWidth || y >= dstHeight)
    {
        return;
    }
    float scaleX = (float)srcWidth / (float)dstWidth;
    float scaleY = (float)srcHeight / (float)dstHeight;

    uint32_t srcX = (uint32_t)(x * scaleX);
    uint32_t srcY = (uint32_t)(y * scaleY);

    // Sample 2x2 region
    uint4    sum = make_uint4 (0, 0, 0, 0);
    uint32_t count = 0;

    for (uint32_t dy = 0; dy < 2; ++dy)
    {
        for (uint32_t dx = 0; dx < 2; ++dx)
        {
            uint32_t sx = srcX + dx;
            uint32_t sy = srcY + dy;

            if (sx < srcWidth && sy < srcHeight)
            {
                uint4 pixel = make_uint4 (srcPixels[sy * srcWidth + sx]);
                sum += pixel;
                count++;
            }
        }
    }

    // Average
    if (count > 0)
    {
        dstPixels[y * dstWidth + x]
            = make_uchar4 (sum.x / count, sum.y / count, sum.z / count, sum.w / count);
    }
    else
    {
        dstPixels[y * dstWidth + x] = make_uchar4 (0, 0, 0, 0);
    }
}

/**
 * @brief CUDA kernel for mipmap generation using Gaussian filter
 * @param srcPixels Source image pixels (RGBA)
 * @param dstPixels Destination mipmap level pixels (RGBA)
 * @param srcWidth Source width
 * @param srcHeight Source height
 * @param dstWidth Destination width
 * @param dstHeight Destination height
 * @param sigma Gaussian sigma value
 */
__global__ void
r_pack_mipmap_gaussian_filter_kernel (
    const uchar4* srcPixels,
    uchar4*       dstPixels,
    uint32_t      srcWidth,
    uint32_t      srcHeight,
    uint32_t      dstWidth,
    uint32_t      dstHeight,
    float         sigma)
{
    uint32_t x = blockIdx.x * blockDim.x + threadIdx.x;
    uint32_t y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x >= dstWidth || y >= dstHeight)
    {
        return;
    }

    float scaleX = (float)srcWidth / (float)dstWidth;
    float scaleY = (float)srcHeight / (float)dstHeight;

    float srcX = x * scaleX;
    float srcY = y * scaleY;

    // Gaussian kernel radius (3 sigma)
    int32_t radius = (int32_t)(3.0f * sigma);
    if (radius < 1) radius = 1;

    float4 sum = make_float4 (0.0f, 0.0f, 0.0f, 0.0f);
    float  weightSum = 0.0f;

    for (int32_t dy = -radius; dy <= radius; ++dy)
    {
        for (int32_t dx = -radius; dx <= radius; ++dx)
        {
            int32_t sx = (int32_t)(srcX + dx);
            int32_t sy = (int32_t)(srcY + dy);

            if (sx >= 0 && sx < (int32_t)srcWidth && sy >= 0 && sy < (int32_t)srcHeight)
            {
                float distance = sqrtf ((float)(dx * dx + dy * dy));
                float weight = expf (-0.5f * (distance * distance) / (sigma * sigma));

                uchar4 pixel = srcPixels[sy * srcWidth + sx];
                float4 fPixel = make_float4 ((float)pixel.x, (float)pixel.y, (float)pixel.z, (float)pixel.w);
                sum += fPixel * weight;
                weightSum += weight;
            }
        }
    }

    if (weightSum > 0.0f)
    {
        dstPixels[y * dstWidth + x] = make_uchar4 (
            (uint8_t)(sum.x / weightSum),
            (uint8_t)(sum.y / weightSum),
            (uint8_t)(sum.z / weightSum),
            (uint8_t)(sum.w / weightSum));
    }
    else
    {
        dstPixels[y * dstWidth + x] = make_uchar4 (0, 0, 0, 0);
    }
}

extern "C" cudaError_t
r_pack_mipmap_launch_box_filter (const void* pSource, void* pDestination, uint32_t sourceWidth,
                              uint32_t sourceHeight, uint32_t destinationWidth, uint32_t destinationHeight,
                              cudaStream_t stream)
{
    dim3 block (16, 16);
    dim3 grid ((destinationWidth + block.x - 1) / block.x, (destinationHeight + block.y - 1) / block.y);
    r_pack_mipmap_box_filter_kernel<<<grid, block, 0, stream>>> (
        (const uchar4*)pSource, (uchar4*)pDestination, sourceWidth, sourceHeight, destinationWidth, destinationHeight);
    return cudaGetLastError ();
}

extern "C" cudaError_t
r_pack_mipmap_launch_gaussian_filter (const void* pSource, void* pDestination, uint32_t sourceWidth,
                                   uint32_t sourceHeight, uint32_t destinationWidth, uint32_t destinationHeight,
                                   float sigma, cudaStream_t stream)
{
    if (!(sigma > 0.0f) || !isfinite (sigma)) return cudaErrorInvalidValue;
    dim3 block (16, 16);
    dim3 grid ((destinationWidth + block.x - 1) / block.x, (destinationHeight + block.y - 1) / block.y);
    r_pack_mipmap_gaussian_filter_kernel<<<grid, block, 0, stream>>> (
        (const uchar4*)pSource, (uchar4*)pDestination, sourceWidth, sourceHeight, destinationWidth, destinationHeight, sigma);
    return cudaGetLastError ();
}

extern "C" int
r_pack_mipmap_initialize (void* pContext, void* pDevice, void* pQueue, const char* pKernelSource,
                         size_t kernelSourceSize, struct r_pack_mipmap_context** ppOutContext)
{
    (void)pContext;
    (void)pDevice;
    (void)pKernelSource;
    (void)kernelSourceSize;
    R_PACK_ASSERT (pQueue != NULL, R_PACK_MIPMAP_ERROR_INVALID_ARGUMENT);
    R_PACK_ASSERT (ppOutContext != NULL, R_PACK_MIPMAP_ERROR_INVALID_ARGUMENT);

    struct r_pack_mipmap_context* pMipmap = calloc (1, sizeof (*pMipmap));
    if (!pMipmap) return R_PACK_MIPMAP_ERROR_INITIALIZATION;
    pMipmap->stream = (cudaStream_t)pQueue;
    *ppOutContext = pMipmap;
    return R_PACK_MIPMAP_OK;
}

extern "C" void
r_pack_mipmap_shutdown (struct r_pack_mipmap_context* pContext)
{
    free (pContext);
}

extern "C" int
r_pack_mipmap_dispatch (struct r_pack_mipmap_context* pContext, void* pSource, void* pDestination,
                       uint32_t sourceWidth, uint32_t sourceHeight, uint32_t destinationWidth,
                       uint32_t destinationHeight, enum r_pack_mipmap_filter filter, float sigma)
{
    R_PACK_ASSERT (pContext != NULL && pSource != NULL && pDestination != NULL,
                             R_PACK_MIPMAP_ERROR_INVALID_ARGUMENT);
    R_PACK_ASSERT (sourceWidth > 0 && sourceHeight > 0 && destinationWidth > 0 && destinationHeight > 0,
                             R_PACK_MIPMAP_ERROR_INVALID_ARGUMENT);
    R_PACK_ASSERT (filter <= R_PACK_MIPMAP_FILTER_GAUSSIAN, R_PACK_MIPMAP_ERROR_INVALID_ARGUMENT);

    cudaError_t error;
    if (filter == R_PACK_MIPMAP_FILTER_GAUSSIAN)
    {
        error = r_pack_mipmap_launch_gaussian_filter (pSource, pDestination, sourceWidth, sourceHeight,
                                                   destinationWidth, destinationHeight, sigma, pContext->stream);
    }
    else
    {
        error = r_pack_mipmap_launch_box_filter (pSource, pDestination, sourceWidth, sourceHeight,
                                              destinationWidth, destinationHeight, pContext->stream);
    }
    return error == cudaSuccess ? R_PACK_MIPMAP_OK : R_PACK_MIPMAP_ERROR_DISPATCH;
}
