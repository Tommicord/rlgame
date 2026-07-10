import Rl.World.Chunk.ChunkGeneratorGPU;

import Rl.World.Biome.BiomeRegistryGPU;
import Rl.World.Unit.UnitRegistryGPU;
import Rl.World.Chunk.UnitChunkBuffer;
import Rl.World.Chunk.UnitGPUSimplexNoise;
import Rl.World.Chunk.ChunkInRenderUnits;
import Rl.RayLog.Macro;
import Rl.RayLog.Logger;

import <cstring>;
import <stdexcept>;
import <algorithm>;
import <vulkan/vulkan.hpp>;
import <mutex>;
import <queue>;

namespace Rl::World::Chunk
{

ChunkGeneratorGPU::~ChunkGeneratorGPU()
{
  // Cleanup should be called explicitly via Shutdown()
}

bool ChunkGeneratorGPU::Initialize(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t seed)
{
  if (initialized)
  {
    RayLog::LogWarning(RAYLOG_TAG, "WorldGeneratorGPU already initialized");
    return true;
  }

  this->device = device;
  this->physicalDevice = physicalDevice;
  noiseGenerator.Create(device, physicalDevice, seed);
  if (!CreateComputePipelines(device, physicalDevice))
  {
    RayLog::LogError(RAYLOG_TAG, "Failed to create compute pipelines");
    return false;
  }
  if (!CreateDescriptorSets(device))
  {
    RayLog::LogError(RAYLOG_TAG, "Failed to create descriptor sets");
    return false;
  }

  // Initialize async queue with priority comparator
  asyncQueue = std::priority_queue<ChunkGenRequest, std::vector<ChunkGenRequest>,
                                   decltype(comparator)>(comparator);

  initialized = true;
  RayLog::LogInfo(RAYLOG_TAG, "WorldGeneratorGPU initialized successfully");
  return true;
}

void ChunkGeneratorGPU::Shutdown(VkDevice device)
{
  if (!initialized)
    return;

  vkDeviceWaitIdle(device);

  // Cleanup pipelines
  if (heightmapPipeline != VK_NULL_HANDLE)
    vkDestroyPipeline(device, heightmapPipeline, nullptr);
  if (biomePipeline != VK_NULL_HANDLE)
    vkDestroyPipeline(device, biomePipeline, nullptr);
  if (unitPlacePipeline != VK_NULL_HANDLE)
    vkDestroyPipeline(device, unitPlacePipeline, nullptr);
  if (polFencePipeline != VK_NULL_HANDLE)
    vkDestroyPipeline(device, polFencePipeline, nullptr);

  // Cleanup pipeline layouts
  if (heightmapLayout != VK_NULL_HANDLE)
    vkDestroyPipelineLayout(device, heightmapLayout, nullptr);
  if (biomeLayout != VK_NULL_HANDLE)
    vkDestroyPipelineLayout(device, biomeLayout, nullptr);
  if (unitPlaceLayout != VK_NULL_HANDLE)
    vkDestroyPipelineLayout(device, unitPlaceLayout, nullptr);
  if (polFenceLayout != VK_NULL_HANDLE)
    vkDestroyPipelineLayout(device, polFenceLayout, nullptr);

  // Cleanup descriptor pool
  if (descriptorPool != VK_NULL_HANDLE)
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);

  // Cleanup intermediate buffers
  if (heightmapBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device, heightmapBuffer, nullptr);
    vkFreeMemory(device, heightmapMemory, nullptr);
  }
  if (biomeBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device, biomeBuffer, nullptr);
    vkFreeMemory(device, biomeMemory, nullptr);
  }
  if (unitBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device, unitBuffer, nullptr);
    vkFreeMemory(device, unitMemory, nullptr);
  }
  if (polFenceBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device, polFenceBuffer, nullptr);
    vkFreeMemory(device, polFenceMemory, nullptr);
  }
  if (stagingBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);
  }
  if (nonCurableBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device, nonCurableBuffer, nullptr);
    vkFreeMemory(device, nonCurableMemory, nullptr);
  }

  // Shutdown noise generator
  noiseGenerator.Destroy(device);

  initialized = false;
  registriesSet = false;
  RayLog::LogInfo(RAYLOG_TAG, "WorldGeneratorGPU shutdown complete");
}

