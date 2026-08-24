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

static enum R_CVulkanError
R_CVulkan_ChooseSwapSurfaceFormat (
    const VkSurfaceFormatKHR* pAvailableFormats,
    uint32_t                  formatCount,
    VkSurfaceFormatKHR*       pOutFormat)
{
        R_CSTL_TRACE_SCOPE ();
        R_CVULKAN_ASSERT (pAvailableFormats);
        R_CVULKAN_ASSERT (pOutFormat);

        R_CSTL_LOG_DEBUG ("R_CVulkan_ChooseSwapSurfaceFormat: Available formats: %u", formatCount);

        if (formatCount == 0)
        {
                R_CSTL_LOG_ERROR ("R_CVulkan_ChooseSwapSurfaceFormat: No surface formats available");
                return R_CVULKAN_ERROR_INVALID_ARGUMENT;
        }

        for (uint32_t i = 0; i < formatCount; ++i)
        {
                VkSurfaceFormatKHR available = pAvailableFormats[i];
                if (available.format == VK_FORMAT_B8G8R8A8_SRGB
                    && available.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                {
                        *pOutFormat = available;
                        R_CSTL_LOG_INFO ("R_CVulkan_ChooseSwapSurfaceFormat: Selected %s for swapchain format", R_CVulkan_FormatToString(available.format));
                        return R_CVULKAN_OK;
                }
        }

        *pOutFormat = pAvailableFormats[0];
        R_CSTL_LOG_INFO ("R_CVulkan_ChooseSwapSurfaceFormat: Fallback to first available format: %s", R_CVulkan_FormatToString (pAvailableFormats[0].format));
        return R_CVULKAN_OK;
}

static enum R_CVulkanError
R_CVulkan_ChooseSwapPresentMode (
    const VkPresentModeKHR* pAvailableModes,
    uint32_t                modeCount,
    VkPresentModeKHR*       pOutMode)
{
        R_CSTL_TRACE_SCOPE ();
        R_CVULKAN_ASSERT (pAvailableModes);
        R_CVULKAN_ASSERT (pOutMode);

        R_CSTL_LOG_DEBUG ("R_CVulkan_ChooseSwapPresentMode: Available present modes: %u", modeCount);

        if (modeCount == 0)
        {
                R_CSTL_LOG_ERROR ("R_CVulkan_ChooseSwapPresentMode: No present modes available");
                return R_CVULKAN_ERROR_INVALID_ARGUMENT;
        }

        for (uint32_t i = 0; i < modeCount; ++i)
        {
                if (pAvailableModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
                {
                        *pOutMode = pAvailableModes[i];
                        R_CSTL_LOG_INFO ("R_CVulkan_ChooseSwapPresentMode: Selected MAILBOX present mode");
                        return R_CVULKAN_OK;
                }
        }

        *pOutMode = VK_PRESENT_MODE_FIFO_KHR;
        R_CSTL_LOG_INFO ("R_CVulkan_ChooseSwapPresentMode: Fallback to FIFO present mode");
        return R_CVULKAN_OK;
}

static VkExtent2D
R_CVulkan_ChooseSwapExtent (const VkSurfaceCapabilitiesKHR* pCapabilities, VkExtent2D requestedExtent)
{
        R_CSTL_TRACE_SCOPE ();
        R_CVULKAN_ASSERT (pCapabilities);

        if (pCapabilities->currentExtent.width != UINT32_MAX)
        {
                R_CSTL_LOG_DEBUG ("R_CVulkan_ChooseSwapExtent: Using current extent: %ux%u", pCapabilities->currentExtent.width, pCapabilities->currentExtent.height);
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

        R_CSTL_LOG_DEBUG ("R_CVulkan_ChooseSwapExtent: Clamped extent from %ux%u to %ux%u", requestedExtent.width, requestedExtent.height, actualExtent.width, actualExtent.height);
        return actualExtent;
}

R_CVULKAN_API enum R_CVulkanError
R_CVulkan_NewSwapchain (
    struct R_CVulkan_Swapchain*                 pSwapchain,
    const struct R_CVulkan_SwapchainCreateInfo* pCreateInfo)
{
        R_CSTL_TRACE_SCOPE ();
        R_CVULKAN_ASSERT (pSwapchain);
        R_CVULKAN_ASSERT (pCreateInfo);

        R_CSTL_LOG_INFO ("R_CVulkan_NewSwapchain: Swapchain info");
        R_CSTL_LOG_DEBUG ("  Context: pSwapchain=%p, pCreateInfo=%p", (void*)pSwapchain, (void*)pCreateInfo);
#if defined(R_CVULKAN_DEBUG)
        if (!pCreateInfo->pDevice || !R_CVulkan_DeviceIsInitialized (pCreateInfo->pDevice))
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }
        if (!pCreateInfo->pSurface || !R_CVulkan_SurfaceIsInitialized (pCreateInfo->pSurface))
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }
#endif

        pSwapchain->device = R_CVulkan_DeviceGetLogicalDevice (pCreateInfo->pDevice);
        pSwapchain->handle = VK_NULL_HANDLE;
        pSwapchain->imageFormat = VK_FORMAT_UNDEFINED;
        pSwapchain->extent.width = 0;
        pSwapchain->extent.height = 0;
        pSwapchain->imageCount = 0;
#if defined(R_CVULKAN_DEBUG)
        pSwapchain->booted = false;
#endif

        VkPhysicalDevice physicalDevice = R_CVulkan_DeviceGetPhysicalDevice (pCreateInfo->pDevice);
        VkSurfaceKHR     surface = R_CVulkan_SurfaceGetHandle (pCreateInfo->pSurface);

        R_CSTL_LOG_DEBUG ("  Physical device: %p, Surface: %p", (void*)physicalDevice, (void*)surface);

        VkSurfaceCapabilitiesKHR capabilities;
        VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR (physicalDevice, surface, &capabilities);
        if (result != VK_SUCCESS)
        {
                R_CSTL_LOG_ERROR ("R_CVulkan_NewSwapchain: Failed to get surface capabilities: %d", result);
                return R_CVulkan_ResultToError (result);
        }
        R_CSTL_LOG_DEBUG ("  Surface capabilities: minImageCount=%u, maxImageCount=%u, currentExtent=%ux%u", capabilities.minImageCount, capabilities.maxImageCount, capabilities.currentExtent.width, capabilities.currentExtent.height);
        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR (physicalDevice, surface, &formatCount, NULL);
        R_CSTL_LOG_DEBUG ("  Surface format count: %u", formatCount);
        VkSurfaceFormatKHR* pFormats
            = (VkSurfaceFormatKHR*)R_CSTL_HeapAlloc (formatCount * sizeof (VkSurfaceFormatKHR));
        if (!pFormats)
        {
                R_CSTL_LOG_ERROR ("R_CVulkan_NewSwapchain: Failed to allocate surface formats array");
                return R_CVULKAN_ERROR_OUT_OF_MEMORY;
        }
        vkGetPhysicalDeviceSurfaceFormatsKHR (physicalDevice, surface, &formatCount, pFormats);

        uint32_t presentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR (physicalDevice, surface, &presentModeCount, NULL);
        R_CSTL_LOG_DEBUG ("  Present mode count: %u", presentModeCount);
        VkPresentModeKHR* pPresentModes
            = (VkPresentModeKHR*)R_CSTL_HeapAlloc (presentModeCount * sizeof (VkPresentModeKHR));
        if (!pPresentModes)
        {
                R_CSTL_LOG_ERROR ("R_CVulkan_NewSwapchain: Failed to allocate present modes array");
                R_CSTL_HeapFree (pFormats);
                return R_CVULKAN_ERROR_OUT_OF_MEMORY;
        }
        vkGetPhysicalDeviceSurfacePresentModesKHR (physicalDevice, surface, &presentModeCount, pPresentModes);

        VkSurfaceFormatKHR surfaceFormat;
        if (pCreateInfo->surfaceFormat.format != VK_FORMAT_UNDEFINED)
        {
                surfaceFormat = pCreateInfo->surfaceFormat;
                R_CSTL_LOG_INFO ("  Using user-specified format: %s", R_CVulkan_FormatToString (surfaceFormat.format));
        }
        else
        {
                enum R_CVulkanError err
                    = R_CVulkan_ChooseSwapSurfaceFormat (pFormats, formatCount, &surfaceFormat);
                if (err != R_CVULKAN_OK)
                {
                        R_CSTL_LOG_ERROR ("R_CVulkan_NewSwapchain: Failed to choose surface format");
                        R_CSTL_HeapFree (pFormats);
                        R_CSTL_HeapFree (pPresentModes);
                        return err;
                }
        }
        VkPresentModeKHR presentMode;
        presentMode = pCreateInfo->presentMode;

        enum R_CVulkanError err
            = R_CVulkan_ChooseSwapPresentMode (pPresentModes, presentModeCount, &presentMode);
        if (err != R_CVULKAN_OK)
        {
                R_CSTL_LOG_ERROR ("R_CVulkan_NewSwapchain: Failed to choose present mode");
                R_CSTL_HeapFree (pFormats);
                R_CSTL_HeapFree (pPresentModes);
                return err;
        }
        VkExtent2D extent = pCreateInfo->extent;
        if (extent.width == 0 || extent.height == 0)
        {
                extent = R_CVulkan_ChooseSwapExtent (&capabilities, capabilities.currentExtent);
        }
        else
        {
                extent = R_CVulkan_ChooseSwapExtent (&capabilities, extent);
        }
        R_CSTL_LOG_DEBUG ("  Chosen extent: %ux%u", extent.width, extent.height);

        uint32_t imageCount = pCreateInfo->imageCount;
        if (imageCount == 0)
        {
                imageCount = capabilities.minImageCount + 1;
        }
        if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
        {
                imageCount = capabilities.maxImageCount;
        }
        R_CSTL_LOG_DEBUG ("  Image count: %u (requested: %u, min: %u, max: %u)", imageCount, pCreateInfo->imageCount, capabilities.minImageCount, capabilities.maxImageCount);

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

        struct R_CVulkan_QueueFamilyIndices indices;
        enum R_CVulkanError err1 = R_CVulkan_DeviceFindQueueFamilies (physicalDevice, surface, &indices);
        if (err1 != R_CVULKAN_OK)
        {
                R_CSTL_LOG_ERROR ("R_CVulkan_NewSwapchain: Failed to find queue families");
                R_CSTL_HeapFree (pFormats);
                R_CSTL_HeapFree (pPresentModes);
                return err1;
        }
        R_CSTL_LOG_DEBUG ("  Queue families: graphics=%u, present=%u", indices.graphicsFamily, indices.presentFamily);

        uint32_t queueFamilyIndices[] = {indices.graphicsFamily, indices.presentFamily};
        if (indices.graphicsFamily != indices.presentFamily)
        {
                createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                createInfo.queueFamilyIndexCount = 2;
                createInfo.pQueueFamilyIndices = queueFamilyIndices;
                R_CSTL_LOG_DEBUG ("  Using CONCURRENT sharing mode (different queues)");
        }
        else
        {
                createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
                R_CSTL_LOG_DEBUG ("  Using EXCLUSIVE sharing mode (same queue)");
        }

        createInfo.preTransform
            = pCreateInfo->preTransform != 0 ? pCreateInfo->preTransform : capabilities.currentTransform;
        createInfo.compositeAlpha = pCreateInfo->compositeAlpha != 0 ? pCreateInfo->compositeAlpha
                                                                     : VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.clipped = pCreateInfo->clipped != 0 ? pCreateInfo->clipped : VK_TRUE;
        createInfo.oldSwapchain = pCreateInfo->oldSwapchain;

        result = vkCreateSwapchainKHR (pSwapchain->device, &createInfo, NULL, &pSwapchain->handle);

        R_CSTL_HeapFree (pFormats);
        R_CSTL_HeapFree (pPresentModes);

        if (result != VK_SUCCESS)
        {
                R_CSTL_LOG_ERROR ("R_CVulkan_NewSwapchain: Failed to create swapchain: %d", result);
                return R_CVulkan_ResultToError (result);
        }
        R_CSTL_LOG_INFO ("R_CVulkan_NewSwapchain: Swapchain created");
        R_CSTL_LOG_INFO ("  - Handle: %p", (void*)pSwapchain->handle);

        vkGetSwapchainImagesKHR (pSwapchain->device, pSwapchain->handle, &imageCount, NULL);
        pSwapchain->imageCount = imageCount;
        pSwapchain->imageFormat = surfaceFormat.format;
        pSwapchain->extent = extent;
        R_CSTL_LOG_INFO ("  - Final configuration: imageCount=%u, format=%s, extent=%ux%u", imageCount, R_CVulkan_FormatToString (surfaceFormat.format), extent.width, extent.height);

#if defined(R_CVULKAN_DEBUG)
        pSwapchain->booted = true;
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
        pSwapchain->booted = false;
#else
        if (pSwapchain->handle != VK_NULL_HANDLE)
        {
                vkDestroySwapchainKHR (pSwapchain->device, pSwapchain->handle, NULL);
        }
        (void)pSwapchain;
#endif
}

R_CVULKAN_API VkSwapchainKHR
R_CVulkan_SwapchainGetHandle (const struct R_CVulkan_Swapchain* pSwapchain)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pSwapchain );
#endif
        return pSwapchain->handle;
}

