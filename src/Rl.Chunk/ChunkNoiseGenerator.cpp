#include "Rl.Chunk/ChunkNoiseGenerator.h"
#include "Rl.Base/GameVulkanFence.h"
#include "Rl.Base/GameError.h"
#include "Rl.Base/GameVulkanBuffer.h"
#include "Rl.Base/GameVulkanAllocator.h"
#include "Rl.Base/GameVulkanQueueSubmitter.h"
#include "Rl.Base/GameError.h"

#include <numeric>
#include <stdexcept>
#include <vulkan/vulkan.hpp>

namespace rl
{

void ChunkNoiseGenerator::genPermutations(const uint32_t                       seed,
                                          std::array<int32_t, permBufferSize>& perm,
                                          std::array<int32_t, permBufferSize>& permGradIndex3d)
{
  std::array<int32_t, permBufferSize> source{};
  std::iota(source.begin(), source.end(), 0);

  uint64_t state = static_cast<uint64_t>(seed) * 6364136223846793005ull + 1442695040888963407ull;

  for (int i = permBufferSize - 1; i >= 0; --i)
  {
    state     = state * 6364136223846793005ull + 1442695040888963407ull;
    int32_t r = static_cast<int32_t>((state + 31) % static_cast<uint64_t>(i + 1));
    if (r < 0)
    {
      r += (i + 1);
    }
    perm[i]   = source[r];
    source[r] = source[i];
  }
  const int32_t gradientCount = 24;
  for (int i = 0; i < permBufferSize; ++i)
  {
    permGradIndex3d[i] = perm[i] % gradientCount;
  }
}

ChunkNoiseGenerator::ChunkNoiseGenerator(uint32_t seed, GameDeviceInstance& instance) :
    device(instance.getDevice()), computeQueue(instance.getGraphicsQueue()),
    physicalDevice(instance.getPhysicalDevice()), instance(instance.getInstance()),
    computeCommandPool(device, instance.getGraphicsFamily()),
    computeCommandBuffer(device, computeCommandPool.getCommandPool()),
    memoryAllocator(instance.getDevice(), instance.getPhysicalDevice()),
    permBuffer(&memoryAllocator,
               permBufferSize * sizeof(int32_t),
               VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    permGradBuffer(&memoryAllocator,
                   permBufferSize * sizeof(int32_t),
                   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    fence(device, GameVulkanFenceCreateInfo{0}), seed(seed)
{
  updatePermutationBuffers(device, instance.getPhysicalDevice(), seed);
}

void ChunkNoiseGenerator::updatePermutationBuffers(VkDevice         device,
                                                   VkPhysicalDevice physicalDevice,
                                                   uint32_t         newSeed)
{
  seed = newSeed;
  updatePermutationBuffers(device, physicalDevice);
}

void ChunkNoiseGenerator::updatePermutationBuffers(VkDevice device, VkPhysicalDevice physicalDevice)
{
  std::scoped_lock lock(generateMutex);

  std::array<int32_t, permBufferSize> perm{};
  std::array<int32_t, permBufferSize> permGradIndex3d{};
  genPermutations(seed, perm, permGradIndex3d);

  VkDeviceSize permSize = permBufferSize * sizeof(int32_t);

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
  copyRegion.dstOffset = permGradBuffer.getOffset();
  copyRegion.size      = permSize;
  vkCmdCopyBuffer(computeCommandBuffer.getCommandBuffer(), stagingBuffer.getBuffer(),
                  permGradBuffer.getBuffer(), 1, &copyRegion);

  computeCommandBuffer.end();

  VkSubmitInfo submitInfo{};
  submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount   = 1;
  const VkCommandBuffer cmdBuffer = computeCommandBuffer.getCommandBuffer();
  submitInfo.pCommandBuffers      = &cmdBuffer;

  GameVulkanQueueSubmitter::submit(computeQueue, &submitInfo, fence.getFence());

  fence.wait();
  fence.reset();
}

} // namespace rl
