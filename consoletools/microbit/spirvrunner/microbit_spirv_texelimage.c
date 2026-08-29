#include "microbit/spirvrunner/microbit_spirv_texelimage.h"

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
    int      exponent = (int)((bits >> 23u) & 0xFFu) - 127 + 15;
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
    uint32_t sign = ((uint32_t)value & 0x8000u) << 16u;
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
    struct R_Microbit_SpirvTexture2D*     texture,
    uint32_t                              width,
    uint32_t                              height,
    enum R_Microbit_SpirvTextureFormat    format,
    struct R_Microbit_SpirvCpuThreadPool* pool)
{
    if (!texture) return MICROBIT_SPIRV_TEXTURE_ERROR_INVALID_ARGUMENT;
    memset (texture, 0, sizeof (*texture));
    int result
        = R_SpirvTexelImageCreateBuffer (&texture->pPixels, &texture->levelStride, width, height, 1u, format);
    if (result != MICROBIT_SPIRV_TEXTURE_OK) return result;
    texture->width = width;
    texture->height = height;
    texture->mipLevels = 1u;
    texture->format = format;
    texture->pixelStride = R_SpirvTexelImagePixelStride (format);
    texture->pPool = pool;
    texture->addressU = MICROBIT_SPIRV_TEXTURE_ADDRESS_CLAMP;
    texture->addressV = MICROBIT_SPIRV_TEXTURE_ADDRESS_CLAMP;
    return result;
}

void
R_Microbit_SpirvTexture2DDelete (struct R_Microbit_SpirvTexture2D* texture)
{
    if (!texture) return;
    R_CSTL_HeapFree (texture->pPixels);
    memset (texture, 0, sizeof (*texture));
}

int
R_Microbit_SpirvTexture3DCreate (
    struct R_Microbit_SpirvTexture3D*     texture,
    uint32_t                              width,
    uint32_t                              height,
    uint32_t                              depth,
    enum R_Microbit_SpirvTextureFormat    format,
    struct R_Microbit_SpirvCpuThreadPool* pool)
{
    if (!texture) return MICROBIT_SPIRV_TEXTURE_ERROR_INVALID_ARGUMENT;
    memset (texture, 0, sizeof (*texture));
    int result = R_SpirvTexelImageCreateBuffer (
        &texture->pPixels,
        &texture->levelStride,
        width,
        height,
        depth,
        format);
    if (result != MICROBIT_SPIRV_TEXTURE_OK) return result;
    texture->width = width;
    texture->height = height;
    texture->depth = depth;
    texture->format = format;
    texture->pixelStride = R_SpirvTexelImagePixelStride (format);
    texture->pPool = pool;
    texture->addressU = MICROBIT_SPIRV_TEXTURE_ADDRESS_CLAMP;
    texture->addressV = MICROBIT_SPIRV_TEXTURE_ADDRESS_CLAMP;
    texture->addressW = MICROBIT_SPIRV_TEXTURE_ADDRESS_CLAMP;
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
    const uint8_t*                     pixel,
    enum R_Microbit_SpirvTextureFormat format,
    R_Microbit_SpirvVec4*              color)
{
    size_t channels = R_SpirvTexelImageChannels (format);
    color->x = color->y = color->z = 0.0f;
    color->w = 1.0f;
    for (size_t i = 0; i < channels; ++i)
        (&color->x)[i] = format <= MICROBIT_SPIRV_TEXTURE_BGRA8_UNORM ? pixel[i] / 255.0f
                         : format <= MICROBIT_SPIRV_TEXTURE_RGBA16_FLOAT
                             ? R_SpirvTexelImageHalfToFloat (((const uint16_t*)pixel)[i])
                             : ((const float*)pixel)[i];
    if (format == MICROBIT_SPIRV_TEXTURE_BGRA8_UNORM)
    {
        float b = color->x;
        color->x = color->z;
        color->z = b;
    }
}

static void
R_SpirvTexelImageWritePixel (
    uint8_t*                           pixel,
    enum R_Microbit_SpirvTextureFormat format,
    R_Microbit_SpirvVec4               color)
{
    if (format == MICROBIT_SPIRV_TEXTURE_BGRA8_UNORM)
    {
        float b = color.x;
        color.x = color.z;
        color.z = b;
    }
    size_t channels = R_SpirvTexelImageChannels (format);
    for (size_t i = 0; i < channels; ++i)
    {
        float value = (&color.x)[i];
        if (format <= MICROBIT_SPIRV_TEXTURE_BGRA8_UNORM)
            pixel[i] = (uint8_t)lrintf (R_SpirvTexelImageClamp01 (value) * 255.0f);
        else if (format <= MICROBIT_SPIRV_TEXTURE_RGBA16_FLOAT)
            ((uint16_t*)pixel)[i] = R_SpirvTexelImageFloatToHalf (value);
        else ((float*)pixel)[i] = value;
    }
}

