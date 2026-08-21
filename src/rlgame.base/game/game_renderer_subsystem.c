#include "rlgame.base/game/game_renderer_subsystem.h"
#include "rlgame.base/game/game_platform.h"
#include "rlgame.base/cvulkan/cvulkan_command_buffer.h"
#include "rlgame.base/cvulkan/cvulkan_command_pool.h"
#include "rlgame.base/cvulkan/cvulkan_semaphore.h"
#include "rlgame.base/cvulkan/cvulkan_fence.h"
#include "rlgame.base/cvulkan/cvulkan_swapchain.h"
#include "rlgame.base/cvulkan/cvulkan_queue.h"
#include "rlgame.base/cvulkan/cvulkan_device.h"
#include "rlgame.base/cstl/cstl_log.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"
#include "rlgame.base/cstl/cstl_string.h"
#include "rlgame.base/cstl/cstl_bytecode.h"
#include "rlgame.base/cstl/cstl_array.h"

#include <string.h>
#include <stdlib.h>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <pthread.h>
#endif

struct R_GameRendererFrame
{
        struct R_CSTL_Array*            pCommandBufferArray;
        struct R_CVulkan_Semaphore*     pRenderFinishedSemaphore;
        struct R_CVulkan_Fence*         pInFlightFence;
        uint32_t                        resourceIndex;
        uint64_t                        frameNumber;
        R_GAME_DEBUG_FIELD
};

struct R_GameRendererLayer
{
        char*                           pName;
        uint32_t                        priority;
        uint32_t                        flags;
        const void*                     pUserData;
        R_GameLifecycleRender           renderCallback;
        R_GameLifecycleBeforePass       beforePassCallback;
        R_GameLifecycleAfterPass        afterPassCallback;
        R_GAME_DEBUG_FIELD
};

struct R_GameRendererResource
{
        uint64_t                handle;
        uint32_t                type;
        const void*             pResource;
        uint64_t                size;
        uint32_t                refCount;
        const char*             pName;
        R_GAME_DEBUG_FIELD
};

struct R_GameRendererLifecycle {
        const void*                     pRenderer;
        R_GameLifecycleConstruct        constructCallback;
        R_GameLifecycleResume           resumeCallback;
        R_GameLifecyclePause            pauseCallback;
        R_GameLifecycleBeforeEach       beforeEachCallback;
        R_GameLifecycleAfterEach        afterEachCallback;
        R_GameLifecycleBeforePass       beforePassCallback;
        R_GameLifecycleAfterPass        afterPassCallback;
        R_GameLifecycleRender           renderCallback;
        R_GameLifecycleStop             stopCallback;
        R_GameLifecycleOver             overCallback;
        R_GAME_DEBUG_FIELD
};

struct R_GameRendererSubsystem
{
        struct R_GameCVulkan_PipelineContext* pPipelineContext;
        struct R_GameRendererLifecycle        lifecycle;
        
        struct R_GameRendererFrame*           pFrames;
        uint32_t                              maxFramesInFlight;
        uint32_t                              currentFrameIndex;
        uint64_t                              frameCounter;
        
        struct R_CSTL_Array*                  pLayerArray;
        struct R_CSTL_Array*                  pResourceArray;
        uint64_t                              nextResourceHandle;
        
        struct R_GameRendererThreadPool*      pThreadPool;
        R_GAME_MUTEX                          layerArrayMutex;
        R_GAME_MUTEX                          resourceArrayMutex;
        
        uint32_t                              state;
        R_GAME_DEBUG_FIELD
};

static struct R_GameRendererLifecycle g_lifecycle = {0};
static struct R_CSTL_BytecodeDecoder* g_pBytecodeDecoder = NULL;

static int
R_GameRenderer_CompareLayers (const void* pA, const void* pB, void* pData)
{
        const struct R_GameRendererLayer* pLayerA = (const struct R_GameRendererLayer*)pA;
        const struct R_GameRendererLayer* pLayerB = (const struct R_GameRendererLayer*)pB;
        return (int)pLayerA->priority - (int)pLayerB->priority;
}

static void R_GameRenderer_ShutdownThreadPool (struct R_GameRendererSubsystem* pSubsystem);

static int
R_GameRenderer_InitializeArrays (struct R_GameRendererSubsystem* pSubsystem)
{
        pSubsystem->pFrames = (struct R_GameRendererFrame*)R_CSTL_HeapAlloc (
            sizeof (struct R_GameRendererFrame) * pSubsystem->maxFramesInFlight);
        if (pSubsystem->pFrames == NULL)
        {
                R_CSTL_LOG_ERROR ("Failed to allocate frame data");
                return -1;
        }
        memset (pSubsystem->pFrames, 0,
                sizeof (struct R_GameRendererFrame) * pSubsystem->maxFramesInFlight);

        pSubsystem->pLayerArray = R_CSTL_NewArrayWithCapacity (
            sizeof (struct R_GameRendererLayer) * R_GAME_RENDERER_MAX_LAYERS);
        if (pSubsystem->pLayerArray == NULL)
        {
                R_CSTL_LOG_ERROR ("Failed to allocate render layer array");
                R_CSTL_HeapFree (pSubsystem->pFrames);
                return -1;
        }

        pSubsystem->pResourceArray = R_CSTL_NewArrayWithCapacity (
            sizeof (struct R_GameRendererResource) * R_GAME_RENDERER_MAX_RESOURCES);
        if (pSubsystem->pResourceArray == NULL)
        {
                R_CSTL_LOG_ERROR ("Failed to allocate resource array");
                R_CSTL_DeleteArray (pSubsystem->pLayerArray);
                R_CSTL_HeapFree (pSubsystem->pFrames);
                return -1;
        }

        return 0;
}

static void
R_GameRenderer_InitializeState (struct R_GameRendererSubsystem* pSubsystem)
{
        pSubsystem->currentFrameIndex = 0;
        pSubsystem->frameCounter = 0;
        pSubsystem->state = R_GAME_RENDERER_STATE_STOPPED;
        pSubsystem->nextResourceHandle = 1;

        R_GAME_MUTEX_INIT (&pSubsystem->layerArrayMutex);
        R_GAME_MUTEX_INIT (&pSubsystem->resourceArrayMutex);
}

static void
R_GameRenderer_CleanupArrays (struct R_GameRendererSubsystem* pSubsystem)
{
        if (pSubsystem->pFrames != NULL)
        {
                R_CSTL_HeapFree (pSubsystem->pFrames);
                pSubsystem->pFrames = NULL;
        }
        if (pSubsystem->pLayerArray != NULL)
        {
                R_CSTL_DeleteArray (pSubsystem->pLayerArray);
                pSubsystem->pLayerArray = NULL;
        }
        if (pSubsystem->pResourceArray != NULL)
        {
                R_CSTL_DeleteArray (pSubsystem->pResourceArray);
                pSubsystem->pResourceArray = NULL;
        }
}

static void
R_GameRenderer_CleanupLayers (struct R_GameRendererSubsystem* pSubsystem)
{
        if (pSubsystem->pLayerArray != NULL)
        {
                size_t layerCount = R_CSTL_ArrayLength (pSubsystem->pLayerArray) / 
                    sizeof (struct R_GameRendererLayer);
                
                for (size_t i = 0; i < layerCount; ++i)
                {
                        struct R_GameRendererLayer layer;
                        R_CSTL_ArrayTypedUncheckedAt (pSubsystem->pLayerArray, struct R_GameRendererLayer, i, &layer);
                        if (layer.pName != NULL)
                        {
                                R_CSTL_HeapFree (layer.pName);
                        }
                }
                R_CSTL_DeleteArray (pSubsystem->pLayerArray);
                pSubsystem->pLayerArray = NULL;
        }
}

static void
R_GameRenderer_CleanupResources (struct R_GameRendererSubsystem* pSubsystem)
{
        if (pSubsystem->pResourceArray != NULL)
        {
                size_t resourceCount = R_CSTL_ArrayLength (pSubsystem->pResourceArray) / 
                    sizeof (struct R_GameRendererResource);
                
                for (size_t i = 0; i < resourceCount; ++i)
                {
                        struct R_GameRendererResource resource;
                        R_CSTL_ArrayTypedUncheckedAt (pSubsystem->pResourceArray, struct R_GameRendererResource, i, &resource);
                        if (resource.pName != NULL)
                        {
                                R_CSTL_HeapFree ((void*)resource.pName);
                        }
                }
                R_CSTL_DeleteArray (pSubsystem->pResourceArray);
                pSubsystem->pResourceArray = NULL;
        }
}

static char*
R_GameRenderer_AllocateNameCopy (const char* pName)
{
        if (pName == NULL)
        {
                return NULL;
        }
        size_t nameLen = strlen (pName);
        char* pNameCopy = (char*)R_CSTL_HeapAlloc (nameLen + 1);
        if (pNameCopy != NULL)
        {
                memcpy (pNameCopy, pName, nameLen);
                pNameCopy[nameLen] = '\0';
        }
        return pNameCopy;
}