void ChunkGeneratorGPU::SetBiomeRegistry(Biome::BiomeRegistryGPU* biomeRegistry)
{
  this->biomeRegistry = biomeRegistry;
  registriesSet = true;
}

void ChunkGeneratorGPU::SetUnitRegistry(UnitRegistryGPU* unitRegistry)
{
  this->unitRegistry = unitRegistry;
  registriesSet = true;
}

void ChunkGeneratorGPU::SetNonCurableUnits(const std::vector<uint32_t>& nonCurableIds)
{
  nonCurableUnitIds = nonCurableIds;

  // Update GPU buffer if initialized
  if (initialized && device != VK_NULL_HANDLE)
  {
    const VkDeviceSize bufferSize = nonCurableIds.size() * sizeof(uint32_t) + sizeof(uint32_t); // + count
    if (nonCurableBuffer != VK_NULL_HANDLE)
    {
      vkDestroyBuffer(device, nonCurableBuffer, nullptr);
      vkFreeMemory(device, nonCurableMemory, nullptr);
    }

    CreateBuffer(device, physicalDevice, bufferSize,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                nonCurableBuffer, nonCurableMemory);

    // Upload data via staging
    VkBuffer staging = VK_NULL_HANDLE;
    VkDeviceMemory stagingMem = VK_NULL_HANDLE;
    CreateBuffer(device, physicalDevice, bufferSize,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                staging, stagingMem);

    void* data = nullptr;
    vkMapMemory(device, stagingMem, 0, bufferSize, 0, &data);
    uint32_t count = static_cast<uint32_t>(nonCurableIds.size());
    memcpy(data, &count, sizeof(uint32_t));
    memcpy(static_cast<uint8_t*>(data) + sizeof(uint32_t), nonCurableIds.data(), nonCurableIds.size() * sizeof(uint32_t));
    vkUnmapMemory(device, stagingMem);

    // Copy to device buffer
    VkCommandBuffer cmd = nullptr; // Need command buffer from caller
    // For now, this would need to be integrated with command buffer recording
    vkDestroyBuffer(device, staging, nullptr);
    vkFreeMemory(device, stagingMem, nullptr);
  }
}

bool ChunkGeneratorGPU::GenerateChunk(VkDevice device, VkCommandBuffer commandBuffer,
                                     const WorldChunkCoord& coord, UnitChunkBuffer& outChunk)
{
  if (!initialized || !registriesSet)
  {
    RayLog::LogError(RAYLOG_TAG, "Cannot generate chunk: not initialized or registries not set");
    return false;
  }

  // Set chunk dimensions
  chunkWidth = UnitChunkBuffer::W;
  chunkHeight = UnitChunkBuffer::H;
  chunkDepth = UnitChunkBuffer::D;

  // Create intermediate buffers for this chunk
  if (!CreateIntermediateBuffers(device, physicalDevice, chunkWidth, chunkHeight, chunkDepth))
  {
    RayLog::LogError(RAYLOG_TAG, "Failed to create intermediate buffers");
    return false;
  }

  // Update registry GPU buffers
  if (biomeRegistry)
    biomeRegistry->UpdateGPUBuffer(device, commandBuffer);
  if (unitRegistry)
    unitRegistry->UpdateGPUBuffer(device, commandBuffer);

  // Execute pipeline stages
  if (!ExecuteNoiseStage(device, commandBuffer, coord))
  {
    RayLog::LogError(RAYLOG_TAG, "Noise stage failed");
    return false;
  }

  if (!ExecuteHeightmapStage(device, commandBuffer))
  {
    RayLog::LogError(RAYLOG_TAG, "Heightmap stage failed");
    return false;
  }

  if (!ExecuteBiomeStage(device, commandBuffer))
  {
    RayLog::LogError(RAYLOG_TAG, "Biome stage failed");
    return false;
  }

  if (!ExecuteUnitPlaceStage(device, commandBuffer))
  {
    RayLog::LogError(RAYLOG_TAG, "Unit placement stage failed");
    return false;
  }

  if (!ExecutePolFenceStage(device, commandBuffer))
  {
    RayLog::LogError(RAYLOG_TAG, "Polygon fence stage failed");
    return false;
  }

  // Read back results to CPU
  if (!ReadbackUnitData(device, commandBuffer, outChunk))
  {
    RayLog::LogError(RAYLOG_TAG, "Readback failed");
    return false;
  }

  return true;
}

