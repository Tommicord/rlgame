#include "rlgame.base/cvulkan/cvulkan_image_view.h"
#include "rlgame.base/cvulkan/cvulkan_device.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"

#include <string.h>
#include <stdlib.h>

enum R_CVulkan_Error
R_CVulkan_NewImageView (
    struct R_CVulkan_ImageView*    imageView,
    const struct R_CVulkan_Device* device,
    VkImage                        image,
    VkImageViewType                viewType,
    VkFormat                       format,
    VkComponentMapping             components,
    VkImageSubresourceRange        subresourceRange)
{
        R_CVULKAN_ASSERT (imageView);
        R_CVULKAN_ASSERT (device);
        R_CVULKAN_ASSERT (image != VK_NULL_HANDLE);

#if defined(R_CVULKAN_DEBUG)
        if (!imageView || !device || image == VK_NULL_HANDLE)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }
        if (!R_CVulkan_DeviceIsInitialized (device))
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }
#endif
        imageView->device = R_CVulkan_DeviceGetLogicalDevice (device);
        imageView->handle = VK_NULL_HANDLE;
        imageView->image = image;
        imageView->format = format;
#if defined(R_CVULKAN_DEBUG)
        imageView->isInitialized = false;
#endif

        VkImageViewCreateInfo viewInfo = {0};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = viewType;
        viewInfo.format = format;
        viewInfo.components = components;
        viewInfo.subresourceRange = subresourceRange;

        VkResult result = vkCreateImageView (imageView->device, &viewInfo, NULL, &imageView->handle);
        if (result != VK_SUCCESS)
        {
                return R_CVULKAN_ERROR_FAILED;
        }

#if defined(R_CVULKAN_DEBUG)
        imageView->isInitialized = true;
#endif
        return R_CVULKAN_OK;
}

void
R_CVulkan_DeleteImageView (struct R_CVulkan_ImageView* imageView)
{
        R_CVULKAN_ASSERT (imageView);

#if defined(R_CVULKAN_DEBUG)
        if (!imageView)
        {
                return;
        }
#endif
        if (imageView->handle != VK_NULL_HANDLE)
        {
                vkDestroyImageView (imageView->device, imageView->handle, NULL);
                imageView->handle = VK_NULL_HANDLE;
        }
#if defined(R_CVULKAN_DEBUG)
        imageView->isInitialized = false;
#endif
}

VkImageView
R_CVulkan_ImageViewGetHandle (const struct R_CVulkan_ImageView* imageView)
{
        return imageView ? imageView->handle : VK_NULL_HANDLE;
}

VkDevice
R_CVulkan_ImageViewGetDevice (const struct R_CVulkan_ImageView* imageView)
{
        return imageView ? imageView->device : VK_NULL_HANDLE;
}

VkImage
R_CVulkan_ImageViewGetImage (const struct R_CVulkan_ImageView* imageView)
{
        return imageView ? imageView->image : VK_NULL_HANDLE;
}

VkFormat
R_CVulkan_ImageViewGetFormat (const struct R_CVulkan_ImageView* imageView)
{
#if defined(R_CVULKAN_DEBUG)
        return imageView ? imageView->isInitialized : 0;
#else
        return imageView ? imageView->format : VK_FORMAT_UNDEFINED;
#endif
}

int
R_CVulkan_ImageViewIsInitialized (const struct R_CVulkan_ImageView* imageView)
{
#if defined(R_CVULKAN_DEBUG)
        return imageView ? imageView->isInitialized : 0;
#else
        (void)imageView;
        return 1;
#endif
}
