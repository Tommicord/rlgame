#include "rpack/rpack_pipeline.h"

#include "rpack_imgdecode_jpeg.h"
#include "rpack_val.h"
#include "rlgame.base/cstl/cstl_array.h"
#include "rlgame.base/cstl/cstl_atomic.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"
#include "rlgame.base/cstl/cstl_log.h"
#include "rlgame.base/cstl/cstl_thread.h"

#include <stdio.h>
#include <string.h>

static void
R_Pack_LogImageWarnings (
    const char*                        pPath,
    const struct R_Pack_InputImage*    pImage,
    const struct R_Pack_EncoderConfig* pConfig)
{
    uint64_t imageBytes = (uint64_t)pImage->width * pImage->height * 4;
    if (imageBytes > 16ULL * 1024ULL * 1024ULL)
    {
        R_CSTL_LOG_WARN (
            "Image %s is %ux%u (%.1f MiB RGBA); decoding and packing may use significant memory",
            pPath,
            pImage->width,
            pImage->height,
            (double)imageBytes / (1024.0 * 1024.0));
    }
    if (pImage->width > pConfig->maxAtlasWidth || pImage->height > pConfig->maxAtlasHeight)
    {
        R_CSTL_LOG_WARN (
            "Image %s (%ux%u) exceeds the configured atlas limit %ux%u and will be skipped",
            pPath,
            pImage->width,
            pImage->height,
            pConfig->maxAtlasWidth,
            pConfig->maxAtlasHeight);
    }
}

R_PACK_API uint32_t
R_Pack_MipmapDimension (uint32_t source, uint32_t other, uint32_t limit)
{
    uint32_t dimension = source > other ? limit : (uint32_t)(((uint64_t)source * limit) / other);
    return dimension == 0 ? 1 : dimension;
}

R_PACK_API uint8_t*
R_Pack_ResizeImageBox (const struct R_Pack_InputImage* pSource, uint32_t width, uint32_t height)
{
    uint8_t* pPixels = (uint8_t*)R_CSTL_HeapAlloc ((size_t)width * height * 4);
    if (!pPixels) return NULL;

    for (uint32_t y = 0; y < height; ++y)
    {
        uint32_t sourceY0 = (uint64_t)y * pSource->height / height;
        uint32_t sourceY1 = ((uint64_t)(y + 1) * pSource->height + height - 1) / height;
        if (sourceY1 > pSource->height) sourceY1 = pSource->height;
        for (uint32_t x = 0; x < width; ++x)
        {
            uint32_t sourceX0 = (uint64_t)x * pSource->width / width;
            uint32_t sourceX1 = ((uint64_t)(x + 1) * pSource->width + width - 1) / width;
            if (sourceX1 > pSource->width) sourceX1 = pSource->width;
            uint64_t sums[4] = {0, 0, 0, 0};
            uint32_t count = 0;
            for (uint32_t sourceY = sourceY0; sourceY < sourceY1; ++sourceY)
                for (uint32_t sourceX = sourceX0; sourceX < sourceX1; ++sourceX)
                {
                    const uint8_t* pSourcePixel
                        = pSource->pPixels + (size_t)sourceY * pSource->stride + sourceX * 4;
                    for (uint32_t channel = 0; channel < 4; ++channel)
                        sums[channel] += pSourcePixel[channel];
                    ++count;
                }
            uint8_t* pDestinationPixel = pPixels + ((size_t)y * width + x) * 4;
            for (uint32_t channel = 0; channel < 4; ++channel)
                pDestinationPixel[channel] = (uint8_t)(sums[channel] / count);
        }
    }
    return pPixels;
}

