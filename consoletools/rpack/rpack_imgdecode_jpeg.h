#pragma once

#include "rpack/rpack_platform.h"

#include <stddef.h>
#include <stdint.h>

struct r_pack_jpeg_image
{
        uint8_t* pPixels;
        uint32_t width;
        uint32_t height;
        uint32_t stride;
};

R_PACK_API enum r_pack_error
r_pack_jpeg_decode (const uint8_t* pData, size_t dataSize, struct r_pack_jpeg_image* pImage);

R_PACK_API enum r_pack_error r_pack_jpeg_decode_file (const char* pPath, struct r_pack_jpeg_image* pImage);

R_PACK_API void r_pack_jpeg_free_image (struct r_pack_jpeg_image* pImage);
