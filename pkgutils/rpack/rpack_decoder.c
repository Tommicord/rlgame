#include "rpack/rpack_decoder.h"

#include "rlgame.base/cstl/cstl_heap_allocator.h"
#include "rlgame.base/cstl/cstl_log.h"
#include <string.h>

struct R_PackDecoder*
R_Pack_NewDecoder (const uint8_t* pData, uint64_t dataSize)
{
    if (!pData || dataSize < R_RPACK_HEADER_SIZE)
    {
        return NULL;
    }
    if (!R_Pack_DecoderValidateFile (pData, dataSize))
    {
        return NULL;
    }

    struct R_PackDecoder* pDecoder = (struct R_PackDecoder*)R_CSTL_HeapAlloc (sizeof (struct R_PackDecoder));
    if (!pDecoder)
    {
        return NULL;
    }

    memset (pDecoder, 0, sizeof (struct R_PackDecoder));
    pDecoder->pData = pData;
    pDecoder->dataSize = dataSize;

    pDecoder->pHeader = (struct R_PackHeader*)pData;

    if (pDecoder->pHeader->textureCount > 0)
    {
        pDecoder->pHashTable = (struct R_PackHashEntry*)(pData + pDecoder->pHeader->hashTableOffset);
    }

    if (pDecoder->pHeader->colorTableSize > 0)
    {
        pDecoder->pColorTable = (struct R_PackColorEntry*)(pData + pDecoder->pHeader->colorTableOffset);
    }

    if (pDecoder->pHeader->pixelIndexTableSize > 0)
    {
        pDecoder->pPixelIndexTable
            = (struct R_PackPixelIndexEntry*)(pData + pDecoder->pHeader->pixelIndexTableOffset);
    }

    return pDecoder;
}

void
R_Pack_DeleteDecoder (struct R_PackDecoder* pDecoder)
{
    if (!pDecoder)
    {
        return;
    }

    R_CSTL_HeapFree (pDecoder);
}

int
R_Pack_DecoderValidateFile (const uint8_t* pData, uint64_t dataSize)
{
    if (!pData || dataSize < R_RPACK_HEADER_SIZE)
    {
        return 0;
    }

    const struct R_PackHeader* pHeader = (const struct R_PackHeader*)pData;
    return R_Pack_ValidateHeader (pHeader);
}

const struct R_PackHashEntry*
R_Pack_DecoderFindTexture (const struct R_PackDecoder* pDecoder, const char* pName)
{
    if (!pDecoder || !pDecoder->pHeader || !pName)
    {
        return NULL;
    }

    uint64_t nameHash = R_Pack_Hash64String (pName, 0);

    for (uint32_t i = 0; i < pDecoder->pHeader->textureCount; ++i)
    {
        if (pDecoder->pHashTable[i].nameHash == nameHash)
        {
            return &pDecoder->pHashTable[i];
        }
    }

    return NULL;
}

uint32_t
R_Pack_DecoderGetTextureCount (const struct R_PackDecoder* pDecoder)
{
    if (!pDecoder || !pDecoder->pHeader)
    {
        return 0;
    }
    return pDecoder->pHeader->textureCount;
}

enum R_PackError
R_Pack_DecoderGetTextureDimensions (
    const struct R_PackDecoder* pDecoder,
    const char*                 pName,
    uint32_t*                   pWidth,
    uint32_t*                   pHeight)
{
    if (!pDecoder || !pName || !pWidth || !pHeight)
    {
        return R_RPACK_ERROR_INVALID_ARGUMENT;
    }

    const struct R_PackHashEntry* pEntry = R_Pack_DecoderFindTexture (pDecoder, pName);
    if (!pEntry)
    {
        return R_RPACK_ERROR_TEXTURE_NOT_FOUND;
    }

    *pWidth = pEntry->width;
    *pHeight = pEntry->height;

    return R_RPACK_OK;
}

uint64_t
R_Pack_DecoderGetTextureSize (const struct R_PackDecoder* pDecoder, const char* pName)
{
    uint32_t width = 0, height = 0;
    if (R_Pack_DecoderGetTextureDimensions (pDecoder, pName, &width, &height) != R_RPACK_OK)
    {
        return 0;
    }
    return (uint64_t)width * height * 4;
}

