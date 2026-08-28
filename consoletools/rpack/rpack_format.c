#include "rpack/rpack_format.h"
#include "rpack/rpack_platform.h"

#include <string.h>
#include <inttypes.h>
#include <stdint.h>

#define R_RPACK_PRIME64_1 0x9E3779B185EBCA87ULL
#define R_RPACK_PRIME64_2 0xC2B2AE3D27D4EB4FULL
#define R_RPACK_PRIME64_3 0x165667B19E3779F9ULL
#define R_RPACK_PRIME64_4 0x85EBCA77C2B2AE63ULL
#define R_RPACK_PRIME64_5 0x27D4EB2F165667C5ULL

static inline uint64_t
R_Pack_Rotl64 (uint64_t value, int count)
{
    return (value << count) | (value >> (64 - count));
}

uint64_t
R_Pack_Hash64 (const void* pData, size_t length, uint64_t seed)
{
    const uint8_t* p = (const uint8_t*)pData;
    const uint8_t* end = p + length;
    uint64_t       h64 = seed + R_RPACK_PRIME64_5;

    if (length >= 32)
    {
        const uint8_t* limit = end - 32;
        uint64_t       v1 = seed + R_RPACK_PRIME64_1 + R_RPACK_PRIME64_2;
        uint64_t       v2 = seed + R_RPACK_PRIME64_2;
        uint64_t       v3 = seed + R_RPACK_PRIME64_3;
        uint64_t       v4 = seed + R_RPACK_PRIME64_4;

        do
        {
            v1 += *(const uint64_t*)p * R_RPACK_PRIME64_2;
            v1 = R_Pack_Rotl64 (v1, 31);
            v1 *= R_RPACK_PRIME64_1;
            p += 8;

            v2 += *(const uint64_t*)p * R_RPACK_PRIME64_2;
            v2 = R_Pack_Rotl64 (v2, 31);
            v2 *= R_RPACK_PRIME64_1;
            p += 8;

            v3 += *(const uint64_t*)p * R_RPACK_PRIME64_2;
            v3 = R_Pack_Rotl64 (v3, 31);
            v3 *= R_RPACK_PRIME64_1;
            p += 8;

            v4 += *(const uint64_t*)p * R_RPACK_PRIME64_2;
            v4 = R_Pack_Rotl64 (v4, 31);
            v4 *= R_RPACK_PRIME64_1;
            p += 8;
        } while (p <= limit);

        h64 = R_Pack_Rotl64 (v1, 1) + R_Pack_Rotl64 (v2, 7) + R_Pack_Rotl64 (v3, 12) + R_Pack_Rotl64 (v4, 18);

        v1 *= R_RPACK_PRIME64_2;
        v1 = R_Pack_Rotl64 (v1, 31);
        v1 *= R_RPACK_PRIME64_1;
        h64 ^= v1;
        h64 = h64 * R_RPACK_PRIME64_1 + R_RPACK_PRIME64_4;

        v2 *= R_RPACK_PRIME64_2;
        v2 = R_Pack_Rotl64 (v2, 31);
        v2 *= R_RPACK_PRIME64_1;
        h64 ^= v2;
        h64 = h64 * R_RPACK_PRIME64_1 + R_RPACK_PRIME64_4;

        v3 *= R_RPACK_PRIME64_2;
        v3 = R_Pack_Rotl64 (v3, 31);
        v3 *= R_RPACK_PRIME64_1;
        h64 ^= v3;
        h64 = h64 * R_RPACK_PRIME64_1 + R_RPACK_PRIME64_4;

        v4 *= R_RPACK_PRIME64_2;
        v4 = R_Pack_Rotl64 (v4, 31);
        v4 *= R_RPACK_PRIME64_1;
        h64 ^= v4;
        h64 = h64 * R_RPACK_PRIME64_1 + R_RPACK_PRIME64_4;
    }

    while (p + 8 <= end)
    {
        uint64_t k1 = *(const uint64_t*)p;
        k1 *= R_RPACK_PRIME64_2;
        k1 = R_Pack_Rotl64 (k1, 31);
        k1 *= R_RPACK_PRIME64_1;
        h64 ^= k1;
        h64 = R_Pack_Rotl64 (h64, 27) * R_RPACK_PRIME64_1 + R_RPACK_PRIME64_4;
        p += 8;
    }

    if (p + 4 <= end)
    {
        h64 ^= *(const uint32_t*)p * R_RPACK_PRIME64_1;
        h64 = R_Pack_Rotl64 (h64, 23) * R_RPACK_PRIME64_2 + R_RPACK_PRIME64_3;
        p += 4;
    }

    while (p < end)
    {
        h64 ^= (*p) * R_RPACK_PRIME64_5;
        h64 = R_Pack_Rotl64 (h64, 11) * R_RPACK_PRIME64_1;
        p++;
    }

    h64 ^= h64 >> 33;
    h64 *= R_RPACK_PRIME64_2;
    h64 ^= h64 >> 29;
    h64 *= R_RPACK_PRIME64_3;
    h64 ^= h64 >> 32;

    return h64;
}