static int
R_GameRenderer_RemoveFromArrayByIndex (
    struct R_CSTL_Array** ppArray,
    size_t                elementSize,
    size_t                removeIndex)
{
        struct R_CSTL_Array* pArray = *ppArray;
        size_t currentCount = R_CSTL_ArrayLength (pArray) / elementSize;
        if (removeIndex >= currentCount)
        {
                return -1;
        }

        struct R_CSTL_Array* pNewArray = R_CSTL_NewArrayWithCapacity (
            elementSize * (currentCount - 1));
        if (pNewArray == NULL)
        {
                return -1;
        }

        for (size_t i = 0; i < currentCount; ++i)
        {
                if (i == removeIndex)
                {
                        continue;
                }
                
                const uint8_t* pOldData = R_CSTL_ArrayData (pArray);
                int result = R_CSTL_ArrayPushData (
                    pNewArray,
                    pOldData + i * elementSize,
                    elementSize);
                if (result != R_CSTL_OK)
                {
                        R_CSTL_DeleteArray (pNewArray);
                        return -1;
                }
        }

        R_CSTL_DeleteArray (pArray);
        *ppArray = pNewArray;
        return 0;
}

static size_t
R_GameRenderer_FindResourceIndexByHandle (
    struct R_GameRendererSubsystem* pSubsystem,
    uint64_t                        handle)
{
        size_t resourceCount = R_CSTL_ArrayLength (pSubsystem->pResourceArray) / 
            sizeof (struct R_GameRendererResource);
        
        for (size_t i = 0; i < resourceCount; ++i)
        {
                struct R_GameRendererResource resource;
                R_CSTL_ArrayTypedUncheckedAt (pSubsystem->pResourceArray, struct R_GameRendererResource, i, &resource);
                if (resource.handle == handle)
                {
                        return i;
                }
        }

        return SIZE_MAX;
}

static int
R_GameRenderer_RemoveResourceByHandle (
    struct R_GameRendererSubsystem* pSubsystem,
    uint64_t                        handle)
{
        size_t index = R_GameRenderer_FindResourceIndexByHandle (pSubsystem, handle);
        if (index == SIZE_MAX)
        {
                return -1;
        }

        return R_GameRenderer_RemoveFromArrayByIndex (
            &pSubsystem->pResourceArray,
            sizeof (struct R_GameRendererResource),
            index);
}

static int
R_GameRenderer_WaitAndResetFence (
    struct R_CVulkan_Device*       pDevice,
    struct R_CVulkan_Fence*        pFence)
{
        enum R_CVulkan_Error err = R_CVulkan_FenceWait (
            pDevice,
            pFence,
            1,
            true,
            UINT64_MAX);
        if (err != R_CVULKAN_OK)
        {
                R_CSTL_LOG_ERROR ("Failed to wait for fence: %s", R_CVulkan_ErrorToString (err));
                return -1;
        }
        err = R_CVulkan_FenceReset (pDevice, pFence, 1);
        if (err != R_CVULKAN_OK)
        {
                R_CSTL_LOG_ERROR ("Failed to reset fence: %s", R_CVulkan_ErrorToString (err));
                return -1;
        }

        return 0;
}

static int
R_GameRenderer_AcquireSwapchainImage (
    struct R_GameCVulkan_PipelineContext* pPipelineContext,
    uint32_t*                             pImageIndex)
{
        enum R_CVulkan_Error err = R_CVulkan_SwapchainAcquireNextImage (
            &pPipelineContext->swapchain,
            UINT64_MAX,
            R_CVulkan_SemaphoreGetHandle (&pPipelineContext->imageAvailableSemaphore),
            VK_NULL_HANDLE,
            pImageIndex);
        if (err != R_CVULKAN_OK)
        {
                R_CSTL_LOG_ERROR ("Failed to acquire next image: %s", R_CVulkan_ErrorToString (err));
                return -1;
        }
        return 0;
}

static int
R_GameRenderer_RenderLayer (
    struct R_GameRendererLayer*     pLayer,
    struct R_CVulkan_CommandBuffer* pCmdBuffer)
{
        enum R_CVulkan_Error err = R_CVulkan_BeginCommandBuffer (pCmdBuffer, 0, NULL);
        if (err != R_CVULKAN_OK)
        {
                R_CSTL_LOG_ERROR (
                    "Failed to begin command buffer: %s",
                    R_CVulkan_ErrorToString (err));
                return -1;
        }

        if (pLayer->beforePassCallback != NULL)
        {
                pLayer->beforePassCallback ((void*)pLayer, pCmdBuffer, sizeof (*pCmdBuffer));
        }

        if (pLayer->renderCallback != NULL)
        {
                pLayer->renderCallback ((void*)pLayer, pCmdBuffer, sizeof (*pCmdBuffer));
        }

        if (pLayer->afterPassCallback != NULL)
        {
                pLayer->afterPassCallback ((void*)pLayer, pCmdBuffer, sizeof (*pCmdBuffer));
        }

        err = R_CVulkan_EndCommandBuffer (pCmdBuffer);
        if (err != R_CVULKAN_OK)
        {
                R_CSTL_LOG_ERROR (
                    "Failed to end command buffer: %s",
                    R_CVulkan_ErrorToString (err));
                return -1;
        }

        return 0;
}

static int
R_GameRenderer_SubmitCommandBuffers (
    struct R_CVulkan_Queue*               pGraphicsQueue,
    struct R_CVulkan_CommandBuffer**      ppCommandBuffers,
    size_t                                commandBufferCount,
    struct R_CVulkan_Semaphore*          pImageAvailableSemaphore,
    struct R_CVulkan_Semaphore*          pRenderFinishedSemaphore,
    struct R_CVulkan_Fence*               pInFlightFence)
{
        VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        enum R_CVulkan_Error err = R_CVulkan_QueueSubmit (
            pGraphicsQueue,
            *ppCommandBuffers,
            commandBufferCount,
            pImageAvailableSemaphore,
            1,
            &waitStages,
            pRenderFinishedSemaphore,
            1,
            pInFlightFence);
        if (err != R_CVULKAN_OK)
        {
                R_CSTL_LOG_ERROR ("Failed to submit command buffers: %s", R_CVulkan_ErrorToString (err));
                return -1;
        }
        return 0;
}

static int
R_GameRenderer_PresentImage (
    struct R_CVulkan_Queue*       pPresentQueue,
    VkSwapchainKHR               swapchainHandle,
    uint32_t                    imageIndex,
    struct R_CVulkan_Semaphore*  pRenderFinishedSemaphore)
{
        enum R_CVulkan_Error err = R_CVulkan_QueuePresent (
            pPresentQueue,
            &swapchainHandle,
            1,
            &imageIndex,
            pRenderFinishedSemaphore,
            1);
        if (err != R_CVULKAN_OK)
        {
                R_CSTL_LOG_ERROR ("Failed to present image: %s", R_CVulkan_ErrorToString (err));
                return -1;
        }
        return 0;
}

