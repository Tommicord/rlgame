import Rl.World.Unit.UnitRegistryGPU;

import Rl.World.Unit.UnitGPUParams;
import Rl.RayLog.Macro;
import Rl.RayLog.Logger;

import <cstring>;
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

  this->device = device;
  this->physicalDevice = physicalDevice;

  // Pre-allocate staging buffer (max expected size: 512 units * 128 bytes = 64KB)
  constexpr VkDeviceSize maxStagingSize = 131072; // 128KB
  if (!CreateBuffer(device, physicalDevice, maxStagingSize,
                   VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                   stagingBuffer, stagingMemory))
  {
    RayLog::LogError(RAYLOG_TAG, "Failed to create staging buffer");
    return false;
  }

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
  gpuDirty = false;
  initialized = false;

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

  // Calculate required buffer size
  VkDeviceSize newUnitSize = cpuUnits.size() * sizeof(UnitGPUParams);

  // Recreate buffer if size changed or first time
  if (newUnitSize > unitBufferSize || unitBuffer == VK_NULL_HANDLE)
  {
    if (unitBuffer != VK_NULL_HANDLE)
    {
      vkDestroyBuffer(device, unitBuffer, nullptr);
      vkFreeMemory(device, unitMemory, nullptr);
    }

    if (!CreateBuffer(device, physicalDevice, newUnitSize,
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     unitBuffer, unitMemory))
    {
      RayLog::LogError(RAYLOG_TAG, "Failed to create unit buffer");
      return false;
    }
    unitBufferSize = newUnitSize;
  }

  // Sort units by unitId for binary search optimization in compute shaders
  std::sort(cpuUnits.begin(), cpuUnits.end(),
            [](const UnitGPUParams& a, const UnitGPUParams& b) {
              return a.unitId < b.unitId;
            });

  // Copy unit data to staging buffer
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
  copyRegion.size = newUnitSize;
  vkCmdCopyBuffer(commandBuffer, stagingBuffer, unitBuffer, 1, &copyRegion);

  gpuDirty = false;
  RayLog::LogInfo(RAYLOG_TAG, "GPU buffer updated: %u units",
                 static_cast<uint32_t>(cpuUnits.size()));

  return true;
}

bool UnitRegistryGPU::CreateBuffer(VkDevice device, VkPhysicalDevice physicalDevice,
                                   VkDeviceSize size, VkBufferUsageFlags usage,
                                   VkMemoryPropertyFlags properties,
                                   VkBuffer& buffer, VkDeviceMemory& memory)
{
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
  {
    RayLog::LogError(RAYLOG_TAG, "Failed to create buffer");
    return false;
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

  if (vkAllocateMemory(device, &allocInfo, nullptr, &memory) != VK_SUCCESS)
  {
    RayLog::LogError(RAYLOG_TAG, "Failed to allocate buffer memory");
    vkDestroyBuffer(device, buffer, nullptr);
    return false;
  }

  vkBindBufferMemory(device, buffer, memory, 0);
  return true;
}

uint32_t UnitRegistryGPU::FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter,
                                        VkMemoryPropertyFlags properties)
{
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
  {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
    {
      return i;
    }
  }

  RayLog::LogError(RAYLOG_TAG, "Failed to find suitable memory type");
  return UINT32_MAX;
}

} // namespace Rl::World
