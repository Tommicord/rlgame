#include "rlgame.base/game/game_cvulkan_pipeline.h"
#include "rlgame.base/game/game_platform.h"
#include "rlgame.base/cvulkan/cvulkan_instance.h"
#include "rlgame.base/cvulkan/cvulkan_device.h"
#include "rlgame.base/cvulkan/cvulkan_queue.h"
#include "rlgame.base/cvulkan/cvulkan_command_pool.h"
#include "rlgame.base/cvulkan/cvulkan_semaphore.h"
#include "rlgame.base/cvulkan/cvulkan_fence.h"
#include "rlgame.base/cvulkan/cvulkan_swapchain.h"
#include "rlgame.base/cvulkan/cvulkan_render_pass.h"
#include "rlgame.base/cvulkan/cvulkan_framebuffer.h"
#include "rlgame.base/cstl/cstl_log.h"
#include "rlgame.base/cstl/cstl_string.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"
#include "rlgame.base/cstl/cstl_trace.h"

#include <string.h>
#include <stdint.h>
#include <inttypes.h>

#if defined(R_CVULKAN_PLATFORM_WINDOWS)
#include <windows.h>
#elif defined(R_CVULKAN_PLATFORM_LINUX)
#include <X11/Xlib.h>
#elif defined(R_CVULKAN_PLATFORM_ANDROID)
#include <android/native_window.h>
#endif

static VkExtent2D
R_GameCVulkan_GetWindowExtent (const struct R_GameCVulkan_PipelineContextCreateInfo* pCreateInfo)
{
        VkExtent2D extent = {0, 0};

#if defined(R_CVULKAN_PLATFORM_WINDOWS)
        if (pCreateInfo->hWnd != NULL)
        {
                RECT rect;
                if (GetClientRect (pCreateInfo->hWnd, &rect))
                {
                        extent.width = rect.right - rect.left;
                        extent.height = rect.bottom - rect.top;
                }
        }
#elif defined(R_CVULKAN_PLATFORM_LINUX)
        if (pCreateInfo->pDisplay != NULL && pCreateInfo->window != 0)
        {
                XWindowAttributes windowAttributes;
                if (XGetWindowAttributes (pCreateInfo->pDisplay, pCreateInfo->window, &windowAttributes))
                {
                        extent.width = windowAttributes.width;
                        extent.height = windowAttributes.height;
                }
        }
#elif defined(R_CVULKAN_PLATFORM_ANDROID)
        if (pCreateInfo->pWindow != NULL)
        {
                extent.width = ANativeWindow_getWidth (pCreateInfo->pWindow);
                extent.height = ANativeWindow_getHeight (pCreateInfo->pWindow);
        }
#elif defined(R_CVULKAN_PLATFORM_MACOS)
        // macOS window dimensions would need to be retrieved via NSWindow
        // This requires Objective-C runtime integration
#endif

        return extent;
}

static enum R_CVulkanError
R_GameCVulkan_InitializeQueues (
    struct R_GameCVulkan_PipelineContext* pContext,
    struct R_CVulkan_Surface*             pSurface)
{
        struct R_CVulkan_QueueFamilyIndices indices;
        enum R_CVulkanError                 err;
        VkSurfaceKHR                        surface = VK_NULL_HANDLE;

#if defined(R_GAME_DEBUG)
        if (pSurface)
        {
#endif
                surface = R_CVulkan_SurfaceGetHandle (pSurface);
#if defined(R_GAME_DEBUG)
        }
#endif