#if defined(_WIN32)
static DWORD WINAPI
R_GameRenderer_WorkerThreadProc (LPVOID pParam)
#elif defined(__linux__) || defined(__APPLE__)
static void*
R_GameRenderer_WorkerThreadProc (void* pParam)
#endif
{
        struct R_GameRendererWorkerThread* pWorker = (struct R_GameRendererWorkerThread*)pParam;
        struct R_GameRendererSubsystem* pSubsystem = pWorker->pSubsystem;
        struct R_GameRendererThreadPool* pPool = pSubsystem->pThreadPool;

        while (true)
        {
                int shutdown = R_GAME_ATOMIC_LOAD (&pPool->atomicShutdownRequested);
                if (shutdown)
                {
                        break;
                }
                R_GAME_MUTEX_LOCK (&pPool->taskMutex);
                
                uint32_t pendingTasks = R_GAME_ATOMIC_LOAD (&pPool->atomicPendingTasks);
                while (pendingTasks == 0)
                {
                        shutdown = R_GAME_ATOMIC_LOAD (&pPool->atomicShutdownRequested);
                        if (shutdown)
                        {
                                R_GAME_MUTEX_UNLOCK (&pPool->taskMutex);
                                goto r_exit;
                        }
                        R_GAME_COND_WAIT (&pPool->taskAvailable, &pPool->taskMutex);
                        pendingTasks = R_GAME_ATOMIC_LOAD (&pPool->atomicPendingTasks);
                }
                size_t taskCount = R_CSTL_ArrayLength (pPool->pTaskQueue) / sizeof (struct R_GameRendererRenderTask);
                if (taskCount == 0)
                {
                        R_GAME_MUTEX_UNLOCK (&pPool->taskMutex);
                        continue;
                }

                struct R_GameRendererRenderTask task;
                R_CSTL_ArrayTypedUncheckedAt (pPool->pTaskQueue, struct R_GameRendererRenderTask, 0, &task);
                
                struct R_CSTL_Array* pNewQueue = R_CSTL_NewArrayWithCapacity (
                    R_CSTL_ArrayLength (pPool->pTaskQueue) - sizeof (struct R_GameRendererRenderTask));
                if (pNewQueue != NULL)
                {
                        const uint8_t* pOldData = R_CSTL_ArrayData (pPool->pTaskQueue);
                        size_t oldSize = R_CSTL_ArrayLength (pPool->pTaskQueue);
                        if (oldSize > sizeof (struct R_GameRendererRenderTask))
                        {
                                R_CSTL_ArrayPushData (pNewQueue, pOldData + sizeof (struct R_GameRendererRenderTask), 
                                    oldSize - sizeof (struct R_GameRendererRenderTask));
                        }
                        R_CSTL_DeleteArray (pPool->pTaskQueue);
                        pPool->pTaskQueue = pNewQueue;
                }

                R_GAME_ATOMIC_DECREMENT (&pPool->atomicPendingTasks);
                R_GAME_MUTEX_UNLOCK (&pPool->taskMutex);

                struct R_GameRendererFrame* pFrame = &pSubsystem->pFrames[task.frameIndex];
                struct R_GameRendererLayer layer;
                R_GAME_MUTEX_LOCK (&pSubsystem->layerArrayMutex);
                R_CSTL_ArrayTypedAt (pSubsystem->pLayerArray, struct R_GameRendererLayer, task.layerIndex, &layer);
                R_GAME_MUTEX_UNLOCK (&pSubsystem->layerArrayMutex);

                struct R_CVulkan_CommandBuffer cmdBuffer;
                R_CSTL_ArrayTypedUncheckedAt (pFrame->pCommandBufferArray, struct R_CVulkan_CommandBuffer, 
                    task.commandBufferIndex, &cmdBuffer);

                enum R_CVulkan_Error err = R_CVulkan_BeginCommandBuffer (&cmdBuffer, 0, NULL);
                if (err == R_CVULKAN_OK)
                {
                        if (layer.beforePassCallback != NULL)
                        {
                                layer.beforePassCallback ((void*)&layer, &cmdBuffer, sizeof (cmdBuffer));
                        }

                        if (layer.renderCallback != NULL)
                        {
                                layer.renderCallback ((void*)&layer, &cmdBuffer, sizeof (cmdBuffer));
                        }

                        if (layer.afterPassCallback != NULL)
                        {
                                layer.afterPassCallback ((void*)&layer, &cmdBuffer, sizeof (cmdBuffer));
                        }

                        R_CVulkan_EndCommandBuffer (&cmdBuffer);
                        R_CSTL_ArrayTypedSetAtUnchecked (pFrame->pCommandBufferArray, struct R_CVulkan_CommandBuffer, 
                            task.commandBufferIndex, &cmdBuffer);
                }
                R_GAME_ATOMIC_INCREMENT (&pPool->atomicCompletedTasks);
                R_GAME_COND_SIGNAL (&pPool->taskComplete);
        }

r_exit:
        R_GAME_ATOMIC_STORE (&pWorker->atomicIsRunning, 0);
#if defined(_WIN32)
        return 0;
#else
        return NULL;
#endif
}

static int
R_GameRenderer_InitializeThreadPool (struct R_GameRendererSubsystem* pSubsystem)
{
        pSubsystem->pThreadPool = (struct R_GameRendererThreadPool*)R_CSTL_HeapAlloc (
            sizeof (struct R_GameRendererThreadPool));
        if (pSubsystem->pThreadPool == NULL)
        {
                R_CSTL_LOG_ERROR ("Failed to allocate thread pool");
                return -1;
        }
        memset (pSubsystem->pThreadPool, 0, sizeof (struct R_GameRendererThreadPool));

        struct R_GameRendererThreadPool* pPool = pSubsystem->pThreadPool;
        
        R_GAME_MUTEX_INIT (&pPool->taskMutex);
        R_GAME_COND_INIT (&pPool->taskAvailable);
        R_GAME_COND_INIT (&pPool->taskComplete);

        pPool->pTaskQueue = R_CSTL_NewArrayWithCapacity (
            sizeof (struct R_GameRendererRenderTask) * R_GAME_RENDERER_MAX_COMMAND_BUFFERS_PER_FRAME);
        if (pPool->pTaskQueue == NULL)
        {
                R_CSTL_LOG_ERROR ("Failed to allocate task queue");
                R_CSTL_HeapFree (pSubsystem->pThreadPool);
                return -1;
        }

        R_GAME_ATOMIC_STORE (&pPool->atomicPendingTasks, 0);
        R_GAME_ATOMIC_STORE (&pPool->atomicCompletedTasks, 0);
        R_GAME_ATOMIC_STORE (&pPool->atomicShutdownRequested, 0);

        uint32_t workerCount = R_GAME_RENDERER_MAX_WORKER_THREADS;
        pPool->workerCount = workerCount;

        for (uint32_t i = 0; i < workerCount; ++i)
        {
                struct R_GameRendererWorkerThread* pWorker = &pPool->workers[i];
                pWorker->workerIndex = i;
                pWorker->pSubsystem = pSubsystem;
                R_GAME_ATOMIC_STORE (&pWorker->atomicIsRunning, 1);

#if defined(_WIN32)
                pWorker->threadHandle = CreateThread (NULL, 0, R_GameRenderer_WorkerThreadProc, pWorker, 0, &pWorker->threadId);
                if (pWorker->threadHandle == NULL)
                {
                        pPool->workerCount = i;
                        R_GameRenderer_ShutdownThreadPool (pSubsystem);
                        return -1;
                }
#elif defined(__linux__) || defined(__APPLE__)
                int result = pthread_create (&pWorker->threadHandle, NULL, R_GameRenderer_WorkerThreadProc, pWorker);
                if (result != 0)
                {
                        pPool->workerCount = i;
                        R_GameRenderer_ShutdownThreadPool (pSubsystem);
                        return -1;
                }
                pWorker->threadId = (pthread_t)pWorker->threadHandle;
#endif
        }
        R_CSTL_LOG_INFO ("Thread pool initialized with %u workers", workerCount);
        return 0;
}

static void
R_GameRenderer_ShutdownThreadPool (struct R_GameRendererSubsystem* pSubsystem)
{
        if (pSubsystem->pThreadPool == NULL)
        {
                return;
        }

        struct R_GameRendererThreadPool* pPool = pSubsystem->pThreadPool;

        R_GAME_ATOMIC_STORE (&pPool->atomicShutdownRequested, 1);
        R_GAME_COND_BROADCAST (&pPool->taskAvailable);

        for (uint32_t i = 0; i < pPool->workerCount; ++i)
        {
                struct R_GameRendererWorkerThread* pWorker = &pPool->workers[i];
                int isRunning = R_GAME_ATOMIC_LOAD (&pWorker->atomicIsRunning);
                if (isRunning)
                {
#if defined(_WIN32)
                        WaitForSingleObject (pWorker->threadHandle, INFINITE);
                        CloseHandle (pWorker->threadHandle);
#elif defined(__linux__) || defined(__APPLE__)
                        pthread_join (pWorker->threadHandle, NULL);
#endif
                }
        }
        R_GAME_MUTEX_DESTROY (&pPool->taskMutex);
        R_GAME_COND_DESTROY (&pPool->taskAvailable);
        R_GAME_COND_DESTROY (&pPool->taskComplete);

        if (pPool->pTaskQueue != NULL)
        {
                R_CSTL_DeleteArray (pPool->pTaskQueue);
        }

        R_CSTL_HeapFree (pSubsystem->pThreadPool);
        pSubsystem->pThreadPool = NULL;

        R_CSTL_LOG_INFO ("Thread pool shutdown complete");
}

static int
R_GameRenderer_InitializeBytecodeDecoder (void)
{
#if defined(R_GAME_DEBUG)
        if (g_pBytecodeDecoder != NULL)
        {
                return 0;
        }

        enum R_CSTL_BytecodeArchitecture arch = R_CSTL_BYTECODE_ARCH_X86_64;
#if defined(R_CVULKAN_PLATFORM_WINDOWS) || defined(R_CVULKAN_PLATFORM_LINUX)
        arch = R_CSTL_BYTECODE_ARCH_X86_64;
#elif defined(R_CVULKAN_PLATFORM_ANDROID)
        arch = R_CSTL_BYTECODE_ARCH_ARMEABI_V7A;
#elif defined(R_CVULKAN_PLATFORM_MACOS)
        arch = R_CSTL_BYTECODE_ARCH_ARMV8A;
#endif

        g_pBytecodeDecoder = (struct R_CSTL_BytecodeDecoder*)R_CSTL_HeapAlloc (
            sizeof (struct R_CSTL_BytecodeDecoder));
        if (g_pBytecodeDecoder == NULL)
        {
                R_CSTL_LOG_ERROR ("Failed to allocate bytecode decoder");
                return -1;
        }

        int result = R_CSTL_BytecodeDecoderCreate (arch, g_pBytecodeDecoder);
        if (result != 0)
        {
                R_CSTL_LOG_ERROR ("Failed to create bytecode decoder");
                R_CSTL_HeapFree (g_pBytecodeDecoder);
                g_pBytecodeDecoder = NULL;
                return -1;
        }

        R_CSTL_LOG_INFO ("Bytecode decoder initialized for validation");
#endif
        return 0;
}

