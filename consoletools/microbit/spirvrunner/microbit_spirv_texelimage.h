#pragma once

#include <stddef.h>
#include <stdint.h>

#include "microbit/spirvrunner/microbit_spirv_threadexecutor.h"
#include "microbit/spirvrunner/microbit_spirv_vecx.h"

enum R_Microbit_SpirvTextureFormat
{
    MICROBIT_SPIRV_TEXTURE_R8_UNORM = 1,
    MICROBIT_SPIRV_TEXTURE_RG8_UNORM = 2,
    MICROBIT_SPIRV_TEXTURE_RGB8_UNORM = 3,
    MICROBIT_SPIRV_TEXTURE_RGBA8_UNORM = 4,
    MICROBIT_SPIRV_TEXTURE_BGRA8_UNORM = 5,
    MICROBIT_SPIRV_TEXTURE_R16_FLOAT = 6,
    MICROBIT_SPIRV_TEXTURE_RG16_FLOAT = 7,
    MICROBIT_SPIRV_TEXTURE_RGB16_FLOAT = 8,
    MICROBIT_SPIRV_TEXTURE_RGBA16_FLOAT = 9,
    MICROBIT_SPIRV_TEXTURE_R32_FLOAT = 10,
    MICROBIT_SPIRV_TEXTURE_RG32_FLOAT = 11,
    MICROBIT_SPIRV_TEXTURE_RGB32_FLOAT = 12,
    MICROBIT_SPIRV_TEXTURE_RGBA32_FLOAT = 13
};

enum R_Microbit_SpirvTextureFilter
{
    MICROBIT_SPIRV_TEXTURE_FILTER_NEAREST = 0,
    MICROBIT_SPIRV_TEXTURE_FILTER_LINEAR = 1
};

enum R_Microbit_SpirvTextureAddressMode
{
    MICROBIT_SPIRV_TEXTURE_ADDRESS_CLAMP = 0,
    MICROBIT_SPIRV_TEXTURE_ADDRESS_REPEAT = 1,
    MICROBIT_SPIRV_TEXTURE_ADDRESS_MIRRORED_REPEAT = 2
};

enum R_Microbit_SpirvTextureError
{
    MICROBIT_SPIRV_TEXTURE_OK = 0,
    MICROBIT_SPIRV_TEXTURE_ERROR_INVALID_ARGUMENT = -1,
    MICROBIT_SPIRV_TEXTURE_ERROR_OUT_OF_MEMORY = -2,
    MICROBIT_SPIRV_TEXTURE_ERROR_UNSUPPORTED_FORMAT = -3,
    MICROBIT_SPIRV_TEXTURE_ERROR_BOUNDS = -4,
    MICROBIT_SPIRV_TEXTURE_ERROR_STATE = -5
};

struct R_Microbit_SpirvTexture2D
{
        uint32_t                                width;
        uint32_t                                height;
        uint32_t                                mipLevels;
        enum R_Microbit_SpirvTextureFormat      format;
        uint8_t*                                pPixels;
        size_t                                  pixelStride;
        size_t                                  levelStride;
        struct R_Microbit_SpirvThreadExecutor* pPool;
        enum R_Microbit_SpirvTextureAddressMode addressU;
        enum R_Microbit_SpirvTextureAddressMode addressV;
};

struct R_Microbit_SpirvTexture3D
{
        uint32_t                                width;
        uint32_t                                height;
        uint32_t                                depth;
        enum R_Microbit_SpirvTextureFormat      format;
        uint8_t*                                pPixels;
        size_t                                  pixelStride;
        size_t                                  levelStride;
        struct R_Microbit_SpirvThreadExecutor* pPool;
        enum R_Microbit_SpirvTextureAddressMode addressU;
        enum R_Microbit_SpirvTextureAddressMode addressV;
        enum R_Microbit_SpirvTextureAddressMode addressW;
};

int R_Microbit_SpirvTexture2DCreate (
    struct R_Microbit_SpirvTexture2D*     pTexture,
    uint32_t                              width,
    uint32_t                              height,
    enum R_Microbit_SpirvTextureFormat    format,
    struct R_Microbit_SpirvThreadExecutor* pool);
void R_Microbit_SpirvTexture2DDelete (struct R_Microbit_SpirvTexture2D* pTexture);
int  R_Microbit_SpirvTexture3DCreate (
     struct R_Microbit_SpirvTexture3D*     pTexture,
     uint32_t                              width,
     uint32_t                              height,
     uint32_t                              depth,
     enum R_Microbit_SpirvTextureFormat    format,
    struct R_Microbit_SpirvThreadExecutor* pPool);
void R_Microbit_SpirvTexture3DDelete (struct R_Microbit_SpirvTexture3D* texture);
int  R_Microbit_SpirvTexture2DRead (
     const struct R_Microbit_SpirvTexture2D* pTexture,
     uint32_t                                x,
     uint32_t                                y,
     R_Microbit_SpirvVec4*                   pVec);
int R_Microbit_SpirvTexture2DWrite (
    struct R_Microbit_SpirvTexture2D* pTexture,
    uint32_t                          x,
    uint32_t                          y,
    R_Microbit_SpirvVec4              pVec);
int R_Microbit_SpirvTexture3DRead (
    const struct R_Microbit_SpirvTexture3D* pTexture,
    uint32_t                                x,
    uint32_t                                y,
    uint32_t                                z,
    R_Microbit_SpirvVec4*                   pVec);
int R_Microbit_SpirvTexture3DWrite (
    struct R_Microbit_SpirvTexture3D* pTexture,
    uint32_t                          x,
    uint32_t                          y,
    uint32_t                          z,
    R_Microbit_SpirvVec4              pVec);
int R_Microbit_SpirvTexture2DSample (
    const struct R_Microbit_SpirvTexture2D* pTexture,
    float                                   u,
    float                                   v,
    enum R_Microbit_SpirvTextureFilter      filter,
    R_Microbit_SpirvVec4*                   pVec);
int R_Microbit_SpirvTexture2DSampleLod (
    const struct R_Microbit_SpirvTexture2D* pTexture,
    float                                   u,
    float                                   v,
    float                                   lod,
    enum R_Microbit_SpirvTextureFilter      filter,
    R_Microbit_SpirvVec4*                   pVec);
int R_Microbit_SpirvTexture3DSample (
    const struct R_Microbit_SpirvTexture3D* pTexture,
    float                                   u,
    float                                   v,
    float                                   w,
    R_Microbit_SpirvVec4*                   pVec);
int R_Microbit_SpirvTexture2DFill (struct R_Microbit_SpirvTexture2D* pTexture, R_Microbit_SpirvVec4 vec);

typedef void (*R_Microbit_SpirvTextureTileCallback) (
    void*    userData,
    uint32_t x,
    uint32_t y,
    uint32_t width,
    uint32_t height);
int R_Microbit_SpirvTexture2DForEachTile (
    struct R_Microbit_SpirvTexture2D*   pTexture,
    uint32_t                            tileWidth,
    uint32_t                            tileHeight,
    R_Microbit_SpirvTextureTileCallback function,
    void*                               pUserData);