        err = R_CVulkan_DeviceFindQueueFamilies (
            R_CVulkan_DeviceGetPhysicalDevice (&pContext->device),
            surface,
            &indices);
        if (err != R_CVULKAN_OK)
        {
                return err;
        }
        err = R_CVulkan_NewQueue (&pContext->graphicsQueue, &pContext->device, indices.graphicsFamily, 0);
        if (err != R_CVULKAN_OK)
        {
                return err;
        }
        err = R_CVulkan_NewQueue (&pContext->computeQueue, &pContext->device, indices.computeFamily, 0);
        if (err != R_CVULKAN_OK)
        {
                goto r_cleanup_queue3;
        }
        err = R_CVulkan_NewQueue (&pContext->transferQueue, &pContext->device, indices.transferFamily, 0);
        if (err != R_CVULKAN_OK)
        {
                goto r_cleanup_queue2;
        }
#if !defined(R_CVULKAN_HEADLESS)
        err = R_CVulkan_NewQueue (&pContext->presentQueue, &pContext->device, indices.presentFamily, 0);
        if (err != R_CVULKAN_OK)
        {
                R_CSTL_LOG_ERROR ("Failed to create present queue: %s", R_CVulkanErrorToString (err));
                goto r_cleanup_queue1;
        }
#endif
        return R_CVULKAN_OK;
r_cleanup_queue1:
#if !defined(R_CVULKAN_HEADLESS)
        R_CVulkan_DeleteQueue (&pContext->presentQueue);
#endif
r_cleanup_queue2:
        R_CVulkan_DeleteQueue (&pContext->transferQueue);
r_cleanup_queue3:
        R_CVulkan_DeleteQueue (&pContext->computeQueue);
r_cleanup_queue4:
        R_CVulkan_DeleteQueue (&pContext->graphicsQueue);
        return err;
}

static enum R_CVulkanError
R_GameCVulkan_InitializeCommandPools (
    struct R_GameCVulkan_PipelineContext* pContext,
    struct R_CVulkan_Surface*             pSurface)
{
        struct R_CVulkan_QueueFamilyIndices indices;
        enum R_CVulkanError                 err;
        VkSurfaceKHR                        surface = VK_NULL_HANDLE;

#if defined(R_GAME_DEBUG)
        if (pSurface)
        {
#endif
                surface = R_CVulkan_SurfaceGetHandle (pSurface);
#if defined(R_GAME_DEBUG)
        }
#endif
        err = R_CVulkan_DeviceFindQueueFamilies (
            R_CVulkan_DeviceGetPhysicalDevice (&pContext->device),
            surface,
            &indices);
        if (err != R_CVULKAN_OK)
        {
                R_CSTL_LOG_ERROR ("Failed to find queue families: %s", R_CVulkanErrorToString (err));
                return err;
        }
        err = R_CVulkan_NewCommandPool (
            &pContext->graphicsCommandPool,
            &pContext->device,
            indices.graphicsFamily,
            0);
        if (err != R_CVULKAN_OK)
        {
                R_CSTL_LOG_ERROR ("Failed to create graphics command pool: %s", R_CVulkanErrorToString (err));
                return err;
        }

        err = R_CVulkan_NewCommandPool (
            &pContext->computeCommandPool,
            &pContext->device,
            indices.computeFamily,
            0);
        if (err != R_CVULKAN_OK)
        {
                R_CSTL_LOG_ERROR ("Failed to create compute command pool: %s", R_CVulkanErrorToString (err));
                return err;
        }

        err = R_CVulkan_NewCommandPool (
            &pContext->transferCommandPool,
            &pContext->device,
            indices.transferFamily,
            0);
        if (err != R_CVULKAN_OK)
        {
                R_CSTL_LOG_ERROR ("Failed to create transfer command pool: %s", R_CVulkanErrorToString (err));
                return err;
        }

        return R_CVULKAN_OK;
}

static enum R_CVulkanError
R_GameCVulkan_InitializeSyncPrimitives (struct R_GameCVulkan_PipelineContext* pContext)
{
        enum R_CVulkanError err;

#if !defined(R_CVULKAN_HEADLESS)
        err = R_CVulkan_NewSemaphore (&pContext->imageAvailableSemaphore, &pContext->device, 0, 0);
        if (err != R_CVULKAN_OK)
        {
                R_CSTL_LOG_ERROR (
                    "Failed to create image available semaphore: %s",
                    R_CVulkanErrorToString (err));
                return err;
        }

        err = R_CVulkan_NewSemaphore (&pContext->renderFinishedSemaphore, &pContext->device, 0, 0);
        if (err != R_CVULKAN_OK)
        {
                R_CSTL_LOG_ERROR (
                    "Failed to create render finished semaphore: %s",
                    R_CVulkanErrorToString (err));
                return err;
        }

        err = R_CVulkan_NewFence (&pContext->inFlightFence, &pContext->device, 1);
        if (err != R_CVULKAN_OK)
        {
                R_CSTL_LOG_ERROR ("Failed to create in-flight fence: %s", R_CVulkanErrorToString (err));
                return err;
        }
#endif
        return R_CVULKAN_OK;
}

