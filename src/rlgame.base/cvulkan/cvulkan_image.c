#include "rlgame.base/cvulkan/cvulkan_image.h"
#include "rlgame.base/cvulkan/cvulkan_device.h"
#include "rlgame.base/cvulkan/cvulkan_memory_allocator.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"

#include <string.h>
#include <stdlib.h>

enum R_CVulkan_Error
R_CVulkan_NewImage (struct R_CVulkan_Image* pImage, const struct R_CVulkan_ImageCreateInfo* pCreateInfo)
{
    R_CVULKAN_ASSERT (pImage);
    R_CVULKAN_ASSERT (pCreateInfo);
    R_CVULKAN_ASSERT (pCreateInfo->device);
    R_CVULKAN_ASSERT (pCreateInfo->physicalDevice);

    if (pCreateInfo->mipLevels == 0 || pCreateInfo->arrayLayers == 0)
    {
        return R_CVULKAN_ERROR_INVALID_ARGUMENT;
    }
    pImage->device = R_CVulkan_DeviceGetLogicalDevice (pCreateInfo->device);
    pImage->width = pCreateInfo->extent.width;
    pImage->height = pCreateInfo->extent.height;
    pImage->mipLevels = pCreateInfo->mipLevels;
    pImage->arrayLayers = pCreateInfo->arrayLayers;
    pImage->format = pCreateInfo->format;
    pImage->usage = pCreateInfo->usage;
    pImage->properties = pCreateInfo->properties;
    pImage->currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    pImage->imageType = pCreateInfo->imageType;
    pImage->samples = pCreateInfo->samples;
    pImage->tiling = pCreateInfo->tiling;
    VkImageCreateInfo imageInfo = {0};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = pCreateInfo->imageType;
    imageInfo.extent = pCreateInfo->extent;
    imageInfo.mipLevels = pCreateInfo->mipLevels;
    imageInfo.arrayLayers = pCreateInfo->arrayLayers;
    imageInfo.format = pCreateInfo->format;
    imageInfo.tiling = pCreateInfo->tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = pCreateInfo->usage;
    imageInfo.samples = pCreateInfo->samples;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateImage (pImage->device, &imageInfo, NULL, &pImage->handle);
    if (result != VK_SUCCESS)
    {
        return R_CVULKAN_ERROR_IMAGE_CREATE_FAILED;
    }

    enum R_CVulkan_Error error = R_CVulkan_MemoryAllocatorAllocateImageMemory (
        pImage->device,
        pCreateInfo->physicalDevice,
        pImage->handle,
        pCreateInfo->properties,
        &pImage->memory);
    if (error != R_CVULKAN_OK)
    {
        vkDestroyImage (pImage->device, pImage->handle, NULL);
        pImage->handle = VK_NULL_HANDLE;
        return error;
    }

    result = vkBindImageMemory (pImage->device, pImage->handle, pImage->memory, 0);
    if (result != VK_SUCCESS)
    {
        R_CVulkan_MemoryAllocatorFreeImageMemory (pImage->device, pImage->memory);
        vkDestroyImage (pImage->device, pImage->handle, NULL);
        pImage->memory = VK_NULL_HANDLE;
        pImage->handle = VK_NULL_HANDLE;
        return R_CVULKAN_ERROR_FAILED;
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements (pImage->device, pImage->handle, &memRequirements);
    pImage->size = memRequirements.size;
    pImage->currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
#if defined(R_CVULKAN_DEBUG)
    
#endif
    return R_CVULKAN_OK;
}

void
R_CVulkan_DeleteImage (struct R_CVulkan_Image* pImage)
{
    R_CVULKAN_ASSERT (pImage);
    R_CVulkan_MemoryAllocatorFreeImageMemory (pImage->device, pImage->memory);

    vkDestroyImage (pImage->device, pImage->handle, NULL);
#if defined(R_CVULKAN_DEBUG)
    memset (pImage, 0, sizeof (struct R_CVulkan_Image));
#endif
}

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_ImageTransitionLayout (
    struct R_CVulkan_Image* pImage,
    VkCommandBuffer         commandBuffer,
    VkImageLayout           oldLayout,
    VkImageLayout           newLayout,
    VkPipelineStageFlags    srcStageMask,
    VkPipelineStageFlags    dstStageMask)
{
    R_CVULKAN_ASSERT (pImage);
    R_CVULKAN_ASSERT (commandBuffer != VK_NULL_HANDLE);
#if defined(R_CVULKAN_DEBUG)
    
    {
        return R_CVULKAN_ERROR_NOT_INITIALIZED;
    }
#endif

    VkImageMemoryBarrier barrier = {0};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = pImage->handle;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = pImage->mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = pImage->arrayLayers;

    VkAccessFlags srcAccessMask = 0;
    VkAccessFlags dstAccessMask = 0;

    if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    }

    if (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    }

    barrier.srcAccessMask = srcAccessMask;
    barrier.dstAccessMask = dstAccessMask;

    vkCmdPipelineBarrier (commandBuffer, srcStageMask, dstStageMask, 0, 0, NULL, 0, NULL, 1, &barrier);

    pImage->currentLayout = newLayout;
    return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_ImageCopyData (
    struct R_CVulkan_Image* pImage,
    const void*             data,
    VkDeviceSize            dataSize,
    VkBuffer                buffer,
    VkCommandBuffer         commandBuffer)
{
    R_CVULKAN_ASSERT (pImage);
    R_CVULKAN_ASSERT (data);
    R_CVULKAN_ASSERT (buffer != VK_NULL_HANDLE);
    R_CVULKAN_ASSERT (commandBuffer != VK_NULL_HANDLE);
#if defined(R_CVULKAN_DEBUG)
    if (!pImage || !data || buffer == VK_NULL_HANDLE || commandBuffer == VK_NULL_HANDLE)
    {
        return R_CVULKAN_ERROR_NULL_POINTER;
    }

    
    {
        return R_CVULKAN_ERROR_NOT_INITIALIZED;
    }

    if (dataSize == 0)
    {
        return R_CVULKAN_ERROR_INVALID_ARGUMENT;
    }
#endif
    VkBufferImageCopy region = {0};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = (VkOffset3D){0, 0, 0};
    region.imageExtent = (VkExtent3D){pImage->width, pImage->height, 1};

    vkCmdCopyBufferToImage (
        commandBuffer,
        buffer,
        pImage->handle,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region);

    return R_CVULKAN_OK;
}

R_CVULKAN_API VkImage
R_CVulkan_ImageGetHandle (const struct R_CVulkan_Image* pImage)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pImage);
#endif
    return pImage->handle;
}

