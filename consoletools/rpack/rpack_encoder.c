#include "rpack/rpack_encoder.h"

#include "rlgame.base/cstl/cstl_heap_allocator.h"
#include "rlgame.base/cstl/cstl_log.h"
#include "rlgame.base/cstl/cstl_string.h"
#include "rlgame.base/cstl/cstl_thread.h"
#include <string.h>
#include <math.h>

#define R_PACK_DEFAULT_ATLAS_WIDTH            4096
#define R_PACK_DEFAULT_ATLAS_HEIGHT           4096
#define R_PACK_DEFAULT_PADDING                1
#define R_PACK_INITIAL_COLOR_TABLE_SIZE       4096
#define R_PACK_INITIAL_PIXEL_INDEX_TABLE_SIZE 65536
#define R_PACK_DEFAULT_WORKER_COUNT           0

static const struct r_pack_encoder_settings s_defaultEncoderSettings
    = {.maxAtlasWidth = R_PACK_DEFAULT_ATLAS_WIDTH,
       .maxAtlasHeight = R_PACK_DEFAULT_ATLAS_HEIGHT,
       .padding = R_PACK_DEFAULT_PADDING,
       .border = 0,
       .similarityThreshold = R_PACK_DEFAULT_SIMILARITY_THRESHOLD,
       .alphaThreshold = 0.0f,
       .workerCount = R_PACK_DEFAULT_WORKER_COUNT,
       .maxTextures = 0};

static uint32_t
r_pack_find_or_add_color (struct r_pack_encoder* pEncoder, uint8_t y, uint8_t yExp, uint8_t u, uint8_t v);

struct r_pack_pixel_work_task
{
        struct r_pack_encoder* pEncoder;
        const struct r_pack_input_image* pImage;
        uint32_t startY;
        uint32_t endY;
        uint32_t* pOutPixelCount;
        int* pOutError;
};