int
R_Microbit_SpirvTexture2DRead (
    const struct R_Microbit_SpirvTexture2D* texture,
    uint32_t                                x,
    uint32_t                                y,
    R_Microbit_SpirvVec4*                   color)
{
    if (!texture || !color) return MICROBIT_SPIRV_TEXTURE_ERROR_INVALID_ARGUMENT;
    if (x >= texture->width || y >= texture->height) return MICROBIT_SPIRV_TEXTURE_ERROR_BOUNDS;
    R_SpirvTexelImageReadPixel (
        texture->pPixels + ((size_t)y * texture->width + x) * texture->pixelStride,
        texture->format,
        color);
    return 0;
}
int
R_Microbit_SpirvTexture2DWrite (
    struct R_Microbit_SpirvTexture2D* texture,
    uint32_t                          x,
    uint32_t                          y,
    R_Microbit_SpirvVec4              color)
{
    if (!texture) return MICROBIT_SPIRV_TEXTURE_ERROR_INVALID_ARGUMENT;
    if (x >= texture->width || y >= texture->height) return MICROBIT_SPIRV_TEXTURE_ERROR_BOUNDS;
    R_SpirvTexelImageWritePixel (
        texture->pPixels + ((size_t)y * texture->width + x) * texture->pixelStride,
        texture->format,
        color);
    return 0;
}
int
R_Microbit_SpirvTexture3DRead (
    const struct R_Microbit_SpirvTexture3D* texture,
    uint32_t                                x,
    uint32_t                                y,
    uint32_t                                z,
    R_Microbit_SpirvVec4*                   color)
{
    if (!texture || !color) return MICROBIT_SPIRV_TEXTURE_ERROR_INVALID_ARGUMENT;
    if (x >= texture->width || y >= texture->height || z >= texture->depth)
        return MICROBIT_SPIRV_TEXTURE_ERROR_BOUNDS;
    size_t index = ((size_t)z * texture->height + y) * texture->width + x;
    R_SpirvTexelImageReadPixel (texture->pPixels + index * texture->pixelStride, texture->format, color);
    return 0;
}
int
R_Microbit_SpirvTexture3DWrite (
    struct R_Microbit_SpirvTexture3D* texture,
    uint32_t                          x,
    uint32_t                          y,
    uint32_t                          z,
    R_Microbit_SpirvVec4              color)
{
    if (!texture) return MICROBIT_SPIRV_TEXTURE_ERROR_INVALID_ARGUMENT;
    if (x >= texture->width || y >= texture->height || z >= texture->depth)
        return MICROBIT_SPIRV_TEXTURE_ERROR_BOUNDS;
    size_t index = ((size_t)z * texture->height + y) * texture->width + x;
    R_SpirvTexelImageWritePixel (texture->pPixels + index * texture->pixelStride, texture->format, color);
    return 0;
}

