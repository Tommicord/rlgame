#pragma once

#include "rpack/rpack_encoder.h"
#include "rlgame.base/cstl/cstl_array.h"
#include "rlgame.base/cstl/cstl_string.h"

#include <stddef.h>
#include <stdint.h>

R_PACK_API uint32_t r_pack_mipmap_dimension (uint32_t source, uint32_t other, uint32_t limit);

R_PACK_API uint8_t*
r_pack_resize_image_box (const struct r_pack_input_image* pSource, uint32_t width, uint32_t height);

R_PACK_API int r_pack_encode_and_write (struct r_pack_encoder* pEncoder, const char* pOutputPath);

R_PACK_API uint32_t r_pack_encode_input_images (
    struct r_pack_encoder*     pEncoder,
    const struct R_CSTL_Array* pInputPaths,
    uint32_t                   mipmapSize);

R_PACK_API int r_pack_has_extension (const char* pPath, const char* pExtension);

R_PACK_API int r_pack_make_variant_path (const char* pOutputPath, uint32_t size, char** ppVariantPath);

R_PACK_API int r_pack_encode_mipmap_variants (
    const struct r_pack_encoder_settings* pSettings,
    const struct R_CSTL_Array*         pInputPaths,
    const char*                        pOutputPath);

R_PACK_API uint32_t r_pack_encode_input_images_threaded (
    struct r_pack_encoder*     pEncoder,
    const struct R_CSTL_Array* pInputPaths,
    uint32_t                   mipmapSize,
    uint32_t                   workerCount);