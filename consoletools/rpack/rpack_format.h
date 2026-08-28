#pragma once

#include <stdint.h>
#include <stddef.h>

#define R_RPACK_MAGIC       0x5250434B // RPCK
#define R_RPACK_VERSION     1
#define R_RPACK_HEADER_SIZE 64
#pragma pack(push, 1)

/**
 * @brief RPACK file header
 */
struct R_PackHeader
{
        union
        {
                char     magic[4]; /**< Magic number "RPCK" */
                uint32_t magicInt32;
        };
        uint32_t version; /**< Format version */
        uint32_t textureCount; /**< Number of textures in atlas */
        uint32_t atlasWidth; /**< Atlas width in pixels */
        uint32_t atlasHeight; /**< Atlas height in pixels */
        uint64_t hashTableOffset; /**< Offset to hash table from file start */
        uint64_t dataOffset; /**< Offset to texture data from file start */
        uint64_t colorTableOffset; /**< Offset to color table from file start */
        uint64_t pixelIndexTableOffset; /**< Offset to pixel index table from file start */
        uint32_t colorTableSize; /**< Number of entries in color table */
        uint32_t pixelIndexTableSize; /**< Number of entries in pixel index table */
        uint32_t reserved[6]; /**< Reserved for future use */
};

/**
 * @brief Hash table entry for texture lookup
 */
struct R_PackHashEntry
{
        uint64_t nameHash; /**< xxHash64 of texture name */
        uint32_t atlasOffsetX; /**< X offset in atlas */
        uint32_t atlasOffsetY; /**< Y offset in atlas */
        uint32_t width; /**< Texture width */
        uint32_t height; /**< Texture height */
        uint32_t colorTableIndex; /**< Index into color table */
        uint32_t pixelIndexTableOffset; /**< Offset to pixel index table for this texture */
};

/**
 * @brief Color table entry (YUV-like encoding)
 */
struct R_PackColorEntry
{
        uint8_t luminance; /**< Luminance (Y) 8 bits */
        uint8_t luminanceExp; /**< Luminance exponent 8 bits */
        uint8_t chrominanceU; /**< Chrominance U 4 bits (packed) */
        uint8_t chrominanceV; /**< Chrominance V 4 bits (packed) */
};

/**
 * @brief Pixel index table entry for run-length encoding
 */
struct R_PackPixelIndexEntry
{
        uint8_t  exponent; /**< Exponent multiplier (8 bits) */
        uint16_t colorIndex; /**< Index into color table (12 bits) */
        uint8_t  runWidth; /**< Run width (6 bits) */
        uint8_t  runHeight; /**< Run height (6 bits) */
};
#pragma pack(pop)

/**
 * @brief Compute xxHash64 of a string
 * @param data Input data
 * @param length Length of data
 * @param seed Seed value (use 0 for default)
 * @return 64-bit hash value
 */
uint64_t R_Pack_Hash64 (const void* pData, size_t length, uint64_t seed);

/**
 * @brief Compute xxHash64 of a null-terminated string
 * @param str Null-terminated string
 * @param seed Seed value (use 0 for default)
 * @return 64-bit hash value
 */
uint64_t R_Pack_Hash64String (const char* pStr, uint64_t seed);

/**
 * @brief Validate RPACK header
 * @param header Pointer to header to validate
 * @return 1 if valid, 0 otherwise
 */
int R_Pack_ValidateHeader (const struct R_PackHeader* pHeader);

/**
 * @brief Get total expected file size from header
 * @param header Pointer to valid header
 * @return Expected file size in bytes
 */
uint64_t R_Pack_GetExpectedFileSize (const struct R_PackHeader* pHeader);