R_CVULKAN_API VkDeviceMemory
R_CVulkan_ImageGetMemory (const struct R_CVulkan_Image* pImage)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pImage);
#endif
    return pImage->memory;
}

R_CVULKAN_API VkDevice
R_CVulkan_ImageGetDevice (const struct R_CVulkan_Image* pImage)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pImage);
#endif
    return pImage->device;
}

R_CVULKAN_API uint32_t
R_CVulkan_ImageGetWidth (const struct R_CVulkan_Image* pImage)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pImage);
#endif
    return pImage->width;
}

R_CVULKAN_API uint32_t
R_CVulkan_ImageGetHeight (const struct R_CVulkan_Image* pImage)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pImage);
#endif
    return pImage->height;
}

R_CVULKAN_API VkFormat
R_CVulkan_ImageGetFormat (const struct R_CVulkan_Image* pImage)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pImage);
#endif
    return pImage->format;
}

R_CVULKAN_API VkImageLayout
R_CVulkan_ImageGetLayout (const struct R_CVulkan_Image* pImage)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pImage);
#endif
    return pImage->currentLayout;
}

R_CVULKAN_API int
R_CVulkan_ImageIsInitialized (const struct R_CVulkan_Image* pImage)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pImage);
    return 1;
#else
    (void)pImage;
    return 1;
#endif
}
