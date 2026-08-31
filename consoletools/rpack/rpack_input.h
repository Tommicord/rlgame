#pragma once

#include "rpack/rpack_encoder.h"

#include <stddef.h>
#include <stdint.h>

enum r_pack_input_encoding
{
    R_PACK_INPUT_RAW_RGBA = 0,
    R_PACK_INPUT_JPEG = 1,
    R_PACK_INPUT_JPEG_BASE64 = 2
};

struct r_pack_owned_image
{
        struct r_pack_input_image image;
        uint8_t*                  pPixels;
};

R_PACK_API enum r_pack_error r_pack_input_from_rawRGBA (
    const uint8_t*             pPixels,
    size_t                     pixelBytes,
    uint32_t                   width,
    uint32_t                   height,
    uint32_t                   stride,
    const char*                pName,
    struct r_pack_owned_image* pOutput);

R_PACK_API enum r_pack_error r_pack_input_from_bytes (
    const uint8_t*             pData,
    size_t                     dataSize,
    const char*                pName,
    struct r_pack_owned_image* pOutput);

R_PACK_API enum r_pack_error r_pack_input_from_base64 (
    const char*                pBase64,
    size_t                     textLength,
    const char*                pName,
    struct r_pack_owned_image* pOutput);

R_PACK_API void r_pack_delete_owned_image (struct r_pack_owned_image* pImage);