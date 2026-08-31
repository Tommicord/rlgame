#include "rpack_input.h"

#include "rpack_imgdecode_jpeg.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"

#include <string.h>

static enum r_pack_error
r_pack_input_check_output (struct r_pack_owned_image* pOutput)
{
    if (!pOutput) return R_PACK_ERROR_INVALID_ARGUMENT;
    memset (pOutput, 0, sizeof (*pOutput));
    return R_PACK_OK;
}

enum r_pack_error
r_pack_input_from_rawRGBA (
    const uint8_t*             pPixels,
    size_t                     pixelBytes,
    uint32_t                   width,
    uint32_t                   height,
    uint32_t                   stride,
    const char*                pName,
    struct r_pack_owned_image* pOutput)
{
    enum r_pack_error error = r_pack_input_check_output (pOutput);
    if (error != R_PACK_OK || !pPixels || !pName || !width || !height
        || (uint64_t)stride < (uint64_t)width * 4 || (size_t)stride > SIZE_MAX / height
        || pixelBytes < (size_t)stride * height)
        return R_PACK_ERROR_INVALID_ARGUMENT;

    size_t size = (size_t)stride * height;
    pOutput->pPixels = (uint8_t*)r_cstl_heap_alloc (size);
    if (!pOutput->pPixels) return R_PACK_ERROR_OUT_OF_MEMORY;
    memcpy (pOutput->pPixels, pPixels, size);
    pOutput->image.pPixels = pOutput->pPixels;
    pOutput->image.width = width;
    pOutput->image.height = height;
    pOutput->image.stride = stride;
    pOutput->image.pName = pName;
    return R_PACK_OK;
}

enum r_pack_error
r_pack_input_from_bytes (
    const uint8_t*             pData,
    size_t                     dataSize,
    const char*                pName,
    struct r_pack_owned_image* pOutput)
{
    enum r_pack_error error = r_pack_input_check_output (pOutput);
    if (error != R_PACK_OK || !pData || !dataSize || !pName) return R_PACK_ERROR_INVALID_ARGUMENT;

    struct r_pack_jpeg_image decoded = {0};
    error = r_pack_jpeg_decode (pData, dataSize, &decoded);
    if (error != R_PACK_OK) return error;
    pOutput->pPixels = decoded.pPixels;
    pOutput->image.pPixels = decoded.pPixels;
    pOutput->image.width = decoded.width;
    pOutput->image.height = decoded.height;
    pOutput->image.stride = decoded.stride;
    pOutput->image.pName = pName;
    return R_PACK_OK;
}

static int
r_pack_base64_value (char character)
{
    if (character >= 'A' && character <= 'Z') return character - 'A';
    if (character >= 'a' && character <= 'z') return character - 'a' + 26;
    if (character >= '0' && character <= '9') return character - '0' + 52;
    if (character == '+') return 62;
    if (character == '/') return 63;
    return -1;
}

enum r_pack_error
r_pack_input_from_base64 (
    const char*                pBase64,
    size_t                     textLength,
    const char*                pName,
    struct r_pack_owned_image* pOutput)
{
    enum r_pack_error error = r_pack_input_check_output (pOutput);
    if (error != R_PACK_OK || !pBase64 || !textLength || !pName || textLength % 4 != 0)
        return R_PACK_ERROR_INVALID_ARGUMENT;

    for (size_t i = 0; i < textLength; ++i)
    {
        if (pBase64[i] == '=' || r_pack_base64_value (pBase64[i]) >= 0) continue;
        return R_PACK_ERROR_INVALID_FORMAT;
    }

    size_t   decodedCapacity = textLength / 4 * 3;
    uint8_t* pDecoded = (uint8_t*)r_cstl_heap_alloc (decodedCapacity);
    if (!pDecoded) return R_PACK_ERROR_OUT_OF_MEMORY;
    size_t decodedSize = 0;
    for (size_t i = 0; i < textLength; i += 4)
    {
        int values[4]
            = {r_pack_base64_value (pBase64[i]),
               r_pack_base64_value (pBase64[i + 1]),
               pBase64[i + 2] == '=' ? 0 : r_pack_base64_value (pBase64[i + 2]),
               pBase64[i + 3] == '=' ? 0 : r_pack_base64_value (pBase64[i + 3])};
        int padding = (pBase64[i + 3] == '=') + (pBase64[i + 2] == '=');
        if (values[0] < 0 || values[1] < 0 || values[2] < 0 || values[3] < 0
            || (padding && i + 4 != textLength) || (pBase64[i + 2] == '=' && pBase64[i + 3] != '='))
        {
            r_cstl_heap_free (pDecoded);
            return R_PACK_ERROR_INVALID_FORMAT;
        }
        pDecoded[decodedSize++] = (uint8_t)((values[0] << 2) | (values[1] >> 4));
        if (!padding) pDecoded[decodedSize++] = (uint8_t)((values[1] << 4) | (values[2] >> 2));
        if (padding < 2) pDecoded[decodedSize++] = (uint8_t)((values[2] << 6) | values[3]);
    }
    error = r_pack_input_from_bytes (pDecoded, decodedSize, pName, pOutput);
    r_cstl_heap_free (pDecoded);
    return error;
}

void
r_pack_delete_owned_image (struct r_pack_owned_image* pImage)
{
    if (!pImage) return;
    if (pImage->pPixels) r_cstl_heap_free (pImage->pPixels);
    memset (pImage, 0, sizeof (*pImage));
}