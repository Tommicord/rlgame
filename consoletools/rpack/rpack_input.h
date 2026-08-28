#pragma once

#include "rpack/rpack_encoder.h"

#include <stddef.h>
#include <stdint.h>

enum R_PackInputEncoding
{
    R_RPACK_INPUT_RAW_RGBA = 0,
    R_RPACK_INPUT_JPEG = 1,
    R_RPACK_INPUT_JPEG_BASE64 = 2
};

struct R_PackOwnedImage
{
    struct R_PackInputImage image;
    uint8_t* pPixels;
};

R_RPACK_API enum R_PackError R_Pack_InputFromRawRGBA (
    const uint8_t* pPixels,
    size_t pixelBytes,
    uint32_t width,
    uint32_t height,
    uint32_t stride,
    const char* pName,
    struct R_PackOwnedImage* pOutput);

R_RPACK_API enum R_PackError R_Pack_InputFromBytes (
    const uint8_t* pData,
    size_t dataSize,
    const char* pName,
    struct R_PackOwnedImage* pOutput);

R_RPACK_API enum R_PackError R_Pack_InputFromBase64 (
    const char* pBase64,
    size_t textLength,
    const char* pName,
    struct R_PackOwnedImage* pOutput);

R_RPACK_API void R_Pack_DeleteOwnedImage (struct R_PackOwnedImage* pImage);