void ChunkGeneratorGPU::EnqueueChunkGeneration(const ChunkGenRequest& request)
{
  std::scoped_lock lock(queueMutex);
  asyncQueue.push(request);
}

void ChunkGeneratorGPU::ProcessAsyncQueue(VkDevice device, VkCommandBuffer commandBuffer)
{
  std::scoped_lock lock(queueMutex);

  while (!asyncQueue.empty())
  {
    ChunkGenRequest request = asyncQueue.top();
    asyncQueue.pop();

    UnitChunkBuffer chunkBuffer;
    if (GenerateChunk(device, commandBuffer, request.coord, chunkBuffer))
    {
      if (request.callback)
        request.callback(request.coord, chunkBuffer);
    }
  }
}

bool ChunkGeneratorGPU::HasPendingWork() const
{
  std::scoped_lock lock(queueMutex);
  return !asyncQueue.empty();
}

uint32_t ChunkGeneratorGPU::GetPendingCount() const
{
  std::scoped_lock lock(queueMutex);
  return static_cast<uint32_t>(asyncQueue.size());
}

bool ChunkGeneratorGPU::CreateComputePipelines(VkDevice device, VkPhysicalDevice physicalDevice)
{

  RayLog::LogInfo(RAYLOG_TAG, "Compute pipelines creation placeholder");
  return true;
}

bool ChunkGeneratorGPU::CreateDescriptorSets(VkDevice device)
{
  // Create descriptor pool
  VkDescriptorPoolSize poolSizes[] = {
    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 32 },
    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 8 }
  };

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = 2;
  poolInfo.pPoolSizes = poolSizes;
  poolInfo.maxSets = 4;

  if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
  {
    RayLog::LogError(RAYLOG_TAG, "Failed to create descriptor pool");
    return false;
  }

  // Create descriptor set layouts and sets for each pipeline
  // This is simplified - production would have proper binding configurations
  RayLog::LogInfo(RAYLOG_TAG, "Descriptor sets creation placeholder");
  return true;
}

bool ChunkGeneratorGPU::CreateIntermediateBuffers(VkDevice device, VkPhysicalDevice physicalDevice,
                                                   uint32_t width, uint32_t height, uint32_t depth)
{
  VkDeviceSize voxelCount = width * height * depth;

  // Heightmap buffer (float per voxel)
  heightmapBufferSize = voxelCount * sizeof(float);
  if (!CreateBuffer(device, physicalDevice, heightmapBufferSize,
                   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                   heightmapBuffer, heightmapMemory))
    return false;

  // Biome buffer (uint per voxel)
  biomeBufferSize = voxelCount * sizeof(uint32_t);
  if (!CreateBuffer(device, physicalDevice, biomeBufferSize,
                   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                   biomeBuffer, biomeMemory))
    return false;

  // Unit buffer (uint per voxel)
  unitBufferSize = voxelCount * sizeof(uint32_t);
  if (!CreateBuffer(device, physicalDevice, unitBufferSize,
                   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                   unitBuffer, unitMemory))
    return false;

  // Polygon fence buffer (4 floats per voxel)
  polFenceBufferSize = voxelCount * 4 * sizeof(float);
  if (!CreateBuffer(device, physicalDevice, polFenceBufferSize,
                   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                   polFenceBuffer, polFenceMemory))
    return false;

  // Staging buffer for readback (largest of all)
  VkDeviceSize stagingSize = unitBufferSize;
  if (!CreateBuffer(device, physicalDevice, stagingSize,
                   VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                   stagingBuffer, stagingMemory))
    return false;

  return true;
}

