#include "Rl.Chunk/WorldUnitPlacement.h"
#include "Rl.Chunk/WorldClimateCompute.h"
#include "Rl.Chunk/ChunkNoiseGenerator.h"

#include "Rl.Base/GameShaderModule.h"
#include "Rl.Base/GameVulkanCommandPool.h"
#include "Rl.Base/GameVulkanImage.h"
#include "Rl.Base/GameVulkanImageView.h"
#include "Rl.Log/Log.h"
#include "Rl.World/Biome.h"
#include "Rl.World/PreBiomeRegistry.h"
#include "Rl.World/PreUnitRegistry.h"
#include "Rl.World/Unit.h"

#include "Rl.Base/GameError.h"
#include "Rl.Base/GameVulkanQueueSubmitter.h"
#include "Rl.Base/GameShaderModule.h"
#include "Rl.Base/GameVulkanBuffer.h"
#include "Rl.Base/GameDevice.h"

#include <numeric>
#include <stdexcept>
#include <thread>
#include <vulkan/vulkan.hpp>

namespace rl
{

extern const uint32_t WorldUnitPlaceComp_data[];
extern const uint32_t WorldUnitPlaceComp_size;

WorldUnitPlacement::WorldUnitPlacement(uint32_t            width,
                                       uint32_t            height,
                                       uint32_t            depth,
                                       uint32_t            seed,
                                       GameDeviceInstance& instance,
                                       IHeightmap&         heightmapGenerator,
                                       IClimateCompute&    climateCompute) :
    instance(instance.getInstance()), device(instance.getDevice()),
    physicalDevice(instance.getPhysicalDevice()),
    completionSemaphore(instance.getDevice(),
                        GameVulkanSemaphoreCreateInfo{VK_SEMAPHORE_TYPE_BINARY}),
    completionFence(instance.getDevice(), GameVulkanFenceCreateInfo{VK_FENCE_CREATE_SIGNALED_BIT}),
    computeQueue(instance.getGraphicsQueue()),
    computeCommandPool(instance.getDevice(), instance.getGraphicsFamily()),
    computeCommandBuffer(instance.getDevice(), computeCommandPool.getCommandPool()),
    memoryAllocator(instance.getDevice(), instance.getPhysicalDevice()),
    planetBuffer(&memoryAllocator,
                 sizeof(WorldPlanetData),
                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    heightmapGenerator(heightmapGenerator), climateCompute(climateCompute), width(width),
    height(height), depth(depth), seed(seed)
{
  if (!width || !height || !depth)
  {
    GameError::exitWithError("WorldUnitPlacement",
                             "Width, height, and depth must be greater than 0");
  }
  createUnitOutputImage(device, instance.getPhysicalDevice());
  createUnitOutputImageView(device);
  createBiomeOutputImage(device, instance.getPhysicalDevice());
  createBiomeOutputImageView(device);

  unitOutputImageHandle.setHandle(static_cast<void*>(&unitOutputImage));
  unitOutputImageHandle.setWidth(width);
  unitOutputImageHandle.setHeight(height);
  unitOutputImageHandle.setDepth(depth);
  unitOutputImageHandle.setFormat(GameOpaqueImageFormat::R32G32B32A32_SFLOAT);
  unitOutputImageHandle.setType(GameOpaqueImageType::Image3D);
  unitOutputImageHandle.setUsage(GameOpaqueImageUsage::Storage);

  biomeOutputImageHandle.setHandle(static_cast<void*>(&biomeOutputImage));
  biomeOutputImageHandle.setWidth(width);
  biomeOutputImageHandle.setHeight(height);
  biomeOutputImageHandle.setDepth(depth);
  biomeOutputImageHandle.setFormat(GameOpaqueImageFormat::R32G32B32A32_SFLOAT);
  biomeOutputImageHandle.setType(GameOpaqueImageType::Image3D);
  biomeOutputImageHandle.setUsage(GameOpaqueImageUsage::Storage);

  completionHandle.setHandle(static_cast<void*>(&completionSemaphore));
  completionHandle.setType(GameOpaqueSyncHandleType::Semaphore);
  completionHandle.setState(GameOpaqueSyncHandleState::Unsignaled);

  initPermutationTables(device, instance.getPhysicalDevice());
  createDescriptorSetLayout(device);
  createDescriptorPool(device);
  createDescriptorSets(device);
  initRegistryBuffers(device, instance.getPhysicalDevice());
  updateDescriptorSets();
  createComputePipeline(device);
}

WorldUnitPlacement::~WorldUnitPlacement()
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

void WorldUnitPlacement::initPermutationTables(VkDevice device, VkPhysicalDevice physicalDevice)
{
  std::scoped_lock lock(generateMutex);

  std::array<int32_t, ChunkNoiseGenerator::permBufferSize> perm{};
  std::array<int32_t, ChunkNoiseGenerator::permBufferSize> permGradIndex3d{};
  ChunkNoiseGenerator::genPermutations(seed, perm, permGradIndex3d);

  VkDeviceSize permSize = ChunkNoiseGenerator::permBufferSize * sizeof(int32_t);

  permBuffer =
      GameVulkanBuffer(&memoryAllocator, permSize,
                       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  permGradIndex3DBuffer =
      GameVulkanBuffer(&memoryAllocator, permSize,
                       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  GameVulkanBuffer stagingBuffer(
      &memoryAllocator, permSize * 2, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  void* data;
  vkMapMemory(device, stagingBuffer.getMemory(), 0, permSize * 2, 0, &data);
  memcpy(static_cast<char*>(data), perm.data(), permSize);
  memcpy(static_cast<char*>(data) + permSize, permGradIndex3d.data(), permSize);
  vkUnmapMemory(device, stagingBuffer.getMemory());

  computeCommandBuffer.reset();
  computeCommandBuffer.begin();

  VkBufferCopy copyRegion{};
  copyRegion.srcOffset = 0;
  copyRegion.dstOffset = permBuffer.getOffset();
  copyRegion.size      = permSize;
  vkCmdCopyBuffer(computeCommandBuffer.getCommandBuffer(), stagingBuffer.getBuffer(),
                  permBuffer.getBuffer(), 1, &copyRegion);

  copyRegion.srcOffset = permSize;
  copyRegion.dstOffset = permGradIndex3DBuffer.getOffset();
  copyRegion.size      = permSize;
  vkCmdCopyBuffer(computeCommandBuffer.getCommandBuffer(), stagingBuffer.getBuffer(),
                  permGradIndex3DBuffer.getBuffer(), 1, &copyRegion);

  VkBufferMemoryBarrier permBarrier{};
  permBarrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  permBarrier.srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
  permBarrier.dstAccessMask       = VK_ACCESS_SHADER_READ_BIT;
  permBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  permBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  permBarrier.buffer              = permBuffer.getBuffer();
  permBarrier.offset              = permBuffer.getOffset();
  permBarrier.size                = permSize;

  VkBufferMemoryBarrier permGradBarrier{};
  permGradBarrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  permGradBarrier.srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
  permGradBarrier.dstAccessMask       = VK_ACCESS_SHADER_READ_BIT;
  permGradBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  permGradBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  permGradBarrier.buffer              = permGradIndex3DBuffer.getBuffer();
  permGradBarrier.offset              = permGradIndex3DBuffer.getOffset();
  permGradBarrier.size                = permSize;

  std::array<VkBufferMemoryBarrier, 2> permBarriers = {permBarrier, permGradBarrier};
  vkCmdPipelineBarrier(computeCommandBuffer.getCommandBuffer(), VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, permBarriers.size(),
                       permBarriers.data(), 0, nullptr);

  computeCommandBuffer.end();

  VkSubmitInfo submitInfo{};
  submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  VkCommandBuffer computeCmd    = computeCommandBuffer.getCommandBuffer();
  submitInfo.pCommandBuffers    = &computeCmd;

  GameVulkanFence localFence(device, GameVulkanFenceCreateInfo{0});
  GameVulkanQueueSubmitter::submit(computeQueue, &submitInfo, localFence.getFence());

  localFence.wait();
}

void WorldUnitPlacement::dispatch(void*                      pResource,
                                  const GameVulkanSemaphore& waitSemaphore,
                                  GameVulkanFence&           fence)
{
  WorldUnitPlacementComputePResource* pOriginalResource =
      static_cast<WorldUnitPlacementComputePResource*>(pResource);
  WorldUnitPlacementPushConstants& params        = *(pOriginalResource->pParams);
  WorldPlanetData&                 planet        = *(pOriginalResource->pPlanet);
  PreUnitRegistry*                 unitRegistry  = pOriginalResource->pUnitRegistry;
  PreBiomeRegistry*                biomeRegistry = pOriginalResource->pBiomeRegistry;

  if (pOriginalResource->pPlanet != nullptr)
  {
    updatePlanetBuffer(device, physicalDevice, planet);
  }
  if (unitRegistry != nullptr)
  {
    updateUnitRegistryData(device, physicalDevice, *unitRegistry);
  }
  if (biomeRegistry != nullptr)
  {
    updateBiomeRegistryData(device, physicalDevice, *biomeRegistry);
  }

  std::scoped_lock lock(generateMutex);

  completionFence.wait();
  completionFence.reset();

  computeCommandBuffer.reset();
  computeCommandBuffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

  vkCmdBindPipeline(computeCommandBuffer.getCommandBuffer(), VK_PIPELINE_BIND_POINT_COMPUTE,
                    pipeline);
  vkCmdBindDescriptorSets(computeCommandBuffer.getCommandBuffer(), VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

  vkCmdPushConstants(computeCommandBuffer.getCommandBuffer(), pipelineLayout,
                     VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(WorldUnitPlacementPushConstants),
                     &params);

  uint32_t groupCountX = (params.width + 7) / 8;
  uint32_t groupCountY = (params.height + 7) / 8;
  uint32_t groupCountZ = (params.depth + 7) / 8;

  std::array<VkImageMemoryBarrier, 7> inputBarriers{};

  for (int i = 0; i < 3; ++i)
  {
    inputBarriers[i].sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    inputBarriers[i].oldLayout                       = VK_IMAGE_LAYOUT_GENERAL;
    inputBarriers[i].newLayout                       = VK_IMAGE_LAYOUT_GENERAL;
    inputBarriers[i].srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    inputBarriers[i].dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    inputBarriers[i].srcAccessMask                   = VK_ACCESS_SHADER_WRITE_BIT;
    inputBarriers[i].dstAccessMask                   = VK_ACCESS_SHADER_READ_BIT;
    inputBarriers[i].subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    inputBarriers[i].subresourceRange.baseMipLevel   = 0;
    inputBarriers[i].subresourceRange.levelCount     = 1;
    inputBarriers[i].subresourceRange.baseArrayLayer = 0;
    inputBarriers[i].subresourceRange.layerCount     = 1;
  }
  // Retrieve VkImage handles from the opaque image handles returned by the
  // IHeightmap interface. The opaque struct contains a void* pointer to
  // the concrete GameVulkanImage instance for Vulkan implementations.
  {
    const GameOpaqueImageHandle& h0 = heightmapGenerator.getBasemapElevationImage();
    if (h0.handle)
    {
      auto img               = static_cast<const GameVulkanImage*>(h0.handle);
      inputBarriers[0].image = img->getImage();
    }
    else
    {
      inputBarriers[0].image = VK_NULL_HANDLE;
    }

    const GameOpaqueImageHandle& h1 = heightmapGenerator.getBasemapTemperatureImage();
    if (h1.handle)
    {
      auto img               = static_cast<const GameVulkanImage*>(h1.handle);
      inputBarriers[1].image = img->getImage();
    }
    else
    {
      inputBarriers[1].image = VK_NULL_HANDLE;
    }

    const GameOpaqueImageHandle& h2 = heightmapGenerator.getBasemapMoistureImage();
    if (h2.handle)
    {
      auto img               = static_cast<const GameVulkanImage*>(h2.handle);
      inputBarriers[2].image = img->getImage();
    }
    else
    {
      inputBarriers[2].image = VK_NULL_HANDLE;
    }
  }

  for (int i = 3; i < 6; ++i)
  {
    inputBarriers[i].sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    inputBarriers[i].oldLayout                       = VK_IMAGE_LAYOUT_GENERAL;
    inputBarriers[i].newLayout                       = VK_IMAGE_LAYOUT_GENERAL;
    inputBarriers[i].srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    inputBarriers[i].dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    inputBarriers[i].srcAccessMask                   = VK_ACCESS_SHADER_WRITE_BIT;
    inputBarriers[i].dstAccessMask                   = VK_ACCESS_SHADER_READ_BIT;
    inputBarriers[i].subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    inputBarriers[i].subresourceRange.baseMipLevel   = 0;
    inputBarriers[i].subresourceRange.levelCount     = 1;
    inputBarriers[i].subresourceRange.baseArrayLayer = 0;
    inputBarriers[i].subresourceRange.layerCount     = 1;
  }

  {
    const GameOpaqueImageHandle& h3 = heightmapGenerator.getDeepmapElevationImage();
    if (h3.handle)
    {
      auto img               = static_cast<const GameVulkanImage*>(h3.handle);
      inputBarriers[3].image = img->getImage();
    }
    else
    {
      inputBarriers[3].image = VK_NULL_HANDLE;
    }

    const GameOpaqueImageHandle& h4 = heightmapGenerator.getDeepmapTemperatureImage();
    if (h4.handle)
    {
      auto img               = static_cast<const GameVulkanImage*>(h4.handle);
      inputBarriers[4].image = img->getImage();
    }
    else
    {
      inputBarriers[4].image = VK_NULL_HANDLE;
    }

    const GameOpaqueImageHandle& h5 = heightmapGenerator.getDeepmapMoistureImage();
    if (h5.handle)
    {
      auto img               = static_cast<const GameVulkanImage*>(h5.handle);
      inputBarriers[5].image = img->getImage();
    }
    else
    {
      inputBarriers[5].image = VK_NULL_HANDLE;
    }
  }

  inputBarriers[6].sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  inputBarriers[6].oldLayout           = VK_IMAGE_LAYOUT_GENERAL;
  inputBarriers[6].newLayout           = VK_IMAGE_LAYOUT_GENERAL;
  inputBarriers[6].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  inputBarriers[6].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  inputBarriers[6].srcAccessMask       = VK_ACCESS_SHADER_WRITE_BIT;
  inputBarriers[6].dstAccessMask       = VK_ACCESS_SHADER_READ_BIT;
  {
    const GameOpaqueImageHandle& heq = climateCompute.getEquatorImage();
    if (heq.handle)
    {
      auto img               = static_cast<const GameVulkanImage*>(heq.handle);
      inputBarriers[6].image = img->getImage();
    }
    else
    {
      inputBarriers[6].image = VK_NULL_HANDLE;
    }
  }
  inputBarriers[6].subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  inputBarriers[6].subresourceRange.baseMipLevel   = 0;
  inputBarriers[6].subresourceRange.levelCount     = 1;
  inputBarriers[6].subresourceRange.baseArrayLayer = 0;
  inputBarriers[6].subresourceRange.layerCount     = 1;

  vkCmdPipelineBarrier(computeCommandBuffer.getCommandBuffer(),
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       0, 0, nullptr, 0, nullptr, static_cast<uint32_t>(inputBarriers.size()),
                       inputBarriers.data());

  vkCmdDispatch(computeCommandBuffer.getCommandBuffer(), groupCountX, groupCountY, groupCountZ);

  VkImageMemoryBarrier unitBarrier{};
  unitBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  unitBarrier.oldLayout                       = VK_IMAGE_LAYOUT_GENERAL;
  unitBarrier.newLayout                       = VK_IMAGE_LAYOUT_GENERAL;
  unitBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  unitBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  unitBarrier.image                           = unitOutputImage.getImage();
  unitBarrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  unitBarrier.subresourceRange.baseMipLevel   = 0;
  unitBarrier.subresourceRange.levelCount     = 1;
  unitBarrier.subresourceRange.baseArrayLayer = 0;
  unitBarrier.subresourceRange.layerCount     = 1;
  unitBarrier.srcAccessMask                   = VK_ACCESS_SHADER_WRITE_BIT;
  unitBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT;

  VkImageMemoryBarrier biomeBarrier{};
  biomeBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  biomeBarrier.oldLayout                       = VK_IMAGE_LAYOUT_GENERAL;
  biomeBarrier.newLayout                       = VK_IMAGE_LAYOUT_GENERAL;
  biomeBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  biomeBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  biomeBarrier.image                           = biomeOutputImage.getImage();
  biomeBarrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  biomeBarrier.subresourceRange.baseMipLevel   = 0;
  biomeBarrier.subresourceRange.levelCount     = 1;
  biomeBarrier.subresourceRange.baseArrayLayer = 0;
  biomeBarrier.subresourceRange.layerCount     = 1;
  biomeBarrier.srcAccessMask                   = VK_ACCESS_SHADER_WRITE_BIT;
  biomeBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT;

  std::array<VkImageMemoryBarrier, 2> barriers2 = {unitBarrier, biomeBarrier};
  vkCmdPipelineBarrier(
      computeCommandBuffer.getCommandBuffer(), VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT |
          VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
      0, 0, nullptr, 0, nullptr, static_cast<uint32_t>(barriers2.size()), barriers2.data());

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

void WorldUnitPlacement::readUnitOutput(VkDevice         device,
                                        VkPhysicalDevice physicalDevice,
                                        uint32_t*        pOutput,
                                        size_t           outputSize)
{
  std::scoped_lock lock(generateMutex);

  completionFence.wait();
  completionFence.reset();

  GameVulkanBuffer stagingBuffer(
      &memoryAllocator, outputSize * sizeof(uint32_t), VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  computeCommandBuffer.reset();
  computeCommandBuffer.begin();

  VkImageMemoryBarrier imageBarrier{};
  imageBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  imageBarrier.oldLayout                       = VK_IMAGE_LAYOUT_GENERAL;
  imageBarrier.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  imageBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  imageBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  imageBarrier.image                           = unitOutputImageHandle->getImage();
  imageBarrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  imageBarrier.subresourceRange.baseMipLevel   = 0;
  imageBarrier.subresourceRange.levelCount     = 1;
  imageBarrier.subresourceRange.baseArrayLayer = 0;
  imageBarrier.subresourceRange.layerCount     = 1;
  imageBarrier.srcAccessMask                   = VK_ACCESS_SHADER_WRITE_BIT;
  imageBarrier.dstAccessMask                   = VK_ACCESS_TRANSFER_READ_BIT;

  vkCmdPipelineBarrier(computeCommandBuffer.getCommandBuffer(),
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                       nullptr, 0, nullptr, 1, &imageBarrier);

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

  vkCmdCopyImageToBuffer(computeCommandBuffer.getCommandBuffer(), unitOutputImageHandle->getImage(),
                         VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stagingBuffer.getBuffer(), 1,
                         &copyRegion);

  VkBufferMemoryBarrier bufferBarrier{};
  bufferBarrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  bufferBarrier.srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
  bufferBarrier.dstAccessMask       = VK_ACCESS_HOST_READ_BIT;
  bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  bufferBarrier.buffer              = stagingBuffer.getBuffer();
  bufferBarrier.offset              = stagingBuffer.getOffset();
  bufferBarrier.size                = outputSize;
  vkCmdPipelineBarrier(computeCommandBuffer.getCommandBuffer(), VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_HOST_BIT, 0, 0, nullptr, 1, &bufferBarrier, 0, nullptr);

  computeCommandBuffer.end();

  VkSubmitInfo submitInfo{};
  submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  VkCommandBuffer computeCmd    = computeCommandBuffer.getCommandBuffer();
  submitInfo.pCommandBuffers    = &computeCmd;

  GameVulkanFence localFence(device, GameVulkanFenceCreateInfo{0});
  GameVulkanQueueSubmitter::submit(computeQueue, &submitInfo, localFence.getFence());

  localFence.wait();
  localFence.reset();

  void* data;
  vkMapMemory(device, stagingBuffer.getMemory(), stagingBuffer.getOffset(), outputSize, 0, &data);
  memcpy(pOutput, data, outputSize);
  vkUnmapMemory(device, stagingBuffer.getMemory());

  computeCommandBuffer.reset();

  VkCommandBufferBeginInfo beginInfo2{};
  beginInfo2.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo2.flags = 0;

  computeCommandBuffer.begin();

  imageBarrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  imageBarrier.newLayout     = VK_IMAGE_LAYOUT_GENERAL;
  imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
  imageBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

  vkCmdPipelineBarrier(computeCommandBuffer.getCommandBuffer(), VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                       &imageBarrier);

  computeCommandBuffer.end();

  VkSubmitInfo submitInfo2{};
  submitInfo2.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo2.commandBufferCount = 1;
  VkCommandBuffer computeCmdA    = computeCommandBuffer.getCommandBuffer();
  submitInfo2.pCommandBuffers    = &computeCmdA;

  GameVulkanQueueSubmitter::submit(computeQueue, &submitInfo2, localFence.getFence());

  localFence.wait();
}

void WorldUnitPlacement::readBiomeOutput(VkDevice         device,
                                         VkPhysicalDevice physicalDevice,
                                         uint32_t*        pOutput,
                                         size_t           outputSize)
{
  std::scoped_lock lock(generateMutex);

  completionFence.wait();
  completionFence.reset();

  GameVulkanBuffer stagingBuffer(
      &memoryAllocator, outputSize * sizeof(uint32_t), VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  computeCommandBuffer.reset();

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = 0;

  computeCommandBuffer.begin();

  VkImageMemoryBarrier imageBarrier{};
  imageBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  imageBarrier.oldLayout                       = VK_IMAGE_LAYOUT_GENERAL;
  imageBarrier.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  imageBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  imageBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  imageBarrier.image                           = biomeOutputImageHandle->getImage();
  imageBarrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  imageBarrier.subresourceRange.baseMipLevel   = 0;
  imageBarrier.subresourceRange.levelCount     = 1;
  imageBarrier.subresourceRange.baseArrayLayer = 0;
  imageBarrier.subresourceRange.layerCount     = 1;
  imageBarrier.srcAccessMask                   = VK_ACCESS_SHADER_WRITE_BIT;
  imageBarrier.dstAccessMask                   = VK_ACCESS_TRANSFER_READ_BIT;

  vkCmdPipelineBarrier(computeCommandBuffer.getCommandBuffer(),
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                       nullptr, 0, nullptr, 1, &imageBarrier);

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

  vkCmdCopyImageToBuffer(computeCommandBuffer.getCommandBuffer(),
                         biomeOutputImageHandle->getImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                         stagingBuffer.getBuffer(), 1, &copyRegion);

  VkBufferMemoryBarrier bufferBarrier{};
  bufferBarrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  bufferBarrier.srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
  bufferBarrier.dstAccessMask       = VK_ACCESS_HOST_READ_BIT;
  bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  bufferBarrier.buffer              = stagingBuffer.getBuffer();
  bufferBarrier.offset              = stagingBuffer.getOffset();
  bufferBarrier.size                = outputSize;
  vkCmdPipelineBarrier(computeCommandBuffer.getCommandBuffer(), VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_HOST_BIT, 0, 0, nullptr, 1, &bufferBarrier, 0, nullptr);

  computeCommandBuffer.end();

  VkSubmitInfo submitInfo1{};
  submitInfo1.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo1.commandBufferCount = 1;
  VkCommandBuffer computeCmdB    = computeCommandBuffer.getCommandBuffer();
  submitInfo1.pCommandBuffers    = &computeCmdB;

  GameVulkanFence localFence(device, GameVulkanFenceCreateInfo{0});
  GameVulkanQueueSubmitter::submit(computeQueue, &submitInfo1, localFence.getFence());

  localFence.wait();
  localFence.reset();

  void* data;
  vkMapMemory(device, stagingBuffer.getMemory(), stagingBuffer.getOffset(), outputSize, 0, &data);
  memcpy(pOutput, data, outputSize);
  vkUnmapMemory(device, stagingBuffer.getMemory());

  computeCommandBuffer.reset();

  VkCommandBufferBeginInfo beginInfo2{};
  beginInfo2.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo2.flags = 0;

  computeCommandBuffer.begin();

  imageBarrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  imageBarrier.newLayout     = VK_IMAGE_LAYOUT_GENERAL;
  imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
  imageBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

  vkCmdPipelineBarrier(computeCommandBuffer.getCommandBuffer(), VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                       &imageBarrier);

  computeCommandBuffer.end();

  VkSubmitInfo submitInfo2{};
  submitInfo2.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo2.commandBufferCount = 1;
  VkCommandBuffer computeCmdC    = computeCommandBuffer.getCommandBuffer();
  submitInfo2.pCommandBuffers    = &computeCmdC;

  GameVulkanQueueSubmitter::submit(computeQueue, &submitInfo2, localFence.getFence());
  localFence.wait();
}

void WorldUnitPlacement::updateDescriptorSets()
{
  VkPhysicalDeviceProperties deviceProperties{};
  vkGetPhysicalDeviceProperties(memoryAllocator.getPhysicalDevice(), &deviceProperties);
  VkDeviceSize minStorageBufferOffsetAlignment =
      deviceProperties.limits.minStorageBufferOffsetAlignment;

  VkDescriptorBufferInfo unitRegistryBufferInfo{};
  unitRegistryBufferInfo.buffer = unitRegistryBuffer.getBuffer();
  unitRegistryBufferInfo.offset = unitRegistryBuffer.getOffset();
  unitRegistryBufferInfo.range  = unitRegistryBuffer.getSize();

  VkDescriptorBufferInfo biomeRegistryBufferInfo{};
  biomeRegistryBufferInfo.buffer = biomeRegistryBuffer.getBuffer();
  biomeRegistryBufferInfo.offset = biomeRegistryBuffer.getOffset();
  biomeRegistryBufferInfo.range  = biomeRegistryBuffer.getSize();

  VkDescriptorBufferInfo planetBufferInfo{};
  planetBufferInfo.buffer = planetBuffer.getBuffer();
  planetBufferInfo.offset = planetBuffer.getOffset();
  planetBufferInfo.range  = sizeof(WorldPlanetData);

  VkDescriptorImageInfo basemapElevationImageInfo{};
  basemapElevationImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
#if defined(_RL_CHUNK_VULKAN_BACKEND)
  basemapElevationImageInfo.imageView =
      reinterpret_cast<GameVulkanImageView*>(heightmapGenerator.getBasemapElevationImageViewPtr())
          ->getImageView();
  basemapElevationImageInfo.sampler = VK_NULL_HANDLE;

  VkDescriptorImageInfo basemapTemperatureImageInfo{};
  basemapTemperatureImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  basemapTemperatureImageInfo.imageView =
      reinterpret_cast<GameVulkanImageView*>(heightmapGenerator.getBasemapTemperatureImageViewPtr())
          ->getImageView();
  basemapTemperatureImageInfo.sampler = VK_NULL_HANDLE;

  VkDescriptorImageInfo basemapMoistureImageInfo{};
  basemapMoistureImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  basemapMoistureImageInfo.imageView =
      reinterpret_cast<GameVulkanImageView*>(heightmapGenerator.getBasemapMoistureImageViewPtr())
          ->getImageView();
  basemapMoistureImageInfo.sampler = VK_NULL_HANDLE;

  VkDescriptorImageInfo deepmapElevationImageInfo{};
  deepmapElevationImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  deepmapElevationImageInfo.imageView =
      reinterpret_cast<GameVulkanImageView*>(heightmapGenerator.getDeepmapElevationImageViewPtr())
          ->getImageView();
  deepmapElevationImageInfo.sampler = VK_NULL_HANDLE;

  VkDescriptorImageInfo deepmapTemperatureImageInfo{};
  deepmapTemperatureImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  deepmapTemperatureImageInfo.imageView =
      reinterpret_cast<GameVulkanImageView*>(heightmapGenerator.getDeepmapTemperatureImageViewPtr())
          ->getImageView();
  deepmapTemperatureImageInfo.sampler = VK_NULL_HANDLE;

  VkDescriptorImageInfo deepmapMoistureImageInfo{};
  deepmapMoistureImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  deepmapMoistureImageInfo.imageView =
      reinterpret_cast<GameVulkanImageView*>(heightmapGenerator.getDeepmapMoistureImageViewPtr())
          ->getImageView();
  deepmapMoistureImageInfo.sampler = VK_NULL_HANDLE;

  VkDescriptorImageInfo equatorImageInfo{};
  equatorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  equatorImageInfo.imageView =
      reinterpret_cast<GameVulkanImageView*>(climateCompute.getEquatorImageViewPtr())
          ->getImageView();
  equatorImageInfo.sampler = VK_NULL_HANDLE;

  VkDescriptorImageInfo unitOutputImageInfo{};
  unitOutputImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  unitOutputImageInfo.imageView   = unitOutputImageView.getImageView();
  unitOutputImageInfo.sampler     = VK_NULL_HANDLE;

  VkDescriptorImageInfo biomeOutputImageInfo{};
  biomeOutputImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  biomeOutputImageInfo.imageView   = biomeOutputImageView.getImageView();
  biomeOutputImageInfo.sampler     = VK_NULL_HANDLE;
#else
  basemapElevationImageInfo.imageView = VK_NULL_HANDLE;
  basemapElevationImageInfo.sampler   = VK_NULL_HANDLE;

  VkDescriptorImageInfo basemapTemperatureImageInfo{};
  basemapTemperatureImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  basemapTemperatureImageInfo.imageView   = VK_NULL_HANDLE;
  basemapTemperatureImageInfo.sampler     = VK_NULL_HANDLE;

  VkDescriptorImageInfo basemapMoistureImageInfo{};
  basemapMoistureImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  basemapMoistureImageInfo.imageView   = VK_NULL_HANDLE;
  basemapMoistureImageInfo.sampler     = VK_NULL_HANDLE;

  VkDescriptorImageInfo deepmapElevationImageInfo{};
  deepmapElevationImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  deepmapElevationImageInfo.imageView   = VK_NULL_HANDLE;
  deepmapElevationImageInfo.sampler     = VK_NULL_HANDLE;

  VkDescriptorImageInfo deepmapTemperatureImageInfo{};
  deepmapTemperatureImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  deepmapTemperatureImageInfo.imageView   = VK_NULL_HANDLE;
  deepmapTemperatureImageInfo.sampler     = VK_NULL_HANDLE;

  VkDescriptorImageInfo deepmapMoistureImageInfo{};
  deepmapMoistureImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  deepmapMoistureImageInfo.imageView   = VK_NULL_HANDLE;
  deepmapMoistureImageInfo.sampler     = VK_NULL_HANDLE;

  VkDescriptorImageInfo equatorImageInfo{};
  equatorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  equatorImageInfo.imageView   = VK_NULL_HANDLE;
  equatorImageInfo.sampler     = VK_NULL_HANDLE;

  VkDescriptorImageInfo unitOutputImageInfo{};
  unitOutputImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  unitOutputImageInfo.imageView   = VK_NULL_HANDLE;
  unitOutputImageInfo.sampler     = VK_NULL_HANDLE;

  VkDescriptorImageInfo biomeOutputImageInfo{};
  biomeOutputImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  biomeOutputImageInfo.imageView   = VK_NULL_HANDLE;
  biomeOutputImageInfo.sampler     = VK_NULL_HANDLE;
#endif

  VkDescriptorBufferInfo permBufferInfo{};
  permBufferInfo.buffer = permBuffer.getBuffer();
  permBufferInfo.offset = permBuffer.getOffset();
  permBufferInfo.range  = ChunkNoiseGenerator::permBufferSize * sizeof(int32_t);

  VkDescriptorBufferInfo permGradIndex3DBufferInfo{};
  permGradIndex3DBufferInfo.buffer = permGradIndex3DBuffer.getBuffer();
  permGradIndex3DBufferInfo.offset = permGradIndex3DBuffer.getOffset();
  permGradIndex3DBufferInfo.range  = ChunkNoiseGenerator::permBufferSize * sizeof(int32_t);

  std::array<VkWriteDescriptorSet, 14> descriptorWrites{};

  // Binding 0: UnitRegistryBuffer
  descriptorWrites[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[0].dstSet          = descriptorSet;
  descriptorWrites[0].dstBinding      = 0;
  descriptorWrites[0].dstArrayElement = 0;
  descriptorWrites[0].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  descriptorWrites[0].descriptorCount = 1;
  descriptorWrites[0].pBufferInfo     = &unitRegistryBufferInfo;

  // Binding 1: BiomeRegistryBuffer
  descriptorWrites[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[1].dstSet          = descriptorSet;
  descriptorWrites[1].dstBinding      = 1;
  descriptorWrites[1].dstArrayElement = 0;
  descriptorWrites[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  descriptorWrites[1].descriptorCount = 1;
  descriptorWrites[1].pBufferInfo     = &biomeRegistryBufferInfo;

  // Binding 2: PlanetBuffer
  descriptorWrites[2].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[2].dstSet          = descriptorSet;
  descriptorWrites[2].dstBinding      = 2;
  descriptorWrites[2].dstArrayElement = 0;
  descriptorWrites[2].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  descriptorWrites[2].descriptorCount = 1;
  descriptorWrites[2].pBufferInfo     = &planetBufferInfo;

  // Binding 3: basemapElevationImage
  descriptorWrites[3].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[3].dstSet          = descriptorSet;
  descriptorWrites[3].dstBinding      = 3;
  descriptorWrites[3].dstArrayElement = 0;
  descriptorWrites[3].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  descriptorWrites[3].descriptorCount = 1;
  descriptorWrites[3].pImageInfo      = &basemapElevationImageInfo;

  // Binding 4: basemapTemperatureImage
  descriptorWrites[4].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[4].dstSet          = descriptorSet;
  descriptorWrites[4].dstBinding      = 4;
  descriptorWrites[4].dstArrayElement = 0;
  descriptorWrites[4].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  descriptorWrites[4].descriptorCount = 1;
  descriptorWrites[4].pImageInfo      = &basemapTemperatureImageInfo;

  // Binding 5: basemapMoistureImage
  descriptorWrites[5].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[5].dstSet          = descriptorSet;
  descriptorWrites[5].dstBinding      = 5;
  descriptorWrites[5].dstArrayElement = 0;
  descriptorWrites[5].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  descriptorWrites[5].descriptorCount = 1;
  descriptorWrites[5].pImageInfo      = &basemapMoistureImageInfo;

  // Binding 6: deepmapElevationImage
  descriptorWrites[6].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[6].dstSet          = descriptorSet;
  descriptorWrites[6].dstBinding      = 6;
  descriptorWrites[6].dstArrayElement = 0;
  descriptorWrites[6].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  descriptorWrites[6].descriptorCount = 1;
  descriptorWrites[6].pImageInfo      = &deepmapElevationImageInfo;

  // Binding 7: deepmapTemperatureImage
  descriptorWrites[7].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[7].dstSet          = descriptorSet;
  descriptorWrites[7].dstBinding      = 7;
  descriptorWrites[7].dstArrayElement = 0;
  descriptorWrites[7].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  descriptorWrites[7].descriptorCount = 1;
  descriptorWrites[7].pImageInfo      = &deepmapTemperatureImageInfo;

  // Binding 8: deepmapMoistureImage
  descriptorWrites[8].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[8].dstSet          = descriptorSet;
  descriptorWrites[8].dstBinding      = 8;
  descriptorWrites[8].dstArrayElement = 0;
  descriptorWrites[8].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  descriptorWrites[8].descriptorCount = 1;
  descriptorWrites[8].pImageInfo      = &deepmapMoistureImageInfo;

  // Binding 9: equatorImage
  descriptorWrites[9].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[9].dstSet          = descriptorSet;
  descriptorWrites[9].dstBinding      = 9;
  descriptorWrites[9].dstArrayElement = 0;
  descriptorWrites[9].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  descriptorWrites[9].descriptorCount = 1;
  descriptorWrites[9].pImageInfo      = &equatorImageInfo;

  // Binding 10: unitOutputImage
  descriptorWrites[10].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[10].dstSet          = descriptorSet;
  descriptorWrites[10].dstBinding      = 10;
  descriptorWrites[10].dstArrayElement = 0;
  descriptorWrites[10].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  descriptorWrites[10].descriptorCount = 1;
  descriptorWrites[10].pImageInfo      = &unitOutputImageInfo;

  // Binding 11: biomeOutputImage
  descriptorWrites[11].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[11].dstSet          = descriptorSet;
  descriptorWrites[11].dstBinding      = 11;
  descriptorWrites[11].dstArrayElement = 0;
  descriptorWrites[11].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  descriptorWrites[11].descriptorCount = 1;
  descriptorWrites[11].pImageInfo      = &biomeOutputImageInfo;

  // Binding 12: PermutationBuffer
  descriptorWrites[12].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[12].dstSet          = descriptorSet;
  descriptorWrites[12].dstBinding      = 12;
  descriptorWrites[12].dstArrayElement = 0;
  descriptorWrites[12].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  descriptorWrites[12].descriptorCount = 1;
  descriptorWrites[12].pBufferInfo     = &permBufferInfo;

  // Binding 13: PermGradIndex3DBuffer
  descriptorWrites[13].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[13].dstSet          = descriptorSet;
  descriptorWrites[13].dstBinding      = 13;
  descriptorWrites[13].dstArrayElement = 0;
  descriptorWrites[13].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  descriptorWrites[13].descriptorCount = 1;
  descriptorWrites[13].pBufferInfo     = &permGradIndex3DBufferInfo;

  vkUpdateDescriptorSets(device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

void WorldUnitPlacement::initRegistryBuffers(VkDevice device, VkPhysicalDevice physicalDevice)
{
  VkDeviceSize unitBufferSize = sizeof(WorldUnitData);
  unitRegistryBuffer =
      GameVulkanBuffer(&memoryAllocator, unitBufferSize,
                       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  VkDeviceSize biomeBufferSize = sizeof(WorldBiomeData);
  biomeRegistryBuffer =
      GameVulkanBuffer(&memoryAllocator, biomeBufferSize,
                       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void WorldUnitPlacement::createDescriptorSetLayout(VkDevice device)
{
  std::array<VkDescriptorSetLayoutBinding, 14> bindings{};

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
  bindings[3].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  bindings[3].descriptorCount = 1;
  bindings[3].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

  bindings[4].binding         = 4;
  bindings[4].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  bindings[4].descriptorCount = 1;
  bindings[4].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

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

  bindings[8].binding         = 8;
  bindings[8].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  bindings[8].descriptorCount = 1;
  bindings[8].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

  bindings[9].binding         = 9;
  bindings[9].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  bindings[9].descriptorCount = 1;
  bindings[9].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

  bindings[10].binding         = 10;
  bindings[10].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  bindings[10].descriptorCount = 1;
  bindings[10].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

  bindings[11].binding         = 11;
  bindings[11].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  bindings[11].descriptorCount = 1;
  bindings[11].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

  bindings[12].binding         = 12;
  bindings[12].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  bindings[12].descriptorCount = 1;
  bindings[12].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

  bindings[13].binding         = 13;
  bindings[13].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  bindings[13].descriptorCount = 1;
  bindings[13].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings    = bindings.data();

  VkResult result = vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout);
  if (result != VK_SUCCESS)
  {
    GameError::exitWithError("vkCreateDescriptorSetLayout",
                             "Failed to create descriptor set layout "
                             "(result = " +
                                 GameError::vulkanResultToString(result) + ")",
                             device, physicalDevice, instance);
  }
}

void WorldUnitPlacement::createDescriptorPool(VkDevice device)
{
  std::array<VkDescriptorPoolSize, 2> poolSizes{};
  poolSizes[0].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  poolSizes[0].descriptorCount = 5; // planet, unitRegistry, biomeRegistry, perm, permGradIndex3D
  poolSizes[1].type            = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  poolSizes[1].descriptorCount =
      9; // basemapElevation, basemapTemperature, basemapMoisture, deepmapElevation,
         // deepmapTemperature, deepmapMoisture, equator, unitOutput, biomeOutput

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

void WorldUnitPlacement::createDescriptorSets(VkDevice device)
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
                             "Failed to allocate descriptor sets "
                             "(result = " +
                                 GameError::vulkanResultToString(result) + ")",
                             device, physicalDevice, instance);
  }

  VkDescriptorBufferInfo planetBufferInfo{};
  planetBufferInfo.buffer = planetBuffer.getBuffer();
  planetBufferInfo.offset = planetBuffer.getOffset();
  planetBufferInfo.range  = sizeof(WorldPlanetData);

  VkDescriptorImageInfo basemapElevationImageInfo{};
  basemapElevationImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  basemapElevationImageInfo.sampler     = VK_NULL_HANDLE;

  VkDescriptorImageInfo basemapTemperatureImageInfo{};
  basemapTemperatureImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  basemapTemperatureImageInfo.sampler     = VK_NULL_HANDLE;

  VkDescriptorImageInfo basemapMoistureImageInfo{};
  basemapMoistureImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  basemapMoistureImageInfo.sampler     = VK_NULL_HANDLE;

  VkDescriptorImageInfo deepmapElevationImageInfo{};
  deepmapElevationImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  deepmapElevationImageInfo.sampler     = VK_NULL_HANDLE;

  VkDescriptorImageInfo deepmapTemperatureImageInfo{};
  deepmapTemperatureImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  deepmapTemperatureImageInfo.sampler     = VK_NULL_HANDLE;

  VkDescriptorImageInfo deepmapMoistureImageInfo{};
  deepmapMoistureImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  deepmapMoistureImageInfo.sampler     = VK_NULL_HANDLE;

  // Select image view handles depending on backend
#if defined(_RL_CHUNK_VULKAN_BACKEND)
  basemapElevationImageInfo.imageView =
      reinterpret_cast<GameVulkanImageView*>(heightmapGenerator.getBasemapElevationImageViewPtr())
          ->getImageView();
  basemapTemperatureImageInfo.imageView =
      reinterpret_cast<GameVulkanImageView*>(heightmapGenerator.getBasemapTemperatureImageViewPtr())
          ->getImageView();
  basemapMoistureImageInfo.imageView =
      reinterpret_cast<GameVulkanImageView*>(heightmapGenerator.getBasemapMoistureImageViewPtr())
          ->getImageView();
  deepmapElevationImageInfo.imageView =
      reinterpret_cast<GameVulkanImageView*>(heightmapGenerator.getDeepmapElevationImageViewPtr())
          ->getImageView();
  deepmapTemperatureImageInfo.imageView =
      reinterpret_cast<GameVulkanImageView*>(heightmapGenerator.getDeepmapTemperatureImageViewPtr())
          ->getImageView();
  deepmapMoistureImageInfo.imageView =
      reinterpret_cast<GameVulkanImageView*>(heightmapGenerator.getDeepmapMoistureImageViewPtr())
          ->getImageView();

  VkDescriptorImageInfo equatorImageInfo{};
  equatorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  equatorImageInfo.imageView =
      reinterpret_cast<GameVulkanImageView*>(climateCompute.getEquatorImageViewPtr())
          ->getImageView();
  equatorImageInfo.sampler = VK_NULL_HANDLE;
#else
  basemapElevationImageInfo.imageView   = VK_NULL_HANDLE;
  basemapTemperatureImageInfo.imageView = VK_NULL_HANDLE;
  basemapMoistureImageInfo.imageView    = VK_NULL_HANDLE;
  deepmapElevationImageInfo.imageView   = VK_NULL_HANDLE;
  deepmapTemperatureImageInfo.imageView = VK_NULL_HANDLE;
  deepmapMoistureImageInfo.imageView    = VK_NULL_HANDLE;

  VkDescriptorImageInfo equatorImageInfo{};
  equatorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  equatorImageInfo.imageView   = VK_NULL_HANDLE;
  equatorImageInfo.sampler     = VK_NULL_HANDLE;
#endif

  VkDescriptorImageInfo unitOutputImageInfo{};
  unitOutputImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  unitOutputImageInfo.imageView   = unitOutputImageView.getImageView();
  unitOutputImageInfo.sampler     = VK_NULL_HANDLE;

  VkDescriptorImageInfo biomeOutputImageInfo{};
  biomeOutputImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  biomeOutputImageInfo.imageView   = biomeOutputImageView.getImageView();
  biomeOutputImageInfo.sampler     = VK_NULL_HANDLE;

  VkDescriptorBufferInfo permBufferInfo{};
  permBufferInfo.buffer = permBuffer.getBuffer();
  permBufferInfo.offset = permBuffer.getOffset();
  permBufferInfo.range  = ChunkNoiseGenerator::permBufferSize * sizeof(int32_t);

  VkDescriptorBufferInfo permGradIndex3DBufferInfo{};
  permGradIndex3DBufferInfo.buffer = permGradIndex3DBuffer.getBuffer();
  permGradIndex3DBufferInfo.offset = permGradIndex3DBuffer.getOffset();
  permGradIndex3DBufferInfo.range  = ChunkNoiseGenerator::permBufferSize * sizeof(int32_t);

  std::array<VkWriteDescriptorSet, 12> descriptorWrites{};

  // Binding 2: PlanetBuffer
  descriptorWrites[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[0].dstSet          = descriptorSet;
  descriptorWrites[0].dstBinding      = 2;
  descriptorWrites[0].dstArrayElement = 0;
  descriptorWrites[0].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  descriptorWrites[0].descriptorCount = 1;
  descriptorWrites[0].pBufferInfo     = &planetBufferInfo;

  // Binding 3: basemapElevationImage
  descriptorWrites[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[1].dstSet          = descriptorSet;
  descriptorWrites[1].dstBinding      = 3;
  descriptorWrites[1].dstArrayElement = 0;
  descriptorWrites[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  descriptorWrites[1].descriptorCount = 1;
  descriptorWrites[1].pImageInfo      = &basemapElevationImageInfo;

  // Binding 4: basemapTemperatureImage
  descriptorWrites[2].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[2].dstSet          = descriptorSet;
  descriptorWrites[2].dstBinding      = 4;
  descriptorWrites[2].dstArrayElement = 0;
  descriptorWrites[2].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  descriptorWrites[2].descriptorCount = 1;
  descriptorWrites[2].pImageInfo      = &basemapTemperatureImageInfo;

  // Binding 5: basemapMoistureImage
  descriptorWrites[3].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[3].dstSet          = descriptorSet;
  descriptorWrites[3].dstBinding      = 5;
  descriptorWrites[3].dstArrayElement = 0;
  descriptorWrites[3].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  descriptorWrites[3].descriptorCount = 1;
  descriptorWrites[3].pImageInfo      = &basemapMoistureImageInfo;

  // Binding 6: deepmapElevationImage
  descriptorWrites[4].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[4].dstSet          = descriptorSet;
  descriptorWrites[4].dstBinding      = 6;
  descriptorWrites[4].dstArrayElement = 0;
  descriptorWrites[4].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  descriptorWrites[4].descriptorCount = 1;
  descriptorWrites[4].pImageInfo      = &deepmapElevationImageInfo;

  // Binding 7: deepmapTemperatureImage
  descriptorWrites[5].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[5].dstSet          = descriptorSet;
  descriptorWrites[5].dstBinding      = 7;
  descriptorWrites[5].dstArrayElement = 0;
  descriptorWrites[5].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  descriptorWrites[5].descriptorCount = 1;
  descriptorWrites[5].pImageInfo      = &deepmapTemperatureImageInfo;

  // Binding 8: deepmapMoistureImage
  descriptorWrites[6].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[6].dstSet          = descriptorSet;
  descriptorWrites[6].dstBinding      = 8;
  descriptorWrites[6].dstArrayElement = 0;
  descriptorWrites[6].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  descriptorWrites[6].descriptorCount = 1;
  descriptorWrites[6].pImageInfo      = &deepmapMoistureImageInfo;

  // Binding 9: equatorImage
  descriptorWrites[7].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[7].dstSet          = descriptorSet;
  descriptorWrites[7].dstBinding      = 9;
  descriptorWrites[7].dstArrayElement = 0;
  descriptorWrites[7].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  descriptorWrites[7].descriptorCount = 1;
  descriptorWrites[7].pImageInfo      = &equatorImageInfo;

  // Binding 10: unitOutputImage
  descriptorWrites[8].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[8].dstSet          = descriptorSet;
  descriptorWrites[8].dstBinding      = 10;
  descriptorWrites[8].dstArrayElement = 0;
  descriptorWrites[8].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  descriptorWrites[8].descriptorCount = 1;
  descriptorWrites[8].pImageInfo      = &unitOutputImageInfo;

  // Binding 11: biomeOutputImage
  descriptorWrites[9].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[9].dstSet          = descriptorSet;
  descriptorWrites[9].dstBinding      = 11;
  descriptorWrites[9].dstArrayElement = 0;
  descriptorWrites[9].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  descriptorWrites[9].descriptorCount = 1;
  descriptorWrites[9].pImageInfo      = &biomeOutputImageInfo;

  // Binding 12: PermutationBuffer
  descriptorWrites[10].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[10].dstSet          = descriptorSet;
  descriptorWrites[10].dstBinding      = 12;
  descriptorWrites[10].dstArrayElement = 0;
  descriptorWrites[10].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  descriptorWrites[10].descriptorCount = 1;
  descriptorWrites[10].pBufferInfo     = &permBufferInfo;

  // Binding 13: PermGradIndex3DBuffer
  descriptorWrites[11].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[11].dstSet          = descriptorSet;
  descriptorWrites[11].dstBinding      = 13;
  descriptorWrites[11].dstArrayElement = 0;
  descriptorWrites[11].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  descriptorWrites[11].descriptorCount = 1;
  descriptorWrites[11].pBufferInfo     = &permGradIndex3DBufferInfo;

  vkUpdateDescriptorSets(device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

void WorldUnitPlacement::createComputePipeline(VkDevice device)
{
  computeShaderModule = GameShaderLoader::createShaderModule(device, WorldUnitPlaceComp_data,
                                                             WorldUnitPlaceComp_size);

  VkPipelineShaderStageCreateInfo shaderStageInfo{};
  shaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStageInfo.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
  shaderStageInfo.module = computeShaderModule.shaderModule;
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

  VkComputePipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipelineInfo.stage  = shaderStageInfo;
  pipelineInfo.layout = pipelineLayout;

  VkResult result2 =
      vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline);
  if (result2 != VK_SUCCESS)
  {
    GameError::exitWithError("vkCreateComputePipelines",
                             "Failed to create compute pipeline "
                             "(result = " +
                                 GameError::vulkanResultToString(result2) + ")",
                             device, physicalDevice, instance);
  }
}

void WorldUnitPlacement::updateUnitRegistryData(VkDevice               device,
                                                VkPhysicalDevice       physicalDevice,
                                                const PreUnitRegistry& unitRegistry)
{
  std::scoped_lock lock(generateMutex);

  auto                       units = unitRegistry.getItems();
  std::vector<WorldUnitData> unitDataArray;
  unitDataArray.reserve(units.size());

  for (const auto* unit : units)
  {
    WorldUnitData data{};
    data.typeId         = unit->getTypeId();
    data.elevationMin   = unit->getElevationStart();
    data.elevationMax   = unit->getElevationEnd();
    data.moistureMin    = unit->getMoistureStart();
    data.moistureMax    = unit->getMoistureEnd();
    data.temperatureMin = unit->getTemperatureStart();
    data.temperatureMax = unit->getTemperatureEnd();
    data.equatorMin     = unit->getEquatorStart();
    data.equatorMax     = unit->getEquatorEnd();
    data._padding0      = 0;
    data._padding1      = 0;
    data._padding2      = 0;

    unitDataArray.emplace_back(data);
  }

  VkDeviceSize bufferSize = unitDataArray.size() * sizeof(WorldUnitData);

  unitRegistryBuffer =
      GameVulkanBuffer(&memoryAllocator, bufferSize,
                       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  GameVulkanBuffer stagingBuffer(
      &memoryAllocator, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  void* data;
  vkMapMemory(device, stagingBuffer.getMemory(), stagingBuffer.getOffset(), bufferSize, 0, &data);
  memset(data, 0, bufferSize);
  std::memcpy(data, unitDataArray.data(), bufferSize);

  vkUnmapMemory(device, stagingBuffer.getMemory());

  computeCommandBuffer.reset();

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = 0;

  computeCommandBuffer.begin();

  VkBufferCopy copyRegion{};
  copyRegion.srcOffset = 0;
  copyRegion.dstOffset = unitRegistryBuffer.getOffset();
  copyRegion.size      = bufferSize;

  vkCmdCopyBuffer(computeCommandBuffer.getCommandBuffer(), stagingBuffer.getBuffer(),
                  unitRegistryBuffer.getBuffer(), 1, &copyRegion);

  VkBufferMemoryBarrier barrier{};
  barrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  barrier.srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask       = VK_ACCESS_SHADER_READ_BIT;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.buffer              = unitRegistryBuffer.getBuffer();
  barrier.offset              = unitRegistryBuffer.getOffset();
  barrier.size                = bufferSize;
  vkCmdPipelineBarrier(computeCommandBuffer.getCommandBuffer(), VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 1, &barrier, 0,
                       nullptr);

  computeCommandBuffer.end();

  VkSubmitInfo submitInfo{};
  submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  VkCommandBuffer computeCmd3   = computeCommandBuffer.getCommandBuffer();
  submitInfo.pCommandBuffers    = &computeCmd3;

  GameVulkanFence localFence(device, GameVulkanFenceCreateInfo{0});
  GameVulkanQueueSubmitter::submit(computeQueue, &submitInfo, localFence.getFence());

  localFence.wait();

  updateDescriptorSets();
}

void WorldUnitPlacement::updateBiomeRegistryData(VkDevice                device,
                                                 VkPhysicalDevice        physicalDevice,
                                                 const PreBiomeRegistry& biomeRegistry)
{
  std::scoped_lock lock(generateMutex);

  std::vector<PreBiome*>      biomes = biomeRegistry.getItems();
  std::vector<WorldBiomeData> biomeDataArray;
  biomeDataArray.clear();

  for (const auto* biome : biomes)
  {
    WorldBiomeData data{};
    data.typeId         = biome->getTypeId();
    data.elevationMin   = biome->getStartElevation();
    data.elevationMax   = biome->getEndElevation();
    data.moistureMin    = biome->getStartMoisture();
    data.moistureMax    = biome->getEndMoisture();
    data.temperatureMin = biome->getStartTemperature();
    data.temperatureMax = biome->getEndTemperature();
    data.equatorMin     = biome->getStartEquator();
    data.equatorMax     = biome->getEndEquator();
    data._padding0      = 0;
    data._padding1      = 0;
    data._padding2      = 0;

    biomeDataArray.emplace_back(data);
  }

  VkDeviceSize bufferSize = biomeDataArray.size() * sizeof(WorldBiomeData);

  biomeRegistryBuffer =
      GameVulkanBuffer(&memoryAllocator, bufferSize,
                       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  GameVulkanBuffer stagingBuffer(
      &memoryAllocator, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  void* data;
  vkMapMemory(device, stagingBuffer.getMemory(), stagingBuffer.getOffset(), bufferSize, 0, &data);
  memset(data, 0, bufferSize);
  std::memcpy(data, biomeDataArray.data(), bufferSize);

  vkUnmapMemory(device, stagingBuffer.getMemory());

  computeCommandBuffer.reset();

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = 0;

  computeCommandBuffer.begin();

  VkBufferCopy copyRegion{};
  copyRegion.srcOffset = 0;
  copyRegion.dstOffset = biomeRegistryBuffer.getOffset();
  copyRegion.size      = bufferSize;

  vkCmdCopyBuffer(computeCommandBuffer.getCommandBuffer(), stagingBuffer.getBuffer(),
                  biomeRegistryBuffer.getBuffer(), 1, &copyRegion);

  VkBufferMemoryBarrier barrier{};
  barrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  barrier.srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask       = VK_ACCESS_SHADER_READ_BIT;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.buffer              = biomeRegistryBuffer.getBuffer();
  barrier.offset              = biomeRegistryBuffer.getOffset();
  barrier.size                = bufferSize;
  vkCmdPipelineBarrier(computeCommandBuffer.getCommandBuffer(), VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 1, &barrier, 0,
                       nullptr);

  computeCommandBuffer.end();

  VkSubmitInfo submitInfo{};
  submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  VkCommandBuffer computeCmd3   = computeCommandBuffer.getCommandBuffer();
  submitInfo.pCommandBuffers    = &computeCmd3;

  GameVulkanFence localFence(device, GameVulkanFenceCreateInfo{0});
  GameVulkanQueueSubmitter::submit(computeQueue, &submitInfo, localFence.getFence());

  localFence.wait();

  updateDescriptorSets();
}

void WorldUnitPlacement::updatePlanetBuffer(VkDevice               device,
                                            VkPhysicalDevice       physicalDevice,
                                            const WorldPlanetData& planet)
{
  std::scoped_lock lock(generateMutex);

  GameVulkanBuffer stagingBuffer(
      &memoryAllocator, sizeof(WorldPlanetData), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  void* data;
  vkMapMemory(device, stagingBuffer.getMemory(), stagingBuffer.getOffset(), sizeof(WorldPlanetData),
              0, &data);
  std::memcpy(data, &planet, sizeof(WorldPlanetData));
  vkUnmapMemory(device, stagingBuffer.getMemory());

  computeCommandBuffer.reset();

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = 0;

  computeCommandBuffer.begin();

  VkBufferCopy copyRegion{};
  copyRegion.srcOffset = 0;
  copyRegion.dstOffset = 0;
  copyRegion.size      = sizeof(WorldPlanetData);

  vkCmdCopyBuffer(computeCommandBuffer.getCommandBuffer(), stagingBuffer.getBuffer(),
                  planetBuffer.getBuffer(), 1, &copyRegion);

  VkBufferMemoryBarrier barrier{};
  barrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  barrier.srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask       = VK_ACCESS_SHADER_READ_BIT;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.buffer              = planetBuffer.getBuffer();
  barrier.offset              = 0;
  barrier.size                = sizeof(WorldPlanetData);
  vkCmdPipelineBarrier(computeCommandBuffer.getCommandBuffer(), VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 1, &barrier, 0,
                       nullptr);

  computeCommandBuffer.end();

  VkSubmitInfo submitInfo{};
  submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  VkCommandBuffer computeCmd4   = computeCommandBuffer.getCommandBuffer();
  submitInfo.pCommandBuffers    = &computeCmd4;

  GameVulkanFence localFence(device, GameVulkanFenceCreateInfo{0});
  GameVulkanQueueSubmitter::submit(computeQueue, &submitInfo, localFence.getFence());

  localFence.wait();
}

void WorldUnitPlacement::createUnitOutputImage(VkDevice device, VkPhysicalDevice physicalDevice)
{
  std::scoped_lock lock(generateMutex);

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

  unitOutputImage = GameVulkanImage(device, physicalDevice, imageInfo);

  computeCommandBuffer.reset();

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = 0;

  computeCommandBuffer.begin();

  VkImageMemoryBarrier barrier{};
  barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
  barrier.newLayout                       = VK_IMAGE_LAYOUT_GENERAL;
  barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.image                           = unitOutputImage.getImage();
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
  submitInfo.sType               = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount  = 1;
  VkCommandBuffer computeCmdUnit = computeCommandBuffer.getCommandBuffer();
  submitInfo.pCommandBuffers     = &computeCmdUnit;

  GameVulkanFence localFence(device, GameVulkanFenceCreateInfo{0});
  GameVulkanQueueSubmitter::submit(computeQueue, &submitInfo, localFence.getFence());

  localFence.wait();
}

const GameOpaqueImageHandle& WorldUnitPlacement::getUnitOutputImage() const
{
  return unitOutputImageHandle.getHandleStruct();
}

const GameOpaqueImageHandle& WorldUnitPlacement::getBiomeOutputImage() const
{
  return biomeOutputImageHandle.getHandleStruct();
}

std::recursive_mutex& WorldUnitPlacement::getGenerateMutex()
{
  return generateMutex;
}

const GameOpaqueSyncHandle& WorldUnitPlacement::getCompletionHandle() const
{
  return completionHandle.getHandleStruct();
}

const GameVulkanSemaphore& WorldUnitPlacement::getCompletionSemaphore() const
{
  return completionSemaphore;
}

GameVulkanSemaphore& WorldUnitPlacement::getCompletionSemaphore()
{
  return completionSemaphore;
}

const GameVulkanFence& WorldUnitPlacement::getCompletionFence() const
{
  return completionFence;
}

GameVulkanFence& WorldUnitPlacement::getCompletionFence()
{
  return completionFence;
}

void WorldUnitPlacement::createUnitOutputImageView(VkDevice device)
{
  GameVulkanImageViewCreateInfo viewInfo{};
  viewInfo.image                           = unitOutputImage.getImage();
  viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_3D;
  viewInfo.format                          = VK_FORMAT_R32_UINT;
  viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.baseMipLevel   = 0;
  viewInfo.subresourceRange.levelCount     = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount     = 1;

  unitOutputImageView = GameVulkanImageView(device, viewInfo);
}

void WorldUnitPlacement::createBiomeOutputImage(VkDevice device, VkPhysicalDevice physicalDevice)
{
  std::scoped_lock lock(generateMutex);

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

  biomeOutputImage = GameVulkanImage(device, physicalDevice, imageInfo);

  computeCommandBuffer.reset();

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = 0;

  computeCommandBuffer.begin();

  VkImageMemoryBarrier barrier{};
  barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
  barrier.newLayout                       = VK_IMAGE_LAYOUT_GENERAL;
  barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.image                           = biomeOutputImage.getImage();
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
  submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  VkCommandBuffer computeCmd5   = computeCommandBuffer.getCommandBuffer();
  submitInfo.pCommandBuffers    = &computeCmd5;

  GameVulkanFence localFence(device, GameVulkanFenceCreateInfo{0});
  GameVulkanQueueSubmitter::submit(computeQueue, &submitInfo, localFence.getFence());

  localFence.wait();
}

void WorldUnitPlacement::createBiomeOutputImageView(VkDevice device)
{
  GameVulkanImageViewCreateInfo viewInfo{};
  viewInfo.image                           = biomeOutputImage.getImage();
  viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_3D;
  viewInfo.format                          = VK_FORMAT_R32_UINT;
  viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.baseMipLevel   = 0;
  viewInfo.subresourceRange.levelCount     = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount     = 1;

  biomeOutputImageView = GameVulkanImageView(device, viewInfo);
}

#if defined(_RL_CHUNK_VULKAN_BACKEND)
void* WorldUnitPlacement::getUnitOutputImageViewPtr() const
{
  return const_cast<GameVulkanImageView*>(&unitOutputImageView);
}

void* WorldUnitPlacement::getBiomeOutputImageViewPtr() const
{
  return const_cast<GameVulkanImageView*>(&biomeOutputImageView);
}
#endif

} // namespace rl