uint64_t
R_Pack_DecoderGetTexturesSize (const struct R_PackDecoder* pDecoder, const char** pNames, uint32_t nameCount)
{
    if (!pDecoder || !pNames || nameCount == 0)
    {
        return 0;
    }

    uint64_t totalSize = 0;
    for (uint32_t i = 0; i < nameCount; ++i)
    {
        totalSize += R_Pack_DecoderGetTextureSize (pDecoder, pNames[i]);
    }
    return totalSize;
}

enum R_PackError
R_Pack_DecoderDecodeTexture (
    const struct R_PackDecoder* pDecoder,
    const char*                 pName,
    uint8_t*                    pOutputBuffer,
    uint64_t                    outputBufferSize,
    uint64_t*                   pBytesWritten)
{
    if (!pDecoder || !pName || !pOutputBuffer)
    {
        return R_RPACK_ERROR_INVALID_ARGUMENT;
    }

    const struct R_PackHashEntry* pEntry = R_Pack_DecoderFindTexture (pDecoder, pName);
    if (!pEntry)
    {
        return R_RPACK_ERROR_TEXTURE_NOT_FOUND;
    }

    uint64_t requiredSize = (uint64_t)pEntry->width * pEntry->height * 4;
    if (outputBufferSize < requiredSize)
    {
        return R_RPACK_ERROR_BUFFER_TOO_SMALL;
    }

    uint32_t pixelCount = pEntry->width * pEntry->height;
    uint32_t startIndex = pEntry->pixelIndexTableOffset;

    for (uint32_t i = 0; i < pixelCount; ++i)
    {
        if (startIndex + i >= pDecoder->pHeader->pixelIndexTableSize)
        {
            return R_RPACK_ERROR_INVALID_DATA;
        }

        const struct R_PackPixelIndexEntry* pPixelEntry = &pDecoder->pPixelIndexTable[startIndex + i];
        if (pPixelEntry->colorIndex >= pDecoder->pHeader->colorTableSize)
        {
            return R_RPACK_ERROR_INVALID_DATA;
        }

        const struct R_PackColorEntry* pColorEntry = &pDecoder->pColorTable[pPixelEntry->colorIndex];

        uint8_t r, g, b;
        R_Pack_YUVToRGBA (
            pColorEntry->luminance,
            pColorEntry->luminanceExp,
            pColorEntry->chrominanceU,
            pColorEntry->chrominanceV,
            &r,
            &g,
            &b);

        uint32_t outputIndex = i * 4;
        pOutputBuffer[outputIndex] = r;
        pOutputBuffer[outputIndex + 1] = g;
        pOutputBuffer[outputIndex + 2] = b;
        pOutputBuffer[outputIndex + 3] = 255;
    }

    if (pBytesWritten)
    {
        *pBytesWritten = requiredSize;
    }

    return R_RPACK_OK;
}

enum R_PackError
R_Pack_DecoderDecodeTextures (
    const struct R_PackDecoder* pDecoder,
    const char**                pNames,
    uint32_t                    nameCount,
    uint8_t*                    pOutputBuffer,
    uint64_t                    outputBufferSize,
    uint64_t*                   pBytesWritten)
{
    if (!pDecoder || !pNames || nameCount == 0 || !pOutputBuffer)
    {
        return R_RPACK_ERROR_INVALID_ARGUMENT;
    }

    uint64_t offset = 0;
    for (uint32_t i = 0; i < nameCount; ++i)
    {
        uint64_t textureSize = R_Pack_DecoderGetTextureSize (pDecoder, pNames[i]);
        if (textureSize == 0)
        {
            return R_RPACK_ERROR_TEXTURE_NOT_FOUND;
        }

        if (offset + textureSize > outputBufferSize)
        {
            return R_RPACK_ERROR_BUFFER_TOO_SMALL;
        }

        uint64_t         written = 0;
        enum R_PackError err = R_Pack_DecoderDecodeTexture (
            pDecoder,
            pNames[i],
            pOutputBuffer + offset,
            textureSize,
            &written);
        if (err != R_RPACK_OK)
        {
            return err;
        }

        offset += written;
    }

    if (pBytesWritten)
    {
        *pBytesWritten = offset;
    }

    return R_RPACK_OK;
}