uint64_t
R_Pack_Hash64String (const char* pStr, uint64_t seed)
{
    if (pStr == NULL) return 0;
    return R_Pack_Hash64 (pStr, strlen (pStr), seed);
}

int
R_Pack_ValidateHeader (const struct R_Pack_Header* pHeader)
{
    if (!pHeader)
    {
        return 0;
    }
    if (pHeader->magicInt32 != R_RPACK_MAGIC)
    {
        return 0;
    }
    if (pHeader->version != R_RPACK_VERSION)
    {
        return 0;
    }

    if (pHeader->atlasWidth == 0 || pHeader->atlasHeight == 0)
    {
        return 0;
    }

    uint64_t hashTableSize = (uint64_t)pHeader->textureCount * sizeof (struct R_Pack_HashEntry);
    uint64_t colorTableSize = (uint64_t)pHeader->colorTableSize * sizeof (struct R_Pack_ColorEntry);
    uint64_t pixelIndexTableSize
        = (uint64_t)pHeader->pixelIndexTableSize * sizeof (struct R_Pack_PixelIndexEntry);
    uint64_t atlasDataSize = (uint64_t)pHeader->atlasWidth * pHeader->atlasHeight * 2;
    if (pHeader->textureCount != 0 && hashTableSize / sizeof (struct R_Pack_HashEntry) != pHeader->textureCount
        || pHeader->colorTableSize != 0
               && colorTableSize / sizeof (struct R_Pack_ColorEntry) != pHeader->colorTableSize
        || pHeader->pixelIndexTableSize != 0
               && pixelIndexTableSize / sizeof (struct R_Pack_PixelIndexEntry) != pHeader->pixelIndexTableSize
        || atlasDataSize / 2 != (uint64_t)pHeader->atlasWidth * pHeader->atlasHeight
        || pHeader->dataOffset > UINT64_MAX - atlasDataSize)
    {
        return 0;
    }

    if (pHeader->hashTableOffset != R_RPACK_HEADER_SIZE
        || pHeader->colorTableOffset != pHeader->hashTableOffset + hashTableSize
        || pHeader->pixelIndexTableOffset != pHeader->colorTableOffset + colorTableSize
        || pHeader->dataOffset != pHeader->pixelIndexTableOffset + pixelIndexTableSize)
    {
        return 0;
    }

    return 1;
}

uint64_t
R_Pack_GetExpectedFileSize (const struct R_Pack_Header* pHeader)
{
    if (!R_Pack_ValidateHeader (pHeader))
    {
        return 0;
    }

    uint64_t hashTableSize = (uint64_t)pHeader->textureCount * sizeof (struct R_Pack_HashEntry);
    uint64_t colorTableSize = (uint64_t)pHeader->colorTableSize * sizeof (struct R_Pack_ColorEntry);
    uint64_t pixelIndexTableSize
        = (uint64_t)pHeader->pixelIndexTableSize * sizeof (struct R_Pack_PixelIndexEntry);
    uint64_t atlasDataSize = (uint64_t)pHeader->atlasWidth * pHeader->atlasHeight * 2;

    if (R_RPACK_HEADER_SIZE > UINT64_MAX - hashTableSize
        || R_RPACK_HEADER_SIZE + hashTableSize > UINT64_MAX - colorTableSize
        || R_RPACK_HEADER_SIZE + hashTableSize + colorTableSize > UINT64_MAX - pixelIndexTableSize
        || R_RPACK_HEADER_SIZE + hashTableSize + colorTableSize + pixelIndexTableSize
               > UINT64_MAX - atlasDataSize)
    {
        return 0;
    }

    return R_RPACK_HEADER_SIZE + hashTableSize + colorTableSize + pixelIndexTableSize + atlasDataSize;
}