static void
r_pack_process_pixel_row_worker (void* pData)
{
    R_PACK_ASSERT (pData);
    struct r_pack_pixel_work_task* pTask = (struct r_pack_pixel_work_task*)pData;
    uint32_t pixelCount = 0;

    for (uint32_t y = pTask->startY; y < pTask->endY; ++y)
    {
        uint32_t x = 0;
        while (x < pTask->pImage->width)
        {
            const uint8_t* pCurrentPixel = pTask->pImage->pPixels + y * pTask->pImage->stride + x * 4;
            uint8_t r = pCurrentPixel[0];
            uint8_t g = pCurrentPixel[1];
            uint8_t b = pCurrentPixel[2];
            
            uint8_t yuvY, yuvYExp, yuvU, yuvV;
            r_pack_RGBAToYUV (r, g, b, &yuvY, &yuvYExp, &yuvU, &yuvV);
            
            uint32_t colorIndex = r_pack_find_or_add_color (pTask->pEncoder, yuvY, yuvYExp, yuvU, yuvV);
            if (colorIndex == UINT32_MAX)
            {
                *pTask->pOutError = 1;
                return;
            }
            
            uint32_t runWidth = 1;
            while (x + runWidth < pTask->pImage->width && runWidth < 63)
            {
                const uint8_t* pNextPixel = pTask->pImage->pPixels + y * pTask->pImage->stride + (x + runWidth) * 4;
                if (pNextPixel[0] == r && pNextPixel[1] == g && pNextPixel[2] == b)
                {
                    runWidth++;
                }
                else
                {
                    break;
                }
            }
            
            R_CSTL_MutexLock (pTask->pEncoder->pMutex);
            if (pTask->pEncoder->pixelIndexTableCount >= pTask->pEncoder->pixelIndexTableCapacity)
            {
                const uint32_t newCapacity = pTask->pEncoder->pixelIndexTableCapacity * 2;
                struct r_pack_pixel_index_entry* pNewTable
                    = (struct r_pack_pixel_index_entry*)R_CSTL_HeapRealloc (
                        pTask->pEncoder->pPixelIndexTable,
                        newCapacity * sizeof (struct r_pack_pixel_index_entry));
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

            struct r_pack_pixel_index_entry* pPixelEntry
                = &pTask->pEncoder->pPixelIndexTable[pTask->pEncoder->pixelIndexTableCount];
            pPixelEntry->colorIndex = (uint16_t)colorIndex;
            pPixelEntry->exponent = 1;
            pPixelEntry->runWidth = (uint8_t)runWidth;
            pPixelEntry->runHeight = 1;

            R_CSTL_MutexLock (pTask->pEncoder->pMutex);
            pTask->pEncoder->pixelIndexTableCount++;
            R_CSTL_MutexUnlock (pTask->pEncoder->pMutex);

            pixelCount += runWidth;
            x += runWidth;
        }
    }

    *pTask->pOutPixelCount = pixelCount;
}

struct r_pack_encoder*
r_pack_new_encoder (const struct r_pack_encoder_settings* pSettings)
{
    R_PACK_ASSERT (pSettings);
    struct r_pack_encoder* pEncoder
        = (struct r_pack_encoder*)R_CSTL_HeapAlloc (sizeof (struct r_pack_encoder));
    memset (pEncoder, 0, sizeof (struct r_pack_encoder));

    if (pSettings)
    {
        pEncoder->config = *pSettings;
    }
    else
    {
        pEncoder->config = s_defaultEncoderSettings;
    }

    if (pEncoder->config.maxAtlasWidth == 0 || pEncoder->config.maxAtlasHeight == 0
        || (uint64_t)pEncoder->config.maxAtlasWidth + pEncoder->config.padding * 2ULL > UINT32_MAX
        || (uint64_t)pEncoder->config.maxAtlasHeight + pEncoder->config.padding * 2ULL > UINT32_MAX
        || !isfinite (pEncoder->config.similarityThreshold) || pEncoder->config.similarityThreshold < 0.0f
        || !isfinite (pEncoder->config.alphaThreshold) || pEncoder->config.alphaThreshold < 0.0f
        || pEncoder->config.alphaThreshold > 1.0f)
    {
        R_CSTL_HeapFree (pEncoder);
        return NULL;
    }

    if (pEncoder->config.workerCount == 0)
    {
        pEncoder->actualWorkerCount = 1;
    }
    else
    {
        pEncoder->actualWorkerCount = pEncoder->config.workerCount;
    }
    pEncoder->pMutex = R_CSTL_NewMutex ();
    if (!pEncoder->pMutex)
    {
        R_CSTL_HeapFree (pEncoder);
        return NULL;
    }

    pEncoder->workersActive = 0;
    pEncoder->ppWorkerThreads = NULL;

    pEncoder->pHeader = (struct r_pack_header*)R_CSTL_HeapAlloc (sizeof (struct r_pack_header));
    if (!pEncoder->pHeader)
    {
        R_CSTL_HeapFree (pEncoder);
        return NULL;
    }
    memset (pEncoder->pHeader, 0, sizeof (struct r_pack_header));
    pEncoder->pHeader->magicInt32 = R_PACK_MAGIC;
    pEncoder->pHeader->version = R_PACK_VERSION;

    pEncoder->colorTableCapacity = R_PACK_INITIAL_COLOR_TABLE_SIZE;
    pEncoder->pColorTable = (struct r_pack_color_entry*)R_CSTL_HeapAlloc (
        pEncoder->colorTableCapacity * sizeof (struct r_pack_color_entry));
    if (!pEncoder->pColorTable)
    {
        R_CSTL_HeapFree (pEncoder->pHeader);
        R_CSTL_HeapFree (pEncoder);
        return NULL;
    }

    pEncoder->pixelIndexTableCapacity = R_PACK_INITIAL_PIXEL_INDEX_TABLE_SIZE;
    pEncoder->pPixelIndexTable = (struct r_pack_pixel_index_entry*)R_CSTL_HeapAlloc (
        pEncoder->pixelIndexTableCapacity * sizeof (struct r_pack_pixel_index_entry));
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
r_pack_delete_encoder (struct r_pack_encoder* pEncoder)
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
r_pack_RGBAToYUV (uint8_t r, uint8_t g, uint8_t b, uint8_t* pY, uint8_t* pYExp, uint8_t* pU, uint8_t* pV)
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
        float mantissa = frexpf (yf, &exp);
        *pY = (uint8_t)(mantissa * 255.0f);
        *pYExp = (uint8_t)(exp + 127);
    }

    *pU = (uint8_t)((uf + 0.5f) * 15.0f);
    *pV = (uint8_t)((vf + 0.5f) * 15.0f);
}

void
r_pack_YUVToRGBA (uint8_t y, uint8_t yExp, uint8_t u, uint8_t v, uint8_t* pR, uint8_t* pG, uint8_t* pB)
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
r_pack_get_color_similarity (uint8_t y1, uint8_t u1, uint8_t v1, uint8_t y2, uint8_t u2, uint8_t v2)
{
    float dy = (float)(y1 - y2) / 255.0f;
    float du = (float)(u1 - u2) / 15.0f;
    float dv = (float)(v1 - v2) / 15.0f;
    return sqrtf (dy * dy + du * du + dv * dv);
}

