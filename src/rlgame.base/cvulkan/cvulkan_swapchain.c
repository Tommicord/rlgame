#include "rlgame.base/cvulkan/cvulkan_swapchain.h"
#include "rlgame.base/cvulkan/cvulkan_device.h"
#include "rlgame.base/cvulkan/cvulkan_surface.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"
#include "rlgame.base/cstl/cstl_log.h"
#include "rlgame.base/cstl/cstl_trace.h"

#include <string.h>
#include <inttypes.h>
#include <vulkan/vulkan.h>

static enum R_CVulkan_Error
r_cvulkan_choose_swap_surface_format (
    const VkSurfaceFormatKHR* pAvailableFormats,
    uint32_t                  formatCount,
    VkSurfaceFormatKHR*       pOutFormat)
{
    R_CSTL_TRACE_SCOPE ();
    R_CVULKAN_ASSERT (pAvailableFormats);
    R_CVULKAN_ASSERT (pOutFormat);

    R_CSTL_LOG_DEBUG ("r_cvulkan_choose_swap_surface_format: Available formats: %u", formatCount);

    if (formatCount == 0)
    {
        R_CSTL_LOG_ERROR ("r_cvulkan_choose_swap_surface_format: No surface formats available");
        return R_CVULKAN_ERROR_INVALID_ARGUMENT;
    }

    for (uint32_t i = 0; i < formatCount; ++i)
    {
        VkSurfaceFormatKHR available = pAvailableFormats[i];
        if (available.format == VK_FORMAT_B8G8R8A8_SRGB
            && available.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            *pOutFormat = available;
            R_CSTL_LOG_INFO (
                "r_cvulkan_choose_swap_surface_format: Selected %s for swapchain format",
                r_cvulkan_format_to_string (available.format));
            return R_CVULKAN_OK;
        }
    }

    *pOutFormat = pAvailableFormats[0];
    R_CSTL_LOG_INFO (
        "r_cvulkan_choose_swap_surface_format: Fallback to first available format: %s",
        r_cvulkan_format_to_string (pAvailableFormats[0].format));
    return R_CVULKAN_OK;
}

static enum R_CVulkan_Error
r_cvulkan_choose_swap_present_mode (
    const VkPresentModeKHR* pAvailableModes,
    uint32_t                modeCount,
    VkPresentModeKHR*       pOutMode)
{
    R_CSTL_TRACE_SCOPE ();
    R_CVULKAN_ASSERT (pAvailableModes);
    R_CVULKAN_ASSERT (pOutMode);

    R_CSTL_LOG_DEBUG ("r_cvulkan_choose_swap_present_mode: Available present modes: %u", modeCount);

    if (modeCount == 0)
    {
        R_CSTL_LOG_ERROR ("r_cvulkan_choose_swap_present_mode: No present modes available");
        return R_CVULKAN_ERROR_INVALID_ARGUMENT;
    }

    for (uint32_t i = 0; i < modeCount; ++i)
    {
        if (pAvailableModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            *pOutMode = pAvailableModes[i];
            R_CSTL_LOG_INFO ("r_cvulkan_choose_swap_present_mode: Selected MAILBOX present mode");
            return R_CVULKAN_OK;
        }
    }

    *pOutMode = VK_PRESENT_MODE_FIFO_KHR;
    R_CSTL_LOG_INFO ("r_cvulkan_choose_swap_present_mode: Fallback to FIFO present mode");
    return R_CVULKAN_OK;
}

