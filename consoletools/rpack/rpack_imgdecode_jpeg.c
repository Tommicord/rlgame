#include "rpack_imgdecode_jpeg.h"

#include "rlgame.base/cstl/cstl_heap_allocator.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

struct R_Pack_JpegHuffman
{
        uint16_t codes[256];
        uint8_t  sizes[256];
        uint8_t  values[256];
        uint16_t count;
};

struct R_Pack_JpegDecoder
{
        const uint8_t*           data;
        size_t                   size;
        size_t                   offset;
        uint32_t                 width;
        uint32_t                 height;
        uint8_t                  components;
        uint8_t                  ids[3];
        uint8_t                  horizontal[3];
        uint8_t                  vertical[3];
        uint8_t                  quant[3];
        uint16_t                 quantTables[2][64];
        uint8_t                  quantPresent[2];
        struct R_Pack_JpegHuffman dc[2];
        struct R_Pack_JpegHuffman ac[2];
        uint8_t                  dcPresent[2];
        uint8_t                  acPresent[2];
        uint8_t                  unsupported;
        uint8_t                  scanComponent[3];
        uint8_t                  scanDc[3];
        uint8_t                  scanAc[3];
        uint8_t                  scanCount;
        uint32_t                 bitBuffer;
        uint8_t                  bits;
        int16_t                  dcPredictor[3];
};

static int
R_Pack_JpegReadByte (struct R_Pack_JpegDecoder* pDecoder, uint8_t* pValue)
{
    if (pDecoder->offset >= pDecoder->size) return 0;
    *pValue = pDecoder->data[pDecoder->offset++];
    return 1;
}

static int
R_Pack_JpegRead16 (struct R_Pack_JpegDecoder* pDecoder, uint16_t* pValue)
{
    uint8_t high, low;
    if (!R_Pack_JpegReadByte (pDecoder, &high) || !R_Pack_JpegReadByte (pDecoder, &low)) return 0;
    *pValue = ((uint16_t)high << 8) | low;
    return 1;
}

static int
R_Pack_JpegBuildHuffman (
    struct R_Pack_JpegHuffman* pTable,
    const uint8_t*            pCounts,
    const uint8_t*            pValues,
    uint16_t                  valueCount)
{
    uint16_t code = 0;
    uint16_t value = 0;
    pTable->count = valueCount;
    for (uint8_t length = 1; length <= 16; ++length)
    {
        for (uint8_t i = 0; i < pCounts[length - 1]; ++i)
        {
            if (value >= valueCount) return 0;
            pTable->codes[value] = code++;
            pTable->sizes[value] = length;
            pTable->values[value] = pValues[value];
            ++value;
        }
        code <<= 1;
    }
    return value == valueCount;
}

static int
R_Pack_JpegReadBits (struct R_Pack_JpegDecoder* pDecoder, uint8_t count, uint32_t* pValue)
{
    while (pDecoder->bits < count)
    {
        uint8_t byte;
        if (!R_Pack_JpegReadByte (pDecoder, &byte)) return 0;
        if (byte == 0xFF)
        {
            uint8_t marker;
            if (!R_Pack_JpegReadByte (pDecoder, &marker) || marker != 0x00) return 0;
        }
        pDecoder->bitBuffer = (pDecoder->bitBuffer << 8) | byte;
        pDecoder->bits += 8;
    }
    pDecoder->bits -= count;
    *pValue = (pDecoder->bitBuffer >> pDecoder->bits) & ((1u << count) - 1u);
    return 1;
}

static int
R_Pack_JpegHuffmanValue (
    struct R_Pack_JpegDecoder*       pDecoder,
    const struct R_Pack_JpegHuffman* pTable,
    uint8_t*                        pValue)
{
    uint32_t code = 0;
    for (uint8_t length = 1; length <= 16; ++length)
    {
        uint32_t bit;
        if (!R_Pack_JpegReadBits (pDecoder, 1, &bit)) return 0;
        code = (code << 1) | bit;
        for (uint16_t i = 0; i < pTable->count; ++i)
        {
            if (pTable->sizes[i] == length && pTable->codes[i] == code)
            {
                *pValue = pTable->values[i];
                return 1;
            }
        }
    }
    return 0;
}

static int16_t
R_Pack_JpegExtend (uint32_t value, uint8_t bits)
{
    if (bits == 0) return 0;
    return (value & (1u << (bits - 1))) ? (int16_t)value : (int16_t)value - (int16_t)((1u << bits) - 1u);
}