bool ChunkGeneratorGPU::ExecuteNoiseStage(VkDevice device, VkCommandBuffer commandBuffer,
                                          const WorldChunkCoord& coord)
{
  // Create noise buffers
  noiseGenerator.CreateNoiseBuffer(device, physicalDevice, chunkWidth, chunkHeight, chunkDepth);
  noiseGenerator.CreateWorldOutputBuffers(device, physicalDevice, chunkWidth, chunkHeight, chunkDepth);

  // Generate simplex noise
  SimplexNoisePushConstants noiseParams{};
  noiseParams.dimension = 3;
  noiseParams.scale = 0.05f;
  noiseParams.offsetX = static_cast<float>(coord.chunkX);
  noiseParams.offsetY = static_cast<float>(coord.chunkY);
  noiseParams.offsetZ = static_cast<float>(coord.chunkZ);
  noiseParams.width = chunkWidth;
  noiseParams.height = chunkHeight;
  noiseParams.depth = chunkDepth;
  noiseParams.octaves = 1;
  noiseParams.persistence = 0.5f;
  noiseParams.lacunarity = 2.0f;
  noiseParams.noiseType = 0;

  noiseGenerator.GenNoise(device, commandBuffer, noiseParams);

  // Generate temperature/moisture/elevation
  UnitGPUSimplexNoise::WorldNoisePushConstants worldParams{};
  worldParams.width = chunkWidth;
  worldParams.height = chunkHeight;
  worldParams.depth = chunkDepth;
  worldParams.globalScale = 1.0f;
  worldParams.tempBase = 0.5f;
  worldParams.tempVariation = 0.3f;
  worldParams.moistBase = 0.5f;
  worldParams.moistVariation = 0.3f;
  worldParams.elevBase = 0.5f;
  worldParams.elevVariation = 0.3f;

  noiseGenerator.GenWorldNoise(device, commandBuffer, worldParams);

  return true;
}

bool ChunkGeneratorGPU::ExecuteHeightmapStage(VkDevice device, VkCommandBuffer commandBuffer)
{
  // Dispatch world.heightmap.comp shader
  // This would bind the heightmap pipeline and dispatch
  // Placeholder for actual implementation
  return true;
}

bool ChunkGeneratorGPU::ExecuteBiomeStage(VkDevice device, VkCommandBuffer commandBuffer)
{
  // Dispatch world.biome.comp shader
  // This would bind the biome pipeline and dispatch
  // Placeholder for actual implementation
  return true;
}

bool ChunkGeneratorGPU::ExecuteUnitPlaceStage(VkDevice device, VkCommandBuffer commandBuffer)
{
  // Dispatch world.unitplace.comp shader
  // This would bind the unit placement pipeline and dispatch
  // Placeholder for actual implementation
  return true;
}

bool ChunkGeneratorGPU::ExecutePolFenceStage(VkDevice device, VkCommandBuffer commandBuffer)
{
  // Dispatch world.gen.pol.comp shader
  // This would bind the polygon fence pipeline and dispatch
  // Placeholder for actual implementation
  return true;
}

void ChunkGeneratorGPU::AddPipelineBarrier(VkCommandBuffer commandBuffer, VkBuffer buffer,
                                          VkAccessFlags srcAccess, VkAccessFlags dstAccess,
                                          VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage)
{
  VkBufferMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  barrier.srcAccessMask = srcAccess;
  barrier.dstAccessMask = dstAccess;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.buffer = buffer;
  barrier.offset = 0;
  barrier.size = VK_WHOLE_SIZE;

  vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, nullptr, 1, &barrier, 0, nullptr);
}

bool ChunkGeneratorGPU::ReadbackUnitData(VkDevice device, VkCommandBuffer commandBuffer,
                                         UnitChunkBuffer& outChunk)
{
  // Copy unit buffer to staging buffer
  VkBufferCopy copyRegion{};
  copyRegion.srcOffset = 0;
  copyRegion.dstOffset = 0;
  copyRegion.size = unitBufferSize;
  vkCmdCopyBuffer(commandBuffer, unitBuffer, stagingBuffer, 1, &copyRegion);

  // Add barrier before readback
  AddPipelineBarrier(commandBuffer, stagingBuffer,
                   VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT,
                   VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT);

  // Map and read staging buffer
  void* data = nullptr;
  if (vkMapMemory(device, stagingMemory, 0, unitBufferSize, 0, &data) != VK_SUCCESS)
  {
    RayLog::LogError(RAYLOG_TAG, "Failed to map staging memory for readback");
    return false;
  }

  // Copy to UnitChunkBuffer
  auto* unitIds = static_cast<uint32_t*>(data);
  for (uint32_t i = 0; i < chunkWidth * chunkHeight * chunkDepth; ++i)
  {
    outChunk.GetRaw()[i] = unitIds[i];
  }

  vkUnmapMemory(device, stagingMemory);
  outChunk.Clear();
  return true;
}

bool ChunkGeneratorGPU::CreateBuffer(VkDevice device, VkPhysicalDevice physicalDevice,
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

uint32_t ChunkGeneratorGPU::FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter,
                                          VkMemoryPropertyFlags properties) const
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

} // namespace Rl::World::Chunk
