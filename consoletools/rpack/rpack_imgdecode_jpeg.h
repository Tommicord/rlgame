#pragma once

#include "rpack/rpack_platform.h"

#include <stddef.h>
#include <stdint.h>

struct R_PackJpegImage
{
        uint8_t* pPixels;
        uint32_t width;
        uint32_t height;
        uint32_t stride;
};

R_RPACK_API enum R_PackError
R_Pack_JpegDecode (const uint8_t* pData, size_t dataSize, struct R_PackJpegImage* pImage);

R_RPACK_API enum R_PackError R_Pack_JpegDecodeFile (const char* pPath, struct R_PackJpegImage* pImage);

R_RPACK_API void R_Pack_JpegFreeImage (struct R_PackJpegImage* pImage);