int
R_Microbit_SpirvTexture2DSampleLod (
    const struct R_Microbit_SpirvTexture2D* texture,
    float                                   u,
    float                                   v,
    float                                   lod,
    enum R_Microbit_SpirvTextureFilter      filter,
    R_Microbit_SpirvVec4*                   color)
{
    (void)lod;
    if (!texture || !color) return MICROBIT_SPIRV_TEXTURE_ERROR_INVALID_ARGUMENT;
    u = R_SpirvTexelImageAddress (u, texture->addressU);
    v = R_SpirvTexelImageAddress (v, texture->addressV);
    if (filter == MICROBIT_SPIRV_TEXTURE_FILTER_NEAREST)
        return R_Microbit_SpirvTexture2DRead (
            texture,
            (uint32_t)floorf (u * (texture->width - 1u) + 0.5f),
            (uint32_t)floorf (v * (texture->height - 1u) + 0.5f),
            color);
    float    fx = u * (texture->width - 1u), fy = v * (texture->height - 1u);
    uint32_t x0 = (uint32_t)floorf (fx), y0 = (uint32_t)floorf (fy),
             x1 = x0 + 1u < texture->width ? x0 + 1u : x0, y1 = y0 + 1u < texture->height ? y0 + 1u : y0;
    R_Microbit_SpirvVec4 a, b, c, d;
    R_Microbit_SpirvTexture2DRead (texture, x0, y0, &a);
    R_Microbit_SpirvTexture2DRead (texture, x1, y0, &b);
    R_Microbit_SpirvTexture2DRead (texture, x0, y1, &c);
    R_Microbit_SpirvTexture2DRead (texture, x1, y1, &d);
    float tx = fx - x0, ty = fy - y0;
    for (int i = 0; i < 4; ++i)
    {
        float ab = (&a.x)[i] + ((&b.x)[i] - (&a.x)[i]) * tx;
        float cd = (&c.x)[i] + ((&d.x)[i] - (&c.x)[i]) * tx;
        (&color->x)[i] = ab + (cd - ab) * ty;
    }
    return 0;
}
int
R_Microbit_SpirvTexture2DSample (
    const struct R_Microbit_SpirvTexture2D* texture,
    float                                   u,
    float                                   v,
    enum R_Microbit_SpirvTextureFilter      filter,
    R_Microbit_SpirvVec4*                   color)
{
    return R_Microbit_SpirvTexture2DSampleLod (texture, u, v, 0.0f, filter, color);
}
int
R_Microbit_SpirvTexture3DSample (
    const struct R_Microbit_SpirvTexture3D* texture,
    float                                   u,
    float                                   v,
    float                                   w,
    R_Microbit_SpirvVec4*                   color)
{
    if (!texture || !color) return -1;
    u = R_SpirvTexelImageAddress (u, texture->addressU);
    v = R_SpirvTexelImageAddress (v, texture->addressV);
    w = R_SpirvTexelImageAddress (w, texture->addressW);
    return R_Microbit_SpirvTexture3DRead (
        texture,
        (uint32_t)floorf (u * (texture->width - 1u) + 0.5f),
        (uint32_t)floorf (v * (texture->height - 1u) + 0.5f),
        (uint32_t)floorf (w * (texture->depth - 1u) + 0.5f),
        color);
}

int
R_Microbit_SpirvTexture2DFill (struct R_Microbit_SpirvTexture2D* texture, R_Microbit_SpirvVec4 color)
{
    if (!texture) return -1;
    for (uint32_t y = 0; y < texture->height; ++y)
        for (uint32_t x = 0; x < texture->width; ++x)
            R_Microbit_SpirvTexture2DWrite (texture, x, y, color);
    return 0;
}

typedef struct
{
        struct R_Microbit_SpirvTexture2D*   texture;
        uint32_t                            x, y, width, height;
        R_Microbit_SpirvTextureTileFunction function;
        void*                               userData;
} R_SpirvTexelImageTile;
static void
R_SpirvTexelImageRunTile (void* data)
{
    R_SpirvTexelImageTile* tile = (R_SpirvTexelImageTile*)data;
    tile->function (tile->userData, tile->x, tile->y, tile->width, tile->height);
}
int
R_Microbit_SpirvTexture2DForEachTile (
    struct R_Microbit_SpirvTexture2D*   texture,
    uint32_t                            tileWidth,
    uint32_t                            tileHeight,
    R_Microbit_SpirvTextureTileFunction function,
    void*                               userData)
{
    if (!texture || !tileWidth || !tileHeight || !function) return -1;
    uint32_t               capacity = texture->pPool ? 4u : 0u;
    R_SpirvTexelImageTile* tasks
        = (R_SpirvTexelImageTile*)R_CSTL_HeapAlloc ((capacity ? capacity : 1u) * sizeof (*tasks));
    if (!tasks) return -2;
    uint32_t count = 0;
    for (uint32_t y = 0; y < texture->height; y += tileHeight)
        for (uint32_t x = 0; x < texture->width; x += tileWidth)
        {
            R_SpirvTexelImageTile tile
                = {texture,
                   x,
                   y,
                   x + tileWidth > texture->width ? texture->width - x : tileWidth,
                   y + tileHeight > texture->height ? texture->height - y : tileHeight,
                   function,
                   userData};
            if (texture->pPool)
            {
                tasks[count++] = tile;
                if (count == capacity)
                {
                    for (uint32_t i = 0; i < count; ++i)
                        R_Microbit_SpirvCpuThreadPoolSubmit (
                            texture->pPool,
                            R_SpirvTexelImageRunTile,
                            &tasks[i]);
                    R_Microbit_SpirvCpuThreadPoolWait (texture->pPool);
                    count = 0;
                }
            }
            else function (userData, tile.x, tile.y, tile.width, tile.height);
        }
    if (texture->pPool && count)
    {
        for (uint32_t i = 0; i < count; ++i)
            R_Microbit_SpirvCpuThreadPoolSubmit (texture->pPool, R_SpirvTexelImageRunTile, &tasks[i]);
        R_Microbit_SpirvCpuThreadPoolWait (texture->pPool);
    }
    R_CSTL_HeapFree (tasks);
    return 0;
}
