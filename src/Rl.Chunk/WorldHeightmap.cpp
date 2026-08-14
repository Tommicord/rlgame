#include "Rl.Chunk/WorldHeightmap.h"
#include "Rl.Chunk/WorldHeightmapUnpackRImg.h"
#include "Rl.Chunk/ChunkNoiseGenerator.h"
#include "Rl.Base/GameVulkanFence.h"
#include "Rl.Base/GameVulkanCommandBuffer.h"
#include "Rl.Base/GameVulkanImage.h"
#include "Rl.Base/GameVulkanImageView.h"

#include "Rl.Base/GameError.h"
#include "Rl.Base/GameVulkanBuffer.h"
#include "Rl.Base/GameVulkanQueueSubmitter.h"
#include "Rl.Base/GameDevice.h"

#if defined(_RL_SIMD_X86)
#include <immintrin.h>
#elif defined(_RL_SIMD_ARM_NEON)
#include <arm_neon.h>
#endif
#include <array>
#include <cstdint>
#include <stdexcept>
#include <vulkan/vulkan.hpp>

namespace rl
{
extern const uint32_t WorldHeightmapComp_data[];
extern const uint32_t WorldHeightmapComp_size;

WorldHeightmap::WorldHeightmap(uint32_t                width,
                               uint32_t                height,
                               uint32_t                depth,
                               uint32_t                seed,
                               GameDeviceInstance& instance) :
    ChunkNoiseGenerator(seed, instance), instance(instance.getInstance()),
    device(instance.getDevice()), physicalDevice(instance.getPhysicalDevice()),
    completionSemaphore(instance.getDevice(),
                        GameVulkanSemaphoreCreateInfo{VK_SEMAPHORE_TYPE_BINARY}),
    completionFence(instance.getDevice(), GameVulkanFenceCreateInfo{VK_FENCE_CREATE_SIGNALED_BIT}),
    computeQueue(instance.getGraphicsQueue()),
    commandPool(instance.getDevice(), instance.getGraphicsFamily()),
    commandBuffer(instance.getDevice(), commandPool.getCommandPool()),
    heightmapBuffer(&memoryAllocator,
                    width * height * depth * sizeof(float) * 3,
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    width(width), height(height), depth(depth)
{
        createBasemapImage(device, instance.getPhysicalDevice());
        createDeepmapImage(device, instance.getPhysicalDevice());
        createBasemapImageView(device);
        createDeepmapImageView(device);

        basemapElevationImageHandle.setHandle(static_cast<void*>(&basemapElevationImage));
        basemapElevationImageHandle.setWidth(width);
        basemapElevationImageHandle.setHeight(height);
        basemapElevationImageHandle.setDepth(depth);
        basemapElevationImageHandle.setFormat(GameOpaqueImageFormat::R32_SFLOAT);
        basemapElevationImageHandle.setType(GameOpaqueImageType::Image3D);
        basemapElevationImageHandle.setUsage(GameOpaqueImageUsage::Storage);

        basemapTemperatureImageHandle.setHandle(static_cast<void*>(&basemapTemperatureImage));
        basemapTemperatureImageHandle.setWidth(width);
        basemapTemperatureImageHandle.setHeight(height);
        basemapTemperatureImageHandle.setDepth(depth);
        basemapTemperatureImageHandle.setFormat(GameOpaqueImageFormat::R32_SFLOAT);
        basemapTemperatureImageHandle.setType(GameOpaqueImageType::Image3D);
        basemapTemperatureImageHandle.setUsage(GameOpaqueImageUsage::Storage);

        basemapMoistureImageHandle.setHandle(static_cast<void*>(&basemapMoistureImage));
        basemapMoistureImageHandle.setWidth(width);
        basemapMoistureImageHandle.setHeight(height);
        basemapMoistureImageHandle.setDepth(depth);
        basemapMoistureImageHandle.setFormat(GameOpaqueImageFormat::R32_SFLOAT);
        basemapMoistureImageHandle.setType(GameOpaqueImageType::Image3D);
        basemapMoistureImageHandle.setUsage(GameOpaqueImageUsage::Storage);

        deepmapElevationImageHandle.setHandle(static_cast<void*>(&deepmapElevationImage));
        deepmapElevationImageHandle.setWidth(width);
        deepmapElevationImageHandle.setHeight(height);
        deepmapElevationImageHandle.setDepth(depth);
        deepmapElevationImageHandle.setFormat(GameOpaqueImageFormat::R32_SFLOAT);
        deepmapElevationImageHandle.setType(GameOpaqueImageType::Image3D);
        deepmapElevationImageHandle.setUsage(GameOpaqueImageUsage::Storage);

        deepmapTemperatureImageHandle.setHandle(static_cast<void*>(&deepmapTemperatureImage));
        deepmapTemperatureImageHandle.setWidth(width);
        deepmapTemperatureImageHandle.setHeight(height);
        deepmapTemperatureImageHandle.setDepth(depth);
        deepmapTemperatureImageHandle.setFormat(GameOpaqueImageFormat::R32_SFLOAT);
        deepmapTemperatureImageHandle.setType(GameOpaqueImageType::Image3D);
        deepmapTemperatureImageHandle.setUsage(GameOpaqueImageUsage::Storage);

        deepmapMoistureImageHandle.setHandle(static_cast<void*>(&deepmapMoistureImage));
        deepmapMoistureImageHandle.setWidth(width);
        deepmapMoistureImageHandle.setHeight(height);
        deepmapMoistureImageHandle.setDepth(depth);
        deepmapMoistureImageHandle.setFormat(GameOpaqueImageFormat::R32_SFLOAT);
        deepmapMoistureImageHandle.setType(GameOpaqueImageType::Image3D);
        deepmapMoistureImageHandle.setUsage(GameOpaqueImageUsage::Storage);

        completionHandle.setHandle(static_cast<void*>(&completionSemaphore));
        completionHandle.setType(GameOpaqueSyncHandleType::Semaphore);
        completionHandle.setState(GameOpaqueSyncHandleState::Unsignaled);

        createDescriptorSetLayout(device);
        createDescriptorPool(device);
        createDescriptorSets(device);
        createComputePipeline(device);

        updatePermutationBuffers(device, instance.getPhysicalDevice());
}

void WorldHeightmap::dispatch(void*                      pResource,
                              const GameVulkanSemaphore& waitSemaphore,
                              GameVulkanFence&           fence)
{
        WorldHeightmapComputePResource* pOriginalResource =
            static_cast<WorldHeightmapComputePResource*>(pResource);
        WorldHeightmapPushConstants& params = *(pOriginalResource->pParams);

        std::scoped_lock lock(generateMutex);

        completionFence.wait();
        completionFence.reset();

        commandBuffer.reset();
        commandBuffer.begin();

        vkCmdBindPipeline(commandBuffer.getCommandBuffer(), VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipeline);
        vkCmdBindDescriptorSets(commandBuffer.getCommandBuffer(), VK_PIPELINE_BIND_POINT_COMPUTE,
                                pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
        vkCmdPushConstants(commandBuffer.getCommandBuffer(), pipelineLayout,
                           VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(WorldHeightmapPushConstants),
                           &params);

        uint32_t groupCountX = (params.width + 7) / 8;
        uint32_t groupCountY = (params.height + 7) / 8;
        uint32_t groupCountZ = (params.depth + 3) / 4;

        vkCmdDispatch(commandBuffer.getCommandBuffer(), groupCountX, groupCountY, groupCountZ);

        VkMemoryBarrier memoryBarrier{};
        memoryBarrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT;
        vkCmdPipelineBarrier(commandBuffer.getCommandBuffer(), VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT |
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);

        commandBuffer.end();

        VkSubmitInfo submitInfo{};
        submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount   = 1;
        const VkCommandBuffer cmdBuffer = commandBuffer.getCommandBuffer();
        submitInfo.pCommandBuffers      = &cmdBuffer;

        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        const VkSemaphore    waitSem   = waitSemaphore.getSemaphore();
        if (waitSem != VK_NULL_HANDLE)
        {
                submitInfo.waitSemaphoreCount = 1;
                submitInfo.pWaitSemaphores    = &waitSem;
                submitInfo.pWaitDstStageMask  = &waitStage;
        }
        else
        {
                submitInfo.waitSemaphoreCount = 0;
                submitInfo.pWaitSemaphores    = nullptr;
                submitInfo.pWaitDstStageMask  = nullptr;
        }
        const VkSemaphore semaphore = completionSemaphore.getSemaphore();
        if (semaphore != VK_NULL_HANDLE)
        {
                submitInfo.signalSemaphoreCount = 1;
                submitInfo.pSignalSemaphores    = &semaphore;
        }
        else
        {
                submitInfo.signalSemaphoreCount = 0;
                submitInfo.pSignalSemaphores    = nullptr;
        }

        GameVulkanQueueSubmitter::submit(computeQueue, &submitInfo, completionFence.getFence());
}

void WorldHeightmap::createDescriptorSetLayout(VkDevice device)
{
        std::array<VkDescriptorSetLayoutBinding, 8> bindings{};

        bindings[0].binding         = 0;
        bindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

        bindings[1].binding         = 1;
        bindings[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[1].descriptorCount = 1;
        bindings[1].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

        // Basemap images (elevation, temperature, moisture)
        bindings[2].binding         = 2;
        bindings[2].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        bindings[2].descriptorCount = 1;
        bindings[2].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

        bindings[3].binding         = 3;
        bindings[3].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        bindings[3].descriptorCount = 1;
        bindings[3].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

        bindings[4].binding         = 4;
        bindings[4].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        bindings[4].descriptorCount = 1;
        bindings[4].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

        // Deepmap images (elevation, temperature, moisture)
        bindings[5].binding         = 5;
        bindings[5].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        bindings[5].descriptorCount = 1;
        bindings[5].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

        bindings[6].binding         = 6;
        bindings[6].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        bindings[6].descriptorCount = 1;
        bindings[6].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

        bindings[7].binding         = 7;
        bindings[7].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        bindings[7].descriptorCount = 1;
        bindings[7].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings    = bindings.data();

        VkResult result =
            vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout);
        if (result != VK_SUCCESS)
        {
                GameError::exitWithError(
                    "vkCreateDescriptorSetLayout",
                    "Failed to create descriptor set layout "
                    "(result = " +
                        GameError::vulkanResultToString(result) + ")",
                    device, physicalDevice, instance);
        }
}

void WorldHeightmap::createDescriptorPool(VkDevice device)
{
        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSizes[0].descriptorCount = 2;
        poolSizes[1].type            = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        poolSizes[1].descriptorCount = 6;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes    = poolSizes.data();
        poolInfo.maxSets       = 1;

        VkResult result = vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool);
        if (result != VK_SUCCESS)
        {
                GameError::exitWithError(
                    "vkCreateDescriptorPool",
                    "Failed to create descriptor pool "
                    "(result = " +
                        GameError::vulkanResultToString(result) + ")",
                    device, physicalDevice, instance);
        }
}

void WorldHeightmap::createDescriptorSets(VkDevice device)
{
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool     = descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts        = &descriptorSetLayout;

        VkResult result = vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet);
        if (result != VK_SUCCESS)
        {
                GameError::exitWithError(
                    "vkAllocateDescriptorSets",
                    "Failed to allocate descriptor sets "
                    "(result = " +
                        GameError::vulkanResultToString(result) + ")",
                    device, physicalDevice, instance);
        }

        VkDescriptorBufferInfo permBufferInfo{};
        permBufferInfo.buffer = permBuffer.getBuffer();
        permBufferInfo.offset = permBuffer.getOffset();
        permBufferInfo.range  = permBufferSize * sizeof(int32_t);

        VkDescriptorBufferInfo permGradBufferInfo{};
        permGradBufferInfo.buffer = permGradBuffer.getBuffer();
        permGradBufferInfo.offset = permGradBuffer.getOffset();
        permGradBufferInfo.range  = permBufferSize * sizeof(int32_t);

        // Basemap image infos
        VkDescriptorImageInfo basemapElevationImageInfo{};
        basemapElevationImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        basemapElevationImageInfo.imageView   = basemapElevationImageView.getImageView();
        basemapElevationImageInfo.sampler     = VK_NULL_HANDLE;

        VkDescriptorImageInfo basemapTemperatureImageInfo{};
        basemapTemperatureImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        basemapTemperatureImageInfo.imageView   = basemapTemperatureImageView.getImageView();
        basemapTemperatureImageInfo.sampler     = VK_NULL_HANDLE;

        VkDescriptorImageInfo basemapMoistureImageInfo{};
        basemapMoistureImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        basemapMoistureImageInfo.imageView   = basemapMoistureImageView.getImageView();
        basemapMoistureImageInfo.sampler     = VK_NULL_HANDLE;

        // Deepmap image infos
        VkDescriptorImageInfo deepmapElevationImageInfo{};
        deepmapElevationImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        deepmapElevationImageInfo.imageView   = deepmapElevationImageView.getImageView();
        deepmapElevationImageInfo.sampler     = VK_NULL_HANDLE;

        VkDescriptorImageInfo deepmapTemperatureImageInfo{};
        deepmapTemperatureImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        deepmapTemperatureImageInfo.imageView   = deepmapTemperatureImageView.getImageView();
        deepmapTemperatureImageInfo.sampler     = VK_NULL_HANDLE;

        VkDescriptorImageInfo deepmapMoistureImageInfo{};
        deepmapMoistureImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        deepmapMoistureImageInfo.imageView   = deepmapMoistureImageView.getImageView();
        deepmapMoistureImageInfo.sampler     = VK_NULL_HANDLE;

        std::array<VkWriteDescriptorSet, 8> descriptorWrites{};

        descriptorWrites[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet          = descriptorSet;
        descriptorWrites[0].dstBinding      = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo     = &permBufferInfo;

        descriptorWrites[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet          = descriptorSet;
        descriptorWrites[1].dstBinding      = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pBufferInfo     = &permGradBufferInfo;

        descriptorWrites[2].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet          = descriptorSet;
        descriptorWrites[2].dstBinding      = 2;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].pImageInfo      = &basemapElevationImageInfo;

        descriptorWrites[3].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[3].dstSet          = descriptorSet;
        descriptorWrites[3].dstBinding      = 3;
        descriptorWrites[3].dstArrayElement = 0;
        descriptorWrites[3].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        descriptorWrites[3].descriptorCount = 1;
        descriptorWrites[3].pImageInfo      = &basemapTemperatureImageInfo;

        descriptorWrites[4].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[4].dstSet          = descriptorSet;
        descriptorWrites[4].dstBinding      = 4;
        descriptorWrites[4].dstArrayElement = 0;
        descriptorWrites[4].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        descriptorWrites[4].descriptorCount = 1;
        descriptorWrites[4].pImageInfo      = &basemapMoistureImageInfo;

        descriptorWrites[5].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[5].dstSet          = descriptorSet;
        descriptorWrites[5].dstBinding      = 5;
        descriptorWrites[5].dstArrayElement = 0;
        descriptorWrites[5].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        descriptorWrites[5].descriptorCount = 1;
        descriptorWrites[5].pImageInfo      = &deepmapElevationImageInfo;

        descriptorWrites[6].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[6].dstSet          = descriptorSet;
        descriptorWrites[6].dstBinding      = 6;
        descriptorWrites[6].dstArrayElement = 0;
        descriptorWrites[6].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        descriptorWrites[6].descriptorCount = 1;
        descriptorWrites[6].pImageInfo      = &deepmapTemperatureImageInfo;

        descriptorWrites[7].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[7].dstSet          = descriptorSet;
        descriptorWrites[7].dstBinding      = 7;
        descriptorWrites[7].dstArrayElement = 0;
        descriptorWrites[7].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        descriptorWrites[7].descriptorCount = 1;
        descriptorWrites[7].pImageInfo      = &deepmapMoistureImageInfo;

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()),
                               descriptorWrites.data(), 0, nullptr);
}