R_PACK_API int
R_Pack_EncodeAndWrite (struct R_Pack_Encoder* pEncoder, const char* pOutputPath)
{
    uint64_t requiredSize = R_Pack_EncoderGetRequiredSize (pEncoder);
    R_CSTL_LOG_INFO ("Required output size: %llu bytes", (unsigned long long)requiredSize);

    if (requiredSize > 32ULL * 1024ULL * 1024ULL)
    {
        R_CSTL_LOG_WARN (
            "Output requires %.1f MiB; the CLI heap is limited to 64 MiB",
            (double)requiredSize / (1024.0 * 1024.0));
    }

    uint8_t* pOutputBuffer = (uint8_t*)R_CSTL_HeapAlloc (requiredSize);
    if (!pOutputBuffer)
    {
        R_CSTL_LOG_ERROR ("Failed to allocate output buffer");
        return -1;
    }

    uint64_t          bytesWritten = 0;
    enum R_Pack_Error encodeErr = R_Pack_EncoderEncode (pEncoder, pOutputBuffer, requiredSize, &bytesWritten);
    if (encodeErr != R_PACK_OK)
    {
        R_CSTL_LOG_ERROR ("Encoding failed: %s", R_Pack_ErrorToString (encodeErr));
        R_CSTL_HeapFree (pOutputBuffer);
        return -1;
    }

    FILE* pOutputFile = fopen (pOutputPath, "wb");
    if (!pOutputFile)
    {
        R_CSTL_LOG_ERROR ("Failed to open output file: %s", pOutputPath);
        R_CSTL_HeapFree (pOutputBuffer);
        return -1;
    }

    size_t written = fwrite (pOutputBuffer, 1, bytesWritten, pOutputFile);
    fclose (pOutputFile);

    if (written != bytesWritten)
    {
        R_CSTL_LOG_ERROR ("Failed to write complete output file");
        R_CSTL_HeapFree (pOutputBuffer);
        return -1;
    }

    struct R_Pack_ValidationReport report;
    if (!R_Pack_ValidatePackedData (pOutputBuffer, bytesWritten, &report))
    {
        R_CSTL_LOG_ERROR (
            "RPACK validation failed: %s at byte %llu (texture %u, pixel %llu)",
            R_Pack_ErrorToString (report.error),
            (unsigned long long)report.offset,
            report.textureIndex,
            (unsigned long long)report.pixelIndex);
        remove (pOutputPath);
        R_CSTL_HeapFree (pOutputBuffer);
        return -1;
    }

    R_CSTL_LOG_INFO ("RPACK validation passed: %llu bytes verified", (unsigned long long)bytesWritten);

    R_CSTL_LOG_INFO ("Successfully wrote %llu bytes to %s", (unsigned long long)bytesWritten, pOutputPath);
    R_CSTL_HeapFree (pOutputBuffer);
    return 0;
}

struct R_Pack_ImageLoadTask
{
        const char*               pPath;
        struct R_Pack_InputImage* pImage;
        uint8_t**                 ppPixelBuffer;
        int*                      pResult;
        struct R_Pack_ThreadPool* pPool;
};

struct R_Pack_ThreadPool
{
        struct R_CSTL_Thread**       ppThreads;
        uint32_t                     threadCount;
        R_CSTL_AtomicUint32          nextTaskIndex;
        R_CSTL_AtomicUint32          completedTasks;
        R_CSTL_AtomicUint32          failedTasks;
        struct R_CSTL_Mutex*         pTaskMutex;
        struct R_CSTL_Condition*     pTaskAvailable;
        struct R_CSTL_Condition*     pTaskComplete;
        struct R_Pack_ImageLoadTask* pTasks;
        uint32_t                     taskCount;
        int                          shutdown;
};

static struct R_Pack_ThreadPool* R_Pack_ThreadPoolCreate (uint32_t workerCount);

static void R_Pack_ThreadPoolDestroy (struct R_Pack_ThreadPool* pPool);

static void R_Pack_WorkerThreadFunc (void* pData);

static int R_Pack_ThreadPoolStart (struct R_Pack_ThreadPool* pPool);

static int R_Pack_ThreadPoolSubmitTasks (
    struct R_Pack_ThreadPool*    pPool,
    struct R_Pack_ImageLoadTask* pTasks,
    uint32_t                     taskCount);

static void R_Pack_ThreadPoolWaitAll (struct R_Pack_ThreadPool* pPool);

static void R_Pack_ThreadPoolShutdown (struct R_Pack_ThreadPool* pPool);

