#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>

#include "rlgame.base/cvulkan/cvulkan_platform.h"
#include "rlgame.base/cvulkan/cvulkan_instance.h"
#include "rlgame.base/cvulkan/cvulkan_device.h"
#include "rlgame.base/cvulkan/cvulkan_queue.h"
#include "rlgame.base/cvulkan/cvulkan_command_pool.h"
#include "rlgame.base/cvulkan/cvulkan_command_buffer.h"
#include "rlgame.base/cvulkan/cvulkan_render_pass.h"
#include "rlgame.base/cvulkan/cvulkan_framebuffer.h"
#include "rlgame.base/cvulkan/cvulkan_semaphore.h"
#include "rlgame.base/cvulkan/cvulkan_surface.h"
#include "rlgame.base/cvulkan/cvulkan_fence.h"
#include "rlgame.base/cvulkan/cvulkan_swapchain.h"
#include "rlgame.base/cstl/cstl_string.h"
#include "rlgame.base/game/game_platform.h"

#if defined(R_CVULKAN_PLATFORM_LINUX)
#include <wayland-client.h>
#include <X11/Xlib.h>
#include <xcb/xcb.h>
#endif

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
 * @brief Settingsuration parameters for pipeline context creation
 */
struct r_game_pipeline_context_create_info
{
        const char* pApplicationName;
        int         windowWidth;
        int         windowHeight;
#if defined(R_CVULKAN_PLATFORM_WINDOWS)
        HINSTANCE hInstance;
        HWND      hWnd;
#elif defined(R_CVULKAN_PLATFORM_LINUX)
        enum r_game_linux_backend linuxBackend;
        struct wl_display*        pDisplay;
        struct wl_surface*        pSurface;
        Display*                  pX11Display;
        Window                    x11Window;
        xcb_connection_t*         pXCBConnection;
        xcb_window_t              xcbWindow;
#elif defined(R_CVULKAN_PLATFORM_ANDROID)
        ANativeWindow* pWindow;
#elif defined(R_CVULKAN_PLATFORM_MACOS)
        void* pNSWindow;
#endif
};

/**
 * @brief Vulkan pipeline context for the game
 *
 * This structure holds all the Vulkan objects needed for rendering
 * and compute operations.
 */
struct r_game_pipeline_context
{
        struct R_CVulkan_Instance     instance;
        struct R_CVulkan_Device       device;
        struct R_CVulkan_Queue        graphicsQueue;
        struct R_CVulkan_Queue        computeQueue;
        struct R_CVulkan_Queue        transferQueue;
        struct R_CVulkan_Queue        presentQueue;
        struct R_CVulkan_Surface*     pSurface;
        struct R_CVulkan_CommandPool  graphicsCommandPool;
        struct R_CVulkan_CommandPool  computeCommandPool;
        struct R_CVulkan_CommandPool  transferCommandPool;
        struct R_CVulkan_Semaphore    imageAvailableSemaphore;
        struct R_CVulkan_Semaphore    renderFinishedSemaphore;
        struct R_CVulkan_Fence        inFlightFence;
        struct R_CVulkan_Swapchain    swapchain;
        struct R_CVulkan_RenderPass   renderPass;
        struct R_CVulkan_Framebuffer* pFramebuffers;
        uint32_t                      framebufferCount;
        uint32_t                      currentFrameIndex;
};

/**
 * @brief Initialize the Vulkan pipeline context
 *
 * @param pContext Pointer to the pipeline context to initialize
 * @param pCreateInfo create info
 * @return CVULKAN_API R_CVULKAN_OK on success, error code otherwise
 */
R_GAME_API enum r_game_error r_game_new_pipeline_context (
    struct r_game_pipeline_context*                   pContext,
    const struct r_game_pipeline_context_create_info* pCreateInfo);

/**
 * @brief Deletes the Vulkan pipeline context
 *
 * @param pContext Pointer to the pipeline context to delete
 */
R_GAME_API void r_game_pipeline_context_delete (struct r_game_pipeline_context* pContext);

/**
 * @brief Get the graphics queue
 *
 * @param pContext Pointer to the pipeline context
 * @return Pointer to the graphics queue
 */
R_GAME_API struct R_CVulkan_Queue*
r_game_pipeline_context_get_graphics_queue (struct r_game_pipeline_context* pContext);

/**
 * @brief Get the compute queue
 *
 * @param pContext Pointer to the pipeline context
 * @return Pointer to the compute queue
 */
R_GAME_API struct R_CVulkan_Queue*
r_game_pipeline_context_get_compute_queue (struct r_game_pipeline_context* pContext);

/**
 * @brief Get the transfer queue
 *
 * @param pContext Pointer to the pipeline context
 * @return Pointer to the transfer queue
 */
R_GAME_API struct R_CVulkan_Queue*
r_game_pipeline_context_get_transfer_queue (struct r_game_pipeline_context* pContext);

/**
 * @brief Get the present queue
 *
 * @param pContext Pointer to the pipeline context
 * @return Pointer to the present queue
 */
R_GAME_API struct R_CVulkan_Queue*
r_game_pipeline_context_get_present_queue (struct r_game_pipeline_context* pContext);

/**
 * @brief Get the graphics command pool
 *
 * @param pContext Pointer to the pipeline context
 * @return Pointer to the graphics command pool
 */
R_GAME_API struct R_CVulkan_CommandPool*
r_game_pipeline_context_get_graphics_command_pool (struct r_game_pipeline_context* pContext);

/**
 * @brief Get the compute command pool
 *
 * @param pContext Pointer to the pipeline context
 * @return Pointer to the compute command pool
 */
R_GAME_API struct R_CVulkan_CommandPool*
r_game_pipeline_context_get_compute_command_pool (struct r_game_pipeline_context* pContext);

/**
 * @brief Get the transfer command pool
 *
 * @param pContext Pointer to the pipeline context
 * @return Pointer to the transfer command pool
 */
R_GAME_API struct R_CVulkan_CommandPool*
r_game_pipeline_context_get_transfer_command_pool (struct r_game_pipeline_context* pContext);

/**
 * @brief Get the device
 *
 * @param pContext Pointer to the pipeline context
 * @return Pointer to the device
 */
R_GAME_API struct R_CVulkan_Device*
r_game_pipeline_context_get_device (struct r_game_pipeline_context* pContext);

R_GAME_API struct R_CVulkan_Semaphore*
r_game_pipeline_context_get_image_available_semaphore (struct r_game_pipeline_context* pContext);

R_GAME_API struct R_CVulkan_Semaphore*
r_game_pipeline_context_get_render_finished_semaphore (struct r_game_pipeline_context* pContext);

R_GAME_API struct R_CVulkan_Fence*
r_game_pipeline_context_get_in_flight_fence (struct r_game_pipeline_context* pContext);

R_GAME_API uint32_t*
r_game_pipeline_context_get_current_frame_index (struct r_game_pipeline_context* pContext);