static VkExtent2D
r_cvulkan_choose_swap_extent (const VkSurfaceCapabilitiesKHR* pCapabilities, VkExtent2D requestedExtent)
{
    R_CSTL_TRACE_SCOPE ();
    R_CVULKAN_ASSERT (pCapabilities);

    if (pCapabilities->currentExtent.width != UINT32_MAX)
    {
        R_CSTL_LOG_DEBUG (
            "r_cvulkan_choose_swap_extent: Using current extent: %ux%u",
            pCapabilities->currentExtent.width,
            pCapabilities->currentExtent.height);
        return pCapabilities->currentExtent;
    }

    VkExtent2D actualExtent = requestedExtent;
    actualExtent.width = (actualExtent.width < pCapabilities->minImageExtent.width)
                             ? pCapabilities->minImageExtent.width
                             : actualExtent.width;
    actualExtent.width = (actualExtent.width > pCapabilities->maxImageExtent.width)
                             ? pCapabilities->maxImageExtent.width
                             : actualExtent.width;
    actualExtent.height = (actualExtent.height < pCapabilities->minImageExtent.height)
                              ? pCapabilities->minImageExtent.height
                              : actualExtent.height;
    actualExtent.height = (actualExtent.height > pCapabilities->maxImageExtent.height)
                              ? pCapabilities->maxImageExtent.height
                              : actualExtent.height;

    R_CSTL_LOG_DEBUG (
        "r_cvulkan_choose_swap_extent: Clamped extent from %ux%u to %ux%u",
        requestedExtent.width,
        requestedExtent.height,
        actualExtent.width,
        actualExtent.height);
    return actualExtent;
}

static enum R_CVulkan_Error
r_cvulkan_get_surface_capabilities (
    VkPhysicalDevice          physicalDevice,
    VkSurfaceKHR              surface,
    VkSurfaceCapabilitiesKHR* pCapabilities)
{
    VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR (physicalDevice, surface, pCapabilities);
    if (result != VK_SUCCESS)
    {
        R_CSTL_LOG_ERROR ("Failed to get surface capabilities: %d", result);
        return r_cvulkan_result_to_error (result);
    }
    R_CSTL_LOG_DEBUG (
        "Surface capabilities: minImageCount=%u, maxImageCount=%u, currentExtent=%ux%u",
        pCapabilities->minImageCount,
        pCapabilities->maxImageCount,
        pCapabilities->currentExtent.width,
        pCapabilities->currentExtent.height);
    return R_CVULKAN_OK;
}

static enum R_CVulkan_Error
r_cvulkan_get_surface_formats (
    VkPhysicalDevice     physicalDevice,
    VkSurfaceKHR         surface,
    VkSurfaceFormatKHR** ppFormats,
    uint32_t*            pFormatCount)
{
    vkGetPhysicalDeviceSurfaceFormatsKHR (physicalDevice, surface, pFormatCount, NULL);
    R_CSTL_LOG_DEBUG ("Surface format count: %u", *pFormatCount);

    *ppFormats = (VkSurfaceFormatKHR*)r_cstl_heap_alloc (*pFormatCount * sizeof (VkSurfaceFormatKHR));
    if (!*ppFormats)
    {
        R_CSTL_LOG_ERROR ("Failed to allocate surface formats array");
        return R_CVULKAN_ERROR_OUT_OF_MEMORY;
    }
    vkGetPhysicalDeviceSurfaceFormatsKHR (physicalDevice, surface, pFormatCount, *ppFormats);
    return R_CVULKAN_OK;
}

static enum R_CVulkan_Error
r_cvulkan_get_present_modes (
    VkPhysicalDevice   physicalDevice,
    VkSurfaceKHR       surface,
    VkPresentModeKHR** ppPresentModes,
    uint32_t*          pPresentModeCount)
{
    vkGetPhysicalDeviceSurfacePresentModesKHR (physicalDevice, surface, pPresentModeCount, NULL);
    R_CSTL_LOG_DEBUG ("Present mode count: %u", *pPresentModeCount);

    *ppPresentModes = (VkPresentModeKHR*)r_cstl_heap_alloc (*pPresentModeCount * sizeof (VkPresentModeKHR));
    if (!*ppPresentModes)
    {
        R_CSTL_LOG_ERROR ("Failed to allocate present modes array");
        return R_CVULKAN_ERROR_OUT_OF_MEMORY;
    }
    vkGetPhysicalDeviceSurfacePresentModesKHR (physicalDevice, surface, pPresentModeCount, *ppPresentModes);
    return R_CVULKAN_OK;
}