R_PACK_API int
R_Pack_HasExtension (const char* pPath, const char* pExtension)
{
    size_t pathLength = strlen (pPath);
    size_t extensionLength = strlen (pExtension);
    if (pathLength < extensionLength)
    {
        return 0;
    }
    pPath += pathLength - extensionLength;
    for (size_t i = 0; i < extensionLength; ++i)
    {
        char pathChar = pPath[i];
        char extensionChar = pExtension[i];
        if (pathChar >= 'A' && pathChar <= 'Z') pathChar = (char)(pathChar + ('a' - 'A'));
        if (extensionChar >= 'A' && extensionChar <= 'Z') extensionChar = (char)(extensionChar + ('a' - 'A'));
        if (pathChar != extensionChar) return 0;
    }
    return 1;
}

R_PACK_API int
R_Pack_MakeVariantPath (const char* pOutputPath, uint32_t size, char** ppVariantPath)
{
    const char* pExtension = strrchr (pOutputPath, '.');
    size_t      stemLength = pExtension ? (size_t)(pExtension - pOutputPath) : strlen (pOutputPath);
    size_t      suffixLength = strlen ("_4294967295x4294967295.rpack");
    char*       pPath = (char*)R_CSTL_HeapAlloc (stemLength + suffixLength + 1);
    if (!pPath) return -1;

    snprintf (
        pPath,
        stemLength + suffixLength + 1,
        "%.*s_%ux%u.rpack",
        (int)stemLength,
        pOutputPath,
        size,
        size);
    *ppVariantPath = pPath;
    return 0;
}

R_PACK_API int
R_Pack_EncodeMipmapVariants (
    const struct R_Pack_EncoderConfig* pConfig,
    const struct R_CSTL_Array*         pInputPaths,
    const char*                        pOutputPath)
{
    static const uint32_t mipmapSizes[] = {64, 32, 16, 8, 4, 2, 1};
    size_t                generated = 0;
    for (size_t i = 0; i < sizeof (mipmapSizes) / sizeof (mipmapSizes[0]); ++i)
    {
        char* pVariantPath = NULL;
        if (R_Pack_MakeVariantPath (pOutputPath, mipmapSizes[i], &pVariantPath) != 0)
        {
            R_CSTL_LOG_ERROR (
                "Failed to allocate mipmap output path for %ux%u",
                mipmapSizes[i],
                mipmapSizes[i]);
            return -1;
        }

        struct R_Pack_Encoder* pVariantEncoder = R_Pack_NewEncoder (pConfig);
        if (!pVariantEncoder)
        {
            R_CSTL_LOG_ERROR (
                "Failed to create encoder for mipmap level %ux%u",
                mipmapSizes[i],
                mipmapSizes[i]);
            R_CSTL_HeapFree (pVariantPath);
            return -1;
        }

        uint32_t successCount = R_Pack_EncodeInputImagesThreaded (
            pVariantEncoder,
            pInputPaths,
            mipmapSizes[i],
            pConfig->workerCount);
        if (successCount == 0 || R_Pack_EncodeAndWrite (pVariantEncoder, pVariantPath) != 0)
        {
            R_CSTL_LOG_WARN ("Mipmap level %ux%u was not generated", mipmapSizes[i], mipmapSizes[i]);
            R_Pack_DeleteEncoder (pVariantEncoder);
            R_CSTL_HeapFree (pVariantPath);
            continue;
        }
        R_CSTL_LOG_INFO ("Generated mipmap variant %s (%u images)", pVariantPath, successCount);
        ++generated;
        R_Pack_DeleteEncoder (pVariantEncoder);
        R_CSTL_HeapFree (pVariantPath);
    }
    return generated == 0 ? -1 : 0;
}

