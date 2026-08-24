#include "rpack/rpack_encoder.h"

#include "rlgame.base/cstl/cstl_heap_allocator.h"
#include "rlgame.base/cstl/cstl_log.h"
#include "rlgame.base/cstl/cstl_string.h"
#include "rlgame.base/cstl/cstl_thread.h"
#include <string.h>
#include <math.h>

#define R_RPACK_DEFAULT_ATLAS_WIDTH 4096
#define R_RPACK_DEFAULT_ATLAS_HEIGHT 4096
#define R_RPACK_DEFAULT_PADDING 1
#define R_RPACK_DEFAULT_SIMILARITY_THRESHOLD 0.1f
#define R_RPACK_INITIAL_COLOR_TABLE_SIZE 4096
#define R_RPACK_INITIAL_PIXEL_INDEX_TABLE_SIZE 65536
#define R_RPACK_DEFAULT_WORKER_COUNT 0

struct R_RPackPixelWorkTask
{
        struct R_RPackEncoder* pEncoder;
        const struct R_RPackInputImage* pImage;
        uint32_t startY;
        uint32_t endY;
        uint32_t* pOutPixelCount;
        int* pOutError;
};

static void
R_RPack_ProcessPixelRowWorker (void* pData)
{
        struct R_RPackPixelWorkTask* pTask = (struct R_RPackPixelWorkTask*)pData;
        uint32_t pixelCount = 0;

        for (uint32_t y = pTask->startY; y < pTask->endY; ++y)
        {
                for (uint32_t x = 0; x < pTask->pImage->width; ++x)
                {
                        uint32_t pixelIndex = y * pTask->pImage->stride + x * 4;
                        uint8_t r = pTask->pImage->pPixels[pixelIndex];
                        uint8_t g = pTask->pImage->pPixels[pixelIndex + 1];
                        uint8_t b = pTask->pImage->pPixels[pixelIndex + 2];
                        uint8_t a = pTask->pImage->pPixels[pixelIndex + 3];

                        uint8_t yuvY, yuvYExp, yuvU, yuvV;
                        R_RPack_RGBAToYUV (r, g, b, &yuvY, &yuvYExp, &yuvU, &yuvV);

                        uint32_t colorIndex = R_RPack_FindOrAddColor (pTask->pEncoder, yuvY, yuvYExp, yuvU, yuvV);
                        if (colorIndex == UINT32_MAX)
                        {
                                *pTask->pOutError = 1;
                                return;
                        }

                        R_CSTL_MutexLock (pTask->pEncoder->pMutex);
                        if (pTask->pEncoder->pixelIndexTableCount >= pTask->pEncoder->pixelIndexTableCapacity)
                        {
                                uint32_t newCapacity = pTask->pEncoder->pixelIndexTableCapacity * 2;
                                struct R_RPackPixelIndexEntry* pNewTable =
                                    (struct R_RPackPixelIndexEntry*)R_CSTL_HeapRealloc (
                                        pTask->pEncoder->pPixelIndexTable,
                                        newCapacity * sizeof (struct R_RPackPixelIndexEntry));
                                if (!pNewTable)
                                {
                                        R_CSTL_MutexUnlock (pTask->pEncoder->pMutex);
                                        *pTask->pOutError = 1;
                                        return;
                                }
                                pTask->pEncoder->pPixelIndexTable = pNewTable;
                                pTask->pEncoder->pixelIndexTableCapacity = newCapacity;
                        }
                        R_CSTL_MutexUnlock (pTask->pEncoder->pMutex);

                        struct R_RPackPixelIndexEntry* pPixelEntry =
                            &pTask->pEncoder->pPixelIndexTable[pTask->pEncoder->pixelIndexTableCount];
                        pPixelEntry->colorIndex = (uint16_t)colorIndex;
                        pPixelEntry->exponent = 1;
                        pPixelEntry->runWidth = 1;
                        pPixelEntry->runHeight = 1;

                        R_CSTL_MutexLock (pTask->pEncoder->pMutex);
                        pTask->pEncoder->pixelIndexTableCount++;
                        R_CSTL_MutexUnlock (pTask->pEncoder->pMutex);
                        pixelCount++;
                }
        }

        *pTask->pOutPixelCount = pixelCount;
}

