#include "rlgame.base/cvulkan/cvulkan_image.h"
#include "rlgame.base/cvulkan/cvulkan_device.h"
#include "rlgame.base/cvulkan/cvulkan_memory_allocator.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"

#include <string.h>
#include <stdlib.h>

enum R_CVulkan_Error
R_CVulkan_Image_Init (
    struct R_CVulkan_Image*        image,
    const struct R_CVulkan_Device* device,
    VkPhysicalDevice               physicalDevice,
    VkImageType                    imageType,
    VkExtent3D                     extent,
    uint32_t                       mipLevels,
    uint32_t                       arrayLayers,
    VkFormat                       format,
    VkImageTiling                  tiling,
    R_CVulkanImageUsageFlags       usage,
    R_CVulkanMemoryPropertyFlags   properties,
    VkSampleCountFlagBits          samples)
{
        R_CVULKAN_ASSERT (image);
        R_CVULKAN_ASSERT (device);
        R_CVULKAN_ASSERT (physicalDevice != VK_NULL_HANDLE);


#if defined(R_CVULKAN_DEBUG)
        if (!image || !device || physicalDevice == VK_NULL_HANDLE)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }
        if (!R_CVulkan_Device_IsInitialized (device))
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }
#endif

        if (extent.width == 0 || extent.height == 0 || extent.depth == 0)
        {
                return R_CVULKAN_ERROR_INVALID_ARGUMENT;
        }

        if (mipLevels == 0 || arrayLayers == 0)
        {
                return R_CVULKAN_ERROR_INVALID_ARGUMENT;
        }

        image->device = R_CVulkan_DeviceGetHandle (device);
        image->handle = VK_NULL_HANDLE;
        image->memory = VK_NULL_HANDLE;
        image->size = 0;
        image->width = extent.width;
        image->height = extent.height;
        image->mipLevels = mipLevels;
        image->arrayLayers = arrayLayers;
        image->format = format;
        image->usage = usage;
        image->properties = properties;
        image->currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image->imageType = imageType;
        image->samples = samples;
        image->tiling = tiling;
#if defined(R_CVULKAN_DEBUG)
        image->isInitialized = false;
#endif

        VkImageCreateInfo imageInfo = {0};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = imageType;
        imageInfo.extent = extent;
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = arrayLayers;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = samples;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkResult result = vkCreateImage (image->device, &imageInfo, NULL, &image->handle);
        if (result != VK_SUCCESS)
        {
                return R_CVULKAN_ERROR_IMAGE_CREATE_FAILED;
        }

        enum R_CVulkan_Error error = R_CVulkan_MemoryAllocatorAllocateImageMemory (
            image->device,
            physicalDevice,
            image->handle,
            properties,
            &image->memory);
        if (error != R_CVULKAN_ERROR_OK)
        {
                vkDestroyImage (image->device, image->handle, NULL);
                image->handle = VK_NULL_HANDLE;
                return error;
        }

        result = vkBindImageMemory (image->device, image->handle, image->memory, 0);
        if (result != VK_SUCCESS)
        {
                R_CVulkan_MemoryAllocatorFreeImageMemory (image->device, image->memory);
                vkDestroyImage (image->device, image->handle, NULL);
                image->memory = VK_NULL_HANDLE;
                image->handle = VK_NULL_HANDLE;
                return R_CVULKAN_ERROR_FAILED;
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements (image->device, image->handle, &memRequirements);
        image->size = memRequirements.size;
        image->currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
#if defined(R_CVULKAN_DEBUG)
        image->isInitialized = true;
#endif
        return R_CVULKAN_ERROR_OK;
}

