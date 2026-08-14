#include "Rl.Chunk/WorldMeshGen.h"
#include "Rl.Chunk/WorldMeshTess.h"

#include "Rl.Base/GameError.h"
#include "Rl.Base/GameVulkanAllocator.h"
#include "Rl.Base/GameVulkanQueueSubmitter.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vulkan/vulkan.hpp>

namespace rl
{

extern const uint32_t WorldMeshGenComp_data[];
extern const uint32_t WorldMeshGenComp_size;

WorldMeshGen::WorldMeshGen(const WorldMeshGenData& data,
                           IMeshTess&              meshTess,
                           GameDeviceInstance&     instance) :
    device(instance.getDevice()), physicalDevice(instance.getPhysicalDevice()),
    graphicsQueue(instance.getGraphicsQueue()), commandPool(instance.getCommandPool()),
    width(data.width), height(data.height), depth(data.depth), maxVertices(data.maxVertices),
    maxIndices(data.maxIndices), subdivisions(data.subdivisions), meshTess(meshTess),
    pipeline(VK_NULL_HANDLE), pipelineLayout(VK_NULL_HANDLE), descriptorSet(VK_NULL_HANDLE),
    descriptorSetLayout(VK_NULL_HANDLE), descriptorPool(VK_NULL_HANDLE),
    completionSemaphore(device, GameVulkanSemaphoreCreateInfo{VK_SEMAPHORE_TYPE_BINARY}),
    completionFence(device, GameVulkanFenceCreateInfo{VK_FENCE_CREATE_SIGNALED_BIT}),
    memoryAllocator(device, physicalDevice),
    vertexBuffer(&memoryAllocator,
                 sizeof(MeshVertex) * maxVertices,
                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    indexBuffer(&memoryAllocator,
                sizeof(uint32_t) * maxIndices,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                    VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    countBuffer(&memoryAllocator,
                sizeof(uint32_t),
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
    commandBuffer(device, commandPool)
{
  vertexBufferHandle.setHandle(static_cast<void*>(&vertexBuffer));
  vertexBufferHandle.setSize(vertexBuffer.getSize());
  vertexBufferHandle.setOffset(vertexBuffer.getOffset());
  vertexBufferHandle.setUsage(GameOpaqueBufferUsage::StorageBuffer);
  vertexBufferHandle.setMemoryProperty(GameOpaqueBufferMemoryProperty::DeviceLocal);

  indexBufferHandle.setHandle(static_cast<void*>(&indexBuffer));
  indexBufferHandle.setSize(indexBuffer.getSize());
  indexBufferHandle.setOffset(indexBuffer.getOffset());
  indexBufferHandle.setUsage(GameOpaqueBufferUsage::StorageBuffer);
  indexBufferHandle.setMemoryProperty(GameOpaqueBufferMemoryProperty::DeviceLocal);

  countBufferHandle.setHandle(static_cast<void*>(&countBuffer));
  countBufferHandle.setSize(countBuffer.getSize());
  countBufferHandle.setOffset(countBuffer.getOffset());
  countBufferHandle.setUsage(GameOpaqueBufferUsage::StorageBuffer);
  countBufferHandle.setMemoryProperty(GameOpaqueBufferMemoryProperty::HostVisible |
                                      GameOpaqueBufferMemoryProperty::HostCoherent);

  completionHandle.setHandle(static_cast<void*>(&completionSemaphore));
  completionHandle.setType(GameOpaqueSyncHandleType::Semaphore);
  completionHandle.setState(GameOpaqueSyncHandleState::Unsignaled);

  createDescriptorSets();
  createPipeline();
  initializeCountBuffer();
}

WorldMeshGen::~WorldMeshGen()
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

void WorldMeshGen::createDescriptorSets()
{
  std::array<VkDescriptorPoolSize, 1> poolSizes{};
  poolSizes[0].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  poolSizes[0].descriptorCount = 4;

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  poolInfo.maxSets       = 1;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes    = poolSizes.data();

  VkResult result = vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool);
  if (result != VK_SUCCESS)
  {
    GameError::exitWithError("vkCreateDescriptorPool",
                             "Failed to create descriptor pool (result = " +
                                 GameError::vulkanResultToString(result) + ")");
  }

  std::array<VkDescriptorSetLayoutBinding, 4> bindings{};
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

  bindings[3].binding         = 3;
  bindings[3].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  bindings[3].descriptorCount = 1;
  bindings[3].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

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

  VkDescriptorBufferInfo tessBufferInfo{};
#if defined(_RL_CHUNK_VULKAN_BACKEND)
  tessBufferInfo.buffer =
      reinterpret_cast<GameVulkanBuffer*>(meshTess.getOutputBufferPtr())->getBuffer();
  tessBufferInfo.offset = 0;
  tessBufferInfo.range  = VK_WHOLE_SIZE;
#else
  // CPU backend: no Vulkan buffer available
  tessBufferInfo.buffer = VK_NULL_HANDLE;
  tessBufferInfo.offset = 0;
  tessBufferInfo.range  = 0;
#endif

  VkDescriptorBufferInfo vertexBufferInfo{};
  vertexBufferInfo.buffer = vertexBuffer.getBuffer();
  vertexBufferInfo.offset = vertexBuffer.getOffset();
  vertexBufferInfo.range  = VK_WHOLE_SIZE;

  VkDescriptorBufferInfo indexBufferInfo{};
  indexBufferInfo.buffer = indexBuffer.getBuffer();
  indexBufferInfo.offset = indexBuffer.getOffset();
  indexBufferInfo.range  = VK_WHOLE_SIZE;

  VkDescriptorBufferInfo countBufferInfo{};
  countBufferInfo.buffer = countBuffer.getBuffer();
  countBufferInfo.offset = countBuffer.getOffset();
  countBufferInfo.range  = sizeof(uint32_t);

  std::array<VkWriteDescriptorSet, 4> descriptorWrites{};
  descriptorWrites[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[0].dstSet          = descriptorSet;
  descriptorWrites[0].dstBinding      = 0;
  descriptorWrites[0].dstArrayElement = 0;
  descriptorWrites[0].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  descriptorWrites[0].descriptorCount = 1;
  descriptorWrites[0].pBufferInfo     = &tessBufferInfo;

  descriptorWrites[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[1].dstSet          = descriptorSet;
  descriptorWrites[1].dstBinding      = 1;
  descriptorWrites[1].dstArrayElement = 0;
  descriptorWrites[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  descriptorWrites[1].descriptorCount = 1;
  descriptorWrites[1].pBufferInfo     = &vertexBufferInfo;

  descriptorWrites[2].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[2].dstSet          = descriptorSet;
  descriptorWrites[2].dstBinding      = 2;
  descriptorWrites[2].dstArrayElement = 0;
  descriptorWrites[2].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  descriptorWrites[2].descriptorCount = 1;
  descriptorWrites[2].pBufferInfo     = &indexBufferInfo;

  descriptorWrites[3].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[3].dstSet          = descriptorSet;
  descriptorWrites[3].dstBinding      = 3;
  descriptorWrites[3].dstArrayElement = 0;
  descriptorWrites[3].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  descriptorWrites[3].descriptorCount = 1;
  descriptorWrites[3].pBufferInfo     = &countBufferInfo;

  vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()),
                         descriptorWrites.data(), 0, nullptr);
}

void WorldMeshGen::initializeCountBuffer()
{
  VkDeviceSize bufferSize = sizeof(uint32_t);
  void*        data       = nullptr;
  vkMapMemory(device, countBuffer.getMemory(), 0, bufferSize, 0, &data);
  memset(data, 0, bufferSize);
  vkUnmapMemory(device, countBuffer.getMemory());
}

void WorldMeshGen::createPipeline()
{
  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  pushConstantRange.offset     = 0;
  pushConstantRange.size       = sizeof(WorldMeshGenPushConstants);

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

  computeShader =
      GameShaderLoader::createShaderModule(device, WorldMeshGenComp_data, WorldMeshGenComp_size);

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

void WorldMeshGen::readVertices(VkDevice         device,
                                VkPhysicalDevice physicalDevice,
                                MeshVertex*      pOutput,
                                const size_t     outputSize)
{
  VkDeviceSize bufferSize = sizeof(MeshVertex) * outputSize;
  void*        data       = nullptr;

  vkMapMemory(device, vertexBuffer.getMemory(), vertexBuffer.getOffset(), bufferSize, 0, &data);
  memcpy(pOutput, data, bufferSize);
  vkUnmapMemory(device, vertexBuffer.getMemory());
}

void WorldMeshGen::readIndices(VkDevice         device,
                               VkPhysicalDevice physicalDevice,
                               uint32_t*        pOutput,
                               const size_t     outputSize)
{
  VkDeviceSize bufferSize = sizeof(uint32_t) * outputSize;
  void*        data       = nullptr;

  vkMapMemory(device, indexBuffer.getMemory(), indexBuffer.getOffset(), bufferSize, 0, &data);
  memcpy(pOutput, data, bufferSize);
  vkUnmapMemory(device, indexBuffer.getMemory());
}

void WorldMeshGen::readVertexCount(uint32_t& vertexCount)
{
  VkDeviceSize size = sizeof(uint32_t);
  void*        data = nullptr;

  vkMapMemory(device, countBuffer.getMemory(), countBuffer.getOffset(), size, 0, &data);
  memcpy(&vertexCount, data, size);
  vkUnmapMemory(device, countBuffer.getMemory());
}

std::recursive_mutex& WorldMeshGen::getGenerateMutex()
{
  return generateMutex;
}

const GameOpaqueSyncHandle& WorldMeshGen::getCompletionHandle() const
{
  return completionHandle.getHandleStruct();
}

const GameVulkanSemaphore& WorldMeshGen::getCompletionSemaphore() const
{
  return completionSemaphore;
}

GameVulkanSemaphore& WorldMeshGen::getCompletionSemaphore()
{
  return completionSemaphore;
}

const GameVulkanFence& WorldMeshGen::getCompletionFence() const
{
  return completionFence;
}

GameVulkanFence& WorldMeshGen::getCompletionFence()
{
  return completionFence;
}

GameOpaqueBufferHandle& WorldMeshGen::getVertexBuffer()
{
  return vertexBufferHandle.getHandleStruct();
}

GameOpaqueBufferHandle& WorldMeshGen::getIndexBuffer()
{
  return indexBufferHandle.getHandleStruct();
}

GameOpaqueBufferHandle& WorldMeshGen::getCountBuffer()
{
  return countBufferHandle.getHandleStruct();
}

#if defined(_RL_CHUNK_VULKAN_BACKEND)
void* WorldMeshGen::getVertexBufferPtr()
{
  return &vertexBuffer;
}

void* WorldMeshGen::getIndexBufferPtr()
{
  return &indexBuffer;
}

void* WorldMeshGen::getCountBufferPtr()
{
  return &countBuffer;
}
#endif

uint32_t WorldMeshGen::getSubdivisions() const
{
  return subdivisions;
}

uint32_t WorldMeshGen::getWidth() const
{
  return width;
}

uint32_t WorldMeshGen::getHeight() const
{
  return height;
}

uint32_t WorldMeshGen::getDepth() const
{
  return depth;
}

void WorldMeshGen::dispatch(void*                      pResource,
                            const GameVulkanSemaphore& waitSemaphore,
                            GameVulkanFence&           fence)
{
  WorldMeshGenPResource*     resource = static_cast<WorldMeshGenPResource*>(pResource);
  WorldMeshGenPushConstants* params   = resource->pParams;

  params->width        = width;
  params->height       = height;
  params->depth        = depth;
  params->maxVertices  = maxVertices;
  params->maxIndices   = maxIndices;
  params->subdivisions = subdivisions;

  std::scoped_lock lock(generateMutex);

  completionFence.wait();
  completionFence.reset();

  VkDescriptorBufferInfo tessBufferInfo{};
#if defined(_RL_CHUNK_VULKAN_BACKEND)
  tessBufferInfo.buffer =
      reinterpret_cast<GameVulkanBuffer*>(meshTess.getOutputBufferPtr())->getBuffer();
  tessBufferInfo.offset = 0;
  tessBufferInfo.range =
      reinterpret_cast<GameVulkanBuffer*>(meshTess.getOutputBufferPtr())->getSize();
#else
  tessBufferInfo.buffer = VK_NULL_HANDLE;
  tessBufferInfo.offset = 0;
  tessBufferInfo.range  = 0;
#endif

  VkDescriptorBufferInfo vertexBufferInfo{};
  vertexBufferInfo.buffer = vertexBuffer.getBuffer();
  vertexBufferInfo.offset = 0;
  vertexBufferInfo.range  = vertexBuffer.getSize();

  VkDescriptorBufferInfo indexBufferInfo{};
  indexBufferInfo.buffer = indexBuffer.getBuffer();
  indexBufferInfo.offset = 0;
  indexBufferInfo.range  = indexBuffer.getSize();

  VkDescriptorBufferInfo countBufferInfo{};
  countBufferInfo.buffer = countBuffer.getBuffer();
  countBufferInfo.offset = countBuffer.getOffset();
  countBufferInfo.range  = sizeof(uint32_t);

  std::array<VkWriteDescriptorSet, 4> writes{};
  writes[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writes[0].dstSet          = descriptorSet;
  writes[0].dstBinding      = 0;
  writes[0].dstArrayElement = 0;
  writes[0].descriptorCount = 1;
  writes[0].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  writes[0].pBufferInfo     = &tessBufferInfo;

  writes[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writes[1].dstSet          = descriptorSet;
  writes[1].dstBinding      = 1;
  writes[1].dstArrayElement = 0;
  writes[1].descriptorCount = 1;
  writes[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  writes[1].pBufferInfo     = &vertexBufferInfo;

  writes[2].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writes[2].dstSet          = descriptorSet;
  writes[2].dstBinding      = 2;
  writes[2].dstArrayElement = 0;
  writes[2].descriptorCount = 1;
  writes[2].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  writes[2].pBufferInfo     = &indexBufferInfo;

  writes[3].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writes[3].dstSet          = descriptorSet;
  writes[3].dstBinding      = 3;
  writes[3].dstArrayElement = 0;
  writes[3].descriptorCount = 1;
  writes[3].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  writes[3].pBufferInfo     = &countBufferInfo;

  vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);

  commandBuffer.reset();
  commandBuffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

  vkCmdFillBuffer(commandBuffer.getCommandBuffer(), countBuffer.getBuffer(), 0, sizeof(uint32_t),
                  0);

  vkCmdFillBuffer(commandBuffer.getCommandBuffer(), vertexBuffer.getBuffer(),
                  vertexBuffer.getOffset(), vertexBuffer.getSize(), 0);

  vkCmdFillBuffer(commandBuffer.getCommandBuffer(), indexBuffer.getBuffer(),
                  indexBuffer.getOffset(), indexBuffer.getSize(), 0);

  std::array<VkBufferMemoryBarrier, 3> fillBarriers{};

  // Count buffer barrier
  fillBarriers[0].sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  fillBarriers[0].srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
  fillBarriers[0].dstAccessMask       = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
  fillBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  fillBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  fillBarriers[0].buffer              = countBuffer.getBuffer();
  fillBarriers[0].offset              = 0;
  fillBarriers[0].size                = sizeof(uint32_t);

  // Vertex buffer barrier
  fillBarriers[1].sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  fillBarriers[1].srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
  fillBarriers[1].dstAccessMask       = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
  fillBarriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  fillBarriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  fillBarriers[1].buffer              = vertexBuffer.getBuffer();
  fillBarriers[1].offset              = vertexBuffer.getOffset();
  fillBarriers[1].size                = vertexBuffer.getSize();

  // Index buffer barrier
  fillBarriers[2].sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  fillBarriers[2].srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
  fillBarriers[2].dstAccessMask       = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
  fillBarriers[2].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  fillBarriers[2].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  fillBarriers[2].buffer              = indexBuffer.getBuffer();
  fillBarriers[2].offset              = indexBuffer.getOffset();
  fillBarriers[2].size                = indexBuffer.getSize();

  vkCmdPipelineBarrier(commandBuffer.getCommandBuffer(), VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr,
                       static_cast<uint32_t>(fillBarriers.size()), fillBarriers.data(), 0, nullptr);

  vkCmdBindPipeline(commandBuffer.getCommandBuffer(), VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
  vkCmdBindDescriptorSets(commandBuffer.getCommandBuffer(), VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
  vkCmdPushConstants(commandBuffer.getCommandBuffer(), pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT,
                     0, sizeof(WorldMeshGenPushConstants), params);

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