static struct R_Pack_ThreadPool*
R_Pack_ThreadPoolCreate (uint32_t workerCount)
{
    struct R_Pack_ThreadPool* pPool
        = (struct R_Pack_ThreadPool*)R_CSTL_HeapAlloc (sizeof (struct R_Pack_ThreadPool));
    if (!pPool)
    {
        return NULL;
    }
    memset (pPool, 0, sizeof (struct R_Pack_ThreadPool));

    pPool->threadCount = workerCount;
    pPool->ppThreads
        = (struct R_CSTL_Thread**)R_CSTL_HeapAlloc (workerCount * sizeof (struct R_CSTL_Thread*));
    if (!pPool->ppThreads)
    {
        R_CSTL_HeapFree (pPool);
        return NULL;
    }
    memset (pPool->ppThreads, 0, workerCount * sizeof (struct R_CSTL_Thread*));

    pPool->pTaskMutex = R_CSTL_NewMutex ();
    if (!pPool->pTaskMutex)
    {
        R_CSTL_HeapFree (pPool->ppThreads);
        R_CSTL_HeapFree (pPool);
        return NULL;
    }

    pPool->pTaskAvailable = R_CSTL_ConditionCreate ();
    if (!pPool->pTaskAvailable)
    {
        R_CSTL_MutexDestroy (pPool->pTaskMutex);
        R_CSTL_HeapFree (pPool->ppThreads);
        R_CSTL_HeapFree (pPool);
        return NULL;
    }

    pPool->pTaskComplete = R_CSTL_ConditionCreate ();
    if (!pPool->pTaskComplete)
    {
        R_CSTL_ConditionDestroy (pPool->pTaskAvailable);
        R_CSTL_MutexDestroy (pPool->pTaskMutex);
        R_CSTL_HeapFree (pPool->ppThreads);
        R_CSTL_HeapFree (pPool);
        return NULL;
    }

    R_CSTL_AtomicUint32Store (&pPool->nextTaskIndex, 0);
    R_CSTL_AtomicUint32Store (&pPool->completedTasks, 0);
    R_CSTL_AtomicUint32Store (&pPool->failedTasks, 0);

    return pPool;
}

static void
R_Pack_ThreadPoolDestroy (struct R_Pack_ThreadPool* pPool)
{
    if (!pPool)
    {
        return;
    }

    if (pPool->ppThreads)
    {
        for (uint32_t i = 0; i < pPool->threadCount; ++i)
        {
            if (pPool->ppThreads[i])
            {
                R_CSTL_ThreadJoin (pPool->ppThreads[i]);
            }
        }
        R_CSTL_HeapFree (pPool->ppThreads);
    }

    if (pPool->pTaskAvailable)
    {
        R_CSTL_ConditionDestroy (pPool->pTaskAvailable);
    }
    if (pPool->pTaskComplete)
    {
        R_CSTL_ConditionDestroy (pPool->pTaskComplete);
    }
    if (pPool->pTaskMutex)
    {
        R_CSTL_MutexDestroy (pPool->pTaskMutex);
    }
    if (pPool->pTasks)
    {
        R_CSTL_HeapFree (pPool->pTasks);
    }

    R_CSTL_HeapFree (pPool);
}

static int
R_Pack_LoadAsset (const char* pPath, struct R_Pack_InputImage* pImage, uint8_t** ppPixelBuffer)
{
    if (!pPath || !pImage || !ppPixelBuffer)
    {
        return -1;
    }
    *ppPixelBuffer = NULL;
    memset (pImage, 0, sizeof (*pImage));

    if (!R_Pack_HasExtension (pPath, ".jpg") && !R_Pack_HasExtension (pPath, ".jpeg"))
    {
        R_CSTL_LOG_WARN ("Skipping unsupported image format: %s (JPEG expected)", pPath);
        return -1;
    }

    struct R_Pack_JpegImage decoded = {0};
    enum R_Pack_Error       error = R_Pack_JpegDecodeFile (pPath, &decoded);
    if (error != R_PACK_OK)
    {
        fprintf (stderr, "JPEG decode failed for %s: %s\n", pPath, R_Pack_ErrorToString (error));
        return -1;
    }

    *ppPixelBuffer = decoded.pPixels;
    pImage->pPixels = decoded.pPixels;
    pImage->width = decoded.width;
    pImage->height = decoded.height;
    pImage->stride = decoded.stride;
    pImage->pName = pPath;
    return 0;
}

