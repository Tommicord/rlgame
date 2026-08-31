#pragma once

#include "rpack/rpack_platform.h"
#include "rpack/rpack_format.h"

#include <stdint.h>
#include <stddef.h>

/**
 * @brief Decoder state
 */
struct r_pack_decoder
{
        struct r_pack_header*            pHeader;
        struct r_pack_hash_entry*        pHashTable;
        struct r_pack_color_entry*       pColorTable;
        struct r_pack_pixel_index_entry* pPixelIndexTable;
        const uint8_t*                   pData;
        uint64_t                         dataSize;
};

/**
 * @brief Create decoder instance from file data
 * @param pData Pointer to RPACK file data
 * @param dataSize Size of file data
 * @return Decoder instance or NULL on failure
 */
R_PACK_API struct r_pack_decoder* r_pack_new_decoder (const uint8_t* pData, uint64_t dataSize);

/**
 * @brief Destroy decoder instance
 * @param decoder Decoder instance to destroy
 */
R_PACK_API void r_pack_delete_decoder (struct r_pack_decoder* pDecoder);

/**
 * @brief Find texture by name
 * @param decoder Decoder instance
 * @param name Texture name
 * @return Hash entry or NULL if not found
 */
R_PACK_API const struct r_pack_hash_entry*
r_pack_decoder_find_texture (const struct r_pack_decoder* pDecoder, const char* pName);

/**
 * @brief Decode texture to RGBA buffer
 * @param decoder Decoder instance
 * @param name Texture name
 * @param outputBuffer Output RGBA buffer (allocated by caller)
 * @param outputBufferSize Output buffer size in bytes
 * @param bytesWritten Number of bytes written
 * @return R_PACK_OK on success, error code otherwise
 */
R_PACK_API enum r_pack_error r_pack_decoder_decode_texture (
    const struct r_pack_decoder* pDecoder,
    const char*                  pName,
    uint8_t*                     pOutputBuffer,
    uint64_t                     outputBufferSize,
    uint64_t*                    pBytesWritten);

/**
 * @brief Decode multiple textures to RGBA buffer
 * @param decoder Decoder instance
 * @param names Array of texture names
 * @param nameCount Number of texture names
 * @param outputBuffer Output RGBA buffer (allocated by caller)
 * @param outputBufferSize Output buffer size in bytes
 * @param bytesWritten Number of bytes written
 * @return R_PACK_OK on success, error code otherwise
 */
R_PACK_API enum r_pack_error r_pack_decoder_decode_textures (
    const struct r_pack_decoder* pDecoder,
    const char**                 pNames,
    uint32_t                     nameCount,
    uint8_t*                     pOutputBuffer,
    uint64_t                     outputBufferSize,
    uint64_t*                    pBytesWritten);

/**
 * @brief Get required buffer size for decoding texture
 * @param decoder Decoder instance
 * @param name Texture name
 * @return Required buffer size in bytes (width * height * 4)
 */
R_PACK_API uint64_t
r_pack_decoder_get_texture_size (const struct r_pack_decoder* pDecoder, const char* pName);

/**
 * @brief Get required buffer size for decoding multiple textures
 * @param decoder Decoder instance
 * @param names Array of texture names
 * @param nameCount Number of texture names
 * @return Required buffer size in bytes
 */
R_PACK_API uint64_t r_pack_decoder_get_textures_size (
    const struct r_pack_decoder* pDecoder,
    const char**                 pNames,
    uint32_t                     nameCount);

/**
 * @brief Get texture dimensions
 * @param decoder Decoder instance
 * @param name Texture name
 * @param width Output width
 * @param height Output height
 * @return R_PACK_OK on success, error code otherwise
 */
R_PACK_API enum r_pack_error r_pack_decoder_get_texture_dimensions (
    const struct r_pack_decoder* pDecoder,
    const char*                  pName,
    uint32_t*                    pWidth,
    uint32_t*                    pHeight);

/**
 * @brief Get number of textures in RPACK file
 * @param decoder Decoder instance
 * @return Number of textures
 */
R_PACK_API uint32_t r_pack_decoder_get_texture_count (const struct r_pack_decoder* pDecoder);

/**
 * @brief Validate RPACK file data
 * @param pData Pointer to file data
 * @param dataSize Size of file data
 * @return 1 if valid, 0 otherwise
 */
R_PACK_API int r_pack_decoder_validate_file (const uint8_t* pData, uint64_t dataSize);
