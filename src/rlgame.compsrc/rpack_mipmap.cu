/**
 * @brief CUDA kernel for mipmap generation using box filter
 * @param srcPixels Source image pixels (RGBA)
 * @param dstPixels Destination mipmap level pixels (RGBA)
 * @param srcWidth Source width
 * @param srcHeight Source height
 * @param dstWidth Destination width
 * @param dstHeight Destination height
 */
__global__ void R_RPack_MipmapBoxFilterKernel (
    const uchar4* srcPixels,
    uchar4* dstPixels,
    uint32_t srcWidth,
    uint32_t srcHeight,
    uint32_t dstWidth,
    uint32_t dstHeight)
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
    uint4 sum = make_uint4 (0, 0, 0, 0);
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
        dstPixels[y * dstWidth + x] = make_uchar4 (
            sum.x / count,
            sum.y / count,
            sum.z / count,
            sum.w / count);
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
__global__ void R_RPack_MipmapGaussianFilterKernel (
    const uchar4* srcPixels,
    uchar4* dstPixels,
    uint32_t srcWidth,
    uint32_t srcHeight,
    uint32_t dstWidth,
    uint32_t dstHeight,
    float sigma)
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
    float weightSum = 0.0f;

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
                float4 fPixel = make_float4 (
                    (float)pixel.x,
                    (float)pixel.y,
                    (float)pixel.z,
                    (float)pixel.w);
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
