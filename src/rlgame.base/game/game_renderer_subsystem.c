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

static int r_game_renderer_acquire_swapchain_image (
    struct r_game_pipeline_context* pPipelineContext,
    uint32_t*                       pImageIndex);

struct r_game_renderer_frame
{
        struct r_cstl_array*        pCommandBufferArray;
        struct R_CVulkan_Semaphore* pRenderFinishedSemaphore;
        struct R_CVulkan_Fence*     pInFlightFence;
        uint32_t                    resourceIndex;
        uint64_t                    frameNumber;
};

struct r_game_renderer_layer
{
        char*                        pName;
        uint32_t                     priority;
        uint32_t                     flags;
        void*                        pUserData;
        r_game_lifecycle_render      renderCallback;
        r_game_lifecycle_before_pass beforePassCallback;
        r_game_lifecycle_after_pass  afterPassCallback;
};

struct r_game_renderer_resource
{
        uint64_t    handle;
        uint32_t    type;
        const void* pResource;
        uint64_t    size;
        uint32_t    refCount;
        char*       pName;
};

struct r_game_renderer_lifecycle
{
        const void*                  pRenderer;
        r_game_lifecycle_construct   constructCallback;
        r_game_lifecycle_resume      resumeCallback;
        r_game_lifecycle_pause       pauseCallback;
        r_game_lifecycle_before_each beforeEachCallback;
        r_game_lifecycle_after_each  afterEachCallback;
        r_game_lifecycle_before_pass beforePassCallback;
        r_game_lifecycle_after_pass  afterPassCallback;
        r_game_lifecycle_render      renderCallback;
        r_game_lifecycle_stop        stopCallback;
        r_game_lifecycle_over        overCallback;
};

struct r_game_renderer_subsystem
{
        struct r_game_pipeline_context*  pPipelineContext;
        struct r_game_renderer_lifecycle lifecycle;

        struct r_game_renderer_frame* pFrames;
        uint32_t                      maxFramesInFlight;
        uint32_t                      currentFrameIndex;
        uint64_t                      frameCounter;

        struct r_cstl_array* pLayerArray;
        struct r_cstl_array* pResourceArray;
        uint64_t             nextResourceHandle;

        struct r_game_renderer_thread_pool* pThreadPool;
        R_GAME_MUTEX                        layerArrayMutex;
        R_GAME_MUTEX                        resourceArrayMutex;

        uint32_t state;
};

struct r_game_renderer_manager
{
        struct r_game_renderer_subsystem_entry subsystems[R_GAME_RENDERER_MAX_SUBSYSTEMS];
        uint32_t                               subsystemCount;
        struct r_game_pipeline_context*        pPipelineContext;
        struct R_CVulkan_RenderPass*           pCompositionRenderPass;
        struct R_CVulkan_Pipeline*             pCompositionPipeline;
        struct R_CVulkan_Image*                pSwapchainImages[R_GAME_RENDERER_MAX_SWAPCHAIN_IMAGES];
        struct R_CVulkan_ImageView*            pSwapchainImageViews[R_GAME_RENDERER_MAX_SWAPCHAIN_IMAGES];
        struct R_CVulkan_Framebuffer*          pSwapchainFramebuffers[R_GAME_RENDERER_MAX_SWAPCHAIN_IMAGES];
        uint32_t                               swapchainImageCount;
        uint32_t                               currentSwapchainIndex;
        R_GAME_MUTEX                           managerMutex;
};
static struct r_cstl_bytecode_decoder* g_pBytecodeDecoder = NULL;

static int
r_game_renderer_compare_layers (const void* pA, const void* pB, void* pData)
{
    const struct r_game_renderer_layer* pLayerA = (const struct r_game_renderer_layer*)pA;
    const struct r_game_renderer_layer* pLayerB = (const struct r_game_renderer_layer*)pB;
    return (int)pLayerA->priority - (int)pLayerB->priority;
}

static void r_game_renderer_shutdown_thread_pool (struct r_game_renderer_subsystem* pSubsystem);

static int
r_game_renderer_initialize_arrays (struct r_game_renderer_subsystem* pSubsystem)
{
    pSubsystem->pFrames = (struct r_game_renderer_frame*)r_cstl_heap_alloc (
        sizeof (struct r_game_renderer_frame) * pSubsystem->maxFramesInFlight);
    if (pSubsystem->pFrames == NULL)
    {
        R_CSTL_LOG_ERROR ("Failed to allocate frame data");
        return R_GAME_ERROR_OUT_OF_MEMORY;
    }
    memset (pSubsystem->pFrames, 0, sizeof (struct r_game_renderer_frame) * pSubsystem->maxFramesInFlight);

    pSubsystem->pLayerArray
        = r_cstl_new_array_with_capacity (sizeof (struct r_game_renderer_layer) * R_GAME_RENDERER_MAX_LAYERS);
    if (pSubsystem->pLayerArray == NULL)
    {
        R_CSTL_LOG_ERROR ("Failed to allocate render layer array");
        r_cstl_heap_free (pSubsystem->pFrames);
        return R_GAME_ERROR_OUT_OF_MEMORY;
    }

    pSubsystem->pResourceArray = r_cstl_new_array_with_capacity (
        sizeof (struct r_game_renderer_resource) * R_GAME_RENDERER_MAX_RESOURCES);
    if (pSubsystem->pResourceArray == NULL)
    {
        R_CSTL_LOG_ERROR ("Failed to allocate resource array");
        r_cstl_delete_array (pSubsystem->pLayerArray);
        r_cstl_heap_free (pSubsystem->pFrames);
        return R_GAME_ERROR_OUT_OF_MEMORY;
    }

    return R_GAME_OK;
}

static int
r_game_renderer_wait_for_fence (
    struct R_CVulkan_Device* pDevice,
    struct R_CVulkan_Fence*  pFence,
    uint64_t                 timeout)
{
    enum R_CVulkan_Error err = R_CVulkan_FenceWait (pDevice, pFence, 1, true, timeout);
    if (err != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("Failed to wait for fence: %s", r_cvulkan_error_to_string (err));
        return R_GAME_ERROR_FAILED;
    }
    return R_GAME_OK;
}

static int
r_game_renderer_acquire_next_image (struct r_game_pipeline_context* pPipelineContext, uint32_t* pImageIndex)
{
    return r_game_renderer_acquire_swapchain_image (pPipelineContext, pImageIndex);
}

static void
r_game_renderer_initialize_state (struct r_game_renderer_subsystem* pSubsystem)
{
    pSubsystem->currentFrameIndex = 0;
    pSubsystem->frameCounter = 0;
    pSubsystem->state = R_GAME_RENDERER_STATE_STOPPED;
    pSubsystem->nextResourceHandle = 1;

    R_GAME_MUTEX_INIT (&pSubsystem->layerArrayMutex);
    R_GAME_MUTEX_INIT (&pSubsystem->resourceArrayMutex);
}

static void
r_game_renderer_cleanup_arrays (struct r_game_renderer_subsystem* pSubsystem)
{
    if (pSubsystem->pFrames)
    {
        r_cstl_heap_free (pSubsystem->pFrames);
        pSubsystem->pFrames = NULL;
    }
    if (pSubsystem->pLayerArray)
    {
        r_cstl_delete_array (pSubsystem->pLayerArray);
        pSubsystem->pLayerArray = NULL;
    }
    if (pSubsystem->pResourceArray)
    {
        r_cstl_delete_array (pSubsystem->pResourceArray);
        pSubsystem->pResourceArray = NULL;
    }
}