struct R_RPackEncoder*
R_RPack_NewEncoder (const struct R_RPackEncoderConfig* pConfig)
{
        struct R_RPackEncoder* pEncoder = (struct R_RPackEncoder*)R_CSTL_HeapAlloc (sizeof (struct R_RPackEncoder));
        if (!pEncoder)
        {
                return NULL;
        }

        memset (pEncoder, 0, sizeof (struct R_RPackEncoder));

        if (pConfig)
        {
                pEncoder->config = *pConfig;
        }
        else
        {
                pEncoder->config.maxAtlasWidth = R_RPACK_DEFAULT_ATLAS_WIDTH;
                pEncoder->config.maxAtlasHeight = R_RPACK_DEFAULT_ATLAS_HEIGHT;
                pEncoder->config.padding = R_RPACK_DEFAULT_PADDING;
                pEncoder->config.similarityThreshold = R_RPACK_DEFAULT_SIMILARITY_THRESHOLD;
                pEncoder->config.workerCount = R_RPACK_DEFAULT_WORKER_COUNT;
        }

        if (pEncoder->config.workerCount == 0)
        {
                pEncoder->actualWorkerCount = 1; // Default to 1 for now, can add CPU detection later
        }
        else
        {
                pEncoder->actualWorkerCount = pEncoder->config.workerCount;
        }

        // Create mutex for thread synchronization
        pEncoder->pMutex = R_CSTL_NewMutex ();
        if (!pEncoder->pMutex)
        {
                R_CSTL_HeapFree (pEncoder);
                return NULL;
        }

        pEncoder->workersActive = 0;
        pEncoder->ppWorkerThreads = NULL;

        pEncoder->pHeader = (struct R_RPackHeader*)R_CSTL_HeapAlloc (sizeof (struct R_RPackHeader));
        if (!pEncoder->pHeader)
        {
                R_CSTL_HeapFree (pEncoder);
                return NULL;
        }
        memset (pEncoder->pHeader, 0, sizeof (struct R_RPackHeader));
        pEncoder->pHeader->magicInt32 = R_RPACK_MAGIC;
        pEncoder->pHeader->version = R_RPACK_VERSION;

        pEncoder->colorTableCapacity = R_RPACK_INITIAL_COLOR_TABLE_SIZE;
        pEncoder->pColorTable = (struct R_RPackColorEntry*)R_CSTL_HeapAlloc (
            pEncoder->colorTableCapacity * sizeof (struct R_RPackColorEntry));
        if (!pEncoder->pColorTable)
        {
                R_CSTL_HeapFree (pEncoder->pHeader);
                R_CSTL_HeapFree (pEncoder);
                return NULL;
        }

        pEncoder->pixelIndexTableCapacity = R_RPACK_INITIAL_PIXEL_INDEX_TABLE_SIZE;
        pEncoder->pPixelIndexTable = (struct R_RPackPixelIndexEntry*)R_CSTL_HeapAlloc (
            pEncoder->pixelIndexTableCapacity * sizeof (struct R_RPackPixelIndexEntry));
        if (!pEncoder->pPixelIndexTable)
        {
                R_CSTL_HeapFree (pEncoder->pColorTable);
                R_CSTL_HeapFree (pEncoder->pHeader);
                R_CSTL_HeapFree (pEncoder);
                return NULL;
        }

        return pEncoder;
}

