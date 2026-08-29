#include "microbit/spirvrunner/microbit_spirv_texelimage.h"
#include "microbit/microbit_platform.h"

#include "rlgame.base/cstl/cstl_heap_allocator.h"

#include <math.h>
#include <string.h>

static size_t
R_SpirvTexelImageChannels (enum R_Microbit_SpirvTextureFormat format)
{
    switch (format)
    {
    case MICROBIT_SPIRV_TEXTURE_R8_UNORM:
    case MICROBIT_SPIRV_TEXTURE_R16_FLOAT:
    case MICROBIT_SPIRV_TEXTURE_R32_FLOAT:
        return 1u;
    case MICROBIT_SPIRV_TEXTURE_RG8_UNORM:
    case MICROBIT_SPIRV_TEXTURE_RG16_FLOAT:
    case MICROBIT_SPIRV_TEXTURE_RG32_FLOAT:
        return 2u;
    case MICROBIT_SPIRV_TEXTURE_RGB8_UNORM:
    case MICROBIT_SPIRV_TEXTURE_RGB16_FLOAT:
    case MICROBIT_SPIRV_TEXTURE_RGB32_FLOAT:
        return 3u;
    case MICROBIT_SPIRV_TEXTURE_RGBA8_UNORM:
    case MICROBIT_SPIRV_TEXTURE_BGRA8_UNORM:
    case MICROBIT_SPIRV_TEXTURE_RGBA16_FLOAT:
    case MICROBIT_SPIRV_TEXTURE_RGBA32_FLOAT:
        return 4u;
    default:
        return 0u;
    }
}

static size_t
R_SpirvTexelImageBytesPerChannel (enum R_Microbit_SpirvTextureFormat format)
{
    return format >= MICROBIT_SPIRV_TEXTURE_R16_FLOAT && format <= MICROBIT_SPIRV_TEXTURE_RGBA16_FLOAT ? 2u
                                                                                                       : 1u;
}

static size_t
R_SpirvTexelImagePixelStride (enum R_Microbit_SpirvTextureFormat format)
{
    size_t channels = R_SpirvTexelImageChannels (format);
    if (!channels) return 0u;
    return format >= MICROBIT_SPIRV_TEXTURE_R32_FLOAT ? channels * sizeof (float)
                                                      : channels * R_SpirvTexelImageBytesPerChannel (format);
}

static uint16_t
R_SpirvTexelImageFloatToHalf (float value)
{
    uint32_t bits;
    memcpy (&bits, &value, sizeof (bits));
    uint32_t sign = (bits >> 16u) & 0x8000u;
    const int exponent = (int)((bits >> 23u) & 0xFFu) - 127 + 15;
    uint32_t mantissa = bits & 0x7FFFFFu;
    if (exponent <= 0)
    {
        if (exponent < -10) return (uint16_t)sign;
        mantissa |= 0x800000u;
        return (uint16_t)(sign | (mantissa >> (14 - exponent)));
    }
    if (exponent >= 31) return (uint16_t)(sign | (mantissa ? 0x7E00u : 0x7C00u));
    return (uint16_t)(sign | ((uint32_t)exponent << 10u) | (mantissa >> 13u));
}

static float
R_SpirvTexelImageHalfToFloat (uint16_t value)
{
    const uint32_t sign = ((uint32_t)value & 0x8000u) << 16u;
    uint32_t exponent = ((uint32_t)value >> 10u) & 0x1Fu;
    uint32_t mantissa = value & 0x3FFu;
    uint32_t bits;
    if (!exponent)
    {
        if (!mantissa) bits = sign;
        else
        {
            exponent = 1u;
            while ((mantissa & 0x400u) == 0u)
            {
                mantissa <<= 1u;
                --exponent;
            }
            mantissa &= 0x3FFu;
            bits = sign | ((exponent + 112u) << 23u) | (mantissa << 13u);
        }
    }
    else if (exponent == 31u) bits = sign | 0x7F800000u | (mantissa << 13u);
    else bits = sign | ((exponent + 112u) << 23u) | (mantissa << 13u);
    float result;
    memcpy (&result, &bits, sizeof (result));
    return result;
}