static void
R_GameRenderer_ShutdownBytecodeDecoder (void)
{
#if defined(R_CSTL_LOG_DEVMODE)
        if (g_pBytecodeDecoder != NULL)
        {
                R_CSTL_DeleteBytecodeDecoder (g_pBytecodeDecoder);
                R_CSTL_HeapFree (g_pBytecodeDecoder);
                g_pBytecodeDecoder = NULL;
        }
#endif
}

static int
R_GameRenderer_ValidateCallbackFunction (
    R_CSTL_BytecodeFunction pFunction,
    const char*             pRequiredSymbol)
{
#if defined(R_GAME_DEBUG)
        if (g_pBytecodeDecoder == NULL || pFunction == NULL || pRequiredSymbol == NULL)
        {
                return 0;
        }
        int found = 0;
        int result = R_CSTL_BytecodeFunctionContainsSymbol (
            g_pBytecodeDecoder,
            pFunction,
            4096,
            pRequiredSymbol,
            &found);
        
        if (result == 0 && found)
        {
                R_CSTL_LOG_INFO ("Validation passed: function contains required symbol '%s'", pRequiredSymbol);
                return 1;
        }
        else
        {
                R_CSTL_LOG_WARN (
                    "Validation failed: function does not contain required symbol call '%s'",
                    pRequiredSymbol);
                return 0;
        }
#else
        (void)pFunction;
        (void)pRequiredSymbol;
        return 1;
#endif
}

static int
R_GameRenderer_InitializeCommandBuffers (
    struct R_GameRendererFrame*           pFrame,
    uint32_t                            frameIdx,
    struct R_CVulkan_Device*           pDevice,
    struct R_CVulkan_CommandPool*      pGraphicsPool)
{
        pFrame->pCommandBufferArray = R_CSTL_NewArrayWithCapacity (
            sizeof (struct R_CVulkan_CommandBuffer) * R_GAME_RENDERER_MAX_COMMAND_BUFFERS_PER_FRAME);
        if (pFrame->pCommandBufferArray == NULL)
        {
                R_CSTL_LOG_ERROR ("Failed to allocate command buffer array for frame %u", frameIdx);
                return -1;
        }

        for (uint32_t bufIdx = 0; bufIdx < R_GAME_RENDERER_MAX_COMMAND_BUFFERS_PER_FRAME; ++bufIdx)
        {
                struct R_CVulkan_CommandBuffer cmdBuffer = {0};
                enum R_CVulkan_Error err = R_CVulkan_NewCommandBuffer (
                    &cmdBuffer,
                    pDevice,
                    R_CVulkan_CommandPoolGetHandle (pGraphicsPool),
                    VK_COMMAND_BUFFER_LEVEL_PRIMARY);
                if (err != R_CVULKAN_OK)
                {
                        R_CSTL_LOG_ERROR (
                            "Failed to create command buffer %u for frame %u: %s",
                            bufIdx,
                            frameIdx,
                            R_CVulkan_ErrorToString (err));
                        return -1;
                }
                
                int result = R_CSTL_ArrayPushData (
                    pFrame->pCommandBufferArray,
                    (const uint8_t*)&cmdBuffer,
                    sizeof (struct R_CVulkan_CommandBuffer));
                if (result != R_CSTL_OK)
                {
                        R_CSTL_LOG_ERROR ("Failed to push command buffer to array");
                        return -1;
                }
        }

        return 0;
}

static int
R_GameRenderer_InitializeSemaphore (
    struct R_GameRendererFrame*   pFrame,
    uint32_t                    frameIdx,
    struct R_CVulkan_Device*   pDevice)
{
        pFrame->pRenderFinishedSemaphore =
            (struct R_CVulkan_Semaphore*)R_CSTL_HeapAlloc (sizeof (struct R_CVulkan_Semaphore));
        if (pFrame->pRenderFinishedSemaphore == NULL)
        {
                R_CSTL_LOG_ERROR ("Failed to allocate semaphore for frame %u", frameIdx);
                return -1;
        }
        memset (pFrame->pRenderFinishedSemaphore, 0, sizeof (struct R_CVulkan_Semaphore));

        enum R_CVulkan_Error err = R_CVulkan_NewSemaphore (
            pFrame->pRenderFinishedSemaphore, pDevice, 0, 0);
        if (err != R_CVULKAN_OK)
        {
                R_CSTL_LOG_ERROR (
                    "Failed to create semaphore for frame %u: %s",
                    frameIdx,
                    R_CVulkan_ErrorToString (err));
                return -1;
        }

        return 0;
}

static int
R_GameRenderer_InitializeFence (
    struct R_GameRendererFrame*   pFrame,
    uint32_t                    frameIdx,
    struct R_CVulkan_Device*   pDevice)
{
        pFrame->pInFlightFence =
            (struct R_CVulkan_Fence*)R_CSTL_HeapAlloc (sizeof (struct R_CVulkan_Fence));
        if (pFrame->pInFlightFence == NULL)
        {
                R_CSTL_LOG_ERROR ("Failed to allocate fence for frame %u", frameIdx);
                return -1;
        }
        memset (pFrame->pInFlightFence, 0, sizeof (struct R_CVulkan_Fence));

        enum R_CVulkan_Error err = R_CVulkan_NewFence (pFrame->pInFlightFence, pDevice, true);
        if (err != R_CVULKAN_OK)
        {
                R_CSTL_LOG_ERROR (
                    "Failed to create fence for frame %u: %s",
                    frameIdx,
                    R_CVulkan_ErrorToString (err));
                return -1;
        }

        return 0;
}

static int
R_GameRenderer_InitializeFrame (
    struct R_GameRendererSubsystem*       pSubsystem,
    struct R_GameCVulkan_PipelineContext*  pPipelineContext)
{
        struct R_CVulkan_Device* pDevice = R_GameCVulkan_PipelineContextGetDevice (pPipelineContext);
        struct R_CVulkan_CommandPool* pGraphicsPool =
            R_GameCVulkan_PipelineContextGetGraphicsCommandPool (pPipelineContext);

        for (uint32_t frameIdx = 0; frameIdx < pSubsystem->maxFramesInFlight; ++frameIdx)
        {
                struct R_GameRendererFrame* pFrame = &pSubsystem->pFrames[frameIdx];

                if (R_GameRenderer_InitializeCommandBuffers (pFrame, frameIdx, pDevice, pGraphicsPool) != 0)
                {
                        return -1;
                }

                if (R_GameRenderer_InitializeSemaphore (pFrame, frameIdx, pDevice) != 0)
                {
                        return -1;
                }

                if (R_GameRenderer_InitializeFence (pFrame, frameIdx, pDevice) != 0)
                {
                        return -1;
                }

                pFrame->frameNumber = 0;
                pFrame->resourceIndex = 0;
#if defined(R_GAME_DEBUG)
                pFrame->isInitialized = true;
#endif
        }

        R_CSTL_LOG_INFO ("Frame data initialized for %u frames", pSubsystem->maxFramesInFlight);
        return 0;
}

static void
R_GameRenderer_CleanupFrame (struct R_GameRendererSubsystem* pSubsystem)
{
        if (pSubsystem->pFrames == NULL)
        {
                return;
        }
        for (uint32_t i = 0; i < pSubsystem->maxFramesInFlight; ++i)
        {
                struct R_GameRendererFrame* pFrame = &pSubsystem->pFrames[i];
                if (pFrame->pCommandBufferArray != NULL)
                {
                        size_t cmdBufferCount = R_CSTL_ArrayLength (pFrame->pCommandBufferArray) / 
                            sizeof (struct R_CVulkan_CommandBuffer);
                        
                        for (size_t j = 0; j < cmdBufferCount; ++j)
                        {
                                struct R_CVulkan_CommandBuffer cmdBuffer;
                                R_CSTL_ArrayTypedUncheckedAt (pFrame->pCommandBufferArray, struct R_CVulkan_CommandBuffer, j, &cmdBuffer);
                                R_CVulkan_DeleteCommandBuffer (&cmdBuffer);
                        }
                        
                        R_CSTL_DeleteArray (pFrame->pCommandBufferArray);
                        pFrame->pCommandBufferArray = NULL;
                }
                if (pFrame->pRenderFinishedSemaphore != NULL)
                {
                        R_CVulkan_DeleteSemaphore (pFrame->pRenderFinishedSemaphore);
                        R_CSTL_HeapFree (pFrame->pRenderFinishedSemaphore);
                }
                if (pFrame->pInFlightFence != NULL)
                {
                        R_CVulkan_DeleteFence (pFrame->pInFlightFence);
                        R_CSTL_HeapFree (pFrame->pInFlightFence);
                }
        }

        R_CSTL_HeapFree (pSubsystem->pFrames);
        pSubsystem->pFrames = NULL;
}