static void
r_game_renderer_cleanup_layers (struct r_game_renderer_subsystem* pSubsystem)
{
    if (pSubsystem->pLayerArray)
    {
        size_t layerCount
            = r_cstl_array_length (pSubsystem->pLayerArray) / sizeof (struct r_game_renderer_layer);

        for (size_t i = 0; i < layerCount; ++i)
        {
            struct r_game_renderer_layer layer;
            r_cstl_array_typed_unchecked_at (pSubsystem->pLayerArray, struct r_game_renderer_layer, i, &layer);
            if (layer.pName)
            {
                r_cstl_heap_free (layer.pName);
            }
        }
        r_cstl_delete_array (pSubsystem->pLayerArray);
        pSubsystem->pLayerArray = NULL;
    }
}

static void
r_game_renderer_cleanup_resources (struct r_game_renderer_subsystem* pSubsystem)
{
    if (pSubsystem->pResourceArray)
    {
        size_t resourceCount
            = r_cstl_array_length (pSubsystem->pResourceArray) / sizeof (struct r_game_renderer_resource);

        for (size_t i = 0; i < resourceCount; ++i)
        {
            struct r_game_renderer_resource resource;
            r_cstl_array_typed_unchecked_at (
                pSubsystem->pResourceArray,
                struct r_game_renderer_resource,
                i,
                &resource);
            if (resource.pName)
            {
                r_cstl_heap_free ((void*)resource.pName);
            }
        }
        r_cstl_delete_array (pSubsystem->pResourceArray);
        pSubsystem->pResourceArray = NULL;
    }
}

static char*
r_game_renderer_allocate_name_copy (const char* pName)
{
    if (pName == NULL)
    {
        return NULL;
    }
    size_t nameLen = strlen (pName);
    char*  pNameCopy = (char*)r_cstl_heap_alloc (nameLen + 1);
    if (pNameCopy)
    {
        memcpy (pNameCopy, pName, nameLen);
        pNameCopy[nameLen] = '\0';
    }
    return pNameCopy;
}

static int
r_game_renderer_remove_from_array_by_index (
    struct r_cstl_array** ppArray,
    size_t                elementSize,
    size_t                removeIndex)
{
    struct r_cstl_array* pArray = *ppArray;
    size_t               currentCount = r_cstl_array_length (pArray) / elementSize;
    R_GAME_ASSERT (removeIndex < currentCount);

    struct r_cstl_array* pNewArray = r_cstl_new_array_with_capacity (elementSize * (currentCount - 1));
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

        const uint8_t* pOldData = r_cstl_array_data (pArray);
        int            result = r_cstl_array_push_data (pNewArray, pOldData + i * elementSize, elementSize);
        if (result != R_CSTL_OK)
        {
            r_cstl_delete_array (pNewArray);
            return R_GAME_ERROR_ARRAY_OPERATION_FAILED;
        }
    }

    r_cstl_delete_array (pArray);
    *ppArray = pNewArray;
    return R_GAME_OK;
}