static float
R_SpirvTexelImageClamp01 (float value)
{
    return fminf (fmaxf (value, 0.0f), 1.0f);
}

static int
R_SpirvTexelImageCreateBuffer (
    uint8_t**                          ppPixels,
    size_t*                            pLevelStride,
    uint32_t                           width,
    uint32_t                           height,
    uint32_t                           depth,
    enum R_Microbit_SpirvTextureFormat format)
{
    R_MICROBIT_ASSERT (ppPixels);
    size_t pixelStride = R_SpirvTexelImagePixelStride (format);
    if (!ppPixels || !pLevelStride || !width || !height || !depth || !pixelStride)
        return MICROBIT_SPIRV_TEXTURE_ERROR_INVALID_ARGUMENT;
    if ((size_t)width > SIZE_MAX / height || (size_t)width * height > SIZE_MAX / depth
        || (size_t)width * height * depth > SIZE_MAX / pixelStride)
        return MICROBIT_SPIRV_TEXTURE_ERROR_OUT_OF_MEMORY;
    *pLevelStride = (size_t)width * height * depth * pixelStride;
    *ppPixels = (uint8_t*)R_CSTL_HeapAlloc (*pLevelStride);
    if (!*ppPixels) return MICROBIT_SPIRV_TEXTURE_ERROR_OUT_OF_MEMORY;
    memset (*ppPixels, 0, *pLevelStride);
    return MICROBIT_SPIRV_TEXTURE_OK;
}

int
R_Microbit_SpirvTexture2DCreate (
    struct R_Microbit_SpirvTexture2D*      pTexture,
    uint32_t                               width,
    uint32_t                               height,
    enum R_Microbit_SpirvTextureFormat     format,
    struct R_Microbit_SpirvThreadExecutor* pool)
{
    R_MICROBIT_ASSERT (pTexture);
    memset (pTexture, 0, sizeof (*pTexture));
    int result
        = R_SpirvTexelImageCreateBuffer (&pTexture->pPixels, &pTexture->levelStride, width, height, 1u, format);
    if (result != MICROBIT_SPIRV_TEXTURE_OK) return result;
    pTexture->width = width;
    pTexture->height = height;
    pTexture->mipLevels = 1u;
    pTexture->format = format;
    pTexture->pixelStride = R_SpirvTexelImagePixelStride (format);
    pTexture->pPool = pool;
    pTexture->addressU = MICROBIT_SPIRV_TEXTURE_ADDRESS_CLAMP;
    pTexture->addressV = MICROBIT_SPIRV_TEXTURE_ADDRESS_CLAMP;
    return result;
}

void
R_Microbit_SpirvTexture2DDelete (struct R_Microbit_SpirvTexture2D* pTexture)
{

    R_CSTL_HeapFree (pTexture->pPixels);
    memset (pTexture, 0, sizeof (*pTexture));
}

int
R_Microbit_SpirvTexture3DCreate (
    struct R_Microbit_SpirvTexture3D*      pTexture,
    uint32_t                               width,
    uint32_t                               height,
    uint32_t                               depth,
    enum R_Microbit_SpirvTextureFormat     format,
    struct R_Microbit_SpirvThreadExecutor* pPool)
{
    if (!pTexture) return MICROBIT_SPIRV_TEXTURE_ERROR_INVALID_ARGUMENT;
    memset (pTexture, 0, sizeof (*pTexture));
    int result = R_SpirvTexelImageCreateBuffer (
        &pTexture->pPixels,
        &pTexture->levelStride,
        width,
        height,
        depth,
        format);
    if (result != MICROBIT_SPIRV_TEXTURE_OK) return result;
    pTexture->width = width;
    pTexture->height = height;
    pTexture->depth = depth;
    pTexture->format = format;
    pTexture->pixelStride = R_SpirvTexelImagePixelStride (format);
    pTexture->pPool = pPool;
    pTexture->addressU = MICROBIT_SPIRV_TEXTURE_ADDRESS_CLAMP;
    pTexture->addressV = MICROBIT_SPIRV_TEXTURE_ADDRESS_CLAMP;
    pTexture->addressW = MICROBIT_SPIRV_TEXTURE_ADDRESS_CLAMP;
    return result;
}

