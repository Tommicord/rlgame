#pragma once

#include "rpack/rpack_platform.h"
#include "rpack/rpack_format.h"

#include <stdint.h>
#include <stddef.h>

/**
 * @brief Decoder state
 */
struct R_Pack_Decoder
{
        struct R_Pack_Header*          pHeader;
        struct R_Pack_HashEntry*       pHashTable;
        struct R_Pack_ColorEntry*      pColorTable;
        struct R_Pack_PixelIndexEntry* pPixelIndexTable;
        const uint8_t*                pData;
        uint64_t                      dataSize;
};

/**
 * @brief Create decoder instance from file data
 * @param pData Pointer to RPACK file data
 * @param dataSize Size of file data
 * @return Decoder instance or NULL on failure
 */
R_PACK_API struct R_Pack_Decoder* R_Pack_NewDecoder (const uint8_t* pData, uint64_t dataSize);

/**
 * @brief Destroy decoder instance
 * @param decoder Decoder instance to destroy
 */
R_PACK_API void R_Pack_DeleteDecoder (struct R_Pack_Decoder* pDecoder);

/**
 * @brief Find texture by name
 * @param decoder Decoder instance
 * @param name Texture name
 * @return Hash entry or NULL if not found
 */
R_PACK_API const struct R_Pack_HashEntry*
R_Pack_DecoderFindTexture (const struct R_Pack_Decoder* pDecoder, const char* pName);

/**
 * @brief Decode texture to RGBA buffer
 * @param decoder Decoder instance
 * @param name Texture name
 * @param outputBuffer Output RGBA buffer (allocated by caller)
 * @param outputBufferSize Output buffer size in bytes
 * @param bytesWritten Number of bytes written
 * @return R_PACK_OK on success, error code otherwise
 */
R_PACK_API enum R_Pack_Error R_Pack_DecoderDecodeTexture (
    const struct R_Pack_Decoder* pDecoder,
    const char*                 pName,
    uint8_t*                    pOutputBuffer,
    uint64_t                    outputBufferSize,
    uint64_t*                   pBytesWritten);

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
R_PACK_API enum R_Pack_Error R_Pack_DecoderDecodeTextures (
    const struct R_Pack_Decoder* pDecoder,
    const char**                pNames,
    uint32_t                    nameCount,
    uint8_t*                    pOutputBuffer,
    uint64_t                    outputBufferSize,
    uint64_t*                   pBytesWritten);

/**
 * @brief Get required buffer size for decoding texture
 * @param decoder Decoder instance
 * @param name Texture name
 * @return Required buffer size in bytes (width * height * 4)
 */
R_PACK_API uint64_t R_Pack_DecoderGetTextureSize (const struct R_Pack_Decoder* pDecoder, const char* pName);

/**
 * @brief Get required buffer size for decoding multiple textures
 * @param decoder Decoder instance
 * @param names Array of texture names
 * @param nameCount Number of texture names
 * @return Required buffer size in bytes
 */
R_PACK_API uint64_t
R_Pack_DecoderGetTexturesSize (const struct R_Pack_Decoder* pDecoder, const char** pNames, uint32_t nameCount);

/**
 * @brief Get texture dimensions
 * @param decoder Decoder instance
 * @param name Texture name
 * @param width Output width
 * @param height Output height
 * @return R_PACK_OK on success, error code otherwise
 */
R_PACK_API enum R_Pack_Error R_Pack_DecoderGetTextureDimensions (
    const struct R_Pack_Decoder* pDecoder,
    const char*                 pName,
    uint32_t*                   pWidth,
    uint32_t*                   pHeight);

/**
 * @brief Get number of textures in RPACK file
 * @param decoder Decoder instance
 * @return Number of textures
 */
R_PACK_API uint32_t R_Pack_DecoderGetTextureCount (const struct R_Pack_Decoder* pDecoder);

/**
 * @brief Validate RPACK file data
 * @param pData Pointer to file data
 * @param dataSize Size of file data
 * @return 1 if valid, 0 otherwise
 */
R_PACK_API int R_Pack_DecoderValidateFile (const uint8_t* pData, uint64_t dataSize);
