#include "rpack_val.h"

#include "rpack_format.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int
R_Pack_ValidationFail (struct R_PackValidationReport* pReport, enum R_PackError error, uint64_t offset,
                       uint32_t textureIndex, uint64_t pixelIndex)
{
    if (pReport)
    {
        pReport->error = error;
        pReport->offset = offset;
        pReport->textureIndex = textureIndex;
        pReport->pixelIndex = pixelIndex;
    }
    return 0;
}

static int
R_Pack_RangesOverlap (uint64_t leftStart, uint64_t leftSize, uint64_t rightStart, uint64_t rightSize)
{
    return leftStart < rightStart + rightSize && rightStart < leftStart + leftSize;
}

int
R_Pack_ValidatePackedData (const uint8_t* pData, uint64_t dataSize, struct R_PackValidationReport* pReport)
{
    if (pReport)
    {
        memset (pReport, 0, sizeof (*pReport));
        pReport->error = R_RPACK_OK;
    }
    if (!pData || dataSize < sizeof (struct R_PackHeader))
    {
        return R_Pack_ValidationFail (pReport, R_RPACK_ERROR_INVALID_ARGUMENT, 0, 0, 0);
    }

    const struct R_PackHeader* pHeader = (const struct R_PackHeader*)pData;
    if (!R_Pack_ValidateHeader (pHeader))
    {
        return R_Pack_ValidationFail (pReport, R_RPACK_ERROR_INVALID_FORMAT, 0, 0, 0);
    }

    uint64_t expectedSize = R_Pack_GetExpectedFileSize (pHeader);
    if (expectedSize == 0 || expectedSize != dataSize)
    {
        return R_Pack_ValidationFail (pReport, R_RPACK_ERROR_INVALID_FORMAT, dataSize, 0, 0);
    }

    uint64_t atlasSize = (uint64_t)pHeader->atlasWidth * pHeader->atlasHeight * 2;
    uint64_t fileEnd = dataSize;
    const struct R_PackHashEntry* pHashes = (const struct R_PackHashEntry*)(pData + pHeader->hashTableOffset);
    const struct R_PackColorEntry* pColors = (const struct R_PackColorEntry*)(pData + pHeader->colorTableOffset);
    const struct R_PackPixelIndexEntry* pPixels
        = (const struct R_PackPixelIndexEntry*)(pData + pHeader->pixelIndexTableOffset);

    if (pHeader->dataOffset > fileEnd || atlasSize > fileEnd - pHeader->dataOffset)
    {
        return R_Pack_ValidationFail (pReport, R_RPACK_ERROR_INVALID_DATA, pHeader->dataOffset, 0, 0);
    }

    for (uint32_t i = 0; i < pHeader->textureCount; ++i)
    {
        const struct R_PackHashEntry* pEntry = &pHashes[i];
        uint64_t texturePixels = (uint64_t)pEntry->width * pEntry->height;
        if (pEntry->width == 0 || pEntry->height == 0 ||
            pEntry->atlasOffsetX > pHeader->atlasWidth || pEntry->width > pHeader->atlasWidth - pEntry->atlasOffsetX ||
            pEntry->atlasOffsetY > pHeader->atlasHeight || pEntry->height > pHeader->atlasHeight - pEntry->atlasOffsetY ||
            pEntry->colorTableIndex >= pHeader->colorTableSize ||
            pEntry->pixelIndexTableOffset > pHeader->pixelIndexTableSize ||
            texturePixels > pHeader->pixelIndexTableSize - pEntry->pixelIndexTableOffset)
        {
            return R_Pack_ValidationFail (pReport, R_RPACK_ERROR_INVALID_DATA, pHeader->hashTableOffset + (uint64_t)i * sizeof (*pEntry), i, 0);
        }

        for (uint32_t j = i + 1; j < pHeader->textureCount; ++j)
        {
            if (pEntry->nameHash == pHashes[j].nameHash)
            {
                return R_Pack_ValidationFail (pReport, R_RPACK_ERROR_INVALID_DATA, pHeader->hashTableOffset + (uint64_t)j * sizeof (*pEntry), j, 0);
            }
            if (R_Pack_RangesOverlap (pEntry->atlasOffsetX, pEntry->width, pHashes[j].atlasOffsetX, pHashes[j].width) &&
                R_Pack_RangesOverlap (pEntry->atlasOffsetY, pEntry->height, pHashes[j].atlasOffsetY, pHashes[j].height))
            {
                return R_Pack_ValidationFail (pReport, R_RPACK_ERROR_INVALID_DATA, pHeader->hashTableOffset + (uint64_t)j * sizeof (*pEntry), j, 0);
            }
        }

        for (uint64_t j = 0; j < texturePixels; ++j)
        {
            uint64_t pixelIndex = pEntry->pixelIndexTableOffset + j;
            const struct R_PackPixelIndexEntry* pPixel = &pPixels[pixelIndex];
            if (pPixel->colorIndex >= pHeader->colorTableSize || pPixel->runWidth == 0 || pPixel->runWidth > 63 ||
                pPixel->runHeight == 0 || pPixel->runHeight > 63 || pPixel->exponent == 0)
            {
                return R_Pack_ValidationFail (pReport, R_RPACK_ERROR_INVALID_DATA,
                                               pHeader->pixelIndexTableOffset + pixelIndex * sizeof (*pPixel), i, j);
            }
            (void)pColors;
        }
    }

    return 1;
}

int
R_Pack_ValidatePackedFile (const char* pPath, struct R_PackValidationReport* pReport)
{
    if (!pPath) return R_Pack_ValidationFail (pReport, R_RPACK_ERROR_INVALID_ARGUMENT, 0, 0, 0);
    FILE* pFile = fopen (pPath, "rb");
    if (!pFile) return R_Pack_ValidationFail (pReport, R_RPACK_ERROR_INVALID_DATA, 0, 0, 0);
    if (fseek (pFile, 0, SEEK_END) != 0)
    {
        fclose (pFile);
        return R_Pack_ValidationFail (pReport, R_RPACK_ERROR_INVALID_DATA, 0, 0, 0);
    }
    long fileSize = ftell (pFile);
    if (fileSize < 0)
    {
        fclose (pFile);
        return R_Pack_ValidationFail (pReport, R_RPACK_ERROR_INVALID_DATA, 0, 0, 0);
    }
    rewind (pFile);
    uint8_t* pData = (uint8_t*)malloc ((size_t)fileSize);
    if (!pData || fread (pData, 1, (size_t)fileSize, pFile) != (size_t)fileSize)
    {
        free (pData);
        fclose (pFile);
        return R_Pack_ValidationFail (pReport, R_RPACK_ERROR_INVALID_DATA, 0, 0, 0);
    }
    fclose (pFile);
    int result = R_Pack_ValidatePackedData (pData, (uint64_t)fileSize, pReport);
    free (pData);
    return result;
}