void WorldHeightmap::createComputePipeline(VkDevice device)
{
        computeShaderModule = GameShaderLoader::createShaderModule(device, WorldHeightmapComp_data,
                                                                   WorldHeightmapComp_size);

        VkPipelineShaderStageCreateInfo shaderStageInfo{};
        shaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageInfo.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
        shaderStageInfo.module = computeShaderModule.shaderModule;
        shaderStageInfo.pName  = "main";

        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        pushConstantRange.offset     = 0;
        pushConstantRange.size       = sizeof(WorldHeightmapPushConstants);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount         = 1;
        pipelineLayoutInfo.pSetLayouts            = &descriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges    = &pushConstantRange;

        VkResult result1 =
            vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout);
        if (result1 != VK_SUCCESS)
        {
                GameError::exitWithError(
                    "vkCreatePipelineLayout",
                    "Failed to create compute pipeline layout"
                    "(result = " +
                        GameError::vulkanResultToString(result1) + ")",
                    device, physicalDevice, instance);
        }

        VkComputePipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.stage  = shaderStageInfo;
        pipelineInfo.layout = pipelineLayout;

        VkResult result2 =
            vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline);
        if (result2 != VK_SUCCESS)
        {
                GameError::exitWithError(
                    "vkCreateComputePipelines",
                    "Failed to create compute pipeline "
                    "(result = " +
                        GameError::vulkanResultToString(result2) + ")",
                    device, physicalDevice, instance);
        }
}