GAME_API struct R_GameRendererSubsystem*
R_GameRenderer_NewSubsystem (struct R_GameRendererSubsystem* pSubsystem)
{
#if defined(R_GAME_DEBUG)
        if (pSubsystem == NULL)
        {
                pSubsystem = (struct R_GameRendererSubsystem*)R_CSTL_HeapAlloc (
                    sizeof (struct R_GameRendererSubsystem));
                if (pSubsystem == NULL)
                {
                        R_CSTL_LOG_ERROR ("Failed to allocate renderer subsystem");
                        return NULL;
                }
        }
#endif
        memset (pSubsystem, 0, sizeof (struct R_GameRendererSubsystem));

        if (R_GameRenderer_InitializeBytecodeDecoder () != 0)
        {
                        R_CSTL_LOG_WARN ("Bytecode decoder initialization failed, validation disabled");
        }

        pSubsystem->maxFramesInFlight = R_GAME_RENDERER_MAX_FRAMES_IN_FLIGHT;

        if (R_GameRenderer_InitializeArrays (pSubsystem) != 0)
        {
                R_CSTL_HeapFree (pSubsystem);
                return NULL;
        }

        R_GameRenderer_InitializeState (pSubsystem);

        if (R_GameRenderer_InitializeThreadPool (pSubsystem) != 0)
        {
                R_CSTL_LOG_ERROR ("Failed to initialize thread pool");
                R_GAME_MUTEX_DESTROY (&pSubsystem->layerArrayMutex);
                R_GAME_MUTEX_DESTROY (&pSubsystem->resourceArrayMutex);
                R_GameRenderer_CleanupArrays (pSubsystem);
                R_CSTL_HeapFree (pSubsystem);
                return NULL;
        }

#if defined(R_GAME_DEBUG)
        pSubsystem->isInitialized = true;
#endif

        R_CSTL_LOG_INFO ("Renderer subsystem created successfully");
        return pSubsystem;
}

GAME_API int
R_GameRenderer_DeleteSubsystem (struct R_GameRendererSubsystem* pSubsystem)
{
#if defined(R_GAME_DEBUG)
        if (pSubsystem == NULL)
        {
                return -1;
        }

        if (!pSubsystem->isInitialized)
        {
                R_CSTL_LOG_WARN ("Deleting uninitialized renderer subsystem");
                return -1;
        }
#endif

        R_GameRenderer_CleanupFrame (pSubsystem);
        R_GameRenderer_CleanupLayers (pSubsystem);
        R_GameRenderer_CleanupResources (pSubsystem);
        R_GameRenderer_ShutdownThreadPool (pSubsystem);
        R_GAME_MUTEX_DESTROY (&pSubsystem->layerArrayMutex);
        R_GAME_MUTEX_DESTROY (&pSubsystem->resourceArrayMutex);

#if defined(R_GAME_DEBUG)
        pSubsystem->isInitialized = false;
#endif

        R_CSTL_HeapFree (pSubsystem);
        R_GameRenderer_ShutdownBytecodeDecoder ();
        R_CSTL_LOG_INFO ("Renderer subsystem deleted successfully");
        return 0;
}

GAME_API int
R_GameRenderer_SetPipelineContext (
    struct R_GameRendererSubsystem*       pSubsystem,
    struct R_GameCVulkan_PipelineContext* pPipelineContext)
{
#if defined(R_GAME_DEBUG)
        if (pSubsystem == NULL || pPipelineContext == NULL)
        {
                R_CSTL_LOG_ERROR ("Invalid parameters for SetPipelineContext");
                return -1;
        }

        if (!pSubsystem->isInitialized)
        {
                R_CSTL_LOG_ERROR ("Renderer subsystem not initialized");
                return -1;
        }
#endif

        if (!R_GameCVulkan_PipelineContextIsInitialized (pPipelineContext))
        {
                R_CSTL_LOG_ERROR ("Pipeline context is not initialized");
                return -1;
        }

        pSubsystem->pPipelineContext = pPipelineContext;

        if (R_GameRenderer_InitializeFrame (pSubsystem, pPipelineContext) != 0)
        {
                R_CSTL_LOG_ERROR ("Failed to initialize frame data");
                pSubsystem->pPipelineContext = NULL;
                return -1;
        }

        R_CSTL_LOG_INFO ("Pipeline context set successfully");
        return 0;
}

GAME_API int
R_GameRenderer_SubsystemStart (struct R_GameRendererSubsystem* pSubsystem)
{
        R_GAME_VALIDATE_SUBSYSTEM_INITIALIZED (pSubsystem);

        if (pSubsystem->pPipelineContext == NULL)
        {
                R_CSTL_LOG_ERROR ("Pipeline context not set");
                return -1;
        }

        if (g_lifecycle.constructCallback != NULL)
        {
                g_lifecycle.constructCallback (pSubsystem, "RendererSubsystem", NULL, 0);
        }

        pSubsystem->state = R_GAME_RENDERER_STATE_RUNNING;
        pSubsystem->frameCounter = 0;
        R_CSTL_LOG_INFO ("Renderer subsystem started");
        return 0;
}

GAME_API int
R_GameRenderer_SubsystemStop (struct R_GameRendererSubsystem* pSubsystem)
{
        R_GAME_VALIDATE_SUBSYSTEM_INITIALIZED (pSubsystem);

        R_GameRenderer_WaitForFrame (pSubsystem);

        if (g_lifecycle.stopCallback != NULL)
        {
                g_lifecycle.stopCallback (pSubsystem, NULL, 0);
        }

        if (g_lifecycle.overCallback != NULL)
        {
                g_lifecycle.overCallback (pSubsystem, NULL, 0);
        }

        pSubsystem->state = R_GAME_RENDERER_STATE_STOPPED;
        R_CSTL_LOG_INFO ("Renderer subsystem stopped");
        return 0;
}

GAME_API int
R_GameRenderer_SubsystemPause (struct R_GameRendererSubsystem* pSubsystem)
{
        R_GAME_VALIDATE_SUBSYSTEM_INITIALIZED (pSubsystem);

        if (pSubsystem->state != R_GAME_RENDERER_STATE_RUNNING)
        {
                R_CSTL_LOG_WARN ("Cannot pause renderer: not running");
                return -1;
        }

        if (g_lifecycle.pauseCallback != NULL)
        {
                g_lifecycle.pauseCallback (pSubsystem, NULL, 0);
        }

        pSubsystem->state = R_GAME_RENDERER_STATE_PAUSED;
        R_CSTL_LOG_INFO ("Renderer subsystem paused");
        return 0;
}

GAME_API int
R_GameRenderer_SubsystemResume (struct R_GameRendererSubsystem* pSubsystem)
{
        R_GAME_VALIDATE_SUBSYSTEM_INITIALIZED (pSubsystem);

        if (pSubsystem->state != R_GAME_RENDERER_STATE_PAUSED)
        {
                R_CSTL_LOG_WARN ("Cannot resume renderer: not paused");
                return -1;
        }

        if (g_lifecycle.resumeCallback != NULL)
        {
                g_lifecycle.resumeCallback (pSubsystem, NULL, 0);
        }

        pSubsystem->state = R_GAME_RENDERER_STATE_RUNNING;
        R_CSTL_LOG_INFO ("Renderer subsystem resumed");
        return 0;
}

GAME_API uint32_t
R_GameRenderer_SubsystemGetState (struct R_GameRendererSubsystem* pSubsystem)
{
        if (pSubsystem == NULL)
        {
                return R_GAME_RENDERER_STATE_ERROR;
        }
        return pSubsystem->state;
}

GAME_API int
R_GameRenderer_BeginFrame (struct R_GameRendererSubsystem* pSubsystem)
{
        R_GAME_VALIDATE_SUBSYSTEM (pSubsystem);
        R_GAME_VALIDATE_PARAM (pSubsystem->pPipelineContext);

        if (pSubsystem->state != R_GAME_RENDERER_STATE_RUNNING)
        {
                return -1;
        }

        struct R_GameRendererFrame* pCurrentFrame =
            &pSubsystem->pFrames[pSubsystem->currentFrameIndex];

        if (pCurrentFrame->pInFlightFence != NULL)
        {
                struct R_CVulkan_Device* pDevice = 
                    R_GameCVulkan_PipelineContextGetDevice (pSubsystem->pPipelineContext);
                
                if (R_GameRenderer_WaitAndResetFence (pDevice, pCurrentFrame->pInFlightFence) != 0)
                {
                        return -1;
                }
        }

        uint32_t imageIndex = 0;
        if (R_GameRenderer_AcquireSwapchainImage (pSubsystem->pPipelineContext, &imageIndex) != 0)
        {
                return -1;
        }

        if (g_lifecycle.beforeEachCallback != NULL)
        {
                g_lifecycle.beforeEachCallback (pSubsystem, NULL, 0);
        }

        pCurrentFrame->frameNumber = pSubsystem->frameCounter;
        return 0;
}

