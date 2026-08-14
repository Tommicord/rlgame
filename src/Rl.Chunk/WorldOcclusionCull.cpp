#include "Rl.Chunk/WorldOcclusionCull.h"
#include "Rl.Chunk/WorldUnitPlacement.h"
#include "Rl.Base/GameVulkanShaderModule.h"
#include "Rl.Base/GameVulkanCommandPool.h"
#include "Rl.Base/GameVulkanImage.h"
#include "Rl.Base/GameVulkanImageView.h"
#include "Rl.Base/GameError.h"
#include "Rl.Base/GameVulkanQueueSubmitter.h"
#include "Rl.Base/GameDevice.h"

#include <stdexcept>
#include <vulkan/vulkan.hpp>

namespace rl
{

extern const uint32_t WorldOcclusionCullComp_data[];
extern const uint32_t WorldOcclusionCullComp_size;

WorldOcclusionCull::WorldOcclusionCull(const WorldOcclusionCullData& data,
                                       WorldUnitPlacement&           unitPlacement,
                                       GameDeviceInstance&           instance) :
    device(instance.getDevice()), physicalDevice(instance.getPhysicalDevice()),
    computeQueue(instance.getGraphicsQueue()), commandPool(instance.getCommandPool()),
    width(data.width), height(data.height), depth(data.depth), airUnitId(data.airUnitId),
    unitPlacement(unitPlacement), pipeline(VK_NULL_HANDLE), pipelineLayout(VK_NULL_HANDLE),
    descriptorSet(VK_NULL_HANDLE), descriptorSetLayout(VK_NULL_HANDLE),
    descriptorPool(VK_NULL_HANDLE),
    completionSemaphore(device, GameVulkanSemaphoreCreateInfo{VK_SEMAPHORE_TYPE_BINARY}),
    completionFence(device, GameVulkanFenceCreateInfo{VK_FENCE_CREATE_SIGNALED_BIT}),
    memoryAllocator(device, physicalDevice), commandBuffer(device, commandPool)
{
  createVisibilityOutputImage(device, instance.getPhysicalDevice());
  createVisibilityOutputImageView(device);
  createDescriptorSets();
  createPipeline();
}

WorldOcclusionCull::~WorldOcclusionCull()
{
  if (pipeline != VK_NULL_HANDLE)
  {
    vkDestroyPipeline(device, pipeline, nullptr);
  }
  if (pipelineLayout != VK_NULL_HANDLE)
  {
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
  }
  if (descriptorSetLayout != VK_NULL_HANDLE)
  {
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
  }
  if (descriptorPool != VK_NULL_HANDLE)
  {
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
  }
}

void WorldOcclusionCull::createVisibilityOutputImage(VkDevice         device,
                                                     VkPhysicalDevice physicalDevice)
{
  GameVulkanImageCreateInfo imageInfo{};
  imageInfo.imageType     = VK_IMAGE_TYPE_3D;
  imageInfo.extent.width  = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth  = depth;
  imageInfo.mipLevels     = 1;
  imageInfo.arrayLayers   = 1;
  imageInfo.format        = VK_FORMAT_R32_UINT;
  imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage         = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  imageInfo.sharingMode      = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.samples          = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

  visibilityOutputImage = GameVulkanImage(device, physicalDevice, imageInfo);

  commandBuffer.reset();

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = 0;

  commandBuffer.begin();

  VkImageMemoryBarrier barrier{};
  barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
  barrier.newLayout                       = VK_IMAGE_LAYOUT_GENERAL;
  barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.image                           = visibilityOutputImage.getImage();
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
  submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  VkCommandBuffer cmdBuffer     = commandBuffer.getCommandBuffer();
  submitInfo.pCommandBuffers    = &cmdBuffer;

  GameVulkanFence localFence(device, GameVulkanFenceCreateInfo{0});
  GameVulkanQueueSubmitter::submit(computeQueue, &submitInfo, localFence.getFence());

  localFence.wait();
}

void WorldOcclusionCull::createVisibilityOutputImageView(VkDevice device)
{
  GameVulkanImageViewCreateInfo viewInfo{};
  viewInfo.image                           = visibilityOutputImage.getImage();
  viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_3D;
  viewInfo.format                          = VK_FORMAT_R32_UINT;
  viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.baseMipLevel   = 0;
  viewInfo.subresourceRange.levelCount     = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount     = 1;

  visibilityOutputImageView = GameVulkanImageView(device, viewInfo);
}

void WorldOcclusionCull::createDescriptorSets()
{
  std::array<VkDescriptorPoolSize, 2> poolSizes{};
  poolSizes[0].type            = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  poolSizes[0].descriptorCount = 1;
  poolSizes[1].type            = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  poolSizes[1].descriptorCount = 1;

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes    = poolSizes.data();
  poolInfo.maxSets       = 1;

  VkResult result = vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool);
  if (result != VK_SUCCESS)
  {
    GameError::exitWithError("vkCreateDescriptorPool",
                             "Failed to create descriptor pool (result = " +
                                 GameError::vulkanResultToString(result) + ")");
  }

