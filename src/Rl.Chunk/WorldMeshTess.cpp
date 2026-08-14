#include "Rl.Chunk/WorldMeshTess.h"
#include "Rl.Chunk/WorldUnitPlacement.h"
#include "Rl.Chunk/WorldOcclusionCull.h"
#include "Rl.Base/GameShaderModule.h"
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
class GameDeviceInstance;

extern const uint32_t WorldMeshTessComp_data[];
extern const uint32_t WorldMeshTessComp_size;
extern const uint32_t WorldMeshTessNoOcclusionComp_data[];
extern const uint32_t WorldMeshTessNoOcclusionComp_size;

WorldMeshTess::WorldMeshTess(const WorldMeshTessData& data,
                             IUnitPlacement&          unitPlacement,
                             GameDeviceInstance&      instance) :
    device(instance.getDevice()), physicalDevice(instance.getPhysicalDevice()),
    graphicsQueue(instance.getGraphicsQueue()), commandPool(instance.getCommandPool()),
    width(data.width), height(data.height), depth(data.depth), seed(data.seed),
    airUnitId(data.airUnitId), unitPlacement(unitPlacement), occlusionCull(nullptr),
    pipeline(VK_NULL_HANDLE), pipelineLayout(VK_NULL_HANDLE), descriptorSet(VK_NULL_HANDLE),
    descriptorSetLayout(VK_NULL_HANDLE), descriptorPool(VK_NULL_HANDLE),
    completionSemaphore(device, GameVulkanSemaphoreCreateInfo{VK_SEMAPHORE_TYPE_BINARY}),
    completionFence(device, GameVulkanFenceCreateInfo{VK_FENCE_CREATE_SIGNALED_BIT}),
    memoryAllocator(device, physicalDevice), outputBuffer(&memoryAllocator,
                                                          sizeof(PostUnit) * width * height * depth,
                                                          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    commandBuffer(device, commandPool)
{
  outputBufferHandle.setHandle(static_cast<void*>(&outputBuffer));
  outputBufferHandle.setSize(outputBuffer.getSize());
  outputBufferHandle.setOffset(outputBuffer.getOffset());
  outputBufferHandle.setUsage(GameOpaqueBufferUsage::StorageBuffer);
  outputBufferHandle.setMemoryProperty(GameOpaqueBufferMemoryProperty::DeviceLocal);

  completionHandle.setHandle(static_cast<void*>(&completionSemaphore));
  completionHandle.setType(GameOpaqueSyncHandleType::Semaphore);
  completionHandle.setState(GameOpaqueSyncHandleState::Unsignaled);

  createDescriptorSets();
  createPipeline();
}

WorldMeshTess::WorldMeshTess(const WorldMeshTessData& data,
                             IUnitPlacement&          unitPlacement,
                             WorldOcclusionCull&      occlusionCull,
                             GameDeviceInstance&      instance) :
    device(instance.getDevice()), physicalDevice(instance.getPhysicalDevice()),
    graphicsQueue(instance.getGraphicsQueue()), commandPool(instance.getCommandPool()),
    width(data.width), height(data.height), depth(data.depth), seed(data.seed),
    airUnitId(data.airUnitId), unitPlacement(unitPlacement), occlusionCull(&occlusionCull),
    pipeline(VK_NULL_HANDLE), pipelineLayout(VK_NULL_HANDLE), descriptorSet(VK_NULL_HANDLE),
    descriptorSetLayout(VK_NULL_HANDLE), descriptorPool(VK_NULL_HANDLE),
    completionSemaphore(device, GameVulkanSemaphoreCreateInfo{0}),
    completionFence(device, GameVulkanFenceCreateInfo{0}), memoryAllocator(device, physicalDevice),
    outputBuffer(&memoryAllocator,
                 sizeof(PostUnit) * width * height * depth,
                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    commandBuffer(device, commandPool)
{
  outputBufferHandle.setHandle(static_cast<void*>(&outputBuffer));
  outputBufferHandle.setSize(outputBuffer.getSize());
  outputBufferHandle.setOffset(outputBuffer.getOffset());
  outputBufferHandle.setUsage(GameOpaqueBufferUsage::StorageBuffer);
  outputBufferHandle.setMemoryProperty(GameOpaqueBufferMemoryProperty::DeviceLocal);

  completionHandle.setHandle(static_cast<void*>(&completionSemaphore));
  completionHandle.setType(GameOpaqueSyncHandleType::Semaphore);
  completionHandle.setState(GameOpaqueSyncHandleState::Unsignaled);

  createDescriptorSets();
  createPipeline();
}

WorldMeshTess::~WorldMeshTess()
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

void WorldMeshTess::createDescriptorSets()
{
  uint32_t bindingCount = occlusionCull ? 3 : 2;

  std::vector<VkDescriptorPoolSize> poolSizes(bindingCount);
  poolSizes[0].type            = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  poolSizes[0].descriptorCount = 1;
  poolSizes[1].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  poolSizes[1].descriptorCount = 1;
  if (occlusionCull)
  {
    poolSizes[2].type            = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    poolSizes[2].descriptorCount = 1;
  }

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = bindingCount;
  poolInfo.pPoolSizes    = poolSizes.data();
  poolInfo.maxSets       = 1;

  VkResult result = vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool);
  if (result != VK_SUCCESS)
  {
    GameError::exitWithError("vkCreateDescriptorPool",
                             "Failed to create descriptor pool (result = " +
                                 GameError::vulkanResultToString(result) + ")");
  }

  std::vector<VkDescriptorSetLayoutBinding> bindings(bindingCount);
  bindings[0].binding         = 0;
  bindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  bindings[0].descriptorCount = 1;
  bindings[0].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

  bindings[1].binding         = 1;
  bindings[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  bindings[1].descriptorCount = 1;
  bindings[1].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

  if (occlusionCull)
  {
    bindings[2].binding         = 2;
    bindings[2].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[2].descriptorCount = 1;
    bindings[2].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;
  }

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = bindingCount;
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

  VkDescriptorBufferInfo outputBufferInfo{};
  outputBufferInfo.buffer = outputBuffer.getBuffer();
  outputBufferInfo.offset = 0;
  outputBufferInfo.range  = outputBuffer.getSize();

  std::vector<VkWriteDescriptorSet> writes(bindingCount);

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
  writes[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  writes[1].pBufferInfo     = &outputBufferInfo;

  if (occlusionCull)
  {
    VkDescriptorImageInfo visibilityImageInfo{};
    visibilityImageInfo.imageLayout      = VK_IMAGE_LAYOUT_GENERAL;
    const GameVulkanImageView& imageView = occlusionCull->getVisibilityOutputImageView();
    visibilityImageInfo.imageView        = imageView.getImageView();
    visibilityImageInfo.sampler          = VK_NULL_HANDLE;

    writes[2].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[2].dstSet          = descriptorSet;
    writes[2].dstBinding      = 2;
    writes[2].dstArrayElement = 0;
    writes[2].descriptorCount = 1;
    writes[2].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    writes[2].pImageInfo      = &visibilityImageInfo;
  }

  vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
}

void WorldMeshTess::createPipeline()
{
  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  pushConstantRange.offset     = 0;
  pushConstantRange.size       = sizeof(WorldMeshTessPushConstants);

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount         = 1;
  pipelineLayoutInfo.pSetLayouts            = &descriptorSetLayout;
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges    = &pushConstantRange;

  VkResult result = vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout);
  if (result != VK_SUCCESS)
  {
    GameError::exitWithError("vkCreatePipelineLayout",
                             "Failed to create pipeline layout (result = " +
                                 GameError::vulkanResultToString(result) + ")");
  }

  if (occlusionCull)
  {
    computeShader = GameShaderLoader::createShaderModule(device, WorldMeshTessComp_data,
                                                         WorldMeshTessComp_size);
  }
  else
  {
    computeShader = GameShaderLoader::createShaderModule(device, WorldMeshTessNoOcclusionComp_data,
                                                         WorldMeshTessNoOcclusionComp_size);
  }
  VkPipelineShaderStageCreateInfo shaderStageInfo{};
  shaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStageInfo.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
  shaderStageInfo.module = computeShader.shaderModule;
  shaderStageInfo.pName  = "main";

  VkComputePipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.stage  = shaderStageInfo;

  result = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline);
  if (result != VK_SUCCESS)
  {
    GameError::exitWithError("vkCreateComputePipelines",
                             "Failed to create compute pipeline (result = " +
                                 GameError::vulkanResultToString(result) + ")");
  }
}

void WorldMeshTess::read(VkDevice         device,
                         VkPhysicalDevice physicalDevice,
                         PostUnit*        pOutput,
                         const size_t     outputSize)
{
  VkDeviceSize bufferSize = sizeof(PostUnit) * outputSize;
  void*        data       = nullptr;
  vkMapMemory(device, outputBuffer.getMemory(), 0, bufferSize, 0, &data);
  memcpy(pOutput, data, bufferSize);
  vkUnmapMemory(device, outputBuffer.getMemory());
}

std::recursive_mutex& WorldMeshTess::getGenerateMutex()
{
  return generateMutex;
}

const GameOpaqueSyncHandle& WorldMeshTess::getCompletionHandle() const
{
  return completionHandle.getHandleStruct();
}

const GameVulkanSemaphore& WorldMeshTess::getCompletionSemaphore() const
{
  return completionSemaphore;
}

GameVulkanSemaphore& WorldMeshTess::getCompletionSemaphore()
{
  return completionSemaphore;
}

const GameVulkanFence& WorldMeshTess::getCompletionFence() const
{
  return completionFence;
}

GameVulkanFence& WorldMeshTess::getCompletionFence()
{
  return completionFence;
}

const GameOpaqueBufferHandle& WorldMeshTess::getOutputBuffer() const
{
  return outputBufferHandle.getHandleStruct();
}

#if defined(_RL_CHUNK_VULKAN_BACKEND)
void* WorldMeshTess::getOutputBufferPtr() const
{
  return const_cast<GameVulkanBuffer*>(&outputBuffer);
}
#endif

void WorldMeshTess::dispatch(void*                      pResource,
                             const GameVulkanSemaphore& waitSemaphore,
                             GameVulkanFence&           fence)
{
  WorldMeshTessPResource*     resource = static_cast<WorldMeshTessPResource*>(pResource);
  WorldMeshTessPushConstants* params   = resource->pParams;

  params->seed      = seed;
  params->width     = width;
  params->height    = height;
  params->depth     = depth;
  params->airUnitId = airUnitId;

  std::scoped_lock lock(generateMutex);

  completionFence.wait();
  completionFence.reset();

  commandBuffer.reset();
  commandBuffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

  vkCmdBindPipeline(commandBuffer.getCommandBuffer(), VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
  vkCmdBindDescriptorSets(commandBuffer.getCommandBuffer(), VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
  vkCmdPushConstants(commandBuffer.getCommandBuffer(), pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT,
                     0, sizeof(WorldMeshTessPushConstants), params);

  uint32_t groupCountX = (width * height * depth + 63) / 64;
  vkCmdDispatch(commandBuffer.getCommandBuffer(), groupCountX, 1, 1);

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

  GameVulkanQueueSubmitter::submit(graphicsQueue, &submitInfo, completionFence.getFence());
}

} // namespace rl
