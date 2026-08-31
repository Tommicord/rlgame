#include "rlgame.base/game/game_state.h"
#include "rlgame.base/game/game_renderer_subsystem.h"
#include "rlgame.base/game/game_platform.h"
#include "rlgame.base/cstl/cstl_log.h"
#include "rlgame.base/cvulkan/cvulkan_surface.h"

#include <string.h>

R_GAME_API enum R_CVulkan_Error
r_game_state_Initialize (struct r_game_state* pState, const struct r_game_state_create_info* pCreateInfo)
{
    if (!pState || !pCreateInfo)
    {
        return R_GAME_ERROR_NULL_POINTER;
    }
    memset (pState, 0, sizeof (*pState));

    struct r_game_pipeline_context_create_info pipelineCreateInfo = {0};
    pipelineCreateInfo.pApplicationName = pCreateInfo->pApplicationName;
    pipelineCreateInfo.windowWidth = pCreateInfo->windowWidth;
    pipelineCreateInfo.windowHeight = pCreateInfo->windowHeight;

#if defined(R_CVULKAN_PLATFORM_WINDOWS)
    pipelineCreateInfo.hInstance = pCreateInfo->hInstance;
    pipelineCreateInfo.hWnd = pCreateInfo->hWnd;
#elif defined(R_CVULKAN_PLATFORM_LINUX)
    pipelineCreateInfo.linuxBackend = pCreateInfo->linuxBackend;
    pipelineCreateInfo.pDisplay = pCreateInfo->pDisplay;
    pipelineCreateInfo.pSurface = pCreateInfo->pSurface;
    pipelineCreateInfo.pX11Display = pCreateInfo->pX11Display;
    pipelineCreateInfo.x11Window = pCreateInfo->x11Window;
    pipelineCreateInfo.pXCBConnection = pCreateInfo->pXCBConnection;
    pipelineCreateInfo.xcbWindow = pCreateInfo->xcbWindow;
#elif defined(R_CVULKAN_PLATFORM_ANDROID)
    pipelineCreateInfo.pWindow = pCreateInfo->pWindow;
#elif defined(R_CVULKAN_PLATFORM_MACOS)
    pipelineCreateInfo.pNSWindow = pCreateInfo->pNSWindow;
#endif

    enum r_game_error result = r_game_new_pipeline_context (&pState->context, &pipelineCreateInfo);
    if (result != R_GAME_OK)
    {
        R_CSTL_LOG_ERROR (
            "GameState: Failed to initialize Vulkan pipeline context (error: %s)",
            r_game_error_to_string (result));
        return R_CVULKAN_ERROR_FAILED;
    }

    pState->pRendererManager = r_game_renderer_new_manager (&pState->context);
    if (pState->pRendererManager == NULL)
    {
        R_CSTL_LOG_ERROR ("GameState: Failed to create renderer manager");
        r_game_pipeline_context_delete (&pState->context);
        return R_CVULKAN_ERROR_FAILED;
    }
    R_CSTL_LOG_INFO ("GameState: Initialized");
    return R_CVULKAN_OK;
}
R_GAME_API void
r_game_state_Cleanup (struct r_game_state* pState)
{
    if (pState->pRendererManager)
    {
        r_game_renderer_delete_manager (pState->pRendererManager);
        pState->pRendererManager = NULL;
    }
    r_game_pipeline_context_delete (&pState->context);
    memset (pState, 0, sizeof (*pState));
}

R_GAME_API struct r_game_pipeline_context*
r_game_state_get_vulkan_context (struct r_game_state* pState)
{
    return &pState->context;
}

R_GAME_API struct r_game_renderer_manager*
r_game_state_get_renderer_manager (struct r_game_state* pState)
{
    return pState->pRendererManager;
}

R_GAME_API int
r_game_state_render_frame (struct r_game_state* pState)
{
    if (pState->pRendererManager == NULL)
    {
        return R_GAME_ERROR_NOT_INITIALIZED;
    }

    if (r_game_renderer_compose_frame (pState->pRendererManager) != R_GAME_OK)
    {
        return R_GAME_ERROR_FAILED;
    }

    if (r_game_renderer_present_frame (pState->pRendererManager) != R_GAME_OK)
    {
        return R_GAME_ERROR_FAILED;
    }
    return R_GAME_OK;
}