void
R_RPack_DeleteEncoder (struct R_RPackEncoder* pEncoder)
{
        if (!pEncoder)
        {
                return;
        }
        if (pEncoder->ppWorkerThreads)
        {
                for (uint32_t i = 0; i < pEncoder->actualWorkerCount; ++i)
                {
                        if (pEncoder->ppWorkerThreads[i])
                        {
                                R_CSTL_ThreadJoin (pEncoder->ppWorkerThreads[i]);
                        }
                }
                R_CSTL_HeapFree (pEncoder->ppWorkerThreads);
        }

        if (pEncoder->pMutex)
        {
                R_CSTL_MutexDestroy (pEncoder->pMutex);
        }

        if (pEncoder->pHeader)
        {
                R_CSTL_HeapFree (pEncoder->pHeader);
        }
        if (pEncoder->pHashTable)
        {
                R_CSTL_HeapFree (pEncoder->pHashTable);
        }
        if (pEncoder->pColorTable)
        {
                R_CSTL_HeapFree (pEncoder->pColorTable);
        }
        if (pEncoder->pPixelIndexTable)
        {
                R_CSTL_HeapFree (pEncoder->pPixelIndexTable);
        }
        if (pEncoder->pAtlasData)
        {
                R_CSTL_HeapFree (pEncoder->pAtlasData);
        }

        R_CSTL_HeapFree (pEncoder);
}

void
R_RPack_RGBAToYUV (uint8_t r, uint8_t g, uint8_t b, uint8_t* pY, uint8_t* pYExp, uint8_t* pU, uint8_t* pV)
{
        float rf = (float)r / 255.0f;
        float gf = (float)g / 255.0f;
        float bf = (float)b / 255.0f;

        float yf = 0.299f * rf + 0.587f * gf + 0.114f * bf;
        float uf = -0.14713f * rf - 0.28886f * gf + 0.436f * bf;
        float vf = 0.615f * rf - 0.51499f * gf - 0.10001f * bf;

        if (yf <= 0.0f)
        {
                *pY = 0;
                *pYExp = 0;
        }
        else
        {
                int exp = 0;
                float mantissa = frexp (yf, &exp);
                *pY = (uint8_t)(mantissa * 255.0f);
                *pYExp = (uint8_t)(exp + 127);
        }

        *pU = (uint8_t)((uf + 0.5f) * 15.0f);
        *pV = (uint8_t)((vf + 0.5f) * 15.0f);
}

void
R_RPack_YUVToRGBA (uint8_t y, uint8_t yExp, uint8_t u, uint8_t v, uint8_t* pR, uint8_t* pG, uint8_t* pB)
{
        float yf = ldexp ((float)y / 255.0f, (int)yExp - 127);
        float uf = ((float)u / 15.0f) - 0.5f;
        float vf = ((float)v / 15.0f) - 0.5f;

        float rf = yf + 1.13983f * vf;
        float gf = yf - 0.39465f * uf - 0.58060f * vf;
        float bf = yf + 2.03211f * uf;

        *pR = (uint8_t)(fmaxf (0.0f, fminf (1.0f, rf)) * 255.0f);
        *pG = (uint8_t)(fmaxf (0.0f, fminf (1.0f, gf)) * 255.0f);
        *pB = (uint8_t)(fmaxf (0.0f, fminf (1.0f, bf)) * 255.0f);
}

float
R_RPack_GetColorSimilarity (
    uint8_t y1,
    uint8_t u1,
    uint8_t v1,
    uint8_t y2,
    uint8_t u2,
    uint8_t v2)
{
        float dy = (float)(y1 - y2) / 255.0f;
        float du = (float)(u1 - u2) / 15.0f;
        float dv = (float)(v1 - v2) / 15.0f;
        return sqrtf (dy * dy + du * du + dv * dv);
}

