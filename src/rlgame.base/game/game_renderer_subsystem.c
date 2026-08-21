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
        
        uint32_t                              state;
        R_GAME_DEBUG_FIELD
};

static struct R_GameRendererLifecycle g_lifecycle = {0};
static struct R_CSTL_BytecodeDecoder* g_pBytecodeDecoder = NULL;

static int
R_GameRenderer_CompareLayers (const void* pA, const void* pB)
{
        const struct R_GameRendererLayer* pLayerA = (const struct R_GameRendererLayer*)pA;
        const struct R_GameRendererLayer* pLayerB = (const struct R_GameRendererLayer*)pB;
        return (int)pLayerA->priority - (int)pLayerB->priority;
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
                R_CSTL_BytecodeDecoderDestroy (g_pBytecodeDecoder);
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
                R_CSTL_LOG_WARNING (
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
R_GameRenderer_InitializeFrameData (
    struct R_GameRendererSubsystem*       pSubsystem,
    struct R_GameCVulkan_PipelineContext*  pPipelineContext)
{
        struct R_CVulkan_Device* pDevice = R_GameCVulkan_PipelineContextGetDevice (pPipelineContext);
        struct R_CVulkan_CommandPool* pGraphicsPool =
            R_GameCVulkan_PipelineContextGetGraphicsCommandPool (pPipelineContext);

        for (uint32_t frameIdx = 0; frameIdx < pSubsystem->maxFramesInFlight; ++frameIdx)
        {
                struct R_GameRendererFrame* pFrame = &pSubsystem->pFrames[frameIdx];

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
                            R_CVulkan_CommandPoolGetPool (pGraphicsPool),
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

                pFrame->pInFlightFence =
                    (struct R_CVulkan_Fence*)R_CSTL_HeapAlloc (sizeof (struct R_CVulkan_Fence));
                if (pFrame->pInFlightFence == NULL)
                {
                        R_CSTL_LOG_ERROR ("Failed to allocate fence for frame %u", frameIdx);
                        return -1;
                }
                memset (pFrame->pInFlightFence, 0, sizeof (struct R_CVulkan_Fence));

                err = R_CVulkan_NewFence (pFrame->pInFlightFence, pDevice, 1);
                if (err != R_CVULKAN_OK)
                {
                        R_CSTL_LOG_ERROR (
                            "Failed to create fence for frame %u: %s",
                            frameIdx,
                            R_CVulkan_ErrorToString (err));
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
R_GameRenderer_CleanupFrameData (struct R_GameRendererSubsystem* pSubsystem)
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
                        const uint8_t* pData = R_CSTL_ArrayData (pFrame->pCommandBufferArray);
                        
                        for (size_t j = 0; j < cmdBufferCount; ++j)
                        {
                                struct R_CVulkan_CommandBuffer* pCmdBuffer = 
                                    (struct R_CVulkan_CommandBuffer*)(pData + j * sizeof (struct R_CVulkan_CommandBuffer));
                                R_CVulkan_DeleteCommandBuffer (pCmdBuffer);
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
                R_CSTL_LOG_WARNING ("Bytecode decoder initialization failed, validation disabled");
        }

        pSubsystem->maxFramesInFlight = R_GAME_RENDERER_MAX_FRAMES_IN_FLIGHT;
        pSubsystem->pFrames = (struct R_GameRendererFrame*)R_CSTL_HeapAlloc (
            sizeof (struct R_GameRendererFrame) * pSubsystem->maxFramesInFlight);
        if (pSubsystem->pFrames == NULL)
        {
                R_CSTL_LOG_ERROR ("Failed to allocate frame data");
                R_CSTL_HeapFree (pSubsystem);
                return NULL;
        }
        memset (pSubsystem->pFrames, 0,
                sizeof (struct R_GameRendererFrame) * pSubsystem->maxFramesInFlight);

        pSubsystem->pLayerArray = R_CSTL_NewArrayWithCapacity (
            sizeof (struct R_GameRendererLayer) * R_GAME_RENDERER_MAX_LAYERS);
        if (pSubsystem->pLayerArray == NULL)
        {
                R_CSTL_LOG_ERROR ("Failed to allocate render layer array");
                R_CSTL_HeapFree (pSubsystem->pFrames);
                R_CSTL_HeapFree (pSubsystem);
                return NULL;
        }

        pSubsystem->pResourceArray = R_CSTL_NewArrayWithCapacity (
            sizeof (struct R_GameRendererResource) * R_GAME_RENDERER_MAX_RESOURCES);
        if (pSubsystem->pResourceArray == NULL)
        {
                R_CSTL_LOG_ERROR ("Failed to allocate resource array");
                R_CSTL_DeleteArray (pSubsystem->pLayerArray);
                R_CSTL_HeapFree (pSubsystem->pFrames);
                R_CSTL_HeapFree (pSubsystem);
                return NULL;
        }

        pSubsystem->currentFrameIndex = 0;
        pSubsystem->frameCounter = 0;
        pSubsystem->state = R_GAME_RENDERER_STATE_STOPPED;
        pSubsystem->nextResourceHandle = 1;

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
                R_CSTL_LOG_WARNING ("Deleting uninitialized renderer subsystem");
                return -1;
        }
#endif

        R_GameRenderer_CleanupFrameData (pSubsystem);

        if (pSubsystem->pLayerArray != NULL)
        {
                size_t layerCount = R_CSTL_ArrayLength (pSubsystem->pLayerArray) / 
                    sizeof (struct R_GameRendererLayer);
                const uint8_t* pData = R_CSTL_ArrayData (pSubsystem->pLayerArray);
                
                for (size_t i = 0; i < layerCount; ++i)
                {
                        struct R_GameRendererLayer* pLayer = 
                            (struct R_GameRendererLayer*)(pData + i * sizeof (struct R_GameRendererLayer));
                        if (pLayer->pName != NULL)
                        {
                                R_CSTL_HeapFree (pLayer->pName);
                        }
                }
                R_CSTL_DeleteArray (pSubsystem->pLayerArray);
        }

        if (pSubsystem->pResourceArray != NULL)
        {
                size_t resourceCount = R_CSTL_ArrayLength (pSubsystem->pResourceArray) / 
                    sizeof (struct R_GameRendererResource);
                const uint8_t* pData = R_CSTL_ArrayData (pSubsystem->pResourceArray);
                
                for (size_t i = 0; i < resourceCount; ++i)
                {
                        struct R_GameRendererResource* pResource = 
                            (struct R_GameRendererResource*)(pData + i * sizeof (struct R_GameRendererResource));
                        if (pResource->pName != NULL)
                        {
                                R_CSTL_HeapFree ((void*)pResource->pName);
                        }
                }
                R_CSTL_DeleteArray (pSubsystem->pResourceArray);
        }

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
    struct R_GameCVulkan_PipelineContext*  pPipelineContext)
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

        if (R_GameRenderer_InitializeFrameData (pSubsystem, pPipelineContext) != 0)
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
#if defined(R_GAME_DEBUG)
        if (pSubsystem == NULL)
        {
                R_CSTL_LOG_ERROR ("Invalid subsystem for Start");
                return -1;
        }

        if (!pSubsystem->isInitialized)
        {
                R_CSTL_LOG_ERROR ("Renderer subsystem not initialized");
                return -1;
        }
#endif

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
        if (pSubsystem == NULL)
        {
                R_CSTL_LOG_ERROR ("Invalid subsystem for Stop");
                return -1;
        }

#if defined(R_GAME_DEBUG)
        if (!pSubsystem->isInitialized)
        {
                R_CSTL_LOG_ERROR ("Renderer subsystem not initialized");
                return -1;
        }
#endif

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
        if (pSubsystem == NULL)
        {
                R_CSTL_LOG_ERROR ("Invalid subsystem for Pause");
                return -1;
        }

#if defined(R_GAME_DEBUG)
        if (!pSubsystem->isInitialized)
        {
                R_CSTL_LOG_ERROR ("Renderer subsystem not initialized");
                return -1;
        }
#endif

        if (pSubsystem->state != R_GAME_RENDERER_STATE_RUNNING)
        {
                R_CSTL_LOG_WARNING ("Cannot pause renderer: not running");
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
#if defined(R_GAME_DEBUG)
        if (pSubsystem == NULL)
        {
                R_CSTL_LOG_ERROR ("Invalid subsystem for Resume");
                return -1;
        }

        if (!pSubsystem->isInitialized)
        {
                R_CSTL_LOG_ERROR ("Renderer subsystem not initialized");
                return -1;
        }
#endif

        if (pSubsystem->state != R_GAME_RENDERER_STATE_PAUSED)
        {
                R_CSTL_LOG_WARNING ("Cannot resume renderer: not paused");
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
#if defined(R_GAME_DEBUG)
        if (pSubsystem == NULL || pSubsystem->pPipelineContext == NULL)
        {
                R_CSTL_LOG_ERROR ("Invalid parameters for BeginFrame");
                return -1;
        }
#endif
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
                
                enum R_CVulkan_Error err = R_CVulkan_FenceWait (
                    pDevice,
                    pCurrentFrame->pInFlightFence,
                    1,
                    UINT64_MAX);
                if (err != R_CVULKAN_OK)
                {
                        R_CSTL_LOG_ERROR ("Failed to wait for fence: %s", R_CVulkan_ErrorToString (err));
                        return -1;
                }
                
                err = R_CVulkan_FenceReset (pDevice, pCurrentFrame->pInFlightFence, 1);
                if (err != R_CVULKAN_OK)
                {
                        R_CSTL_LOG_ERROR ("Failed to reset fence: %s", R_CVulkan_ErrorToString (err));
                        return -1;
                }
        }
        uint32_t imageIndex = 0;
        enum R_CVulkan_Error err = R_CVulkan_SwapchainAcquireNextImage (
            &pSubsystem->pPipelineContext->swapchain,
            UINT64_MAX,
            R_CVulkan_SemaphoreGetHandle (&pSubsystem->pPipelineContext->imageAvailableSemaphore),
            VK_NULL_HANDLE,
            &imageIndex);
        if (err != R_CVULKAN_OK)
        {
                R_CSTL_LOG_ERROR ("Failed to acquire next image: %s", R_CVulkan_ErrorToString (err));
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
#if defined(R_GAME_DEBUG)
        if (pSubsystem == NULL || pSubsystem->pPipelineContext == NULL)
        {
                R_CSTL_LOG_ERROR ("Invalid parameters for RenderFrame");
                return -1;
        }
#endif
        if (pSubsystem->state != R_GAME_RENDERER_STATE_RUNNING)
        {
                return -1;
        }

        struct R_GameRendererFrame* pCurrentFrame =
            &pSubsystem->pFrames[pSubsystem->currentFrameIndex];

        size_t cmdBufferCount = R_CSTL_ArrayLength (pCurrentFrame->pCommandBufferArray) / 
            sizeof (struct R_CVulkan_CommandBuffer);
        const uint8_t* pCmdBufferData = R_CSTL_ArrayData (pCurrentFrame->pCommandBufferArray);
        
        size_t layerCount = R_CSTL_ArrayLength (pSubsystem->pLayerArray) / 
            sizeof (struct R_GameRendererLayer);
        const uint8_t* pLayerData = R_CSTL_ArrayData (pSubsystem->pLayerArray);
        
        size_t currentBufferIndex = 0;
        
        for (size_t layerIdx = 0; layerIdx < layerCount; ++layerIdx)
        {
                struct R_GameRendererLayer* pLayer = 
                    (struct R_GameRendererLayer*)(pLayerData + layerIdx * sizeof (struct R_GameRendererLayer));
                if (!(pLayer->flags & R_GAME_RENDERER_LAYER_FLAG_ENABLED))
                {
                        continue;
                }

                if (currentBufferIndex >= cmdBufferCount)
                {
                        R_CSTL_LOG_WARNING ("Exceeded maximum command buffers per frame");
                        break;
                }

                struct R_CVulkan_CommandBuffer* pCmdBuffer =
                    (struct R_CVulkan_CommandBuffer*)(pCmdBufferData + currentBufferIndex * sizeof (struct R_CVulkan_CommandBuffer));

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
                        pLayer->beforePassCallback ((void*)pLayer, pCmdBuffer, sizeof (pCmdBuffer));
                }

                if (pLayer->renderCallback != NULL)
                {
                        pLayer->renderCallback ((void*)pLayer, pCmdBuffer, sizeof (pCmdBuffer));
                }

                if (pLayer->afterPassCallback != NULL)
                {
                        pLayer->afterPassCallback ((void*)pLayer, pCmdBuffer, sizeof (pCmdBuffer));
                }

                err = R_CVulkan_EndCommandBuffer (pCmdBuffer);
                if (err != R_CVULKAN_OK)
                {
                        R_CSTL_LOG_ERROR (
                            "Failed to end command buffer: %s",
                            R_CVulkan_ErrorToString (err));
                        return -1;
                }

                currentBufferIndex++;
        }

        if (g_lifecycle.renderCallback != NULL)
        {
                g_lifecycle.renderCallback (pSubsystem, NULL, 0);
        }

        return 0;
}

GAME_API int
R_GameRenderer_EndFrame (struct R_GameRendererSubsystem* pSubsystem)
{
#if defined(R_GAME_DEBUG)
        if (pSubsystem == NULL || pSubsystem->pPipelineContext == NULL)
        {
                R_CSTL_LOG_ERROR ("Invalid parameters for EndFrame");
                return -1;
        }
#endif
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

        size_t cmdBufferCount = R_CSTL_ArrayLength (pCurrentFrame->pCommandBufferArray) / 
            sizeof (struct R_CVulkan_CommandBuffer);
        const uint8_t* pCmdBufferData = R_CSTL_ArrayData (pCurrentFrame->pCommandBufferArray);
        
        size_t layerCount = R_CSTL_ArrayLength (pSubsystem->pLayerArray) / 
            sizeof (struct R_GameRendererLayer);
        const uint8_t* pLayerData = R_CSTL_ArrayData (pSubsystem->pLayerArray);
        
        struct R_CVulkan_CommandBuffer* commandBuffers[R_GAME_RENDERER_MAX_COMMAND_BUFFERS_PER_FRAME];
        size_t commandBufferCount = 0;
        
        for (size_t layerIdx = 0; layerIdx < layerCount; ++layerIdx)
        {
                struct R_GameRendererLayer* pLayer = 
                    (struct R_GameRendererLayer*)(pLayerData + layerIdx * sizeof (struct R_GameRendererLayer));
                if (!(pLayer->flags & R_GAME_RENDERER_LAYER_FLAG_ENABLED))
                {
                        continue;
                }

                if (commandBufferCount < R_GAME_RENDERER_MAX_COMMAND_BUFFERS_PER_FRAME)
                {
                        commandBuffers[commandBufferCount] = 
                            (struct R_CVulkan_CommandBuffer*)(pCmdBufferData + commandBufferCount * sizeof (struct R_CVulkan_CommandBuffer));
                        commandBufferCount++;
                }
        }
        VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        enum R_CVulkan_Error err = R_CVulkan_QueueSubmit (
            pGraphicsQueue,
            commandBuffers,
            commandBufferCount,
            &pSubsystem->pPipelineContext->imageAvailableSemaphore,
            1,
            &waitStages,
            pCurrentFrame->pRenderFinishedSemaphore,
            1,
            pCurrentFrame->pInFlightFence);
        if (err != R_CVULKAN_OK)
        {
                R_CSTL_LOG_ERROR ("Failed to submit command buffers: %s", R_CVulkan_ErrorToString (err));
                return -1;
        }

        VkSwapchainKHR swapchainHandle = 
            R_CVulkan_SwapchainGetHandle (&pSubsystem->pPipelineContext->swapchain);
        uint32_t imageIndex = 0;
        
        err = R_CVulkan_QueuePresent (
            pPresentQueue,
            &swapchainHandle,
            1,
            &imageIndex,
            pCurrentFrame->pRenderFinishedSemaphore,
            1);
        if (err != R_CVULKAN_OK)
        {
                R_CSTL_LOG_ERROR ("Failed to present image: %s", R_CVulkan_ErrorToString (err));
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
        if (pSubsystem == NULL || pSubsystem->pPipelineContext == NULL)
        {
                return -1;
        }

        struct R_CVulkan_Device* pDevice = 
            R_GameCVulkan_PipelineContextGetDevice (pSubsystem->pPipelineContext);

        for (uint32_t i = 0; i < pSubsystem->maxFramesInFlight; ++i)
        {
                struct R_GameRendererFrame* pFrame = &pSubsystem->pFrames[i];
                if (pFrame->pInFlightFence != NULL)
                {
                        R_CVulkan_WaitForFences (pDevice, pFrame->pInFlightFence, 1, UINT64_MAX);
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
#if defined(R_GAME_DEBUG)
        if (pSubsystem == NULL || pName == NULL)
        {
                R_CSTL_LOG_ERROR ("Invalid parameters for AddLayer");
                return -1;
        }

        if (!pSubsystem->isInitialized)
        {
                R_CSTL_LOG_ERROR ("Renderer subsystem not initialized");
                return -1;
        }
#endif

        size_t currentLayerCount = R_CSTL_ArrayLength (pSubsystem->pLayerArray) / 
            sizeof (struct R_GameRendererLayer);
        if (currentLayerCount >= R_GAME_RENDERER_MAX_LAYERS)
        {
                R_CSTL_LOG_ERROR ("Maximum number of layers reached");
                return -1;
        }

        struct R_GameRendererLayer newLayer = {0};
        
        size_t nameLen = strlen (pName);
        newLayer.pName = (char*)R_CSTL_HeapAlloc (nameLen + 1);
        if (newLayer.pName == NULL)
        {
                R_CSTL_LOG_ERROR ("Failed to allocate layer name");
                return -1;
        }
        memcpy (newLayer.pName, pName, nameLen);
        newLayer.pName[nameLen] = '\0';

        newLayer.priority = priority;
        newLayer.flags = flags | R_GAME_RENDERER_LAYER_FLAG_ENABLED;
        newLayer.pUserData = pUserData;
        newLayer.renderCallback = NULL;
        newLayer.beforePassCallback = NULL;
        newLayer.afterPassCallback = NULL;

#if defined(R_GAME_DEBUG)
        newLayer.isInitialized = true;
#endif

        int result = R_CSTL_ArrayPushData (
            pSubsystem->pLayerArray,
            (const uint8_t*)&newLayer,
            sizeof (struct R_GameRendererLayer));
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
        if (pSubsystem == NULL)
        {
                R_CSTL_LOG_ERROR ("Invalid parameters for RemoveLayer");
                return -1;
        }

        size_t layerCount = R_CSTL_ArrayLength (pSubsystem->pLayerArray) / 
            sizeof (struct R_GameRendererLayer);
        if (layerIndex >= layerCount)
        {
                R_CSTL_LOG_ERROR ("Invalid layer index: %u", layerIndex);
                return -1;
        }

        const uint8_t* pLayerData = R_CSTL_ArrayData (pSubsystem->pLayerArray);
        struct R_GameRendererLayer* pLayer = 
            (struct R_GameRendererLayer*)(pLayerData + layerIndex * sizeof (struct R_GameRendererLayer));

        if (pLayer->pName != NULL)
        {
                R_CSTL_HeapFree (pLayer->pName);
        }
        struct R_CSTL_Array* pNewArray = R_CSTL_NewArrayWithCapacity (
            sizeof (struct R_GameRendererLayer) * (layerCount - 1));
        if (pNewArray == NULL)
        {
                R_CSTL_LOG_ERROR ("Failed to allocate new layer array");
                return -1;
        }
        for (size_t i = 0; i < layerCount; ++i)
        {
                if (i == layerIndex)
                {
                        continue;
                }
                struct R_GameRendererLayer* pSrcLayer = 
                    (struct R_GameRendererLayer*)(pLayerData + i * sizeof (struct R_GameRendererLayer));
                int result = R_CSTL_ArrayPushData (
                    pNewArray,
                    (const uint8_t*)pSrcLayer,
                    sizeof (struct R_GameRendererLayer));
                if (result != R_CSTL_OK)
                {
                        R_CSTL_LOG_ERROR ("Failed to copy layer to new array");
                        R_CSTL_DeleteArray (pNewArray);
                        return -1;
                }
        }
        R_CSTL_DeleteArray (pSubsystem->pLayerArray);
        pSubsystem->pLayerArray = pNewArray;

        R_CSTL_LOG_INFO ("Removed render layer at index %u", layerIndex);
        return 0;
}

GAME_API int
R_GameRenderer_SetLayerEnabled (
    struct R_GameRendererSubsystem* pSubsystem,
    uint32_t                        layerIndex,
    int                             enabled)
{
#if defined(R_GAME_DEBUG)
        if (pSubsystem == NULL)
        {
                R_CSTL_LOG_ERROR ("Invalid parameters for SetLayerEnabled");
                return -1;
        }
#endif
        size_t layerCount = R_CSTL_ArrayLength (pSubsystem->pLayerArray) / 
            sizeof (struct R_GameRendererLayer);
        if (layerIndex >= layerCount)
        {
                R_CSTL_LOG_ERROR ("Invalid layer index: %u", layerIndex);
                return -1;
        }

        const uint8_t* pLayerData = R_CSTL_ArrayData (pSubsystem->pLayerArray);
        struct R_GameRendererLayer* pLayer = 
            (struct R_GameRendererLayer*)(pLayerData + layerIndex * sizeof (struct R_GameRendererLayer));
        if (enabled)
        {
                pLayer->flags |= R_GAME_RENDERER_LAYER_FLAG_ENABLED;
        }
        else
        {
                pLayer->flags &= ~R_GAME_RENDERER_LAYER_FLAG_ENABLED;
        }

        return 0;
}

GAME_API struct R_GameRendererLayer*
R_GameRenderer_GetLayer (struct R_GameRendererSubsystem* pSubsystem, uint32_t layerIndex)
{
#if defined(R_GAME_DEBUG)
        if (pSubsystem == NULL)
        {
                return NULL;
        }
#endif
        size_t layerCount = R_CSTL_ArrayLength (pSubsystem->pLayerArray) / 
            sizeof (struct R_GameRendererLayer);
        if (layerIndex >= layerCount)
        {
                return NULL;
        }

        const uint8_t* pLayerData = R_CSTL_ArrayData (pSubsystem->pLayerArray);
        return (struct R_GameRendererLayer*)(pLayerData + layerIndex * sizeof (struct R_GameRendererLayer));
}

GAME_API int
R_GameRenderer_SortLayers (struct R_GameRendererSubsystem* pSubsystem)
{
        if (pSubsystem == NULL)
        {
                R_CSTL_LOG_ERROR ("Invalid subsystem for SortLayers");
                return -1;
        }
        int result = R_CSTL_ArraySort (
            pSubsystem->pLayerArray,
            sizeof (struct R_GameRendererLayer),
            R_GameRenderer_CompareLayers,
            NULL);
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
        if (pSubsystem == NULL || pResource == NULL)
        {
                R_CSTL_LOG_ERROR ("Invalid parameters for RegisterResource");
                return 0;
        }

#if defined(R_GAME_DEBUG)
        if (!pSubsystem->isInitialized)
        {
                R_CSTL_LOG_ERROR ("Renderer subsystem not initialized");
                return 0;
        }
#endif

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

        if (pName != NULL)
        {
                size_t nameLen = strlen (pName);
                char* pNameCopy = (char*)R_CSTL_HeapAlloc (nameLen + 1);
                if (pNameCopy != NULL)
                {
                        memcpy (pNameCopy, pName, nameLen);
                        pNameCopy[nameLen] = '\0';
                        newResource.pName = pNameCopy;
                }
                else
                {
                        newResource.pName = NULL;
                }
        }
        else
        {
                newResource.pName = NULL;
        }

#if defined(R_GAME_DEBUG)
        newResource.isInitialized = true;
#endif

        int result = R_CSTL_ArrayPushData (
            pSubsystem->pResourceArray,
            (const uint8_t*)&newResource,
            sizeof (struct R_GameRendererResource));
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
        if (pSubsystem == NULL || handle == 0)
        {
                R_CSTL_LOG_ERROR ("Invalid parameters for UnregisterResource");
                return -1;
        }

        size_t resourceCount = R_CSTL_ArrayLength (pSubsystem->pResourceArray) / 
            sizeof (struct R_GameRendererResource);
        const uint8_t* pResourceData = R_CSTL_ArrayData (pSubsystem->pResourceArray);
        
        size_t foundIndex = SIZE_MAX;
        for (size_t i = 0; i < resourceCount; ++i)
        {
                struct R_GameRendererResource* pResource = 
                    (struct R_GameRendererResource*)(pResourceData + i * sizeof (struct R_GameRendererResource));
                if (pResource->handle == handle)
                {
                        foundIndex = i;
                        break;
                }
        }

        if (foundIndex == SIZE_MAX)
        {
                R_CSTL_LOG_WARNING ("Resource not found: %llu", handle);
                return -1;
        }

        struct R_GameRendererResource* pResource = 
            (struct R_GameRendererResource*)(pResourceData + foundIndex * sizeof (struct R_GameRendererResource));
        
        if (pResource->pName != NULL)
        {
                R_CSTL_HeapFree ((void*)pResource->pName);
        }

        struct R_CSTL_Array* pNewArray = R_CSTL_NewArrayWithCapacity (
            sizeof (struct R_GameRendererResource) * (resourceCount - 1));
        if (pNewArray == NULL)
        {
                R_CSTL_LOG_ERROR ("Failed to allocate new resource array");
                return -1;
        }
        for (size_t i = 0; i < resourceCount; ++i)
        {
                if (i == foundIndex)
                {
                        continue;
                }
                
                struct R_GameRendererResource* pSrcResource = 
                    (struct R_GameRendererResource*)(pResourceData + i * sizeof (struct R_GameRendererResource));
                int result = R_CSTL_ArrayPushData (
                    pNewArray,
                    (const uint8_t*)pSrcResource,
                    sizeof (struct R_GameRendererResource));
                if (result != R_CSTL_OK)
                {
                        R_CSTL_LOG_ERROR ("Failed to copy resource to new array");
                        R_CSTL_DeleteArray (pNewArray);
                        return -1;
                }
        }

        R_CSTL_DeleteArray (pSubsystem->pResourceArray);
        pSubsystem->pResourceArray = pNewArray;

        R_CSTL_LOG_INFO ("Unregistered resource: %llu", handle);
        return 0;
}

GAME_API const void*
R_GameRenderer_GetResource (
    struct R_GameRendererSubsystem* pSubsystem,
    uint64_t                        handle)
{
        if (pSubsystem == NULL || handle == 0)
        {
                return NULL;
        }

        size_t resourceCount = R_CSTL_ArrayLength (pSubsystem->pResourceArray) / 
            sizeof (struct R_GameRendererResource);
        const uint8_t* pResourceData = R_CSTL_ArrayData (pSubsystem->pResourceArray);
        
        for (size_t i = 0; i < resourceCount; ++i)
        {
                struct R_GameRendererResource* pResource = 
                    (struct R_GameRendererResource*)(pResourceData + i * sizeof (struct R_GameRendererResource));
                if (pResource->handle == handle)
                {
                        return pResource->pResource;
                }
        }

        return NULL;
}

GAME_API int
R_GameRenderer_AddResourceRef (
    struct R_GameRendererSubsystem* pSubsystem,
    uint64_t                        handle)
{
        if (pSubsystem == NULL || handle == 0)
        {
                return -1;
        }

        size_t resourceCount = R_CSTL_ArrayLength (pSubsystem->pResourceArray) / 
            sizeof (struct R_GameRendererResource);
        const uint8_t* pResourceData = R_CSTL_ArrayData (pSubsystem->pResourceArray);
        
        for (size_t i = 0; i < resourceCount; ++i)
        {
                struct R_GameRendererResource* pResource = 
                    (struct R_GameRendererResource*)(pResourceData + i * sizeof (struct R_GameRendererResource));
                if (pResource->handle == handle)
                {
                        pResource->refCount++;
                        return 0;
                }
        }

        return -1;
}

GAME_API int
R_GameRenderer_ReleaseResourceRef (
    struct R_GameRendererSubsystem* pSubsystem,
    uint64_t                        handle)
{
        if (pSubsystem == NULL || handle == 0)
        {
                return -1;
        }

        size_t resourceCount = R_CSTL_ArrayLength (pSubsystem->pResourceArray) / 
            sizeof (struct R_GameRendererResource);
        const uint8_t* pResourceData = R_CSTL_ArrayData (pSubsystem->pResourceArray);
        
        for (size_t i = 0; i < resourceCount; ++i)
        {
                struct R_GameRendererResource* pResource = 
                    (struct R_GameRendererResource*)(pResourceData + i * sizeof (struct R_GameRendererResource));
                if (pResource->handle == handle)
                {
                        if (pResource->refCount > 0)
                        {
                                pResource->refCount--;
                        }
                        return 0;
                }
        }

        return -1;
}

GAME_API struct R_CVulkan_CommandBuffer*
R_GameRenderer_GetCommandBuffer (
    struct R_GameRendererSubsystem* pSubsystem,
    uint32_t                        frameIndex,
    uint32_t                        bufferIndex)
{
        if (pSubsystem == NULL)
        {
                return NULL;
        }

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

        const uint8_t* pCmdBufferData = R_CSTL_ArrayData (pFrame->pCommandBufferArray);
        return (struct R_CVulkan_CommandBuffer*)(pCmdBufferData + bufferIndex * sizeof (struct R_CVulkan_CommandBuffer));
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

GAME_API int
R_GameRenderer_ValidateLifecycle (struct R_GameRenderer* pRenderer)
{
        (void)pRenderer;
        
        if (g_lifecycle.pRenderer == NULL)
        {
                R_CSTL_LOG_WARNING ("No renderer registered in lifecycle");
                return -1;
        }

        if (g_lifecycle.constructCallback == NULL)
        {
                R_CSTL_LOG_WARNING ("No construct callback registered");
        }

        if (g_lifecycle.overCallback == NULL)
        {
                R_CSTL_LOG_WARNING ("No over callback registered");
        }

        R_CSTL_LOG_INFO ("Lifecycle validation completed");
        return 0;
}