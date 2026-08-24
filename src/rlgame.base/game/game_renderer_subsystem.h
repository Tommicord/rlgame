#pragma once

#include <stdint.h>
#include <inttypes.h>
#include <stddef.h>

#include "rlgame.base/game/game_platform.h"
#include "rlgame.base/game/game_cvulkan_pipeline.h"
#include "rlgame.base/cstl/cstl_stack.h"

typedef void (*R_GameLifecycleConstruct) (
    void*        pDrawable,
    const char*  pName,
    const void*  pResource,
    const size_t resourceSize);

typedef void (*R_GameLifecycleResume) (void* pDrawable, const void* pResource, const size_t resourceSize);

typedef void (*R_GameLifecycleBeforeEach) (void* pDrawable, const void* pResource, const size_t resourceSize);

typedef void (*R_GameLifecycleAfterEach) (void* pDrawable, const void* pResource, const size_t resourceSize);

typedef void (*R_GameLifecycleBeforePass) (void* pDrawable, const void* pResource, const size_t resourceSize);

typedef void (*R_GameLifecyclePause) (void* pDrawable, const void* pResource, const size_t resourceSize);

typedef void (*R_GameLifecycleAfterPass) (void* pDrawable, const void* pResource, const size_t resourceSize);

typedef void (*R_GameLifecycleRender) (void* pDrawable, const void* pResource, const size_t resourceSize);

typedef void (*R_GameLifecycleStop) (void* pDrawable, const void* pResource, const size_t resourceSize);

typedef void (*R_GameLifecycleOver) (void* pDrawable, const void* pResource, const size_t resourceSize);
typedef uint64_t R_GameRendererResourceHandle;

struct R_GameRendererRenderTask
{
        uint32_t            layerIndex;
        uint32_t            commandBufferIndex;
        uint32_t            frameIndex;
        int                 completed;
        R_GAME_ATOMIC_INT32 atomicCompleted;
};

struct R_GameRendererWorkerThread
{
        R_GAME_THREAD_HANDLE            threadHandle;
        R_GAME_THREAD_ID                threadId;
        uint32_t                        workerIndex;
        int                             isRunning;
        R_GAME_ATOMIC_INT32             atomicIsRunning;
        struct R_GameRendererSubsystem* pSubsystem;
};

struct R_GameRendererThreadPool
{
        struct R_GameRendererWorkerThread workers[R_GAME_RENDERER_MAX_WORKER_THREADS];
        uint32_t                          workerCount;
        R_GAME_MUTEX                      taskMutex;
        R_GAME_CONDITION_VARIABLE         taskAvailable;
        R_GAME_CONDITION_VARIABLE         taskComplete;
        struct R_CSTL_Stack*              pTaskQueue;
        R_GAME_ATOMIC_UINT32              atomicPendingTasks;
        R_GAME_ATOMIC_UINT32              atomicCompletedTasks;
        int                               shutdownRequested;
        R_GAME_ATOMIC_INT32               atomicShutdownRequested;
};

struct R_GameRendererFrame;
struct R_GameRendererLayer;
struct R_GameRendererResource;
struct R_GameRendererLifecycle;
struct R_GameRendererSubsystem;

struct R_GameRendererRenderTarget
{
        struct R_CVulkan_Image*       pColorImage;
        struct R_CVulkan_ImageView*   pColorImageView;
        struct R_CVulkan_Image*       pDepthImage;
        struct R_CVulkan_ImageView*   pDepthImageView;
        struct R_CVulkan_Framebuffer* pFramebuffer;
        uint32_t                      width;
        uint32_t                      height;
        uint32_t                      format;
};

struct R_GameRendererSubsystemEntry
{
        struct R_GameRendererSubsystem*   pSubsystem;
        struct R_GameRendererRenderTarget renderTarget;
        uint32_t                          priority;
        uint32_t                          flags;
        int                               isVisible;
        float                             blendFactor;
};
struct R_GameRendererManager;

R_GAME_API struct R_GameRendererSubsystem*
                    R_GameRenderer_NewSubsystem (struct R_GameRendererSubsystem* pSubsystem);
R_GAME_API int      R_GameRenderer_DeleteSubsystem (struct R_GameRendererSubsystem* pSubsystem);
R_GAME_API int      R_GameRenderer_SubsystemStart (struct R_GameRendererSubsystem* pSubsystem);
R_GAME_API int      R_GameRenderer_SubsystemStop (struct R_GameRendererSubsystem* pSubsystem);
R_GAME_API int      R_GameRenderer_SubsystemPause (struct R_GameRendererSubsystem* pSubsystem);
R_GAME_API int      R_GameRenderer_SubsystemResume (struct R_GameRendererSubsystem* pSubsystem);
R_GAME_API uint32_t R_GameRenderer_SubsystemGetState (struct R_GameRendererSubsystem* pSubsystem);

R_GAME_API int R_GameRenderer_SetPipelineContext (
    struct R_GameRendererSubsystem* pSubsystem,
    struct R_Game_PipelineContext*  pPipelineContext);

R_GAME_API int R_GameRenderer_BeginFrame (struct R_GameRendererSubsystem* pSubsystem);
R_GAME_API int R_GameRenderer_RenderFrame (struct R_GameRendererSubsystem* pSubsystem);
R_GAME_API int R_GameRenderer_EndFrame (struct R_GameRendererSubsystem* pSubsystem);
R_GAME_API int R_GameRenderer_WaitForFrame (struct R_GameRendererSubsystem* pSubsystem);

