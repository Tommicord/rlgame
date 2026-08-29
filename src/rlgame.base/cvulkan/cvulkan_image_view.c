#include "rlgame.base/cvulkan/cvulkan_image_view.h"
#include "rlgame.base/cvulkan/cvulkan_device.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"

#include <string.h>
#include <stdlib.h>

enum R_CVulkanError
R_CVulkan_NewImageView (
    struct R_CVulkan_ImageView*                 pImageView,
    const struct R_CVulkan_ImageViewCreateInfo* pCreateInfo)
{
    R_CVULKAN_ASSERT (pImageView);
    R_CVULKAN_ASSERT (pCreateInfo);
    R_CVULKAN_ASSERT (pCreateInfo->pDevice);
    R_CVULKAN_ASSERT (pCreateInfo->image != VK_NULL_HANDLE);

#if defined(R_CVULKAN_DEBUG)
    if (!R_CVulkan_DeviceIsInitialized (pCreateInfo->pDevice))
    {
        return R_CVULKAN_ERROR_NOT_INITIALIZED;
    }
#endif
    pImageView->device = R_CVulkan_DeviceGetLogicalDevice (pCreateInfo->pDevice);
    pImageView->handle = VK_NULL_HANDLE;
    pImageView->image = pCreateInfo->image;
    pImageView->format = pCreateInfo->format;
#if defined(R_CVULKAN_DEBUG)
    
#endif
    VkImageViewCreateInfo viewInfo = {0};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = pCreateInfo->image;
    viewInfo.viewType = pCreateInfo->viewType;
    viewInfo.format = pCreateInfo->format;
    viewInfo.components = pCreateInfo->components;
    viewInfo.subresourceRange = pCreateInfo->subresourceRange;

    VkResult result = vkCreateImageView (pImageView->device, &viewInfo, NULL, &pImageView->handle);
    if (result != VK_SUCCESS)
    {
        return R_CVULKAN_ERROR_FAILED;
    }

#if defined(R_CVULKAN_DEBUG)
    
#endif
    return R_CVULKAN_OK;
}

void
R_CVulkan_DeleteImageView (struct R_CVulkan_ImageView* pImageView)
{
    R_CVULKAN_ASSERT (pImageView);

    vkDestroyImageView (pImageView->device, pImageView->handle, NULL);
#if defined(R_CVULKAN_DEBUG)
    
#endif
}

VkImageView
R_CVulkan_ImageViewGetHandle (const struct R_CVulkan_ImageView* pImageView)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pImageView);
#endif
    return pImageView->handle;
}

VkDevice
R_CVulkan_ImageViewGetDevice (const struct R_CVulkan_ImageView* pImageView)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pImageView);
#endif
    return pImageView->device;
}

VkImage
R_CVulkan_ImageViewGetImage (const struct R_CVulkan_ImageView* pImageView)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pImageView);
#endif
    return pImageView->image;
}

VkFormat
R_CVulkan_ImageViewGetFormat (const struct R_CVulkan_ImageView* pImageView)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pImageView);
#endif
    return pImageView->format;
}

int
R_CVulkan_ImageViewIsInitialized (const struct R_CVulkan_ImageView* pImageView)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pImageView);
    return 1;
#else
    (void)pImageView;
    return 1;
#endif
}