static uint32_t
r_pack_find_or_add_color (struct r_pack_encoder* pEncoder, uint8_t y, uint8_t yExp, uint8_t u, uint8_t v)
{
    R_CSTL_MutexLock (pEncoder->pMutex);

    // Cache-friendly linear search with early exit
    // For better cache locality, we process the color table sequentially
    const float threshold = pEncoder->config.similarityThreshold;
    uint32_t bestMatch = UINT32_MAX;
    float bestSimilarity = threshold;
    
    for (uint32_t i = 0; i < pEncoder->colorTableCount; ++i)
    {
        struct r_pack_color_entry* pEntry = &pEncoder->pColorTable[i];
        float similarity = r_pack_get_color_similarity (
            y,
            u,
            v,
            pEntry->luminance,
            pEntry->chrominanceU,
            pEntry->chrominanceV);
        if (similarity < 0.01f)
        {
            R_CSTL_MutexUnlock (pEncoder->pMutex);
            return i;
        }
        if (similarity < bestSimilarity)
        {
            bestSimilarity = similarity;
            bestMatch = i;
        }
    }
    if (bestMatch != UINT32_MAX)
    {
        R_CSTL_MutexUnlock (pEncoder->pMutex);
        return bestMatch;
    }
    if (pEncoder->colorTableCount >= pEncoder->colorTableCapacity)
    {
        uint32_t newCapacity = pEncoder->colorTableCapacity << 1u;
        struct r_pack_color_entry* pNewTable = (struct r_pack_color_entry*)R_CSTL_HeapRealloc (
            pEncoder->pColorTable,
            newCapacity * sizeof (struct r_pack_color_entry));
        if (!pNewTable)
        {
            R_CSTL_MutexUnlock (pEncoder->pMutex);
            return UINT32_MAX;
        }
        pEncoder->pColorTable = pNewTable;
        pEncoder->colorTableCapacity = newCapacity;
    }

    struct r_pack_color_entry* pEntry = &pEncoder->pColorTable[pEncoder->colorTableCount];
    pEntry->luminance = y;
    pEntry->luminanceExp = yExp;
    pEntry->chrominanceU = u;
    pEntry->chrominanceV = v;

    uint32_t colorIndex = pEncoder->colorTableCount++;
    R_CSTL_MutexUnlock (pEncoder->pMutex);
    return colorIndex;
}

static enum r_pack_error
r_pack_validate_input_image (const struct r_pack_input_image* pImage)
{
    R_PACK_ASSERT (pImage);
    R_PACK_ASSERT (pImage->pPixels);
    R_PACK_ASSERT (pImage->pName);
    if (pImage->width == 0 || pImage->height == 0)
    {
        return R_PACK_ERROR_INVALID_DIMENSIONS;
    }
    if (pImage->stride < pImage->width * 4)
    {
        return R_PACK_ERROR_INVALID_DIMENSIONS;
    }

    return R_PACK_OK;
}

static enum r_pack_error
r_pack_expand_hash_table (struct r_pack_encoder* pEncoder, uint32_t* pOutTextureIndex)
{
    R_PACK_ASSERT (pEncoder);
    R_PACK_ASSERT (pOutTextureIndex);
    R_CSTL_MutexLock (pEncoder->pMutex);
    const uint32_t textureIndex = pEncoder->pHeader->textureCount;

    if (textureIndex == 0)
    {
        pEncoder->pHashTable = (struct r_pack_hash_entry*)R_CSTL_HeapAlloc (sizeof (struct r_pack_hash_entry));
        if (!pEncoder->pHashTable)
        {
            R_CSTL_MutexUnlock (pEncoder->pMutex);
            return R_PACK_ERROR_OUT_OF_MEMORY;
        }
    }
    else
    {
        struct r_pack_hash_entry* pNewTable = (struct r_pack_hash_entry*)R_CSTL_HeapRealloc (
            pEncoder->pHashTable,
            (textureIndex + 1) * sizeof (struct r_pack_hash_entry));
        if (!pNewTable)
        {
            R_CSTL_MutexUnlock (pEncoder->pMutex);
            return R_PACK_ERROR_OUT_OF_MEMORY;
        }
        pEncoder->pHashTable = pNewTable;
    }

    *pOutTextureIndex = textureIndex;
    R_CSTL_MutexUnlock (pEncoder->pMutex);
    return R_PACK_OK;
}