static void
R_Pack_WorkerThreadFunc (void* pData)
{
    struct R_Pack_ThreadPool* pPool = (struct R_Pack_ThreadPool*)pData;

    while (1)
    {
        R_CSTL_MutexLock (pPool->pTaskMutex);
        while (R_CSTL_AtomicUint32Load (&pPool->nextTaskIndex) >= pPool->taskCount && !pPool->shutdown)
        {
            R_CSTL_ConditionWait (pPool->pTaskAvailable, pPool->pTaskMutex);
        }

        if (pPool->shutdown)
        {
            R_CSTL_MutexUnlock (pPool->pTaskMutex);
            break;
        }

        uint32_t taskIndex = R_CSTL_AtomicUint32Load (&pPool->nextTaskIndex);
        if (taskIndex >= pPool->taskCount)
        {
            R_CSTL_MutexUnlock (pPool->pTaskMutex);
            break;
        }

        R_CSTL_AtomicUint32Inc (&pPool->nextTaskIndex);
        struct R_Pack_ImageLoadTask task = pPool->pTasks[taskIndex];
        R_CSTL_MutexUnlock (pPool->pTaskMutex);

        int loadResult = R_Pack_LoadAsset (task.pPath, task.pImage, task.ppPixelBuffer);
        *task.pResult = loadResult;

        uint32_t completed = R_CSTL_AtomicUint32Inc (&pPool->completedTasks);
        if (loadResult < 0)
        {
            R_CSTL_AtomicUint32Inc (&pPool->failedTasks);
        }

        if (completed == pPool->taskCount)
        {
            R_CSTL_MutexLock (pPool->pTaskMutex);
            R_CSTL_ConditionSignal (pPool->pTaskComplete);
            R_CSTL_MutexUnlock (pPool->pTaskMutex);
        }
    }
}

static int
R_Pack_ThreadPoolStart (struct R_Pack_ThreadPool* pPool)
{
    for (uint32_t i = 0; i < pPool->threadCount; ++i)
    {
        pPool->ppThreads[i] = R_CSTL_NewThread (R_Pack_WorkerThreadFunc, pPool);
        if (!pPool->ppThreads[i])
        {
            pPool->shutdown = 1;
            R_CSTL_MutexLock (pPool->pTaskMutex);
            R_CSTL_ConditionBroadcast (pPool->pTaskAvailable);
            R_CSTL_MutexUnlock (pPool->pTaskMutex);
            for (uint32_t j = 0; j < i; ++j)
            {
                R_CSTL_ThreadJoin (pPool->ppThreads[j]);
            }
            return -1;
        }
    }
    return 0;
}

static int
R_Pack_ThreadPoolSubmitTasks (
    struct R_Pack_ThreadPool*    pPool,
    struct R_Pack_ImageLoadTask* pTasks,
    uint32_t                     taskCount)
{
    pPool->pTasks = pTasks;
    pPool->taskCount = taskCount;
    R_CSTL_AtomicUint32Store (&pPool->nextTaskIndex, 0);
    R_CSTL_AtomicUint32Store (&pPool->completedTasks, 0);
    R_CSTL_AtomicUint32Store (&pPool->failedTasks, 0);

    R_CSTL_MutexLock (pPool->pTaskMutex);
    R_CSTL_ConditionBroadcast (pPool->pTaskAvailable);
    R_CSTL_MutexUnlock (pPool->pTaskMutex);

    return 0;
}

static void
R_Pack_ThreadPoolWaitAll (struct R_Pack_ThreadPool* pPool)
{
    R_CSTL_MutexLock (pPool->pTaskMutex);
    while (R_CSTL_AtomicUint32Load (&pPool->completedTasks) < pPool->taskCount)
    {
        R_CSTL_ConditionWait (pPool->pTaskComplete, pPool->pTaskMutex);
    }
    R_CSTL_MutexUnlock (pPool->pTaskMutex);
}

static void
R_Pack_ThreadPoolShutdown (struct R_Pack_ThreadPool* pPool)
{
    if (!pPool)
    {
        return;
    }
    pPool->shutdown = 1;
    R_CSTL_MutexLock (pPool->pTaskMutex);
    R_CSTL_ConditionBroadcast (pPool->pTaskAvailable);
    R_CSTL_MutexUnlock (pPool->pTaskMutex);
}