static void
R_GameCVulkan_CleanupPartialInitialization (struct R_GameCVulkan_PipelineContext* pContext)
{
        R_CVulkan_DeleteQueue (&pContext->graphicsQueue);
        R_CVulkan_DeleteQueue (&pContext->computeQueue);
        R_CVulkan_DeleteQueue (&pContext->transferQueue);
#if !defined(R_CVULKAN_HEADLESS)
        R_CVulkan_DeleteQueue (&pContext->presentQueue);
        R_CVulkan_DeleteSemaphore (&pContext->imageAvailableSemaphore);
        R_CVulkan_DeleteSemaphore (&pContext->renderFinishedSemaphore);
        R_CVulkan_DeleteFence (&pContext->inFlightFence);
        R_CVulkan_DeleteSwapchain (&pContext->swapchain);
#endif
        R_CVulkan_DeleteCommandPool (&pContext->graphicsCommandPool);
        R_CVulkan_DeleteCommandPool (&pContext->computeCommandPool);
        R_CVulkan_DeleteCommandPool (&pContext->transferCommandPool);
        if (pContext->pSurface != NULL)
        {
                R_CVulkan_DeleteSurface (pContext->pSurface);
                R_CSTL_HeapFree (pContext->pSurface);
                pContext->pSurface = NULL;
        }
        R_CVulkan_DeleteDevice (&pContext->device);
}