static uint32_t
R_RPack_FindOrAddColor (
    struct R_RPackEncoder* pEncoder,
    uint8_t y,
    uint8_t yExp,
    uint8_t u,
    uint8_t v)
{
        R_CSTL_MutexLock (pEncoder->pMutex);

        for (uint32_t i = 0; i < pEncoder->colorTableCount; ++i)
        {
                struct R_RPackColorEntry* pEntry = &pEncoder->pColorTable[i];
                float similarity = R_RPack_GetColorSimilarity (
                    y,
                    u,
                    v,
                    pEntry->luminance,
                    pEntry->chrominanceU,
                    pEntry->chrominanceV);
                if (similarity < pEncoder->config.similarityThreshold)
                {
                        R_CSTL_MutexUnlock (pEncoder->pMutex);
                        return i;
                }
        }

        if (pEncoder->colorTableCount >= pEncoder->colorTableCapacity)
        {
                uint32_t newCapacity = pEncoder->colorTableCapacity * 2;
                struct R_RPackColorEntry* pNewTable = (struct R_RPackColorEntry*)R_CSTL_HeapRealloc (
                    pEncoder->pColorTable,
                    newCapacity * sizeof (struct R_RPackColorEntry));
                if (!pNewTable)
                {
                        R_CSTL_MutexUnlock (pEncoder->pMutex);
                        return UINT32_MAX;
                }
                pEncoder->pColorTable = pNewTable;
                pEncoder->colorTableCapacity = newCapacity;
        }

        struct R_RPackColorEntry* pEntry = &pEncoder->pColorTable[pEncoder->colorTableCount];
        pEntry->luminance = y;
        pEntry->luminanceExp = yExp;
        pEntry->chrominanceU = u;
        pEntry->chrominanceV = v;

        uint32_t colorIndex = pEncoder->colorTableCount++;
        R_CSTL_MutexUnlock (pEncoder->pMutex);
        return colorIndex;
}

static enum R_RPackError
R_RPack_ValidateInputImage (const struct R_RPackInputImage* pImage)
{
        if (!pImage || !pImage->pPixels || !pImage->pName)
        {
                return R_RPACK_ERROR_INVALID_ARGUMENT;
        }

        if (pImage->width == 0 || pImage->height == 0)
        {
                return R_RPACK_ERROR_INVALID_DIMENSIONS;
        }

        return R_RPACK_OK;
}

static enum R_RPackError
R_RPack_ExpandHashTable (struct R_RPackEncoder* pEncoder, uint32_t* pOutTextureIndex)
{
        R_CSTL_MutexLock (pEncoder->pMutex);
        uint32_t textureIndex = pEncoder->pHeader->textureCount;

        if (textureIndex == 0)
        {
                pEncoder->pHashTable = (struct R_RPackHashEntry*)R_CSTL_HeapAlloc (
                    sizeof (struct R_RPackHashEntry));
                if (!pEncoder->pHashTable)
                {
                        R_CSTL_MutexUnlock (pEncoder->pMutex);
                        return R_RPACK_ERROR_OUT_OF_MEMORY;
                }
        }
        else
        {
                struct R_RPackHashEntry* pNewTable = (struct R_RPackHashEntry*)R_CSTL_HeapRealloc (
                    pEncoder->pHashTable,
                    (textureIndex + 1) * sizeof (struct R_RPackHashEntry));
                if (!pNewTable)
                {
                        R_CSTL_MutexUnlock (pEncoder->pMutex);
                        return R_RPACK_ERROR_OUT_OF_MEMORY;
                }
                pEncoder->pHashTable = pNewTable;
        }

        *pOutTextureIndex = textureIndex;
        R_CSTL_MutexUnlock (pEncoder->pMutex);
        return R_RPACK_OK;
}

static void
R_RPack_GetAtlasPosition (
    struct R_RPackEncoder*       pEncoder,
    const struct R_RPackInputImage* pImage,
    uint32_t                     textureIndex,
    uint32_t*                    pOutX,
    uint32_t*                    pOutY)
{
        uint32_t currentX = 0;
        uint32_t currentY = 0;

        if (textureIndex > 0)
        {
                struct R_RPackHashEntry* pPrevEntry = &pEncoder->pHashTable[textureIndex - 1];
                currentX = pPrevEntry->atlasOffsetX + pPrevEntry->width + pEncoder->config.padding;
                if (currentX + pImage->width > pEncoder->config.maxAtlasWidth)
                {
                        currentX = 0;
                        currentY = pPrevEntry->atlasOffsetY + pPrevEntry->height + pEncoder->config.padding;
                }
                else
                {
                        currentY = pPrevEntry->atlasOffsetY;
                }
        }

        *pOutX = currentX;
        *pOutY = currentY;
}