void
R_CVulkan_Image_Shutdown (struct R_CVulkan_Image* image)
{
        R_CVULKAN_ASSERT (image);

#if defined(R_CVULKAN_DEBUG)
        if (!image)
        {
                return;
        }
#endif

        if (image->memory != VK_NULL_HANDLE)
        {
                R_CVulkan_MemoryAllocatorFreeImageMemory (image->device, image->memory);
                image->memory = VK_NULL_HANDLE;
        }

        if (image->handle != VK_NULL_HANDLE)
        {
                vkDestroyImage (image->device, image->handle, NULL);
                image->handle = VK_NULL_HANDLE;
        }

        image->device = VK_NULL_HANDLE;
        image->size = 0;
        image->width = 0;
        image->height = 0;
        image->mipLevels = 0;
        image->arrayLayers = 0;
        image->format = VK_FORMAT_UNDEFINED;
        image->usage = 0;
        image->properties = 0;
        image->currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image->imageType = VK_IMAGE_TYPE_2D;
        image->samples = VK_SAMPLE_COUNT_1_BIT;
        image->tiling = VK_IMAGE_TILING_OPTIMAL;
#if defined(R_CVULKAN_DEBUG)
        image->isInitialized = false;
#endif
}

enum R_CVulkan_Error
R_CVulkan_Image_TransitionLayout (
    struct R_CVulkan_Image*     image,
    VkCommandBuffer             commandBuffer,
    VkImageLayout               oldLayout,
    VkImageLayout               newLayout,
    R_CVulkanPipelineStageFlags srcStageMask,
    R_CVulkanPipelineStageFlags dstStageMask)
{
        R_CVULKAN_ASSERT (image);
        R_CVULKAN_ASSERT (commandBuffer != VK_NULL_HANDLE);

        if (!image || commandBuffer == VK_NULL_HANDLE)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

        if (!image->isInitialized)
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }

        VkImageMemoryBarrier barrier = {0};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image->handle;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = image->mipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = image->arrayLayers;

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

        image->currentLayout = newLayout;
        return R_CVULKAN_ERROR_OK;
}

enum R_CVulkan_Error
R_CVulkan_Image_CopyData (
    struct R_CVulkan_Image* image,
    const void*             data,
    R_CVulkanDeviceSize     dataSize,
    VkBuffer                buffer,
    VkCommandBuffer         commandBuffer)
{
        R_CVULKAN_ASSERT (image);
        R_CVULKAN_ASSERT (data);
        R_CVULKAN_ASSERT (buffer != VK_NULL_HANDLE);
        R_CVULKAN_ASSERT (commandBuffer != VK_NULL_HANDLE);

        if (!image || !data || buffer == VK_NULL_HANDLE || commandBuffer == VK_NULL_HANDLE)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

        if (!image->isInitialized)
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }

        if (dataSize == 0)
        {
                return R_CVULKAN_ERROR_INVALID_ARGUMENT;
        }

        VkBufferImageCopy region = {0};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {image->width, image->height, 1};

        vkCmdCopyBufferToImage (
            commandBuffer,
            buffer,
            image->handle,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region);

        return R_CVULKAN_ERROR_OK;
}

VkImage
R_CVulkan_Image_GetHandle (const struct R_CVulkan_Image* image)
{
        return image ? image->handle : VK_NULL_HANDLE;
}

VkDeviceMemory
R_CVulkan_Image_GetMemory (const struct R_CVulkan_Image* image)
{
        return image ? image->memory : VK_NULL_HANDLE;
}

VkDevice
R_CVulkan_Image_GetDevice (const struct R_CVulkan_Image* image)
{
        return image ? image->device : VK_NULL_HANDLE;
}

uint32_t
R_CVulkan_Image_GetWidth (const struct R_CVulkan_Image* image)
{
        return image ? image->width : 0;
}

uint32_t
R_CVulkan_Image_GetHeight (const struct R_CVulkan_Image* image)
{
        return image ? image->height : 0;
}

VkFormat
R_CVulkan_Image_GetFormat (const struct R_CVulkan_Image* image)
{
        return image ? image->format : VK_FORMAT_UNDEFINED;
}

VkImageLayout
R_CVulkan_Image_GetLayout (const struct R_CVulkan_Image* image)
{
        return image ? image->currentLayout : VK_IMAGE_LAYOUT_UNDEFINED;
}

int
R_CVulkan_Image_IsInitialized (const struct R_CVulkan_Image* image)
{
#if defined(R_CVULKAN_DEBUG)
        return image ? image->isInitialized : 0;
#else
        (void)image;
        return 1;
#endif
}
