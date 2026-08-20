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
#include "rlgame.base/game/game_cvulkan_platform.h"

/**
 * @brief Configuration parameters for pipeline context creation
 */
struct R_GameCVulkan_PipelineContextCreateInfo
{
                const struct R_CSTL_String* pApplicationName;
#if defined(R_CVULKAN_PLATFORM_WINDOWS)
                HINSTANCE hInstance;
                HWND      hWnd;
#elif defined(R_CVULKAN_PLATFORM_LINUX)
                Display* pDisplay;
                Window   window;
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
struct R_GameCVulkan_PipelineContext
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
                R_CVULKAN_DEBUG_FIELD
};

/**
 * @brief Initialize the Vulkan pipeline context
 *
 * @param pContext Pointer to the pipeline context to initialize
 * @param surface Vulkan surface (VK_NULL_HANDLE in headless mode)
 * @return CVULKAN_API R_CVULKAN_OK on success, error code otherwise
 */
R_GAME_CVULKAN_API enum R_CVulkan_Error R_GameCVulkan_NewPipelineContext (
    struct R_GameCVulkan_PipelineContext*                 pContext,
    const struct R_GameCVulkan_PipelineContextCreateInfo* pCreateInfo);

/**
 * @brief Deletes the Vulkan pipeline context
 *
 * @param pContext Pointer to the pipeline context to delete
 */
R_GAME_CVULKAN_API void R_GameCVulkan_PipelineContextDelete (struct R_GameCVulkan_PipelineContext* pContext);

/**
 * @brief Get the graphics queue
 *
 * @param pContext Pointer to the pipeline context
 * @return Pointer to the graphics queue
 */
R_GAME_CVULKAN_API struct R_CVulkan_Queue*
R_GameCVulkan_PipelineContextGetGraphicsQueue (struct R_GameCVulkan_PipelineContext* pContext);

/**
 * @brief Get the compute queue
 *
 * @param pContext Pointer to the pipeline context
 * @return Pointer to the compute queue
 */
R_GAME_CVULKAN_API struct R_CVulkan_Queue*
R_GameCVulkan_PipelineContextGetComputeQueue (struct R_GameCVulkan_PipelineContext* pContext);

/**
 * @brief Get the transfer queue
 *
 * @param pContext Pointer to the pipeline context
 * @return Pointer to the transfer queue
 */
R_GAME_CVULKAN_API struct R_CVulkan_Queue*
R_GameCVulkan_PipelineContextGetTransferQueue (struct R_GameCVulkan_PipelineContext* pContext);

/**
 * @brief Get the present queue
 *
 * @param pContext Pointer to the pipeline context
 * @return Pointer to the present queue
 */
R_GAME_CVULKAN_API struct R_CVulkan_Queue*
R_GameCVulkan_PipelineContextGetPresentQueue (struct R_GameCVulkan_PipelineContext* pContext);

/**
 * @brief Get the graphics command pool
 *
 * @param pContext Pointer to the pipeline context
 * @return Pointer to the graphics command pool
 */
R_GAME_CVULKAN_API struct R_CVulkan_CommandPool*
R_GameCVulkan_PipelineContextGetGraphicsCommandPool (struct R_GameCVulkan_PipelineContext* pContext);

/**
 * @brief Get the compute command pool
 *
 * @param pContext Pointer to the pipeline context
 * @return Pointer to the compute command pool
 */
R_GAME_CVULKAN_API struct R_CVulkan_CommandPool*
R_GameCVulkan_PipelineContextGetComputeCommandPool (struct R_GameCVulkan_PipelineContext* pContext);

/**
 * @brief Get the transfer command pool
 *
 * @param pContext Pointer to the pipeline context
 * @return Pointer to the transfer command pool
 */
R_GAME_CVULKAN_API struct R_CVulkan_CommandPool*
R_GameCVulkan_PipelineContextGetTransferCommandPool (struct R_GameCVulkan_PipelineContext* pContext);

/**
 * @brief Get the device
 *
 * @param pContext Pointer to the pipeline context
 * @return Pointer to the device
 */
R_GAME_CVULKAN_API struct R_CVulkan_Device*
R_GameCVulkan_PipelineContextGetDevice (struct R_GameCVulkan_PipelineContext* pContext);

/**
 * @brief Check if the context is initialized
 *
 * @param pContext Pointer to the pipeline context
 * @return 1 if initialized, 0 otherwise
 */
R_GAME_CVULKAN_API int
R_GameCVulkan_PipelineContextIsInitialized (const struct R_GameCVulkan_PipelineContext* pContext);