R_PACK_API uint32_t
R_Pack_EncodeInputImages (
    struct R_Pack_Encoder*     pEncoder,
    const struct R_CSTL_Array* pInputPaths,
    uint32_t                   mipmapSize)
{
    uint32_t    successCount = 0;
    size_t      inputCount = 0;
    size_t      offset = 0;
    size_t      inputBytes = R_CSTL_ArrayLength (pInputPaths);
    const char* pInputData = (const char*)R_CSTL_ArrayData (pInputPaths);

    while (offset < inputBytes)
    {
        size_t pathLength = strnlen (pInputData + offset, inputBytes - offset);
        if (pathLength == inputBytes - offset) break;
        ++inputCount;
        offset += pathLength + 1;
    }
    offset = 0;

    for (size_t i = 0; i < inputCount; ++i)
    {
        char* pPath = (char*)(pInputData + offset);
        offset += strlen (pPath) + 1;

        R_CSTL_LOG_INFO ("Processing input %zu: %s", i + 1, pPath);
        uint8_t*                 pPixelBuffer = NULL;
        struct R_Pack_InputImage image = {0};

        int loadResult = R_Pack_LoadAsset (pPath, &image, &pPixelBuffer);
        if (loadResult < 0)
        {
            R_CSTL_LOG_WARN ("Skipping image after load failure: %s", pPath);
            continue;
        }

        if (mipmapSize != 0 && (image.width > mipmapSize || image.height > mipmapSize))
        {
            uint32_t width = R_Pack_MipmapDimension (image.width, image.height, mipmapSize);
            uint32_t height = R_Pack_MipmapDimension (image.height, image.width, mipmapSize);
            uint8_t* pResizedPixels = R_Pack_ResizeImageBox (&image, width, height);
            if (!pResizedPixels)
            {
                R_CSTL_LOG_WARN ("Skipping mipmap level for %s: resize allocation failed", pPath);
                R_CSTL_HeapFree (pPixelBuffer);
                continue;
            }
            R_CSTL_HeapFree (pPixelBuffer);
            pPixelBuffer = pResizedPixels;
            image.pPixels = pResizedPixels;
            image.width = width;
            image.height = height;
            image.stride = width * 4;
        }

        R_Pack_LogImageWarnings (pPath, &image, &pEncoder->config);

        enum R_Pack_Error err = R_Pack_EncoderAddImage (pEncoder, &image);
        if (err != R_PACK_OK)
        {
            R_CSTL_LOG_WARN ("Skipping image '%s': %s", pPath, R_Pack_ErrorToString (err));
            if (pPixelBuffer)
            {
                R_CSTL_HeapFree (pPixelBuffer);
            }
            continue;
        }
        successCount++;
        if (pPixelBuffer)
        {
            R_CSTL_HeapFree (pPixelBuffer);
        }
    }

    return successCount;
}

