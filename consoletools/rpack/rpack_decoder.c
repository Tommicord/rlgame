#include "rpack/rpack_decoder.h"
#include "rpack/rpack_encoder.h"

#include "rlgame.base/cstl/cstl_heap_allocator.h"
#include "rlgame.base/cstl/cstl_log.h"
#include <string.h>

struct r_pack_decoder*
r_pack_new_decoder (const uint8_t* pData, uint64_t dataSize)
{
    if (!pData || dataSize < R_PACK_HEADER_SIZE)
    {
        return NULL;
    }
    if (!r_pack_decoder_validate_file (pData, dataSize))
    {
        return NULL;
    }

    struct r_pack_decoder* pDecoder
        = (struct r_pack_decoder*)R_CSTL_HeapAlloc (sizeof (struct r_pack_decoder));
    if (!pDecoder)
    {
        return NULL;
    }

    memset (pDecoder, 0, sizeof (struct r_pack_decoder));
    pDecoder->pData = pData;
    pDecoder->dataSize = dataSize;

    pDecoder->pHeader = (struct r_pack_header*)pData;

    if (pDecoder->pHeader->textureCount > 0)
    {
        pDecoder->pHashTable = (struct r_pack_hash_entry*)(pData + pDecoder->pHeader->hashTableOffset);
    }

    if (pDecoder->pHeader->colorTableSize > 0)
    {
        pDecoder->pColorTable = (struct r_pack_color_entry*)(pData + pDecoder->pHeader->colorTableOffset);
    }

    if (pDecoder->pHeader->pixelIndexTableSize > 0)
    {
        pDecoder->pPixelIndexTable
            = (struct r_pack_pixel_index_entry*)(pData + pDecoder->pHeader->pixelIndexTableOffset);
    }

    return pDecoder;
}

void
r_pack_delete_decoder (struct r_pack_decoder* pDecoder)
{
    if (!pDecoder)
    {
        return;
    }

    R_CSTL_HeapFree (pDecoder);
}

int
r_pack_decoder_validate_file (const uint8_t* pData, uint64_t dataSize)
{
    if (!pData || dataSize < R_PACK_HEADER_SIZE)
    {
        return 0;
    }

    const struct r_pack_header* pHeader = (const struct r_pack_header*)pData;
    uint64_t                    expectedSize = r_pack_get_expected_file_size (pHeader);
    return expectedSize != 0 && expectedSize <= dataSize;
}

