#include "rl/Base/Texture2.h"

#include "rl/Base/Game.h"

#include <vulkan/vulkan.h>
#include <stdexcept>
#include <cstring>

namespace Rl::Providers {

VkFormat Texture2::GetVkFormat() const
{
    switch (properties.format) {
        case TextureFormat::RGB8:
            return VK_FORMAT_R8G8B8_UNORM;
        case TextureFormat::RGBA8:
            return VK_FORMAT_R8G8B8A8_UNORM;
        case TextureFormat::RGB16F:
            return VK_FORMAT_R16G16B16_SFLOAT;
        case TextureFormat::RGBA16F:
            return VK_FORMAT_R16G16B16A16_SFLOAT;
        case TextureFormat::RGB32F:
            return VK_FORMAT_R32G32B32_SFLOAT;
        case TextureFormat::RGBA32F:
            return VK_FORMAT_R32G32B32A32_SFLOAT;
        case TextureFormat::R8:
            return VK_FORMAT_R8_UNORM;
        case TextureFormat::RG8:
            return VK_FORMAT_R8G8_UNORM;
        case TextureFormat::R16F:
            return VK_FORMAT_R16_SFLOAT;
        case TextureFormat::RG16F:
            return VK_FORMAT_R16G16_SFLOAT;
        case TextureFormat::DEPTH16:
            return VK_FORMAT_D16_UNORM;
        case TextureFormat::DEPTH24:
            return VK_FORMAT_X8_D24_UNORM_PACK32;
        case TextureFormat::DEPTH32F:
            return VK_FORMAT_D32_SFLOAT;
        default:
            return VK_FORMAT_R8G8B8A8_UNORM;
    }
}

VkFilter Texture2::GetVkFilter(TextureFilter filter) const
{
    switch (filter) {
        case TextureFilter::NEAREST:
            return VK_FILTER_NEAREST;
        case TextureFilter::LINEAR:
            return VK_FILTER_LINEAR;
        case TextureFilter::NEAREST_MIPMAP_NEAREST:
            return VK_FILTER_NEAREST;
        case TextureFilter::LINEAR_MIPMAP_NEAREST:
            return VK_FILTER_LINEAR;
        case TextureFilter::NEAREST_MIPMAP_LINEAR:
            return VK_FILTER_NEAREST;
        case TextureFilter::LINEAR_MIPMAP_LINEAR:
            return VK_FILTER_LINEAR;
        default:
            return VK_FILTER_LINEAR;
    }
}

VkSamplerMipmapMode Texture2::GetVkMipmapMode(TextureFilter filter) const
{
    switch (filter) {
        case TextureFilter::NEAREST:
        case TextureFilter::LINEAR:
            return VK_SAMPLER_MIPMAP_MODE_NEAREST;
        case TextureFilter::NEAREST_MIPMAP_NEAREST:
        case TextureFilter::LINEAR_MIPMAP_NEAREST:
            return VK_SAMPLER_MIPMAP_MODE_NEAREST;
        case TextureFilter::NEAREST_MIPMAP_LINEAR:
        case TextureFilter::LINEAR_MIPMAP_LINEAR:
            return VK_SAMPLER_MIPMAP_MODE_LINEAR;
        default:
            return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    }
}

VkSamplerAddressMode Texture2::GetVkWrapMode(TextureWrap wrap) const
{
    switch (wrap) {
        case TextureWrap::REPEAT:
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case TextureWrap::MIRRORED_REPEAT:
            return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        case TextureWrap::CLAMP_TO_EDGE:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case TextureWrap::CLAMP_TO_BORDER:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        default:
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }
}

void Texture2::CreateVulkanImage(Game::VulkanContext& context)
{
    if (!loaded || binding.vkImage != VK_NULL_HANDLE) {
        return;
    }

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = static_cast<uint32_t>(width);
    imageInfo.extent.height = static_cast<uint32_t>(height);
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = static_cast<uint32_t>(mipmapLevels);
    imageInfo.arrayLayers = 1;
    imageInfo.format = GetVkFormat();
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(context.device, &imageInfo, nullptr, &binding.vkImage) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan image");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(context.device, binding.vkImage, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = 0; // TODO: Find appropriate memory type

    if (vkAllocateMemory(context.device, &allocInfo, nullptr, &binding.vkImageMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate Vulkan image memory");
    }

    vkBindImageMemory(context.device, binding.vkImage, binding.vkImageMemory, 0);
}

void Texture2::UploadTextureData(Game::VulkanContext& context)
{
    if (!loaded || data == nullptr || binding.vkImage == VK_NULL_HANDLE) {
        return;
    }

    // Create staging buffer
    VkBufferCreateInfo stagingBufferInfo{};
    stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingBufferInfo.size = dataSize;
    stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(context.device, &stagingBufferInfo, nullptr, &binding.vkStagingBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create staging buffer");
    }

    VkMemoryRequirements stagingMemRequirements;
    vkGetBufferMemoryRequirements(context.device, binding.vkStagingBuffer, &stagingMemRequirements);

    VkMemoryAllocateInfo stagingAllocInfo{};
    stagingAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    stagingAllocInfo.allocationSize = stagingMemRequirements.size;
    stagingAllocInfo.memoryTypeIndex = 0; // TODO: Find appropriate memory type

    if (vkAllocateMemory(context.device, &stagingAllocInfo, nullptr, &binding.vkStagingBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate staging buffer memory");
    }

    vkBindBufferMemory(context.device, binding.vkStagingBuffer, binding.vkStagingBufferMemory, 0);
    
    // Copy texture data to staging buffer
    void* mappedData;
    vkMapMemory(context.device, binding.vkStagingBufferMemory, 0, dataSize, 0, &mappedData);
    memcpy(mappedData, data, dataSize);
    vkUnmapMemory(context.device, binding.vkStagingBufferMemory);

    // Create command buffer for image layout transitions and copy
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = context.commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(context.device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    // Transition image layout from UNDEFINED to TRANSFER_DST_OPTIMAL
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = binding.vkImage;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = static_cast<uint32_t>(mipmapLevels);
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier);

    // Copy buffer to image
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height),
        1
    };

    vkCmdCopyBufferToImage(commandBuffer,
        binding.vkStagingBuffer,
        binding.vkImage,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region);

    // Generate mipmaps if enabled
    if (mipmapLevels > 1) {
        VkImageMemoryBarrier mipBarrier{};
        mipBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        mipBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        mipBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        mipBarrier.image = binding.vkImage;
        mipBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        mipBarrier.subresourceRange.baseArrayLayer = 0;
        mipBarrier.subresourceRange.layerCount = 1;
        mipBarrier.subresourceRange.levelCount = 1;

        int32_t mipWidth = width;
        int32_t mipHeight = height;

        for (uint32_t i = 1; i < static_cast<uint32_t>(mipmapLevels); i++) {
            // Transition previous mip level to TRANSFER_SRC
            mipBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            mipBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            mipBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            mipBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            mipBarrier.subresourceRange.baseMipLevel = i - 1;

            vkCmdPipelineBarrier(commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &mipBarrier);

            // Blit to generate next mip level
            VkImageBlit blit{};
            blit.srcOffsets[0] = {0, 0, 0};
            blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = {0, 0, 0};
            blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(commandBuffer,
                binding.vkImage,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                binding.vkImage,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &blit,
                VK_FILTER_LINEAR);

            if (mipWidth > 1) mipWidth /= 2;
            if (mipHeight > 1) mipHeight /= 2;
        }

        // Transition all mip levels to SHADER_READ_ONLY
        // First transition all levels except the last one from TRANSFER_SRC_OPTIMAL
        mipBarrier.subresourceRange.baseMipLevel = 0;
        mipBarrier.subresourceRange.levelCount = static_cast<uint32_t>(mipmapLevels) - 1;
        mipBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        mipBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        mipBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        mipBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &mipBarrier);

        // Then transition the last mip level from TRANSFER_DST_OPTIMAL
        mipBarrier.subresourceRange.baseMipLevel = static_cast<uint32_t>(mipmapLevels) - 1;
        mipBarrier.subresourceRange.levelCount = 1;
        mipBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        mipBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        mipBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        mipBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &mipBarrier);
    } else {
        // Transition image layout from TRANSFER_DST_OPTIMAL to SHADER_READ_ONLY_OPTIMAL
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier);
    }

    vkEndCommandBuffer(commandBuffer);

    // Submit command buffer and wait for completion
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(context.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(context.graphicsQueue);

    // Free command buffer
    vkFreeCommandBuffers(context.device, context.commandPool, 1, &commandBuffer);

    // Cleanup staging buffer
    vkDestroyBuffer(context.device, binding.vkStagingBuffer, nullptr);
    vkFreeMemory(context.device, binding.vkStagingBufferMemory, nullptr);
    binding.vkStagingBuffer = VK_NULL_HANDLE;
    binding.vkStagingBufferMemory = VK_NULL_HANDLE;
}

