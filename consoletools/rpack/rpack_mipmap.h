#pragma once

#include <stddef.h>
#include <stdint.h>

#include "rpack/rpack_platform.h"

enum r_pack_mipmap_filter
{
    R_PACK_MIPMAP_FILTER_BOX = 0,
    R_PACK_MIPMAP_FILTER_GAUSSIAN = 1
};

enum r_pack_mipmap_error
{
    R_PACK_MIPMAP_OK = 0,
    R_PACK_MIPMAP_ERROR_INVALID_ARGUMENT = -1,
    R_PACK_MIPMAP_ERROR_INITIALIZATION = -2,
    R_PACK_MIPMAP_ERROR_DISPATCH = -3,
    R_PACK_MIPMAP_ERROR_UNSUPPORTED = -4,
    R_PACK_MIPMAP_ERROR_UNKNOWN = -99,
};
struct r_pack_mipmap_context;

R_PACK_API int r_pack_mipmap_initialize (
    void*                         pContext,
    void*                         pDevice,
    void*                         pQueue,
    const char*                   pKernelSource,
    size_t                        kernelSourceSize,
    struct r_pack_mipmap_context** ppOutContext);

R_PACK_API void r_pack_mipmap_shutdown (struct r_pack_mipmap_context* pContext);

R_PACK_API int r_pack_mipmap_dispatch (
    struct r_pack_mipmap_context* pContext,
    void*                        pSource,
    void*                        pDestination,
    uint32_t                     sourceWidth,
    uint32_t                     sourceHeight,
    uint32_t                     destinationWidth,
    uint32_t                     destinationHeight,
    enum r_pack_mipmap_filter     filter,
    float                        sigma);