static enum R_CVulkan_Error
r_cvulkan_choose_surface_format (
    const VkSurfaceFormatKHR* pFormats,
    uint32_t                  formatCount,
    const VkSurfaceFormatKHR* pUserFormat,
    VkSurfaceFormatKHR*       pChosenFormat)
{
    if (pUserFormat->format != VK_FORMAT_UNDEFINED)
    {
        *pChosenFormat = *pUserFormat;
        R_CSTL_LOG_INFO ("Using user-specified format: %s", r_cvulkan_format_to_string (pUserFormat->format));
        return R_CVULKAN_OK;
    }

    return r_cvulkan_choose_swap_surface_format (pFormats, formatCount, pChosenFormat);
}

static enum R_CVulkan_Error
r_cvulkan_get_image_count (
    const VkSurfaceCapabilitiesKHR* pCapabilities,
    uint32_t                        requestedCount,
    uint32_t*                       pImageCount)
{
    *pImageCount = requestedCount;
    if (*pImageCount == 0)
    {
        *pImageCount = pCapabilities->minImageCount + 1;
    }
    if (pCapabilities->maxImageCount > 0 && *pImageCount > pCapabilities->maxImageCount)
    {
        *pImageCount = pCapabilities->maxImageCount;
    }
    R_CSTL_LOG_DEBUG (
        "Image count: %u (requested: %u, min: %u, max: %u)",
        *pImageCount,
        requestedCount,
        pCapabilities->minImageCount,
        pCapabilities->maxImageCount);
    return R_CVULKAN_OK;
}

