#include "Rl.Chunk/ChunkOpenSimplex.h"
#include "Rl.Base/GameVulkanFence.h"
#include "Rl.Base/GameError.h"

#include "Rl.Base/GameShaderModule.h"
#include "Rl.Base/GameVulkanBuffer.h"
#include "Rl.Base/GameVulkanQueueSubmitter.h"
#include "Rl.Base/GameVulkanAllocator.h"
#include "Rl.Base/GameVulkanCommandBuffer.h"
#include "Rl.Base/GameDevice.h"

#include <array>
#include <cstdint>
#include <stdexcept>
#include <vulkan/vulkan.hpp>

namespace rl
{

extern const uint32_t SimplexComp_data[];
extern const uint32_t SimplexComp_size;

ChunkOpenSimplex::ChunkOpenSimplex(
    uint32_t width, uint32_t height, uint32_t depth, uint32_t seed, GameDeviceInstance& instance) :
    ChunkNoiseGenerator(seed, instance), instance(instance.getInstance()),
    completionSemaphore(instance.getDevice(),
                        GameVulkanSemaphoreCreateInfo{VK_SEMAPHORE_TYPE_BINARY}),
    completionFence(instance.getDevice(), GameVulkanFenceCreateInfo{VK_FENCE_CREATE_SIGNALED_BIT}),
    noiseBuffer(&memoryAllocator,
                width * height * depth * sizeof(float),
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    state{seed, width, height, depth}
{
  createDescriptorSetLayout(device);
  createDescriptorPool(device);
  createDescriptorSets(device);
  createComputePipeline(device);
  updatePermutationBuffers(device, instance.getPhysicalDevice());
}

void ChunkOpenSimplex::setWidth(uint32_t newWidth)
{
  state.noiseWidth = newWidth;
}

uint32_t ChunkOpenSimplex::getWidth() const
{
  return state.noiseWidth;
}

void ChunkOpenSimplex::setHeight(uint32_t newHeight)
{
  state.noiseHeight = newHeight;
}

uint32_t ChunkOpenSimplex::getHeight() const
{
  return state.noiseHeight;
}

void ChunkOpenSimplex::setDepth(uint32_t newDepth)
{
  state.noiseDepth = newDepth;
}

uint32_t ChunkOpenSimplex::getDepth() const
{
  return state.noiseDepth;
}

void ChunkOpenSimplex::setSeed(uint32_t newSeed)
{
  state.seed = newSeed;
}

void ChunkOpenSimplex::updateSeed(VkDevice         device,
                                  VkPhysicalDevice physicalDevice,
                                  uint32_t         newSeed)
{
  state.seed = newSeed;
  updatePermutationBuffers(device, physicalDevice, newSeed);
}

uint32_t ChunkOpenSimplex::getSeed() const
{
  return state.seed;
}

std::recursive_mutex& ChunkOpenSimplex::getGenerateMutex()
{
  return generateMutex;
}

const GameVulkanSemaphore& ChunkOpenSimplex::getCompletionSemaphore() const
{
  return completionSemaphore;
}

GameVulkanSemaphore& ChunkOpenSimplex::getCompletionSemaphore()
{
  return completionSemaphore;
}

const GameVulkanFence& ChunkOpenSimplex::getCompletionFence() const
{
  return completionFence;
}

GameVulkanFence& ChunkOpenSimplex::getCompletionFence()
{
  return completionFence;
}

void ChunkOpenSimplex::dispatch(void*                      pResource,
                                const GameVulkanSemaphore& waitSemaphore,
                                GameVulkanFence&           fence)
{
  ChunkOpenSimplexComputePResource* pOriginalResource =
      static_cast<ChunkOpenSimplexComputePResource*>(pResource);
  ChunkOpenSimplexPushConstants& params = *(pOriginalResource->pParams);

  std::scoped_lock lock(generateMutex);

  completionFence.wait();
  completionFence.reset();

  computeCommandBuffer.reset();
  computeCommandBuffer.begin();

  VkCommandBuffer cmd = computeCommandBuffer.getCommandBuffer();

  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descriptorSet,
                          0, nullptr);
  vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
                     sizeof(ChunkOpenSimplexPushConstants), &params);

  uint32_t groupCountX = (params.width + 7) / 8;
  uint32_t groupCountY = (params.height + 7) / 8;
  uint32_t groupCountZ = (params.depth + 7) / 8;

  vkCmdDispatch(cmd, groupCountX, groupCountY, groupCountZ);

  VkMemoryBarrier memoryBarrier{};
  memoryBarrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT;
  vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1,
                       &memoryBarrier, 0, nullptr, 0, nullptr);

  computeCommandBuffer.end();

  VkSubmitInfo submitInfo{};
  submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers    = &cmd;

  VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

  const VkSemaphore waitSem = waitSemaphore.getSemaphore();
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

void ChunkOpenSimplex::createDescriptorSetLayout(VkDevice device)
{
  std::array<VkDescriptorSetLayoutBinding, 3> bindings{};

  bindings[0].binding         = 0;
  bindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  bindings[0].descriptorCount = 1;
  bindings[0].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

  bindings[1].binding         = 1;
  bindings[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  bindings[1].descriptorCount = 1;
  bindings[1].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

  bindings[2].binding         = 2;
  bindings[2].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  bindings[2].descriptorCount = 1;
  bindings[2].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings    = bindings.data();

  VkResult result = vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout);
  if (result != VK_SUCCESS)
  {
    GameError::exitWithError("vkCreateDescriptorSetLayout",
                             "Failed to create descriptor set layout"
                             "(result = " +
                                 GameError::vulkanResultToString(result) + ")",
                             device, physicalDevice, instance);
  }
}

