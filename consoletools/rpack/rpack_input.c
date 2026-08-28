#include "rpack_input.h"

#include "rpack_imgdecode_jpeg.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"

#include <string.h>

static enum R_Pack_Error
R_Pack_InputCheckOutput (struct R_Pack_OwnedImage* pOutput)
{
    if (!pOutput) return R_RPACK_ERROR_INVALID_ARGUMENT;
    memset (pOutput, 0, sizeof (*pOutput));
    return R_RPACK_OK;
}

enum R_Pack_Error
R_Pack_InputFromRawRGBA (const uint8_t* pPixels, size_t pixelBytes, uint32_t width, uint32_t height,
                         uint32_t stride, const char* pName, struct R_Pack_OwnedImage* pOutput)
{
    enum R_Pack_Error error = R_Pack_InputCheckOutput (pOutput);
    if (error != R_RPACK_OK || !pPixels || !pName || !width || !height ||
        (uint64_t)stride < (uint64_t)width * 4 ||
        (size_t)stride > SIZE_MAX / height || pixelBytes < (size_t)stride * height)
        return R_RPACK_ERROR_INVALID_ARGUMENT;

    size_t size = (size_t)stride * height;
    pOutput->pPixels = (uint8_t*)R_CSTL_HeapAlloc (size);
    if (!pOutput->pPixels) return R_RPACK_ERROR_OUT_OF_MEMORY;
    memcpy (pOutput->pPixels, pPixels, size);
    pOutput->image.pPixels = pOutput->pPixels;
    pOutput->image.width = width;
    pOutput->image.height = height;
    pOutput->image.stride = stride;
    pOutput->image.pName = pName;
    return R_RPACK_OK;
}

enum R_Pack_Error
R_Pack_InputFromBytes (const uint8_t* pData, size_t dataSize, const char* pName, struct R_Pack_OwnedImage* pOutput)
{
    enum R_Pack_Error error = R_Pack_InputCheckOutput (pOutput);
    if (error != R_RPACK_OK || !pData || !dataSize || !pName) return R_RPACK_ERROR_INVALID_ARGUMENT;

    struct R_Pack_JpegImage decoded = {0};
    error = R_Pack_JpegDecode (pData, dataSize, &decoded);
    if (error != R_RPACK_OK) return error;
    pOutput->pPixels = decoded.pPixels;
    pOutput->image.pPixels = decoded.pPixels;
    pOutput->image.width = decoded.width;
    pOutput->image.height = decoded.height;
    pOutput->image.stride = decoded.stride;
    pOutput->image.pName = pName;
    return R_RPACK_OK;
}

static int
R_Pack_Base64Value (char character)
{
    if (character >= 'A' && character <= 'Z') return character - 'A';
    if (character >= 'a' && character <= 'z') return character - 'a' + 26;
    if (character >= '0' && character <= '9') return character - '0' + 52;
    if (character == '+') return 62;
    if (character == '/') return 63;
    return -1;
}

enum R_Pack_Error
R_Pack_InputFromBase64 (const char* pBase64, size_t textLength, const char* pName,
                        struct R_Pack_OwnedImage* pOutput)
{
    enum R_Pack_Error error = R_Pack_InputCheckOutput (pOutput);
    if (error != R_RPACK_OK || !pBase64 || !textLength || !pName || textLength % 4 != 0)
        return R_RPACK_ERROR_INVALID_ARGUMENT;

    for (size_t i = 0; i < textLength; ++i)
    {
        if (pBase64[i] == '=' || R_Pack_Base64Value (pBase64[i]) >= 0) continue;
        return R_RPACK_ERROR_INVALID_FORMAT;
    }

    size_t decodedCapacity = textLength / 4 * 3;
    uint8_t* pDecoded = (uint8_t*)R_CSTL_HeapAlloc (decodedCapacity);
    if (!pDecoded) return R_RPACK_ERROR_OUT_OF_MEMORY;
    size_t decodedSize = 0;
    for (size_t i = 0; i < textLength; i += 4)
    {
        int values[4] = {R_Pack_Base64Value (pBase64[i]), R_Pack_Base64Value (pBase64[i + 1]),
                            pBase64[i + 2] == '=' ? 0 : R_Pack_Base64Value (pBase64[i + 2]),
                            pBase64[i + 3] == '=' ? 0 : R_Pack_Base64Value (pBase64[i + 3])};
        int padding = (pBase64[i + 3] == '=') + (pBase64[i + 2] == '=');
        if (values[0] < 0 || values[1] < 0 || values[2] < 0 || values[3] < 0 ||
            (padding && i + 4 != textLength) || (pBase64[i + 2] == '=' && pBase64[i + 3] != '='))
        {
            R_CSTL_HeapFree (pDecoded);
            return R_RPACK_ERROR_INVALID_FORMAT;
        }
        pDecoded[decodedSize++] = (uint8_t)((values[0] << 2) | (values[1] >> 4));
        if (!padding) pDecoded[decodedSize++] = (uint8_t)((values[1] << 4) | (values[2] >> 2));
        if (padding < 2) pDecoded[decodedSize++] = (uint8_t)((values[2] << 6) | values[3]);
    }
    error = R_Pack_InputFromBytes (pDecoded, decodedSize, pName, pOutput);
    R_CSTL_HeapFree (pDecoded);
    return error;
}

void
R_Pack_DeleteOwnedImage (struct R_Pack_OwnedImage* pImage)
{
    if (!pImage) return;
    if (pImage->pPixels) R_CSTL_HeapFree (pImage->pPixels);
    memset (pImage, 0, sizeof (*pImage));
}