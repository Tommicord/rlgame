import Rl.World.Unit.UnitRegistryGPU;

import Rl.World.Unit.UnitGPUParams;
import Rl.Client.Render.Buffer;
import Rl.RayLog.Macro;
import Rl.RayLog.Logger;

import <stdexcept>;
import <algorithm>;
import <vulkan/vulkan.hpp>;

namespace Rl::World
{

UnitRegistryGPU::~UnitRegistryGPU()
{
  Shutdown(device);
}

bool UnitRegistryGPU::Initialize(VkDevice device, VkPhysicalDevice physicalDevice)
{
  if (initialized)
  {
    RayLog::LogWarning(RAYLOG_TAG, "UnitRegistryGPU already initialized");
    return true;
  }

  this->device         = device;
  this->physicalDevice = physicalDevice;

  constexpr VkDeviceSize maxStagingSize = 131072; // 128KB
  Client::Render::CreateBuffer(
      device, physicalDevice, maxStagingSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      stagingBuffer, stagingMemory);
  initialized = true;
  RayLog::LogInfo(RAYLOG_TAG, "UnitRegistryGPU initialized successfully");
  return true;
}

void UnitRegistryGPU::Shutdown(VkDevice device)
{
  if (!initialized)
    return;

  vkDeviceWaitIdle(device);

  if (unitBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device, unitBuffer, nullptr);
    unitBuffer = VK_NULL_HANDLE;
  }
  if (unitMemory != VK_NULL_HANDLE)
  {
    vkFreeMemory(device, unitMemory, nullptr);
    unitMemory = VK_NULL_HANDLE;
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

  cpuUnits.clear();
  unitBufferSize = 0;
  gpuDirty       = false;
  initialized    = false;

  RayLog::LogInfo(RAYLOG_TAG, "UnitRegistryGPU shutdown complete");
}

bool UnitRegistryGPU::UpdateGPUBuffer(VkDevice device, VkCommandBuffer commandBuffer)
{
  if (!initialized)
  {
    RayLog::LogError(RAYLOG_TAG, "Cannot update GPU buffer: registry not initialized");
    return false;
  }

  if (!gpuDirty && unitBuffer != VK_NULL_HANDLE)
  {
    return true; // No changes needed
  }
  VkDeviceSize newUnitSize = cpuUnits.size() * sizeof(UnitGPUParams);
  
  // Skip update if no units registered
  if (newUnitSize == 0)
  {
    RayLog::LogWarning(RAYLOG_TAG, "No units registered, skipping GPU buffer update");
    return true;
  }
  
  if (newUnitSize > unitBufferSize || unitBuffer == VK_NULL_HANDLE)
  {
    if (unitBuffer != VK_NULL_HANDLE)
    {
      vkDestroyBuffer(device, unitBuffer, nullptr);
      vkFreeMemory(device, unitMemory, nullptr);
    }

    Client::Render::CreateBuffer(
        device, physicalDevice, newUnitSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, unitBuffer, unitMemory);
    unitBufferSize = newUnitSize;
  }
  void* stagingData = nullptr;
  if (vkMapMemory(device, stagingMemory, 0, newUnitSize, 0, &stagingData) != VK_SUCCESS)
  {
    RayLog::LogError(RAYLOG_TAG, "Failed to map staging memory");
    return false;
  }

  memcpy(stagingData, cpuUnits.data(), newUnitSize);
  vkUnmapMemory(device, stagingMemory);

  // Copy from staging to GPU buffer
  VkBufferCopy copyRegion{};
  copyRegion.srcOffset = 0;
  copyRegion.dstOffset = 0;
  copyRegion.size      = newUnitSize;
  vkCmdCopyBuffer(commandBuffer, stagingBuffer, unitBuffer, 1, &copyRegion);

  gpuDirty = false;
  RayLog::LogInfo(RAYLOG_TAG, "GPU buffer updated: %u units",
                  static_cast<uint32_t>(cpuUnits.size()));

  return true;
}

} // namespace Rl::World
