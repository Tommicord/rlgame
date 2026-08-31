#pragma once

#include <stdint.h>
#include <inttypes.h>
#include <stddef.h>

#include "rlgame.base/game/game_platform.h"
#include "rlgame.base/game/game_pipeline.h"
#include "rlgame.base/cstl/cstl_stack.h"

typedef void (*r_game_lifecycle_construct) (
    void*        pDrawable,
    const char*  pName,
    const void*  pResource,
    const size_t resourceSize);

typedef void (*r_game_lifecycle_resume) (void* pDrawable, const void* pResource, const size_t resourceSize);

typedef void (*r_game_lifecycle_before_each) (void* pDrawable, const void* pResource, const size_t resourceSize);

typedef void (*r_game_lifecycle_after_each) (void* pDrawable, const void* pResource, const size_t resourceSize);

typedef void (*r_game_lifecycle_before_pass) (void* pDrawable, const void* pResource, const size_t resourceSize);

typedef void (*r_game_lifecycle_pause) (void* pDrawable, const void* pResource, const size_t resourceSize);

typedef void (*r_game_lifecycle_after_pass) (void* pDrawable, const void* pResource, const size_t resourceSize);

typedef void (*r_game_lifecycle_render) (void* pDrawable, const void* pResource, const size_t resourceSize);

typedef void (*r_game_lifecycle_stop) (void* pDrawable, const void* pResource, const size_t resourceSize);

typedef void (*r_game_lifecycle_over) (void* pDrawable, const void* pResource, const size_t resourceSize);
typedef uint64_t r_game_renderer_resource_handle;

struct r_game_renderer_render_task
{
        uint32_t            layerIndex;
        uint32_t            commandBufferIndex;
        uint32_t            frameIndex;
        int                 completed;
        R_GAME_ATOMIC_INT32 atomicCompleted;
};

struct r_game_renderer_worker_thread
{
        R_GAME_THREAD_HANDLE            threadHandle;
        R_GAME_THREAD_ID                threadId;
        uint32_t                        workerIndex;
        int                             isRunning;
        R_GAME_ATOMIC_INT32             atomicIsRunning;
        struct r_game_renderer_subsystem* pSubsystem;
};