void
R_Microbit_SpirvTexture3DDelete (struct R_Microbit_SpirvTexture3D* texture)
{
    if (!texture) return;
    R_CSTL_HeapFree (texture->pPixels);
    memset (texture, 0, sizeof (*texture));
}

static float
R_SpirvTexelImageAddress (float value, enum R_Microbit_SpirvTextureAddressMode mode)
{
    if (mode == MICROBIT_SPIRV_TEXTURE_ADDRESS_REPEAT) return value - floorf (value);
    if (mode == MICROBIT_SPIRV_TEXTURE_ADDRESS_MIRRORED_REPEAT)
    {
        float wrapped = value - floorf (value);
        return ((int)floorf (value) & 1) ? 1.0f - wrapped : wrapped;
    }
    return R_SpirvTexelImageClamp01 (value);
}

static void
R_SpirvTexelImageReadPixel (
    const uint8_t*                     pPixel,
    enum R_Microbit_SpirvTextureFormat format,
    R_Microbit_SpirvVec4*              pVec)
{
    size_t channels = R_SpirvTexelImageChannels (format);
    pVec->x = pVec->y = pVec->z = 0.0f;
    pVec->w = 1.0f;
    for (size_t i = 0; i < channels; ++i)
        (&pVec->x)[i] = format <= MICROBIT_SPIRV_TEXTURE_BGRA8_UNORM ? pPixel[i] / 255.0f
                        : format <= MICROBIT_SPIRV_TEXTURE_RGBA16_FLOAT
                            ? R_SpirvTexelImageHalfToFloat (((const uint16_t*)pPixel)[i])
                            : ((const float*)pPixel)[i];
    if (format == MICROBIT_SPIRV_TEXTURE_BGRA8_UNORM)
    {
        float b = pVec->x;
        pVec->x = pVec->z;
        pVec->z = b;
    }
}

static void
R_SpirvTexelImageWritePixel (
    uint8_t*                           pPixel,
    enum R_Microbit_SpirvTextureFormat format,
    R_Microbit_SpirvVec4               vec)
{
    if (format == MICROBIT_SPIRV_TEXTURE_BGRA8_UNORM)
    {
        float b = vec.x;
        vec.x = vec.z;
        vec.z = b;
    }
    size_t channels = R_SpirvTexelImageChannels (format);
    for (size_t i = 0; i < channels; ++i)
    {
        float value = (&vec.x)[i];
        if (format <= MICROBIT_SPIRV_TEXTURE_BGRA8_UNORM)
            pPixel[i] = (uint8_t)lrintf (R_SpirvTexelImageClamp01 (value) * 255.0f);
        else if (format <= MICROBIT_SPIRV_TEXTURE_RGBA16_FLOAT)
            ((uint16_t*)pPixel)[i] = R_SpirvTexelImageFloatToHalf (value);
        else ((float*)pPixel)[i] = value;
    }
}

