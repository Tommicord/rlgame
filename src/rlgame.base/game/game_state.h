#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <vulkan/vulkan.h>

#include "rlgame.base/cvulkan/cvulkan_common.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"
#include "rlgame.base/game/game_cvulkan_pipeline.h"
#include "rlgame.base/game/game_cvulkan_platform.h"

/**
 * @file game_state.h
 * @brief Game state management for the main game loop
 *
 * This module provides the high-level game state that integrates
 * the Vulkan pipeline context with game logic. It serves as the
 * bridge between the platform-specific main loop and the rendering/game systems.
 */

/**
 * @brief Main game state structure
 *
 * This structure holds all game-related state including the Vulkan
 * rendering pipeline and game-specific data. It is designed to be
 * managed by the main game loop with clear initialization/cleanup phases.
 */
struct R_GameState
{
                struct R_GameCVulkan_PipelineContext context; /**< Vulkan rendering pipeline context */
                R_GAME_CVULKAN_DEBUG_FIELD
};

/**
 * @brief Configuration parameters for game state initialization
 */
struct R_GameStateCreateInfo
{
                const struct R_CSTL_String* pApplicationName; /**< Application name (can be NULL) */
#if defined(R_CVULKAN_PLATFORM_WINDOWS)
                HINSTANCE hInstance; /**< Windows instance handle (required if not headless) */
                HWND      hWnd; /**< Windows window handle (required if not headless) */
#elif defined(R_CVULKAN_PLATFORM_LINUX)
                Display* pDisplay; /**< X11 display connection (required if not headless) */
                Window   window; /**< X11 window handle (required if not headless) */
#elif defined(R_CVULKAN_PLATFORM_ANDROID)
                ANativeWindow* pWindow; /**< Android native window (required if not headless) */
#elif defined(R_CVULKAN_PLATFORM_MACOS)
                void* pNSWindow; /**< macOS NSWindow pointer (required if not headless) */
#endif
};

/**
 * @brief Initialize the game state
 *
 * This function initializes the Vulkan pipeline context and prepares
 * the game for the main loop. It should be called once before entering
 * the game loop.
 *
 * @param pState Pointer to game state to initialize
 * @param pCreateInfo Game state creation parameters
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_GAME_CVULKAN_API enum R_CVulkan_Error
R_GameState_Initialize (struct R_GameState* pState, const struct R_GameStateCreateInfo* pCreateInfo);

/**
 * @brief Cleanup the game state
 *
 * This function destroys the Vulkan pipeline context and releases
 * all game resources. It should be called when shutting down the game.
 *
 * @param pState Pointer to game state to cleanup
 */
R_GAME_CVULKAN_API void R_GameState_Cleanup (struct R_GameState* pState);

/**
 * @brief Check if the game state is initialized
 *
 * @param pState Pointer to game state
 * @return 1 if initialized, 0 otherwise
 */
R_GAME_CVULKAN_API int R_GameState_IsInitialized (const struct R_GameState* pState);

/**
 * @brief Check if running in headless mode
 *
 * @param pState Pointer to game state
 * @return 1 if headless, 0 otherwise
 */
R_GAME_CVULKAN_API int R_GameState_IsHeadless (const struct R_GameState* pState);

/**
 * @brief Get the Vulkan pipeline context
 *
 * @param pState Pointer to game state
 * @return Pointer to Vulkan pipeline context, or NULL if not initialized
 */
R_GAME_CVULKAN_API struct R_GameCVulkan_PipelineContext*
R_GameState_GetVulkanContext (struct R_GameState* pState);
