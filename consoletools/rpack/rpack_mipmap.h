#pragma once

#include <stddef.h>
#include <stdint.h>

#include "rpack_platform.h"

enum R_Pack_MipmapFilter
{
    R_PACK_MIPMAP_FILTER_BOX = 0,
    R_PACK_MIPMAP_FILTER_GAUSSIAN = 1
};

enum R_Pack_MipmapError
{
    R_PACK_MIPMAP_OK = 0,
    R_PACK_MIPMAP_ERROR_INVALID_ARGUMENT = -1,
    R_PACK_MIPMAP_ERROR_INITIALIZATION = -2,
    R_PACK_MIPMAP_ERROR_DISPATCH = -3,
    R_PACK_MIPMAP_ERROR_UNSUPPORTED = -4
};

struct R_Pack_MipmapContext;

#if defined(R_DEVMODE)
#include <assert.h>
#define R_PACK_MIPMAP_VALIDATE(condition, error)                                                             \
    do                                                                                                       \
    {                                                                                                        \
        assert (condition);                                                                                  \
        if (!(condition)) return (error);                                                                    \
    } while (0)
#else
#define R_PACK_MIPMAP_VALIDATE(condition, error) ((void)0)
#endif

R_PACK_API int R_Pack_MipmapInitialize (
    void*                         pContext,
    void*                         pDevice,
    void*                         pQueue,
    const char*                   pKernelSource,
    size_t                        kernelSourceSize,
    struct R_Pack_MipmapContext** ppOutContext);

R_PACK_API void R_Pack_MipmapShutdown (struct R_Pack_MipmapContext* pContext);

R_PACK_API int R_Pack_MipmapDispatch (
    struct R_Pack_MipmapContext* pContext,
    void*                        pSource,
    void*                        pDestination,
    uint32_t                     sourceWidth,
    uint32_t                     sourceHeight,
    uint32_t                     destinationWidth,
    uint32_t                     destinationHeight,
    enum R_Pack_MipmapFilter     filter,
    float                        sigma);
