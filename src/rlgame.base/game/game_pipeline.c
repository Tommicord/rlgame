#include "rlgame.base/game/game_pipeline.h"
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
#include "rlgame.base/cvulkan/cvulkan_image_view.h"
#include "rlgame.base/cstl/cstl_log.h"
#include "rlgame.base/cstl/cstl_string.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"
#include "rlgame.base/cstl/cstl_trace.h"
#include "rlgame.base/main_platform_handle.h"

#include <string.h>
#include <stdint.h>
#include <inttypes.h>

#if defined(R_CVULKAN_PLATFORM_WINDOWS)
#include <windows.h>
#elif defined(R_CVULKAN_PLATFORM_LINUX)
#include <wayland-client.h>
#include <X11/Xlib.h>
#include <xcb/xcb.h>
#elif defined(R_CVULKAN_PLATFORM_ANDROID)
#include <android/native_window.h>
#endif

static enum R_CVulkan_Error r_game_initialize_sync_primitives (struct r_game_pipeline_context* pContext);

static VkExtent2D
r_game_get_window_extent (const struct r_game_pipeline_context_create_info* pCreateInfo)
{
    VkExtent2D extent = {0, 0};

#if defined(R_CVULKAN_PLATFORM_WINDOWS)
    if (pCreateInfo->hWnd)
    {
        RECT rect;
        if (GetClientRect (pCreateInfo->hWnd, &rect))
        {
            extent.width = rect.right - rect.left;
            extent.height = rect.bottom - rect.top;
        }
    }
#elif defined(R_CVULKAN_PLATFORM_LINUX)
    // Use the window dimensions from create info
    if (pCreateInfo->windowWidth > 0 && pCreateInfo->windowHeight > 0)
    {
        extent.width = pCreateInfo->windowWidth;
        extent.height = pCreateInfo->windowHeight;
    }
#elif defined(R_CVULKAN_PLATFORM_ANDROID)
    if (pCreateInfo->pWindow)
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

static enum R_CVulkan_Error
r_game_initialize_render_pass (struct r_game_pipeline_context* pContext)
{
    R_CSTL_TRACE_SCOPE ();

    R_CSTL_LOG_INFO ("r_game_initialize_render_pass: Starting render pass initialization");
    VkFormat swapchainFormat = R_CVulkan_SwapchainGetImageFormat (&pContext->swapchain);
    R_CSTL_LOG_DEBUG ("r_game_initialize_render_pass: Swapchain format: %s", R_CVulkan_FormatToString(swapchainFormat));

    VkAttachmentDescription colorAttachment = {0};
    colorAttachment.format = swapchainFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {0};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {0};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    R_CSTL_LOG_DEBUG ("r_game_initialize_render_pass: Subpass configured");
    R_CSTL_LOG_DEBUG ("  PipelineBindPoint: GRAPHICS");
    R_CSTL_LOG_DEBUG ("  ColorAttachmentCount: 1");

    VkSubpassDependency dependency = {0};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    R_CSTL_LOG_DEBUG ("r_game_initialize_render_pass: Subpass dependency configured");
    R_CSTL_LOG_DEBUG ("  srcSubpass: EXTERNAL");
    R_CSTL_LOG_DEBUG ("  dstSubpass: 0");
    R_CSTL_LOG_DEBUG ("  srcStageMask: COLOR_ATTACHMENT_OUTPUT_BIT");
    R_CSTL_LOG_DEBUG ("  dstAccessMask: COLOR_ATTACHMENT_WRITE_BIT");

    struct R_CVulkan_RenderPassCreateInfo renderPassCreateInfo = {0};
    renderPassCreateInfo.pDevice = &pContext->device;
    renderPassCreateInfo.pAttachments = &colorAttachment;
    renderPassCreateInfo.attachmentCount = 1;
    renderPassCreateInfo.pSubpasses = &subpass;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pDependencies = &dependency;
    renderPassCreateInfo.dependencyCount = 1;

    enum R_CVulkan_Error err = R_CVulkan_NewRenderPass (&pContext->renderPass, &renderPassCreateInfo);
    if (err != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("r_game_initialize_render_pass: Failed to create render pass");
        R_CSTL_LOG_ERROR ("  Error: %s", R_CVulkan_ErrorToString (err));
        return err;
    }

    R_CSTL_LOG_INFO ("r_game_initialize_render_pass: Render pass created");
    R_CSTL_LOG_INFO ("  Handle: %p", (void*)R_CVulkan_RenderPassGetHandle (&pContext->renderPass));

    return R_CVULKAN_OK;
}

static enum R_CVulkan_Error
r_game_initialize_image_views (
    struct r_game_pipeline_context* pContext,
    VkImage*                       pSwapchainImages,
    uint32_t                       imageCount)
{
    R_CSTL_TRACE_SCOPE ();
    VkFormat swapchainFormat = R_CVulkan_SwapchainGetImageFormat (&pContext->swapchain);
    for (uint32_t i = 0; i < imageCount; ++i)
    {
        struct R_CVulkan_ImageView           imageView = {0};
        struct R_CVulkan_ImageViewCreateInfo imageViewCreateInfo = {0};
        imageViewCreateInfo.pDevice = &pContext->device;
        imageViewCreateInfo.image = pSwapchainImages[i];
        imageViewCreateInfo.format = swapchainFormat;
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;

        enum R_CVulkan_Error imgErr = R_CVulkan_NewImageView (&imageView, &imageViewCreateInfo);
        if (imgErr != R_CVULKAN_OK)
        {
            R_CSTL_LOG_ERROR ("r_game_initialize_image_views: Failed to create image view %u", i);
            R_CSTL_LOG_ERROR ("  Error: %s", R_CVulkan_ErrorToString (imgErr));
            R_CSTL_LOG_ERROR ("  Image handle: %p", (void*)pSwapchainImages[i]);
            R_CSTL_LOG_ERROR ("  Format: %d", swapchainFormat);
            return imgErr;
        }
    }
    R_CSTL_LOG_INFO ("r_game_initialize_image_views: Image views initialized");
    R_CSTL_LOG_INFO ("  Total image views: %u", imageCount);
    return R_CVULKAN_OK;
}

static enum R_CVulkan_Error
r_game_initialize_framebuffers (struct r_game_pipeline_context* pContext)
{
    R_CSTL_TRACE_SCOPE ();

    R_CSTL_LOG_INFO ("r_game_initialize_framebuffers: Starting framebuffer initialization");

    VkDevice       device = R_CVulkan_DeviceGetLogicalDevice (&pContext->device);
    VkSwapchainKHR swapchainHandle = R_CVulkan_SwapchainGetHandle (&pContext->swapchain);
    uint32_t       imageCount = R_CVulkan_SwapchainGetImageCount (&pContext->swapchain);

    R_CSTL_LOG_DEBUG ("r_game_initialize_framebuffers: Swapchain image count: %u", imageCount);
    R_CSTL_LOG_DEBUG ("r_game_initialize_framebuffers: Device handle: %p", (void*)device);
    R_CSTL_LOG_DEBUG ("r_game_initialize_framebuffers: Swapchain handle: %p", (void*)swapchainHandle);

    VkImage* pSwapchainImages = (VkImage*)R_CSTL_HeapAlloc (sizeof (VkImage) * imageCount);
    if (!pSwapchainImages)
    {
        R_CSTL_LOG_ERROR ("r_game_initialize_framebuffers: Failed to allocate swapchain images array");
        R_CSTL_LOG_ERROR ("  Requested size: %zu bytes", sizeof (VkImage) * imageCount);
        R_CSTL_LOG_ERROR ("  Image count: %u", imageCount);
        return R_CVULKAN_ERROR_OUT_OF_MEMORY;
    }

    VkResult result = vkGetSwapchainImagesKHR (device, swapchainHandle, &imageCount, pSwapchainImages);
    if (result != VK_SUCCESS)
    {
        R_CSTL_LOG_ERROR ("r_game_initialize_framebuffers: Failed to get swapchain images");
        R_CSTL_LOG_ERROR ("  Vulkan result: %d", result);
        R_CSTL_HeapFree (pSwapchainImages);
        return R_CVULKAN_ERROR_OUT_OF_MEMORY;
    }

    R_CSTL_LOG_DEBUG ("r_game_initialize_framebuffers: Retrieved %u swapchain images", imageCount);

    pContext->pFramebuffers = (struct R_CVulkan_Framebuffer*)R_CSTL_HeapAlloc (
        sizeof (struct R_CVulkan_Framebuffer) * imageCount);
    if (!pContext->pFramebuffers)
    {
        R_CSTL_LOG_ERROR ("r_game_initialize_framebuffers: Failed to allocate framebuffers array");
        R_CSTL_LOG_ERROR ("  Requested size: %zu bytes", sizeof (struct R_CVulkan_Framebuffer) * imageCount);
        R_CSTL_LOG_ERROR ("  Image count: %u", imageCount);
        R_CSTL_HeapFree (pSwapchainImages);
        return R_CVULKAN_ERROR_OUT_OF_MEMORY;
    }

    pContext->framebufferCount = imageCount;
    R_CSTL_LOG_DEBUG (
        "r_game_initialize_framebuffers: Allocated framebuffer array for %u framebuffers",
        imageCount);

    VkExtent2D swapchainExtent = R_CVulkan_SwapchainGetExtent (&pContext->swapchain);
    R_CSTL_LOG_DEBUG (
        "r_game_initialize_framebuffers: Swapchain extent: %ux%u",
        swapchainExtent.width,
        swapchainExtent.height);

    for (uint32_t i = 0; i < imageCount; ++i)
    {
        struct R_CVulkan_ImageViewCreateInfo imageViewCreateInfo = {0};
        imageViewCreateInfo.pDevice = &pContext->device;
        imageViewCreateInfo.image = pSwapchainImages[i];
        imageViewCreateInfo.format = R_CVulkan_SwapchainGetImageFormat (&pContext->swapchain);
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;

        struct R_CVulkan_ImageView imageView;
        {
            enum R_CVulkan_Error err = R_CVulkan_NewImageView (&imageView, &imageViewCreateInfo);
            if (err != R_CVULKAN_OK)
            {
                R_CSTL_LOG_ERROR ("r_game_initialize_framebuffers: Failed to create image view %u", i);
                R_CSTL_LOG_ERROR ("  Error: %s", R_CVulkan_ErrorToString (err));
                R_CSTL_LOG_ERROR ("  Image handle: %p", (void*)pSwapchainImages[i]);
                R_CSTL_HeapFree (pSwapchainImages);
                return err;
            }
        }
        struct R_CVulkan_FramebufferCreateInfo framebufferCreateInfo = {0};
        framebufferCreateInfo.pDevice = &pContext->device;
        framebufferCreateInfo.pRenderPass = R_CVulkan_RenderPassGetHandle (&pContext->renderPass);

        const VkImageView attachments[] = {R_CVulkan_ImageViewGetHandle (&imageView)};
        if (attachments[0] == VK_NULL_HANDLE)
        {
            R_CSTL_LOG_ERROR ("r_game_initialize_framebuffers: Image view handle is NULL for image %u", i);
            R_CVulkan_DeleteImageView (&imageView);
            R_CSTL_HeapFree (pSwapchainImages);
            R_CSTL_HeapFree (pContext->pFramebuffers);
            return R_CVULKAN_ERROR_FAILED;
        }

        framebufferCreateInfo.pAttachments = attachments;
        framebufferCreateInfo.attachmentCount = 1;
        framebufferCreateInfo.width = swapchainExtent.width;
        framebufferCreateInfo.height = swapchainExtent.height;
        framebufferCreateInfo.layers = 1;
        {
            enum R_CVulkan_Error err
                = R_CVulkan_NewFramebuffer (&pContext->pFramebuffers[i], &framebufferCreateInfo);
            if (err != R_CVULKAN_OK)
            {
                R_CSTL_LOG_ERROR ("r_game_initialize_framebuffers: Failed to create framebuffer %u", i);
                R_CSTL_LOG_ERROR ("  Error: %s", R_CVulkan_ErrorToString (err));
                R_CSTL_LOG_ERROR ("  Width: %u", framebufferCreateInfo.width);
                R_CSTL_LOG_ERROR ("  Height: %u", framebufferCreateInfo.height);
                R_CSTL_LOG_ERROR ("  - Attachment count: %u", framebufferCreateInfo.attachmentCount);
                R_CVulkan_DeleteImageView (&imageView);
                R_CSTL_HeapFree (pSwapchainImages);
                return err;
            }
        }
    }
    R_CSTL_HeapFree (pSwapchainImages);
    R_CSTL_LOG_INFO ("r_game_initialize_framebuffers: Framebuffers initialized");
    R_CSTL_LOG_INFO ("  Total framebuffers: %u", imageCount);
    R_CSTL_LOG_INFO ("  Extent: %ux%u", swapchainExtent.width, swapchainExtent.height);

    return R_CVULKAN_OK;
}

static enum R_CVulkan_Error
r_game_initialize_queues (struct r_game_pipeline_context* pContext, struct R_CVulkan_Surface* pSurface)
{
    R_CSTL_TRACE_SCOPE ();
    struct R_CVulkan_QueueFamilyIndices indices;
    enum R_CVulkan_Error                 err;
    VkSurfaceKHR                        surface = VK_NULL_HANDLE;

#if defined(R_GAME_DEBUG)
    if (pSurface)
    {
#endif
        surface = R_CVulkan_SurfaceGetHandle (pSurface);
        R_CSTL_LOG_DEBUG ("r_game_initialize_queues: Surface handle: %p", (void*)surface);
#if defined(R_GAME_DEBUG)
    }
#endif
    err = R_CVulkan_DeviceFindQueueFamilies (
        R_CVulkan_DeviceGetPhysicalDevice (&pContext->device),
        surface,
        &indices);
    if (err != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("r_game_initialize_queues: Failed to find queue families");
        R_CSTL_LOG_ERROR ("  Error: %s", R_CVulkan_ErrorToString (err));
        return err;
    }
    R_CSTL_LOG_DEBUG ("r_game_initialize_queues: Queue family indices found");
    R_CSTL_LOG_DEBUG ("  Graphics family: %u", indices.graphicsFamily);
    R_CSTL_LOG_DEBUG ("  Compute family: %u", indices.computeFamily);
    R_CSTL_LOG_DEBUG ("  Transfer family: %u", indices.transferFamily);
#if !defined(R_CVULKAN_HEADLESS)
    R_CSTL_LOG_DEBUG ("  Present family: %u", indices.presentFamily);
#endif
    err = R_CVulkan_NewQueue (&pContext->graphicsQueue, &pContext->device, indices.graphicsFamily, 0);
    if (err != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("r_game_initialize_queues: Failed to create graphics queue");
        R_CSTL_LOG_ERROR ("  Error: %s", R_CVulkan_ErrorToString (err));
        R_CSTL_LOG_ERROR ("  Queue family index: %u", indices.graphicsFamily);
        return err;
    }
    err = R_CVulkan_NewQueue (&pContext->computeQueue, &pContext->device, indices.computeFamily, 0);
    if (err != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("r_game_initialize_queues: Failed to create compute queue");
        R_CSTL_LOG_ERROR ("  Error: %s", R_CVulkan_ErrorToString (err));
        R_CSTL_LOG_ERROR ("  Queue family index: %u", indices.computeFamily);
        goto r_cleanup_queue3;
    }
    err = R_CVulkan_NewQueue (&pContext->transferQueue, &pContext->device, indices.transferFamily, 0);
    if (err != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("r_game_initialize_queues: Failed to create transfer queue");
        R_CSTL_LOG_ERROR ("  Error: %s", R_CVulkan_ErrorToString (err));
        R_CSTL_LOG_ERROR ("  Queue family index: %u", indices.transferFamily);
        goto r_cleanup_queue2;
    }

#if !defined(R_CVULKAN_HEADLESS)
    err = R_CVulkan_NewQueue (&pContext->presentQueue, &pContext->device, indices.presentFamily, 0);
    if (err != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("r_game_initialize_queues: Failed to create present queue");
        R_CSTL_LOG_ERROR ("  Error: %s", R_CVulkan_ErrorToString (err));
        R_CSTL_LOG_ERROR ("  Queue family index: %u", indices.presentFamily);
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

static enum R_CVulkan_Error
r_game_initialize_command_pools (struct r_game_pipeline_context* pContext, struct R_CVulkan_Surface* pSurface)
{
    struct R_CVulkan_QueueFamilyIndices indices;
    enum R_CVulkan_Error                 err;
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
        R_CSTL_LOG_ERROR ("Failed to find queue families: %s", R_CVulkan_ErrorToString (err));
        return err;
    }
    err = R_CVulkan_NewCommandPool (
        &pContext->graphicsCommandPool,
        &pContext->device,
        indices.graphicsFamily,
        0);
    if (err != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("Failed to create graphics command pool: %s", R_CVulkan_ErrorToString (err));
        return err;
    }

    err = R_CVulkan_NewCommandPool (
        &pContext->computeCommandPool,
        &pContext->device,
        indices.computeFamily,
        0);
    if (err != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("Failed to create compute command pool: %s", R_CVulkan_ErrorToString (err));
        return err;
    }

    err = R_CVulkan_NewCommandPool (
        &pContext->transferCommandPool,
        &pContext->device,
        indices.transferFamily,
        0);
    if (err != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("Failed to create transfer command pool: %s", R_CVulkan_ErrorToString (err));
        return err;
    }

    return R_CVULKAN_OK;
}

static enum r_game_error
r_game_initialize_vulkan_core (
    struct r_game_pipeline_context*                 pContext,
    const struct r_game_pipeline_context_create_info* pCreateInfo)
{
    enum R_CVulkan_Error                 error;
    struct R_CVulkan_InstanceCreateInfo instanceCreateInfo = {0};
    instanceCreateInfo.pApplicationName = pCreateInfo->pApplicationName;
    {
        error = R_CVulkan_NewInstance (&pContext->instance, &instanceCreateInfo);
        if (error != R_CVULKAN_OK)
        {
            R_CSTL_LOG_ERROR ("r_game_initialize_vulkan_core: Failed to create Vulkan instance");
            R_CSTL_LOG_ERROR ("  Error: %s", R_CVulkan_ErrorToString (error));
            return error;
        }
    }
    pContext->pSurface = (struct R_CVulkan_Surface*)R_CSTL_HeapAlloc (sizeof (struct R_CVulkan_Surface));
    if (pContext->pSurface == NULL)
    {
        R_CSTL_LOG_ERROR ("r_game_initialize_vulkan_core: Failed to allocate memory for surface");
        R_CSTL_LOG_ERROR ("  Requested size: %zu bytes", sizeof (struct R_CVulkan_Surface));
        error = R_GAME_ERROR_OUT_OF_MEMORY;
        goto r_cleanup_instance;
    }
    memset (pContext->pSurface, 0, sizeof (struct R_CVulkan_Surface));
    struct R_CVulkan_SurfaceCreateInfo surfaceCreateInfo = {0};
    surfaceCreateInfo.pInstance = &pContext->instance;
#if defined(R_CVULKAN_PLATFORM_WINDOWS)
    surfaceCreateInfo.hInstance = pCreateInfo->hInstance;
    surfaceCreateInfo.hWnd = pCreateInfo->hWnd;
#elif defined(R_CVULKAN_PLATFORM_LINUX)
    surfaceCreateInfo.linuxBackend = pCreateInfo->linuxBackend;
    surfaceCreateInfo.pDisplay = pCreateInfo->pDisplay;
    surfaceCreateInfo.pSurface = pCreateInfo->pSurface;
    surfaceCreateInfo.pX11Display = pCreateInfo->pX11Display;
    surfaceCreateInfo.x11Window = pCreateInfo->x11Window;
    surfaceCreateInfo.pXCBConnection = pCreateInfo->pXCBConnection;
    surfaceCreateInfo.xcbWindow = pCreateInfo->xcbWindow;

    if (pCreateInfo->linuxBackend == R_GAME_LINUX_BACKEND_WAYLAND)
    {
        R_CSTL_LOG_INFO (
            "r_game_initialize_vulkan_core: Wayland display=%p, surface=%p",
            (void*)pCreateInfo->pDisplay,
            (void*)pCreateInfo->pSurface);
    }
    else if (pCreateInfo->linuxBackend == R_GAME_LINUX_BACKEND_X11)
    {
        R_CSTL_LOG_INFO (
            "r_game_initialize_vulkan_core: X11 display=%p, window=%lu",
            (void*)pCreateInfo->pX11Display,
            (unsigned long)pCreateInfo->x11Window);
    }
    else if (pCreateInfo->linuxBackend == R_GAME_LINUX_BACKEND_XCB)
    {
        R_CSTL_LOG_INFO (
            "r_game_initialize_vulkan_core: XCB connection=%p, window=%u",
            (void*)pCreateInfo->pXCBConnection,
            pCreateInfo->xcbWindow);
    }
#elif defined(R_CVULKAN_PLATFORM_ANDROID)
    surfaceCreateInfo.pWindow = pCreateInfo->pWindow;
#elif defined(R_CVULKAN_PLATFORM_MACOS)
    surfaceCreateInfo.pNSWindow = pCreateInfo->pNSWindow;
#endif

#if !defined(R_CVULKAN_HEADLESS)
    error = R_CVulkan_NewSurface (pContext->pSurface, &surfaceCreateInfo);
    if (error != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("r_game_initialize_vulkan_core: Failed to create Vulkan surface");
        R_CSTL_LOG_ERROR ("  Error: %s", R_CVulkan_ErrorToString (error));
        goto r_cleanup_surface_allocation;
    }
#endif
    struct R_CVulkan_DeviceCreateInfo deviceCreateInfo = {0};
    deviceCreateInfo.pInstance = &pContext->instance;
    deviceCreateInfo.pSurface = pContext->pSurface;

    error = R_CVulkan_NewDevice (&pContext->device, &deviceCreateInfo);
    if (error != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("r_game_initialize_vulkan_core: Failed to create Vulkan device");
        R_CSTL_LOG_ERROR ("  Error: %s", R_CVulkan_ErrorToString (error));
        goto r_cleanup_surface;
    }
    return R_GAME_OK;
r_cleanup_surface:
#if !defined(R_CVULKAN_HEADLESS)
    R_CVulkan_DeleteSurface (pContext->pSurface);
#endif
r_cleanup_surface_allocation:
    R_CSTL_HeapFree (pContext->pSurface);
r_cleanup_instance:
    R_CVulkan_DeleteInstance (&pContext->instance);
    return error;
}

static enum r_game_error
r_game_initialize_vulkan_resources (struct r_game_pipeline_context* pContext)
{
    R_CSTL_TRACE_SCOPE ();
    enum R_CVulkan_Error err;
    err = r_game_initialize_queues (pContext, pContext->pSurface);
    if (err != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("r_game_initialize_vulkan_resources: Queue initialization failed");
        R_CSTL_LOG_ERROR ("  Error: %s", R_CVulkan_ErrorToString (err));
        return R_GAME_ERROR_INITIALIZATION_FAILED;
    }
    err = r_game_initialize_command_pools (pContext, pContext->pSurface);
    if (err != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("r_game_initialize_vulkan_resources: Command pool initialization failed");
        R_CSTL_LOG_ERROR ("  Error: %s", R_CVulkan_ErrorToString (err));
        goto r_cleanup_queues;
    }
    err = r_game_initialize_sync_primitives (pContext);
    if (err != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("r_game_initialize_vulkan_resources: Sync primitives initialization failed");
        R_CSTL_LOG_ERROR ("  Error: %s", R_CVulkan_ErrorToString (err));
        goto r_cleanup_command_pool;
    }
    return R_GAME_OK;
r_cleanup_command_pool:
    R_CVulkan_DeleteCommandPool (&pContext->graphicsCommandPool);
    R_CVulkan_DeleteCommandPool (&pContext->computeCommandPool);
    R_CVulkan_DeleteCommandPool (&pContext->transferCommandPool);
r_cleanup_queues:
    R_CVulkan_DeleteQueue (&pContext->graphicsQueue);
    R_CVulkan_DeleteQueue (&pContext->computeQueue);
    R_CVulkan_DeleteQueue (&pContext->transferQueue);
#if !defined(R_CVULKAN_HEADLESS)
    R_CVulkan_DeleteQueue (&pContext->presentQueue);
#endif
    return R_GAME_ERROR_INITIALIZATION_FAILED;
}

static enum R_CVulkan_Error
r_game_initialize_sync_primitives (struct r_game_pipeline_context* pContext)
{
    enum R_CVulkan_Error err;
#if !defined(R_CVULKAN_HEADLESS)
    err = R_CVulkan_NewSemaphore (&pContext->imageAvailableSemaphore, &pContext->device, 0, 0);
    if (err != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("r_game_initialize_sync_primitives: Failed to create image available semaphore");
        R_CSTL_LOG_ERROR ("  Error: %s", R_CVulkan_ErrorToString (err));
        return err;
    }
    err = R_CVulkan_NewSemaphore (&pContext->renderFinishedSemaphore, &pContext->device, 0, 0);
    if (err != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("r_game_initialize_sync_primitives: Failed to create render finished semaphore");
        R_CSTL_LOG_ERROR ("  Error: %s", R_CVulkan_ErrorToString (err));
        return err;
    }

    err = R_CVulkan_NewFence (&pContext->inFlightFence, &pContext->device, 1);
    if (err != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("r_game_initialize_sync_primitives: Failed to create in-flight fence");
        R_CSTL_LOG_ERROR ("  Error: %s", R_CVulkan_ErrorToString (err));
        return err;
    }
    R_CSTL_LOG_DEBUG ("r_game_initialize_sync_primitives: In-flight fence created");
#endif
    return R_CVULKAN_OK;
}

static enum r_game_error
r_game_initialize_vulkan_swapchain (
    struct r_game_pipeline_context*                 pContext,
    const struct r_game_pipeline_context_create_info* pCreateInfo)
{
    VkExtent2D windowExtent = r_game_get_window_extent (pCreateInfo);
    R_CSTL_LOG_DEBUG (
        "r_game_initialize_vulkan_swapchain: Window extent: %ux%u",
        windowExtent.width,
        windowExtent.height);
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
        enum R_CVulkan_Error err = R_CVulkan_NewSwapchain (&pContext->swapchain, &swapchainCreateInfo);
        if (err != R_CVULKAN_OK)
        {
            R_CSTL_LOG_ERROR ("r_game_initialize_vulkan_swapchain: Failed to create swapchain");
            R_CSTL_LOG_ERROR ("  Error: %s", R_CVulkan_ErrorToString (err));
            return R_GAME_ERROR_INITIALIZATION_FAILED;
        }
    }
    {
        enum R_CVulkan_Error err = r_game_initialize_render_pass (pContext);
        if (err != R_CVULKAN_OK)
        {
            R_CSTL_LOG_ERROR ("r_game_initialize_vulkan_swapchain: Render pass initialization failed");
            R_CSTL_LOG_ERROR ("  Error: %s", R_CVulkan_ErrorToString (err));
            goto r_cleanup_swapchain;
        }
    }
    {
        enum R_CVulkan_Error err = r_game_initialize_framebuffers (pContext);
        if (err != R_CVULKAN_OK)
        {
            R_CSTL_LOG_ERROR ("r_game_initialize_vulkan_swapchain: Framebuffer initialization failed");
            R_CSTL_LOG_ERROR ("  Error: %s", R_CVulkan_ErrorToString (err));
            goto r_cleanup_render_pass;
        }
    }
    return R_GAME_OK;

r_cleanup_render_pass:
    R_CVulkan_DeleteRenderPass (&pContext->renderPass);
r_cleanup_swapchain:
    R_CVulkan_DeleteSwapchain (&pContext->swapchain);

    R_CSTL_LOG_ERROR ("r_game_initialize_vulkan_swapchain: Cleanup completed after initialization failure");
    return R_GAME_ERROR_INITIALIZATION_FAILED;
}

R_GAME_API enum r_game_error
r_game_new_pipeline_context (
    struct r_game_pipeline_context*                 pContext,
    const struct r_game_pipeline_context_create_info* pCreateInfo)
{
    R_CVULKAN_ASSERT (pContext);
    R_CVULKAN_ASSERT (pCreateInfo);
    R_CSTL_TRACE_SCOPE ();

    R_CSTL_LOG_INFO ("r_game_new_pipeline_context: Starting pipeline context initialization");
    R_CSTL_LOG_INFO (
        "  Application name: %s",
        pCreateInfo->pApplicationName ? pCreateInfo->pApplicationName : "NULL");
    enum r_game_error err;
    err = r_game_initialize_vulkan_core (pContext, pCreateInfo);
    if (err != R_GAME_OK)
    {
        R_CSTL_LOG_ERROR ("r_game_new_pipeline_context: Vulkan core initialization failed");
        return err;
    }
    err = r_game_initialize_vulkan_resources (pContext);
    if (err != R_GAME_OK)
    {
        R_CSTL_LOG_ERROR ("r_game_new_pipeline_context: Vulkan resources initialization failed");
        goto r_cleanup_core;
    }

#if !defined(R_CVULKAN_HEADLESS)
    err = r_game_initialize_vulkan_swapchain (pContext, pCreateInfo);
    if (err != R_GAME_OK)
    {
        R_CSTL_LOG_ERROR ("r_game_new_pipeline_context: Vulkan swapchain initialization failed");
        goto r_cleanup_resources;
    }
#endif
    pContext->currentFrameIndex = 0;
    R_CSTL_LOG_INFO ("r_game_new_pipeline_context: Pipeline context initialized");
    return R_GAME_OK;

#if !defined(R_CVULKAN_HEADLESS)
r_cleanup_resources:
    R_CVulkan_DeleteQueue (&pContext->graphicsQueue);
    R_CVulkan_DeleteQueue (&pContext->computeQueue);
    R_CVulkan_DeleteQueue (&pContext->transferQueue);
    R_CVulkan_DeleteQueue (&pContext->presentQueue);
    R_CVulkan_DeleteSemaphore (&pContext->imageAvailableSemaphore);
    R_CVulkan_DeleteSemaphore (&pContext->renderFinishedSemaphore);
    R_CVulkan_DeleteFence (&pContext->inFlightFence);
    R_CVulkan_DeleteCommandPool (&pContext->graphicsCommandPool);
    R_CVulkan_DeleteCommandPool (&pContext->computeCommandPool);
    R_CVulkan_DeleteCommandPool (&pContext->transferCommandPool);
#endif
r_cleanup_core:
    R_CVulkan_DeleteDevice (&pContext->device);
#if !defined(R_CVULKAN_HEADLESS)
    R_CVulkan_DeleteSurface (pContext->pSurface);
#endif
    R_CSTL_HeapFree (pContext->pSurface);
    pContext->pSurface = NULL;
    R_CVulkan_DeleteInstance (&pContext->instance);

    R_CSTL_LOG_ERROR ("r_game_new_pipeline_context: Cleanup completed after initialization failure");
    return err;
}

R_GAME_API void
r_game_pipeline_context_delete (struct r_game_pipeline_context* pContext)
{
    R_CVULKAN_ASSERT (pContext);
    if (pContext->pFramebuffers)
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

    if (pContext->pSurface)
    {
        R_CVulkan_DeleteSurface (pContext->pSurface);
        R_CSTL_HeapFree (pContext->pSurface);
        pContext->pSurface = NULL;
    }
    R_CVulkan_DeleteDevice (&pContext->device);
    R_CVulkan_DeleteInstance (&pContext->instance);
}

R_GAME_API struct R_CVulkan_Queue*
r_game_pipeline_context_get_graphics_queue (struct r_game_pipeline_context* pContext)
{
    R_CVULKAN_ASSERT (pContext);
    return &pContext->graphicsQueue;
}

R_GAME_API struct R_CVulkan_Queue*
r_game_pipeline_context_get_compute_queue (struct r_game_pipeline_context* pContext)
{
    R_CVULKAN_ASSERT (pContext);
    return &pContext->computeQueue;
}

R_GAME_API struct R_CVulkan_Queue*
r_game_pipeline_context_get_transfer_queue (struct r_game_pipeline_context* pContext)
{
    R_CVULKAN_ASSERT (pContext);
    return &pContext->transferQueue;
}

R_GAME_API struct R_CVulkan_Queue*
r_game_pipeline_context_get_present_queue (struct r_game_pipeline_context* pContext)
{
    R_CVULKAN_ASSERT (pContext);
    return &pContext->presentQueue;
}

R_GAME_API struct R_CVulkan_CommandPool*
r_game_pipeline_context_get_graphics_command_pool (struct r_game_pipeline_context* pContext)
{
    R_CVULKAN_ASSERT (pContext);
    return &pContext->graphicsCommandPool;
}

R_GAME_API struct R_CVulkan_CommandPool*
r_game_pipeline_context_get_compute_command_pool (struct r_game_pipeline_context* pContext)
{
    R_CVULKAN_ASSERT (pContext);
    return &pContext->computeCommandPool;
}

R_GAME_API struct R_CVulkan_CommandPool*
r_game_pipeline_context_get_transfer_command_pool (struct r_game_pipeline_context* pContext)
{
    R_CVULKAN_ASSERT (pContext);
    return &pContext->transferCommandPool;
}

R_GAME_API struct R_CVulkan_Device*
r_game_pipeline_context_get_device (struct r_game_pipeline_context* pContext)
{
    R_CVULKAN_ASSERT (pContext);
    return &pContext->device;
}

R_GAME_API struct R_CVulkan_Semaphore*
r_game_pipeline_context_get_image_available_semaphore (struct r_game_pipeline_context* pContext)
{
    R_CVULKAN_ASSERT (pContext);
    return &pContext->imageAvailableSemaphore;
}

R_GAME_API struct R_CVulkan_Semaphore*
r_game_pipeline_context_get_render_finished_semaphore (struct r_game_pipeline_context* pContext)
{
    R_CVULKAN_ASSERT (pContext);
    return &pContext->renderFinishedSemaphore;
}

R_GAME_API struct R_CVulkan_Fence*
r_game_pipeline_context_get_in_flight_fence (struct r_game_pipeline_context* pContext)
{
    R_CVULKAN_ASSERT (pContext);
    return &pContext->inFlightFence;
}

R_GAME_API uint32_t*
r_game_pipeline_context_get_current_frame_index (struct r_game_pipeline_context* pContext)
{
    R_CVULKAN_ASSERT (pContext);
    return &pContext->currentFrameIndex;
}