int
R_Microbit_SpirvTexture2DRead (
    const struct R_Microbit_SpirvTexture2D* pTexture,
    uint32_t                                x,
    uint32_t                                y,
    R_Microbit_SpirvVec4*                   pVec)
{
    R_MICROBIT_ASSERT (pTexture);
    R_MICROBIT_ASSERT (pVec);
    if (x >= pTexture->width || y >= pTexture->height) return MICROBIT_SPIRV_TEXTURE_ERROR_BOUNDS;
    R_SpirvTexelImageReadPixel (
        pTexture->pPixels + ((size_t)y * pTexture->width + x) * pTexture->pixelStride,
        pTexture->format,
        pVec);
    return 0;
}
int
R_Microbit_SpirvTexture2DWrite (
    struct R_Microbit_SpirvTexture2D* pTexture,
    uint32_t                          x,
    uint32_t                          y,
    R_Microbit_SpirvVec4              pVec)
{
    R_MICROBIT_ASSERT (pTexture);
    if (x >= pTexture->width || y >= pTexture->height) return MICROBIT_SPIRV_TEXTURE_ERROR_BOUNDS;
    R_SpirvTexelImageWritePixel (
        pTexture->pPixels + ((size_t)y * pTexture->width + x) * pTexture->pixelStride,
        pTexture->format,
        pVec);
    return 0;
}
int
R_Microbit_SpirvTexture3DRead (
    const struct R_Microbit_SpirvTexture3D* pTexture,
    uint32_t                                x,
    uint32_t                                y,
    uint32_t                                z,
    R_Microbit_SpirvVec4*                   pVec)
{
    R_MICROBIT_ASSERT (pTexture);
    R_MICROBIT_ASSERT (pVec);
    if (x >= pTexture->width || y >= pTexture->height || z >= pTexture->depth)
        return MICROBIT_SPIRV_TEXTURE_ERROR_BOUNDS;
    size_t index = ((size_t)z * pTexture->height + y) * pTexture->width + x;
    R_SpirvTexelImageReadPixel (pTexture->pPixels + index * pTexture->pixelStride, pTexture->format, pVec);
    return 0;
}
int
R_Microbit_SpirvTexture3DWrite (
    struct R_Microbit_SpirvTexture3D* pTexture,
    uint32_t                          x,
    uint32_t                          y,
    uint32_t                          z,
    R_Microbit_SpirvVec4              pVec)
{
    R_MICROBIT_ASSERT (pTexture);
    if (x >= pTexture->width || y >= pTexture->height || z >= pTexture->depth)
        return MICROBIT_SPIRV_TEXTURE_ERROR_BOUNDS;
    size_t index = ((size_t)z * pTexture->height + y) * pTexture->width + x;
    R_SpirvTexelImageWritePixel (pTexture->pPixels + index * pTexture->pixelStride, pTexture->format, pVec);
    return 0;
}

int
R_Microbit_SpirvTexture2DSampleLod (
    const struct R_Microbit_SpirvTexture2D* pTexture,
    float                                   u,
    float                                   v,
    float                                   lod,
    enum R_Microbit_SpirvTextureFilter      filter,
    R_Microbit_SpirvVec4*                   pVec)
{
    (void)lod;
    R_MICROBIT_ASSERT (pTexture);
    R_MICROBIT_ASSERT (pVec);
    u = R_SpirvTexelImageAddress (u, pTexture->addressU);
    v = R_SpirvTexelImageAddress (v, pTexture->addressV);
    if (filter == MICROBIT_SPIRV_TEXTURE_FILTER_NEAREST)
        return R_Microbit_SpirvTexture2DRead (
            pTexture,
            (uint32_t)floorf (u * (pTexture->width - 1u) + 0.5f),
            (uint32_t)floorf (v * (pTexture->height - 1u) + 0.5f),
            pVec);
    float    fx = u * (pTexture->width - 1u);
    float    fy = v * (pTexture->height - 1u);
    uint32_t x0 = (uint32_t)floorf (fx), y0 = (uint32_t)floorf (fy),
             x1 = x0 + 1u < pTexture->width ? x0 + 1u : x0, y1 = y0 + 1u < pTexture->height ? y0 + 1u : y0;
    R_Microbit_SpirvVec4 a, b, c, d;
    R_Microbit_SpirvTexture2DRead (pTexture, x0, y0, &a);
    R_Microbit_SpirvTexture2DRead (pTexture, x1, y0, &b);
    R_Microbit_SpirvTexture2DRead (pTexture, x0, y1, &c);
    R_Microbit_SpirvTexture2DRead (pTexture, x1, y1, &d);
    float tx = fx - x0;
    float ty = fy - y0;
    for (int i = 0; i < 4; ++i)
    {
        float ab = (&a.x)[i] + ((&b.x)[i] - (&a.x)[i]) * tx;
        float cd = (&c.x)[i] + ((&d.x)[i] - (&c.x)[i]) * tx;
        (&pVec->x)[i] = ab + (cd - ab) * ty;
    }
    return 0;
}
int
R_Microbit_SpirvTexture2DSample (
    const struct R_Microbit_SpirvTexture2D* pTexture,
    float                                   u,
    float                                   v,
    enum R_Microbit_SpirvTextureFilter      filter,
    R_Microbit_SpirvVec4*                   pVec)
{
    return R_Microbit_SpirvTexture2DSampleLod (pTexture, u, v, 0.0f, filter, pVec);
}
int
R_Microbit_SpirvTexture3DSample (
    const struct R_Microbit_SpirvTexture3D* pTexture,
    float                                   u,
    float                                   v,
    float                                   w,
    R_Microbit_SpirvVec4*                   pVec)
{
    if (!pTexture || !pVec) return -1;
    u = R_SpirvTexelImageAddress (u, pTexture->addressU);
    v = R_SpirvTexelImageAddress (v, pTexture->addressV);
    w = R_SpirvTexelImageAddress (w, pTexture->addressW);
    return R_Microbit_SpirvTexture3DRead (
        pTexture,
        (uint32_t)floorf (u * (pTexture->width - 1u) + 0.5f),
        (uint32_t)floorf (v * (pTexture->height - 1u) + 0.5f),
        (uint32_t)floorf (w * (pTexture->depth - 1u) + 0.5f),
        pVec);
}