static int
R_Pack_JpegDecodeBlock (struct R_Pack_JpegDecoder* pDecoder, uint8_t component, int16_t* pBlock)
{
    memset (pBlock, 0, 64 * sizeof (*pBlock));
    uint8_t dcSymbol;
    if (!R_Pack_JpegHuffmanValue (pDecoder, &pDecoder->dc[pDecoder->scanDc[component]], &dcSymbol)) return 0;
    uint32_t bits = 0;
    if (dcSymbol && !R_Pack_JpegReadBits (pDecoder, dcSymbol, &bits)) return 0;
    pDecoder->dcPredictor[component] += R_Pack_JpegExtend (bits, dcSymbol);
    pBlock[0]
        = pDecoder->dcPredictor[component] * (int16_t)pDecoder->quantTables[pDecoder->quant[component]][0];

    static const uint8_t zigzag[64]
        = {0,  1,  8,  16, 9,  2,  3,  10, 17, 24, 32, 25, 18, 11, 4,  5,  12, 19, 26, 33, 40, 48,
           41, 34, 27, 20, 13, 6,  7,  14, 21, 28, 35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23,
           30, 37, 44, 51, 58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63};
    for (uint8_t index = 1; index < 64;)
    {
        uint8_t symbol;
        if (!R_Pack_JpegHuffmanValue (pDecoder, &pDecoder->ac[pDecoder->scanAc[component]], &symbol))
            return 0;
        uint8_t run = symbol >> 4;
        uint8_t size = symbol & 15;
        if (size == 0)
        {
            if (run == 15)
            {
                index += 16;
                continue;
            }
            break;
        }
        index += run;
        if (index >= 64 || !R_Pack_JpegReadBits (pDecoder, size, &bits)) return 0;
        uint8_t coefficientIndex = zigzag[index++];
        pBlock[coefficientIndex]
            = R_Pack_JpegExtend (bits, size)
              * (int16_t)pDecoder->quantTables[pDecoder->quant[component]][coefficientIndex];
    }
    return 1;
}

static uint8_t
R_Pack_JpegClamp (int value)
{
    return value < 0 ? 0 : value > 255 ? 255 : (uint8_t)value;
}

static void
R_Pack_JpegIdct (const int16_t* pBlock, uint8_t* pOutput)
{
    const double pi = 3.14159265358979323846;
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 8; ++x)
        {
            double sum = 0.0;
            for (int v = 0; v < 8; ++v)
                for (int u = 0; u < 8; ++u)
                {
                    double cu = u == 0 ? 0.7071067811865475 : 1.0;
                    double cv = v == 0 ? 0.7071067811865475 : 1.0;
                    sum += cu * cv * pBlock[v * 8 + u] * cos ((2 * x + 1) * u * pi / 16.0)
                           * cos ((2 * y + 1) * v * pi / 16.0);
                }
            pOutput[y * 8 + x] = R_Pack_JpegClamp ((int)lrint (sum / 4.0) + 128);
        }
}