R_GAME_API int R_GameRenderer_AddLayer (
    struct R_GameRendererSubsystem* pSubsystem,
    const char*                     pName,
    uint32_t                        priority,
    uint32_t                        flags,
    void*                           pUserData);
R_GAME_API int R_GameRenderer_RemoveLayer (struct R_GameRendererSubsystem* pSubsystem, uint32_t layerIndex);
R_GAME_API int
R_GameRenderer_SetLayerEnabled (struct R_GameRendererSubsystem* pSubsystem, uint32_t layerIndex, int enabled);
R_GAME_API struct R_GameRendererLayer*
               R_GameRenderer_GetLayer (struct R_GameRendererSubsystem* pSubsystem, uint32_t layerIndex);
R_GAME_API int R_GameRenderer_SortLayers (struct R_GameRendererSubsystem* pSubsystem);

R_GAME_API int R_GameRenderer_SetLayerRenderCallback (
    struct R_GameRendererSubsystem* pSubsystem,
    uint32_t                        layerIndex,
    R_GameLifecycleRender           callback);
R_GAME_API int R_GameRenderer_SetLayerBeforePassCallback (
    struct R_GameRendererSubsystem* pSubsystem,
    uint32_t                        layerIndex,
    R_GameLifecycleBeforePass       callback);
R_GAME_API int R_GameRenderer_SetLayerAfterPassCallback (
    struct R_GameRendererSubsystem* pSubsystem,
    uint32_t                        layerIndex,
    R_GameLifecycleAfterPass        callback);

R_GAME_API R_GameRendererResourceHandle R_GameRenderer_RegisterResource (
    struct R_GameRendererSubsystem* pSubsystem,
    uint32_t                        type,
    void*                           pResource,
    uint64_t                        size,
    const char*                     pName);
R_GAME_API int R_GameRenderer_UnregisterResource (
    struct R_GameRendererSubsystem* pSubsystem,
    R_GameRendererResourceHandle    handle);
R_GAME_API const void*
R_GameRenderer_GetResource (struct R_GameRendererSubsystem* pSubsystem, R_GameRendererResourceHandle handle);
R_GAME_API int R_GameRenderer_AddResourceRef (
    struct R_GameRendererSubsystem* pSubsystem,
    R_GameRendererResourceHandle    handle);
R_GAME_API int R_GameRenderer_ReleaseResourceRef (
    struct R_GameRendererSubsystem* pSubsystem,
    R_GameRendererResourceHandle    handle);

R_GAME_API struct R_CVulkan_CommandBuffer* R_GameRenderer_GetCommandBuffer (
    struct R_GameRendererSubsystem* pSubsystem,
    uint32_t                        frameIndex,
    uint32_t                        bufferIndex);

R_GAME_API void R_GameRendererLifecycle_RegisterRenderer (const void* pRenderer);
R_GAME_API void R_GameRendererLifecycle_RegisterConstruct (R_GameLifecycleConstruct callback);
R_GAME_API void R_GameRendererLifecycle_RegisterResume (R_GameLifecycleResume callback);
R_GAME_API void R_GameRendererLifecycle_RegisterPause (R_GameLifecyclePause callback);
R_GAME_API void R_GameRendererLifecycle_RegisterBeforeEach (R_GameLifecycleBeforeEach callback);
R_GAME_API void R_GameRendererLifecycle_RegisterAfterEach (R_GameLifecycleAfterEach callback);
R_GAME_API void R_GameRendererLifecycle_RegisterBeforePass (R_GameLifecycleBeforePass callback);
R_GAME_API void R_GameRendererLifecycle_RegisterAfterPass (R_GameLifecycleAfterPass callback);
R_GAME_API void R_GameRendererLifecycle_RegisterRender (R_GameLifecycleRender callback);
R_GAME_API void R_GameRendererLifecycle_RegisterStop (R_GameLifecycleStop callback);
R_GAME_API void R_GameRendererLifecycle_RegisterOver (R_GameLifecycleOver callback);

R_GAME_API struct R_GameRendererManager*
               R_GameRenderer_NewManager (struct R_Game_PipelineContext* pPipelineContext);
R_GAME_API int R_GameRenderer_DeleteManager (struct R_GameRendererManager* pManager);
R_GAME_API int R_GameRenderer_AddSubsystem (
    struct R_GameRendererManager*   pManager,
    struct R_GameRendererSubsystem* pSubsystem,
    uint32_t                        priority,
    uint32_t                        flags,
    float                           blendFactor);
R_GAME_API int R_GameRenderer_RemoveSubsystem (
    struct R_GameRendererManager*   pManager,
    struct R_GameRendererSubsystem* pSubsystem);
R_GAME_API int R_GameRenderer_SetSubsystemVisible (
    struct R_GameRendererManager*   pManager,
    struct R_GameRendererSubsystem* pSubsystem,
    int                             visible);
R_GAME_API int R_GameRenderer_SetSubsystemBlendFactor (
    struct R_GameRendererManager*   pManager,
    struct R_GameRendererSubsystem* pSubsystem,
    float                           blendFactor);
R_GAME_API int R_GameRenderer_ComposeFrame (struct R_GameRendererManager* pManager);
R_GAME_API int R_GameRenderer_PresentFrame (struct R_GameRendererManager* pManager);
