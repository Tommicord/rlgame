#pragma once

#include "rpack/rpack_platform.h"
#include "rpack/rpack_format.h"

#include <stdint.h>
#include <stddef.h>

/* Default configuration values */
#define R_RPACK_DEFAULT_MAX_ATLAS_WIDTH      4096
#define R_RPACK_DEFAULT_MAX_ATLAS_HEIGHT     4096
#define R_RPACK_DEFAULT_PADDING              1
#define R_RPACK_DEFAULT_BORDER               0
#define R_RPACK_DEFAULT_SIMILARITY_THRESHOLD 0.125f
#define R_RPACK_DEFAULT_ALPHA_THRESHOLD      0.0f
#define R_RPACK_DEFAULT_WORKER_COUNT         0
#define R_RPACK_DEFAULT_MAX_TEXTURES         0
#define R_RPACK_DEFAULT_POWER_OF_TWO         0
#define R_RPACK_DEFAULT_ENABLE_ROTATION      0

/**
 * @brief Input image data for encoding
 */
struct R_PackInputImage
{
                const uint8_t* pPixels; /**< Raw pixel data (RGBA format) */
                uint32_t       width; /**< Image width */
                uint32_t       height; /**< Image height */
                uint32_t       stride; /**< Bytes per row */
                const char*    pName; /**< Texture name for hashing */
};

/**
 * @brief Encoder configuration
 */
struct R_PackEncoderConfig
{
                uint32_t maxAtlasWidth; /**< Maximum atlas width (default: R_RPACK_DEFAULT_MAX_ATLAS_WIDTH) */
                uint32_t
                    maxAtlasHeight; /**< Maximum atlas height (default: R_RPACK_DEFAULT_MAX_ATLAS_HEIGHT) */
                uint32_t padding; /**< Padding between textures (default: R_RPACK_DEFAULT_PADDING) */
                uint32_t border; /**< Border size around textures (default: R_RPACK_DEFAULT_BORDER) */
                float    similarityThreshold; /**< Color similarity threshold (default:
                                              R_RPACK_DEFAULT_SIMILARITY_THRESHOLD) */
                float alphaThreshold; /**< Alpha threshold for transparency (default:
                                         R_RPACK_DEFAULT_ALPHA_THRESHOLD) */
                uint32_t workerCount; /**< Number of worker threads (default: R_RPACK_DEFAULT_WORKER_COUNT =
                                         auto-detect) */
                uint32_t maxTextures; /**< Maximum number of textures to pack (default:
                                         R_RPACK_DEFAULT_MAX_TEXTURES = unlimited) */
                uint8_t
                    powerOfTwo; /**< Force power of two dimensions (default: R_RPACK_DEFAULT_POWER_OF_TWO) */
                uint8_t
                    enableRotation; /**< Enable texture rotation (default: R_RPACK_DEFAULT_ENABLE_ROTATION) */
};

/**
 * @brief Encoder state
 */
struct R_PackEncoder
{
                struct R_PackEncoderConfig    config;
                struct R_PackHeader*          pHeader;
                struct R_PackHashEntry*       pHashTable;
                struct R_PackColorEntry*      pColorTable;
                struct R_PackPixelIndexEntry* pPixelIndexTable;
                uint32_t                      colorTableCapacity;
                uint32_t                      colorTableCount;
                uint32_t                      pixelIndexTableCapacity;
                uint32_t                      pixelIndexTableCount;
                uint8_t*                      pAtlasData;
                uint64_t                      atlasDataSize;
                struct R_CSTL_Mutex*          pMutex;
                struct R_CSTL_Thread**        ppWorkerThreads;
                uint32_t                      actualWorkerCount;
                volatile int                  workersActive;
};

/**
 * @brief Create encoder instance
 * @param config Encoder configuration (NULL for defaults)
 * @return Encoder instance or NULL on failure
 */
R_RPACK_API struct R_PackEncoder* R_Pack_NewEncoder (const struct R_PackEncoderConfig* pConfig);

/**
 * @brief Destroy encoder instance
 * @param encoder Encoder instance to destroy
 */
R_RPACK_API void R_Pack_DeleteEncoder (struct R_PackEncoder* pEncoder);

/**
 * @brief Add image to encoder
 * @param encoder Encoder instance
 * @param image Input image data
 * @return R_RPACK_OK on success, error code otherwise
 */
R_RPACK_API enum R_PackError
R_Pack_EncoderAddImage (struct R_PackEncoder* pEncoder, const struct R_PackInputImage* pImage);

/**
 * @brief Encode all added images to RPACK format
 * @param encoder Encoder instance
 * @param outputBuffer Output buffer (allocated by caller)
 * @param outputBufferSize Output buffer size
 * @param bytesWritten Number of bytes written to output
 * @return R_RPACK_OK on success, error code otherwise
 */
R_RPACK_API enum R_PackError R_Pack_EncoderEncode (
    struct R_PackEncoder* pEncoder,
    uint8_t*              pOutputBuffer,
    uint64_t              outputBufferSize,
    uint64_t*             pBytesWritten);

/**
 * @brief Get required output buffer size for encoding
 * @param encoder Encoder instance
 * @return Required buffer size in bytes
 */
R_RPACK_API uint64_t R_Pack_EncoderGetRequiredSize (const struct R_PackEncoder* pEncoder);

/**
 * @brief Get number of images added to encoder
 * @param encoder Encoder instance
 * @return Number of images
 */
R_RPACK_API uint32_t R_Pack_EncoderGetImageCount (const struct R_PackEncoder* pEncoder);

/**
 * @brief Convert RGBA to YUV-like encoding
 * @param rgba RGBA color (8 bits per channel)
 * @param y Output luminance (8 bits)
 * @param yExp Output luminance exponent (8 bits)
 * @param u Output chrominance U (4 bits)
 * @param v Output chrominance V (4 bits)
 */
R_RPACK_API void
R_Pack_RGBAToYUV (uint8_t r, uint8_t g, uint8_t b, uint8_t* pY, uint8_t* pYExp, uint8_t* pU, uint8_t* pV);

/**
 * @brief Convert YUV-like encoding to RGBA
 * @param y Luminance (8 bits)
 * @param yExp Luminance exponent (8 bits)
 * @param u Chrominance U (4 bits)
 * @param v Chrominance V (4 bits)
 * @param r Output red (8 bits)
 * @param g Output green (8 bits)
 * @param b Output blue (8 bits)
 */
R_RPACK_API void
R_Pack_YUVToRGBA (uint8_t y, uint8_t yExp, uint8_t u, uint8_t v, uint8_t* pR, uint8_t* pG, uint8_t* pB);

/**
 * @brief Get color similarity (rotation-based)
 * @param y1 First luminance
 * @param u1 First chrominance U
 * @param v1 First chrominance V
 * @param y2 Second luminance
 * @param u2 Second chrominance U
 * @param v2 Second chrominance V
 * @return Similarity score (0.0 = identical, higher = more different)
 */
R_RPACK_API float
R_Pack_GetColorSimilarity (uint8_t y1, uint8_t u1, uint8_t v1, uint8_t y2, uint8_t u2, uint8_t v2);