int
R_Microbit_SpirvTexture2DFill (struct R_Microbit_SpirvTexture2D* pTexture, R_Microbit_SpirvVec4 vec)
{
    if (!pTexture) return -1;
    for (uint32_t y = 0; y < pTexture->height; ++y)
        for (uint32_t x = 0; x < pTexture->width; ++x)
            R_Microbit_SpirvTexture2DWrite (pTexture, x, y, vec);
    return 0;
}

struct R_SpirvTexelImageTile
{
        struct R_Microbit_SpirvTexture2D*   pTexture;
        uint32_t                            x, y, width, height;
        R_Microbit_SpirvTextureTileCallback function;
        void*                               userData;
};

static void
R_SpirvTexelImageRunTile (void* pData)
{
    const struct R_SpirvTexelImageTile* tile = (struct R_SpirvTexelImageTile*)pData;
    tile->function (tile->userData, tile->x, tile->y, tile->width, tile->height);
}
int
R_Microbit_SpirvTexture2DForEachTile (
    struct R_Microbit_SpirvTexture2D*   pTexture,
    uint32_t                            tileWidth,
    uint32_t                            tileHeight,
    R_Microbit_SpirvTextureTileCallback function,
    void*                               pUserData)
{
    R_MICROBIT_ASSERT (pTexture);
    R_MICROBIT_ASSERT (tileWidth);
    R_MICROBIT_ASSERT (tileHeight);
    R_MICROBIT_ASSERT (pUserData);
    if (!pTexture || !tileWidth || !tileHeight || !function) return -1;
    const uint32_t                capacity = pTexture->pPool ? 4u : 0u;
    struct R_SpirvTexelImageTile* tasks
        = (struct R_SpirvTexelImageTile*)R_CSTL_HeapAlloc ((capacity ? capacity : 1u) * sizeof (*tasks));
    if (!tasks) return -2;
    uint32_t count = 0;
    for (uint32_t y = 0; y < pTexture->height; y += tileHeight)
        for (uint32_t x = 0; x < pTexture->width; x += tileWidth)
        {
            struct R_SpirvTexelImageTile tile
                = {.pTexture = pTexture,
                   .x = x,
                   .y = y,
                   .width = x + tileWidth > pTexture->width ? pTexture->width - x : tileWidth,
                   .height = y + tileHeight > pTexture->height ? pTexture->height - y : tileHeight,
                   .function = function,
                   .userData = pUserData};
            if (pTexture->pPool)
            {
                tasks[count++] = tile;
                if (count == capacity)
                {
                    for (uint32_t i = 0; i < count; ++i)
                        R_Microbit_SpirvThreadExecutorSubmit (
                            pTexture->pPool,
                            R_SpirvTexelImageRunTile,
                            &tasks[i]);
                    R_Microbit_SpirvThreadExecutorWait (pTexture->pPool);
                    count = 0;
                }
            }
            else function (pUserData, tile.x, tile.y, tile.width, tile.height);
        }
    if (pTexture->pPool && count)
    {
        for (uint32_t i = 0; i < count; ++i)
            R_Microbit_SpirvThreadExecutorSubmit (pTexture->pPool, R_SpirvTexelImageRunTile, &tasks[i]);
        R_Microbit_SpirvThreadExecutorWait (pTexture->pPool);
    }
    R_CSTL_HeapFree (tasks);
    return 0;
}
