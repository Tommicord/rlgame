#include "rlgame.base/game/game_state.h"
#include "rlgame.base/cstl/cstl_log.h"
#include "rlgame.base/cvulkan/cvulkan_surface.h"
#include "rlgame.base/cvulkan/cvulkan_common.h"

#include <string.h>

R_GAME_CVULKAN_API enum R_CVulkan_Error
R_GameState_Initialize (struct R_GameState* pState, const struct R_GameStateCreateInfo* pCreateInfo)
{
#if defined(R_CVULKAN_DEBUG)
        if (!pState || !pCreateInfo)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }
#endif
        memset (pState, 0, sizeof (*pState));
#if defined(R_CVULKAN_DEBUG)
        pState->isInitialized = false;
#endif

        struct R_GameCVulkan_PipelineContextCreateInfo pipelineCreateInfo = {0};
        pipelineCreateInfo.pApplicationName = pCreateInfo->pApplicationName;

#if defined(R_CVULKAN_PLATFORM_WINDOWS)
        pipelineCreateInfo.hInstance = pCreateInfo->hInstance;
        pipelineCreateInfo.hWnd = pCreateInfo->hWnd;
#elif defined(R_CVULKAN_PLATFORM_LINUX)
        pipelineCreateInfo.pDisplay = pCreateInfo->pDisplay;
        pipelineCreateInfo.window = pCreateInfo->window;
#elif defined(R_CVULKAN_PLATFORM_ANDROID)
        pipelineCreateInfo.pWindow = pCreateInfo->pWindow;
#elif defined(R_CVULKAN_PLATFORM_MACOS)
        pipelineCreateInfo.pNSWindow = pCreateInfo->pNSWindow;
#endif

        enum R_CVulkan_Error result
            = R_GameCVulkan_NewPipelineContext (&pState->context, &pipelineCreateInfo);
        if (result != R_CVULKAN_OK)
        {
                R_CSTL_LOG_ERROR (
                    "GameState: Failed to initialize Vulkan pipeline context (error: %s)",
                    R_CVulkan_ErrorToString (result));
                return result;
        }

#if defined(R_CVULKAN_DEBUG)
        pState->isInitialized = true;
#endif
        R_CSTL_LOG_INFO ("GameState: Initialized successfully");
        return R_CVULKAN_OK;
}

R_GAME_CVULKAN_API void
R_GameState_Cleanup (struct R_GameState* pState)
{
#if defined(R_CVULKAN_DEBUG)
        R_GAME_CVULKAN_ASSERT (pState != NULL);
        if (!pState->isInitialized)
        {
                return;
        }
#endif
        R_GameCVulkan_PipelineContextDelete (&pState->context);
        memset (pState, 0, sizeof (*pState));
}

R_GAME_CVULKAN_API int
R_GameState_IsInitialized (const struct R_GameState* pState)
{
#if defined(R_CVULKAN_DEBUG)
        R_GAME_CVULKAN_ASSERT (pState != NULL);
        return pState->isInitialized;
#else
        return R_GameCVulkan_PipelineContextIsInitialized (&pState->context);
#endif
}

R_GAME_CVULKAN_API struct R_GameCVulkan_PipelineContext*
R_GameState_GetVulkanContext (struct R_GameState* pState)
{
#if defined(R_CVULKAN_DEBUG)
        R_GAME_CVULKAN_ASSERT (pState != NULL);
        return &pState->context;
#else
        return &pState->context;
#endif
}