GAME_API int
R_GameRenderer_RenderFrame (struct R_GameRendererSubsystem* pSubsystem)
{
        R_GAME_VALIDATE_SUBSYSTEM (pSubsystem);
        R_GAME_VALIDATE_PARAM (pSubsystem->pPipelineContext);

        if (pSubsystem->state != R_GAME_RENDERER_STATE_RUNNING)
        {
                return -1;
        }

        struct R_GameRendererFrame* pCurrentFrame =
            &pSubsystem->pFrames[pSubsystem->currentFrameIndex];

        size_t cmdBufferCount = R_CSTL_ArrayLength (pCurrentFrame->pCommandBufferArray) / 
            sizeof (struct R_CVulkan_CommandBuffer);
        
        size_t layerCount = R_CSTL_ArrayLength (pSubsystem->pLayerArray) / 
            sizeof (struct R_GameRendererLayer);
        
        size_t currentBufferIndex = 0;
        
        R_GAME_MUTEX_LOCK (&pSubsystem->pThreadPool->taskMutex);
        R_CSTL_DeleteArray (pSubsystem->pThreadPool->pTaskQueue);
        pSubsystem->pThreadPool->pTaskQueue = R_CSTL_NewArrayWithCapacity (
            sizeof (struct R_GameRendererRenderTask) * R_GAME_RENDERER_MAX_COMMAND_BUFFERS_PER_FRAME);
        R_GAME_ATOMIC_STORE (&pSubsystem->pThreadPool->atomicPendingTasks, 0);
        R_GAME_ATOMIC_STORE (&pSubsystem->pThreadPool->atomicCompletedTasks, 0);
        R_GAME_MUTEX_UNLOCK (&pSubsystem->pThreadPool->taskMutex);
        
        for (size_t layerIdx = 0; layerIdx < layerCount; ++layerIdx)
        {
                struct R_GameRendererLayer layer;
                R_CSTL_ArrayTypedUncheckedAt (pSubsystem->pLayerArray, struct R_GameRendererLayer, layerIdx, &layer);
                
                if (!(layer.flags & R_GAME_RENDERER_LAYER_FLAG_ENABLED))
                {
                        continue;
                }

                if (currentBufferIndex >= cmdBufferCount)
                {
                        R_CSTL_LOG_WARN ("Exceeded maximum command buffers per frame");
                        break;
                }

                struct R_GameRendererRenderTask task = {0};
                task.layerIndex = (uint32_t)layerIdx;
                task.commandBufferIndex = (uint32_t)currentBufferIndex;
                task.frameIndex = pSubsystem->currentFrameIndex;
                task.completed = 0;
                R_GAME_ATOMIC_STORE (&task.atomicCompleted, 0);
                
                R_GAME_MUTEX_LOCK (&pSubsystem->pThreadPool->taskMutex);
                int result = R_CSTL_ArrayPushData (
                    pSubsystem->pThreadPool->pTaskQueue,
                    (const uint8_t*)&task,
                    sizeof (struct R_GameRendererRenderTask));
                R_GAME_ATOMIC_INCREMENT (&pSubsystem->pThreadPool->atomicPendingTasks);
                R_GAME_MUTEX_UNLOCK (&pSubsystem->pThreadPool->taskMutex);
                
                if (result != R_CSTL_OK)
                {
                        R_CSTL_LOG_ERROR ("Failed to add render task to queue");
                        return -1;
                }
                
                currentBufferIndex++;
        }
        
        R_GAME_COND_BROADCAST (&pSubsystem->pThreadPool->taskAvailable);
        
        R_GAME_MUTEX_LOCK (&pSubsystem->pThreadPool->taskMutex);
        uint32_t pendingTasks = R_GAME_ATOMIC_LOAD (&pSubsystem->pThreadPool->atomicPendingTasks);
        uint32_t completedTasks = R_GAME_ATOMIC_LOAD (&pSubsystem->pThreadPool->atomicCompletedTasks);
        while (completedTasks < pendingTasks)
        {
                R_GAME_COND_WAIT (&pSubsystem->pThreadPool->taskComplete, &pSubsystem->pThreadPool->taskMutex);
                completedTasks = R_GAME_ATOMIC_LOAD (&pSubsystem->pThreadPool->atomicCompletedTasks);
        }
        R_GAME_MUTEX_UNLOCK (&pSubsystem->pThreadPool->taskMutex);

        if (g_lifecycle.renderCallback != NULL)
        {
                g_lifecycle.renderCallback (pSubsystem, NULL, 0);
        }

        return 0;
}

GAME_API int
R_GameRenderer_EndFrame (struct R_GameRendererSubsystem* pSubsystem)
{
        R_GAME_VALIDATE_SUBSYSTEM (pSubsystem);
        R_GAME_VALIDATE_PARAM (pSubsystem->pPipelineContext);

        if (pSubsystem->state != R_GAME_RENDERER_STATE_RUNNING)
        {
                return -1;
        }

        struct R_GameRendererFrame* pCurrentFrame =
            &pSubsystem->pFrames[pSubsystem->currentFrameIndex];

        struct R_CVulkan_Queue* pGraphicsQueue =
            R_GameCVulkan_PipelineContextGetGraphicsQueue (pSubsystem->pPipelineContext);
        struct R_CVulkan_Queue* pPresentQueue =
            R_GameCVulkan_PipelineContextGetPresentQueue (pSubsystem->pPipelineContext);

        size_t layerCount = R_CSTL_ArrayLength (pSubsystem->pLayerArray) / 
            sizeof (struct R_GameRendererLayer);
        
        struct R_CVulkan_CommandBuffer* commandBuffers[R_GAME_RENDERER_MAX_COMMAND_BUFFERS_PER_FRAME];
        size_t commandBufferCount = 0;
        
        for (size_t layerIdx = 0; layerIdx < layerCount; ++layerIdx)
        {
                struct R_GameRendererLayer layer;
                R_CSTL_ArrayTypedUncheckedAt (pSubsystem->pLayerArray, struct R_GameRendererLayer, layerIdx, &layer);
                
                if (!(layer.flags & R_GAME_RENDERER_LAYER_FLAG_ENABLED))
                {
                        continue;
                }

                if (commandBufferCount < R_GAME_RENDERER_MAX_COMMAND_BUFFERS_PER_FRAME)
                {
                        struct R_CVulkan_CommandBuffer cmdBuffer;
                        R_CSTL_ArrayTypedUncheckedAt (pCurrentFrame->pCommandBufferArray, struct R_CVulkan_CommandBuffer, commandBufferCount, &cmdBuffer);
                        
                        static struct R_CVulkan_CommandBuffer s_cmdBuffers[R_GAME_RENDERER_MAX_COMMAND_BUFFERS_PER_FRAME];
                        s_cmdBuffers[commandBufferCount] = cmdBuffer;
                        commandBuffers[commandBufferCount] = &s_cmdBuffers[commandBufferCount];
                        commandBufferCount++;
                }
        }

        if (R_GameRenderer_SubmitCommandBuffers (
            pGraphicsQueue,
            commandBuffers,
            commandBufferCount,
            &pSubsystem->pPipelineContext->imageAvailableSemaphore,
            pCurrentFrame->pRenderFinishedSemaphore,
            pCurrentFrame->pInFlightFence) != 0)
        {
                return -1;
        }

        VkSwapchainKHR swapchainHandle = 
            R_CVulkan_SwapchainGetHandle (&pSubsystem->pPipelineContext->swapchain);
        uint32_t imageIndex = 0;
        
        if (R_GameRenderer_PresentImage (
            pPresentQueue,
            swapchainHandle,
            imageIndex,
            pCurrentFrame->pRenderFinishedSemaphore) != 0)
        {
                return -1;
        }

        if (g_lifecycle.afterEachCallback != NULL)
        {
                g_lifecycle.afterEachCallback (pSubsystem, NULL, 0);
        }

        pSubsystem->currentFrameIndex =
            (pSubsystem->currentFrameIndex + 1) % pSubsystem->maxFramesInFlight;
        pSubsystem->frameCounter++;

        return 0;
}