R_GAME_API enum R_GameError
R_GameCVulkan_NewPipelineContext (
    struct R_GameCVulkan_PipelineContext*                 pContext,
    const struct R_GameCVulkan_PipelineContextCreateInfo* pCreateInfo)
{
        R_CVULKAN_ASSERT (pContext != NULL);
        R_CVULKAN_ASSERT (pCreateInfo != NULL);

#if defined(R_CVULKAN_DEBUG)
        if (!pContext || !pCreateInfo)
        {
                return R_GAME_ERROR_NULL_POINTER;
        }
#endif

#if defined(R_CVULKAN_DEBUG)
        pContext->booted = false;
#endif
        struct R_CVulkan_InstanceCreateInfo instanceCreateInfo = {0};
        instanceCreateInfo.pApplicationName = pCreateInfo->pApplicationName;
        instanceCreateInfo.enableValidationLayers = true;
        instanceCreateInfo.enableHeadlessMode = false;
        {
                enum R_CVulkanError err = R_CVulkan_NewInstance (&pContext->instance, &instanceCreateInfo);
                if (err != R_CVULKAN_OK)
                {
                        R_CSTL_LOG_ERROR (
                            "Failed to create Vulkan instance: %s",
                            R_CVulkanErrorToString (err));
                        return R_GAME_ERROR_INITIALIZATION_FAILED;
                }
        }
        enum R_GameError err;
        pContext->pSurface = (struct R_CVulkan_Surface*)R_CSTL_HeapAlloc (sizeof (struct R_CVulkan_Surface));
        if (pContext->pSurface == NULL)
        {
                R_CSTL_LOG_ERROR ("Failed to allocate memory for surface");
                err = R_GAME_ERROR_OUT_OF_MEMORY;
                goto r_cleanup;
        }
        memset (pContext->pSurface, 0, sizeof (struct R_CVulkan_Surface));

        struct R_CVulkan_SurfaceCreateInfo surfaceCreateInfo = {0};
        surfaceCreateInfo.pInstance = &pContext->instance;

#if defined(R_CVULKAN_PLATFORM_WINDOWS)
        surfaceCreateInfo.hInstance = pCreateInfo->hInstance;
        surfaceCreateInfo.hWnd = pCreateInfo->hWnd;
#elif defined(R_CVULKAN_PLATFORM_LINUX)
        surfaceCreateInfo.pDisplay = pCreateInfo->pDisplay;
        surfaceCreateInfo.window = pCreateInfo->window;
#elif defined(R_CVULKAN_PLATFORM_ANDROID)
        surfaceCreateInfo.pWindow = pCreateInfo->pWindow;
#elif defined(R_CVULKAN_PLATFORM_MACOS)
        surfaceCreateInfo.pNSWindow = pCreateInfo->pNSWindow;
#endif

#if !defined(R_CVULKAN_HEADLESS)
        {
                enum R_CVulkanError err = R_CVulkan_NewSurface (pContext->pSurface, &surfaceCreateInfo);
                if (err != R_CVULKAN_OK)
                {
                        R_CSTL_LOG_ERROR (
                            "Failed to create Vulkan surface: %s",
                            R_CVulkanErrorToString (err));
                        goto r_cleanup;
                }
        }
#endif
        struct R_CVulkan_DeviceCreateInfo deviceCreateInfo = {0};
        deviceCreateInfo.pInstance = &pContext->instance;
        deviceCreateInfo.pSurface = pContext->pSurface;
        {
                enum R_CVulkanError err = R_CVulkan_NewDevice (&pContext->device, &deviceCreateInfo);
                if (err != R_CVULKAN_OK)
                {
                        R_CSTL_LOG_ERROR ("Failed to create Vulkan device: %s", R_CVulkanErrorToString (err));
                        goto r_cleanup;
                }
        }

        {
                enum R_CVulkanError err;
                err = R_GameCVulkan_InitializeQueues (pContext, pContext->pSurface);
                if (err != R_CVULKAN_OK)
                {
                        goto r_cleanup;
                }
                err = R_GameCVulkan_InitializeCommandPools (pContext, pContext->pSurface);
                if (err != R_CVULKAN_OK)
                {
                        goto r_cleanup;
                }
                err = R_GameCVulkan_InitializeSyncPrimitives (pContext);
                if (err != R_CVULKAN_OK)
                {
                        goto r_cleanup;
                }
        }
#if !defined(R_CVULKAN_HEADLESS)
        VkExtent2D windowExtent = R_GameCVulkan_GetWindowExtent (pCreateInfo);

        struct R_CVulkan_SwapchainCreateInfo swapchainCreateInfo = {0};
        swapchainCreateInfo.pDevice = &pContext->device;
        swapchainCreateInfo.pSurface = pContext->pSurface;
        swapchainCreateInfo.imageCount = 0;
        swapchainCreateInfo.surfaceFormat.format = VK_FORMAT_UNDEFINED;
        swapchainCreateInfo.surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        swapchainCreateInfo.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
        swapchainCreateInfo.extent = windowExtent;
        swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchainCreateInfo.arrayLayers = 1;
        swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainCreateInfo.clipped = VK_TRUE;
        swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
        {
                enum R_CVulkanError err = R_CVulkan_NewSwapchain (&pContext->swapchain, &swapchainCreateInfo);
                if (err != R_CVULKAN_OK)
                {
                        R_CSTL_LOG_ERROR ("Failed to create swapchain: %s", R_CVulkanErrorToString (err));
                        goto r_cleanup;
                }
        }
#endif
        pContext->pFramebuffers = NULL;
        pContext->framebufferCount = 0;
        pContext->currentFrameIndex = 0;

#if defined(R_CVULKAN_DEBUG)
        pContext->booted = true;
#endif
        return R_GAME_OK;

r_cleanup:
        if (pContext->pSurface != NULL)
        {
                R_CVulkan_DeleteSurface (pContext->pSurface);
                R_CSTL_HeapFree (pContext->pSurface);
                pContext->pSurface = NULL;
        }
        R_CVulkan_DeleteDevice (&pContext->device);
        R_CVulkan_DeleteInstance (&pContext->instance);
        return err;
}

R_GAME_API void
R_GameCVulkan_PipelineContextDelete (struct R_GameCVulkan_PipelineContext* pContext)
{
        R_CSTL_TRACE_SCOPE ();

        R_CVULKAN_ASSERT (pContext);
#if defined(R_CVULKAN_DEBUG)
        if (!pContext)
        {
                return;
        }
#endif
        if (pContext->pFramebuffers != NULL)
        {
                for (uint32_t i = 0; i < pContext->framebufferCount; ++i)
                {
                        R_CVulkan_DeleteFramebuffer (&pContext->pFramebuffers[i]);
                }
                R_CSTL_HeapFree (pContext->pFramebuffers);
                pContext->pFramebuffers = NULL;
        }
        R_CVulkan_DeleteRenderPass (&pContext->renderPass);

        R_CVulkan_DeleteQueue (&pContext->graphicsQueue);
        R_CVulkan_DeleteQueue (&pContext->computeQueue);
        R_CVulkan_DeleteQueue (&pContext->transferQueue);

#if !defined(R_CVULKAN_HEADLESS)
        R_CVulkan_DeleteQueue (&pContext->presentQueue);
        R_CVulkan_DeleteSemaphore (&pContext->imageAvailableSemaphore);
        R_CVulkan_DeleteSemaphore (&pContext->renderFinishedSemaphore);
        R_CVulkan_DeleteFence (&pContext->inFlightFence);
        R_CVulkan_DeleteSwapchain (&pContext->swapchain);
#endif

        R_CVulkan_DeleteCommandPool (&pContext->graphicsCommandPool);
        R_CVulkan_DeleteCommandPool (&pContext->computeCommandPool);
        R_CVulkan_DeleteCommandPool (&pContext->transferCommandPool);

        if (pContext->pSurface != NULL)
        {
                R_CVulkan_DeleteSurface (pContext->pSurface);
                R_CSTL_HeapFree (pContext->pSurface);
                pContext->pSurface = NULL;
        }

        R_CVulkan_DeleteDevice (&pContext->device);
        R_CVulkan_DeleteInstance (&pContext->instance);

#if defined(R_CVULKAN_DEBUG)
        pContext->booted = false;
#endif
}