static void
r_pack_get_atlas_position (
    struct r_pack_encoder* pEncoder,
    const struct r_pack_input_image* pImage,
    uint32_t textureIndex,
    uint32_t* pOutX,
    uint32_t* pOutY)
{
    R_PACK_ASSERT (pEncoder);
    R_PACK_ASSERT (pImage);
    R_PACK_ASSERT (pOutX);
    R_PACK_ASSERT (pOutY);
    uint32_t currentX = 0;
    uint32_t currentY = 0;
    if (textureIndex > 0)
    {
        const struct r_pack_hash_entry* pPrevEntry = &pEncoder->pHashTable[textureIndex - 1];
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
r_pack_update_atlas_dimensions (
    struct r_pack_encoder* pEncoder,
    uint32_t currentX,
    uint32_t currentY,
    uint32_t imageWidth,
    uint32_t imageHeight)
{
    R_PACK_ASSERT (pEncoder);
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

static enum r_pack_error
r_pack_process_pixels_serial (struct r_pack_encoder* pEncoder, const struct r_pack_input_image* pImage)
{
    R_PACK_ASSERT (pEncoder);
    
    uint32_t x = 0;
    uint32_t y = 0;
    
    while (y < pImage->height)
    {
        const uint8_t* pCurrentPixel = pImage->pPixels + y * pImage->stride + x * 4;
        uint8_t r = pCurrentPixel[0];
        uint8_t g = pCurrentPixel[1];
        uint8_t b = pCurrentPixel[2];
        
        uint8_t yuvY, yuvYExp, yuvU, yuvV;
        r_pack_RGBAToYUV (r, g, b, &yuvY, &yuvYExp, &yuvU, &yuvV);
        
        uint32_t colorIndex = r_pack_find_or_add_color (pEncoder, yuvY, yuvYExp, yuvU, yuvV);
        if (colorIndex == UINT32_MAX)
        {
            return R_PACK_ERROR_OUT_OF_MEMORY;
        }
        uint32_t runWidth = 1;
        while (x + runWidth < pImage->width && runWidth < 63)
        {
            const uint8_t* pNextPixel = pImage->pPixels + y * pImage->stride + (x + runWidth) * 4;
            if (pNextPixel[0] == r && pNextPixel[1] == g && pNextPixel[2] == b)
            {
                runWidth++;
            }
            else
            {
                break;
            }
        }
        uint32_t runHeight = 1;
        if (pEncoder->pixelIndexTableCount >= pEncoder->pixelIndexTableCapacity)
        {
            const uint32_t newCapacity = pEncoder->pixelIndexTableCapacity * 2;
            struct r_pack_pixel_index_entry* pNewTable
                = (struct r_pack_pixel_index_entry*)R_CSTL_HeapRealloc (
                    pEncoder->pPixelIndexTable,
                    newCapacity * sizeof (struct r_pack_pixel_index_entry));
            if (!pNewTable)
            {
                return R_PACK_ERROR_OUT_OF_MEMORY;
            }
            pEncoder->pPixelIndexTable = pNewTable;
            pEncoder->pixelIndexTableCapacity = newCapacity;
        }
        struct r_pack_pixel_index_entry* pPixelEntry
            = &pEncoder->pPixelIndexTable[pEncoder->pixelIndexTableCount];
        pPixelEntry->colorIndex = (uint16_t)colorIndex;
        pPixelEntry->exponent = 1;
        pPixelEntry->runWidth = (uint8_t)runWidth;
        pPixelEntry->runHeight = 1;
        pEncoder->pixelIndexTableCount++;
        
        x += runWidth;
        if (x >= pImage->width)
        {
            x = 0;
            y++;
        }
    }
    
    return R_PACK_OK;
}

static enum r_pack_error
r_pack_process_pixels (struct r_pack_encoder* pEncoder, const struct r_pack_input_image* pImage)
{
    R_PACK_ASSERT (pEncoder);
    uint32_t rowsPerWorker = pImage->height / pEncoder->actualWorkerCount;
    struct r_pack_pixel_work_task* pTasks = (struct r_pack_pixel_work_task*)R_CSTL_HeapAlloc (
        pEncoder->actualWorkerCount * sizeof (struct r_pack_pixel_work_task));
    uint32_t* pPixelCounts = (uint32_t*)R_CSTL_HeapAlloc (pEncoder->actualWorkerCount * sizeof (uint32_t));
    int* pErrors = (int*)R_CSTL_HeapAlloc (pEncoder->actualWorkerCount * sizeof (int));

    if (!pTasks || !pPixelCounts || !pErrors)
    {
        if (pTasks) R_CSTL_HeapFree (pTasks);
        if (pPixelCounts) R_CSTL_HeapFree (pPixelCounts);
        if (pErrors) R_CSTL_HeapFree (pErrors);
        return R_PACK_ERROR_OUT_OF_MEMORY;
    }
    memset (pErrors, 0, pEncoder->actualWorkerCount * sizeof (int));

    for (uint32_t i = 0; i < pEncoder->actualWorkerCount; ++i)
    {
        pTasks[i].pEncoder = pEncoder;
        pTasks[i].pImage = pImage;
        pTasks[i].startY = i * rowsPerWorker;
        pTasks[i].endY = (i == pEncoder->actualWorkerCount - 1) ? pImage->height : (i + 1) * rowsPerWorker;
        pTasks[i].pOutPixelCount = &pPixelCounts[i];
        pTasks[i].pOutError = &pErrors[i];
    }

    for (uint32_t i = 0; i < pEncoder->actualWorkerCount; ++i)
    {
        struct R_CSTL_Thread* pThread = R_CSTL_NewThread (r_pack_process_pixel_row_worker, &pTasks[i]);
        if (!pThread)
        {
            for (uint32_t j = i; j < pEncoder->actualWorkerCount; ++j)
            {
                r_pack_process_pixel_row_worker (&pTasks[j]);
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
            return R_PACK_ERROR_OUT_OF_MEMORY;
        }
    }

    R_CSTL_HeapFree (pTasks);
    R_CSTL_HeapFree (pPixelCounts);
    R_CSTL_HeapFree (pErrors);
    return R_PACK_OK;
}

static enum r_pack_error
r_pack_process_image_pixels (struct r_pack_encoder* pEncoder, const struct r_pack_input_image* pImage)
{
    R_PACK_ASSERT (pEncoder);
    if (pEncoder->actualWorkerCount > 1 && pImage->height > 100)
    {
        return r_pack_process_pixels (pEncoder, pImage);
    }
    else
    {
        return r_pack_process_pixels_serial (pEncoder, pImage);
    }
}

enum r_pack_error
r_pack_encoder_add_image (struct r_pack_encoder* pEncoder, const struct r_pack_input_image* pImage)
{
    R_PACK_ASSERT (pEncoder);
    enum r_pack_error error = r_pack_validate_input_image (pImage);
    if (error != R_PACK_OK)
    {
        return error;
    }

    if (pEncoder->config.maxTextures != 0 && pEncoder->pHeader->textureCount >= pEncoder->config.maxTextures)
    {
        return R_PACK_ERROR_INVALID_DIMENSIONS;
    }

    uint64_t paddedWidth = (uint64_t)pImage->width + pEncoder->config.padding * 2ULL;
    uint64_t paddedHeight = (uint64_t)pImage->height + pEncoder->config.padding * 2ULL;
    if (paddedWidth > pEncoder->config.maxAtlasWidth || paddedHeight > pEncoder->config.maxAtlasHeight)
    {
        return R_PACK_ERROR_INVALID_DIMENSIONS;
    }

    uint32_t textureIndex;
    error = r_pack_expand_hash_table (pEncoder, &textureIndex);
    if (error != R_PACK_OK)
    {
        return error;
    }

    struct r_pack_hash_entry* pEntry = &pEncoder->pHashTable[textureIndex];
    pEntry->nameHash = r_pack_hash64_string (pImage->pName, 0);
    pEntry->width = pImage->width;
    pEntry->height = pImage->height;
    pEntry->colorTableIndex = pEncoder->colorTableCount;
    pEntry->pixelIndexTableOffset = pEncoder->pixelIndexTableCount;

    uint32_t currentX, currentY;
    r_pack_get_atlas_position (pEncoder, pImage, textureIndex, &currentX, &currentY);

    pEntry->atlasOffsetX = currentX;
    pEntry->atlasOffsetY = currentY;

    r_pack_update_atlas_dimensions (pEncoder, currentX, currentY, pImage->width, pImage->height);

    error = r_pack_process_image_pixels (pEncoder, pImage);
    if (error != R_PACK_OK)
    {
        return error;
    }

    R_CSTL_MutexLock (pEncoder->pMutex);
    pEncoder->pHeader->textureCount++;
    R_CSTL_MutexUnlock (pEncoder->pMutex);
    return R_PACK_OK;
}

uint64_t
r_pack_encoder_get_required_size (const struct r_pack_encoder* pEncoder)
{
    R_PACK_ASSERT (pEncoder);
    R_PACK_ASSERT (pEncoder->pHeader);

    uint64_t hashTableSize = (uint64_t)pEncoder->pHeader->textureCount * sizeof (struct r_pack_hash_entry);
    uint64_t colorTableSize = (uint64_t)pEncoder->colorTableCount * sizeof (struct r_pack_color_entry);
    uint64_t pixelIndexTableSize
        = (uint64_t)pEncoder->pixelIndexTableCount * sizeof (struct r_pack_pixel_index_entry);
    if (pEncoder->pHeader->atlasWidth != 0
        && pEncoder->pHeader->atlasHeight > UINT64_MAX / pEncoder->pHeader->atlasWidth / 2)
    {
        return 0;
    }
    uint64_t atlasDataSize = (uint64_t)pEncoder->pHeader->atlasWidth * pEncoder->pHeader->atlasHeight * 2;

    return R_PACK_HEADER_SIZE + hashTableSize + colorTableSize + pixelIndexTableSize + atlasDataSize;
}

enum r_pack_error
r_pack_encoder_encode (
    struct r_pack_encoder* pEncoder,
    uint8_t* pOutputBuffer,
    uint64_t outputBufferSize,
    uint64_t* pBytesWritten)
{
    R_PACK_ASSERT (pEncoder);
    R_PACK_ASSERT (pOutputBuffer);
    R_PACK_ASSERT (pBytesWritten);

    uint64_t requiredSize = r_pack_encoder_get_required_size (pEncoder);
    if (outputBufferSize < requiredSize)
    {
        return R_PACK_ERROR_BUFFER_TOO_SMALL;
    }

    uint64_t offset = 0;

    memcpy (pOutputBuffer + offset, pEncoder->pHeader, sizeof (struct r_pack_header));
    offset += sizeof (struct r_pack_header);

    pEncoder->pHeader->hashTableOffset = offset;
    if (pEncoder->pHeader->textureCount > 0)
    {
        memcpy (
            pOutputBuffer + offset,
            pEncoder->pHashTable,
            pEncoder->pHeader->textureCount * sizeof (struct r_pack_hash_entry));
        offset += pEncoder->pHeader->textureCount * sizeof (struct r_pack_hash_entry);
    }

    pEncoder->pHeader->colorTableOffset = offset;
    if (pEncoder->colorTableCount > 0)
    {
        memcpy (
            pOutputBuffer + offset,
            pEncoder->pColorTable,
            pEncoder->colorTableCount * sizeof (struct r_pack_color_entry));
        offset += pEncoder->colorTableCount * sizeof (struct r_pack_color_entry);
    }
    pEncoder->pHeader->colorTableSize = pEncoder->colorTableCount;

    pEncoder->pHeader->pixelIndexTableOffset = offset;
    if (pEncoder->pixelIndexTableCount > 0)
    {
        memcpy (
            pOutputBuffer + offset,
            pEncoder->pPixelIndexTable,
            pEncoder->pixelIndexTableCount * sizeof (struct r_pack_pixel_index_entry));
        offset += pEncoder->pixelIndexTableCount * sizeof (struct r_pack_pixel_index_entry);
    }
    pEncoder->pHeader->pixelIndexTableSize = pEncoder->pixelIndexTableCount;
    pEncoder->pHeader->dataOffset = offset;

    uint64_t atlasDataSize = (uint64_t)pEncoder->pHeader->atlasWidth * pEncoder->pHeader->atlasHeight * 2;
    memset (pOutputBuffer + offset, 0, (size_t)atlasDataSize);
    offset += atlasDataSize;

    if (pBytesWritten)
    {
        *pBytesWritten = offset;
    }
    memcpy (pOutputBuffer, pEncoder->pHeader, sizeof (struct r_pack_header));
    return R_PACK_OK;
}

uint32_t
r_pack_encoder_get_image_count (const struct r_pack_encoder* pEncoder)
{
    R_PACK_ASSERT(pEncoder);
    R_PACK_ASSERT(pEncoder->pHeader);
    return pEncoder->pHeader->textureCount;
}