static void
R_RPack_UpdateAtlasDimensions (
    struct R_RPackEncoder* pEncoder,
    uint32_t               currentX,
    uint32_t               currentY,
    uint32_t               imageWidth,
    uint32_t               imageHeight)
{
        R_CSTL_MutexLock (pEncoder->pMutex);
        uint32_t atlasWidth = currentX + imageWidth;
        uint32_t atlasHeight = currentY + imageHeight;
        if (atlasWidth > pEncoder->pHeader->atlasWidth)
        {
                pEncoder->pHeader->atlasWidth = atlasWidth;
        }
        if (atlasHeight > pEncoder->pHeader->atlasHeight)
        {
                pEncoder->pHeader->atlasHeight = atlasHeight;
        }
        R_CSTL_MutexUnlock (pEncoder->pMutex);
}

static enum R_RPackError
R_RPack_ProcessPixelsSerial (
    struct R_RPackEncoder*       pEncoder,
    const struct R_RPackInputImage* pImage)
{
        for (uint32_t y = 0; y < pImage->height; ++y)
        {
                for (uint32_t x = 0; x < pImage->width; ++x)
                {
                        uint32_t pixelIndex = y * pImage->stride + x * 4;
                        uint8_t r = pImage->pPixels[pixelIndex];
                        uint8_t g = pImage->pPixels[pixelIndex + 1];
                        uint8_t b = pImage->pPixels[pixelIndex + 2];
                        uint8_t a = pImage->pPixels[pixelIndex + 3];

                        uint8_t yuvY, yuvYExp, yuvU, yuvV;
                        R_RPack_RGBAToYUV (r, g, b, &yuvY, &yuvYExp, &yuvU, &yuvV);

                        uint32_t colorIndex = R_RPack_FindOrAddColor (pEncoder, yuvY, yuvYExp, yuvU, yuvV);
                        if (colorIndex == UINT32_MAX)
                        {
                                return R_RPACK_ERROR_OUT_OF_MEMORY;
                        }

                        R_CSTL_MutexLock (pEncoder->pMutex);
                        if (pEncoder->pixelIndexTableCount >= pEncoder->pixelIndexTableCapacity)
                        {
                                uint32_t newCapacity = pEncoder->pixelIndexTableCapacity * 2;
                                struct R_RPackPixelIndexEntry* pNewTable =
                                    (struct R_RPackPixelIndexEntry*)R_CSTL_HeapRealloc (
                                        pEncoder->pPixelIndexTable,
                                        newCapacity * sizeof (struct R_RPackPixelIndexEntry));
                                if (!pNewTable)
                                {
                                        R_CSTL_MutexUnlock (pEncoder->pMutex);
                                        return R_RPACK_ERROR_OUT_OF_MEMORY;
                                }
                                pEncoder->pPixelIndexTable = pNewTable;
                                pEncoder->pixelIndexTableCapacity = newCapacity;
                        }
                        R_CSTL_MutexUnlock (pEncoder->pMutex);

                        struct R_RPackPixelIndexEntry* pPixelEntry =
                            &pEncoder->pPixelIndexTable[pEncoder->pixelIndexTableCount];
                        pPixelEntry->colorIndex = (uint16_t)colorIndex;
                        pPixelEntry->exponent = 1;
                        pPixelEntry->runWidth = 1;
                        pPixelEntry->runHeight = 1;

                        R_CSTL_MutexLock (pEncoder->pMutex);
                        pEncoder->pixelIndexTableCount++;
                        R_CSTL_MutexUnlock (pEncoder->pMutex);
                }
        }
        return R_RPACK_OK;
}