  std::array<VkDescriptorSetLayoutBinding, 2> bindings{};
  bindings[0].binding         = 0;
  bindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  bindings[0].descriptorCount = 1;
  bindings[0].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

  bindings[1].binding         = 1;
  bindings[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  bindings[1].descriptorCount = 1;
  bindings[1].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings    = bindings.data();

  result = vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout);
  if (result != VK_SUCCESS)
  {
    GameError::exitWithError("vkCreateDescriptorSetLayout",
                             "Failed to create descriptor set layout (result = " +
                                 GameError::vulkanResultToString(result) + ")");
  }

  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool     = descriptorPool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts        = &descriptorSetLayout;

  result = vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet);
  if (result != VK_SUCCESS)
  {
    GameError::exitWithError("vkAllocateDescriptorSets",
                             "Failed to allocate descriptor sets (result = " +
                                 GameError::vulkanResultToString(result) + ")");
  }

  // Initialize descriptor set with the unit output image view and visibility output image
  // view
  VkDescriptorImageInfo unitImageInfo{};
  unitImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
#if defined(_RL_CHUNK_VULKAN_BACKEND)
  unitImageInfo.imageView =
      reinterpret_cast<GameVulkanImageView*>(unitPlacement.getUnitOutputImageViewPtr())
          ->getImageView();
#else
  unitImageInfo.imageView = VK_NULL_HANDLE;
