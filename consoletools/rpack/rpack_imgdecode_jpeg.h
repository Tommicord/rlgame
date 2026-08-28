#pragma once

#include "rpack/rpack_platform.h"

#include <stddef.h>
#include <stdint.h>

struct R_Pack_JpegImage
{
        uint8_t* pPixels;
        uint32_t width;
        uint32_t height;
        uint32_t stride;
};

R_PACK_API enum R_Pack_Error
R_Pack_JpegDecode (const uint8_t* pData, size_t dataSize, struct R_Pack_JpegImage* pImage);

R_PACK_API enum R_Pack_Error R_Pack_JpegDecodeFile (const char* pPath, struct R_Pack_JpegImage* pImage);

R_PACK_API void R_Pack_JpegFreeImage (struct R_Pack_JpegImage* pImage);
