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
r_pack_log_image_warnings (
    const char*                           pPath,
    const struct r_pack_input_image*      pImage,
    const struct r_pack_encoder_settings* pSettings)
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
    if (pImage->width > pSettings->maxAtlasWidth || pImage->height > pSettings->maxAtlasHeight)
    {
        R_CSTL_LOG_WARN (
            "Image %s (%ux%u) exceeds the configured atlas limit %ux%u and will be skipped",
            pPath,
            pImage->width,
            pImage->height,
            pSettings->maxAtlasWidth,
            pSettings->maxAtlasHeight);
    }
}

R_PACK_API uint32_t
r_pack_mipmap_dimension (uint32_t source, uint32_t other, uint32_t limit)
{
    uint32_t dimension = source > other ? limit : (uint32_t)(((uint64_t)source * limit) / other);
    return dimension == 0 ? 1 : dimension;
}

R_PACK_API uint8_t*
r_pack_resize_image_box (const struct r_pack_input_image* pSource, uint32_t width, uint32_t height)
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
r_pack_encode_and_write (struct r_pack_encoder* pEncoder, const char* pOutputPath)
{
    uint64_t requiredSize = r_pack_encoder_get_required_size (pEncoder);
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
    enum r_pack_error encodeErr
        = r_pack_encoder_encode (pEncoder, pOutputBuffer, requiredSize, &bytesWritten);
    if (encodeErr != R_PACK_OK)
    {
        R_CSTL_LOG_ERROR ("Encoding failed: %s", r_pack_error_to_string (encodeErr));
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

    struct r_pack_validation_report report;
    if (!r_pack_validate_packed_data (pOutputBuffer, bytesWritten, &report))
    {
        R_CSTL_LOG_ERROR (
            "RPACK validation failed: %s at byte %llu (texture %u, pixel %llu)",
            r_pack_error_to_string (report.error),
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

struct r_pack_image_load_task
{
        const char*                pPath;
        struct r_pack_input_image* pImage;
        uint8_t**                  ppPixelBuffer;
        int*                       pResult;
        struct r_pack_thread_pool* pPool;
};

struct r_pack_thread_pool
{
        struct R_CSTL_Thread**         ppThreads;
        uint32_t                       threadCount;
        R_CSTL_AtomicUint32            nextTaskIndex;
        R_CSTL_AtomicUint32            completedTasks;
        R_CSTL_AtomicUint32            failedTasks;
        struct R_CSTL_Mutex*           pTaskMutex;
        struct R_CSTL_Condition*       pTaskAvailable;
        struct R_CSTL_Condition*       pTaskComplete;
        struct r_pack_image_load_task* pTasks;
        uint32_t                       taskCount;
        int                            shutdown;
};

static struct r_pack_thread_pool* r_pack_thread_pool_create (uint32_t workerCount);

static void r_pack_thread_pool_destroy (struct r_pack_thread_pool* pPool);

static void r_pack_worker_thread_func (void* pData);

static int r_pack_thread_pool_start (struct r_pack_thread_pool* pPool);

static int r_pack_thread_pool_submit_tasks (
    struct r_pack_thread_pool*     pPool,
    struct r_pack_image_load_task* pTasks,
    uint32_t                       taskCount);

static void r_pack_thread_pool_wait_all (struct r_pack_thread_pool* pPool);

static void r_pack_thread_pool_shutdown (struct r_pack_thread_pool* pPool);

R_PACK_API int
r_pack_has_extension (const char* pPath, const char* pExtension)
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
r_pack_make_variant_path (const char* pOutputPath, uint32_t size, char** ppVariantPath)
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
r_pack_encode_mipmap_variants (
    const struct r_pack_encoder_settings* pSettings,
    const struct R_CSTL_Array*            pInputPaths,
    const char*                           pOutputPath)
{
    static const uint32_t mipmapSizes[] = {64, 32, 16, 8, 4, 2, 1};
    size_t                generated = 0;
    for (size_t i = 0; i < sizeof (mipmapSizes) / sizeof (mipmapSizes[0]); ++i)
    {
        char* pVariantPath = NULL;
        if (r_pack_make_variant_path (pOutputPath, mipmapSizes[i], &pVariantPath) != 0)
        {
            R_CSTL_LOG_ERROR (
                "Failed to allocate mipmap output path for %ux%u",
                mipmapSizes[i],
                mipmapSizes[i]);
            return -1;
        }

        struct r_pack_encoder* pVariantEncoder = r_pack_new_encoder (pSettings);
        if (!pVariantEncoder)
        {
            R_CSTL_LOG_ERROR (
                "Failed to create encoder for mipmap level %ux%u",
                mipmapSizes[i],
                mipmapSizes[i]);
            R_CSTL_HeapFree (pVariantPath);
            return -1;
        }

        uint32_t successCount = r_pack_encode_input_images_threaded (
            pVariantEncoder,
            pInputPaths,
            mipmapSizes[i],
            pSettings->workerCount);
        if (successCount == 0 || r_pack_encode_and_write (pVariantEncoder, pVariantPath) != 0)
        {
            R_CSTL_LOG_WARN ("Mipmap level %ux%u was not generated", mipmapSizes[i], mipmapSizes[i]);
            r_pack_delete_encoder (pVariantEncoder);
            R_CSTL_HeapFree (pVariantPath);
            continue;
        }
        R_CSTL_LOG_INFO ("Generated mipmap variant %s (%u images)", pVariantPath, successCount);
        ++generated;
        r_pack_delete_encoder (pVariantEncoder);
        R_CSTL_HeapFree (pVariantPath);
    }
    return generated == 0 ? -1 : 0;
}

static struct r_pack_thread_pool*
r_pack_thread_pool_create (uint32_t workerCount)
{
    struct r_pack_thread_pool* pPool
        = (struct r_pack_thread_pool*)R_CSTL_HeapAlloc (sizeof (struct r_pack_thread_pool));
    if (!pPool)
    {
        return NULL;
    }
    memset (pPool, 0, sizeof (struct r_pack_thread_pool));

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
r_pack_thread_pool_destroy (struct r_pack_thread_pool* pPool)
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
r_pack_load_asset (const char* pPath, struct r_pack_input_image* pImage, uint8_t** ppPixelBuffer)
{
    if (!pPath || !pImage || !ppPixelBuffer)
    {
        return -1;
    }
    *ppPixelBuffer = NULL;
    memset (pImage, 0, sizeof (*pImage));

    if (!r_pack_has_extension (pPath, ".jpg") && !r_pack_has_extension (pPath, ".jpeg"))
    {
        R_CSTL_LOG_WARN ("Skipping unsupported image format: %s (JPEG expected)", pPath);
        return -1;
    }

    struct r_pack_jpeg_image decoded = {0};
    enum r_pack_error        error = r_pack_jpeg_decode_file (pPath, &decoded);
    if (error != R_PACK_OK)
    {
        fprintf (stderr, "JPEG decode failed for %s: %s\n", pPath, r_pack_error_to_string (error));
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
r_pack_worker_thread_func (void* pData)
{
    struct r_pack_thread_pool* pPool = (struct r_pack_thread_pool*)pData;

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
        struct r_pack_image_load_task task = pPool->pTasks[taskIndex];
        R_CSTL_MutexUnlock (pPool->pTaskMutex);

        int loadResult = r_pack_load_asset (task.pPath, task.pImage, task.ppPixelBuffer);
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
r_pack_thread_pool_start (struct r_pack_thread_pool* pPool)
{
    for (uint32_t i = 0; i < pPool->threadCount; ++i)
    {
        pPool->ppThreads[i] = R_CSTL_NewThread (r_pack_worker_thread_func, pPool);
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
r_pack_thread_pool_submit_tasks (
    struct r_pack_thread_pool*     pPool,
    struct r_pack_image_load_task* pTasks,
    uint32_t                       taskCount)
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
r_pack_thread_pool_wait_all (struct r_pack_thread_pool* pPool)
{
    R_CSTL_MutexLock (pPool->pTaskMutex);
    while (R_CSTL_AtomicUint32Load (&pPool->completedTasks) < pPool->taskCount)
    {
        R_CSTL_ConditionWait (pPool->pTaskComplete, pPool->pTaskMutex);
    }
    R_CSTL_MutexUnlock (pPool->pTaskMutex);
}

static void
r_pack_thread_pool_shutdown (struct r_pack_thread_pool* pPool)
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
r_pack_encode_input_images (
    struct r_pack_encoder*     pEncoder,
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
        uint8_t*                  pPixelBuffer = NULL;
        struct r_pack_input_image image = {0};

        int loadResult = r_pack_load_asset (pPath, &image, &pPixelBuffer);
        if (loadResult < 0)
        {
            R_CSTL_LOG_WARN ("Skipping image after load failure: %s", pPath);
            continue;
        }

        if (mipmapSize != 0 && (image.width > mipmapSize || image.height > mipmapSize))
        {
            uint32_t width = r_pack_mipmap_dimension (image.width, image.height, mipmapSize);
            uint32_t height = r_pack_mipmap_dimension (image.height, image.width, mipmapSize);
            uint8_t* pResizedPixels = r_pack_resize_image_box (&image, width, height);
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

        r_pack_log_image_warnings (pPath, &image, &pEncoder->config);

        enum r_pack_error err = r_pack_encoder_add_image (pEncoder, &image);
        if (err != R_PACK_OK)
        {
            R_CSTL_LOG_WARN ("Skipping image '%s': %s", pPath, r_pack_error_to_string (err));
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
r_pack_encode_input_images_threaded (
    struct r_pack_encoder*     pEncoder,
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
        return r_pack_encode_input_images (pEncoder, pInputPaths, mipmapSize);
    }
    struct r_pack_thread_pool* pPool = r_pack_thread_pool_create (workerCount);
    if (!pPool)
    {
        R_CSTL_LOG_WARN ("Failed to create thread pool, falling back to serial processing");
        return r_pack_encode_input_images (pEncoder, pInputPaths, mipmapSize);
    }

    if (r_pack_thread_pool_start (pPool) != 0)
    {
        R_CSTL_LOG_WARN ("Failed to start thread pool, falling back to serial processing");
        r_pack_thread_pool_destroy (pPool);
        return r_pack_encode_input_images (pEncoder, pInputPaths, mipmapSize);
    }

    struct r_pack_image_load_task* pTasks = (struct r_pack_image_load_task*)R_CSTL_HeapAlloc (
        inputCount * sizeof (struct r_pack_image_load_task));
    struct r_pack_input_image* pImages
        = (struct r_pack_input_image*)R_CSTL_HeapAlloc (inputCount * sizeof (struct r_pack_input_image));
    uint8_t** ppPixelBuffers = (uint8_t**)R_CSTL_HeapAlloc (inputCount * sizeof (uint8_t*));
    int*      pResults = (int*)R_CSTL_HeapAlloc (inputCount * sizeof (int));

    if (!pTasks || !pImages || !ppPixelBuffers || !pResults)
    {
        R_CSTL_LOG_ERROR ("Failed to allocate task arrays, falling back to serial processing");
        if (pTasks) R_CSTL_HeapFree (pTasks);
        if (pImages) R_CSTL_HeapFree (pImages);
        if (ppPixelBuffers) R_CSTL_HeapFree (ppPixelBuffers);
        if (pResults) R_CSTL_HeapFree (pResults);
        r_pack_thread_pool_shutdown (pPool);
        r_pack_thread_pool_destroy (pPool);
        return r_pack_encode_input_images (pEncoder, pInputPaths, mipmapSize);
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

    r_pack_thread_pool_submit_tasks (pPool, pTasks, (uint32_t)inputCount);
    r_pack_thread_pool_wait_all (pPool);
    r_pack_thread_pool_shutdown (pPool);

    for (size_t i = 0; i < inputCount; ++i)
    {
        struct r_pack_input_image* pImage = &pImages[i];
        uint8_t*                   pPixelBuffer = ppPixelBuffers[i];
        int                        loadResult = pResults[i];

        const struct r_pack_image_load_task* pTask = &pTasks[i];
        const char*                          pPath = pTask->pPath;
        R_CSTL_LOG_INFO ("Processing input %zu: %s", i + 1, pPath);

        if (loadResult < 0)
        {
            R_CSTL_LOG_WARN ("Skipping image after load failure: %s", pPath);
            continue;
        }

        if (mipmapSize != 0 && (pImage->width > mipmapSize || pImage->height > mipmapSize))
        {
            uint32_t width = r_pack_mipmap_dimension (pImage->width, pImage->height, mipmapSize);
            uint32_t height = r_pack_mipmap_dimension (pImage->height, pImage->width, mipmapSize);
            uint8_t* pResizedPixels = r_pack_resize_image_box (pImage, width, height);
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
        r_pack_log_image_warnings (pPath, pImage, &pEncoder->config);

        enum r_pack_error err = r_pack_encoder_add_image (pEncoder, pImage);
        if (err != R_PACK_OK)
        {
            R_CSTL_LOG_WARN ("Skipping image '%s': %s", pPath, r_pack_error_to_string (err));
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
    r_pack_thread_pool_destroy (pPool);

    return successCount;
}