static enum R_CVulkan_Error
r_cvulkan_choose_composite_alpha (
    const VkSurfaceCapabilitiesKHR* pCapabilities,
    VkCompositeAlphaFlagBitsKHR     userAlpha,
    VkCompositeAlphaFlagBitsKHR*    pChosenAlpha)
{
    // Log supported composite alpha modes for debugging
    R_CSTL_LOG_DEBUG ("Supported composite alpha: 0x%x", pCapabilities->supportedCompositeAlpha);
    if (pCapabilities->supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
    {
        R_CSTL_LOG_DEBUG ("  - OPAQUE_BIT_KHR supported");
    }
    if (pCapabilities->supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
    {
        R_CSTL_LOG_DEBUG ("  - PRE_MULTIPLIED_BIT_KHR supported");
    }
    if (pCapabilities->supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR)
    {
        R_CSTL_LOG_DEBUG ("  - POST_MULTIPLIED_BIT_KHR supported");
    }
    if (pCapabilities->supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
    {
        R_CSTL_LOG_DEBUG ("  - INHERIT_BIT_KHR supported");
    }

    // Handle composite alpha with proper fallback for Wayland compatibility
    if (userAlpha != 0)
    {
        *pChosenAlpha = userAlpha;
    }
    else if (pCapabilities->supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
    {
        *pChosenAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    }
    else if (pCapabilities->supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
    {
        *pChosenAlpha = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
    }
    else if (pCapabilities->supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR)
    {
        *pChosenAlpha = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
    }
    else if (pCapabilities->supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
    {
        *pChosenAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    }
    else
    {
        R_CSTL_LOG_ERROR ("No supported composite alpha mode found");
        return R_CVULKAN_ERROR_SURFACE_LOST;
    }
    R_CSTL_LOG_DEBUG ("Selected composite alpha: %u", *pChosenAlpha);
    return R_CVULKAN_OK;
}

static enum R_CVulkan_Error
r_cvulkan_settingsure_queue_families (
    VkPhysicalDevice          physicalDevice,
    VkSurfaceKHR              surface,
    VkSwapchainCreateInfoKHR* pCreateInfo)
{
    struct r_cvulkan_queue_family_indices indices;
    enum R_CVulkan_Error err = r_cvulkan_device_find_queue_families (physicalDevice, surface, &indices);
    if (err != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("Failed to find queue families");
        return err;
    }

    R_CSTL_LOG_DEBUG (
        "Queue families: graphics=%u, present=%u",
        indices.graphicsFamily,
        indices.presentFamily);

    uint32_t queueFamilyIndices[] = {indices.graphicsFamily, indices.presentFamily};
    if (indices.graphicsFamily != indices.presentFamily)
    {
        pCreateInfo->imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        pCreateInfo->queueFamilyIndexCount = sizeof (queueFamilyIndices) / sizeof (queueFamilyIndices[0]);
        pCreateInfo->pQueueFamilyIndices = queueFamilyIndices;
        R_CSTL_LOG_DEBUG ("Using CONCURRENT sharing mode (different queues)");
    }
    else
    {
        pCreateInfo->imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        R_CSTL_LOG_DEBUG ("Using EXCLUSIVE sharing mode (same queue)");
    }

    return R_CVULKAN_OK;
}

static enum R_CVulkan_Error
R_CVulkan_CreateSwapchain (
    struct R_CVulkan_Swapchain*     pSwapchain,
    const VkSwapchainCreateInfoKHR* pCreateInfo)
{
    VkResult result = vkCreateSwapchainKHR (pSwapchain->device, pCreateInfo, NULL, &pSwapchain->handle);
    if (result != VK_SUCCESS)
    {
        R_CSTL_LOG_ERROR ("Failed to create swapchain: %d", result);
        return r_cvulkan_result_to_error (result);
    }
    R_CSTL_LOG_INFO ("Swapchain created");
    R_CSTL_LOG_INFO ("  - Handle: %p", (void*)pSwapchain->handle);
    return R_CVULKAN_OK;
}

static enum R_CVulkan_Error
r_cvulkan_get_swapchain_images (struct R_CVulkan_Swapchain* pSwapchain)
{
    uint32_t imageCount = 0;
    vkGetSwapchainImagesKHR (pSwapchain->device, pSwapchain->handle, &imageCount, NULL);
    pSwapchain->imageCount = imageCount;

    VkImage* pImages = (VkImage*)r_cstl_heap_alloc (imageCount * sizeof (VkImage));
    if (!pImages)
    {
        R_CSTL_LOG_ERROR ("Failed to allocate swapchain images array");
        return R_CVULKAN_ERROR_OUT_OF_MEMORY;
    }

    vkGetSwapchainImagesKHR (pSwapchain->device, pSwapchain->handle, &imageCount, pImages);
    r_cstl_heap_free (pImages);

    R_CSTL_LOG_INFO (
        "  - Final configuration: imageCount=%u, format=%s, extent=%ux%u",
        pSwapchain->imageCount,
        r_cvulkan_format_to_string (pSwapchain->imageFormat),
        pSwapchain->extent.width,
        pSwapchain->extent.height);
    return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_NewSwapchain (
    struct R_CVulkan_Swapchain*                 pSwapchain,
    const struct r_cvulkan_swapchain_create_info* pCreateInfo)
{
    R_CVULKAN_ASSERT (pSwapchain);
    R_CVULKAN_ASSERT (pCreateInfo);
    R_CVULKAN_ASSERT (pCreateInfo->pDevice);
    R_CVULKAN_ASSERT (pCreateInfo->pSurface);

    R_CSTL_TRACE_SCOPE ();
    R_CSTL_LOG_INFO ("R_CVulkan_NewSwapchain: Swapchain info");
    R_CSTL_LOG_DEBUG ("  Context: pSwapchain=%p, pCreateInfo=%p", (void*)pSwapchain, (void*)pCreateInfo);

    pSwapchain->device = r_cvulkan_device_get_logical_device (pCreateInfo->pDevice);
    VkPhysicalDevice physicalDevice = r_cvulkan_device_get_physical_device (pCreateInfo->pDevice);
    VkSurfaceKHR     surface = r_cvulkan_surface_get_handle (pCreateInfo->pSurface);

    R_CSTL_LOG_DEBUG ("  Physical device: %p, Surface: %p", (void*)physicalDevice, (void*)surface);

    VkSurfaceCapabilitiesKHR capabilities;
    enum R_CVulkan_Error     err = r_cvulkan_get_surface_capabilities (physicalDevice, surface, &capabilities);
    if (err != R_CVULKAN_OK)
    {
        return err;
    }

    VkSurfaceFormatKHR* pFormats = NULL;
    uint32_t            formatCount = 0;
    err = r_cvulkan_get_surface_formats (physicalDevice, surface, &pFormats, &formatCount);
    if (err != R_CVULKAN_OK)
    {
        return err;
    }

    VkPresentModeKHR* pPresentModes = NULL;
    uint32_t          presentModeCount = 0;
    err = r_cvulkan_get_present_modes (physicalDevice, surface, &pPresentModes, &presentModeCount);
    if (err != R_CVULKAN_OK)
    {
        r_cstl_heap_free (pFormats);
        return err;
    }

    VkSurfaceFormatKHR surfaceFormat;
    err = r_cvulkan_choose_surface_format (pFormats, formatCount, &pCreateInfo->surfaceFormat, &surfaceFormat);
    if (err != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("Failed to choose surface format");
        r_cstl_heap_free (pFormats);
        r_cstl_heap_free (pPresentModes);
        return err;
    }

    VkPresentModeKHR presentMode = pCreateInfo->presentMode;
    err = r_cvulkan_choose_swap_present_mode (pPresentModes, presentModeCount, &presentMode);
    if (err != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("Failed to choose present mode");
        r_cstl_heap_free (pFormats);
        r_cstl_heap_free (pPresentModes);
        return err;
    }

    VkExtent2D extent = pCreateInfo->extent;
    if (extent.width == 0 || extent.height == 0)
    {
        extent = r_cvulkan_choose_swap_extent (&capabilities, capabilities.currentExtent);
    }
    else
    {
        extent = r_cvulkan_choose_swap_extent (&capabilities, extent);
    }
    R_CSTL_LOG_DEBUG ("  Chosen extent: %ux%u", extent.width, extent.height);

    uint32_t imageCount = 0;
    err = r_cvulkan_get_image_count (&capabilities, pCreateInfo->imageCount, &imageCount);
    if (err != R_CVULKAN_OK)
    {
        r_cstl_heap_free (pFormats);
        r_cstl_heap_free (pPresentModes);
        return err;
    }

    VkSwapchainCreateInfoKHR createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = pCreateInfo->arrayLayers > 0 ? pCreateInfo->arrayLayers : 1;
    createInfo.imageUsage
        = pCreateInfo->imageUsage != 0 ? pCreateInfo->imageUsage : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    err = r_cvulkan_settingsure_queue_families (physicalDevice, surface, &createInfo);
    if (err != R_CVULKAN_OK)
    {
        r_cstl_heap_free (pFormats);
        r_cstl_heap_free (pPresentModes);
        return err;
    }

    createInfo.preTransform
        = pCreateInfo->preTransform != 0 ? pCreateInfo->preTransform : capabilities.currentTransform;

    VkCompositeAlphaFlagBitsKHR compositeAlpha;
    err = r_cvulkan_choose_composite_alpha (&capabilities, pCreateInfo->compositeAlpha, &compositeAlpha);
    if (err != R_CVULKAN_OK)
    {
        r_cstl_heap_free (pFormats);
        r_cstl_heap_free (pPresentModes);
        return err;
    }
    createInfo.compositeAlpha = compositeAlpha;
    createInfo.clipped = pCreateInfo->clipped != 0 ? pCreateInfo->clipped : VK_TRUE;
    createInfo.oldSwapchain = pCreateInfo->oldSwapchain;

    err = R_CVulkan_CreateSwapchain (pSwapchain, &createInfo);
    r_cstl_heap_free (pFormats);
    r_cstl_heap_free (pPresentModes);
    if (err != R_CVULKAN_OK)
    {
        return err;
    }

    pSwapchain->imageFormat = surfaceFormat.format;
    pSwapchain->extent = extent;

    err = r_cvulkan_get_swapchain_images (pSwapchain);
    if (err != R_CVULKAN_OK)
    {
        return err;
    }

#if defined(R_CVULKAN_DEBUG)

#endif
    return R_CVULKAN_OK;
}

R_CVULKAN_API void
R_CVulkan_DeleteSwapchain (struct R_CVulkan_Swapchain* pSwapchain)
{
    R_CVULKAN_ASSERT (pSwapchain);
#if defined(R_CVULKAN_DEBUG)
    if (!pSwapchain)
    {
        return;
    }
    if (pSwapchain->handle != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR (pSwapchain->device, pSwapchain->handle, NULL);
    }
    pSwapchain->handle = VK_NULL_HANDLE;
    pSwapchain->device = VK_NULL_HANDLE;
    pSwapchain->imageFormat = VK_FORMAT_UNDEFINED;
    pSwapchain->extent.width = 0;
    pSwapchain->extent.height = 0;
    pSwapchain->imageCount = 0;

#else
    if (pSwapchain->handle != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR (pSwapchain->device, pSwapchain->handle, NULL);
    }
    (void)pSwapchain;
#endif
}

R_CVULKAN_API VkSwapchainKHR
r_cvulkan_swapchain_get_handle (const struct R_CVulkan_Swapchain* pSwapchain)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pSwapchain);
#endif
    return pSwapchain->handle;
}

R_CVULKAN_API VkDevice
r_cvulkan_swapchain_get_device (const struct R_CVulkan_Swapchain* pSwapchain)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pSwapchain);
#endif
    return pSwapchain->device;
}

R_CVULKAN_API VkFormat
r_cvulkan_swapchain_get_image_format (const struct R_CVulkan_Swapchain* pSwapchain)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pSwapchain);
#endif
    return pSwapchain->imageFormat;
}

R_CVULKAN_API VkExtent2D
r_cvulkan_swapchain_get_extent (const struct R_CVulkan_Swapchain* pSwapchain)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pSwapchain);
#endif
    return pSwapchain->extent;
}

