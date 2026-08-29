#pragma once

#include "rpack/rpack_encoder.h"
#include "rlgame.base/cstl/cstl_array.h"
#include "rlgame.base/cstl/cstl_string.h"

#include <stddef.h>
#include <stdint.h>

R_PACK_API uint32_t R_Pack_MipmapDimension (uint32_t source, uint32_t other, uint32_t limit);

R_PACK_API uint8_t*
R_Pack_ResizeImageBox (const struct R_Pack_InputImage* pSource, uint32_t width, uint32_t height);

R_PACK_API int R_Pack_EncodeAndWrite (struct R_Pack_Encoder* pEncoder, const char* pOutputPath);

R_PACK_API uint32_t R_Pack_EncodeInputImages (
    struct R_Pack_Encoder*     pEncoder,
    const struct R_CSTL_Array* pInputPaths,
    uint32_t                   mipmapSize);

R_PACK_API int R_Pack_HasExtension (const char* pPath, const char* pExtension);

R_PACK_API int R_Pack_MakeVariantPath (const char* pOutputPath, uint32_t size, char** ppVariantPath);

R_PACK_API int R_Pack_EncodeMipmapVariants (
    const struct R_Pack_EncoderConfig* pConfig,
    const struct R_CSTL_Array*         pInputPaths,
    const char*                        pOutputPath);

R_PACK_API uint32_t R_Pack_EncodeInputImagesThreaded (
    struct R_Pack_Encoder*     pEncoder,
    const struct R_CSTL_Array* pInputPaths,
    uint32_t                   mipmapSize,
    uint32_t                   workerCount);