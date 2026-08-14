#include "Rl.Chunk/WorldClimateCompute.h"
#include "Rl.Base/GameVulkanFence.h"
#include "Rl.Base/GameVulkanShaderModule.h"
#include "Rl.Base/GameVulkanImage.h"
#include "Rl.Base/GameVulkanImageView.h"

#include "Rl.Base/GameVulkanShaderModule.h"
#include "Rl.Base/GameVulkanQueueSubmitter.h"
#include "Rl.Base/GameVulkanAllocator.h"
#include "Rl.Base/GameError.h"
#include "Rl.Base/GameVulkanBuffer.h"
#include "Rl.Base/GameDevice.h"

#include <stdexcept>
#include <vulkan/vulkan.hpp>

namespace rl
{

extern const uint32_t WorldClimateComp_data[];
extern const uint32_t WorldClimateComp_size;

WorldClimateCompute::WorldClimateCompute(uint32_t            width,
                                         uint32_t            height,
                                         GameDeviceInstance& instance) :
    instance(instance.getInstance()), device(instance.getDevice()),
    physicalDevice(instance.getPhysicalDevice()),
    completionSemaphore(instance.getDevice(),
                        GameVulkanSemaphoreCreateInfo{VK_SEMAPHORE_TYPE_BINARY}),
    completionFence(instance.getDevice(), GameVulkanFenceCreateInfo{VK_FENCE_CREATE_SIGNALED_BIT}),
    computeQueue(instance.getGraphicsQueue()),
    computeCommandPool(instance.getDevice(), instance.getGraphicsFamily()),
    computeCommandBuffer(instance.getDevice(), computeCommandPool.getCommandPool()),
    readFence(instance.getDevice(), GameVulkanFenceCreateInfo{0}),
    memoryAllocator(instance.getDevice(), instance.getPhysicalDevice()),
    planetBuffer(&memoryAllocator,
                 sizeof(WorldPlanetData),
                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    width(width), height(height)
{
  if (!width || !height)
  {
    GameError::exitWithError("Width and height must be greater than 0");
  }
  createEquatorImage(device, instance.getPhysicalDevice());
  createEquatorImageView(device);

  equatorImageHandle.setHandle(static_cast<void*>(&equatorImage));
  equatorImageHandle.setWidth(width);
  equatorImageHandle.setHeight(height);
  equatorImageHandle.setDepth(1);
  equatorImageHandle.setFormat(GameOpaqueImageFormat::R32G32B32A32_SFLOAT);
  equatorImageHandle.setType(GameOpaqueImageType::Image2D);
  equatorImageHandle.setUsage(GameOpaqueImageUsage::Storage);

  completionHandle.setHandle(static_cast<void*>(&completionSemaphore));
  completionHandle.setType(GameOpaqueSyncHandleType::Semaphore);
  completionHandle.setState(GameOpaqueSyncHandleState::Unsignaled);

  createDescriptorSetLayout(device);
  createDescriptorPool(device);
  createDescriptorSets(device);
  createComputePipeline(device);
}

WorldClimateCompute::~WorldClimateCompute()
{
  completionFence.wait();

  if (pipeline != VK_NULL_HANDLE)
  {
    vkDestroyPipeline(device, pipeline, nullptr);
  }
  if (pipelineLayout != VK_NULL_HANDLE)
  {
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
  }
  if (descriptorPool != VK_NULL_HANDLE)
  {
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
  }
  if (descriptorSetLayout != VK_NULL_HANDLE)
  {
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
  }
}

void WorldClimateCompute::dispatch(void*                      pResource,
                                   const GameVulkanSemaphore& waitSemaphore,
                                   GameVulkanFence&           fence)
{
  WorldClimateComputePResource* pOriginalResource =
      static_cast<WorldClimateComputePResource*>(pResource);
  WorldClimateComputePushConstants& params = *(pOriginalResource->pParams);

  if (pOriginalResource->pPlanet != nullptr)
  {
    updatePlanetBuffer(*pOriginalResource->pPlanet);
  }

  std::scoped_lock lock(generateMutex);

  completionFence.wait();
  completionFence.reset();

  computeCommandBuffer.reset();
  computeCommandBuffer.begin();

  computeCommandBuffer.bindPipeline(VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
  computeCommandBuffer.bindDescriptorSets(VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1,
                                          &descriptorSet, 0, nullptr);
  computeCommandBuffer.pushConstants(pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
                                     sizeof(WorldClimateComputePushConstants), &params);

  uint32_t groupCountX = (params.width + 7) / 8;
  uint32_t groupCountY = (params.height + 7) / 8;

  computeCommandBuffer.dispatch(groupCountX, groupCountY, 1);

  VkMemoryBarrier memoryBarrier{};
  memoryBarrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT;
  vkCmdPipelineBarrier(computeCommandBuffer.getCommandBuffer(),
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1,
                       &memoryBarrier, 0, nullptr, 0, nullptr);

  computeCommandBuffer.end();

  VkSubmitInfo submitInfo{};
  submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount   = 1;
  const VkCommandBuffer cmdBuffer = computeCommandBuffer.getCommandBuffer();
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

void WorldClimateCompute::read(VkDevice                       device,
                               VkPhysicalDevice               physicalDevice,
                               std::vector<WorldClimateData>& output)
{
  std::scoped_lock lock(generateMutex);

  VkDeviceSize bufferSize = width * height * 4; // RGBA8 = 4 bytes per pixel
  output.resize(width * height);

  GameVulkanBuffer stagingBuffer(
      &memoryAllocator, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  computeCommandBuffer.reset();

  computeCommandBuffer.begin();

  VkImageMemoryBarrier preCopyBarrier{};
  preCopyBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  preCopyBarrier.oldLayout                       = VK_IMAGE_LAYOUT_GENERAL;
  preCopyBarrier.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  preCopyBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  preCopyBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  preCopyBarrier.image                           = equatorImage.getImage();
  preCopyBarrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  preCopyBarrier.subresourceRange.baseMipLevel   = 0;
  preCopyBarrier.subresourceRange.levelCount     = 1;
  preCopyBarrier.subresourceRange.baseArrayLayer = 0;
  preCopyBarrier.subresourceRange.layerCount     = 1;
  preCopyBarrier.srcAccessMask                   = VK_ACCESS_SHADER_WRITE_BIT;
  preCopyBarrier.dstAccessMask                   = VK_ACCESS_TRANSFER_READ_BIT;

  vkCmdPipelineBarrier(computeCommandBuffer.getCommandBuffer(),
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                       nullptr, 0, nullptr, 1, &preCopyBarrier);

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

  vkCmdCopyImageToBuffer(computeCommandBuffer.getCommandBuffer(), equatorImage.getImage(),
                         VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stagingBuffer.getBuffer(), 1,
                         &copyRegion);

  VkImageMemoryBarrier postCopyBarrier{};
  postCopyBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  postCopyBarrier.oldLayout                       = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  postCopyBarrier.newLayout                       = VK_IMAGE_LAYOUT_GENERAL;
  postCopyBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  postCopyBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  postCopyBarrier.image                           = equatorImage.getImage();
  postCopyBarrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  postCopyBarrier.subresourceRange.baseMipLevel   = 0;
  postCopyBarrier.subresourceRange.levelCount     = 1;
  postCopyBarrier.subresourceRange.baseArrayLayer = 0;
  postCopyBarrier.subresourceRange.layerCount     = 1;
  postCopyBarrier.srcAccessMask                   = VK_ACCESS_TRANSFER_READ_BIT;
  postCopyBarrier.dstAccessMask                   = VK_ACCESS_SHADER_WRITE_BIT;

  vkCmdPipelineBarrier(computeCommandBuffer.getCommandBuffer(), VK_PIPELINE_STAGE_TRANSFER_BIT,
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
  vkCmdPipelineBarrier(computeCommandBuffer.getCommandBuffer(), VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_HOST_BIT, 0, 0, nullptr, 1, &hostBarrier, 0, nullptr);

  computeCommandBuffer.end();

  VkSubmitInfo submitInfo{};
  submitInfo.sType                       = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount          = 1;
  const VkCommandBuffer computeCmdBuffer = computeCommandBuffer.getCommandBuffer();
  submitInfo.pCommandBuffers             = &computeCmdBuffer;

  GameVulkanQueueSubmitter::submit(computeQueue, &submitInfo, readFence.getFence());

  readFence.wait();
  readFence.reset();

  void* data = stagingBuffer.map(bufferSize);

  uint8_t* pixelData = static_cast<uint8_t*>(data);
  for (size_t i = 0; i < output.size(); ++i)
  {
    output[i].latitude    = (pixelData[i * 4] / 255.0f) * 2.0f - 1.0f;
    output[i].darkening   = (pixelData[i * 4 + 1] / 255.0f);
    output[i].temperature = (pixelData[i * 4 + 2] / 255.0f);
    output[i].moisture    = (pixelData[i * 4 + 3] / 255.0f);
  }

  stagingBuffer.unmap();
}

void WorldClimateCompute::createDescriptorSetLayout(VkDevice device)
{
  std::array<VkDescriptorSetLayoutBinding, 2> bindings{};
  bindings[0].binding         = 0;
  bindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
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

  if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create descriptor set layout");
  }
}

void WorldClimateCompute::createDescriptorPool(VkDevice device)
{
  std::array<VkDescriptorPoolSize, 2> poolSizes{};
  poolSizes[0].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  poolSizes[0].descriptorCount = 1;
  poolSizes[1].type            = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  poolSizes[1].descriptorCount = 1;

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes    = poolSizes.data();
  poolInfo.maxSets       = 1;

  if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create descriptor pool");
  }
}

void WorldClimateCompute::createDescriptorSets(VkDevice device)
{
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool     = descriptorPool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts        = &descriptorSetLayout;

  if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to allocate descriptor sets");
  }

  VkDescriptorBufferInfo planetBufferInfo{};
  planetBufferInfo.buffer = planetBuffer.getBuffer();
  planetBufferInfo.offset = planetBuffer.getOffset();
  planetBufferInfo.range  = sizeof(WorldPlanetData);

  VkDescriptorImageInfo equatorImageInfo{};
  equatorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  equatorImageInfo.imageView   = equatorImageView.getImageView();
  equatorImageInfo.sampler     = VK_NULL_HANDLE;

  std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

  descriptorWrites[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[0].dstSet          = descriptorSet;
  descriptorWrites[0].dstBinding      = 0;
  descriptorWrites[0].dstArrayElement = 0;
  descriptorWrites[0].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  descriptorWrites[0].descriptorCount = 1;
  descriptorWrites[0].pBufferInfo     = &planetBufferInfo;

  descriptorWrites[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[1].dstSet          = descriptorSet;
  descriptorWrites[1].dstBinding      = 1;
  descriptorWrites[1].dstArrayElement = 0;
  descriptorWrites[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  descriptorWrites[1].descriptorCount = 1;
  descriptorWrites[1].pImageInfo      = &equatorImageInfo;

  vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()),
                         descriptorWrites.data(), 0, nullptr);
}

void WorldClimateCompute::createComputePipeline(VkDevice device)
{
  computeShaderModule =
      GameVulkanShader::shader(device, WorldClimateComp_data, WorldClimateComp_size);

  VkPipelineShaderStageCreateInfo shaderStageInfo{};
  shaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStageInfo.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
  shaderStageInfo.module = computeShaderModule.getShaderModule();
  shaderStageInfo.pName  = "main";

  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  pushConstantRange.offset     = 0;
  pushConstantRange.size       = sizeof(WorldClimateComputePushConstants);

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount         = 1;
  pipelineLayoutInfo.pSetLayouts            = &descriptorSetLayout;
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges    = &pushConstantRange;

  if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create compute pipeline layout");
  }

  VkComputePipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipelineInfo.stage  = shaderStageInfo;
  pipelineInfo.layout = pipelineLayout;

  if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) !=
      VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create compute pipeline");
  }
}

void WorldClimateCompute::createEquatorImage(VkDevice device, VkPhysicalDevice physicalDevice)
{
  GameVulkanImageCreateInfo imageInfo{};
  imageInfo.imageType     = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width  = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth  = 1;
  imageInfo.mipLevels     = 1;
  imageInfo.arrayLayers   = 1;
  imageInfo.format        = VK_FORMAT_R8G8B8A8_UNORM;
  imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage         = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  imageInfo.sharingMode      = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.samples          = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

  equatorImage = GameVulkanImage(device, physicalDevice, imageInfo);

  std::scoped_lock lock(generateMutex);

  computeCommandBuffer.reset();

  computeCommandBuffer.begin();

  VkImageMemoryBarrier barrier{};
  barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
  barrier.newLayout                       = VK_IMAGE_LAYOUT_GENERAL;
  barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.image                           = equatorImage.getImage();
  barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel   = 0;
  barrier.subresourceRange.levelCount     = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount     = 1;
  barrier.srcAccessMask                   = 0;
  barrier.dstAccessMask                   = VK_ACCESS_SHADER_WRITE_BIT;

  vkCmdPipelineBarrier(computeCommandBuffer.getCommandBuffer(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                       &barrier);

  computeCommandBuffer.end();

  VkSubmitInfo submitInfo{};
  submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount   = 1;
  const VkCommandBuffer cmdBuffer = computeCommandBuffer.getCommandBuffer();
  submitInfo.pCommandBuffers      = &cmdBuffer;

  GameVulkanFence localFence(device, GameVulkanFenceCreateInfo{0});
  GameVulkanQueueSubmitter::submit(computeQueue, &submitInfo, localFence.getFence());

  localFence.wait();
}

void WorldClimateCompute::updatePlanetBuffer(const WorldPlanetData& planet)
{
  std::scoped_lock lock(generateMutex);

  VkDeviceSize bufferSize = sizeof(WorldPlanetData);

  GameVulkanBuffer stagingBuffer(
      &memoryAllocator, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  void* data = stagingBuffer.map(bufferSize);
  memcpy(data, &planet, bufferSize);
  stagingBuffer.unmap();

  computeCommandBuffer.reset();
  computeCommandBuffer.begin();

  VkBufferCopy copyRegion{};
  copyRegion.srcOffset = 0;
  copyRegion.dstOffset = planetBuffer.getOffset();
  copyRegion.size      = bufferSize;
  vkCmdCopyBuffer(computeCommandBuffer.getCommandBuffer(), stagingBuffer.getBuffer(),
                  planetBuffer.getBuffer(), 1, &copyRegion);

  VkBufferMemoryBarrier bufferBarrier{};
  bufferBarrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  bufferBarrier.srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
  bufferBarrier.dstAccessMask       = VK_ACCESS_SHADER_READ_BIT;
  bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  bufferBarrier.buffer              = planetBuffer.getBuffer();
  bufferBarrier.offset              = planetBuffer.getOffset();
  bufferBarrier.size                = bufferSize;
  vkCmdPipelineBarrier(computeCommandBuffer.getCommandBuffer(), VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 1, &bufferBarrier, 0,
                       nullptr);

  computeCommandBuffer.end();

  VkSubmitInfo submitInfo{};
  submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount   = 1;
  const VkCommandBuffer cmdBuffer = computeCommandBuffer.getCommandBuffer();
  submitInfo.pCommandBuffers      = &cmdBuffer;

  GameVulkanFence localFence(device, GameVulkanFenceCreateInfo{0});
  GameVulkanQueueSubmitter::submit(computeQueue, &submitInfo, localFence.getFence());

  localFence.wait();
}

const GameOpaqueImageHandle& WorldClimateCompute::getEquatorImage() const
{
  return equatorImageHandle.getHandleStruct();
}

const GameOpaqueSyncHandle& WorldClimateCompute::getCompletionHandle() const
{
  return completionHandle.getHandleStruct();
}

std::recursive_mutex& WorldClimateCompute::getGenerateMutex()
{
  return generateMutex;
}

const GameVulkanSemaphore& WorldClimateCompute::getCompletionSemaphore() const
{
  return completionSemaphore;
}

GameVulkanSemaphore& WorldClimateCompute::getCompletionSemaphore()
{
  return completionSemaphore;
}

const GameVulkanFence& WorldClimateCompute::getCompletionFence() const
{
  return completionFence;
}

#if defined(_RL_CHUNK_VULKAN_BACKEND)
void* WorldClimateCompute::getEquatorImageViewPtr() const
{
  return const_cast<GameVulkanImageView*>(&equatorImageView);
}
#endif

GameVulkanFence& WorldClimateCompute::getCompletionFence()
{
  return completionFence;
}

void WorldClimateCompute::createEquatorImageView(VkDevice device)
{
  GameVulkanImageViewCreateInfo viewInfo{};
  viewInfo.image                           = equatorImage.getImage();
  viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format                          = VK_FORMAT_R8G8B8A8_UNORM;
  viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.baseMipLevel   = 0;
  viewInfo.subresourceRange.levelCount     = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount     = 1;

  equatorImageView = GameVulkanImageView(device, viewInfo);
}

} // namespace rl
