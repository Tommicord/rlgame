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
#include "rlgame.base/cstl/cstl_stack.h"
#include "rlgame.base/cstl/cstl_trace.h"
#include "rlgame.base/cstl/cstl_string.h"

#include <string.h>
#include <stdlib.h>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <pthread.h>
#endif

static int
R_GameRenderer_AcquireSwapchainImage (struct R_Game_PipelineContext* pPipelineContext, uint32_t* pImageIndex);

struct R_GameRendererFrame
{
        struct R_CSTL_Array*        pCommandBufferArray;
        struct R_CVulkan_Semaphore* pRenderFinishedSemaphore;
        struct R_CVulkan_Fence*     pInFlightFence;
        uint32_t                    resourceIndex;
        uint64_t                    frameNumber;
};

struct R_GameRendererLayer
{
        char*                     pName;
        uint32_t                  priority;
        uint32_t                  flags;
        const void*               pUserData;
        R_GameLifecycleRender     renderCallback;
        R_GameLifecycleBeforePass beforePassCallback;
        R_GameLifecycleAfterPass  afterPassCallback;
};

struct R_GameRendererResource
{
        uint64_t    handle;
        uint32_t    type;
        const void* pResource;
        uint64_t    size;
        uint32_t    refCount;
        char*       pName;
};

struct R_GameRendererLifecycle
{
        const void*               pRenderer;
        R_GameLifecycleConstruct  constructCallback;
        R_GameLifecycleResume     resumeCallback;
        R_GameLifecyclePause      pauseCallback;
        R_GameLifecycleBeforeEach beforeEachCallback;
        R_GameLifecycleAfterEach  afterEachCallback;
        R_GameLifecycleBeforePass beforePassCallback;
        R_GameLifecycleAfterPass  afterPassCallback;
        R_GameLifecycleRender     renderCallback;
        R_GameLifecycleStop       stopCallback;
        R_GameLifecycleOver       overCallback;
};

struct R_GameRendererSubsystem
{
        struct R_Game_PipelineContext* pPipelineContext;
        struct R_GameRendererLifecycle lifecycle;

        struct R_GameRendererFrame* pFrames;
        uint32_t                    maxFramesInFlight;
        uint32_t                    currentFrameIndex;
        uint64_t                    frameCounter;

        struct R_CSTL_Array* pLayerArray;
        struct R_CSTL_Array* pResourceArray;
        uint64_t             nextResourceHandle;

        struct R_GameRendererThreadPool* pThreadPool;
        R_GAME_MUTEX                     layerArrayMutex;
        R_GAME_MUTEX                     resourceArrayMutex;

        uint32_t state;
};

struct R_Game_RendererManager
{
        struct R_GameRendererSubsystemEntry subsystems[R_GAME_RENDERER_MAX_SUBSYSTEMS];
        uint32_t                            subsystemCount;
        struct R_Game_PipelineContext*      pPipelineContext;
        struct R_CVulkan_RenderPass*        pCompositionRenderPass;
        struct R_CVulkan_Pipeline*          pCompositionPipeline;
        struct R_CVulkan_Image*             pSwapchainImages[R_GAME_RENDERER_MAX_SWAPCHAIN_IMAGES];
        struct R_CVulkan_ImageView*         pSwapchainImageViews[R_GAME_RENDERER_MAX_SWAPCHAIN_IMAGES];
        struct R_CVulkan_Framebuffer*       pSwapchainFramebuffers[R_GAME_RENDERER_MAX_SWAPCHAIN_IMAGES];
        uint32_t                            swapchainImageCount;
        uint32_t                            currentSwapchainIndex;
        R_GAME_MUTEX                        managerMutex;
};
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
        return R_GAME_ERROR_OUT_OF_MEMORY;
    }
    memset (pSubsystem->pFrames, 0, sizeof (struct R_GameRendererFrame) * pSubsystem->maxFramesInFlight);

    pSubsystem->pLayerArray
        = R_CSTL_NewArrayWithCapacity (sizeof (struct R_GameRendererLayer) * R_GAME_RENDERER_MAX_LAYERS);
    if (pSubsystem->pLayerArray == NULL)
    {
        R_CSTL_LOG_ERROR ("Failed to allocate render layer array");
        R_CSTL_HeapFree (pSubsystem->pFrames);
        return R_GAME_ERROR_OUT_OF_MEMORY;
    }

    pSubsystem->pResourceArray = R_CSTL_NewArrayWithCapacity (
        sizeof (struct R_GameRendererResource) * R_GAME_RENDERER_MAX_RESOURCES);
    if (pSubsystem->pResourceArray == NULL)
    {
        R_CSTL_LOG_ERROR ("Failed to allocate resource array");
        R_CSTL_DeleteArray (pSubsystem->pLayerArray);
        R_CSTL_HeapFree (pSubsystem->pFrames);
        return R_GAME_ERROR_OUT_OF_MEMORY;
    }

    return R_GAME_OK;
}

static int
R_GameRenderer_WaitForFence (
    struct R_CVulkan_Device* pDevice,
    struct R_CVulkan_Fence*  pFence,
    uint64_t                 timeout)
{
    enum R_CVulkanError err = R_CVulkan_FenceWait (pDevice, pFence, 1, true, timeout);
    if (err != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("Failed to wait for fence: %s", R_CVulkanErrorToString (err));
        return R_GAME_ERROR_FAILED;
    }
    return R_GAME_OK;
}

static int
R_GameRenderer_AcquireNextImage (struct R_Game_PipelineContext* pPipelineContext, uint32_t* pImageIndex)
{
    return R_GameRenderer_AcquireSwapchainImage (pPipelineContext, pImageIndex);
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
    if (pSubsystem->pFrames)
    {
        R_CSTL_HeapFree (pSubsystem->pFrames);
        pSubsystem->pFrames = NULL;
    }
    if (pSubsystem->pLayerArray)
    {
        R_CSTL_DeleteArray (pSubsystem->pLayerArray);
        pSubsystem->pLayerArray = NULL;
    }
    if (pSubsystem->pResourceArray)
    {
        R_CSTL_DeleteArray (pSubsystem->pResourceArray);
        pSubsystem->pResourceArray = NULL;
    }
}

