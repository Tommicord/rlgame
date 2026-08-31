/**
 * @brief OpenCL kernel for mipmap generation using box filter
 * @param srcPixels Source image pixels (RGBA)
 * @param dstPixels Destination mipmap level pixels (RGBA)
 * @param srcWidth Source width
 * @param srcHeight Source height
 * @param dstWidth Destination width
 * @param dstHeight Destination height
 */
__kernel void r_pack_mipmap_box_filter_kernel (
    __global const uchar4* srcPixels,
    __global uchar4* dstPixels,
    const uint srcWidth,
    const uint srcHeight,
    const uint dstWidth,
    const uint dstHeight)
{
    uint x = get_global_id (0);
    uint y = get_global_id (1);

    if (x >= dstWidth || y >= dstHeight)
    {
        return;
    }
    float scaleX = (float)srcWidth / (float)dstWidth;
    float scaleY = (float)srcHeight / (float)dstHeight;

    uint srcX = (uint)(x * scaleX);
    uint srcY = (uint)(y * scaleY);

    // Sample 2x2 region
    uint4 sum = (uint4)(0, 0, 0, 0);
    uint count = 0;

    for (uint dy = 0; dy < 2; ++dy)
    {
        for (uint dx = 0; dx < 2; ++dx)
        {
            uint sx = srcX + dx;
            uint sy = srcY + dy;

            if (sx < srcWidth && sy < srcHeight)
            {
                uint4 pixel = convert_uint4 (srcPixels[sy * srcWidth + sx]);
                sum += pixel;
                count++;
            }
        }
    }

    // Average
    if (count > 0)
    {
        dstPixels[y * dstWidth + x] = convert_uchar4 (sum / count);
    }
    else
    {
        dstPixels[y * dstWidth + x] = (uchar4)(0, 0, 0, 0);
    }
}

/**
 * @brief OpenCL kernel for mipmap generation using Gaussian filter
 * @param srcPixels Source image pixels (RGBA)
 * @param dstPixels Destination mipmap level pixels (RGBA)
 * @param srcWidth Source width
 * @param srcHeight Source height
 * @param dstWidth Destination width
 * @param dstHeight Destination height
 * @param sigma Gaussian sigma value
 */
__kernel void r_pack_mipmap_gaussian_filter_kernel (
    __global const uchar4* srcPixels,
    __global uchar4* dstPixels,
    const uint srcWidth,
    const uint srcHeight,
    const uint dstWidth,
    const uint dstHeight,
    const float sigma)
{
    uint x = get_global_id (0);
    uint y = get_global_id (1);

    if (x >= dstWidth || y >= dstHeight)
    {
        return;
    }

    float scaleX = (float)srcWidth / (float)dstWidth;
    float scaleY = (float)srcHeight / (float)dstHeight;

    float srcX = x * scaleX;
    float srcY = y * scaleY;

    // Gaussian kernel radius (3 sigma)
    int radius = (int)(3.0f * sigma);
    if (radius < 1) radius = 1;

    float4 sum = (float4)(0.0f, 0.0f, 0.0f, 0.0f);
    float weightSum = 0.0f;

    for (int dy = -radius; dy <= radius; ++dy)
    {
        for (int dx = -radius; dx <= radius; ++dx)
        {
            int sx = (int)(srcX + dx);
            int sy = (int)(srcY + dy);

            if (sx >= 0 && sx < (int)srcWidth && sy >= 0 && sy < (int)srcHeight)
            {
                float distance = sqrt ((float)(dx * dx + dy * dy));
                float weight = exp (-0.5f * (distance * distance) / (sigma * sigma));

                float4 pixel = convert_float4 (srcPixels[sy * srcWidth + sx]);
                sum += pixel * weight;
                weightSum += weight;
            }
        }
    }

    if (weightSum > 0.0f)
    {
        dstPixels[y * dstWidth + x] = convert_uchar4 (sum / weightSum);
    }
    else
    {
        dstPixels[y * dstWidth + x] = (uchar4)(0, 0, 0, 0);
    }
}