const struct r_pack_hash_entry*
r_pack_decoder_find_texture (const struct r_pack_decoder* pDecoder, const char* pName)
{
    if (!pDecoder || !pDecoder->pHeader || !pName)
    {
        return NULL;
    }

    uint64_t nameHash = r_pack_hash64_string (pName, 0);

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
r_pack_decoder_get_texture_count (const struct r_pack_decoder* pDecoder)
{
    if (!pDecoder || !pDecoder->pHeader)
    {
        return 0;
    }
    return pDecoder->pHeader->textureCount;
}

enum r_pack_error
r_pack_decoder_get_texture_dimensions (
    const struct r_pack_decoder* pDecoder,
    const char*                  pName,
    uint32_t*                    pWidth,
    uint32_t*                    pHeight)
{
    if (!pDecoder || !pName || !pWidth || !pHeight)
    {
        return R_PACK_ERROR_INVALID_ARGUMENT;
    }

    const struct r_pack_hash_entry* pEntry = r_pack_decoder_find_texture (pDecoder, pName);
    if (!pEntry)
    {
        return R_PACK_ERROR_TEXTURE_NOT_FOUND;
    }

    *pWidth = pEntry->width;
    *pHeight = pEntry->height;

    return R_PACK_OK;
}

uint64_t
r_pack_decoder_get_texture_size (const struct r_pack_decoder* pDecoder, const char* pName)
{
    uint32_t width = 0, height = 0;
    if (r_pack_decoder_get_texture_dimensions (pDecoder, pName, &width, &height) != R_PACK_OK)
    {
        return 0;
    }
    return (uint64_t)width * height * 4;
}

uint64_t
r_pack_decoder_get_textures_size (const struct r_pack_decoder* pDecoder, const char** pNames, uint32_t nameCount)
{
    if (!pDecoder || !pNames || nameCount == 0)
    {
        return 0;
    }

    uint64_t totalSize = 0;
    for (uint32_t i = 0; i < nameCount; ++i)
    {
        totalSize += r_pack_decoder_get_texture_size (pDecoder, pNames[i]);
    }
    return totalSize;
}

enum r_pack_error
r_pack_decoder_decode_texture (
    const struct r_pack_decoder* pDecoder,
    const char*                  pName,
    uint8_t*                     pOutputBuffer,
    uint64_t                     outputBufferSize,
    uint64_t*                    pBytesWritten)
{
    if (!pDecoder || !pName || !pOutputBuffer)
    {
        return R_PACK_ERROR_INVALID_ARGUMENT;
    }

    const struct r_pack_hash_entry* pEntry = r_pack_decoder_find_texture (pDecoder, pName);
    if (!pEntry)
    {
        return R_PACK_ERROR_TEXTURE_NOT_FOUND;
    }

    uint64_t requiredSize = (uint64_t)pEntry->width * pEntry->height * 4;
    if (outputBufferSize < requiredSize)
    {
        return R_PACK_ERROR_BUFFER_TOO_SMALL;
    }

    uint32_t pixelCount = pEntry->width * pEntry->height;
    uint32_t startIndex = pEntry->pixelIndexTableOffset;

    // Prefetch color table for better cache locality
    const struct r_pack_color_entry* pColorTable = pDecoder->pColorTable;
    const struct r_pack_pixel_index_entry* pPixelIndexTable = pDecoder->pPixelIndexTable;

    // Process RLE-encoded pixel index table (horizontal-only RLE)
    uint32_t currentX = 0;
    uint32_t currentY = 0;
    uint32_t tableIndex = 0;
    
    while (currentY < pEntry->height && tableIndex < pDecoder->pHeader->pixelIndexTableSize)
    {
        if (startIndex + tableIndex >= pDecoder->pHeader->pixelIndexTableSize)
        {
            return R_PACK_ERROR_INVALID_DATA;
        }

        const struct r_pack_pixel_index_entry* pPixelEntry = &pPixelIndexTable[startIndex + tableIndex];
        if (pPixelEntry->colorIndex >= pDecoder->pHeader->colorTableSize)
        {
            return R_PACK_ERROR_INVALID_DATA;
        }

        const struct r_pack_color_entry* pColorEntry = &pColorTable[pPixelEntry->colorIndex];

        uint8_t r, g, b;
        r_pack_YUVToRGBA (
            pColorEntry->luminance,
            pColorEntry->luminanceExp,
            pColorEntry->chrominanceU,
            pColorEntry->chrominanceV,
            &r,
            &g,
            &b);
        
        // Expand horizontal RLE run
        uint32_t runWidth = pPixelEntry->runWidth;
        
        for (uint32_t rx = 0; rx < runWidth && currentX + rx < pEntry->width; ++rx)
        {
            uint32_t outputX = currentX + rx;
            uint32_t outputIndex = (currentY * pEntry->width + outputX) * 4;
            
            pOutputBuffer[outputIndex + 0] = r;
            pOutputBuffer[outputIndex + 1] = g;
            pOutputBuffer[outputIndex + 2] = b;
            pOutputBuffer[outputIndex + 3] = 255;
        }
        
        // Advance position
        currentX += runWidth;
        
        // Move to next row if we reached the end of current row
        if (currentX >= pEntry->width)
        {
            currentX = 0;
            currentY++;
        }
        
        tableIndex++;
    }

    if (pBytesWritten)
    {
        *pBytesWritten = requiredSize;
    }

    return R_PACK_OK;
}

enum r_pack_error
r_pack_decoder_decode_textures (
    const struct r_pack_decoder* pDecoder,
    const char**                 pNames,
    uint32_t                     nameCount,
    uint8_t*                     pOutputBuffer,
    uint64_t                     outputBufferSize,
    uint64_t*                    pBytesWritten)
{
    if (!pDecoder || !pNames || nameCount == 0 || !pOutputBuffer)
    {
        return R_PACK_ERROR_INVALID_ARGUMENT;
    }
    uint64_t offset = 0;
    for (uint32_t i = 0; i < nameCount; ++i)
    {
        uint64_t textureSize = r_pack_decoder_get_texture_size (pDecoder, pNames[i]);
        if (textureSize == 0)
        {
            return R_PACK_ERROR_TEXTURE_NOT_FOUND;
        }

        if (offset + textureSize > outputBufferSize)
        {
            return R_PACK_ERROR_BUFFER_TOO_SMALL;
        }
        uint64_t          written = 0;
        enum r_pack_error err = r_pack_decoder_decode_texture (
            pDecoder,
            pNames[i],
            pOutputBuffer + offset,
            textureSize,
            &written);
        if (err != R_PACK_OK)
        {
            return err;
        }
        offset += written;
    }

    if (pBytesWritten)
    {
        *pBytesWritten = offset;
    }

    return R_PACK_OK;
}
