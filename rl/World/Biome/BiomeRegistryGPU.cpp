import Rl.World.Biome.BiomeRegistryGPU;

import Rl.Client.Render.Buffer;
import Rl.World.Biome;
import Rl.RayLog.Macro;
import Rl.RayLog.Logger;

import <cstring>;
import <stdexcept>;
import <vulkan/vulkan.hpp>;
import Rl.World.Biome.BiomeGPUParams;

namespace Rl::World::Biome
{

bool BiomeRegistryGPU::Initialize(VkDevice device, VkPhysicalDevice physicalDevice)
{
  if (initialized)
  {
    RayLog::LogWarning(RAYLOG_TAG, "BiomeRegistryGPU already initialized");
    return true;
  }

  this->device         = device;
  this->physicalDevice = physicalDevice;

  // Pre-allocate staging buffer (max expected size: 256 biomes * 64 bytes = 16KB)
  constexpr VkDeviceSize maxStagingSize = 65536; // 64KB
  Client::Render::CreateBuffer(
      device, physicalDevice, maxStagingSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      stagingBuffer, stagingMemory);

  initialized = true;
  RayLog::LogInfo(RAYLOG_TAG, "BiomeRegistryGPU initialized successfully");
  return true;
}

void BiomeRegistryGPU::Shutdown(VkDevice device)
{
  if (!initialized)
    return;

  vkDeviceWaitIdle(device);

  if (biomeBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device, biomeBuffer, nullptr);
    biomeBuffer = VK_NULL_HANDLE;
  }
  if (biomeMemory != VK_NULL_HANDLE)
  {
    vkFreeMemory(device, biomeMemory, nullptr);
    biomeMemory = VK_NULL_HANDLE;
  }

  if (unitRulesBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device, unitRulesBuffer, nullptr);
    unitRulesBuffer = VK_NULL_HANDLE;
  }
  if (unitRulesMemory != VK_NULL_HANDLE)
  {
    vkFreeMemory(device, unitRulesMemory, nullptr);
    unitRulesMemory = VK_NULL_HANDLE;
  }

  if (stagingBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    stagingBuffer = VK_NULL_HANDLE;
  }
  if (stagingMemory != VK_NULL_HANDLE)
  {
    vkFreeMemory(device, stagingMemory, nullptr);
    stagingMemory = VK_NULL_HANDLE;
  }

  cpuBiomes.clear();
  cpuUnitRules.clear();
  biomeBufferSize     = 0;
  unitRulesBufferSize = 0;
  gpuDirty            = false;
  initialized         = false;

  RayLog::LogInfo(RAYLOG_TAG, "BiomeRegistryGPU shutdown complete");
}

bool BiomeRegistryGPU::UpdateGPUBuffer(VkDevice device, VkCommandBuffer commandBuffer)
{
  if (!initialized)
  {
    RayLog::LogError(RAYLOG_TAG, "Cannot update GPU buffer: registry not initialized");
    return false;
  }

  if (!gpuDirty && biomeBuffer != VK_NULL_HANDLE)
  {
    return true; // No changes needed
  }

  // Calculate required buffer sizes
  VkDeviceSize newBiomeSize     = cpuBiomes.size() * sizeof(BiomeGPUParams);
  VkDeviceSize newUnitRulesSize = cpuUnitRules.size() * sizeof(BiomeUnitRuleGPU);

  // Recreate buffers if size changed or first time
  if (newBiomeSize > biomeBufferSize || biomeBuffer == VK_NULL_HANDLE)
  {
    if (biomeBuffer != VK_NULL_HANDLE)
    {
      vkDestroyBuffer(device, biomeBuffer, nullptr);
      vkFreeMemory(device, biomeMemory, nullptr);
    }

    Client::Render::CreateBuffer(
        device, physicalDevice, newBiomeSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, biomeBuffer, biomeMemory);
    biomeBufferSize = newBiomeSize;
  }

  if (newUnitRulesSize > unitRulesBufferSize || unitRulesBuffer == VK_NULL_HANDLE)
  {
    if (unitRulesBuffer != VK_NULL_HANDLE)
    {
      vkDestroyBuffer(device, unitRulesBuffer, nullptr);
      vkFreeMemory(device, unitRulesMemory, nullptr);
    }

    Client::Render::CreateBuffer(
        device, physicalDevice, newUnitRulesSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, unitRulesBuffer, unitRulesMemory);
    unitRulesBufferSize = newUnitRulesSize;
  }

  // Copy biome data to staging buffer
  void* stagingData = nullptr;
  if (vkMapMemory(device, stagingMemory, 0, newBiomeSize + newUnitRulesSize, 0,
                  &stagingData) != VK_SUCCESS)
  {
    RayLog::LogError(RAYLOG_TAG, "Failed to map staging memory");
    return false;
  }

  memcpy(stagingData, cpuBiomes.data(), newBiomeSize);
  memcpy(static_cast<uint8_t*>(stagingData) + newBiomeSize, cpuUnitRules.data(),
         newUnitRulesSize);
  vkUnmapMemory(device, stagingMemory);

  // Copy from staging to GPU buffers
  VkBufferCopy biomeCopy{};
  biomeCopy.srcOffset = 0;
  biomeCopy.dstOffset = 0;
  biomeCopy.size      = newBiomeSize;
  vkCmdCopyBuffer(commandBuffer, stagingBuffer, biomeBuffer, 1, &biomeCopy);

  VkBufferCopy rulesCopy{};
  rulesCopy.srcOffset = newBiomeSize;
  rulesCopy.dstOffset = 0;
  rulesCopy.size      = newUnitRulesSize;
  vkCmdCopyBuffer(commandBuffer, stagingBuffer, unitRulesBuffer, 1, &rulesCopy);

  gpuDirty = false;
  RayLog::LogInfo(RAYLOG_TAG, "GPU buffers updated: %u biomes, %u unit rules",
                  static_cast<uint32_t>(cpuBiomes.size()),
                  static_cast<uint32_t>(cpuUnitRules.size()));

  return true;
}

} // namespace Rl::World::Biome