struct r_game_renderer_thread_pool
{
        struct r_game_renderer_worker_thread workers[R_GAME_RENDERER_MAX_WORKER_THREADS];
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

struct r_game_renderer_frame;
struct r_game_renderer_layer;
struct r_game_renderer_resource;
struct r_game_renderer_lifecycle;
struct r_game_renderer_subsystem;

struct r_game_renderer_render_target
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

struct r_game_renderer_subsystem_entry
{
        struct r_game_renderer_subsystem*   pSubsystem;
        struct r_game_renderer_render_target renderTarget;
        uint32_t                          priority;
        uint32_t                          flags;
        int                               isVisible;
        float                             blendFactor;
};
struct r_game_renderer_manager;

R_GAME_API struct r_game_renderer_subsystem*
                    r_game_renderer_new_subsystem (struct r_game_renderer_subsystem* pSubsystem);
R_GAME_API int      r_game_renderer_delete_subsystem (struct r_game_renderer_subsystem* pSubsystem);
R_GAME_API int      r_game_renderer_subsystem_start (struct r_game_renderer_subsystem* pSubsystem);
R_GAME_API int      r_game_renderer_subsystem_stop (struct r_game_renderer_subsystem* pSubsystem);
R_GAME_API int      r_game_renderer_subsystem_pause (struct r_game_renderer_subsystem* pSubsystem);
R_GAME_API int      r_game_renderer_subsystem_resume (struct r_game_renderer_subsystem* pSubsystem);
R_GAME_API uint32_t r_game_renderer_subsystem_get_state (struct r_game_renderer_subsystem* pSubsystem);

R_GAME_API int r_game_renderer_set_pipeline_context (
    struct r_game_renderer_subsystem* pSubsystem,
    struct r_game_pipeline_context*  pPipelineContext);

R_GAME_API int r_game_renderer_begin_frame (struct r_game_renderer_subsystem* pSubsystem);
R_GAME_API int r_game_renderer_render_frame (struct r_game_renderer_subsystem* pSubsystem);
R_GAME_API int r_game_renderer_end_frame (struct r_game_renderer_subsystem* pSubsystem);
R_GAME_API int r_game_renderer_wait_for_frame (struct r_game_renderer_subsystem* pSubsystem);

R_GAME_API int r_game_renderer_add_layer (
    struct r_game_renderer_subsystem* pSubsystem,
    const char*                     pName,
    uint32_t                        priority,
    uint32_t                        flags,
    void*                           pUserData);
R_GAME_API int r_game_renderer_remove_layer (struct r_game_renderer_subsystem* pSubsystem, uint32_t layerIndex);
R_GAME_API int
r_game_renderer_set_layer_enabled (struct r_game_renderer_subsystem* pSubsystem, uint32_t layerIndex, int enabled);
R_GAME_API struct r_game_renderer_layer*
               r_game_renderer_get_layer (struct r_game_renderer_subsystem* pSubsystem, uint32_t layerIndex);
R_GAME_API int r_game_renderer_sort_layers (struct r_game_renderer_subsystem* pSubsystem);

R_GAME_API int r_game_renderer_set_layer_renderCallback (
    struct r_game_renderer_subsystem* pSubsystem,
    uint32_t                        layerIndex,
    r_game_lifecycle_render           callback);
R_GAME_API int r_game_renderer_set_layer_beforePassCallback (
    struct r_game_renderer_subsystem* pSubsystem,
    uint32_t                        layerIndex,
    r_game_lifecycle_before_pass       callback);
R_GAME_API int r_game_renderer_set_layer_afterPassCallback (
    struct r_game_renderer_subsystem* pSubsystem,
    uint32_t                        layerIndex,
    r_game_lifecycle_after_pass        callback);

R_GAME_API r_game_renderer_resource_handle r_game_renderer_register_resource (
    struct r_game_renderer_subsystem* pSubsystem,
    uint32_t                        type,
    void*                           pResource,
    uint64_t                        size,
    const char*                     pName);
R_GAME_API int r_game_renderer_unregister_resource (
    struct r_game_renderer_subsystem* pSubsystem,
    r_game_renderer_resource_handle    handle);
R_GAME_API const void*
r_game_renderer_get_resource (struct r_game_renderer_subsystem* pSubsystem, r_game_renderer_resource_handle handle);
R_GAME_API int r_game_renderer_add_resource_ref (
    struct r_game_renderer_subsystem* pSubsystem,
    r_game_renderer_resource_handle    handle);
R_GAME_API int r_game_renderer_release_resource_ref (
    struct r_game_renderer_subsystem* pSubsystem,
    r_game_renderer_resource_handle    handle);

R_GAME_API struct R_CVulkan_CommandBuffer* r_game_renderer_get_command_buffer (
    struct r_game_renderer_subsystem* pSubsystem,
    uint32_t                        frameIndex,
    uint32_t                        bufferIndex);

R_GAME_API void r_game_renderer_lifecycle_register_renderer (const void* pRenderer);
R_GAME_API void r_game_renderer_lifecycle_register_construct (r_game_lifecycle_construct callback);
R_GAME_API void r_game_renderer_lifecycle_register_resume (r_game_lifecycle_resume callback);
R_GAME_API void r_game_renderer_lifecycle_register_pause (r_game_lifecycle_pause callback);
R_GAME_API void r_game_renderer_lifecycle_register_beforeEach (r_game_lifecycle_before_each callback);
R_GAME_API void r_game_renderer_lifecycle_register_afterEach (r_game_lifecycle_after_each callback);
R_GAME_API void r_game_renderer_lifecycle_register_beforePass (r_game_lifecycle_before_pass callback);
R_GAME_API void r_game_renderer_lifecycle_register_afterPass (r_game_lifecycle_after_pass callback);
R_GAME_API void r_game_renderer_lifecycle_register_render (r_game_lifecycle_render callback);
R_GAME_API void r_game_renderer_lifecycle_register_stop (r_game_lifecycle_stop callback);
R_GAME_API void r_game_renderer_lifecycle_register_over (r_game_lifecycle_over callback);

R_GAME_API struct r_game_renderer_manager*
               r_game_renderer_new_manager (struct r_game_pipeline_context* pPipelineContext);
R_GAME_API int r_game_renderer_delete_manager (struct r_game_renderer_manager* pManager);
R_GAME_API int r_game_renderer_add_subsystem (
    struct r_game_renderer_manager*   pManager,
    struct r_game_renderer_subsystem* pSubsystem,
    uint32_t                        priority,
    uint32_t                        flags,
    float                           blendFactor);
R_GAME_API int r_game_renderer_remove_subsystem (
    struct r_game_renderer_manager*   pManager,
    struct r_game_renderer_subsystem* pSubsystem);
R_GAME_API int r_game_renderer_set_subsystem_visible (
    struct r_game_renderer_manager*   pManager,
    struct r_game_renderer_subsystem* pSubsystem,
    int                             visible);
R_GAME_API int r_game_renderer_set_subsystem_blendFactor (
    struct r_game_renderer_manager*   pManager,
    struct r_game_renderer_subsystem* pSubsystem,
    float                           blendFactor);
R_GAME_API int r_game_renderer_compose_frame (struct r_game_renderer_manager* pManager);
R_GAME_API int r_game_renderer_present_frame (struct r_game_renderer_manager* pManager);