R_CVULKAN_API VkDevice
R_CVulkan_SwapchainGetDevice (const struct R_CVulkan_Swapchain* pSwapchain)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pSwapchain );
#endif
        return pSwapchain->device;
}

R_CVULKAN_API VkFormat
R_CVulkan_SwapchainGetImageFormat (const struct R_CVulkan_Swapchain* pSwapchain)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pSwapchain );
#endif
        return pSwapchain->imageFormat;
}

R_CVULKAN_API VkExtent2D
R_CVulkan_SwapchainGetExtent (const struct R_CVulkan_Swapchain* pSwapchain)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pSwapchain );
#endif
        return pSwapchain->extent;
}

R_CVULKAN_API uint32_t
R_CVulkan_SwapchainGetImageCount (const struct R_CVulkan_Swapchain* pSwapchain)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pSwapchain );
#endif
        return pSwapchain->imageCount;
}

R_CVULKAN_API enum R_CVulkanError
R_CVulkan_SwapchainAcquireNextImage (
    struct R_CVulkan_Swapchain* pSwapchain,
    uint64_t                    timeout,
    VkSemaphore                 semaphore,
    VkFence                     fence,
    uint32_t*                   pImageIndex)
{
        R_CVULKAN_ASSERT (pSwapchain);
        R_CVULKAN_ASSERT (pImageIndex);
#if defined(R_CVULKAN_DEBUG)
        if (!R_CVulkan_SwapchainIsInitialized (pSwapchain))
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }
#endif
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
                return R_CVulkan_ResultToError (result);
        }
}

R_CVULKAN_API int
R_CVulkan_SwapchainIsInitialized (const struct R_CVulkan_Swapchain* pSwapchain)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pSwapchain );
        return pSwapchain->booted;
#else
        (void)pSwapchain;
        return pSwapchain->handle != VK_NULL_HANDLE ? 1 : 0;
#endif
}