#endif
  unitImageInfo.sampler = VK_NULL_HANDLE;

  VkDescriptorImageInfo visibilityImageInfo{};
  visibilityImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  visibilityImageInfo.imageView   = visibilityOutputImageView.getImageView();
  visibilityImageInfo.sampler     = VK_NULL_HANDLE;

  std::array<VkWriteDescriptorSet, 2> writes{};
  writes[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writes[0].dstSet          = descriptorSet;
  writes[0].dstBinding      = 0;
  writes[0].dstArrayElement = 0;
  writes[0].descriptorCount = 1;
  writes[0].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  writes[0].pImageInfo      = &unitImageInfo;

  writes[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writes[1].dstSet          = descriptorSet;
  writes[1].dstBinding      = 1;
  writes[1].dstArrayElement = 0;
  writes[1].descriptorCount = 1;
  writes[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  writes[1].pImageInfo      = &visibilityImageInfo;

  vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
}

void WorldOcclusionCull::createPipeline()
{
  computeShaderModule = GameVulkanShader::shader(device, WorldOcclusionCullComp_data,
                                                             WorldOcclusionCullComp_size);
  VkPipelineShaderStageCreateInfo shaderStageInfo{};
  shaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStageInfo.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
  shaderStageInfo.module = computeShaderModule.getShaderModule();
  shaderStageInfo.pName  = "main";

  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  pushConstantRange.offset     = 0;
  pushConstantRange.size       = sizeof(WorldUnitPlacementPushConstants);

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount         = 1;
  pipelineLayoutInfo.pSetLayouts            = &descriptorSetLayout;
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges    = &pushConstantRange;

  VkResult result1 = vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout);
  if (result1 != VK_SUCCESS)
  {
    GameError::exitWithError("vkCreatePipelineLayout",
                             "Failed to create compute pipeline layout "
                             "(result = " +
                                 GameError::vulkanResultToString(result1) + ")",
                             device, physicalDevice, instance);
  }

  VkResult result2 = vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout);
  if (result2 != VK_SUCCESS)
  {
    GameError::exitWithError("vkCreatePipelineLayout",
                             "Failed to create pipeline layout (result = " +
                                 GameError::vulkanResultToString(result2) + ")");
  }

  VkComputePipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipelineInfo.stage  = shaderStageInfo;
  pipelineInfo.layout = pipelineLayout;

  VkResult result3 =
      vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline);
  if (result3 != VK_SUCCESS)
  {
    GameError::exitWithError("vkCreateComputePipelines",
                             "Failed to create compute pipeline (result = " +
                                 GameError::vulkanResultToString(result3) + ")");
  }
}

void WorldOcclusionCull::dispatch(void*                      pResource,
                                  const GameVulkanSemaphore& waitSemaphore,
                                  GameVulkanFence&           fence)
{
  WorldOcclusionCullPResource*     resource = static_cast<WorldOcclusionCullPResource*>(pResource);
  WorldOcclusionCullPushConstants* params   = resource->pParams;

  params->width     = width;
  params->height    = height;
  params->depth     = depth;
  params->airUnitId = airUnitId;

  std::scoped_lock lock(generateMutex);

  completionFence.wait();
  completionFence.reset();

  commandBuffer.reset();
  commandBuffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

  commandBuffer.bindPipeline(VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
  commandBuffer.bindDescriptorSets(VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1,
                                    &descriptorSet, 0, nullptr);
  commandBuffer.pushConstants(pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
                              sizeof(WorldOcclusionCullPushConstants), params);

  uint32_t groupCountX = (width * height * depth + 63) / 64;
  commandBuffer.dispatch(groupCountX, 1, 1);

  commandBuffer.end();

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore          waitSemaphores[] = {waitSemaphore.getSemaphore()};
  VkPipelineStageFlags waitStages[]     = {VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT};
  if (waitSemaphore.getSemaphore() != VK_NULL_HANDLE)
  {
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores    = waitSemaphores;
    submitInfo.pWaitDstStageMask  = waitStages;
  }
  else
  {
    submitInfo.waitSemaphoreCount = 0;
  }

  submitInfo.commandBufferCount   = 1;
  const VkCommandBuffer cmdBuffer = commandBuffer.getCommandBuffer();
  submitInfo.pCommandBuffers      = &cmdBuffer;

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

std::recursive_mutex& WorldOcclusionCull::getGenerateMutex()
{
  return generateMutex;
}

const GameVulkanSemaphore& WorldOcclusionCull::getCompletionSemaphore() const
{
  return completionSemaphore;
}

GameVulkanSemaphore& WorldOcclusionCull::getCompletionSemaphore()
{
  return completionSemaphore;
}

const GameVulkanFence& WorldOcclusionCull::getCompletionFence() const
{
  return completionFence;
}

GameVulkanFence& WorldOcclusionCull::getCompletionFence()
{
  return completionFence;
}

const GameVulkanImage& WorldOcclusionCull::getVisibilityOutputImage() const
{
  return visibilityOutputImage;
}

const GameVulkanImageView& WorldOcclusionCull::getVisibilityOutputImageView() const
{
  return visibilityOutputImageView;
}

} // namespace rl