R_CVULKAN_API uint32_t
r_cvulkan_swapchain_get_image_count (const struct R_CVulkan_Swapchain* pSwapchain)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pSwapchain);
#endif
    return pSwapchain->imageCount;
}

R_CVULKAN_API enum R_CVulkan_Error
r_cvulkan_swapchain_acquire_next_image (
    struct R_CVulkan_Swapchain* pSwapchain,
    uint64_t                    timeout,
    VkSemaphore                 semaphore,
    VkFence                     fence,
    uint32_t*                   pImageIndex)
{
    R_CVULKAN_ASSERT (pSwapchain);
    R_CVULKAN_ASSERT (pImageIndex);

    VkResult result = vkAcquireNextImageKHR (
        pSwapchain->device,
        pSwapchain->handle,
        timeout,
        semaphore,
        fence,
        pImageIndex);
    if (result == VK_SUCCESS)
    {
        return R_CVULKAN_OK;
    }
    else if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        return R_CVULKAN_ERROR_SWAPCHAIN_OUT_OF_DATE;
    }
    else if (result == VK_SUBOPTIMAL_KHR)
    {
        return R_CVULKAN_ERROR_SWAPCHAIN_SUBOPTIMAL;
    }
    else
    {
        return r_cvulkan_result_to_error (result);
    }
}