void ChunkOpenSimplex::createDescriptorPool(VkDevice device)
{
  std::array<VkDescriptorPoolSize, 1> poolSizes{};
  poolSizes[0].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  poolSizes[0].descriptorCount = 3;

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes    = poolSizes.data();
  poolInfo.maxSets       = 1;

  VkResult result = vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool);
  if (result != VK_SUCCESS)
  {
    GameError::exitWithError("vkCreateDescriptorPool",
                             "Failed to create descriptor pool "
                             "(result = " +
                                 GameError::vulkanResultToString(result) + ")",
                             device, physicalDevice, instance);
  }
}

void ChunkOpenSimplex::createDescriptorSets(VkDevice device)
{
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool     = descriptorPool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts        = &descriptorSetLayout;

  VkResult result = vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet);
  if (result != VK_SUCCESS)
  {
    GameError::exitWithError("vkAllocateDescriptorSets",
                             "Failed to allocate descriptor sets"
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

  VkDescriptorBufferInfo noiseBufferInfo{};
  noiseBufferInfo.buffer = noiseBuffer.getBuffer();
  noiseBufferInfo.offset = noiseBuffer.getOffset();
  noiseBufferInfo.range  = state.noiseWidth * state.noiseHeight * state.noiseDepth * sizeof(float);

  std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

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
  descriptorWrites[2].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  descriptorWrites[2].descriptorCount = 1;
  descriptorWrites[2].pBufferInfo     = &noiseBufferInfo;

  vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()),
                         descriptorWrites.data(), 0, nullptr);
}

void ChunkOpenSimplex::createComputePipeline(VkDevice device)
{
  computeShaderModule =
      GameShaderLoader::createShaderModule(device, SimplexComp_data, SimplexComp_size);

  VkPipelineShaderStageCreateInfo shaderStageInfo{};
  shaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStageInfo.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
  shaderStageInfo.module = computeShaderModule.shaderModule;
  shaderStageInfo.pName  = "main";

  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  pushConstantRange.offset     = 0;
  pushConstantRange.size       = sizeof(ChunkOpenSimplexPushConstants);

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
                             "Failed to create pipeline layout"
                             "(result = " +
                                 GameError::vulkanResultToString(result) + ")",
                             device, physicalDevice, instance);
  }

  VkComputePipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.stage  = shaderStageInfo;

  result = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline);
  if (result != VK_SUCCESS)
  {
    GameError::exitWithError("vkCreateComputePipelines",
                             "Failed to create compute pipeline"
                             "(result = " +
                                 GameError::vulkanResultToString(result) + ")",
                             device, physicalDevice, instance);
  }
}

VkBuffer ChunkOpenSimplex::getNoiseBuffer() const
{
  return noiseBuffer.getBuffer();
}

VkDeviceSize ChunkOpenSimplex::getNoiseBufferSize() const
{
  return state.noiseWidth * state.noiseHeight * state.noiseDepth * sizeof(float);
}

void ChunkOpenSimplex::read(VkDevice            device,
                            VkPhysicalDevice    physicalDevice,
                            std::vector<float>& output)
{
  VkDeviceSize bufferSize = state.noiseWidth * state.noiseHeight * state.noiseDepth * sizeof(float);
  output.resize(state.noiseWidth * state.noiseHeight * state.noiseDepth);

  GameVulkanBuffer stagingBuffer(
      &memoryAllocator, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  std::scoped_lock lock(generateMutex);

  computeCommandBuffer.reset();
  computeCommandBuffer.begin();

  VkBufferCopy copyRegion{};
  copyRegion.srcOffset = noiseBuffer.getOffset();
  copyRegion.dstOffset = 0;
  copyRegion.size      = bufferSize;
  vkCmdCopyBuffer(computeCommandBuffer.getCommandBuffer(), noiseBuffer.getBuffer(),
                  stagingBuffer.getBuffer(), 1, &copyRegion);

  VkBufferMemoryBarrier barrier{};
  barrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  barrier.srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask       = VK_ACCESS_HOST_READ_BIT;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.buffer              = stagingBuffer.getBuffer();
  barrier.offset              = 0;
  barrier.size                = bufferSize;
  vkCmdPipelineBarrier(computeCommandBuffer.getCommandBuffer(), VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_HOST_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);

  computeCommandBuffer.end();

  VkSubmitInfo submitInfo{};
  submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  const VkCommandBuffer cmdBuf  = computeCommandBuffer.getCommandBuffer();
  submitInfo.pCommandBuffers    = &cmdBuf;

  GameVulkanQueueSubmitter::submit(computeQueue, &submitInfo, fence.getFence());

  fence.wait();
  fence.reset();

  void* data;
  vkMapMemory(device, stagingBuffer.getMemory(), stagingBuffer.getOffset(), bufferSize, 0, &data);
  memcpy(output.data(), data, bufferSize);
  vkUnmapMemory(device, stagingBuffer.getMemory());
}

ChunkOpenSimplex::~ChunkOpenSimplex()
{
  if (device != VK_NULL_HANDLE)
  {
    if (pipeline != VK_NULL_HANDLE)
    {
      vkDestroyPipeline(device, pipeline, nullptr);
      pipeline = VK_NULL_HANDLE;
    }
    if (pipelineLayout != VK_NULL_HANDLE)
    {
      vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
      pipelineLayout = VK_NULL_HANDLE;
    }
    if (descriptorPool != VK_NULL_HANDLE)
    {
      vkDestroyDescriptorPool(device, descriptorPool, nullptr);
      descriptorPool = VK_NULL_HANDLE;
    }
    if (descriptorSetLayout != VK_NULL_HANDLE)
    {
      vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
      descriptorSetLayout = VK_NULL_HANDLE;
    }
  }
}

} // namespace rl
