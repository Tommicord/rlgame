#include "rlgame.base/game/game_state.h"
#include "rlgame.base/game/game_renderer_subsystem.h"
#include "rlgame.base/game/game_platform.h"
#include "rlgame.base/cstl/cstl_log.h"
#include "rlgame.base/cvulkan/cvulkan_surface.h"

#include <string.h>

R_GAME_API enum R_CVulkan_Error
R_GameState_Initialize (struct R_GameState* pState, const struct R_GameStateCreateInfo* pCreateInfo)
{
    if (!pState || !pCreateInfo)
    {
        return R_GAME_ERROR_NULL_POINTER;
    }
    memset (pState, 0, sizeof (*pState));

    struct R_Game_PipelineContextCreateInfo pipelineCreateInfo = {0};
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

    enum R_GameError result = R_Game_NewPipelineContext (&pState->context, &pipelineCreateInfo);
    if (result != R_GAME_OK)
    {
        R_CSTL_LOG_ERROR (
            "GameState: Failed to initialize Vulkan pipeline context (error: %s)",
            R_GameErrorToString (result));
        return R_CVULKAN_ERROR_FAILED;
    }

    pState->pRendererManager = R_GameRenderer_NewManager (&pState->context);
    if (pState->pRendererManager == NULL)
    {
        R_CSTL_LOG_ERROR ("GameState: Failed to create renderer manager");
        R_Game_PipelineContextDelete (&pState->context);
        return R_CVULKAN_ERROR_FAILED;
    }
    R_CSTL_LOG_INFO ("GameState: Initialized");
    return R_CVULKAN_OK;
}
R_GAME_API void
R_GameState_Cleanup (struct R_GameState* pState)
{
    if (pState->pRendererManager)
    {
        R_GameRenderer_DeleteManager (pState->pRendererManager);
        pState->pRendererManager = NULL;
    }
    R_Game_PipelineContextDelete (&pState->context);
    memset (pState, 0, sizeof (*pState));
}

R_GAME_API struct R_Game_PipelineContext*
R_GameState_GetVulkanContext (struct R_GameState* pState)
{
    return &pState->context;
}

R_GAME_API struct R_Game_RendererManager*
R_GameState_GetRendererManager (struct R_GameState* pState)
{
    return pState->pRendererManager;
}

R_GAME_API int
R_GameState_RenderFrame (struct R_GameState* pState)
{
    if (pState->pRendererManager == NULL)
    {
        return R_GAME_ERROR_NOT_INITIALIZED;
    }

    if (R_GameRenderer_ComposeFrame (pState->pRendererManager) != R_GAME_OK)
    {
        return R_GAME_ERROR_FAILED;
    }

    if (R_GameRenderer_PresentFrame (pState->pRendererManager) != R_GAME_OK)
    {
        return R_GAME_ERROR_FAILED;
    }
    return R_GAME_OK;
}
