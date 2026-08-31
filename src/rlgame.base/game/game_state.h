#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <vulkan/vulkan.h>

#include "rlgame.base/cvulkan/cvulkan_platform.h"
#include "rlgame.base/game/game_pipeline.h"
#include "rlgame.base/game/game_platform.h"
#include "rlgame.base/game/game_renderer_subsystem.h"

#if defined(R_CVULKAN_PLATFORM_LINUX)
#include <wayland-client.h>
#endif

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
 * rendering pipeline, renderer manager, and game-specific data. It is designed to be
 * managed by the main game loop with clear initialization/cleanup phases.
 */
struct r_game_state
{
        struct r_game_pipeline_context context; /**< Vulkan rendering pipeline context */
        struct r_game_renderer_manager* pRendererManager; /**< Multi-renderer manager */
};

/**
 * @brief Linux window backend type
 */
enum r_game_linux_backend
{
    R_GAME_LINUX_BACKEND_WAYLAND = 0,
    R_GAME_LINUX_BACKEND_X11 = 1,
    R_GAME_LINUX_BACKEND_XCB = 2
};

/**
 * @brief Settingsuration parameters for game state initialization
 */
struct r_game_state_create_info
{
        const char* pApplicationName; /**< Application name */
        int         windowWidth; /**< Initial window width */
        int         windowHeight; /**< Initial window height */
#if defined(R_CVULKAN_PLATFORM_WINDOWS)
        HINSTANCE hInstance; /**< Windows instance handle (required if not headless) */
        HWND      hWnd; /**< Windows window handle (required if not headless) */
#elif defined(R_CVULKAN_PLATFORM_LINUX)
        enum r_game_linux_backend linuxBackend; /**< Linux window backend */
        struct wl_display*       pDisplay; /**< Wayland display connection (required if not headless) */
        struct wl_surface*       pSurface; /**< Wayland surface (required if not headless) */
        Display*                 pX11Display; /**< X11 display connection (required if not headless) */
        Window                   x11Window; /**< X11 window handle (required if not headless) */
        xcb_connection_t*        pXCBConnection; /**< XCB connection (required if not headless) */
        xcb_window_t             xcbWindow; /**< XCB window handle (required if not headless) */
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
R_GAME_API enum R_CVulkan_Error
r_game_state_Initialize (struct r_game_state* pState, const struct r_game_state_create_info* pCreateInfo);

/**
 * @brief Cleanup the game state
 *
 * This function destroys the Vulkan pipeline context and releases
 * all game resources. It should be called when shutting down the game.
 *
 * @param pState Pointer to game state to cleanup
 */
R_GAME_API void r_game_state_Cleanup (struct r_game_state* pState);

/**
 * @brief Get the Vulkan pipeline context
 *
 * @param pState Pointer to game state
 * @return Pointer to Vulkan pipeline context, or NULL if not initialized
 */
R_GAME_API struct r_game_pipeline_context* r_game_state_get_vulkan_context (struct r_game_state* pState);

/**
 * @brief Get the renderer manager
 *
 * @param pState Pointer to game state
 * @return Pointer to renderer manager, or NULL if not initialized
 */
R_GAME_API struct r_game_renderer_manager* r_game_state_get_renderer_manager (struct r_game_state* pState);

/**
 * @brief Render a frame using the renderer manager
 *
 * This function composes and presents a frame using all registered
 * renderer subsystems. It should be called each frame in the game loop.
 *
 * @param pState Pointer to game state
 * @return 0 on success, -1 on error
 */
R_GAME_API int r_game_state_render_frame (struct r_game_state* pState);