GAME_API int
R_GameRenderer_WaitForFrame (struct R_GameRendererSubsystem* pSubsystem)
{
        R_GAME_VALIDATE_SUBSYSTEM (pSubsystem);
        R_GAME_VALIDATE_PARAM (pSubsystem->pPipelineContext);

        struct R_CVulkan_Device* pDevice = 
            R_GameCVulkan_PipelineContextGetDevice (pSubsystem->pPipelineContext);

        for (uint32_t i = 0; i < pSubsystem->maxFramesInFlight; ++i)
        {
                struct R_GameRendererFrame* pFrame = &pSubsystem->pFrames[i];
                if (pFrame->pInFlightFence != NULL)
                {
                        R_CVulkan_FenceWait (pDevice, pFrame->pInFlightFence, 1, true, UINT64_MAX);
                }
        }
        return 0;
}

GAME_API int
R_GameRenderer_AddLayer (
    struct R_GameRendererSubsystem* pSubsystem,
    const char*                      pName,
    uint32_t                        priority,
    uint32_t                        flags,
    const void*                     pUserData)
{
        R_GAME_VALIDATE_SUBSYSTEM_INITIALIZED (pSubsystem);
        R_GAME_VALIDATE_PARAM (pName);

        size_t currentLayerCount = R_CSTL_ArrayLength (pSubsystem->pLayerArray) / 
            sizeof (struct R_GameRendererLayer);
        if (currentLayerCount >= R_GAME_RENDERER_MAX_LAYERS)
        {
                R_CSTL_LOG_ERROR ("Maximum number of layers reached");
                return -1;
        }

        struct R_GameRendererLayer newLayer = {0};
        newLayer.pName = R_GameRenderer_AllocateNameCopy (pName);
        if (newLayer.pName == NULL)
        {
                R_CSTL_LOG_ERROR ("Failed to allocate layer name");
                return -1;
        }
        newLayer.priority = priority;
        newLayer.flags = flags | R_GAME_RENDERER_LAYER_FLAG_ENABLED;
        newLayer.pUserData = pUserData;
        newLayer.renderCallback = NULL;
        newLayer.beforePassCallback = NULL;
        newLayer.afterPassCallback = NULL;

#if defined(R_GAME_DEBUG)
        newLayer.isInitialized = true;
#endif

        R_GAME_MUTEX_LOCK (&pSubsystem->layerArrayMutex);
        int result = R_CSTL_ArrayPushData (
            pSubsystem->pLayerArray,
            (const uint8_t*)&newLayer,
            sizeof (struct R_GameRendererLayer));
        R_GAME_MUTEX_UNLOCK (&pSubsystem->layerArrayMutex);
        
        if (result != R_CSTL_OK)
        {
                R_CSTL_LOG_ERROR ("Failed to push layer to array");
                R_CSTL_HeapFree (newLayer.pName);
                return -1;
        }

        R_CSTL_LOG_INFO ("Added render layer: %s (priority: %u)", pName, priority);
        return (int)(R_CSTL_ArrayLength (pSubsystem->pLayerArray) / sizeof (struct R_GameRendererLayer) - 1);
}

GAME_API int
R_GameRenderer_RemoveLayer (struct R_GameRendererSubsystem* pSubsystem, uint32_t layerIndex)
{
        R_GAME_VALIDATE_SUBSYSTEM (pSubsystem);
        size_t layerCount = R_CSTL_ArrayLength (pSubsystem->pLayerArray) / 
            sizeof (struct R_GameRendererLayer);
        if (layerIndex >= layerCount)
        {
                R_CSTL_LOG_ERROR ("Invalid layer index: %u", layerIndex);
                return -1;
        }

        R_GAME_MUTEX_LOCK (&pSubsystem->layerArrayMutex);
        struct R_GameRendererLayer layer;
        R_CSTL_ArrayTypedAt (pSubsystem->pLayerArray, struct R_GameRendererLayer, layerIndex, &layer);

        if (layer.pName != NULL)
        {
                R_CSTL_HeapFree (layer.pName);
        }
        
        if (R_GameRenderer_RemoveFromArrayByIndex (
            &pSubsystem->pLayerArray,
            sizeof (struct R_GameRendererLayer),
            layerIndex) != 0)
        {
                R_GAME_MUTEX_UNLOCK (&pSubsystem->layerArrayMutex);
                R_CSTL_LOG_ERROR ("Failed to remove layer from array");
                return -1;
        }
        R_GAME_MUTEX_UNLOCK (&pSubsystem->layerArrayMutex);

        R_CSTL_LOG_INFO ("Removed render layer at index %u", layerIndex);
        return 0;
}

GAME_API int
R_GameRenderer_SetLayerEnabled (
    struct R_GameRendererSubsystem* pSubsystem,
    uint32_t                        layerIndex,
    int                             enabled)
{
        R_GAME_VALIDATE_SUBSYSTEM (pSubsystem);
        size_t layerCount = R_CSTL_ArrayLength (pSubsystem->pLayerArray) / 
            sizeof (struct R_GameRendererLayer);
        if (layerIndex >= layerCount)
        {
                R_CSTL_LOG_ERROR ("Invalid layer index: %u", layerIndex);
                return -1;
        }

        R_GAME_MUTEX_LOCK (&pSubsystem->layerArrayMutex);
        struct R_GameRendererLayer layer;
        R_CSTL_ArrayTypedAt (pSubsystem->pLayerArray, struct R_GameRendererLayer, layerIndex, &layer);
        
        if (enabled)
        {
                layer.flags |= R_GAME_RENDERER_LAYER_FLAG_ENABLED;
        }
        else
        {
                layer.flags &= ~R_GAME_RENDERER_LAYER_FLAG_ENABLED;
        }

        R_CSTL_ArrayTypedSetAt (pSubsystem->pLayerArray, struct R_GameRendererLayer, layerIndex, &layer);
        R_GAME_MUTEX_UNLOCK (&pSubsystem->layerArrayMutex);

        return 0;
}

GAME_API struct R_GameRendererLayer*
R_GameRenderer_GetLayer (struct R_GameRendererSubsystem* pSubsystem, uint32_t layerIndex)
{
        R_GAME_VALIDATE_SUBSYSTEM (pSubsystem != NULL);
        size_t layerCount = R_CSTL_ArrayLength (pSubsystem->pLayerArray) / 
            sizeof (struct R_GameRendererLayer);
#if defined(R_GAME_DEBUG)
        if (layerIndex >= layerCount)
        {
                return NULL;
        }
#endif
        R_GAME_MUTEX_LOCK (&pSubsystem->layerArrayMutex);
        const uint8_t* pLayerData = R_CSTL_ArrayData (pSubsystem->pLayerArray);
        static struct R_GameRendererLayer s_layer;
        memcpy (&s_layer, pLayerData + layerIndex * sizeof (struct R_GameRendererLayer), sizeof (struct R_GameRendererLayer));
        R_GAME_MUTEX_UNLOCK (&pSubsystem->layerArrayMutex);
        return &s_layer;
}

GAME_API int
R_GameRenderer_SortLayers (struct R_GameRendererSubsystem* pSubsystem)
{
        R_GAME_VALIDATE_SUBSYSTEM (pSubsystem);
        R_GAME_MUTEX_LOCK (&pSubsystem->layerArrayMutex);
        int result = R_CSTL_ArraySort (
            pSubsystem->pLayerArray,
            sizeof (struct R_GameRendererLayer),
            R_GameRenderer_CompareLayers,
            NULL);
        R_GAME_MUTEX_UNLOCK (&pSubsystem->layerArrayMutex);
        
        if (result != R_CSTL_OK)
        {
                R_CSTL_LOG_ERROR ("Failed to sort layers");
                return -1;
        }

        R_CSTL_LOG_INFO ("Render layers sorted by priority");
        return 0;
}

GAME_API uint64_t
R_GameRenderer_RegisterResource (
    struct R_GameRendererSubsystem* pSubsystem,
    uint32_t                        type,
    const void*                     pResource,
    uint64_t                        size,
    const char*                     pName)
{
        R_GAME_VALIDATE_SUBSYSTEM_INITIALIZED (pSubsystem);
        R_GAME_VALIDATE_PARAM (pResource);

        size_t currentResourceCount = R_CSTL_ArrayLength (pSubsystem->pResourceArray) / 
            sizeof (struct R_GameRendererResource);
        if (currentResourceCount >= R_GAME_RENDERER_MAX_RESOURCES)
        {
                R_CSTL_LOG_ERROR ("Maximum number of resources reached");
                return 0;
        }

        struct R_GameRendererResource newResource = {0};
        newResource.handle = pSubsystem->nextResourceHandle++;
        newResource.type = type;
        newResource.pResource = pResource;
        newResource.size = size;
        newResource.refCount = 1;

        newResource.pName = R_GameRenderer_AllocateNameCopy (pName);

#if defined(R_GAME_DEBUG)
        newResource.isInitialized = true;
#endif

        R_GAME_MUTEX_LOCK (&pSubsystem->resourceArrayMutex);
        int result = R_CSTL_ArrayPushData (
            pSubsystem->pResourceArray,
            (const uint8_t*)&newResource,
            sizeof (struct R_GameRendererResource));
        R_GAME_MUTEX_UNLOCK (&pSubsystem->resourceArrayMutex);
        
        if (result != R_CSTL_OK)
        {
                R_CSTL_LOG_ERROR ("Failed to push resource to array");
                if (newResource.pName != NULL)
                {
                        R_CSTL_HeapFree ((void*)newResource.pName);
                }
                return 0;
        }

        R_CSTL_LOG_INFO (
            "Registered resource: %s (handle: %" PRIu64 ", type: %u)",
            pName ? pName : "unnamed",
            newResource.handle,
            type);

        return newResource.handle;
}