static int
R_Pack_JpegParse (struct R_Pack_JpegDecoder* pDecoder)
{
    uint8_t marker;
    if (!R_Pack_JpegReadByte (pDecoder, &marker) || marker != 0xFF || !R_Pack_JpegReadByte (pDecoder, &marker)
        || marker != 0xD8)
        return 0;
    for (;;)
    {
        do
        {
            if (!R_Pack_JpegReadByte (pDecoder, &marker)) return 0;
        } while (marker != 0xFF);
        do
        {
            if (!R_Pack_JpegReadByte (pDecoder, &marker)) return 0;
        } while (marker == 0xFF);
        if (marker == 0xDA) break;
        if (marker == 0xC2 || marker == 0xC6 || marker == 0xCA || marker == 0xCE)
        {
            pDecoder->unsupported = 1;
            return 0;
        }
        if (marker == 0xD9) return 0;
        uint16_t length;
        if (!R_Pack_JpegRead16 (pDecoder, &length) || length < 2 || pDecoder->offset > pDecoder->size
            || length - 2 > pDecoder->size - pDecoder->offset)
            return 0;
        size_t end = pDecoder->offset + length - 2;
        if (marker == 0xDB)
        {
            while (pDecoder->offset < end)
            {
                uint8_t info;
                if (!R_Pack_JpegReadByte (pDecoder, &info)) return 0;
                uint8_t table = info & 15;
                if ((info >> 4) != 0 || table > 1 || end - pDecoder->offset < 64) return 0;
                static const uint8_t zigzag[64]
                    = {0,  1,  8,  16, 9,  2,  3,  10, 17, 24, 32, 25, 18, 11, 4,  5,  12, 19, 26, 33, 40, 48,
                       41, 34, 27, 20, 13, 6,  7,  14, 21, 28, 35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23,
                       30, 37, 44, 51, 58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63};
                for (uint8_t i = 0; i < 64; ++i)
                {
                    uint8_t value;
                    if (!R_Pack_JpegReadByte (pDecoder, &value)) return 0;
                    pDecoder->quantTables[table][zigzag[i]] = value;
                }
                pDecoder->quantPresent[table] = 1;
            }
        }
        else if (marker == 0xC4)
        {
            while (pDecoder->offset < end)
            {
                uint8_t  info, counts[16], values[256];
                uint16_t count = 0;
                if (!R_Pack_JpegReadByte (pDecoder, &info)) return 0;
                for (uint8_t i = 0; i < 16; ++i)
                {
                    if (!R_Pack_JpegReadByte (pDecoder, &counts[i])) return 0;
                    count += counts[i];
                }
                if (count > 256 || end - pDecoder->offset < count) return 0;
                for (uint16_t i = 0; i < count; ++i)
                    if (!R_Pack_JpegReadByte (pDecoder, &values[i])) return 0;
                uint8_t table = info & 15;
                if (table > 1
                    || !R_Pack_JpegBuildHuffman (
                        (info >> 4) ? &pDecoder->ac[table] : &pDecoder->dc[table],
                        counts,
                        values,
                        count))
                    return 0;
                if (info >> 4) pDecoder->acPresent[table] = 1;
                else pDecoder->dcPresent[table] = 1;
            }
        }
        else if (marker == 0xC0)
        {
            uint8_t  precision, count;
            uint16_t width, height;
            if (!R_Pack_JpegReadByte (pDecoder, &precision) || precision != 8
                || !R_Pack_JpegRead16 (pDecoder, &height) || !R_Pack_JpegRead16 (pDecoder, &width)
                || !R_Pack_JpegReadByte (pDecoder, &count) || count != 1 && count != 3)
                return 0;
            pDecoder->height = height;
            pDecoder->width = width;
            pDecoder->components = count;
            for (uint8_t i = 0; i < count; ++i)
            {
                uint8_t sampling;
                if (!R_Pack_JpegReadByte (pDecoder, &pDecoder->ids[i])
                    || !R_Pack_JpegReadByte (pDecoder, &sampling)
                    || !R_Pack_JpegReadByte (pDecoder, &pDecoder->quant[i]))
                    return 0;
                pDecoder->horizontal[i] = sampling >> 4;
                pDecoder->vertical[i] = sampling & 15;
                if (!pDecoder->horizontal[i] || pDecoder->horizontal[i] > 2 || !pDecoder->vertical[i]
                    || pDecoder->vertical[i] > 2 || pDecoder->quant[i] > 1)
                    return 0;
            }
        }
        pDecoder->offset = end;
    }
    uint8_t  count, spectral;
    uint16_t length;
    if (!R_Pack_JpegRead16 (pDecoder, &length) || length < 6 || !R_Pack_JpegReadByte (pDecoder, &count)
        || count != pDecoder->components)
        return 0;
    pDecoder->scanCount = count;
    for (uint8_t i = 0; i < count; ++i)
    {
        uint8_t id, tables;
        if (!R_Pack_JpegReadByte (pDecoder, &id) || !R_Pack_JpegReadByte (pDecoder, &tables)) return 0;
        uint8_t component = 0;
        while (component < pDecoder->components && pDecoder->ids[component] != id)
            ++component;
        if (component == pDecoder->components) return 0;
        pDecoder->scanComponent[i] = component;
        pDecoder->scanDc[component] = tables >> 4;
        pDecoder->scanAc[component] = tables & 15;
    }
    if (!R_Pack_JpegReadByte (pDecoder, &spectral) || spectral != 0
        || !R_Pack_JpegReadByte (pDecoder, &spectral) || spectral != 63
        || !R_Pack_JpegReadByte (pDecoder, &spectral) || spectral != 0)
        return 0;
    for (uint8_t i = 0; i < pDecoder->components; ++i)
        if (!pDecoder->quantPresent[pDecoder->quant[i]] || !pDecoder->dcPresent[pDecoder->scanDc[i]]
            || !pDecoder->acPresent[pDecoder->scanAc[i]])
            return 0;
    return 1;
}

