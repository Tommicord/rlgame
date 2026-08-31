#include "rlgame.base/cvulkan/cvulkan_image_view.h"
#include "rlgame.base/cvulkan/cvulkan_device.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"

#include <string.h>
#include <stdlib.h>

enum R_CVulkan_Error
r_cvulkan_new_image_view (
    struct R_CVulkan_ImageView*                 pImageView,
    const struct r_cvulkan_image_view_create_info* pCreateInfo)
{
    R_CVULKAN_ASSERT (pImageView);
    R_CVULKAN_ASSERT (pCreateInfo);
    R_CVULKAN_ASSERT (pCreateInfo->pDevice);
    R_CVULKAN_ASSERT (pCreateInfo->image != VK_NULL_HANDLE);

    pImageView->device = r_cvulkan_device_get_logical_device (pCreateInfo->pDevice);
    pImageView->handle = VK_NULL_HANDLE;
    pImageView->image = pCreateInfo->image;
    pImageView->format = pCreateInfo->format;
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
    return R_CVULKAN_OK;
}

void
r_cvulkan_delete_image_view (struct R_CVulkan_ImageView* pImageView)
{
    R_CVULKAN_ASSERT (pImageView);
    vkDestroyImageView (pImageView->device, pImageView->handle, NULL);
#if defined(R_CVULKAN_DEBUG)
    pImageView->device = VK_NULL_HANDLE;
    pImageView->handle = VK_NULL_HANDLE;
    pImageView->format = VK_FORMAT_UNDEFINED;
    pImageView->image = VK_NULL_HANDLE;
#endif
}

VkImageView
r_cvulkan_image_view_get_handle (const struct R_CVulkan_ImageView* pImageView)
{
    R_CVULKAN_ASSERT (pImageView);
    return pImageView->handle;
}

VkDevice
r_cvulkan_image_view_get_device (const struct R_CVulkan_ImageView* pImageView)
{
    R_CVULKAN_ASSERT (pImageView);
    return pImageView->device;
}

VkImage
r_cvulkan_image_view_get_image (const struct R_CVulkan_ImageView* pImageView)
{
    R_CVULKAN_ASSERT (pImageView);
    return pImageView->image;
}

VkFormat
r_cvulkan_image_view_get_format (const struct R_CVulkan_ImageView* pImageView)
{
    R_CVULKAN_ASSERT (pImageView);
    return pImageView->format;
}