void Texture2::CreateVulkanSampler(Game::VulkanContext& context)
{
    if (binding.vkSampler != VK_NULL_HANDLE) {
        return;
    }

    // Check if sampler anisotropy feature is enabled
    VkPhysicalDeviceFeatures deviceFeatures{};
    vkGetPhysicalDeviceFeatures(context.physicalDevice, &deviceFeatures);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = GetVkFilter(properties.magFilter);
    samplerInfo.minFilter = GetVkFilter(properties.minFilter);
    samplerInfo.addressModeU = GetVkWrapMode(properties.wrapS);
    samplerInfo.addressModeV = GetVkWrapMode(properties.wrapT);
    samplerInfo.addressModeW = GetVkWrapMode(properties.wrapT);
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = GetVkMipmapMode(properties.minFilter);
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<float>(mipmapLevels);

    if (vkCreateSampler(context.device, &samplerInfo, nullptr, &binding.vkSampler) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan sampler");
    }
}

void Texture2::GetSampler(Game::VulkanContext& context)
{
    if (binding.vkSampler == VK_NULL_HANDLE) {
        CreateVulkanSampler(context);
    }
}

void Texture2::GetImageView(Game::VulkanContext& context)
{
    if (binding.vkImageView != VK_NULL_HANDLE) {
        return;
    }

    if (binding.vkImage == VK_NULL_HANDLE) {
        CreateVulkanImage(context);
        UploadTextureData(context);
    }

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = binding.vkImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = GetVkFormat();
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = static_cast<uint32_t>(mipmapLevels);
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(context.device, &viewInfo, nullptr, &binding.vkImageView) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan image view");
    }
}

}