void WorldHeightmap::createBasemapImage(VkDevice device, VkPhysicalDevice physicalDevice)
{
        createBasemapChannelImage(device, physicalDevice, basemapElevationImage,
                                  VK_FORMAT_R8_UNORM);
        createBasemapChannelImage(device, physicalDevice, basemapTemperatureImage,
                                  VK_FORMAT_R8_UNORM);
        createBasemapChannelImage(device, physicalDevice, basemapMoistureImage, VK_FORMAT_R8_UNORM);
}

void WorldHeightmap::createBasemapChannelImage(VkDevice         device,
                                               VkPhysicalDevice physicalDevice,
                                               GameVulkanImage& image,
                                               VkFormat         format)
{
        GameVulkanImageCreateInfo imageInfo{};
        imageInfo.imageType     = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width  = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth  = 1;
        imageInfo.mipLevels     = 1;
        imageInfo.arrayLayers   = 1;
        imageInfo.format        = format;
        imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage         = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                          VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        imageInfo.sharingMode      = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples          = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        image                      = GameVulkanImage(device, physicalDevice, imageInfo);

        commandBuffer.reset();
        commandBuffer.begin();

        VkImageMemoryBarrier barrier{};
        barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout                       = VK_IMAGE_LAYOUT_GENERAL;
        barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.image                           = image.getImage();
        barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel   = 0;
        barrier.subresourceRange.levelCount     = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount     = 1;
        barrier.srcAccessMask                   = 0;
        barrier.dstAccessMask                   = VK_ACCESS_SHADER_WRITE_BIT;

        vkCmdPipelineBarrier(commandBuffer.getCommandBuffer(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                             &barrier);

        commandBuffer.end();

        VkSubmitInfo submitInfo{};
        submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount   = 1;
        const VkCommandBuffer cmdBuffer = commandBuffer.getCommandBuffer();
        submitInfo.pCommandBuffers      = &cmdBuffer;

        GameVulkanFence localFence(device, GameVulkanFenceCreateInfo{0});
        GameVulkanQueueSubmitter::submit(computeQueue, &submitInfo, localFence.getFence());

        localFence.wait();
}

void WorldHeightmap::createDeepmapImage(VkDevice device, VkPhysicalDevice physicalDevice)
{
        createDeepmapChannelImage(device, physicalDevice, deepmapElevationImage,
                                  VK_FORMAT_R8_UNORM);
        createDeepmapChannelImage(device, physicalDevice, deepmapTemperatureImage,
                                  VK_FORMAT_R8_UNORM);
        createDeepmapChannelImage(device, physicalDevice, deepmapMoistureImage, VK_FORMAT_R8_UNORM);
}

void WorldHeightmap::createDeepmapChannelImage(VkDevice         device,
                                               VkPhysicalDevice physicalDevice,
                                               GameVulkanImage& image,
                                               VkFormat         format)
{
        GameVulkanImageCreateInfo imageInfo{};
        imageInfo.imageType     = VK_IMAGE_TYPE_3D;
        imageInfo.extent.width  = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth  = depth;
        imageInfo.mipLevels     = 1;
        imageInfo.arrayLayers   = 1;
        imageInfo.format        = format;
        imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage         = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                          VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        imageInfo.sharingMode      = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples          = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        image                      = GameVulkanImage(device, physicalDevice, imageInfo);

        commandBuffer.reset();
        commandBuffer.begin();

        VkImageMemoryBarrier barrier{};
        barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout                       = VK_IMAGE_LAYOUT_GENERAL;
        barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.image                           = image.getImage();
        barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel   = 0;
        barrier.subresourceRange.levelCount     = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount     = 1;
        barrier.srcAccessMask                   = 0;
        barrier.dstAccessMask                   = VK_ACCESS_SHADER_WRITE_BIT;

        vkCmdPipelineBarrier(commandBuffer.getCommandBuffer(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                             &barrier);

        commandBuffer.end();

        VkSubmitInfo submitInfo{};
        submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount   = 1;
        const VkCommandBuffer cmdBuffer = commandBuffer.getCommandBuffer();
        submitInfo.pCommandBuffers      = &cmdBuffer;

        GameVulkanFence localFence(device, GameVulkanFenceCreateInfo{0});
        GameVulkanQueueSubmitter::submit(computeQueue, &submitInfo, localFence.getFence());

        localFence.wait();
}

void WorldHeightmap::createBasemapImageView(VkDevice device)
{
        createBasemapChannelImageView(device, basemapElevationImage, basemapElevationImageView,
                                      VK_FORMAT_R8_UNORM);
        createBasemapChannelImageView(device, basemapTemperatureImage, basemapTemperatureImageView,
                                      VK_FORMAT_R8_UNORM);
        createBasemapChannelImageView(device, basemapMoistureImage, basemapMoistureImageView,
                                      VK_FORMAT_R8_UNORM);
}

void WorldHeightmap::createBasemapChannelImageView(VkDevice             device,
                                                   GameVulkanImage&     image,
                                                   GameVulkanImageView& imageView,
                                                   VkFormat             format)
{
        GameVulkanImageViewCreateInfo viewInfo{};
        viewInfo.image                           = image.getImage();
        viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format                          = format;
        viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel   = 0;
        viewInfo.subresourceRange.levelCount     = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount     = 1;

        imageView = GameVulkanImageView(device, viewInfo);
}

void WorldHeightmap::createDeepmapImageView(VkDevice device)
{
        createDeepmapChannelImageView(device, deepmapElevationImage, deepmapElevationImageView,
                                      VK_FORMAT_R8_UNORM);
        createDeepmapChannelImageView(device, deepmapTemperatureImage, deepmapTemperatureImageView,
                                      VK_FORMAT_R8_UNORM);
        createDeepmapChannelImageView(device, deepmapMoistureImage, deepmapMoistureImageView,
                                      VK_FORMAT_R8_UNORM);
}

void WorldHeightmap::createDeepmapChannelImageView(VkDevice             device,
                                                   GameVulkanImage&     image,
                                                   GameVulkanImageView& imageView,
                                                   VkFormat             format)
{
        GameVulkanImageViewCreateInfo viewInfo{};
        viewInfo.image                           = image.getImage();
        viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_3D;
        viewInfo.format                          = format;
        viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel   = 0;
        viewInfo.subresourceRange.levelCount     = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount     = 1;

        imageView = GameVulkanImageView(device, viewInfo);
}

VkImageView WorldHeightmap::getBasemapElevationImageView() const
{
        return basemapElevationImageView.getImageView();
}

VkImageView WorldHeightmap::getBasemapTemperatureImageView() const
{
        return basemapTemperatureImageView.getImageView();
}

VkImageView WorldHeightmap::getBasemapMoistureImageView() const
{
        return basemapMoistureImageView.getImageView();
}

VkImageView WorldHeightmap::getDeepmapElevationImageView() const
{
        return deepmapElevationImageView.getImageView();
}

VkImageView WorldHeightmap::getDeepmapTemperatureImageView() const
{
        return deepmapTemperatureImageView.getImageView();
}

VkImageView WorldHeightmap::getDeepmapMoistureImageView() const
{
        return deepmapMoistureImageView.getImageView();
}

#if defined(_RL_CHUNK_VULKAN_BACKEND)
void* WorldHeightmap::getBasemapElevationImageViewPtr() const
{
        return const_cast<GameVulkanImageView*>(&basemapElevationImageView);
}

void* WorldHeightmap::getBasemapTemperatureImageViewPtr() const
{
        return const_cast<GameVulkanImageView*>(&basemapTemperatureImageView);
}

void* WorldHeightmap::getBasemapMoistureImageViewPtr() const
{
        return const_cast<GameVulkanImageView*>(&basemapMoistureImageView);
}

void* WorldHeightmap::getDeepmapElevationImageViewPtr() const
{
        return const_cast<GameVulkanImageView*>(&deepmapElevationImageView);
}

void* WorldHeightmap::getDeepmapTemperatureImageViewPtr() const
{
        return const_cast<GameVulkanImageView*>(&deepmapTemperatureImageView);
}

void* WorldHeightmap::getDeepmapMoistureImageViewPtr() const
{
        return const_cast<GameVulkanImageView*>(&deepmapMoistureImageView);
}
#endif

std::recursive_mutex& WorldHeightmap::getGenerateMutex()
{
        return generateMutex;
}

const GameVulkanSemaphore& WorldHeightmap::getCompletionSemaphore() const
{
        return completionSemaphore;
}

GameVulkanSemaphore& WorldHeightmap::getCompletionSemaphore()
{
        return completionSemaphore;
}

const GameVulkanFence& WorldHeightmap::getCompletionFence() const
{
        return completionFence;
}

GameVulkanFence& WorldHeightmap::getCompletionFence()
{
        return completionFence;
}

void WorldHeightmap::read(VkDevice                         device,
                          VkPhysicalDevice                 physicalDevice,
                          std::vector<WorldHeightmapData>& output)
{
        // Add padding for SIMD loads, 16 bytes per channel for safety
        VkDeviceSize bufferSize = (width * height * 3) + 64;
        output.resize(width * height);

        GameVulkanBuffer stagingBuffer(
            &memoryAllocator, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        std::scoped_lock lock(generateMutex);

        commandBuffer.reset();
        commandBuffer.begin();

        std::array<VkImageMemoryBarrier, 3> preCopyBarriers{};

        preCopyBarriers[0].sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        preCopyBarriers[0].oldLayout                       = VK_IMAGE_LAYOUT_GENERAL;
        preCopyBarriers[0].newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        preCopyBarriers[0].srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        preCopyBarriers[0].dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        preCopyBarriers[0].image                           = basemapElevationImage.getImage();
        preCopyBarriers[0].subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        preCopyBarriers[0].subresourceRange.baseMipLevel   = 0;
        preCopyBarriers[0].subresourceRange.levelCount     = 1;
        preCopyBarriers[0].subresourceRange.baseArrayLayer = 0;
        preCopyBarriers[0].subresourceRange.layerCount     = 1;
        preCopyBarriers[0].srcAccessMask                   = VK_ACCESS_SHADER_WRITE_BIT;
        preCopyBarriers[0].dstAccessMask                   = VK_ACCESS_TRANSFER_READ_BIT;

        preCopyBarriers[1].sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        preCopyBarriers[1].oldLayout                       = VK_IMAGE_LAYOUT_GENERAL;
        preCopyBarriers[1].newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        preCopyBarriers[1].srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        preCopyBarriers[1].dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        preCopyBarriers[1].image                           = basemapTemperatureImage.getImage();
        preCopyBarriers[1].subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        preCopyBarriers[1].subresourceRange.baseMipLevel   = 0;
        preCopyBarriers[1].subresourceRange.levelCount     = 1;
        preCopyBarriers[1].subresourceRange.baseArrayLayer = 0;
        preCopyBarriers[1].subresourceRange.layerCount     = 1;
        preCopyBarriers[1].srcAccessMask                   = VK_ACCESS_SHADER_WRITE_BIT;
        preCopyBarriers[1].dstAccessMask                   = VK_ACCESS_TRANSFER_READ_BIT;

        preCopyBarriers[2].sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        preCopyBarriers[2].oldLayout                       = VK_IMAGE_LAYOUT_GENERAL;
        preCopyBarriers[2].newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        preCopyBarriers[2].srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        preCopyBarriers[2].dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        preCopyBarriers[2].image                           = basemapMoistureImage.getImage();
        preCopyBarriers[2].subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        preCopyBarriers[2].subresourceRange.baseMipLevel   = 0;
        preCopyBarriers[2].subresourceRange.levelCount     = 1;
        preCopyBarriers[2].subresourceRange.baseArrayLayer = 0;
        preCopyBarriers[2].subresourceRange.layerCount     = 1;
        preCopyBarriers[2].srcAccessMask                   = VK_ACCESS_SHADER_WRITE_BIT;
        preCopyBarriers[2].dstAccessMask                   = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer.getCommandBuffer(), VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr,
                             static_cast<uint32_t>(preCopyBarriers.size()), preCopyBarriers.data());

        VkBufferImageCopy copyRegion{};
        copyRegion.bufferOffset                    = 0;
        copyRegion.bufferRowLength                 = 0;
        copyRegion.bufferImageHeight               = 0;
        copyRegion.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.mipLevel       = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount     = 1;
        copyRegion.imageOffset                     = {0, 0, 0};
        copyRegion.imageExtent                     = {width, height, 1};

        copyRegion.bufferOffset = 0;
        vkCmdCopyImageToBuffer(commandBuffer.getCommandBuffer(), basemapElevationImage.getImage(),
                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stagingBuffer.getBuffer(), 1,
                               &copyRegion);

        copyRegion.bufferOffset = width * height;
        vkCmdCopyImageToBuffer(commandBuffer.getCommandBuffer(), basemapTemperatureImage.getImage(),
                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stagingBuffer.getBuffer(), 1,
                               &copyRegion);

        copyRegion.bufferOffset = width * height * 2;
        vkCmdCopyImageToBuffer(commandBuffer.getCommandBuffer(), basemapMoistureImage.getImage(),
                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stagingBuffer.getBuffer(), 1,
                               &copyRegion);

        std::array<VkImageMemoryBarrier, 3> postCopyBarriers{};

        postCopyBarriers[0].sType                         = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        postCopyBarriers[0].oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        postCopyBarriers[0].newLayout                     = VK_IMAGE_LAYOUT_GENERAL;
        postCopyBarriers[0].srcQueueFamilyIndex           = VK_QUEUE_FAMILY_IGNORED;
        postCopyBarriers[0].dstQueueFamilyIndex           = VK_QUEUE_FAMILY_IGNORED;
        postCopyBarriers[0].image                         = basemapElevationImage.getImage();
        postCopyBarriers[0].subresourceRange.aspectMask   = VK_IMAGE_ASPECT_COLOR_BIT;
        postCopyBarriers[0].subresourceRange.baseMipLevel = 0;
        postCopyBarriers[0].subresourceRange.levelCount   = 1;
        postCopyBarriers[0].subresourceRange.baseArrayLayer = 0;
        postCopyBarriers[0].subresourceRange.layerCount     = 1;
        postCopyBarriers[0].srcAccessMask                   = VK_ACCESS_TRANSFER_READ_BIT;
        postCopyBarriers[0].dstAccessMask                   = VK_ACCESS_SHADER_WRITE_BIT;

        postCopyBarriers[1].sType                         = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        postCopyBarriers[1].oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        postCopyBarriers[1].newLayout                     = VK_IMAGE_LAYOUT_GENERAL;
        postCopyBarriers[1].srcQueueFamilyIndex           = VK_QUEUE_FAMILY_IGNORED;
        postCopyBarriers[1].dstQueueFamilyIndex           = VK_QUEUE_FAMILY_IGNORED;
        postCopyBarriers[1].image                         = basemapTemperatureImage.getImage();
        postCopyBarriers[1].subresourceRange.aspectMask   = VK_IMAGE_ASPECT_COLOR_BIT;
        postCopyBarriers[1].subresourceRange.baseMipLevel = 0;
        postCopyBarriers[1].subresourceRange.levelCount   = 1;
        postCopyBarriers[1].subresourceRange.baseArrayLayer = 0;
        postCopyBarriers[1].subresourceRange.layerCount     = 1;
        postCopyBarriers[1].srcAccessMask                   = VK_ACCESS_TRANSFER_READ_BIT;
        postCopyBarriers[1].dstAccessMask                   = VK_ACCESS_SHADER_WRITE_BIT;

        postCopyBarriers[2].sType                         = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        postCopyBarriers[2].oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        postCopyBarriers[2].newLayout                     = VK_IMAGE_LAYOUT_GENERAL;
        postCopyBarriers[2].srcQueueFamilyIndex           = VK_QUEUE_FAMILY_IGNORED;
        postCopyBarriers[2].dstQueueFamilyIndex           = VK_QUEUE_FAMILY_IGNORED;
        postCopyBarriers[2].image                         = basemapMoistureImage.getImage();
        postCopyBarriers[2].subresourceRange.aspectMask   = VK_IMAGE_ASPECT_COLOR_BIT;
        postCopyBarriers[2].subresourceRange.baseMipLevel = 0;
        postCopyBarriers[2].subresourceRange.levelCount   = 1;
        postCopyBarriers[2].subresourceRange.baseArrayLayer = 0;
        postCopyBarriers[2].subresourceRange.layerCount     = 1;
        postCopyBarriers[2].srcAccessMask                   = VK_ACCESS_TRANSFER_READ_BIT;
        postCopyBarriers[2].dstAccessMask                   = VK_ACCESS_SHADER_WRITE_BIT;

        vkCmdPipelineBarrier(commandBuffer.getCommandBuffer(), VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr,
                             static_cast<uint32_t>(postCopyBarriers.size()),
                             postCopyBarriers.data());

        commandBuffer.end();

        VkSubmitInfo submitInfo{};
        submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount   = 1;
        const VkCommandBuffer cmdBuffer = commandBuffer.getCommandBuffer();
        submitInfo.pCommandBuffers      = &cmdBuffer;

        GameVulkanFence localFence(device, GameVulkanFenceCreateInfo{0});
        GameVulkanQueueSubmitter::submit(computeQueue, &submitInfo, localFence.getFence());

        localFence.wait();

        void* data;
        vkMapMemory(device, stagingBuffer.getMemory(), stagingBuffer.getOffset(), bufferSize, 0,
                    &data);
        const uint8_t* pixelData = static_cast<const uint8_t*>(data);

        unpackR8ToFloatSIMD(pixelData, output);

        vkUnmapMemory(device, stagingBuffer.getMemory());
}

const GameOpaqueImageHandle& WorldHeightmap::getBasemapElevationImage() const
{
        return basemapElevationImageHandle.getHandleStruct();
}

const GameOpaqueImageHandle& WorldHeightmap::getBasemapTemperatureImage() const
{
        return basemapTemperatureImageHandle.getHandleStruct();
}

const GameOpaqueImageHandle& WorldHeightmap::getBasemapMoistureImage() const
{
        return basemapMoistureImageHandle.getHandleStruct();
}

const GameOpaqueImageHandle& WorldHeightmap::getDeepmapElevationImage() const
{
        return deepmapElevationImageHandle.getHandleStruct();
}

const GameOpaqueImageHandle& WorldHeightmap::getDeepmapTemperatureImage() const
{
        return deepmapTemperatureImageHandle.getHandleStruct();
}

const GameOpaqueImageHandle& WorldHeightmap::getDeepmapMoistureImage() const
{
        return deepmapMoistureImageHandle.getHandleStruct();
}

const GameOpaqueSyncHandle& WorldHeightmap::getCompletionHandle() const
{
        return completionHandle.getHandleStruct();
}

void WorldHeightmap::generateCompressedGrayscale(VkDevice              device,
                                                 VkPhysicalDevice      physicalDevice,
                                                 std::vector<uint8_t>& output)
{
        VkDeviceSize bufferSize = width * height * depth; // R8 = 1 byte per pixel
        output.resize(width * height * depth);

        GameVulkanBuffer stagingBuffer(
            &memoryAllocator, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        std::scoped_lock lock(generateMutex);

        commandBuffer.reset();
        commandBuffer.begin();

        VkImageMemoryBarrier preCopyBarrier{};
        preCopyBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        preCopyBarrier.oldLayout                       = VK_IMAGE_LAYOUT_GENERAL;
        preCopyBarrier.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        preCopyBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        preCopyBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        preCopyBarrier.image                           = deepmapElevationImage.getImage();
        preCopyBarrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        preCopyBarrier.subresourceRange.baseMipLevel   = 0;
        preCopyBarrier.subresourceRange.levelCount     = 1;
        preCopyBarrier.subresourceRange.baseArrayLayer = 0;
        preCopyBarrier.subresourceRange.layerCount     = 1;
        preCopyBarrier.srcAccessMask                   = VK_ACCESS_SHADER_WRITE_BIT;
        preCopyBarrier.dstAccessMask                   = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer.getCommandBuffer(), VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                             &preCopyBarrier);

        VkBufferImageCopy copyRegion{};
        copyRegion.bufferOffset                    = 0;
        copyRegion.bufferRowLength                 = 0;
        copyRegion.bufferImageHeight               = 0;
        copyRegion.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.mipLevel       = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount     = 1;
        copyRegion.imageOffset                     = {0, 0, 0};
        copyRegion.imageExtent                     = {width, height, depth};

        vkCmdCopyImageToBuffer(commandBuffer.getCommandBuffer(), deepmapElevationImage.getImage(),
                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stagingBuffer.getBuffer(), 1,
                               &copyRegion);

        VkImageMemoryBarrier postCopyBarrier{};
        postCopyBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        postCopyBarrier.oldLayout                       = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        postCopyBarrier.newLayout                       = VK_IMAGE_LAYOUT_GENERAL;
        postCopyBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        postCopyBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        postCopyBarrier.image                           = deepmapElevationImage.getImage();
        postCopyBarrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        postCopyBarrier.subresourceRange.baseMipLevel   = 0;
        postCopyBarrier.subresourceRange.levelCount     = 1;
        postCopyBarrier.subresourceRange.baseArrayLayer = 0;
        postCopyBarrier.subresourceRange.layerCount     = 1;
        postCopyBarrier.srcAccessMask                   = VK_ACCESS_TRANSFER_READ_BIT;
        postCopyBarrier.dstAccessMask                   = VK_ACCESS_SHADER_WRITE_BIT;

        vkCmdPipelineBarrier(commandBuffer.getCommandBuffer(), VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                             &postCopyBarrier);

        VkBufferMemoryBarrier hostBarrier{};
        hostBarrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        hostBarrier.srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
        hostBarrier.dstAccessMask       = VK_ACCESS_HOST_READ_BIT;
        hostBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        hostBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        hostBarrier.buffer              = stagingBuffer.getBuffer();
        hostBarrier.offset              = stagingBuffer.getOffset();
        hostBarrier.size                = bufferSize;
        vkCmdPipelineBarrier(commandBuffer.getCommandBuffer(), VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_HOST_BIT, 0, 0, nullptr, 1, &hostBarrier, 0,
                             nullptr);

        commandBuffer.end();

        VkSubmitInfo submitInfo{};
        submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount   = 1;
        const VkCommandBuffer cmdBuffer = commandBuffer.getCommandBuffer();
        submitInfo.pCommandBuffers      = &cmdBuffer;

        GameVulkanFence localFence(device, GameVulkanFenceCreateInfo{0});
        GameVulkanQueueSubmitter::submit(computeQueue, &submitInfo, localFence.getFence());

        localFence.wait();

        void* data;
        vkMapMemory(device, stagingBuffer.getMemory(), stagingBuffer.getOffset(), bufferSize, 0,
                    &data);

        uint8_t* pixelData = static_cast<uint8_t*>(data);
        for (size_t i = 0; i < output.size(); ++i)
        {
                output[i] = 1.0f - (pixelData[i] / 255.0f);
        }

        vkUnmapMemory(device, stagingBuffer.getMemory());
}

WorldHeightmap::~WorldHeightmap()
{
        completionFence.wait();

        if (descriptorSetLayout != VK_NULL_HANDLE)
        {
                vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
                descriptorSetLayout = VK_NULL_HANDLE;
        }
        if (descriptorPool != VK_NULL_HANDLE)
        {
                vkDestroyDescriptorPool(device, descriptorPool, nullptr);
                descriptorPool = VK_NULL_HANDLE;
        }
        if (pipelineLayout != VK_NULL_HANDLE)
        {
                vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
                pipelineLayout = VK_NULL_HANDLE;
        }
        if (pipeline != VK_NULL_HANDLE)
        {
                vkDestroyPipeline(device, pipeline, nullptr);
                pipeline = VK_NULL_HANDLE;
        }
}

} // namespace rl