GAME_API int
R_GameRenderer_UnregisterResource (
    struct R_GameRendererSubsystem* pSubsystem,
    uint64_t                        handle)
{
        R_GAME_VALIDATE_SUBSYSTEM (pSubsystem);
        R_GAME_VALIDATE_PARAM (handle != 0);
        size_t foundIndex = R_GameRenderer_FindResourceIndexByHandle (pSubsystem, handle);
        if (foundIndex == SIZE_MAX)
        {
                R_CSTL_LOG_WARN ("Resource not found: %llu", handle);
                return -1;
        }

        R_GAME_MUTEX_LOCK (&pSubsystem->resourceArrayMutex);
        struct R_GameRendererResource resource;
        R_CSTL_ArrayTypedAt (pSubsystem->pResourceArray, struct R_GameRendererResource, foundIndex, &resource);
        
        if (resource.pName != NULL)
        {
                R_CSTL_HeapFree ((void*)resource.pName);
        }

        if (R_GameRenderer_RemoveFromArrayByIndex (
            &pSubsystem->pResourceArray,
            sizeof (struct R_GameRendererResource),
            foundIndex) != 0)
        {
                R_GAME_MUTEX_UNLOCK (&pSubsystem->resourceArrayMutex);
                R_CSTL_LOG_ERROR ("Failed to remove resource from array");
                return -1;
        }
        R_GAME_MUTEX_UNLOCK (&pSubsystem->resourceArrayMutex);

        R_CSTL_LOG_INFO ("Unregistered resource: %llu", handle);
        return 0;
}

GAME_API const void*
R_GameRenderer_GetResource (
    struct R_GameRendererSubsystem* pSubsystem,
    uint64_t                        handle)
{
        R_GAME_VALIDATE_SUBSYSTEM (pSubsystem != NULL);
        R_GAME_VALIDATE_PARAM (handle != 0);
        size_t index = R_GameRenderer_FindResourceIndexByHandle (pSubsystem, handle);
        if (index == SIZE_MAX)
        {
                return NULL;
        }
        R_GAME_MUTEX_LOCK (&pSubsystem->resourceArrayMutex);
        struct R_GameRendererResource resource;
        R_CSTL_ArrayTypedUncheckedAt (pSubsystem->pResourceArray, struct R_GameRendererResource, index, &resource);
        R_GAME_MUTEX_UNLOCK (&pSubsystem->resourceArrayMutex);
        return resource.pResource;
}

GAME_API int
R_GameRenderer_AddResourceRef (
    struct R_GameRendererSubsystem* pSubsystem,
    uint64_t                        handle)
{
        R_GAME_VALIDATE_SUBSYSTEM (pSubsystem);
        R_GAME_VALIDATE_PARAM (handle != 0);

        size_t index = R_GameRenderer_FindResourceIndexByHandle (pSubsystem, handle);
        if (index == SIZE_MAX)
        {
                return -1;
        }

        R_GAME_MUTEX_LOCK (&pSubsystem->resourceArrayMutex);
        struct R_GameRendererResource resource;
        R_CSTL_ArrayTypedUncheckedAt (pSubsystem->pResourceArray, struct R_GameRendererResource, index, &resource);
        resource.refCount++;
        R_CSTL_ArrayTypedSetAtUnchecked (pSubsystem->pResourceArray, struct R_GameRendererResource, index, &resource);
        R_GAME_MUTEX_UNLOCK (&pSubsystem->resourceArrayMutex);
        return 0;
}

GAME_API int
R_GameRenderer_ReleaseResourceRef (
    struct R_GameRendererSubsystem* pSubsystem,
    uint64_t                        handle)
{
        R_GAME_VALIDATE_SUBSYSTEM (pSubsystem);
        R_GAME_VALIDATE_PARAM (handle != 0);

        size_t index = R_GameRenderer_FindResourceIndexByHandle (pSubsystem, handle);
        if (index == SIZE_MAX)
        {
                return -1;
        }

        R_GAME_MUTEX_LOCK (&pSubsystem->resourceArrayMutex);
        struct R_GameRendererResource resource;
        R_CSTL_ArrayTypedUncheckedAt (pSubsystem->pResourceArray, struct R_GameRendererResource, index, &resource);
        if (resource.refCount > 0)
        {
                resource.refCount--;
                R_CSTL_ArrayTypedSetAtUnchecked (pSubsystem->pResourceArray, struct R_GameRendererResource, index, &resource);
        }
        R_GAME_MUTEX_UNLOCK (&pSubsystem->resourceArrayMutex);
        return 0;
}

GAME_API struct R_CVulkan_CommandBuffer*
R_GameRenderer_GetCommandBuffer (
    struct R_GameRendererSubsystem* pSubsystem,
    uint32_t                        frameIndex,
    uint32_t                        bufferIndex)
{
        R_GAME_VALIDATE_SUBSYSTEM (pSubsystem != NULL);

        if (frameIndex >= pSubsystem->maxFramesInFlight)
        {
                R_CSTL_LOG_ERROR ("Invalid frame index: %u", frameIndex);
                return NULL;
        }

        struct R_GameRendererFrame* pFrame = &pSubsystem->pFrames[frameIndex];
        
        size_t cmdBufferCount = R_CSTL_ArrayLength (pFrame->pCommandBufferArray) / 
            sizeof (struct R_CVulkan_CommandBuffer);
        if (bufferIndex >= cmdBufferCount)
        {
                R_CSTL_LOG_ERROR ("Invalid buffer index: %u", bufferIndex);
                return NULL;
        }

        struct R_CVulkan_CommandBuffer cmdBuffer;
        R_CSTL_ArrayTypedAt (pFrame->pCommandBufferArray, struct R_CVulkan_CommandBuffer, bufferIndex, &cmdBuffer);
        
        static struct R_CVulkan_CommandBuffer s_cmdBuffer;
        s_cmdBuffer = cmdBuffer;
        return &s_cmdBuffer;
}

GAME_API void
R_GameRendererLifecycle_RegisterRenderer (const void* pRenderer)
{
        g_lifecycle.pRenderer = pRenderer;
}

GAME_API void
R_GameRendererLifecycle_RegisterConstruct (R_GameLifecycleConstruct callback)
{
        g_lifecycle.constructCallback = callback;
        if (callback != NULL)
        {
                R_GameRenderer_ValidateCallbackFunction ((R_CSTL_BytecodeFunction)callback, "construct");
        }
}

GAME_API void
R_GameRendererLifecycle_RegisterResume (R_GameLifecycleResume callback)
{
        g_lifecycle.resumeCallback = callback;
}

GAME_API void
R_GameRendererLifecycle_RegisterPause (R_GameLifecyclePause callback)
{
        g_lifecycle.pauseCallback = callback;
}

GAME_API void
R_GameRendererLifecycle_RegisterBeforeEach (R_GameLifecycleBeforeEach callback)
{
        g_lifecycle.beforeEachCallback = callback;
}

GAME_API void
R_GameRendererLifecycle_RegisterAfterEach (R_GameLifecycleAfterEach callback)
{
        g_lifecycle.afterEachCallback = callback;
}

GAME_API void
R_GameRendererLifecycle_RegisterBeforePass (R_GameLifecycleBeforePass callback)
{
        g_lifecycle.beforePassCallback = callback;
}

GAME_API void
R_GameRendererLifecycle_RegisterAfterPass (R_GameLifecycleAfterPass callback)
{
        g_lifecycle.afterPassCallback = callback;
}

GAME_API void
R_GameRendererLifecycle_RegisterRender (R_GameLifecycleRender callback)
{
        g_lifecycle.renderCallback = callback;
}

GAME_API void
R_GameRendererLifecycle_RegisterStop (R_GameLifecycleStop callback)
{
        g_lifecycle.stopCallback = callback;
}

GAME_API void
R_GameRendererLifecycle_RegisterOver (R_GameLifecycleOver callback)
{
        g_lifecycle.overCallback = callback;
        if (callback != NULL)
        {
                R_GameRenderer_ValidateCallbackFunction ((R_CSTL_BytecodeFunction)callback, "over");
        }
}