static enum R_RPackError
R_RPack_ProcessPixelsParallel (
    struct R_RPackEncoder*       pEncoder,
    const struct R_RPackInputImage* pImage)
{
        uint32_t rowsPerWorker = pImage->height / pEncoder->actualWorkerCount;
        struct R_RPackPixelWorkTask* pTasks = (struct R_RPackPixelWorkTask*)R_CSTL_HeapAlloc (
            pEncoder->actualWorkerCount * sizeof (struct R_RPackPixelWorkTask));
        uint32_t* pPixelCounts = (uint32_t*)R_CSTL_HeapAlloc (
            pEncoder->actualWorkerCount * sizeof (uint32_t));
        int* pErrors = (int*)R_CSTL_HeapAlloc (pEncoder->actualWorkerCount * sizeof (int));

        if (!pTasks || !pPixelCounts || !pErrors)
        {
                if (pTasks) R_CSTL_HeapFree (pTasks);
                if (pPixelCounts) R_CSTL_HeapFree (pPixelCounts);
                if (pErrors) R_CSTL_HeapFree (pErrors);
                return R_RPACK_ERROR_OUT_OF_MEMORY;
        }

        memset (pErrors, 0, pEncoder->actualWorkerCount * sizeof (int));

        for (uint32_t i = 0; i < pEncoder->actualWorkerCount; ++i)
        {
                pTasks[i].pEncoder = pEncoder;
                pTasks[i].pImage = pImage;
                pTasks[i].startY = i * rowsPerWorker;
                pTasks[i].endY = (i == pEncoder->actualWorkerCount - 1) ? pImage->height
                                                                      : (i + 1) * rowsPerWorker;
                pTasks[i].pOutPixelCount = &pPixelCounts[i];
                pTasks[i].pOutError = &pErrors[i];
        }

        for (uint32_t i = 0; i < pEncoder->actualWorkerCount; ++i)
        {
                struct R_CSTL_Thread* pThread =
                    R_CSTL_NewThread (R_RPack_ProcessPixelRowWorker, &pTasks[i]);
                if (!pThread)
                {
                        for (uint32_t j = i; j < pEncoder->actualWorkerCount; ++j)
                        {
                                R_RPack_ProcessPixelRowWorker (&pTasks[j]);
                        }
                        break;
                }
                R_CSTL_ThreadJoin (pThread);
        }

        for (uint32_t i = 0; i < pEncoder->actualWorkerCount; ++i)
        {
                if (pErrors[i])
                {
                        R_CSTL_HeapFree (pTasks);
                        R_CSTL_HeapFree (pPixelCounts);
                        R_CSTL_HeapFree (pErrors);
                        return R_RPACK_ERROR_OUT_OF_MEMORY;
                }
        }

        R_CSTL_HeapFree (pTasks);
        R_CSTL_HeapFree (pPixelCounts);
        R_CSTL_HeapFree (pErrors);
        return R_RPACK_OK;
}

static enum R_RPackError
R_RPack_ProcessImagePixels (
    struct R_RPackEncoder*       pEncoder,
    const struct R_RPackInputImage* pImage)
{
        if (pEncoder->actualWorkerCount > 1 && pImage->height > 100)
        {
                return R_RPack_ProcessPixelsParallel (pEncoder, pImage);
        }
        else
        {
                return R_RPack_ProcessPixelsSerial (pEncoder, pImage);
        }
}

enum R_RPackError
R_RPack_EncoderAddImage (
    struct R_RPackEncoder*       pEncoder,
    const struct R_RPackInputImage* pImage)
{
        enum R_RPackError error = R_RPack_ValidateInputImage (pImage);
        if (error != R_RPACK_OK)
        {
                return error;
        }

        uint32_t textureIndex;
        error = R_RPack_ExpandHashTable (pEncoder, &textureIndex);
        if (error != R_RPACK_OK)
        {
                return error;
        }

        struct R_RPackHashEntry* pEntry = &pEncoder->pHashTable[textureIndex];
        pEntry->nameHash = R_RPack_Hash64String (pImage->pName, 0);
        pEntry->width = pImage->width;
        pEntry->height = pImage->height;
        pEntry->colorTableIndex = pEncoder->colorTableCount;
        pEntry->pixelIndexTableOffset = pEncoder->pixelIndexTableCount;

        uint32_t currentX, currentY;
        R_RPack_GetAtlasPosition (pEncoder, pImage, textureIndex, &currentX, &currentY);

        pEntry->atlasOffsetX = currentX;
        pEntry->atlasOffsetY = currentY;

        R_RPack_UpdateAtlasDimensions (pEncoder, currentX, currentY, pImage->width, pImage->height);

        error = R_RPack_ProcessImagePixels (pEncoder, pImage);
        if (error != R_RPACK_OK)
        {
                return error;
        }

        R_CSTL_MutexLock (pEncoder->pMutex);
        pEncoder->pHeader->textureCount++;
        R_CSTL_MutexUnlock (pEncoder->pMutex);
        return R_RPACK_OK;
}

