#include "rpack_val.h"

#include "rpack_format.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int
r_pack_validation_fail (
    struct r_pack_validation_report* pReport,
    enum r_pack_error               error,
    uint64_t                        offset,
    uint32_t                        textureIndex,
    uint64_t                        pixelIndex)
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
r_pack_ranges_overlap (uint64_t leftStart, uint64_t leftSize, uint64_t rightStart, uint64_t rightSize)
{
    return leftStart < rightStart + rightSize && rightStart < leftStart + leftSize;
}

int
r_pack_validate_packed_data (const uint8_t* pData, uint64_t dataSize, struct r_pack_validation_report* pReport)
{
    if (pReport)
    {
        memset (pReport, 0, sizeof (*pReport));
        pReport->error = R_PACK_OK;
    }
    if (!pData || dataSize < sizeof (struct r_pack_header))
    {
        return r_pack_validation_fail (pReport, R_PACK_ERROR_INVALID_ARGUMENT, 0, 0, 0);
    }

    const struct r_pack_header* pHeader = (const struct r_pack_header*)pData;
    if (!r_pack_validate_header (pHeader))
    {
        return r_pack_validation_fail (pReport, R_PACK_ERROR_INVALID_FORMAT, 0, 0, 0);
    }

    uint64_t expectedSize = r_pack_get_expected_file_size (pHeader);
    if (expectedSize == 0 || expectedSize != dataSize)
    {
        return r_pack_validation_fail (pReport, R_PACK_ERROR_INVALID_FORMAT, dataSize, 0, 0);
    }

    uint64_t                       atlasSize = (uint64_t)pHeader->atlasWidth * pHeader->atlasHeight * 2;
    uint64_t                       fileEnd = dataSize;
    const struct r_pack_hash_entry* pHashes
        = (const struct r_pack_hash_entry*)(pData + pHeader->hashTableOffset);
    const struct r_pack_color_entry* pColors
        = (const struct r_pack_color_entry*)(pData + pHeader->colorTableOffset);
    const struct r_pack_pixel_index_entry* pPixels
        = (const struct r_pack_pixel_index_entry*)(pData + pHeader->pixelIndexTableOffset);

    if (pHeader->dataOffset > fileEnd || atlasSize > fileEnd - pHeader->dataOffset)
    {
        return r_pack_validation_fail (pReport, R_PACK_ERROR_INVALID_DATA, pHeader->dataOffset, 0, 0);
    }

    for (uint32_t i = 0; i < pHeader->textureCount; ++i)
    {
        const struct r_pack_hash_entry* pEntry = &pHashes[i];
        uint64_t                       texturePixels = (uint64_t)pEntry->width * pEntry->height;
        if (pEntry->width == 0 || pEntry->height == 0 || pEntry->atlasOffsetX > pHeader->atlasWidth
            || pEntry->width > pHeader->atlasWidth - pEntry->atlasOffsetX
            || pEntry->atlasOffsetY > pHeader->atlasHeight
            || pEntry->height > pHeader->atlasHeight - pEntry->atlasOffsetY
            || pEntry->colorTableIndex >= pHeader->colorTableSize
            || pEntry->pixelIndexTableOffset > pHeader->pixelIndexTableSize)
        {
            return r_pack_validation_fail (
                pReport,
                R_PACK_ERROR_INVALID_DATA,
                pHeader->hashTableOffset + (uint64_t)i * sizeof (*pEntry),
                i,
                0);
        }

        for (uint32_t j = i + 1; j < pHeader->textureCount; ++j)
        {
            if (pEntry->nameHash == pHashes[j].nameHash)
            {
                return r_pack_validation_fail (
                    pReport,
                    R_PACK_ERROR_INVALID_DATA,
                    pHeader->hashTableOffset + (uint64_t)j * sizeof (*pEntry),
                    j,
                    0);
            }
            if (r_pack_ranges_overlap (
                    pEntry->atlasOffsetX,
                    pEntry->width,
                    pHashes[j].atlasOffsetX,
                    pHashes[j].width)
                && r_pack_ranges_overlap (
                    pEntry->atlasOffsetY,
                    pEntry->height,
                    pHashes[j].atlasOffsetY,
                    pHashes[j].height))
            {
                return r_pack_validation_fail (
                    pReport,
                    R_PACK_ERROR_INVALID_DATA,
                    pHeader->hashTableOffset + (uint64_t)j * sizeof (*pEntry),
                    j,
                    0);
            }
        }

        // Validate RLE pixel index entries
        uint64_t totalPixelsCovered = 0;
        uint64_t j = 0;
        while (totalPixelsCovered < texturePixels && pEntry->pixelIndexTableOffset + j < pHeader->pixelIndexTableSize)
        {
            uint64_t                             pixelIndex = pEntry->pixelIndexTableOffset + j;
            const struct r_pack_pixel_index_entry* pPixel = &pPixels[pixelIndex];
            if (pPixel->colorIndex >= pHeader->colorTableSize || pPixel->runWidth == 0
                || pPixel->runWidth > 63 || pPixel->runHeight == 0 || pPixel->runHeight > 63
                || pPixel->exponent == 0)
            {
                return r_pack_validation_fail (
                    pReport,
                    R_PACK_ERROR_INVALID_DATA,
                    pHeader->pixelIndexTableOffset + pixelIndex * sizeof (*pPixel),
                    i,
                    j);
            }
            
            uint64_t pixelsInRun = (uint64_t)pPixel->runWidth * pPixel->runHeight;
            totalPixelsCovered += pixelsInRun;
            j++;
        }
        
        // Check if RLE entries covered all pixels
        if (totalPixelsCovered != texturePixels)
        {
            return r_pack_validation_fail (
                pReport,
                R_PACK_ERROR_INVALID_DATA,
                pHeader->pixelIndexTableOffset + pEntry->pixelIndexTableOffset * sizeof (struct r_pack_pixel_index_entry),
                i,
                0);
        }
        
        (void)pColors;
    }

    return 1;
}

int
r_pack_validate_packed_file (const char* pPath, struct r_pack_validation_report* pReport)
{
    if (!pPath) return r_pack_validation_fail (pReport, R_PACK_ERROR_INVALID_ARGUMENT, 0, 0, 0);
    FILE* pFile = fopen (pPath, "rb");
    if (!pFile) return r_pack_validation_fail (pReport, R_PACK_ERROR_INVALID_DATA, 0, 0, 0);
    if (fseek (pFile, 0, SEEK_END) != 0)
    {
        fclose (pFile);
        return r_pack_validation_fail (pReport, R_PACK_ERROR_INVALID_DATA, 0, 0, 0);
    }
    long fileSize = ftell (pFile);
    if (fileSize < 0)
    {
        fclose (pFile);
        return r_pack_validation_fail (pReport, R_PACK_ERROR_INVALID_DATA, 0, 0, 0);
    }
    rewind (pFile);
    uint8_t* pData = (uint8_t*)malloc ((size_t)fileSize);
    if (!pData || fread (pData, 1, (size_t)fileSize, pFile) != (size_t)fileSize)
    {
        free (pData);
        fclose (pFile);
        return r_pack_validation_fail (pReport, R_PACK_ERROR_INVALID_DATA, 0, 0, 0);
    }
    fclose (pFile);
    int result = r_pack_validate_packed_data (pData, (uint64_t)fileSize, pReport);
    free (pData);
    return result;
}