enum R_Pack_Error
R_Pack_JpegDecode (const uint8_t* pData, size_t dataSize, struct R_Pack_JpegImage* pImage)
{
    if (!pData || !pImage || dataSize < 4) return R_RPACK_ERROR_INVALID_ARGUMENT;
    memset (pImage, 0, sizeof (*pImage));
    struct R_Pack_JpegDecoder decoder = {.data = pData, .size = dataSize};
    if (!R_Pack_JpegParse (&decoder))
        return decoder.unsupported ? R_RPACK_ERROR_UNSUPPORTED_FORMAT : R_RPACK_ERROR_INVALID_FORMAT;
    if (!decoder.width || !decoder.height || decoder.width > UINT32_MAX / 4
        || (size_t)decoder.width * 4 > SIZE_MAX / decoder.height)
        return R_RPACK_ERROR_INVALID_FORMAT;
    size_t stride = (size_t)decoder.width * 4, outputSize = stride * decoder.height;
    pImage->pPixels = R_CSTL_HeapAlloc (outputSize);
    if (!pImage->pPixels) return R_RPACK_ERROR_OUT_OF_MEMORY;
    pImage->width = decoder.width;
    pImage->height = decoder.height;
    pImage->stride = (uint32_t)stride;
    uint8_t maxH = decoder.horizontal[0], maxV = decoder.vertical[0];
    for (uint8_t i = 1; i < decoder.components; ++i)
    {
        if (decoder.horizontal[i] > maxH) maxH = decoder.horizontal[i];
        if (decoder.vertical[i] > maxV) maxV = decoder.vertical[i];
    }
    uint32_t mcuWidth = maxH * 8, mcuHeight = maxV * 8;
    uint8_t  blocks[3][4][64];
    int16_t  coefficients[64];
    for (uint32_t my = 0; my < decoder.height; my += mcuHeight)
        for (uint32_t mx = 0; mx < decoder.width; mx += mcuWidth)
        {
            for (uint8_t component = 0; component < decoder.components; ++component)
            {
                for (uint8_t block = 0; block < decoder.horizontal[component] * decoder.vertical[component];
                     ++block)
                {
                    if (!R_Pack_JpegDecodeBlock (&decoder, component, coefficients))
                    {
                        R_CSTL_HeapFree (pImage->pPixels);
                        memset (pImage, 0, sizeof (*pImage));
                        return R_RPACK_ERROR_INVALID_DATA;
                    }
                    R_Pack_JpegIdct (coefficients, blocks[component][block]);
                }
            }
            for (uint32_t y = 0; y < mcuHeight && my + y < decoder.height; ++y)
                for (uint32_t x = 0; x < mcuWidth && mx + x < decoder.width; ++x)
                {
                    uint8_t values[3] = {0, 128, 128};
                    for (uint8_t component = 0; component < decoder.components; ++component)
                    {
                        uint32_t sx = x * decoder.horizontal[component] / maxH,
                                 sy = y * decoder.vertical[component] / maxV;
                        uint8_t block = (uint8_t)((sy / 8) * decoder.horizontal[component] + sx / 8);
                        values[component] = blocks[component][block][(sy % 8) * 8 + sx % 8];
                    }
                    uint8_t* pixel = pImage->pPixels + (size_t)(my + y) * stride + (mx + x) * 4;
                    if (decoder.components == 1) pixel[0] = pixel[1] = pixel[2] = values[0];
                    else
                    {
                        pixel[0] = R_Pack_JpegClamp (values[0] + 1.402 * ((int)values[2] - 128));
                        pixel[1] = R_Pack_JpegClamp (
                            values[0] - 0.344136 * ((int)values[1] - 128)
                            - 0.714136 * ((int)values[2] - 128));
                        pixel[2] = R_Pack_JpegClamp (values[0] + 1.772 * ((int)values[1] - 128));
                    }
                    pixel[3] = 255;
                }
        }
    return R_RPACK_OK;
}

enum R_Pack_Error
R_Pack_JpegDecodeFile (const char* pPath, struct R_Pack_JpegImage* pImage)
{
    if (!pPath || !pImage) return R_RPACK_ERROR_INVALID_ARGUMENT;
    FILE* pFile = fopen (pPath, "rb");
    if (!pFile) return R_RPACK_ERROR_INVALID_DATA;
    if (fseek (pFile, 0, SEEK_END) != 0)
    {
        fclose (pFile);
        return R_RPACK_ERROR_INVALID_DATA;
    }
    long length = ftell (pFile);
    if (length <= 0)
    {
        fclose (pFile);
        return R_RPACK_ERROR_INVALID_DATA;
    }
    rewind (pFile);
    uint8_t* data = R_CSTL_HeapAlloc ((size_t)length);
    if (!data || fread (data, 1, (size_t)length, pFile) != (size_t)length)
    {
        if (data) R_CSTL_HeapFree (data);
        fclose (pFile);
        return R_RPACK_ERROR_INVALID_DATA;
    }
    fclose (pFile);
    enum R_Pack_Error result = R_Pack_JpegDecode (data, (size_t)length, pImage);
    R_CSTL_HeapFree (data);
    return result;
}

void
R_Pack_JpegFreeImage (struct R_Pack_JpegImage* pImage)
{
    if (!pImage) return;
    if (pImage->pPixels) R_CSTL_HeapFree (pImage->pPixels);
    memset (pImage, 0, sizeof (*pImage));
}