R_GAME_API struct R_CVulkan_Queue*
R_GameCVulkan_PipelineContextGetGraphicsQueue (struct R_GameCVulkan_PipelineContext* pContext)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pContext != NULL);
#endif
        return &pContext->graphicsQueue;
}

R_GAME_API struct R_CVulkan_Queue*
R_GameCVulkan_PipelineContextGetComputeQueue (struct R_GameCVulkan_PipelineContext* pContext)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pContext != NULL);
#endif
        return &pContext->computeQueue;
}

R_GAME_API struct R_CVulkan_Queue*
R_GameCVulkan_PipelineContextGetTransferQueue (struct R_GameCVulkan_PipelineContext* pContext)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pContext != NULL);
#endif
        return &pContext->transferQueue;
}

R_GAME_API struct R_CVulkan_Queue*
R_GameCVulkan_PipelineContextGetPresentQueue (struct R_GameCVulkan_PipelineContext* pContext)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pContext != NULL);
#endif
        return &pContext->presentQueue;
}

R_GAME_API struct R_CVulkan_CommandPool*
R_GameCVulkan_PipelineContextGetGraphicsCommandPool (struct R_GameCVulkan_PipelineContext* pContext)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pContext != NULL);
#endif
        return &pContext->graphicsCommandPool;
}

R_GAME_API struct R_CVulkan_CommandPool*
R_GameCVulkan_PipelineContextGetComputeCommandPool (struct R_GameCVulkan_PipelineContext* pContext)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pContext != NULL);
#endif
        return &pContext->computeCommandPool;
}

R_GAME_API struct R_CVulkan_CommandPool*
R_GameCVulkan_PipelineContextGetTransferCommandPool (struct R_GameCVulkan_PipelineContext* pContext)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pContext != NULL);
#endif
        return &pContext->transferCommandPool;
}

R_GAME_API struct R_CVulkan_Device*
R_GameCVulkan_PipelineContextGetDevice (struct R_GameCVulkan_PipelineContext* pContext)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pContext != NULL);
#endif
        return &pContext->device;
}

R_GAME_API int
R_GameCVulkan_PipelineContextIsInitialized (const struct R_GameCVulkan_PipelineContext* pContext)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pContext != NULL);
        return pContext->booted;
#else
        (void)pContext;
        return 1;
#endif
}

R_GAME_API struct R_CVulkan_Semaphore*
R_GameCVulkan_PipelineContextGetImageAvailableSemaphore (struct R_GameCVulkan_PipelineContext* pContext)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pContext != NULL);
#endif
        return &pContext->imageAvailableSemaphore;
}

R_GAME_API struct R_CVulkan_Semaphore*
R_GameCVulkan_PipelineContextGetRenderFinishedSemaphore (struct R_GameCVulkan_PipelineContext* pContext)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pContext != NULL);
#endif
        return &pContext->renderFinishedSemaphore;
}

R_GAME_API struct R_CVulkan_Fence*
R_GameCVulkan_PipelineContextGetInFlightFence (struct R_GameCVulkan_PipelineContext* pContext)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pContext != NULL);
#endif
        return &pContext->inFlightFence;
}

R_GAME_API uint32_t*
R_GameCVulkan_PipelineContextGetCurrentFrameIndex (struct R_GameCVulkan_PipelineContext* pContext)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pContext != NULL);
#endif
        return &pContext->currentFrameIndex;
}
