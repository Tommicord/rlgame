import Rl.World.Biome.BiomeRegistryGPU;

import Rl.World.Biome.IBiome;
import Rl.World.Biome.BaseBiome;
import Rl.RayLog.Macro;
import Rl.RayLog.Logger;

import <cstring>;
import <stdexcept>;
import <vulkan/vulkan.hpp>;

namespace Rl::World::Biome
{

BiomeRegistryGPU::~BiomeRegistryGPU()
{
  // Cleanup should be called explicitly via Shutdown()
}

bool BiomeRegistryGPU::Initialize(VkDevice device, VkPhysicalDevice physicalDevice)
{
  if (initialized)
  {
    RayLog::LogWarning(RAYLOG_TAG, "BiomeRegistryGPU already initialized");
    return true;
  }

  this->device = device;
  this->physicalDevice = physicalDevice;

  // Pre-allocate staging buffer (max expected size: 256 biomes * 64 bytes = 16KB)
  constexpr VkDeviceSize maxStagingSize = 65536; // 64KB
  if (!CreateBuffer(device, physicalDevice, maxStagingSize,
                   VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                   stagingBuffer, stagingMemory))
  {
    RayLog::LogError(RAYLOG_TAG, "Failed to create staging buffer");
    return false;
  }

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
  biomeBufferSize = 0;
  unitRulesBufferSize = 0;
  gpuDirty = false;
  initialized = false;

  RayLog::LogInfo(RAYLOG_TAG, "BiomeRegistryGPU shutdown complete");
}

void BiomeRegistryGPU::RegisterBiome(const IBiome& biome)
{
  if (!initialized)
  {
    RayLog::LogError(RAYLOG_TAG, "Cannot register biome: registry not initialized");
    return;
  }

  // Create GPU params from biome
  BiomeGPUParams params{};
  params.biomeType = biome.GetBiomeType();

  // Extract temperature layer
  auto tempLayer = biome.GetTemperatureNoiseLayer();
  params.temperatureBase = 0.5f; // Default, can be customized
  params.temperatureVariation = tempLayer.persistence;

  // Extract moisture layer
  auto moistLayer = biome.GetMoistureNoiseLayer();
  params.moistureBase = 0.5f; // Default, can be customized
  params.moistureVariation = moistLayer.persistence;

  // Extract elevation layer
  auto elevLayer = biome.GetElevationNoiseLayer();
  params.elevationBase = 0.5f; // Default, can be customized
  params.elevationVariation = elevLayer.persistence;

  // Count unit rules
  const auto& rules = biome.GetUnitRules();
  params.unitRuleCount = static_cast<uint32_t>(rules.size());

  cpuBiomes.push_back(params);

  // Convert unit rules to GPU format
  for (const auto& rule : rules)
  {
    BiomeUnitRuleGPU gpuRule{};
    gpuRule.unitId = rule.unitId;
    gpuRule.minHeight = rule.minHeight;
    gpuRule.maxHeight = rule.maxHeight;
    gpuRule.minTemperature = rule.minTemperature;
    gpuRule.maxTemperature = rule.maxTemperature;
    gpuRule.minMoisture = rule.minMoisture;
    gpuRule.maxMoisture = rule.maxMoisture;
    gpuRule.minElevation = rule.minElevation;
    gpuRule.maxElevation = rule.maxElevation;
    gpuRule.probability = rule.probability;
    gpuRule.density = rule.density;
    gpuRule.padding[0] = 0.0f;
    gpuRule.padding[1] = 0.0f;

    cpuUnitRules.push_back(gpuRule);
  }

  gpuDirty = true;
  RayLog::LogInfo(RAYLOG_TAG, "Registered biome: %s (ID: %u, Rules: %u)",
                 biome.GetBiomeName(), params.biomeType, params.unitRuleCount);
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
  VkDeviceSize newBiomeSize = cpuBiomes.size() * sizeof(BiomeGPUParams);
  VkDeviceSize newUnitRulesSize = cpuUnitRules.size() * sizeof(BiomeUnitRuleGPU);

  // Recreate buffers if size changed or first time
  if (newBiomeSize > biomeBufferSize || biomeBuffer == VK_NULL_HANDLE)
  {
    if (biomeBuffer != VK_NULL_HANDLE)
    {
      vkDestroyBuffer(device, biomeBuffer, nullptr);
      vkFreeMemory(device, biomeMemory, nullptr);
    }

    if (!CreateBuffer(device, physicalDevice, newBiomeSize,
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     biomeBuffer, biomeMemory))
    {
      RayLog::LogError(RAYLOG_TAG, "Failed to create biome buffer");
      return false;
    }
    biomeBufferSize = newBiomeSize;
  }

  if (newUnitRulesSize > unitRulesBufferSize || unitRulesBuffer == VK_NULL_HANDLE)
  {
    if (unitRulesBuffer != VK_NULL_HANDLE)
    {
      vkDestroyBuffer(device, unitRulesBuffer, nullptr);
      vkFreeMemory(device, unitRulesMemory, nullptr);
    }

    if (!CreateBuffer(device, physicalDevice, newUnitRulesSize,
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     unitRulesBuffer, unitRulesMemory))
    {
      RayLog::LogError(RAYLOG_TAG, "Failed to create unit rules buffer");
      return false;
    }
    unitRulesBufferSize = newUnitRulesSize;
  }

  // Copy biome data to staging buffer
  void* stagingData = nullptr;
  if (vkMapMemory(device, stagingMemory, 0, newBiomeSize + newUnitRulesSize, 0, &stagingData) != VK_SUCCESS)
  {
    RayLog::LogError(RAYLOG_TAG, "Failed to map staging memory");
    return false;
  }

  memcpy(stagingData, cpuBiomes.data(), newBiomeSize);
  memcpy(static_cast<uint8_t*>(stagingData) + newBiomeSize, cpuUnitRules.data(), newUnitRulesSize);
  vkUnmapMemory(device, stagingMemory);

  // Copy from staging to GPU buffers
  VkBufferCopy biomeCopy{};
  biomeCopy.srcOffset = 0;
  biomeCopy.dstOffset = 0;
  biomeCopy.size = newBiomeSize;
  vkCmdCopyBuffer(commandBuffer, stagingBuffer, biomeBuffer, 1, &biomeCopy);

  VkBufferCopy rulesCopy{};
  rulesCopy.srcOffset = newBiomeSize;
  rulesCopy.dstOffset = 0;
  rulesCopy.size = newUnitRulesSize;
  vkCmdCopyBuffer(commandBuffer, stagingBuffer, unitRulesBuffer, 1, &rulesCopy);

  gpuDirty = false;
  RayLog::LogInfo(RAYLOG_TAG, "GPU buffers updated: %u biomes, %u unit rules",
                 static_cast<uint32_t>(cpuBiomes.size()), static_cast<uint32_t>(cpuUnitRules.size()));

  return true;
}

bool BiomeRegistryGPU::CreateBuffer(VkDevice device, VkPhysicalDevice physicalDevice,
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

bool BiomeRegistryGPU::CopyBufferToGPU(VkDevice device, VkCommandBuffer commandBuffer,
                                      VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
  VkBufferCopy copyRegion{};
  copyRegion.srcOffset = 0;
  copyRegion.dstOffset = 0;
  copyRegion.size = size;
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
  return true;
}

uint32_t BiomeRegistryGPU::FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter,
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

} // namespace Rl::World::Biome