uint64_t
R_RPack_EncoderGetRequiredSize (const struct R_RPackEncoder* pEncoder)
{
        if (!pEncoder || !pEncoder->pHeader)
        {
                return 0;
        }

        uint64_t hashTableSize = (uint64_t)pEncoder->pHeader->textureCount * sizeof (struct R_RPackHashEntry);
        uint64_t colorTableSize = (uint64_t)pEncoder->colorTableCount * sizeof (struct R_RPackColorEntry);
        uint64_t pixelIndexTableSize =
            (uint64_t)pEncoder->pixelIndexTableCount * sizeof (struct R_RPackPixelIndexEntry);
        uint64_t atlasDataSize =
            (uint64_t)pEncoder->pHeader->atlasWidth * pEncoder->pHeader->atlasHeight * 2;

        return R_RPACK_HEADER_SIZE + hashTableSize + colorTableSize + pixelIndexTableSize + atlasDataSize;
}

enum R_RPackError
R_RPack_EncoderEncode (
    struct R_RPackEncoder* pEncoder,
    uint8_t*               pOutputBuffer,
    uint64_t               outputBufferSize,
    uint64_t*              pBytesWritten)
{
        if (!pEncoder || !pEncoder->pHeader || !pOutputBuffer)
        {
                return R_RPACK_ERROR_INVALID_ARGUMENT;
        }

        uint64_t requiredSize = R_RPack_EncoderGetRequiredSize (pEncoder);
        if (outputBufferSize < requiredSize)
        {
                return R_RPACK_ERROR_BUFFER_TOO_SMALL;
        }

        uint64_t offset = 0;

        memcpy (pOutputBuffer + offset, pEncoder->pHeader, sizeof (struct R_RPackHeader));
        offset += sizeof (struct R_RPackHeader);

        pEncoder->pHeader->hashTableOffset = offset;
        if (pEncoder->pHeader->textureCount > 0)
        {
                memcpy (
                    pOutputBuffer + offset,
                    pEncoder->pHashTable,
                    pEncoder->pHeader->textureCount * sizeof (struct R_RPackHashEntry));
                offset += pEncoder->pHeader->textureCount * sizeof (struct R_RPackHashEntry);
        }

        pEncoder->pHeader->colorTableOffset = offset;
        if (pEncoder->colorTableCount > 0)
        {
                memcpy (
                    pOutputBuffer + offset,
                    pEncoder->pColorTable,
                    pEncoder->colorTableCount * sizeof (struct R_RPackColorEntry));
                offset += pEncoder->colorTableCount * sizeof (struct R_RPackColorEntry);
        }
        pEncoder->pHeader->colorTableSize = pEncoder->colorTableCount;

        pEncoder->pHeader->pixelIndexTableOffset = offset;
        if (pEncoder->pixelIndexTableCount > 0)
        {
                memcpy (
                    pOutputBuffer + offset,
                    pEncoder->pPixelIndexTable,
                    pEncoder->pixelIndexTableCount * sizeof (struct R_RPackPixelIndexEntry));
                offset += pEncoder->pixelIndexTableCount * sizeof (struct R_RPackPixelIndexEntry);
        }
        pEncoder->pHeader->pixelIndexTableSize = pEncoder->pixelIndexTableCount;

        pEncoder->pHeader->dataOffset = offset;

        if (pBytesWritten)
        {
                *pBytesWritten = offset;
        }

        memcpy (pOutputBuffer, pEncoder->pHeader, sizeof (struct R_RPackHeader));

        return R_RPACK_OK;
}

uint32_t
R_RPack_EncoderGetImageCount (const struct R_RPackEncoder* pEncoder)
{
        if (!pEncoder || !pEncoder->pHeader)
        {
                return 0;
        }
        return pEncoder->pHeader->textureCount;
}