R_PACK_API uint32_t
R_Pack_EncodeInputImagesThreaded (
    struct R_Pack_Encoder*     pEncoder,
    const struct R_CSTL_Array* pInputPaths,
    uint32_t                   mipmapSize,
    uint32_t                   workerCount)
{
    uint32_t    successCount = 0;
    size_t      inputCount = 0;
    size_t      offset = 0;
    size_t      inputBytes = R_CSTL_ArrayLength (pInputPaths);
    const char* pInputData = (const char*)R_CSTL_ArrayData (pInputPaths);

    while (offset < inputBytes)
    {
        size_t pathLength = strnlen (pInputData + offset, inputBytes - offset);
        if (pathLength == inputBytes - offset) break;
        ++inputCount;
        offset += pathLength + 1;
    }
    if (inputCount == 0)
    {
        return 0;
    }
    if (workerCount == 0 || workerCount == 1 || inputCount < 2)
    {
        return R_Pack_EncodeInputImages (pEncoder, pInputPaths, mipmapSize);
    }
    struct R_Pack_ThreadPool* pPool = R_Pack_ThreadPoolCreate (workerCount);
    if (!pPool)
    {
        R_CSTL_LOG_WARN ("Failed to create thread pool, falling back to serial processing");
        return R_Pack_EncodeInputImages (pEncoder, pInputPaths, mipmapSize);
    }

    if (R_Pack_ThreadPoolStart (pPool) != 0)
    {
        R_CSTL_LOG_WARN ("Failed to start thread pool, falling back to serial processing");
        R_Pack_ThreadPoolDestroy (pPool);
        return R_Pack_EncodeInputImages (pEncoder, pInputPaths, mipmapSize);
    }

    struct R_Pack_ImageLoadTask* pTasks
        = (struct R_Pack_ImageLoadTask*)R_CSTL_HeapAlloc (inputCount * sizeof (struct R_Pack_ImageLoadTask));
    struct R_Pack_InputImage* pImages
        = (struct R_Pack_InputImage*)R_CSTL_HeapAlloc (inputCount * sizeof (struct R_Pack_InputImage));
    uint8_t** ppPixelBuffers = (uint8_t**)R_CSTL_HeapAlloc (inputCount * sizeof (uint8_t*));
    int*      pResults = (int*)R_CSTL_HeapAlloc (inputCount * sizeof (int));

    if (!pTasks || !pImages || !ppPixelBuffers || !pResults)
    {
        R_CSTL_LOG_ERROR ("Failed to allocate task arrays, falling back to serial processing");
        if (pTasks) R_CSTL_HeapFree (pTasks);
        if (pImages) R_CSTL_HeapFree (pImages);
        if (ppPixelBuffers) R_CSTL_HeapFree (ppPixelBuffers);
        if (pResults) R_CSTL_HeapFree (pResults);
        R_Pack_ThreadPoolShutdown (pPool);
        R_Pack_ThreadPoolDestroy (pPool);
        return R_Pack_EncodeInputImages (pEncoder, pInputPaths, mipmapSize);
    }
    memset (ppPixelBuffers, 0, inputCount * sizeof (uint8_t*));
    memset (pResults, 0, inputCount * sizeof (int));
    offset = 0;
    for (size_t i = 0; i < inputCount; ++i)
    {
        char* pPath = (char*)(pInputData + offset);
        offset += strlen (pPath) + 1;

        pTasks[i].pPath = pPath;
        pTasks[i].pImage = &pImages[i];
        pTasks[i].ppPixelBuffer = &ppPixelBuffers[i];
        pTasks[i].pResult = &pResults[i];
        pTasks[i].pPool = pPool;
    }

    R_Pack_ThreadPoolSubmitTasks (pPool, pTasks, (uint32_t)inputCount);
    R_Pack_ThreadPoolWaitAll (pPool);
    R_Pack_ThreadPoolShutdown (pPool);

    for (size_t i = 0; i < inputCount; ++i)
    {
        struct R_Pack_InputImage* pImage = &pImages[i];
        uint8_t*                  pPixelBuffer = ppPixelBuffers[i];
        int                       loadResult = pResults[i];

        const struct R_Pack_ImageLoadTask* pTask = &pTasks[i];
        const char*                        pPath = pTask->pPath;
        R_CSTL_LOG_INFO ("Processing input %zu: %s", i + 1, pPath);

        if (loadResult < 0)
        {
            R_CSTL_LOG_WARN ("Skipping image after load failure: %s", pPath);
            continue;
        }

        if (mipmapSize != 0 && (pImage->width > mipmapSize || pImage->height > mipmapSize))
        {
            uint32_t width = R_Pack_MipmapDimension (pImage->width, pImage->height, mipmapSize);
            uint32_t height = R_Pack_MipmapDimension (pImage->height, pImage->width, mipmapSize);
            uint8_t* pResizedPixels = R_Pack_ResizeImageBox (pImage, width, height);
            if (!pResizedPixels)
            {
                R_CSTL_LOG_WARN ("Skipping mipmap level for %s: resize allocation failed", pPath);
                R_CSTL_HeapFree (pPixelBuffer);
                continue;
            }
            R_CSTL_HeapFree (pPixelBuffer);
            pPixelBuffer = pResizedPixels;
            pImage->pPixels = pResizedPixels;
            pImage->width = width;
            pImage->height = height;
            pImage->stride = width * 4;
        }
        R_Pack_LogImageWarnings (pPath, pImage, &pEncoder->config);

        enum R_Pack_Error err = R_Pack_EncoderAddImage (pEncoder, pImage);
        if (err != R_PACK_OK)
        {
            R_CSTL_LOG_WARN ("Skipping image '%s': %s", pPath, R_Pack_ErrorToString (err));
            if (pPixelBuffer)
            {
                R_CSTL_HeapFree (pPixelBuffer);
            }
            continue;
        }
        successCount++;

        if (pPixelBuffer)
        {
            R_CSTL_HeapFree (pPixelBuffer);
        }
    }

    R_CSTL_HeapFree (pTasks);
    R_CSTL_HeapFree (pImages);
    R_CSTL_HeapFree (ppPixelBuffers);
    R_CSTL_HeapFree (pResults);
    R_Pack_ThreadPoolDestroy (pPool);

    return successCount;
}