static void
R_GameRenderer_CleanupLayers (struct R_GameRendererSubsystem* pSubsystem)
{
    if (pSubsystem->pLayerArray)
    {
        size_t layerCount
            = R_CSTL_ArrayLength (pSubsystem->pLayerArray) / sizeof (struct R_GameRendererLayer);

        for (size_t i = 0; i < layerCount; ++i)
        {
            struct R_GameRendererLayer layer;
            R_CSTL_ArrayTypedUncheckedAt (pSubsystem->pLayerArray, struct R_GameRendererLayer, i, &layer);
            if (layer.pName)
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
    if (pSubsystem->pResourceArray)
    {
        size_t resourceCount
            = R_CSTL_ArrayLength (pSubsystem->pResourceArray) / sizeof (struct R_GameRendererResource);

        for (size_t i = 0; i < resourceCount; ++i)
        {
            struct R_GameRendererResource resource;
            R_CSTL_ArrayTypedUncheckedAt (
                pSubsystem->pResourceArray,
                struct R_GameRendererResource,
                i,
                &resource);
            if (resource.pName)
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
    char*  pNameCopy = (char*)R_CSTL_HeapAlloc (nameLen + 1);
    if (pNameCopy)
    {
        memcpy (pNameCopy, pName, nameLen);
        pNameCopy[nameLen] = '\0';
    }
    return pNameCopy;
}

static int
R_GameRenderer_RemoveFromArrayByIndex (struct R_CSTL_Array** ppArray, size_t elementSize, size_t removeIndex)
{
    struct R_CSTL_Array* pArray = *ppArray;
    size_t               currentCount = R_CSTL_ArrayLength (pArray) / elementSize;
    R_GAME_ASSERT (removeIndex < currentCount);

    struct R_CSTL_Array* pNewArray = R_CSTL_NewArrayWithCapacity (elementSize * (currentCount - 1));
    if (pNewArray == NULL)
    {
        return R_GAME_ERROR_OUT_OF_MEMORY;
    }

    for (size_t i = 0; i < currentCount; ++i)
    {
        if (i == removeIndex)
        {
            continue;
        }

        const uint8_t* pOldData = R_CSTL_ArrayData (pArray);
        int            result = R_CSTL_ArrayPushData (pNewArray, pOldData + i * elementSize, elementSize);
        if (result != R_CSTL_OK)
        {
            R_CSTL_DeleteArray (pNewArray);
            return R_GAME_ERROR_ARRAY_OPERATION_FAILED;
        }
    }

    R_CSTL_DeleteArray (pArray);
    *ppArray = pNewArray;
    return R_GAME_OK;
}

static size_t
R_GameRenderer_FindResourceIndexByHandle (struct R_GameRendererSubsystem* pSubsystem, uint64_t handle)
{
    size_t resourceCount
        = R_CSTL_ArrayLength (pSubsystem->pResourceArray) / sizeof (struct R_GameRendererResource);

    for (size_t i = 0; i < resourceCount; ++i)
    {
        struct R_GameRendererResource resource;
        R_CSTL_ArrayTypedUncheckedAt (
            pSubsystem->pResourceArray,
            struct R_GameRendererResource,
            i,
            &resource);
        if (resource.handle == handle)
        {
            return i;
        }
    }

    return SIZE_MAX;
}

static int
R_GameRenderer_RemoveResourceByHandle (struct R_GameRendererSubsystem* pSubsystem, uint64_t handle)
{
    size_t index = R_GameRenderer_FindResourceIndexByHandle (pSubsystem, handle);
    if (index == SIZE_MAX)
    {
        return R_GAME_ERROR_RESOURCE_NOT_FOUND;
    }

    return R_GameRenderer_RemoveFromArrayByIndex (
        &pSubsystem->pResourceArray,
        sizeof (struct R_GameRendererResource),
        index);
}

static int
R_GameRenderer_WaitAndResetFence (struct R_CVulkan_Device* pDevice, struct R_CVulkan_Fence* pFence)
{
    enum R_CVulkanError err = R_CVulkan_FenceWait (pDevice, pFence, 1, true, UINT64_MAX);
    if (err != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("Failed to wait for fence: %s", R_CVulkanErrorToString (err));
        return R_GAME_ERROR_FAILED;
    }
    err = R_CVulkan_FenceReset (pDevice, pFence, 1);
    if (err != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("Failed to reset fence: %s", R_CVulkanErrorToString (err));
        return R_GAME_ERROR_FAILED;
    }

    return R_GAME_OK;
}

static int
R_GameRenderer_AcquireSwapchainImage (struct R_Game_PipelineContext* pPipelineContext, uint32_t* pImageIndex)
{
    enum R_CVulkanError err = R_CVulkan_SwapchainAcquireNextImage (
        &pPipelineContext->swapchain,
        UINT64_MAX,
        R_CVulkan_SemaphoreGetHandle (&pPipelineContext->imageAvailableSemaphore),
        VK_NULL_HANDLE,
        pImageIndex);
    if (err != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("Failed to acquire next image: %s", R_CVulkanErrorToString (err));
        return R_GAME_ERROR_FAILED;
    }
    return R_GAME_OK;
}

static int
R_GameRenderer_RenderLayer (struct R_GameRendererLayer* pLayer, struct R_CVulkan_CommandBuffer* pCmdBuffer)
{
    enum R_CVulkanError err = R_CVulkan_BeginCommandBuffer (pCmdBuffer, 0, NULL);
    if (err != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("Failed to begin command buffer: %s", R_CVulkanErrorToString (err));
        return R_GAME_ERROR_COMMAND_BUFFER_FAILED;
    }

    void* pLayerOp = (void*)pLayer;
    if (pLayer->beforePassCallback)
    {
        pLayer->beforePassCallback (pLayerOp, pCmdBuffer, sizeof (*pCmdBuffer));
    }
    if (pLayer->renderCallback)
    {
        pLayer->renderCallback (pLayerOp, pCmdBuffer, sizeof (*pCmdBuffer));
    }

    if (pLayer->afterPassCallback)
    {
        pLayer->afterPassCallback (pLayerOp, pCmdBuffer, sizeof (*pCmdBuffer));
    }
    err = R_CVulkan_EndCommandBuffer (pCmdBuffer);
    if (err != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("Failed to end command buffer: %s", R_CVulkanErrorToString (err));
        return R_GAME_ERROR_COMMAND_BUFFER_FAILED;
    }
    return R_GAME_OK;
}

static int
R_GameRenderer_SubmitCommandBuffers (
    struct R_CVulkan_Queue*          pGraphicsQueue,
    struct R_CVulkan_CommandBuffer** ppCommandBuffers,
    size_t                           commandBufferCount,
    struct R_CVulkan_Semaphore*      pImageAvailableSemaphore,
    struct R_CVulkan_Semaphore*      pRenderFinishedSemaphore,
    struct R_CVulkan_Fence*          pInFlightFence)
{
    VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    enum R_CVulkanError  err = R_CVulkan_QueueSubmit (
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
        R_CSTL_LOG_ERROR ("Failed to submit command buffers: %s", R_CVulkanErrorToString (err));
        return R_GAME_ERROR_FAILED;
    }
    return R_GAME_OK;
}

static int
R_GameRenderer_PresentImage (
    struct R_CVulkan_Queue*     pPresentQueue,
    struct R_CVulkan_Swapchain  swapchain,
    uint32_t                    imageIndex,
    struct R_CVulkan_Semaphore* pRenderFinishedSemaphore)
{
    VkSwapchainKHR swapchainHandle = R_CVulkan_SwapchainGetHandle (&swapchain);

    enum R_CVulkanError err = R_CVulkan_QueuePresent (
        pPresentQueue,
        &swapchainHandle,
        1,
        &imageIndex,
        pRenderFinishedSemaphore,
        1);
    if (err != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("Failed to present image: %s", R_CVulkanErrorToString (err));
        return R_GAME_ERROR_FAILED;
    }
    return R_GAME_OK;
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
    struct R_GameRendererSubsystem*    pSubsystem = pWorker->pSubsystem;
    struct R_GameRendererThreadPool*   pPool = pSubsystem->pThreadPool;

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
        size_t taskCount = R_CSTL_StackSize (pPool->pTaskQueue) / sizeof (struct R_GameRendererRenderTask);
        if (taskCount == 0)
        {
            R_GAME_MUTEX_UNLOCK (&pPool->taskMutex);
            continue;
        }

        struct R_GameRendererRenderTask task;
        R_CSTL_StackPopData (pPool->pTaskQueue, (uint8_t*)&task, sizeof (struct R_GameRendererRenderTask));

        R_GAME_ATOMIC_DECREMENT (&pPool->atomicPendingTasks);
        R_GAME_MUTEX_UNLOCK (&pPool->taskMutex);

        struct R_GameRendererFrame* pFrame = &pSubsystem->pFrames[task.frameIndex];
        struct R_GameRendererLayer  layer;
        R_GAME_MUTEX_LOCK (&pSubsystem->layerArrayMutex);
        R_CSTL_ArrayTypedAt (pSubsystem->pLayerArray, struct R_GameRendererLayer, task.layerIndex, &layer);
        R_GAME_MUTEX_UNLOCK (&pSubsystem->layerArrayMutex);

        struct R_CVulkan_CommandBuffer cmdBuffer;
        R_CSTL_ArrayTypedUncheckedAt (
            pFrame->pCommandBufferArray,
            struct R_CVulkan_CommandBuffer,
            task.commandBufferIndex,
            &cmdBuffer);

        enum R_CVulkanError err = R_CVulkan_BeginCommandBuffer (&cmdBuffer, 0, NULL);
        if (err == R_CVULKAN_OK)
        {
            if (layer.beforePassCallback)
            {
                layer.beforePassCallback (layer.pUserData, &cmdBuffer, sizeof (cmdBuffer));
            }

            if (layer.renderCallback)
            {
                layer.renderCallback (layer.pUserData, &cmdBuffer, sizeof (cmdBuffer));
            }

            if (layer.afterPassCallback)
            {
                layer.afterPassCallback (layer.pUserData, &cmdBuffer, sizeof (cmdBuffer));
            }

            R_CVulkan_EndCommandBuffer (&cmdBuffer);
            R_CSTL_ArrayTypedSetAtUnchecked (
                pFrame->pCommandBufferArray,
                struct R_CVulkan_CommandBuffer,
                task.commandBufferIndex,
                &cmdBuffer);
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
    pSubsystem->pThreadPool
        = (struct R_GameRendererThreadPool*)R_CSTL_HeapAlloc (sizeof (struct R_GameRendererThreadPool));
    if (pSubsystem->pThreadPool == NULL)
    {
        R_CSTL_LOG_ERROR ("Failed to allocate thread pool");
        return R_GAME_ERROR_OUT_OF_MEMORY;
    }
    memset (pSubsystem->pThreadPool, 0, sizeof (struct R_GameRendererThreadPool));

    struct R_GameRendererThreadPool* pPool = pSubsystem->pThreadPool;

    R_GAME_MUTEX_INIT (&pPool->taskMutex);
    R_GAME_COND_INIT (&pPool->taskAvailable);
    R_GAME_COND_INIT (&pPool->taskComplete);

    pPool->pTaskQueue = R_CSTL_NewStackWithCapacity (
        sizeof (struct R_GameRendererRenderTask) * R_GAME_RENDERER_MAX_COMMAND_BUFFERS_PER_FRAME);
    if (pPool->pTaskQueue == NULL)
    {
        R_CSTL_LOG_ERROR ("Failed to allocate task queue");
        R_CSTL_HeapFree (pSubsystem->pThreadPool);
        return R_GAME_ERROR_OUT_OF_MEMORY;
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
        pWorker->threadHandle
            = CreateThread (NULL, 0, R_GameRenderer_WorkerThreadProc, pWorker, 0, &pWorker->threadId);
        if (pWorker->threadHandle == NULL)
        {
            pPool->workerCount = i;
            R_GameRenderer_ShutdownThreadPool (pSubsystem);
            return R_GAME_ERROR_THREAD_CREATE_FAILED;
        }
#elif defined(__linux__) || defined(__APPLE__)
        int result = pthread_create (&pWorker->threadHandle, NULL, R_GameRenderer_WorkerThreadProc, pWorker);
        if (result != 0)
        {
            pPool->workerCount = i;
            R_GameRenderer_ShutdownThreadPool (pSubsystem);
            return R_GAME_ERROR_THREAD_CREATE_FAILED;
        }
        pWorker->threadId = (pthread_t)pWorker->threadHandle;
#endif
    }
    R_CSTL_LOG_INFO ("Thread pool initialized with %u workers", workerCount);
    return R_GAME_OK;
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
        int                                isRunning = R_GAME_ATOMIC_LOAD (&pWorker->atomicIsRunning);
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

    if (pPool->pTaskQueue)
    {
        R_CSTL_DeleteStack (pPool->pTaskQueue);
    }

    R_CSTL_HeapFree (pSubsystem->pThreadPool);
    pSubsystem->pThreadPool = NULL;

    R_CSTL_LOG_INFO ("Thread pool shutdown complete");
}

static int
R_GameRenderer_InitializeBytecodeDecoder (void)
{
#if defined(R_GAME_DEBUG)
    if (g_pBytecodeDecoder)
    {
        return R_GAME_OK;
    }

    enum R_CSTL_BytecodeArchitecture arch = R_CSTL_BYTECODE_ARCH_X86_64;
#if defined(R_CVULKAN_PLATFORM_WINDOWS) || defined(R_CVULKAN_PLATFORM_LINUX)
    arch = R_CSTL_BYTECODE_ARCH_X86_64;
#elif defined(R_CVULKAN_PLATFORM_ANDROID)
    arch = R_CSTL_BYTECODE_ARCH_ARMEABI_V7A;
#elif defined(R_CVULKAN_PLATFORM_MACOS)
    arch = R_CSTL_BYTECODE_ARCH_ARMV8A;
#endif

    g_pBytecodeDecoder
        = (struct R_CSTL_BytecodeDecoder*)R_CSTL_HeapAlloc (sizeof (struct R_CSTL_BytecodeDecoder));
    if (g_pBytecodeDecoder == NULL)
    {
        R_CSTL_LOG_ERROR ("Failed to allocate bytecode decoder");
        return R_GAME_ERROR_OUT_OF_MEMORY;
    }

    int result = R_CSTL_BytecodeDecoderCreate (arch, g_pBytecodeDecoder);
    if (result != 0)
    {
        R_CSTL_LOG_ERROR ("Failed to create bytecode decoder");
        R_CSTL_HeapFree (g_pBytecodeDecoder);
        g_pBytecodeDecoder = NULL;
        return R_GAME_ERROR_FAILED;
    }
#endif
    return R_GAME_OK;
}

static void
R_GameRenderer_ShutdownBytecodeDecoder (void)
{
#if defined(R_LOG)
    if (g_pBytecodeDecoder)
    {
        R_CSTL_DeleteBytecodeDecoder (g_pBytecodeDecoder);
        R_CSTL_HeapFree (g_pBytecodeDecoder);
        g_pBytecodeDecoder = NULL;
    }
#endif
}

static int
R_GameRenderer_ValidateCallbackFunction (R_CSTL_BytecodeFunction pFunction, const char* pRequiredSymbol)
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
    struct R_GameRendererFrame*   pFrame,
    uint32_t                      frameIdx,
    struct R_CVulkan_Device*      pDevice,
    struct R_CVulkan_CommandPool* pGraphicsPool)
{
    pFrame->pCommandBufferArray = R_CSTL_NewArrayWithCapacity (
        sizeof (struct R_CVulkan_CommandBuffer) * R_GAME_RENDERER_MAX_COMMAND_BUFFERS_PER_FRAME);
    if (pFrame->pCommandBufferArray == NULL)
    {
        R_CSTL_LOG_ERROR ("Failed to allocate command buffer array for frame %u", frameIdx);
        return R_GAME_ERROR_OUT_OF_MEMORY;
    }

    for (uint32_t bufIdx = 0; bufIdx < R_GAME_RENDERER_MAX_COMMAND_BUFFERS_PER_FRAME; ++bufIdx)
    {
        struct R_CVulkan_CommandBuffer cmdBuffer = {0};
        enum R_CVulkanError            err = R_CVulkan_NewCommandBuffer (
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
                R_CVulkanErrorToString (err));
            return R_GAME_ERROR_COMMAND_BUFFER_FAILED;
        }

        int result = R_CSTL_ArrayPushData (
            pFrame->pCommandBufferArray,
            (const uint8_t*)&cmdBuffer,
            sizeof (struct R_CVulkan_CommandBuffer));
        if (result != R_CSTL_OK)
        {
            R_CSTL_LOG_ERROR ("Failed to push command buffer to array");
            return R_GAME_ERROR_ARRAY_OPERATION_FAILED;
        }
    }

    return R_GAME_OK;
}

static int
R_GameRenderer_InitializeSemaphore (
    struct R_GameRendererFrame* pFrame,
    uint32_t                    frameIdx,
    struct R_CVulkan_Device*    pDevice)
{
    pFrame->pRenderFinishedSemaphore
        = (struct R_CVulkan_Semaphore*)R_CSTL_HeapAlloc (sizeof (struct R_CVulkan_Semaphore));
    if (pFrame->pRenderFinishedSemaphore == NULL)
    {
        R_CSTL_LOG_ERROR ("Failed to allocate semaphore for frame %u", frameIdx);
        return R_GAME_ERROR_OUT_OF_MEMORY;
    }
    memset (pFrame->pRenderFinishedSemaphore, 0, sizeof (struct R_CVulkan_Semaphore));

    enum R_CVulkanError err = R_CVulkan_NewSemaphore (pFrame->pRenderFinishedSemaphore, pDevice, 0, 0);
    if (err != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR (
            "Failed to create semaphore for frame %u: %s",
            frameIdx,
            R_CVulkanErrorToString (err));
        return R_GAME_ERROR_FAILED;
    }

    return R_GAME_OK;
}

static int
R_GameRenderer_InitializeFence (
    struct R_GameRendererFrame* pFrame,
    uint32_t                    frameIdx,
    struct R_CVulkan_Device*    pDevice)
{
    pFrame->pInFlightFence = (struct R_CVulkan_Fence*)R_CSTL_HeapAlloc (sizeof (struct R_CVulkan_Fence));
    if (pFrame->pInFlightFence == NULL)
    {
        R_CSTL_LOG_ERROR ("Failed to allocate fence for frame %u", frameIdx);
        return R_GAME_ERROR_OUT_OF_MEMORY;
    }
    memset (pFrame->pInFlightFence, 0, sizeof (struct R_CVulkan_Fence));

    enum R_CVulkanError err = R_CVulkan_NewFence (pFrame->pInFlightFence, pDevice, true);
    if (err != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("Failed to create fence for frame %u: %s", frameIdx, R_CVulkanErrorToString (err));
        return R_GAME_ERROR_FAILED;
    }

    return R_GAME_OK;
}

static int
R_GameRenderer_InitializeFrame (
    struct R_GameRendererSubsystem* pSubsystem,
    struct R_Game_PipelineContext*  pPipelineContext)
{
    struct R_CVulkan_Device*      pDevice = R_Game_PipelineContextGetDevice (pPipelineContext);
    struct R_CVulkan_CommandPool* pGraphicsPool
        = R_Game_PipelineContextGetGraphicsCommandPool (pPipelineContext);

    for (uint32_t frameIdx = 0; frameIdx < pSubsystem->maxFramesInFlight; ++frameIdx)
    {
        struct R_GameRendererFrame* pFrame = &pSubsystem->pFrames[frameIdx];

        if (R_GameRenderer_InitializeCommandBuffers (pFrame, frameIdx, pDevice, pGraphicsPool) != R_GAME_OK)
        {
            return R_GAME_ERROR_FAILED;
        }

        if (R_GameRenderer_InitializeSemaphore (pFrame, frameIdx, pDevice) != R_GAME_OK)
        {
            return R_GAME_ERROR_FAILED;
        }

        if (R_GameRenderer_InitializeFence (pFrame, frameIdx, pDevice) != R_GAME_OK)
        {
            return R_GAME_ERROR_FAILED;
        }

        pFrame->frameNumber = 0;
        pFrame->resourceIndex = 0;
    }

    R_CSTL_LOG_INFO ("Frame data initialized for %u frames", pSubsystem->maxFramesInFlight);
    return R_GAME_OK;
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
        if (pFrame->pCommandBufferArray)
        {
            size_t cmdBufferCount
                = R_CSTL_ArrayLength (pFrame->pCommandBufferArray) / sizeof (struct R_CVulkan_CommandBuffer);

            for (size_t j = 0; j < cmdBufferCount; ++j)
            {
                struct R_CVulkan_CommandBuffer cmdBuffer;
                R_CSTL_ArrayTypedUncheckedAt (
                    pFrame->pCommandBufferArray,
                    struct R_CVulkan_CommandBuffer,
                    j,
                    &cmdBuffer);
                R_CVulkan_DeleteCommandBuffer (&cmdBuffer);
            }

            R_CSTL_DeleteArray (pFrame->pCommandBufferArray);
            pFrame->pCommandBufferArray = NULL;
        }
        if (pFrame->pRenderFinishedSemaphore)
        {
            R_CVulkan_DeleteSemaphore (pFrame->pRenderFinishedSemaphore);
            R_CSTL_HeapFree (pFrame->pRenderFinishedSemaphore);
        }
        if (pFrame->pInFlightFence)
        {
            R_CVulkan_DeleteFence (pFrame->pInFlightFence);
            R_CSTL_HeapFree (pFrame->pInFlightFence);
        }
    }

    R_CSTL_HeapFree (pSubsystem->pFrames);
    pSubsystem->pFrames = NULL;
}

R_GAME_API struct R_GameRendererSubsystem*
R_GameRenderer_NewSubsystem (struct R_GameRendererSubsystem* pSubsystem)
{
#if defined(R_GAME_DEBUG)
    if (pSubsystem == NULL)
    {
        pSubsystem
            = (struct R_GameRendererSubsystem*)R_CSTL_HeapAlloc (sizeof (struct R_GameRendererSubsystem));
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
        R_GAME_MUTEX_DESTROY (&pSubsystem->layerArrayMutex);
        R_GAME_MUTEX_DESTROY (&pSubsystem->resourceArrayMutex);
        R_GameRenderer_CleanupArrays (pSubsystem);
        R_CSTL_HeapFree (pSubsystem);
        return NULL;
    }

return pSubsystem;
}

R_GAME_API int
R_GameRenderer_DeleteSubsystem (struct R_GameRendererSubsystem* pSubsystem)
{
    if (pSubsystem == NULL)
    {
        return R_GAME_ERROR_NULL_POINTER;
    }
    R_GameRenderer_CleanupFrame (pSubsystem);
    R_GameRenderer_CleanupLayers (pSubsystem);
    R_GameRenderer_CleanupResources (pSubsystem);
    R_GameRenderer_ShutdownThreadPool (pSubsystem);
    R_GAME_MUTEX_DESTROY (&pSubsystem->layerArrayMutex);
    R_GAME_MUTEX_DESTROY (&pSubsystem->resourceArrayMutex);

    R_CSTL_HeapFree (pSubsystem);
    R_GameRenderer_ShutdownBytecodeDecoder ();
    return R_GAME_OK;
}

R_GAME_API int
R_GameRenderer_SetPipelineContext (
    struct R_GameRendererSubsystem* pSubsystem,
    struct R_Game_PipelineContext*  pPipelineContext)
{
    if (pSubsystem == NULL || pPipelineContext == NULL)
    {
        R_CSTL_LOG_ERROR ("Invalid parameters for SetPipelineContext");
        return R_GAME_ERROR_INVALID_ARGUMENT;
    }

    pSubsystem->pPipelineContext = pPipelineContext;

    if (R_GameRenderer_InitializeFrame (pSubsystem, pPipelineContext) != R_GAME_OK)
    {
        R_CSTL_LOG_ERROR ("Failed to initialize frame data");
        pSubsystem->pPipelineContext = NULL;
        return R_GAME_ERROR_FRAMEBUFFER_NOT_READY;
    }

    R_CSTL_LOG_INFO ("Pipeline context set successfully");
    return R_GAME_OK;
}

R_GAME_API int
R_GameRenderer_SubsystemStart (struct R_GameRendererSubsystem* pSubsystem)
{
    R_CSTL_TRACE_FUNCTION ();
    R_GAME_VALIDATE_PARAM_BOOTED (pSubsystem);

    if (pSubsystem->pPipelineContext == NULL)
    {
        R_CSTL_LOG_ERROR ("Pipeline context not set");
        R_CSTL_TRACE_RETURN ();
        return R_GAME_ERROR_RENDERER_NOT_SET;
    }

    pSubsystem->state = R_GAME_RENDERER_STATE_RUNNING;
    R_CSTL_TRACE_POINT ("subsystem_started");
    R_CSTL_TRACE_RETURN ();
    return R_GAME_OK;
}

R_GAME_API int
R_GameRenderer_SubsystemStop (struct R_GameRendererSubsystem* pSubsystem)
{
    R_CSTL_TRACE_FUNCTION ();
    R_GAME_VALIDATE_PARAM_BOOTED (pSubsystem);

    R_GameRenderer_WaitForFrame (pSubsystem);

    pSubsystem->state = R_GAME_RENDERER_STATE_STOPPED;
    R_CSTL_TRACE_POINT ("subsystem_stopped");
    R_CSTL_TRACE_RETURN ();
    return R_GAME_OK;
}

R_GAME_API int
R_GameRenderer_SubsystemPause (struct R_GameRendererSubsystem* pSubsystem)
{
    R_CSTL_TRACE_FUNCTION ();
    R_GAME_VALIDATE_PARAM_BOOTED (pSubsystem);

    if (pSubsystem->state != R_GAME_RENDERER_STATE_RUNNING)
    {
        R_CSTL_LOG_ERROR ("Subsystem not running, cannot pause");
        R_CSTL_TRACE_RETURN ();
        return R_GAME_ERROR_INVALID_STATE;
    }

    pSubsystem->state = R_GAME_RENDERER_STATE_PAUSED;
    R_CSTL_TRACE_POINT ("subsystem_paused");
    R_CSTL_TRACE_RETURN ();
    return R_GAME_OK;
}

R_GAME_API int
R_GameRenderer_SubsystemResume (struct R_GameRendererSubsystem* pSubsystem)
{
    R_CSTL_TRACE_FUNCTION ();
    R_GAME_VALIDATE_PARAM_BOOTED (pSubsystem);

    if (pSubsystem->state != R_GAME_RENDERER_STATE_PAUSED)
    {
        R_CSTL_LOG_ERROR ("Subsystem not paused, cannot resume");
        R_CSTL_TRACE_RETURN ();
        return R_GAME_ERROR_INVALID_STATE;
    }

    pSubsystem->state = R_GAME_RENDERER_STATE_RUNNING;
    R_CSTL_TRACE_POINT ("subsystem_resumed");
    R_CSTL_TRACE_RETURN ();
    return R_GAME_OK;
}

R_GAME_API int
R_GameRenderer_BeginFrame (struct R_GameRendererSubsystem* pSubsystem)
{
    R_GAME_VALIDATE_PARAM (pSubsystem);
    R_GAME_VALIDATE_PARAM (pSubsystem->pPipelineContext);

    if (pSubsystem->state != R_GAME_RENDERER_STATE_RUNNING)
    {
        return R_GAME_ERROR_INVALID_STATE;
    }

    struct R_CVulkan_Device* pDevice = R_Game_PipelineContextGetDevice (pSubsystem->pPipelineContext);
    struct R_CVulkan_Fence*  pFence = R_Game_PipelineContextGetInFlightFence (pSubsystem->pPipelineContext);
    uint32_t* pImageIndex = R_Game_PipelineContextGetCurrentFrameIndex (pSubsystem->pPipelineContext);

    if (R_GameRenderer_WaitForFence (pDevice, pFence, UINT64_MAX) != R_GAME_OK)
    {
        return R_GAME_ERROR_FAILED;
    }

    if (R_GameRenderer_AcquireNextImage (pSubsystem->pPipelineContext, pImageIndex) != R_GAME_OK)
    {
        return R_GAME_ERROR_FAILED;
    }

    pSubsystem->currentFrameIndex = *pImageIndex;
    return R_GAME_OK;
}

R_GAME_API int
R_GameRenderer_RenderFrame (struct R_GameRendererSubsystem* pSubsystem)
{
    R_GAME_VALIDATE_PARAM (pSubsystem);
    R_GAME_VALIDATE_PARAM (pSubsystem->pPipelineContext);

    if (pSubsystem->state != R_GAME_RENDERER_STATE_RUNNING)
    {
        return R_GAME_ERROR_INVALID_STATE;
    }

    struct R_GameRendererFrame* pCurrentFrame = &pSubsystem->pFrames[pSubsystem->currentFrameIndex];

    size_t cmdBufferCount
        = R_CSTL_ArrayLength (pCurrentFrame->pCommandBufferArray) / sizeof (struct R_CVulkan_CommandBuffer);

    size_t layerCount = R_CSTL_ArrayLength (pSubsystem->pLayerArray) / sizeof (struct R_GameRendererLayer);

    size_t currentBufferIndex = 0;

    R_GAME_MUTEX_LOCK (&pSubsystem->pThreadPool->taskMutex);
    R_CSTL_DeleteStack (pSubsystem->pThreadPool->pTaskQueue);
    pSubsystem->pThreadPool->pTaskQueue = R_CSTL_NewStackWithCapacity (
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
        int result = R_CSTL_StackPushData (
            pSubsystem->pThreadPool->pTaskQueue,
            (const uint8_t*)&task,
            sizeof (struct R_GameRendererRenderTask));
        R_GAME_ATOMIC_INCREMENT (&pSubsystem->pThreadPool->atomicPendingTasks);
        R_GAME_MUTEX_UNLOCK (&pSubsystem->pThreadPool->taskMutex);

        if (result != R_CSTL_OK)
        {
            R_CSTL_LOG_ERROR ("Failed to add render task to queue");
            return R_GAME_ERROR_ARRAY_OPERATION_FAILED;
        }

        currentBufferIndex++;
    }

    R_GAME_COND_BROADCAST (&pSubsystem->pThreadPool->taskAvailable);

    R_GAME_MUTEX_LOCK (&pSubsystem->pThreadPool->taskMutex);
    while (R_GAME_ATOMIC_LOAD (&pSubsystem->pThreadPool->atomicPendingTasks) > 0)
    {
        R_GAME_MUTEX_UNLOCK (&pSubsystem->pThreadPool->taskMutex);
        R_GAME_COND_WAIT (&pSubsystem->pThreadPool->taskComplete, &pSubsystem->pThreadPool->taskMutex);
        R_GAME_MUTEX_LOCK (&pSubsystem->pThreadPool->taskMutex);
    }
    R_GAME_MUTEX_UNLOCK (&pSubsystem->pThreadPool->taskMutex);

    return R_GAME_OK;
}

R_GAME_API int
R_GameRenderer_EndFrame (struct R_GameRendererSubsystem* pSubsystem)
{
    R_GAME_VALIDATE_PARAM (pSubsystem);
    R_GAME_VALIDATE_PARAM (pSubsystem->pPipelineContext);

    if (pSubsystem->state != R_GAME_RENDERER_STATE_RUNNING)
    {
        return R_GAME_ERROR_INVALID_STATE;
    }

    struct R_CVulkan_Device* pDevice = R_Game_PipelineContextGetDevice (pSubsystem->pPipelineContext);
    struct R_CVulkan_Queue*  pQueue = R_Game_PipelineContextGetGraphicsQueue (pSubsystem->pPipelineContext);
    struct R_CVulkan_Semaphore* pSignalSemaphore
        = R_Game_PipelineContextGetRenderFinishedSemaphore (pSubsystem->pPipelineContext);
    struct R_CVulkan_Semaphore* pWaitSemaphore
        = R_Game_PipelineContextGetImageAvailableSemaphore (pSubsystem->pPipelineContext);
    struct R_CVulkan_Fence* pFence = R_Game_PipelineContextGetInFlightFence (pSubsystem->pPipelineContext);
    uint32_t                imageIndex = pSubsystem->currentFrameIndex;

    struct R_GameRendererFrame* pFrame = &pSubsystem->pFrames[imageIndex];
    size_t                      cmdBufferCount
        = R_CSTL_ArrayLength (pFrame->pCommandBufferArray) / sizeof (struct R_CVulkan_CommandBuffer);

    if (cmdBufferCount > 0)
    {
        struct R_CVulkan_CommandBuffer* pCmdBuffers
            = (struct R_CVulkan_CommandBuffer*)R_CSTL_ArrayData (pFrame->pCommandBufferArray);
        if (R_GameRenderer_SubmitCommandBuffers (
                pQueue,
                &pCmdBuffers,
                cmdBufferCount,
                pWaitSemaphore,
                pSignalSemaphore,
                pFence)
            != R_GAME_OK)
        {
            return R_GAME_ERROR_FAILED;
        }
    }

    return R_GAME_OK;
}

R_GAME_API int
R_GameRenderer_WaitForFrame (struct R_GameRendererSubsystem* pSubsystem)
{
    R_GAME_VALIDATE_PARAM (pSubsystem);
    R_GAME_VALIDATE_PARAM (pSubsystem->pPipelineContext);

    struct R_Game_PipelineContext* pPipelineContext = pSubsystem->pPipelineContext;
    struct R_CVulkan_Queue*        pQueue = R_Game_PipelineContextGetPresentQueue (pPipelineContext);
    struct R_CVulkan_Semaphore*    pSignalSemaphore
        = R_Game_PipelineContextGetRenderFinishedSemaphore (pPipelineContext);
    uint32_t imageIndex = pSubsystem->currentFrameIndex;

    struct R_CVulkan_Swapchain swapchain = pPipelineContext->swapchain;
    if (R_GameRenderer_PresentImage (pQueue, swapchain, imageIndex, pSignalSemaphore) != R_GAME_OK)
    {
        return R_GAME_ERROR_FAILED;
    }

    pSubsystem->currentFrameIndex = (pSubsystem->currentFrameIndex + 1) % pSubsystem->maxFramesInFlight;
    return R_GAME_OK;
}

R_GAME_API int
R_GameRenderer_AddLayer (
    struct R_GameRendererSubsystem* pSubsystem,
    const char*                     pName,
    uint32_t                        priority,
    uint32_t                        flags,
    void*                           pUserData)
{
    R_GAME_VALIDATE_PARAM_BOOTED (pSubsystem);
    R_GAME_VALIDATE_PARAM (pName);

    size_t currentLayerCount
        = R_CSTL_ArrayLength (pSubsystem->pLayerArray) / sizeof (struct R_GameRendererLayer);
    if (currentLayerCount >= R_GAME_RENDERER_MAX_LAYERS)
    {
        return R_GAME_ERROR_MAX_RESOURCES_REACHED;
    }
    struct R_GameRendererLayer newLayer = {0};
    newLayer.priority = priority;
    newLayer.flags = flags;
    newLayer.pUserData = (void*)pUserData;

    if (pName)
    {
        size_t nameLen = strlen (pName);
        newLayer.pName = (char*)R_CSTL_HeapAlloc (nameLen + 1);
        if (newLayer.pName == NULL)
        {
            R_CSTL_LOG_ERROR ("Failed to allocate layer name");
            return R_GAME_ERROR_OUT_OF_MEMORY;
        }
        memcpy (newLayer.pName, pName, nameLen + 1);
    }

    R_GAME_MUTEX_LOCK (&pSubsystem->layerArrayMutex);
    int result = R_CSTL_ArrayPushData (
        pSubsystem->pLayerArray,
        (const uint8_t*)&newLayer,
        sizeof (struct R_GameRendererLayer));
    R_GAME_MUTEX_UNLOCK (&pSubsystem->layerArrayMutex);

    if (result != R_CSTL_OK)
    {
        if (newLayer.pName)
        {
            R_CSTL_HeapFree (newLayer.pName);
        }
        return R_GAME_ERROR_ARRAY_OPERATION_FAILED;
    }

    return R_GAME_OK;
}

R_GAME_API int
R_GameRenderer_RemoveLayer (struct R_GameRendererSubsystem* pSubsystem, uint32_t layerIndex)
{
    R_GAME_VALIDATE_PARAM (pSubsystem);
    size_t layerCount = R_CSTL_ArrayLength (pSubsystem->pLayerArray) / sizeof (struct R_GameRendererLayer);
    R_GAME_ASSERT (layerIndex < layerCount);

    R_GAME_MUTEX_LOCK (&pSubsystem->layerArrayMutex);
    struct R_GameRendererLayer layer;
    R_CSTL_ArrayTypedAt (pSubsystem->pLayerArray, struct R_GameRendererLayer, layerIndex, &layer);
    if (layer.pName)
    {
        R_CSTL_HeapFree (layer.pName);
    }
    R_GameRenderer_RemoveFromArrayByIndex (
        &pSubsystem->pLayerArray,
        sizeof (struct R_GameRendererLayer),
        layerIndex);
    R_GAME_MUTEX_UNLOCK (&pSubsystem->layerArrayMutex);

    return R_GAME_OK;
}

R_GAME_API int
R_GameRenderer_SetLayerEnabled (struct R_GameRendererSubsystem* pSubsystem, uint32_t layerIndex, int enabled)
{
    R_GAME_VALIDATE_PARAM (pSubsystem);
    size_t layerCount = R_CSTL_ArrayLength (pSubsystem->pLayerArray) / sizeof (struct R_GameRendererLayer);
    R_GAME_ASSERT (layerIndex < layerCount);

    R_GAME_MUTEX_LOCK (&pSubsystem->layerArrayMutex);
    struct R_GameRendererLayer layer;
    R_CSTL_ArrayTypedAt (pSubsystem->pLayerArray, struct R_GameRendererLayer, layerIndex, &layer);
    if (enabled) layer.flags |= R_GAME_RENDERER_LAYER_FLAG_ENABLED;
    else layer.flags &= ~R_GAME_RENDERER_LAYER_FLAG_ENABLED;
    R_CSTL_ArrayTypedSetAt (pSubsystem->pLayerArray, struct R_GameRendererLayer, layerIndex, &layer);
    R_GAME_MUTEX_UNLOCK (&pSubsystem->layerArrayMutex);

    return R_GAME_OK;
}

R_GAME_API int
R_GameRenderer_SetLayerRenderCallback (
    struct R_GameRendererSubsystem* pSubsystem,
    uint32_t                        layerIndex,
    R_GameLifecycleRender           callback)
{
    R_GAME_VALIDATE_PARAM (pSubsystem);
    size_t layerCount = R_CSTL_ArrayLength (pSubsystem->pLayerArray) / sizeof (struct R_GameRendererLayer);
    R_GAME_ASSERT (layerIndex < layerCount);

    R_GAME_MUTEX_LOCK (&pSubsystem->layerArrayMutex);
    struct R_GameRendererLayer layer;
    R_CSTL_ArrayTypedAt (pSubsystem->pLayerArray, struct R_GameRendererLayer, layerIndex, &layer);
    layer.renderCallback = callback;
    R_CSTL_ArrayTypedSetAt (pSubsystem->pLayerArray, struct R_GameRendererLayer, layerIndex, &layer);
    R_GAME_MUTEX_UNLOCK (&pSubsystem->layerArrayMutex);

    return R_GAME_OK;
}

R_GAME_API int
R_GameRenderer_SetLayerBeforePassCallback (
    struct R_GameRendererSubsystem* pSubsystem,
    uint32_t                        layerIndex,
    R_GameLifecycleBeforePass       callback)
{
    R_GAME_VALIDATE_PARAM (pSubsystem);
    size_t layerCount = R_CSTL_ArrayLength (pSubsystem->pLayerArray) / sizeof (struct R_GameRendererLayer);
    R_GAME_ASSERT (layerIndex < layerCount);

    R_GAME_MUTEX_LOCK (&pSubsystem->layerArrayMutex);
    struct R_GameRendererLayer layer;
    R_CSTL_ArrayTypedAt (pSubsystem->pLayerArray, struct R_GameRendererLayer, layerIndex, &layer);
    layer.beforePassCallback = callback;
    R_CSTL_ArrayTypedSetAt (pSubsystem->pLayerArray, struct R_GameRendererLayer, layerIndex, &layer);
    R_GAME_MUTEX_UNLOCK (&pSubsystem->layerArrayMutex);

    return R_GAME_OK;
}

R_GAME_API int
R_GameRenderer_SetLayerAfterPassCallback (
    struct R_GameRendererSubsystem* pSubsystem,
    uint32_t                        layerIndex,
    R_GameLifecycleAfterPass        callback)
{
    R_GAME_VALIDATE_PARAM (pSubsystem);
    size_t layerCount = R_CSTL_ArrayLength (pSubsystem->pLayerArray) / sizeof (struct R_GameRendererLayer);
    R_GAME_ASSERT (layerIndex < layerCount);

    R_GAME_MUTEX_LOCK (&pSubsystem->layerArrayMutex);
    struct R_GameRendererLayer layer;
    R_CSTL_ArrayTypedAt (pSubsystem->pLayerArray, struct R_GameRendererLayer, layerIndex, &layer);
    layer.afterPassCallback = callback;
    R_CSTL_ArrayTypedSetAt (pSubsystem->pLayerArray, struct R_GameRendererLayer, layerIndex, &layer);
    R_GAME_MUTEX_UNLOCK (&pSubsystem->layerArrayMutex);

    return R_GAME_OK;
}

R_GAME_API struct R_GameRendererLayer*
R_GameRenderer_GetLayer (struct R_GameRendererSubsystem* pSubsystem, uint32_t layerIndex)
{
    R_GAME_VALIDATE_PARAM (pSubsystem);
    size_t layerCount = R_CSTL_ArrayLength (pSubsystem->pLayerArray) / sizeof (struct R_GameRendererLayer);
    R_GAME_ASSERT (layerIndex < layerCount);
    R_GAME_MUTEX_LOCK (&pSubsystem->layerArrayMutex);
    const uint8_t*                    pLayerData = R_CSTL_ArrayData (pSubsystem->pLayerArray);
    static struct R_GameRendererLayer s_layer;
    memcpy (
        &s_layer,
        pLayerData + layerIndex * sizeof (struct R_GameRendererLayer),
        sizeof (struct R_GameRendererLayer));
    R_GAME_MUTEX_UNLOCK (&pSubsystem->layerArrayMutex);
    return &s_layer;
}

R_GAME_API int
R_GameRenderer_SortLayers (struct R_GameRendererSubsystem* pSubsystem)
{
    R_GAME_VALIDATE_PARAM (pSubsystem);
    R_GAME_MUTEX_LOCK (&pSubsystem->layerArrayMutex);
    int result = R_CSTL_ArraySort (
        pSubsystem->pLayerArray,
        sizeof (struct R_GameRendererLayer),
        R_GameRenderer_CompareLayers,
        NULL);
    R_GAME_MUTEX_UNLOCK (&pSubsystem->layerArrayMutex);
    return result;
}

R_GAME_API uint64_t
R_GameRenderer_RegisterResource (
    struct R_GameRendererSubsystem* pSubsystem,
    uint32_t                        type,
    void*                           pResource,
    uint64_t                        size,
    const char*                     pName)
{
    R_GAME_VALIDATE_PARAM_BOOTED (pSubsystem);
    R_GAME_VALIDATE_PARAM (pResource);

    size_t currentResourceCount
        = R_CSTL_ArrayLength (pSubsystem->pResourceArray) / sizeof (struct R_GameRendererResource);
    if (currentResourceCount >= R_GAME_RENDERER_MAX_RESOURCES)
    {
        R_CSTL_LOG_ERROR ("Maximum number of resources reached");
        return 0;
    }

    struct R_GameRendererResource newResource = {0};
    newResource.type = type;
    newResource.pResource = pResource;
    newResource.size = size;
    newResource.handle = ++pSubsystem->nextResourceHandle;
    if (newResource.handle == 0)
    {
        newResource.handle = ++pSubsystem->nextResourceHandle;
    }
    if (pName)
    {
        size_t nameLen = strlen (pName);
        newResource.pName = (char*)R_CSTL_HeapAlloc (nameLen + 1);
        if (newResource.pName == NULL)
        {
            return 0;
        }
        memcpy (newResource.pName, pName, nameLen + 1);
    }

    R_GAME_MUTEX_LOCK (&pSubsystem->resourceArrayMutex);
    int result = R_CSTL_ArrayPushData (
        pSubsystem->pResourceArray,
        (const uint8_t*)&newResource,
        sizeof (struct R_GameRendererResource));
    R_GAME_MUTEX_UNLOCK (&pSubsystem->resourceArrayMutex);

    if (result != R_CSTL_OK)
    {
        if (newResource.pName)
        {
            R_CSTL_HeapFree ((void*)newResource.pName);
        }
        return 0;
    }
    return newResource.handle;
}

R_GAME_API int
R_GameRenderer_UnregisterResource (struct R_GameRendererSubsystem* pSubsystem, uint64_t handle)
{
    R_GAME_VALIDATE_PARAM (pSubsystem);
    R_GAME_VALIDATE_PARAM (handle != 0);
    size_t foundIndex = R_GameRenderer_FindResourceIndexByHandle (pSubsystem, handle);
    if (foundIndex == SIZE_MAX)
    {
        return R_GAME_ERROR_RESOURCE_NOT_FOUND;
    }

    R_GAME_MUTEX_LOCK (&pSubsystem->resourceArrayMutex);
    struct R_GameRendererResource resource;
    R_CSTL_ArrayTypedAt (pSubsystem->pResourceArray, struct R_GameRendererResource, foundIndex, &resource);
    if (resource.pName)
    {
        R_CSTL_HeapFree ((void*)resource.pName);
    }
    R_GameRenderer_RemoveFromArrayByIndex (
        &pSubsystem->pResourceArray,
        sizeof (struct R_GameRendererResource),
        foundIndex);
    R_GAME_MUTEX_UNLOCK (&pSubsystem->resourceArrayMutex);

    return R_GAME_OK;
}

R_GAME_API const void*
R_GameRenderer_GetResource (struct R_GameRendererSubsystem* pSubsystem, uint64_t handle)
{
    R_GAME_VALIDATE_PARAM (pSubsystem);
    R_GAME_VALIDATE_PARAM (handle != 0);
    size_t index = R_GameRenderer_FindResourceIndexByHandle (pSubsystem, handle);
    if (index == SIZE_MAX)
    {
        return NULL;
    }

    R_GAME_MUTEX_LOCK (&pSubsystem->resourceArrayMutex);
    struct R_GameRendererResource resource;
    R_CSTL_ArrayTypedAt (pSubsystem->pResourceArray, struct R_GameRendererResource, index, &resource);
    R_GAME_MUTEX_UNLOCK (&pSubsystem->resourceArrayMutex);

    return (void*)resource.pResource;
}

R_GAME_API uint32_t
R_GameRenderer_GetResourceType (struct R_GameRendererSubsystem* pSubsystem, uint64_t handle)
{
    R_GAME_VALIDATE_PARAM (pSubsystem);
    R_GAME_VALIDATE_PARAM (handle != 0);

    size_t index = R_GameRenderer_FindResourceIndexByHandle (pSubsystem, handle);
    if (index == SIZE_MAX)
    {
        return 0;
    }

    R_GAME_MUTEX_LOCK (&pSubsystem->resourceArrayMutex);
    struct R_GameRendererResource resource;
    R_CSTL_ArrayTypedAt (pSubsystem->pResourceArray, struct R_GameRendererResource, index, &resource);
    R_GAME_MUTEX_UNLOCK (&pSubsystem->resourceArrayMutex);

    return resource.type;
}

R_GAME_API uint64_t
R_GameRenderer_GetResourceSize (struct R_GameRendererSubsystem* pSubsystem, uint64_t handle)
{
    R_GAME_VALIDATE_PARAM (pSubsystem);
    R_GAME_VALIDATE_PARAM (handle != 0);

    size_t index = R_GameRenderer_FindResourceIndexByHandle (pSubsystem, handle);
    if (index == SIZE_MAX)
    {
        return 0;
    }

    R_GAME_MUTEX_LOCK (&pSubsystem->resourceArrayMutex);
    struct R_GameRendererResource resource;
    R_CSTL_ArrayTypedAt (pSubsystem->pResourceArray, struct R_GameRendererResource, index, &resource);
    R_GAME_MUTEX_UNLOCK (&pSubsystem->resourceArrayMutex);

    return resource.size;
}

R_GAME_API int
R_GameRenderer_SetFrameResource (
    struct R_GameRendererSubsystem* pSubsystem,
    uint32_t                        frameIndex,
    uint32_t                        bufferIndex)
{
    R_GAME_VALIDATE_PARAM (pSubsystem);
    R_GAME_ASSERT (frameIndex < pSubsystem->maxFramesInFlight);

    struct R_GameRendererFrame* pFrame = &pSubsystem->pFrames[frameIndex];
    pFrame->resourceIndex = bufferIndex;
    return R_GAME_OK;
}

R_GAME_API struct R_Game_RendererManager*
R_GameRenderer_NewManager (struct R_Game_PipelineContext* pPipelineContext)
{
    R_CSTL_TRACE_FUNCTION ();
    R_GAME_VALIDATE_PARAM (pPipelineContext);

    struct R_Game_RendererManager* pManager
        = (struct R_Game_RendererManager*)R_CSTL_HeapAlloc (sizeof (struct R_Game_RendererManager));
    if (pManager == NULL)
    {
        R_CSTL_TRACE_RETURN ();
        return NULL;
    }

    memset (pManager, 0, sizeof (struct R_Game_RendererManager));
    pManager->pPipelineContext = pPipelineContext;
    pManager->subsystemCount = 0;
    pManager->swapchainImageCount = 0;
    pManager->currentSwapchainIndex = 0;

    R_GAME_MUTEX_INIT (&pManager->managerMutex);

    R_CSTL_TRACE_POINT ("manager_created");
    R_CSTL_TRACE_RETURN ();
    return pManager;
}

R_GAME_API int
R_GameRenderer_DeleteManager (struct R_Game_RendererManager* pManager)
{
    R_CSTL_TRACE_FUNCTION ();
    R_GAME_VALIDATE_PARAM (pManager);
    R_GAME_VALIDATE_PARAM_BOOTED (pManager);

    R_GAME_MUTEX_LOCK (&pManager->managerMutex);

    for (uint32_t i = 0; i < pManager->subsystemCount; ++i)
    {
        struct R_GameRendererSubsystemEntry* pEntry = &pManager->subsystems[i];
        if (pEntry->pSubsystem)
        {
            R_GameRenderer_DeleteSubsystem (pEntry->pSubsystem);
        }
    }

    R_GAME_MUTEX_UNLOCK (&pManager->managerMutex);
    R_GAME_MUTEX_DESTROY (&pManager->managerMutex);

    R_CSTL_HeapFree (pManager);
    R_CSTL_TRACE_POINT ("manager_deleted");
    R_CSTL_TRACE_RETURN ();
    return R_GAME_OK;
}

R_GAME_API int
R_GameRenderer_AddSubsystem (
    struct R_Game_RendererManager*   pManager,
    struct R_GameRendererSubsystem* pSubsystem,
    uint32_t                        priority,
    uint32_t                        flags,
    float                           blendFactor)
{
    R_CSTL_TRACE_FUNCTION_CTX ("priority=%u, blend=%f", priority, blendFactor);
    R_GAME_VALIDATE_PARAM (pManager);
    R_GAME_VALIDATE_PARAM (pSubsystem);
    R_GAME_VALIDATE_PARAM_BOOTED (pManager);

    R_GAME_MUTEX_LOCK (&pManager->managerMutex);

    if (pManager->subsystemCount >= R_GAME_RENDERER_MAX_SUBSYSTEMS)
    {
        R_CSTL_LOG_ERROR ("Maximum number of subsystems reached");
        R_GAME_MUTEX_UNLOCK (&pManager->managerMutex);
        R_CSTL_TRACE_RETURN ();
        return R_GAME_ERROR_MAX_RESOURCES_REACHED;
    }

    struct R_GameRendererSubsystemEntry* pEntry = &pManager->subsystems[pManager->subsystemCount];
    pEntry->pSubsystem = pSubsystem;
    pEntry->priority = priority;
    pEntry->flags = flags;
    pEntry->isVisible = 1;
    pEntry->blendFactor = blendFactor;
    memset (&pEntry->renderTarget, 0, sizeof (struct R_GameRendererRenderTarget));

    pManager->subsystemCount++;
    R_GAME_MUTEX_UNLOCK (&pManager->managerMutex);
    R_CSTL_TRACE_RETURN ();
    return R_GAME_OK;
}

R_GAME_API int
R_GameRenderer_RemoveSubsystem (
    struct R_Game_RendererManager*   pManager,
    struct R_GameRendererSubsystem* pSubsystem)
{
    R_CSTL_TRACE_FUNCTION ();
    R_GAME_VALIDATE_PARAM (pManager);
    R_GAME_VALIDATE_PARAM (pSubsystem);
    R_GAME_VALIDATE_PARAM_BOOTED (pManager);

    R_GAME_MUTEX_LOCK (&pManager->managerMutex);

    uint32_t foundIndex = UINT32_MAX;
    for (uint32_t i = 0; i < pManager->subsystemCount; ++i)
    {
        if (pManager->subsystems[i].pSubsystem == pSubsystem)
        {
            foundIndex = i;
            break;
        }
    }
    if (foundIndex == UINT32_MAX)
    {
        R_CSTL_LOG_WARN ("Subsystem not found in manager");
        R_GAME_MUTEX_UNLOCK (&pManager->managerMutex);
        return R_GAME_ERROR_SUBSYSTEM_NOT_FOUND;
    }

    for (uint32_t i = foundIndex; i < pManager->subsystemCount - 1; ++i)
    {
        pManager->subsystems[i] = pManager->subsystems[i + 1];
    }
    pManager->subsystemCount--;
    R_GAME_MUTEX_UNLOCK (&pManager->managerMutex);
    R_CSTL_TRACE_RETURN ();
    return R_GAME_OK;
}

R_GAME_API int
R_GameRenderer_SetSubsystemVisible (
    struct R_Game_RendererManager*   pManager,
    struct R_GameRendererSubsystem* pSubsystem,
    int                             visible)
{
    R_CSTL_TRACE_FUNCTION_CTX ("visible=%d", visible);
    R_GAME_VALIDATE_PARAM (pManager);
    R_GAME_VALIDATE_PARAM (pSubsystem);
    R_GAME_VALIDATE_PARAM_BOOTED (pManager);

    R_GAME_MUTEX_LOCK (&pManager->managerMutex);

    for (uint32_t i = 0; i < pManager->subsystemCount; ++i)
    {
        if (pManager->subsystems[i].pSubsystem == pSubsystem)
        {
            pManager->subsystems[i].isVisible = visible ? 1 : 0;
            R_GAME_MUTEX_UNLOCK (&pManager->managerMutex);
            R_CSTL_TRACE_RETURN ();
            return R_GAME_OK;
        }
    }

    R_GAME_MUTEX_UNLOCK (&pManager->managerMutex);
    R_CSTL_TRACE_RETURN ();
    return R_GAME_ERROR_SUBSYSTEM_NOT_FOUND;
}

R_GAME_API int
R_GameRenderer_SetSubsystemBlendFactor (
    struct R_Game_RendererManager*   pManager,
    struct R_GameRendererSubsystem* pSubsystem,
    float                           blendFactor)
{
    R_CSTL_TRACE_FUNCTION_CTX ("blendFactor=%f", blendFactor);
    R_GAME_VALIDATE_PARAM (pManager);
    R_GAME_VALIDATE_PARAM (pSubsystem);
    R_GAME_VALIDATE_PARAM_BOOTED (pManager);

    R_GAME_MUTEX_LOCK (&pManager->managerMutex);

    for (uint32_t i = 0; i < pManager->subsystemCount; ++i)
    {
        if (pManager->subsystems[i].pSubsystem == pSubsystem)
        {
            pManager->subsystems[i].blendFactor = blendFactor;
            R_GAME_MUTEX_UNLOCK (&pManager->managerMutex);
            R_CSTL_TRACE_RETURN ();
            return R_GAME_OK;
        }
    }
    R_GAME_MUTEX_UNLOCK (&pManager->managerMutex);
    R_CSTL_TRACE_RETURN ();
    return R_GAME_ERROR_SUBSYSTEM_NOT_FOUND;
}

R_GAME_API int
R_GameRenderer_ComposeFrame (struct R_Game_RendererManager* pManager)
{
    R_GAME_VALIDATE_PARAM (pManager);
    R_GAME_VALIDATE_PARAM_BOOTED (pManager);

    R_GAME_MUTEX_LOCK (&pManager->managerMutex);

    for (uint32_t i = 0; i < pManager->subsystemCount; ++i)
    {
        struct R_GameRendererSubsystemEntry* pEntry = &pManager->subsystems[i];
        if (pEntry->pSubsystem && pEntry->isVisible)
        {
            R_GameRenderer_BeginFrame (pEntry->pSubsystem);
            R_GameRenderer_RenderFrame (pEntry->pSubsystem);
            R_GameRenderer_EndFrame (pEntry->pSubsystem);
        }
    }
    R_GAME_MUTEX_UNLOCK (&pManager->managerMutex);
    return R_GAME_OK;
}

R_GAME_API int
R_GameRenderer_PresentFrame (struct R_Game_RendererManager* pManager)
{
    R_GAME_VALIDATE_PARAM (pManager);
    R_GAME_VALIDATE_PARAM_BOOTED (pManager);

    R_GAME_MUTEX_LOCK (&pManager->managerMutex);

    for (uint32_t i = 0; i < pManager->subsystemCount; ++i)
    {
        struct R_GameRendererSubsystemEntry* pEntry = &pManager->subsystems[i];
        if (pEntry->pSubsystem && pEntry->isVisible)
        {
            R_GameRenderer_WaitForFrame (pEntry->pSubsystem);
        }
    }
    R_GAME_MUTEX_UNLOCK (&pManager->managerMutex);
    return R_GAME_OK;
}