static size_t
r_game_renderer_find_resource_index_by_handle (struct r_game_renderer_subsystem* pSubsystem, uint64_t handle)
{
    size_t resourceCount
        = r_cstl_array_length (pSubsystem->pResourceArray) / sizeof (struct r_game_renderer_resource);

    for (size_t i = 0; i < resourceCount; ++i)
    {
        struct r_game_renderer_resource resource;
        r_cstl_array_typed_unchecked_at (
            pSubsystem->pResourceArray,
            struct r_game_renderer_resource,
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
r_game_renderer_remove_resource_byHandle (struct r_game_renderer_subsystem* pSubsystem, uint64_t handle)
{
    size_t index = r_game_renderer_find_resource_index_by_handle (pSubsystem, handle);
    if (index == SIZE_MAX)
    {
        return R_GAME_ERROR_RESOURCE_NOT_FOUND;
    }

    return r_game_renderer_remove_from_array_by_index (
        &pSubsystem->pResourceArray,
        sizeof (struct r_game_renderer_resource),
        index);
}

static int
r_game_renderer_wait_and_reset_fence (struct R_CVulkan_Device* pDevice, struct R_CVulkan_Fence* pFence)
{
    enum R_CVulkan_Error err = R_CVulkan_FenceWait (pDevice, pFence, 1, true, UINT64_MAX);
    if (err != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("Failed to wait for fence: %s", r_cvulkan_error_to_string (err));
        return R_GAME_ERROR_FAILED;
    }
    err = R_CVulkan_FenceReset (pDevice, pFence, 1);
    if (err != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("Failed to reset fence: %s", r_cvulkan_error_to_string (err));
        return R_GAME_ERROR_FAILED;
    }

    return R_GAME_OK;
}

static int
r_game_renderer_acquire_swapchain_image (
    struct r_game_pipeline_context* pPipelineContext,
    uint32_t*                       pImageIndex)
{
    enum R_CVulkan_Error err = r_cvulkan_swapchain_acquire_next_image (
        &pPipelineContext->swapchain,
        UINT64_MAX,
        r_cvulkan_semaphore_get_handle (&pPipelineContext->imageAvailableSemaphore),
        VK_NULL_HANDLE,
        pImageIndex);
    if (err != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("Failed to acquire next image: %s", r_cvulkan_error_to_string (err));
        return R_GAME_ERROR_FAILED;
    }
    return R_GAME_OK;
}

static int
r_game_renderer_render_layer (
    struct r_game_renderer_layer*   pLayer,
    struct R_CVulkan_CommandBuffer* pCmdBuffer)
{
    enum R_CVulkan_Error err = r_cvulkan_begin_command_buffer (pCmdBuffer, 0, NULL);
    if (err != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("Failed to begin command buffer: %s", r_cvulkan_error_to_string (err));
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
    err = r_cvulkan_end_command_buffer (pCmdBuffer);
    if (err != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("Failed to end command buffer: %s", r_cvulkan_error_to_string (err));
        return R_GAME_ERROR_COMMAND_BUFFER_FAILED;
    }
    return R_GAME_OK;
}

static int
r_game_renderer_submit_command_buffers (
    struct R_CVulkan_Queue*          pGraphicsQueue,
    struct R_CVulkan_CommandBuffer** ppCommandBuffers,
    size_t                           commandBufferCount,
    struct R_CVulkan_Semaphore*      pImageAvailableSemaphore,
    struct R_CVulkan_Semaphore*      pRenderFinishedSemaphore,
    struct R_CVulkan_Fence*          pInFlightFence)
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
        R_CSTL_LOG_ERROR ("Failed to submit command buffers: %s", r_cvulkan_error_to_string (err));
        return R_GAME_ERROR_FAILED;
    }
    return R_GAME_OK;
}

static int
r_game_renderer_present_image (
    struct R_CVulkan_Queue*     pPresentQueue,
    struct R_CVulkan_Swapchain  swapchain,
    uint32_t                    imageIndex,
    struct R_CVulkan_Semaphore* pRenderFinishedSemaphore)
{
    VkSwapchainKHR swapchainHandle = r_cvulkan_swapchain_get_handle (&swapchain);

    enum R_CVulkan_Error err = R_CVulkan_QueuePresent (
        pPresentQueue,
        &swapchainHandle,
        1,
        &imageIndex,
        pRenderFinishedSemaphore,
        1);
    if (err != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("Failed to present image: %s", r_cvulkan_error_to_string (err));
        return R_GAME_ERROR_FAILED;
    }
    return R_GAME_OK;
}

#if defined(_WIN32)
static DWORD WINAPI
r_game_renderer_worker_thread_proc (LPVOID pParam)
#elif defined(__linux__) || defined(__APPLE__)
static void*
r_game_renderer_worker_thread_proc (void* pParam)
#endif
{
    struct r_game_renderer_worker_thread* pWorker = (struct r_game_renderer_worker_thread*)pParam;
    struct r_game_renderer_subsystem*     pSubsystem = pWorker->pSubsystem;
    struct r_game_renderer_thread_pool*   pPool = pSubsystem->pThreadPool;

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
        size_t taskCount = r_cstl_stack_size (pPool->pTaskQueue) / sizeof (struct r_game_renderer_render_task);
        if (taskCount == 0)
        {
            R_GAME_MUTEX_UNLOCK (&pPool->taskMutex);
            continue;
        }

        struct r_game_renderer_render_task task;
        r_cstl_stack_pop_data (pPool->pTaskQueue, (uint8_t*)&task, sizeof (struct r_game_renderer_render_task));

        R_GAME_ATOMIC_DECREMENT (&pPool->atomicPendingTasks);
        R_GAME_MUTEX_UNLOCK (&pPool->taskMutex);

        struct r_game_renderer_frame* pFrame = &pSubsystem->pFrames[task.frameIndex];
        struct r_game_renderer_layer  layer;
        R_GAME_MUTEX_LOCK (&pSubsystem->layerArrayMutex);
        r_cstl_array_typed_at (pSubsystem->pLayerArray, struct r_game_renderer_layer, task.layerIndex, &layer);
        R_GAME_MUTEX_UNLOCK (&pSubsystem->layerArrayMutex);

        struct R_CVulkan_CommandBuffer cmdBuffer;
        r_cstl_array_typed_unchecked_at (
            pFrame->pCommandBufferArray,
            struct R_CVulkan_CommandBuffer,
            task.commandBufferIndex,
            &cmdBuffer);

        enum R_CVulkan_Error err = r_cvulkan_begin_command_buffer (&cmdBuffer, 0, NULL);
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
            r_cvulkan_end_command_buffer (&cmdBuffer);
            r_cstl_array_typed_set_at_unchecked (
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
r_game_renderer_initialize_thread_pool (struct r_game_renderer_subsystem* pSubsystem)
{
    pSubsystem->pThreadPool
        = (struct r_game_renderer_thread_pool*)r_cstl_heap_alloc (sizeof (struct r_game_renderer_thread_pool));
    if (pSubsystem->pThreadPool == NULL)
    {
        R_CSTL_LOG_ERROR ("Failed to allocate thread pool");
        return R_GAME_ERROR_OUT_OF_MEMORY;
    }
    memset (pSubsystem->pThreadPool, 0, sizeof (struct r_game_renderer_thread_pool));

    struct r_game_renderer_thread_pool* pPool = pSubsystem->pThreadPool;

    R_GAME_MUTEX_INIT (&pPool->taskMutex);
    R_GAME_COND_INIT (&pPool->taskAvailable);
    R_GAME_COND_INIT (&pPool->taskComplete);

    pPool->pTaskQueue = r_cstl_new_stack_with_capacity (
        sizeof (struct r_game_renderer_render_task) * R_GAME_RENDERER_MAX_COMMAND_BUFFERS_PER_FRAME);
    if (pPool->pTaskQueue == NULL)
    {
        R_CSTL_LOG_ERROR ("Failed to allocate task queue");
        r_cstl_heap_free (pSubsystem->pThreadPool);
        return R_GAME_ERROR_OUT_OF_MEMORY;
    }

    R_GAME_ATOMIC_STORE (&pPool->atomicPendingTasks, 0);
    R_GAME_ATOMIC_STORE (&pPool->atomicCompletedTasks, 0);
    R_GAME_ATOMIC_STORE (&pPool->atomicShutdownRequested, 0);

    uint32_t workerCount = R_GAME_RENDERER_MAX_WORKER_THREADS;
    pPool->workerCount = workerCount;

    for (uint32_t i = 0; i < workerCount; ++i)
    {
        struct r_game_renderer_worker_thread* pWorker = &pPool->workers[i];
        pWorker->workerIndex = i;
        pWorker->pSubsystem = pSubsystem;
        R_GAME_ATOMIC_STORE (&pWorker->atomicIsRunning, 1);

#if defined(_WIN32)
        pWorker->threadHandle
            = CreateThread (NULL, 0, r_game_renderer_worker_thread_proc, pWorker, 0, &pWorker->threadId);
        if (pWorker->threadHandle == NULL)
        {
            pPool->workerCount = i;
            r_game_renderer_shutdown_thread_pool (pSubsystem);
            return R_GAME_ERROR_THREAD_CREATE_FAILED;
        }
#elif defined(__linux__) || defined(__APPLE__)
        int result
            = pthread_create (&pWorker->threadHandle, NULL, r_game_renderer_worker_thread_proc, pWorker);
        if (result != 0)
        {
            pPool->workerCount = i;
            r_game_renderer_shutdown_thread_pool (pSubsystem);
            return R_GAME_ERROR_THREAD_CREATE_FAILED;
        }
        pWorker->threadId = (pthread_t)pWorker->threadHandle;
#endif
    }
    R_CSTL_LOG_INFO ("Thread pool initialized with %u workers", workerCount);
    return R_GAME_OK;
}

static void
r_game_renderer_shutdown_thread_pool (struct r_game_renderer_subsystem* pSubsystem)
{
    if (pSubsystem->pThreadPool == NULL)
    {
        return;
    }

    struct r_game_renderer_thread_pool* pPool = pSubsystem->pThreadPool;

    R_GAME_ATOMIC_STORE (&pPool->atomicShutdownRequested, 1);
    R_GAME_COND_BROADCAST (&pPool->taskAvailable);

    for (uint32_t i = 0; i < pPool->workerCount; ++i)
    {
        struct r_game_renderer_worker_thread* pWorker = &pPool->workers[i];
        int                                   isRunning = R_GAME_ATOMIC_LOAD (&pWorker->atomicIsRunning);
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
        r_cstl_delete_stack (pPool->pTaskQueue);
    }

    r_cstl_heap_free (pSubsystem->pThreadPool);
    pSubsystem->pThreadPool = NULL;

    R_CSTL_LOG_INFO ("Thread pool shutdown complete");
}

static int
r_game_renderer_initialize_bytecode_decoder (void)
{
#if defined(R_GAME_DEBUG)
    if (g_pBytecodeDecoder)
    {
        return R_GAME_OK;
    }

    enum r_cstl_bytecode_architecture arch = R_CSTL_BYTECODE_ARCH_X86_64;
#if defined(R_CVULKAN_PLATFORM_WINDOWS) || defined(R_CVULKAN_PLATFORM_LINUX)
    arch = R_CSTL_BYTECODE_ARCH_X86_64;
#elif defined(R_CVULKAN_PLATFORM_ANDROID)
    arch = R_CSTL_BYTECODE_ARCH_ARMEABI_V7A;
#elif defined(R_CVULKAN_PLATFORM_MACOS)
    arch = R_CSTL_BYTECODE_ARCH_ARMV8A;
#endif

    g_pBytecodeDecoder
        = (struct r_cstl_bytecode_decoder*)r_cstl_heap_alloc (sizeof (struct r_cstl_bytecode_decoder));
    if (g_pBytecodeDecoder == NULL)
    {
        R_CSTL_LOG_ERROR ("Failed to allocate bytecode decoder");
        return R_GAME_ERROR_OUT_OF_MEMORY;
    }

    int result = r_cstl_bytecode_decoder_create (arch, g_pBytecodeDecoder);
    if (result != 0)
    {
        R_CSTL_LOG_ERROR ("Failed to create bytecode decoder");
        r_cstl_heap_free (g_pBytecodeDecoder);
        g_pBytecodeDecoder = NULL;
        return R_GAME_ERROR_FAILED;
    }
#endif
    return R_GAME_OK;
}

static void
r_game_renderer_shutdown_bytecode_decoder (void)
{
#if defined(R_LOG)
    if (g_pBytecodeDecoder)
    {
        r_cstl_delete_bytecode_decoder (g_pBytecodeDecoder);
        r_cstl_heap_free (g_pBytecodeDecoder);
        g_pBytecodeDecoder = NULL;
    }
#endif
}

static int
r_game_renderer_validate_callback_function (r_cstl_bytecode_function pFunction, const char* pRequiredSymbol)
{
#if defined(R_GAME_DEBUG)
    if (g_pBytecodeDecoder == NULL || pFunction == NULL || pRequiredSymbol == NULL)
    {
        return 0;
    }
    int found = 0;
    int result = r_cstl_bytecode_function_contains_symbol (
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
r_game_renderer_initialize_command_buffers (
    struct r_game_renderer_frame* pFrame,
    uint32_t                      frameIdx,
    struct R_CVulkan_Device*      pDevice,
    struct R_CVulkan_CommandPool* pGraphicsPool)
{
    pFrame->pCommandBufferArray = r_cstl_new_array_with_capacity (
        sizeof (struct R_CVulkan_CommandBuffer) * R_GAME_RENDERER_MAX_COMMAND_BUFFERS_PER_FRAME);
    if (pFrame->pCommandBufferArray == NULL)
    {
        R_CSTL_LOG_ERROR ("Failed to allocate command buffer array for frame %u", frameIdx);
        return R_GAME_ERROR_OUT_OF_MEMORY;
    }

    for (uint32_t bufIdx = 0; bufIdx < R_GAME_RENDERER_MAX_COMMAND_BUFFERS_PER_FRAME; ++bufIdx)
    {
        struct R_CVulkan_CommandBuffer cmdBuffer = {0};
        enum R_CVulkan_Error           err = r_cvulkan_new_command_buffer (
            &cmdBuffer,
            pDevice,
            r_cvulkan_command_pool_get_handle (pGraphicsPool),
            VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        if (err != R_CVULKAN_OK)
        {
            R_CSTL_LOG_ERROR (
                "Failed to create command buffer %u for frame %u: %s",
                bufIdx,
                frameIdx,
                r_cvulkan_error_to_string (err));
            return R_GAME_ERROR_COMMAND_BUFFER_FAILED;
        }

        int result = r_cstl_array_push_data (
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
r_game_renderer_initialize_semaphore (
    struct r_game_renderer_frame* pFrame,
    uint32_t                      frameIdx,
    struct R_CVulkan_Device*      pDevice)
{
    pFrame->pRenderFinishedSemaphore
        = (struct R_CVulkan_Semaphore*)r_cstl_heap_alloc (sizeof (struct R_CVulkan_Semaphore));
    if (pFrame->pRenderFinishedSemaphore == NULL)
    {
        R_CSTL_LOG_ERROR ("Failed to allocate semaphore for frame %u", frameIdx);
        return R_GAME_ERROR_OUT_OF_MEMORY;
    }
    memset (pFrame->pRenderFinishedSemaphore, 0, sizeof (struct R_CVulkan_Semaphore));

    enum R_CVulkan_Error err = R_CVulkan_NewSemaphore (pFrame->pRenderFinishedSemaphore, pDevice, 0, 0);
    if (err != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR (
            "Failed to create semaphore for frame %u: %s",
            frameIdx,
            r_cvulkan_error_to_string (err));
        return R_GAME_ERROR_FAILED;
    }

    return R_GAME_OK;
}

static int
r_game_renderer_initialize_fence (
    struct r_game_renderer_frame* pFrame,
    uint32_t                      frameIdx,
    struct R_CVulkan_Device*      pDevice)
{
    pFrame->pInFlightFence = (struct R_CVulkan_Fence*)r_cstl_heap_alloc (sizeof (struct R_CVulkan_Fence));
    if (pFrame->pInFlightFence == NULL)
    {
        R_CSTL_LOG_ERROR ("Failed to allocate fence for frame %u", frameIdx);
        return R_GAME_ERROR_OUT_OF_MEMORY;
    }
    memset (pFrame->pInFlightFence, 0, sizeof (struct R_CVulkan_Fence));

    enum R_CVulkan_Error err = R_CVulkan_NewFence (pFrame->pInFlightFence, pDevice, true);
    if (err != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("Failed to create fence for frame %u: %s", frameIdx, r_cvulkan_error_to_string (err));
        return R_GAME_ERROR_FAILED;
    }

    return R_GAME_OK;
}

static int
r_game_renderer_initialize_frame (
    struct r_game_renderer_subsystem* pSubsystem,
    struct r_game_pipeline_context*   pPipelineContext)
{
    struct R_CVulkan_Device*      pDevice = r_game_pipeline_context_get_device (pPipelineContext);
    struct R_CVulkan_CommandPool* pGraphicsPool
        = r_game_pipeline_context_get_graphics_command_pool (pPipelineContext);

    for (uint32_t frameIdx = 0; frameIdx < pSubsystem->maxFramesInFlight; ++frameIdx)
    {
        struct r_game_renderer_frame* pFrame = &pSubsystem->pFrames[frameIdx];

        if (r_game_renderer_initialize_command_buffers (pFrame, frameIdx, pDevice, pGraphicsPool)
            != R_GAME_OK)
        {
            return R_GAME_ERROR_FAILED;
        }

        if (r_game_renderer_initialize_semaphore (pFrame, frameIdx, pDevice) != R_GAME_OK)
        {
            return R_GAME_ERROR_FAILED;
        }

        if (r_game_renderer_initialize_fence (pFrame, frameIdx, pDevice) != R_GAME_OK)
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
r_game_renderer_cleanup_frame (struct r_game_renderer_subsystem* pSubsystem)
{
    if (pSubsystem->pFrames == NULL)
    {
        return;
    }
    for (uint32_t i = 0; i < pSubsystem->maxFramesInFlight; ++i)
    {
        struct r_game_renderer_frame* pFrame = &pSubsystem->pFrames[i];
        if (pFrame->pCommandBufferArray)
        {
            size_t cmdBufferCount
                = r_cstl_array_length (pFrame->pCommandBufferArray) / sizeof (struct R_CVulkan_CommandBuffer);

            for (size_t j = 0; j < cmdBufferCount; ++j)
            {
                struct R_CVulkan_CommandBuffer cmdBuffer;
                r_cstl_array_typed_unchecked_at (
                    pFrame->pCommandBufferArray,
                    struct R_CVulkan_CommandBuffer,
                    j,
                    &cmdBuffer);
                r_cvulkan_delete_command_buffer (&cmdBuffer);
            }

            r_cstl_delete_array (pFrame->pCommandBufferArray);
            pFrame->pCommandBufferArray = NULL;
        }
        if (pFrame->pRenderFinishedSemaphore)
        {
            R_CVulkan_DeleteSemaphore (pFrame->pRenderFinishedSemaphore);
            r_cstl_heap_free (pFrame->pRenderFinishedSemaphore);
        }
        if (pFrame->pInFlightFence)
        {
            R_CVulkan_DeleteFence (pFrame->pInFlightFence);
            r_cstl_heap_free (pFrame->pInFlightFence);
        }
    }

    r_cstl_heap_free (pSubsystem->pFrames);
    pSubsystem->pFrames = NULL;
}

R_GAME_API struct r_game_renderer_subsystem*
r_game_renderer_new_subsystem (struct r_game_renderer_subsystem* pSubsystem)
{
#if defined(R_GAME_DEBUG)
    if (pSubsystem == NULL)
    {
        pSubsystem
            = (struct r_game_renderer_subsystem*)r_cstl_heap_alloc (sizeof (struct r_game_renderer_subsystem));
        if (pSubsystem == NULL)
        {
            R_CSTL_LOG_ERROR ("Failed to allocate renderer subsystem");
            return NULL;
        }
    }
#endif
    memset (pSubsystem, 0, sizeof (struct r_game_renderer_subsystem));

    if (r_game_renderer_initialize_bytecode_decoder () != 0)
    {
        R_CSTL_LOG_WARN ("Bytecode decoder initialization failed, validation disabled");
    }

    pSubsystem->maxFramesInFlight = R_GAME_RENDERER_MAX_FRAMES_IN_FLIGHT;

    if (r_game_renderer_initialize_arrays (pSubsystem) != 0)
    {
        r_cstl_heap_free (pSubsystem);
        return NULL;
    }

    r_game_renderer_initialize_state (pSubsystem);

    if (r_game_renderer_initialize_thread_pool (pSubsystem) != 0)
    {
        R_GAME_MUTEX_DESTROY (&pSubsystem->layerArrayMutex);
        R_GAME_MUTEX_DESTROY (&pSubsystem->resourceArrayMutex);
        r_game_renderer_cleanup_arrays (pSubsystem);
        r_cstl_heap_free (pSubsystem);
        return NULL;
    }

    return pSubsystem;
}

R_GAME_API int
r_game_renderer_delete_subsystem (struct r_game_renderer_subsystem* pSubsystem)
{
    if (pSubsystem == NULL)
    {
        return R_GAME_ERROR_NULL_POINTER;
    }
    r_game_renderer_cleanup_frame (pSubsystem);
    r_game_renderer_cleanup_layers (pSubsystem);
    r_game_renderer_cleanup_resources (pSubsystem);
    r_game_renderer_shutdown_thread_pool (pSubsystem);
    R_GAME_MUTEX_DESTROY (&pSubsystem->layerArrayMutex);
    R_GAME_MUTEX_DESTROY (&pSubsystem->resourceArrayMutex);

    r_cstl_heap_free (pSubsystem);
    r_game_renderer_shutdown_bytecode_decoder ();
    return R_GAME_OK;
}

R_GAME_API int
r_game_renderer_set_pipeline_context (
    struct r_game_renderer_subsystem* pSubsystem,
    struct r_game_pipeline_context*   pPipelineContext)
{
    if (pSubsystem == NULL || pPipelineContext == NULL)
    {
        R_CSTL_LOG_ERROR ("Invalid parameters for SetPipelineContext");
        return R_GAME_ERROR_INVALID_ARGUMENT;
    }

    pSubsystem->pPipelineContext = pPipelineContext;

    if (r_game_renderer_initialize_frame (pSubsystem, pPipelineContext) != R_GAME_OK)
    {
        R_CSTL_LOG_ERROR ("Failed to initialize frame data");
        pSubsystem->pPipelineContext = NULL;
        return R_GAME_ERROR_FRAMEBUFFER_NOT_READY;
    }

    R_CSTL_LOG_INFO ("Pipeline context set successfully");
    return R_GAME_OK;
}

R_GAME_API int
r_game_renderer_subsystem_start (struct r_game_renderer_subsystem* pSubsystem)
{
    R_CSTL_TRACE_FUNCTION ();
    R_GAME_ASSERT (pSubsystem);

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
r_game_renderer_subsystem_stop (struct r_game_renderer_subsystem* pSubsystem)
{
    R_CSTL_TRACE_FUNCTION ();
    R_GAME_ASSERT (pSubsystem);

    r_game_renderer_wait_for_frame (pSubsystem);

    pSubsystem->state = R_GAME_RENDERER_STATE_STOPPED;
    R_CSTL_TRACE_POINT ("subsystem_stopped");
    R_CSTL_TRACE_RETURN ();
    return R_GAME_OK;
}

R_GAME_API int
r_game_renderer_subsystem_pause (struct r_game_renderer_subsystem* pSubsystem)
{
    R_CSTL_TRACE_FUNCTION ();
    R_GAME_ASSERT (pSubsystem);

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
r_game_renderer_subsystem_resume (struct r_game_renderer_subsystem* pSubsystem)
{
    R_CSTL_TRACE_FUNCTION ();
    R_GAME_ASSERT (pSubsystem);

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
r_game_renderer_begin_frame (struct r_game_renderer_subsystem* pSubsystem)
{
    R_GAME_ASSERT (pSubsystem);
    R_GAME_ASSERT (pSubsystem->pPipelineContext);

    if (pSubsystem->state != R_GAME_RENDERER_STATE_RUNNING)
    {
        return R_GAME_ERROR_INVALID_STATE;
    }

    struct R_CVulkan_Device* pDevice = r_game_pipeline_context_get_device (pSubsystem->pPipelineContext);
    struct R_CVulkan_Fence*  pFence
        = r_game_pipeline_context_get_in_flight_fence (pSubsystem->pPipelineContext);
    uint32_t* pImageIndex = r_game_pipeline_context_get_current_frame_index (pSubsystem->pPipelineContext);

    if (r_game_renderer_wait_for_fence (pDevice, pFence, UINT64_MAX) != R_GAME_OK)
    {
        return R_GAME_ERROR_FAILED;
    }

    if (r_game_renderer_acquire_next_image (pSubsystem->pPipelineContext, pImageIndex) != R_GAME_OK)
    {
        return R_GAME_ERROR_FAILED;
    }

    pSubsystem->currentFrameIndex = *pImageIndex;
    return R_GAME_OK;
}

R_GAME_API int
r_game_renderer_render_frame (struct r_game_renderer_subsystem* pSubsystem)
{
    R_GAME_ASSERT (pSubsystem);
    R_GAME_ASSERT (pSubsystem->pPipelineContext);

    if (pSubsystem->state != R_GAME_RENDERER_STATE_RUNNING)
    {
        return R_GAME_ERROR_INVALID_STATE;
    }

    struct r_game_renderer_frame* pCurrentFrame = &pSubsystem->pFrames[pSubsystem->currentFrameIndex];

    size_t cmdBufferCount
        = r_cstl_array_length (pCurrentFrame->pCommandBufferArray) / sizeof (struct R_CVulkan_CommandBuffer);

    size_t layerCount = r_cstl_array_length (pSubsystem->pLayerArray) / sizeof (struct r_game_renderer_layer);

    size_t currentBufferIndex = 0;

    R_GAME_MUTEX_LOCK (&pSubsystem->pThreadPool->taskMutex);
    r_cstl_delete_stack (pSubsystem->pThreadPool->pTaskQueue);
    pSubsystem->pThreadPool->pTaskQueue = r_cstl_new_stack_with_capacity (
        sizeof (struct r_game_renderer_render_task) * R_GAME_RENDERER_MAX_COMMAND_BUFFERS_PER_FRAME);
    R_GAME_ATOMIC_STORE (&pSubsystem->pThreadPool->atomicPendingTasks, 0);
    R_GAME_ATOMIC_STORE (&pSubsystem->pThreadPool->atomicCompletedTasks, 0);
    R_GAME_MUTEX_UNLOCK (&pSubsystem->pThreadPool->taskMutex);

    for (size_t layerIdx = 0; layerIdx < layerCount; ++layerIdx)
    {
        struct r_game_renderer_layer layer;
        r_cstl_array_typed_unchecked_at (
            pSubsystem->pLayerArray,
            struct r_game_renderer_layer,
            layerIdx,
            &layer);

        if (!(layer.flags & R_GAME_RENDERER_LAYER_FLAG_ENABLED))
        {
            continue;
        }

        if (currentBufferIndex >= cmdBufferCount)
        {
            R_CSTL_LOG_WARN ("Exceeded maximum command buffers per frame");
            break;
        }

        struct r_game_renderer_render_task task = {0};
        task.layerIndex = (uint32_t)layerIdx;
        task.commandBufferIndex = (uint32_t)currentBufferIndex;
        task.frameIndex = pSubsystem->currentFrameIndex;
        task.completed = 0;
        R_GAME_ATOMIC_STORE (&task.atomicCompleted, 0);

        R_GAME_MUTEX_LOCK (&pSubsystem->pThreadPool->taskMutex);
        int result = r_cstl_stack_push_data (
            pSubsystem->pThreadPool->pTaskQueue,
            (const uint8_t*)&task,
            sizeof (struct r_game_renderer_render_task));
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
r_game_renderer_end_frame (struct r_game_renderer_subsystem* pSubsystem)
{
    R_GAME_ASSERT (pSubsystem);
    R_GAME_ASSERT (pSubsystem->pPipelineContext);

    if (pSubsystem->state != R_GAME_RENDERER_STATE_RUNNING)
    {
        return R_GAME_ERROR_INVALID_STATE;
    }

    struct R_CVulkan_Device* pDevice = r_game_pipeline_context_get_device (pSubsystem->pPipelineContext);
    struct R_CVulkan_Queue*  pQueue
        = r_game_pipeline_context_get_graphics_queue (pSubsystem->pPipelineContext);
    struct R_CVulkan_Semaphore* pSignalSemaphore
        = r_game_pipeline_context_get_image_available_semaphore (pSubsystem->pPipelineContext);
    struct R_CVulkan_Semaphore* pWaitSemaphore
        = r_game_pipeline_context_get_render_finished_semaphore (pSubsystem->pPipelineContext);
    struct R_CVulkan_Fence* pFence
        = r_game_pipeline_context_get_in_flight_fence (pSubsystem->pPipelineContext);
    uint32_t imageIndex = pSubsystem->currentFrameIndex;

    struct r_game_renderer_frame* pFrame = &pSubsystem->pFrames[imageIndex];
    size_t                        cmdBufferCount
        = r_cstl_array_length (pFrame->pCommandBufferArray) / sizeof (struct R_CVulkan_CommandBuffer);

    if (cmdBufferCount > 0)
    {
        struct R_CVulkan_CommandBuffer* pCmdBuffers
            = (struct R_CVulkan_CommandBuffer*)r_cstl_array_data (pFrame->pCommandBufferArray);
        if (r_game_renderer_submit_command_buffers (
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
r_game_renderer_wait_for_frame (struct r_game_renderer_subsystem* pSubsystem)
{
    R_GAME_ASSERT (pSubsystem);
    R_GAME_ASSERT (pSubsystem->pPipelineContext);

    struct r_game_pipeline_context* pPipelineContext = pSubsystem->pPipelineContext;
    struct R_CVulkan_Queue*         pQueue = r_game_pipeline_context_get_present_queue (pPipelineContext);
    struct R_CVulkan_Semaphore*     pSignalSemaphore
        = r_game_pipeline_context_get_render_finished_semaphore (pPipelineContext);
    uint32_t imageIndex = pSubsystem->currentFrameIndex;

    struct R_CVulkan_Swapchain swapchain = pPipelineContext->swapchain;
    if (r_game_renderer_present_image (pQueue, swapchain, imageIndex, pSignalSemaphore) != R_GAME_OK)
    {
        return R_GAME_ERROR_FAILED;
    }

    pSubsystem->currentFrameIndex = (pSubsystem->currentFrameIndex + 1) % pSubsystem->maxFramesInFlight;
    return R_GAME_OK;
}

R_GAME_API int
r_game_renderer_add_layer (
    struct r_game_renderer_subsystem* pSubsystem,
    const char*                       pName,
    uint32_t                          priority,
    uint32_t                          flags,
    void*                             pUserData)
{
    R_GAME_ASSERT (pSubsystem);
    R_GAME_ASSERT (pName);

    size_t currentLayerCount
        = r_cstl_array_length (pSubsystem->pLayerArray) / sizeof (struct r_game_renderer_layer);
    if (currentLayerCount >= R_GAME_RENDERER_MAX_LAYERS)
    {
        return R_GAME_ERROR_MAX_RESOURCES_REACHED;
    }
    struct r_game_renderer_layer newLayer = {0};
    newLayer.priority = priority;
    newLayer.flags = flags;
    newLayer.pUserData = (void*)pUserData;

    if (pName)
    {
        size_t nameLen = strlen (pName);
        newLayer.pName = (char*)r_cstl_heap_alloc (nameLen + 1);
        if (newLayer.pName == NULL)
        {
            R_CSTL_LOG_ERROR ("Failed to allocate layer name");
            return R_GAME_ERROR_OUT_OF_MEMORY;
        }
        memcpy (newLayer.pName, pName, nameLen + 1);
    }

    R_GAME_MUTEX_LOCK (&pSubsystem->layerArrayMutex);
    int result = r_cstl_array_push_data (
        pSubsystem->pLayerArray,
        (const uint8_t*)&newLayer,
        sizeof (struct r_game_renderer_layer));
    R_GAME_MUTEX_UNLOCK (&pSubsystem->layerArrayMutex);

    if (result != R_CSTL_OK)
    {
        if (newLayer.pName)
        {
            r_cstl_heap_free (newLayer.pName);
        }
        return R_GAME_ERROR_ARRAY_OPERATION_FAILED;
    }

    return R_GAME_OK;
}

R_GAME_API int
r_game_renderer_remove_layer (struct r_game_renderer_subsystem* pSubsystem, uint32_t layerIndex)
{
    R_GAME_ASSERT (pSubsystem);
    size_t layerCount = r_cstl_array_length (pSubsystem->pLayerArray) / sizeof (struct r_game_renderer_layer);
    R_GAME_ASSERT (layerIndex < layerCount);

    R_GAME_MUTEX_LOCK (&pSubsystem->layerArrayMutex);
    struct r_game_renderer_layer layer;
    r_cstl_array_typed_at (pSubsystem->pLayerArray, struct r_game_renderer_layer, layerIndex, &layer);
    if (layer.pName)
    {
        r_cstl_heap_free (layer.pName);
    }
    r_game_renderer_remove_from_array_by_index (
        &pSubsystem->pLayerArray,
        sizeof (struct r_game_renderer_layer),
        layerIndex);
    R_GAME_MUTEX_UNLOCK (&pSubsystem->layerArrayMutex);

    return R_GAME_OK;
}

R_GAME_API int
r_game_renderer_set_layer_enabled (
    struct r_game_renderer_subsystem* pSubsystem,
    uint32_t                          layerIndex,
    int                               enabled)
{
    R_GAME_ASSERT (pSubsystem);
    size_t layerCount = r_cstl_array_length (pSubsystem->pLayerArray) / sizeof (struct r_game_renderer_layer);
    R_GAME_ASSERT (layerIndex < layerCount);

    R_GAME_MUTEX_LOCK (&pSubsystem->layerArrayMutex);
    struct r_game_renderer_layer layer;
    r_cstl_array_typed_at (pSubsystem->pLayerArray, struct r_game_renderer_layer, layerIndex, &layer);
    if (enabled) layer.flags |= R_GAME_RENDERER_LAYER_FLAG_ENABLED;
    else layer.flags &= ~R_GAME_RENDERER_LAYER_FLAG_ENABLED;
    r_cstl_array_typed_set_at (pSubsystem->pLayerArray, struct r_game_renderer_layer, layerIndex, &layer);
    R_GAME_MUTEX_UNLOCK (&pSubsystem->layerArrayMutex);

    return R_GAME_OK;
}

R_GAME_API int
r_game_renderer_set_layer_renderCallback (
    struct r_game_renderer_subsystem* pSubsystem,
    uint32_t                          layerIndex,
    r_game_lifecycle_render           callback)
{
    R_GAME_ASSERT (pSubsystem);
    size_t layerCount = r_cstl_array_length (pSubsystem->pLayerArray) / sizeof (struct r_game_renderer_layer);
    R_GAME_ASSERT (layerIndex < layerCount);

    R_GAME_MUTEX_LOCK (&pSubsystem->layerArrayMutex);
    struct r_game_renderer_layer layer;
    r_cstl_array_typed_at (pSubsystem->pLayerArray, struct r_game_renderer_layer, layerIndex, &layer);
    layer.renderCallback = callback;
    r_cstl_array_typed_set_at (pSubsystem->pLayerArray, struct r_game_renderer_layer, layerIndex, &layer);
    R_GAME_MUTEX_UNLOCK (&pSubsystem->layerArrayMutex);

    return R_GAME_OK;
}

R_GAME_API int
r_game_renderer_set_layer_before_pass_callback (
    struct r_game_renderer_subsystem* pSubsystem,
    uint32_t                          layerIndex,
    r_game_lifecycle_before_pass      callback)
{
    R_GAME_ASSERT (pSubsystem);
    size_t layerCount = r_cstl_array_length (pSubsystem->pLayerArray) / sizeof (struct r_game_renderer_layer);
    R_GAME_ASSERT (layerIndex < layerCount);

    R_GAME_MUTEX_LOCK (&pSubsystem->layerArrayMutex);
    struct r_game_renderer_layer layer;
    r_cstl_array_typed_at (pSubsystem->pLayerArray, struct r_game_renderer_layer, layerIndex, &layer);
    layer.beforePassCallback = callback;
    r_cstl_array_typed_set_at (pSubsystem->pLayerArray, struct r_game_renderer_layer, layerIndex, &layer);
    R_GAME_MUTEX_UNLOCK (&pSubsystem->layerArrayMutex);

    return R_GAME_OK;
}

R_GAME_API int
r_game_renderer_set_layer_after_pass_callback (
    struct r_game_renderer_subsystem* pSubsystem,
    uint32_t                          layerIndex,
    r_game_lifecycle_after_pass       callback)
{
    R_GAME_ASSERT (pSubsystem);
    size_t layerCount = r_cstl_array_length (pSubsystem->pLayerArray) / sizeof (struct r_game_renderer_layer);
    R_GAME_ASSERT (layerIndex < layerCount);

    R_GAME_MUTEX_LOCK (&pSubsystem->layerArrayMutex);
    struct r_game_renderer_layer layer;
    r_cstl_array_typed_at (pSubsystem->pLayerArray, struct r_game_renderer_layer, layerIndex, &layer);
    layer.afterPassCallback = callback;
    r_cstl_array_typed_set_at (pSubsystem->pLayerArray, struct r_game_renderer_layer, layerIndex, &layer);
    R_GAME_MUTEX_UNLOCK (&pSubsystem->layerArrayMutex);

    return R_GAME_OK;
}

R_GAME_API struct r_game_renderer_layer*
r_game_renderer_get_layer (struct r_game_renderer_subsystem* pSubsystem, uint32_t layerIndex)
{
    R_GAME_ASSERT (pSubsystem);
    size_t layerCount = r_cstl_array_length (pSubsystem->pLayerArray) / sizeof (struct r_game_renderer_layer);
    R_GAME_ASSERT (layerIndex < layerCount);
    R_GAME_MUTEX_LOCK (&pSubsystem->layerArrayMutex);
    const uint8_t*                      pLayerData = r_cstl_array_data (pSubsystem->pLayerArray);
    static struct r_game_renderer_layer s_layer;
    memcpy (
        &s_layer,
        pLayerData + layerIndex * sizeof (struct r_game_renderer_layer),
        sizeof (struct r_game_renderer_layer));
    R_GAME_MUTEX_UNLOCK (&pSubsystem->layerArrayMutex);
    return &s_layer;
}

R_GAME_API int
r_game_renderer_sort_layers (struct r_game_renderer_subsystem* pSubsystem)
{
    R_GAME_ASSERT (pSubsystem);
    R_GAME_MUTEX_LOCK (&pSubsystem->layerArrayMutex);
    int result = r_cstl_array_sort (
        pSubsystem->pLayerArray,
        sizeof (struct r_game_renderer_layer),
        r_game_renderer_compare_layers,
        NULL);
    R_GAME_MUTEX_UNLOCK (&pSubsystem->layerArrayMutex);
    return result;
}

R_GAME_API uint64_t
r_game_renderer_register_resource (
    struct r_game_renderer_subsystem* pSubsystem,
    uint32_t                          type,
    void*                             pResource,
    uint64_t                          size,
    const char*                       pName)
{
    R_GAME_ASSERT (pResource);

    size_t currentResourceCount
        = r_cstl_array_length (pSubsystem->pResourceArray) / sizeof (struct r_game_renderer_resource);
    if (currentResourceCount >= R_GAME_RENDERER_MAX_RESOURCES)
    {
        R_CSTL_LOG_ERROR ("Maximum number of resources reached");
        return 0;
    }
    struct r_game_renderer_resource newResource = {0};
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
        const size_t nameLen = strlen (pName);
        newResource.pName = (char*)r_cstl_heap_alloc (nameLen + 1);
        if (newResource.pName == NULL)
        {
            return 0;
        }
        memcpy (newResource.pName, pName, nameLen + 1);
    }
    R_GAME_MUTEX_LOCK (&pSubsystem->resourceArrayMutex);
    int result = r_cstl_array_push_data (
        pSubsystem->pResourceArray,
        (const uint8_t*)&newResource,
        sizeof (struct r_game_renderer_resource));
    R_GAME_MUTEX_UNLOCK (&pSubsystem->resourceArrayMutex);

    if (result != R_CSTL_OK)
    {
        if (newResource.pName)
        {
            r_cstl_heap_free ((void*)newResource.pName);
        }
        return 0;
    }
    return newResource.handle;
}

R_GAME_API int
r_game_renderer_unregister_resource (struct r_game_renderer_subsystem* pSubsystem, uint64_t handle)
{
    R_GAME_ASSERT (pSubsystem);
    R_GAME_ASSERT (handle != 0);
    size_t foundIndex = r_game_renderer_find_resource_index_by_handle (pSubsystem, handle);
    if (foundIndex == SIZE_MAX)
    {
        return R_GAME_ERROR_RESOURCE_NOT_FOUND;
    }

    R_GAME_MUTEX_LOCK (&pSubsystem->resourceArrayMutex);
    struct r_game_renderer_resource resource;
    r_cstl_array_typed_at (pSubsystem->pResourceArray, struct r_game_renderer_resource, foundIndex, &resource);
    if (resource.pName)
    {
        r_cstl_heap_free ((void*)resource.pName);
    }
    r_game_renderer_remove_from_array_by_index (
        &pSubsystem->pResourceArray,
        sizeof (struct r_game_renderer_resource),
        foundIndex);
    R_GAME_MUTEX_UNLOCK (&pSubsystem->resourceArrayMutex);

    return R_GAME_OK;
}

R_GAME_API const void*
r_game_renderer_get_resource (struct r_game_renderer_subsystem* pSubsystem, uint64_t handle)
{
    R_GAME_ASSERT (pSubsystem);
    R_GAME_ASSERT (handle != 0);
    size_t index = r_game_renderer_find_resource_index_by_handle (pSubsystem, handle);
    if (index == SIZE_MAX)
    {
        return NULL;
    }

    R_GAME_MUTEX_LOCK (&pSubsystem->resourceArrayMutex);
    struct r_game_renderer_resource resource;
    r_cstl_array_typed_at (pSubsystem->pResourceArray, struct r_game_renderer_resource, index, &resource);
    R_GAME_MUTEX_UNLOCK (&pSubsystem->resourceArrayMutex);

    return (void*)resource.pResource;
}

R_GAME_API uint32_t
r_game_renderer_get_resource_type (struct r_game_renderer_subsystem* pSubsystem, uint64_t handle)
{
    R_GAME_ASSERT (pSubsystem);
    R_GAME_ASSERT (handle != 0);

    size_t index = r_game_renderer_find_resource_index_by_handle (pSubsystem, handle);
    if (index == SIZE_MAX)
    {
        return 0;
    }

    R_GAME_MUTEX_LOCK (&pSubsystem->resourceArrayMutex);
    struct r_game_renderer_resource resource;
    r_cstl_array_typed_at (pSubsystem->pResourceArray, struct r_game_renderer_resource, index, &resource);
    R_GAME_MUTEX_UNLOCK (&pSubsystem->resourceArrayMutex);

    return resource.type;
}

R_GAME_API uint64_t
r_game_renderer_get_resource_size (struct r_game_renderer_subsystem* pSubsystem, uint64_t handle)
{
    R_GAME_ASSERT (pSubsystem);
    R_GAME_ASSERT (handle != 0);

    size_t index = r_game_renderer_find_resource_index_by_handle (pSubsystem, handle);
    if (index == SIZE_MAX)
    {
        return 0;
    }

    R_GAME_MUTEX_LOCK (&pSubsystem->resourceArrayMutex);
    struct r_game_renderer_resource resource;
    r_cstl_array_typed_at (pSubsystem->pResourceArray, struct r_game_renderer_resource, index, &resource);
    R_GAME_MUTEX_UNLOCK (&pSubsystem->resourceArrayMutex);

    return resource.size;
}

R_GAME_API int
r_game_renderer_set_frame_resource (
    struct r_game_renderer_subsystem* pSubsystem,
    uint32_t                          frameIndex,
    uint32_t                          bufferIndex)
{
    R_GAME_ASSERT (pSubsystem);
    R_GAME_ASSERT (frameIndex < pSubsystem->maxFramesInFlight);

    struct r_game_renderer_frame* pFrame = &pSubsystem->pFrames[frameIndex];
    pFrame->resourceIndex = bufferIndex;
    return R_GAME_OK;
}

R_GAME_API struct r_game_renderer_manager*
r_game_renderer_new_manager (struct r_game_pipeline_context* pPipelineContext)
{
    R_CSTL_TRACE_FUNCTION ();
    R_GAME_ASSERT (pPipelineContext);

    struct r_game_renderer_manager* pManager
        = (struct r_game_renderer_manager*)r_cstl_heap_alloc (sizeof (struct r_game_renderer_manager));
    if (pManager == NULL)
    {
        R_CSTL_TRACE_RETURN ();
        return NULL;
    }

    memset (pManager, 0, sizeof (struct r_game_renderer_manager));
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
r_game_renderer_delete_manager (struct r_game_renderer_manager* pManager)
{
    R_CSTL_TRACE_FUNCTION ();
    R_GAME_ASSERT (pManager);
    R_GAME_ASSERT (pManager);

    R_GAME_MUTEX_LOCK (&pManager->managerMutex);

    for (uint32_t i = 0; i < pManager->subsystemCount; ++i)
    {
        struct r_game_renderer_subsystem_entry* pEntry = &pManager->subsystems[i];
        if (pEntry->pSubsystem)
        {
            r_game_renderer_delete_subsystem (pEntry->pSubsystem);
        }
    }

    R_GAME_MUTEX_UNLOCK (&pManager->managerMutex);
    R_GAME_MUTEX_DESTROY (&pManager->managerMutex);

    r_cstl_heap_free (pManager);
    R_CSTL_TRACE_POINT ("manager_deleted");
    R_CSTL_TRACE_RETURN ();
    return R_GAME_OK;
}

R_GAME_API int
r_game_renderer_add_subsystem (
    struct r_game_renderer_manager*   pManager,
    struct r_game_renderer_subsystem* pSubsystem,
    uint32_t                          priority,
    uint32_t                          flags,
    float                             blendFactor)
{
    R_CSTL_TRACE_FUNCTION_CTX ("priority=%u, blend=%f", priority, blendFactor);
    R_GAME_ASSERT (pManager);
    R_GAME_ASSERT (pSubsystem);
    R_GAME_ASSERT (pManager);

    R_GAME_MUTEX_LOCK (&pManager->managerMutex);

    if (pManager->subsystemCount >= R_GAME_RENDERER_MAX_SUBSYSTEMS)
    {
        R_CSTL_LOG_ERROR ("Maximum number of subsystems reached");
        R_GAME_MUTEX_UNLOCK (&pManager->managerMutex);
        R_CSTL_TRACE_RETURN ();
        return R_GAME_ERROR_MAX_RESOURCES_REACHED;
    }

    struct r_game_renderer_subsystem_entry* pEntry = &pManager->subsystems[pManager->subsystemCount];
    pEntry->pSubsystem = pSubsystem;
    pEntry->priority = priority;
    pEntry->flags = flags;
    pEntry->isVisible = 1;
    pEntry->blendFactor = blendFactor;
    memset (&pEntry->renderTarget, 0, sizeof (struct r_game_renderer_render_target));

    pManager->subsystemCount++;
    R_GAME_MUTEX_UNLOCK (&pManager->managerMutex);
    R_CSTL_TRACE_RETURN ();
    return R_GAME_OK;
}

R_GAME_API int
r_game_renderer_remove_subsystem (
    struct r_game_renderer_manager*   pManager,
    struct r_game_renderer_subsystem* pSubsystem)
{
    R_CSTL_TRACE_FUNCTION ();
    R_GAME_ASSERT (pManager);
    R_GAME_ASSERT (pSubsystem);
    R_GAME_ASSERT (pManager);

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
r_game_renderer_set_subsystem_visible (
    struct r_game_renderer_manager*   pManager,
    struct r_game_renderer_subsystem* pSubsystem,
    int                               visible)
{
    R_CSTL_TRACE_FUNCTION_CTX ("visible=%d", visible);
    R_GAME_ASSERT (pManager);
    R_GAME_ASSERT (pSubsystem);
    R_GAME_ASSERT (pManager);

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
r_game_renderer_set_subsystem_blendFactor (
    struct r_game_renderer_manager*   pManager,
    struct r_game_renderer_subsystem* pSubsystem,
    float                             blendFactor)
{
    R_CSTL_TRACE_FUNCTION_CTX ("blendFactor=%f", blendFactor);
    R_GAME_ASSERT (pManager);
    R_GAME_ASSERT (pSubsystem);
    R_GAME_ASSERT (pManager);

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
r_game_renderer_compose_frame (struct r_game_renderer_manager* pManager)
{
    R_GAME_ASSERT (pManager);
    R_GAME_ASSERT (pManager);

    R_GAME_MUTEX_LOCK (&pManager->managerMutex);

    for (uint32_t i = 0; i < pManager->subsystemCount; ++i)
    {
        struct r_game_renderer_subsystem_entry* pEntry = &pManager->subsystems[i];
        if (pEntry->pSubsystem && pEntry->isVisible)
        {
            r_game_renderer_begin_frame (pEntry->pSubsystem);
            r_game_renderer_render_frame (pEntry->pSubsystem);
            r_game_renderer_end_frame (pEntry->pSubsystem);
        }
    }
    R_GAME_MUTEX_UNLOCK (&pManager->managerMutex);
    return R_GAME_OK;
}

R_GAME_API int
r_game_renderer_present_frame (struct r_game_renderer_manager* pManager)
{
    R_GAME_ASSERT (pManager);
    R_GAME_ASSERT (pManager);

    R_GAME_MUTEX_LOCK (&pManager->managerMutex);

    for (uint32_t i = 0; i < pManager->subsystemCount; ++i)
    {
        struct r_game_renderer_subsystem_entry* pEntry = &pManager->subsystems[i];
        if (pEntry->pSubsystem && pEntry->isVisible)
        {
            r_game_renderer_wait_for_frame (pEntry->pSubsystem);
        }
    }
    R_GAME_MUTEX_UNLOCK (&pManager->managerMutex);
    return R_GAME_OK;
}
