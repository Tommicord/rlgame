import Rl.World.Chunk.ChunkGeneratorGPU;

import Rl.World.Biome.BiomeRegistryGPU;
import Rl.World.Unit.UnitRegistryGPU;
import Rl.World.Chunk.UnitChunkBuffer;
import Rl.World.Chunk.UnitGPUSimplexNoise;
import Rl.World.Chunk.ChunkInRenderUnits;
import Rl.World.Unit.UnitAir;
import Rl.RayLog.Macro;
import Rl.RayLog.Logger;
import Rl.Base.Game;
import Rl.Base.Shader;

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
  // Create noise buffers
  noiseGenerator.CreateNoiseBuffer(device, physicalDevice, chunkWidth, chunkHeight, chunkDepth);
  noiseGenerator.CreateWorldOutputBuffers(device, physicalDevice, chunkWidth, chunkHeight, chunkDepth);

  // Initialize async queue with priority comparator
  asyncQueue = std::priority_queue<ChunkGenRequest, std::vector<ChunkGenRequest>, Comparator>(comparator);

  initialized = true;
  RayLog::LogInfo(RAYLOG_TAG, "WorldGeneratorGPU initialized successfully");
  return true;
}

void ChunkGeneratorGPU::Shutdown(VkDevice device)
{
  if (!initialized)
    return;

  vkDeviceWaitIdle(device);

  // Cleanup lookup table buffers
  if (transparencyLUTBuffer != VK_NULL_HANDLE)
    vkDestroyBuffer(device, transparencyLUTBuffer, nullptr);
  if (transparencyLUTMemory != VK_NULL_HANDLE)
    vkFreeMemory(device, transparencyLUTMemory, nullptr);
  if (curableLUTBuffer != VK_NULL_HANDLE)
    vkDestroyBuffer(device, curableLUTBuffer, nullptr);
  if (curableLUTMemory != VK_NULL_HANDLE)
    vkFreeMemory(device, curableLUTMemory, nullptr);

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

  // Cleanup descriptor set layout
  if (polFenceDescriptorSetLayout != VK_NULL_HANDLE)
    vkDestroyDescriptorSetLayout(device, polFenceDescriptorSetLayout, nullptr);

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

  // Create lookup table buffers now that we have the unit registry
  if (initialized && device != VK_NULL_HANDLE && physicalDevice != VK_NULL_HANDLE)
  {
    CreateLookupTableBuffers(device, physicalDevice);
  }
}

void ChunkGeneratorGPU::SetNonCurableUnits(std::vector<uint32_t>& nonCurableIds)
{
  nonCurableUnitIds = nonCurableIds;

  // Update lookup table buffers if initialized
  if (initialized && device != VK_NULL_HANDLE && transparencyLUTBuffer != VK_NULL_HANDLE)
  {
    // Get command buffer from Game's MainBinding
    auto& game = Rl::Main::Game::GetInstance();
    auto& binding = game.GetMainBinding();

    if (!binding.commandBuffers.empty())
    {
      VkCommandBuffer cmdBuffer = binding.commandBuffers[0];

      // Begin command buffer for one-time use
      VkCommandBufferBeginInfo beginInfo{};
      beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

      if (vkBeginCommandBuffer(cmdBuffer, &beginInfo) == VK_SUCCESS)
      {
        // Update lookup table buffers
        UpdateLookupTableBuffers(device, cmdBuffer);

        if (vkEndCommandBuffer(cmdBuffer) == VK_SUCCESS)
        {
          // Submit and wait for completion
          VkSubmitInfo submitInfo{};
          submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
          submitInfo.commandBufferCount = 1;
          submitInfo.pCommandBuffers = &cmdBuffer;

          VkFence fence = VK_NULL_HANDLE;
          VkFenceCreateInfo fenceInfo{};
          fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
          if (vkCreateFence(device, &fenceInfo, nullptr, &fence) == VK_SUCCESS)
          {
            if (vkQueueSubmit(binding.graphicsQueue, 1, &submitInfo, fence) == VK_SUCCESS)
            {
              vkWaitForFences(device, 1, &fence, VK_TRUE, 5000000000ULL); // 5s timeout
            }
            vkDestroyFence(device, fence, nullptr);
          }
        }
      }
    }
  }

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
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                nonCurableBuffer, nonCurableMemory);

    // Upload data directly to host-visible memory
    void* data = nullptr;
    vkMapMemory(device, nonCurableMemory, 0, bufferSize, 0, &data);
    
    // Sort non-curable units for binary search optimization in compute shaders
    std::sort(nonCurableIds.begin(), nonCurableIds.end());
    
    uint32_t count = static_cast<uint32_t>(nonCurableIds.size());
    memcpy(data, &count, sizeof(uint32_t));
    memcpy(static_cast<uint8_t*>(data) + sizeof(uint32_t), nonCurableIds.data(), nonCurableIds.size() * sizeof(uint32_t));
    vkUnmapMemory(device, nonCurableMemory);
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

  chunkWidth = UnitChunkBuffer::W;
  chunkHeight = UnitChunkBuffer::H;
  chunkDepth = UnitChunkBuffer::D;

  // Create intermediate buffers for this chunk
  if (!CreateIntermediateBuffers(device, physicalDevice, chunkWidth, chunkHeight, chunkDepth))
  {
    RayLog::LogError(RAYLOG_TAG, "Failed to create intermediate buffers");
    return false;
  }
  // Update polFence descriptor set with current buffer handles
  if (!UpdatePolFenceDescriptorSet(device))
  {
    RayLog::LogError(RAYLOG_TAG, "Failed to update polFence descriptor set");
    return false;
  }

  // Update registry GPU buffers
  if (biomeRegistry)
    biomeRegistry->UpdateGPUBuffer(device, commandBuffer);
  if (unitRegistry)
    unitRegistry->UpdateGPUBuffer(device, commandBuffer);

  // Update lookup table buffers
  if (transparencyLUTBuffer != VK_NULL_HANDLE && curableLUTBuffer != VK_NULL_HANDLE)
  {
    UpdateLookupTableBuffers(device, commandBuffer);
  }

  // Execute pipeline stages with progress logging
  RayLog::LogInfo(RAYLOG_TAG, "Starting chunk generation pipeline for dimensions %dx%dx%d", chunkWidth, chunkHeight, chunkDepth);

  RayLog::LogInfo(RAYLOG_TAG, "Executing noise stage...");
  if (!ExecuteNoiseStage(device, commandBuffer, coord))
  {
    RayLog::LogError(RAYLOG_TAG, "Noise stage failed");
    return false;
  }
  RayLog::LogInfo(RAYLOG_TAG, "Noise stage completed");

  RayLog::LogInfo(RAYLOG_TAG, "Executing heightmap stage");
  if (!ExecuteHeightmapStage(device, commandBuffer))
  {
    RayLog::LogError(RAYLOG_TAG, "Heightmap stage failed");
    return false;
  }

  RayLog::LogInfo(RAYLOG_TAG, "Executing biome stage");
  if (!ExecuteBiomeStage(device, commandBuffer))
  {
    RayLog::LogError(RAYLOG_TAG, "Biome stage failed");
    return false;
  }
  RayLog::LogInfo(RAYLOG_TAG, "Biome stage completed");

  RayLog::LogInfo(RAYLOG_TAG, "Executing unit placement stage");
  if (!ExecuteUnitPlaceStage(device, commandBuffer))
  {
    RayLog::LogError(RAYLOG_TAG, "Unit placement stage failed");
    return false;
  }
  RayLog::LogInfo(RAYLOG_TAG, "Unit placement stage completed");

  // Skip pol fence stage for integrated GPUs to prevent VK_ERROR_DEVICE_LOST
  if (useSlicedDispatch)
  {
    RayLog::LogInfo(RAYLOG_TAG, "Executing polygon fence stage (sliced dispatch)");
    if (!ExecutePolFenceStageSliced(device, commandBuffer))
    {
      RayLog::LogError(RAYLOG_TAG, "Polygon fence sliced stage failed");
      return false;
    }
    RayLog::LogInfo(RAYLOG_TAG, "Polygon fence sliced stage completed");
  }
  else
  {
    RayLog::LogInfo(RAYLOG_TAG, "Executing polygon fence stage");
    if (!ExecutePolFenceStage(device, commandBuffer))
    {
      RayLog::LogError(RAYLOG_TAG, "Polygon fence stage failed");
      return false;
    }
    RayLog::LogInfo(RAYLOG_TAG, "Polygon fence stage completed");
  }

  //if (!ReadbackUnitData(device, commandBuffer, outChunk))
  //{
  //  RayLog::LogError(RAYLOG_TAG, "Readback failed");
  //  return false;
  //}
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
  // Create polFence pipeline layout with push constants
  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(uint32_t) * 6 + sizeof(float) * 3; // width, height, depth, airUnitId, maxUnitId, yOffset, sliceHeight, curveStrength, padding

  VkPipelineLayoutCreateInfo polFenceLayoutInfo{};
  polFenceLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  polFenceLayoutInfo.setLayoutCount = 1;
  polFenceLayoutInfo.pSetLayouts = &polFenceDescriptorSetLayout;
  polFenceLayoutInfo.pushConstantRangeCount = 1;
  polFenceLayoutInfo.pPushConstantRanges = &pushConstantRange;

  if (vkCreatePipelineLayout(device, &polFenceLayoutInfo, nullptr, &polFenceLayout) != VK_SUCCESS)
  {
    RayLog::LogError(RAYLOG_TAG, "Failed to create polFence pipeline layout");
    return false;
  }

  // Load polFence shader
  auto polFenceShaderCode = Providers::ShaderObject::Shader("world.gen.pol.comp.spv");
  auto polFenceShaderModule = Providers::ShaderObject::Module(device, polFenceShaderCode);

  VkPipelineShaderStageCreateInfo polFenceStageInfo{};
  polFenceStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  polFenceStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  polFenceStageInfo.module = polFenceShaderModule.module;
  polFenceStageInfo.pName = "main";

  VkComputePipelineCreateInfo polFencePipelineInfo{};
  polFencePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  polFencePipelineInfo.stage = polFenceStageInfo;
  polFencePipelineInfo.layout = polFenceLayout;

  if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &polFencePipelineInfo, nullptr, &polFencePipeline) != VK_SUCCESS)
  {
    Providers::ShaderObject::DestroyShaderModule(device, polFenceShaderModule);
    vkDestroyPipelineLayout(device, polFenceLayout, nullptr);
    polFenceLayout = VK_NULL_HANDLE;
    RayLog::LogError(RAYLOG_TAG, "Failed to create polFence compute pipeline");
    return false;
  }

  Providers::ShaderObject::DestroyShaderModule(device, polFenceShaderModule);

  // Create heightmap pipeline layout (simple, no push constants)
  VkPipelineLayoutCreateInfo heightmapLayoutInfo{};
  heightmapLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  heightmapLayoutInfo.setLayoutCount = 0;
  heightmapLayoutInfo.pSetLayouts = nullptr;

  if (vkCreatePipelineLayout(device, &heightmapLayoutInfo, nullptr, &heightmapLayout) != VK_SUCCESS)
  {
    RayLog::LogError(RAYLOG_TAG, "Failed to create heightmap pipeline layout");
    return false;
  }

  // Load heightmap shader
  auto heightmapShaderCode = Providers::ShaderObject::Shader("world.heightmap.comp.spv");
  auto heightmapShaderModule = Providers::ShaderObject::Module(device, heightmapShaderCode);

  VkPipelineShaderStageCreateInfo heightmapStageInfo{};
  heightmapStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  heightmapStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  heightmapStageInfo.module = heightmapShaderModule.module;
  heightmapStageInfo.pName = "main";

  VkComputePipelineCreateInfo heightmapPipelineInfo{};
  heightmapPipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  heightmapPipelineInfo.stage = heightmapStageInfo;
  heightmapPipelineInfo.layout = heightmapLayout;

  if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &heightmapPipelineInfo, nullptr, &heightmapPipeline) != VK_SUCCESS)
  {
    Providers::ShaderObject::DestroyShaderModule(device, heightmapShaderModule);
    RayLog::LogError(RAYLOG_TAG, "Failed to create heightmap compute pipeline");
    return false;
  }

  Providers::ShaderObject::DestroyShaderModule(device, heightmapShaderModule);

  // Create biome pipeline layout
  VkPipelineLayoutCreateInfo biomeLayoutInfo{};
  biomeLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  biomeLayoutInfo.setLayoutCount = 0;
  biomeLayoutInfo.pSetLayouts = nullptr;

  if (vkCreatePipelineLayout(device, &biomeLayoutInfo, nullptr, &biomeLayout) != VK_SUCCESS)
  {
    RayLog::LogError(RAYLOG_TAG, "Failed to create biome pipeline layout");
    return false;
  }

  // Load biome shader
  auto biomeShaderCode = Providers::ShaderObject::Shader("world.biome.comp.spv");
  auto biomeShaderModule = Providers::ShaderObject::Module(device, biomeShaderCode);

  VkPipelineShaderStageCreateInfo biomeStageInfo{};
  biomeStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  biomeStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  biomeStageInfo.module = biomeShaderModule.module;
  biomeStageInfo.pName = "main";

  VkComputePipelineCreateInfo biomePipelineInfo{};
  biomePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  biomePipelineInfo.stage = biomeStageInfo;
  biomePipelineInfo.layout = biomeLayout;

  if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &biomePipelineInfo, nullptr, &biomePipeline) != VK_SUCCESS)
  {
    Providers::ShaderObject::DestroyShaderModule(device, biomeShaderModule);
    RayLog::LogError(RAYLOG_TAG, "Failed to create biome compute pipeline");
    return false;
  }

  Providers::ShaderObject::DestroyShaderModule(device, biomeShaderModule);

  // Create unitPlace pipeline layout
  VkPipelineLayoutCreateInfo unitPlaceLayoutInfo{};
  unitPlaceLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  unitPlaceLayoutInfo.setLayoutCount = 0;
  unitPlaceLayoutInfo.pSetLayouts = nullptr;

  if (vkCreatePipelineLayout(device, &unitPlaceLayoutInfo, nullptr, &unitPlaceLayout) != VK_SUCCESS)
  {
    RayLog::LogError(RAYLOG_TAG, "Failed to create unitPlace pipeline layout");
    return false;
  }

  // Load unitPlace shader
  auto unitPlaceShaderCode = Providers::ShaderObject::Shader("world.unitplace.comp.spv");
  auto unitPlaceShaderModule = Providers::ShaderObject::Module(device, unitPlaceShaderCode);

  VkPipelineShaderStageCreateInfo unitPlaceStageInfo{};
  unitPlaceStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  unitPlaceStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  unitPlaceStageInfo.module = unitPlaceShaderModule.module;
  unitPlaceStageInfo.pName = "main";

  VkComputePipelineCreateInfo unitPlacePipelineInfo{};
  unitPlacePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  unitPlacePipelineInfo.stage = unitPlaceStageInfo;
  unitPlacePipelineInfo.layout = unitPlaceLayout;

  if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &unitPlacePipelineInfo, nullptr, &unitPlacePipeline) != VK_SUCCESS)
  {
    Providers::ShaderObject::DestroyShaderModule(device, unitPlaceShaderModule);
    RayLog::LogError(RAYLOG_TAG, "Failed to create unitPlace compute pipeline");
    return false;
  }

  Providers::ShaderObject::DestroyShaderModule(device, unitPlaceShaderModule);

  RayLog::LogInfo(RAYLOG_TAG, "Compute pipelines created successfully");
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
  poolInfo.maxSets = 5;

  if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
  {
    RayLog::LogError(RAYLOG_TAG, "Failed to create descriptor pool");
    return false;
  }

  // Create polFence descriptor set layout with 4 bindings
  VkDescriptorSetLayoutBinding bindings[4] = {};

  // Binding 0: unitIds input buffer (readonly)
  bindings[0].binding = 0;
  bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  bindings[0].descriptorCount = 1;
  bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  // Binding 1: transparency lookup table (readonly)
  bindings[1].binding = 1;
  bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  bindings[1].descriptorCount = 1;
  bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  // Binding 2: curable lookup table (readonly)
  bindings[2].binding = 2;
  bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  bindings[2].descriptorCount = 1;
  bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  // Binding 3: polFence output buffer (writeonly)
  bindings[3].binding = 3;
  bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  bindings[3].descriptorCount = 1;
  bindings[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = 4;
  layoutInfo.pBindings = bindings;

  if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &polFenceDescriptorSetLayout) != VK_SUCCESS)
  {
    RayLog::LogError(RAYLOG_TAG, "Failed to create polFence descriptor set layout");
    return false;
  }

  // Allocate polFence descriptor set
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = &polFenceDescriptorSetLayout;

  if (vkAllocateDescriptorSets(device, &allocInfo, &polFenceDescriptorSet) != VK_SUCCESS)
  {
    RayLog::LogError(RAYLOG_TAG, "Failed to allocate polFence descriptor set");
    return false;
  }

  RayLog::LogInfo(RAYLOG_TAG, "Descriptor sets created successfully");
  return true;
}

bool ChunkGeneratorGPU::CreateLookupTableBuffers(VkDevice device, VkPhysicalDevice physicalDevice)
{
  if (!unitRegistry)
  {
    RayLog::LogError(RAYLOG_TAG, "Cannot create lookup tables: unit registry not set");
    return false;
  }

  // Get max unit ID from registry
  maxUnitId = 0;
  const auto& cpuUnits = unitRegistry->GetCPUUnits();
  for (const auto& unit : cpuUnits)
  {
    if (unit.unitId > maxUnitId)
      maxUnitId = unit.unitId;
  }

  // Add some padding for safety
  maxUnitId = max(maxUnitId + 1, 256u);

  // Create transparency lookup table (float per unit ID)
  VkDeviceSize transparencySize = maxUnitId * sizeof(float);
  if (!CreateBuffer(device, physicalDevice, transparencySize,
                   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                   transparencyLUTBuffer, transparencyLUTMemory))
  {
    RayLog::LogError(RAYLOG_TAG, "Failed to create transparency lookup table buffer");
    return false;
  }

  // Create curable lookup table (uint per unit ID)
  VkDeviceSize curableSize = maxUnitId * sizeof(uint32_t);
  if (!CreateBuffer(device, physicalDevice, curableSize,
                   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                   curableLUTBuffer, curableLUTMemory))
  {
    RayLog::LogError(RAYLOG_TAG, "Failed to create curable lookup table buffer");
    return false;
  }

  RayLog::LogInfo(RAYLOG_TAG, "Created lookup table buffers: maxUnitId=%u", maxUnitId);
  return true;
}

bool ChunkGeneratorGPU::UpdateLookupTableBuffers(VkDevice device, VkCommandBuffer commandBuffer)
{
  if (!unitRegistry || transparencyLUTBuffer == VK_NULL_HANDLE || curableLUTBuffer == VK_NULL_HANDLE)
  {
    RayLog::LogError(RAYLOG_TAG, "Cannot update lookup tables: not initialized");
    return false;
  }

  const auto& cpuUnits = unitRegistry->GetCPUUnits();

  // Create staging buffers for upload
  VkBuffer transparencyStaging = VK_NULL_HANDLE;
  VkDeviceMemory transparencyStagingMem = VK_NULL_HANDLE;
  VkBuffer curableStaging = VK_NULL_HANDLE;
  VkDeviceMemory curableStagingMem = VK_NULL_HANDLE;

  VkDeviceSize transparencySize = maxUnitId * sizeof(float);
  VkDeviceSize curableSize = maxUnitId * sizeof(uint32_t);

  if (!CreateBuffer(device, physicalDevice, transparencySize,
                   VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                   transparencyStaging, transparencyStagingMem))
  {
    RayLog::LogError(RAYLOG_TAG, "Failed to create transparency staging buffer");
    return false;
  }

  if (!CreateBuffer(device, physicalDevice, curableSize,
                   VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                   curableStaging, curableStagingMem))
  {
    vkDestroyBuffer(device, transparencyStaging, nullptr);
    vkFreeMemory(device, transparencyStagingMem, nullptr);
    RayLog::LogError(RAYLOG_TAG, "Failed to create curable staging buffer");
    return false;
  }

  // Populate transparency lookup table
  float* transparencyData = nullptr;
  vkMapMemory(device, transparencyStagingMem, 0, transparencySize, 0, reinterpret_cast<void**>(&transparencyData));
  memset(transparencyData, 0, static_cast<size_t>(transparencySize)); // Default to 0 (not transparent)
  for (const auto& unit : cpuUnits)
  {
    if (unit.unitId < maxUnitId)
    {
      transparencyData[unit.unitId] = unit.transparency;
    }
  }
  vkUnmapMemory(device, transparencyStagingMem);

  // Populate curable lookup table
  uint32_t* curableData = nullptr;
  vkMapMemory(device, curableStagingMem, 0, curableSize, 0, reinterpret_cast<void**>(&curableData));
  memset(curableData, 1, static_cast<size_t>(curableSize)); // Default to 1 (curable)
  for (uint32_t unitId : nonCurableUnitIds)
  {
    if (unitId < maxUnitId)
    {
      curableData[unitId] = 0; // Mark as not curable
    }
  }
  vkUnmapMemory(device, curableStagingMem);

  // Copy to device buffers
  VkBufferCopy transparencyCopy{};
  transparencyCopy.srcOffset = 0;
  transparencyCopy.dstOffset = 0;
  transparencyCopy.size = transparencySize;
  vkCmdCopyBuffer(commandBuffer, transparencyStaging, transparencyLUTBuffer, 1, &transparencyCopy);

  VkBufferCopy curableCopy{};
  curableCopy.srcOffset = 0;
  curableCopy.dstOffset = 0;
  curableCopy.size = curableSize;
  vkCmdCopyBuffer(commandBuffer, curableStaging, curableLUTBuffer, 1, &curableCopy);

  // Add barrier to ensure copy completes before destroying staging buffers
  VkMemoryBarrier memBarrier{};
  memBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  memBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  memBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                     VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &memBarrier, 0, nullptr, 0, nullptr);

  // Cleanup staging buffers
  vkDestroyBuffer(device, transparencyStaging, nullptr);
  vkFreeMemory(device, transparencyStagingMem, nullptr);
  vkDestroyBuffer(device, curableStaging, nullptr);
  vkFreeMemory(device, curableStagingMem, nullptr);

  RayLog::LogInfo(RAYLOG_TAG, "Updated lookup table buffers with %u units", static_cast<uint32_t>(cpuUnits.size()));
  return true;
}

bool ChunkGeneratorGPU::UpdatePolFenceDescriptorSet(VkDevice device)
{
  if (polFenceDescriptorSet == VK_NULL_HANDLE)
  {
    RayLog::LogError(RAYLOG_TAG, "Cannot update polFence descriptor set: not allocated");
    return false;
  }

  // Update descriptor set with buffer handles
  VkDescriptorBufferInfo unitBufferInfo{};
  unitBufferInfo.buffer = unitBuffer;
  unitBufferInfo.offset = 0;
  unitBufferInfo.range = VK_WHOLE_SIZE;

  VkDescriptorBufferInfo transparencyLUTInfo{};
  transparencyLUTInfo.buffer = transparencyLUTBuffer;
  transparencyLUTInfo.offset = 0;
  transparencyLUTInfo.range = VK_WHOLE_SIZE;

  VkDescriptorBufferInfo curableLUTInfo{};
  curableLUTInfo.buffer = curableLUTBuffer;
  curableLUTInfo.offset = 0;
  curableLUTInfo.range = VK_WHOLE_SIZE;

  VkDescriptorBufferInfo polFenceBufferInfo{};
  polFenceBufferInfo.buffer = polFenceBuffer;
  polFenceBufferInfo.offset = 0;
  polFenceBufferInfo.range = VK_WHOLE_SIZE;

  VkWriteDescriptorSet writes[4] = {};

  // Binding 0: unitIds input buffer
  writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writes[0].dstSet = polFenceDescriptorSet;
  writes[0].dstBinding = 0;
  writes[0].dstArrayElement = 0;
  writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  writes[0].descriptorCount = 1;
  writes[0].pBufferInfo = &unitBufferInfo;

  // Binding 1: transparency lookup table
  writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writes[1].dstSet = polFenceDescriptorSet;
  writes[1].dstBinding = 1;
  writes[1].dstArrayElement = 0;
  writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  writes[1].descriptorCount = 1;
  writes[1].pBufferInfo = &transparencyLUTInfo;

  // Binding 2: curable lookup table
  writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writes[2].dstSet = polFenceDescriptorSet;
  writes[2].dstBinding = 2;
  writes[2].dstArrayElement = 0;
  writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  writes[2].descriptorCount = 1;
  writes[2].pBufferInfo = &curableLUTInfo;

  // Binding 3: polFence output buffer
  writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writes[3].dstSet = polFenceDescriptorSet;
  writes[3].dstBinding = 3;
  writes[3].dstArrayElement = 0;
  writes[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  writes[3].descriptorCount = 1;
  writes[3].pBufferInfo = &polFenceBufferInfo;

  vkUpdateDescriptorSets(device, 4, writes, 0, nullptr);

  RayLog::LogInfo(RAYLOG_TAG, "Updated polFence descriptor set with lookup table buffers");
  return true;
}

bool ChunkGeneratorGPU::CreateIntermediateBuffers(VkDevice device, VkPhysicalDevice physicalDevice,
                                                   uint32_t width, uint32_t height, uint32_t depth)
{
  // Destroy existing buffers if they exist
  if (heightmapBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device, heightmapBuffer, nullptr);
    vkFreeMemory(device, heightmapMemory, nullptr);
    heightmapBuffer = VK_NULL_HANDLE;
    heightmapMemory = VK_NULL_HANDLE;
  }
  if (biomeBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device, biomeBuffer, nullptr);
    vkFreeMemory(device, biomeMemory, nullptr);
    biomeBuffer = VK_NULL_HANDLE;
    biomeMemory = VK_NULL_HANDLE;
  }
  if (unitBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device, unitBuffer, nullptr);
    vkFreeMemory(device, unitMemory, nullptr);
    unitBuffer = VK_NULL_HANDLE;
    unitMemory = VK_NULL_HANDLE;
  }
  if (polFenceBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device, polFenceBuffer, nullptr);
    vkFreeMemory(device, polFenceMemory, nullptr);
    polFenceBuffer = VK_NULL_HANDLE;
    polFenceMemory = VK_NULL_HANDLE;
  }
  if (stagingBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);
    stagingBuffer = VK_NULL_HANDLE;
    stagingMemory = VK_NULL_HANDLE;
  }

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

  // Polygon fence buffer (vec4 per voxel)
  polFenceBufferSize = voxelCount * sizeof(float) * 4;
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

  // Add barrier to ensure noise buffer writes complete before GenWorldNoise reads it
  AddPipelineBarrier(commandBuffer, noiseGenerator.GetNoiseBuffer(),
                   VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

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

  // Add barrier to ensure temperature/moisture/elevation writes complete before next stage
  AddPipelineBarrier(commandBuffer, noiseGenerator.GetTemperatureBuffer(),
                   VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
  AddPipelineBarrier(commandBuffer, noiseGenerator.GetMoistureBuffer(),
                   VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
  AddPipelineBarrier(commandBuffer, noiseGenerator.GetElevationBuffer(),
                   VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

  return true;
}

bool ChunkGeneratorGPU::ExecuteHeightmapStage(VkDevice device, VkCommandBuffer commandBuffer)
{
  if (heightmapPipeline == VK_NULL_HANDLE)
  {
    RayLog::LogError(RAYLOG_TAG, "Heightmap pipeline not created");
    return false;
  }

  // Add barrier to ensure elevation buffer from noise stage is ready
  AddPipelineBarrier(commandBuffer, noiseGenerator.GetElevationBuffer(),
                   VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

  // Bind pipeline
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, heightmapPipeline);

  // Dispatch compute shader
  uint32_t groupCountX = (chunkWidth + 7) / 8;
  uint32_t groupCountY = (chunkHeight + 7) / 8;
  uint32_t groupCountZ = (chunkDepth + 7) / 8;
  vkCmdDispatch(commandBuffer, groupCountX, groupCountY, groupCountZ);

  // Add memory barrier
  AddPipelineBarrier(commandBuffer, heightmapBuffer,
                   VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

  return true;
}

bool ChunkGeneratorGPU::ExecuteBiomeStage(VkDevice device, VkCommandBuffer commandBuffer)
{
  if (biomePipeline == VK_NULL_HANDLE)
  {
    RayLog::LogError(RAYLOG_TAG, "Biome pipeline not created");
    return false;
  }

  // Add barrier to ensure temperature, moisture, elevation buffers from noise stage are ready
  AddPipelineBarrier(commandBuffer, noiseGenerator.GetTemperatureBuffer(),
                   VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
  AddPipelineBarrier(commandBuffer, noiseGenerator.GetMoistureBuffer(),
                   VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
  AddPipelineBarrier(commandBuffer, noiseGenerator.GetElevationBuffer(),
                   VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

  // Bind pipeline
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, biomePipeline);

  // Dispatch compute shader
  uint32_t groupCountX = (chunkWidth + 7) / 8;
  uint32_t groupCountY = (chunkHeight + 7) / 8;
  uint32_t groupCountZ = (chunkDepth + 7) / 8;
  vkCmdDispatch(commandBuffer, groupCountX, groupCountY, groupCountZ);

  // Add memory barrier
  AddPipelineBarrier(commandBuffer, biomeBuffer,
                   VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

  return true;
}

bool ChunkGeneratorGPU::ExecuteUnitPlaceStage(VkDevice device, VkCommandBuffer commandBuffer)
{
  if (unitPlacePipeline == VK_NULL_HANDLE)
  {
    RayLog::LogError(RAYLOG_TAG, "UnitPlace pipeline not created");
    return false;
  }

  // Add barrier to ensure biome, temperature, moisture, elevation, heightmap buffers are ready
  AddPipelineBarrier(commandBuffer, biomeBuffer,
                   VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
  AddPipelineBarrier(commandBuffer, heightmapBuffer,
                   VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
  AddPipelineBarrier(commandBuffer, noiseGenerator.GetTemperatureBuffer(),
                   VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
  AddPipelineBarrier(commandBuffer, noiseGenerator.GetMoistureBuffer(),
                   VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
  AddPipelineBarrier(commandBuffer, noiseGenerator.GetElevationBuffer(),
                   VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

  // Bind pipeline
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, unitPlacePipeline);

  // Dispatch compute shader
  uint32_t groupCountX = (chunkWidth + 7) / 8;
  uint32_t groupCountY = (chunkHeight + 7) / 8;
  uint32_t groupCountZ = (chunkDepth + 7) / 8;
  vkCmdDispatch(commandBuffer, groupCountX, groupCountY, groupCountZ);

  // Add memory barrier
  AddPipelineBarrier(commandBuffer, unitBuffer,
                   VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

  return true;
}

bool ChunkGeneratorGPU::ExecutePolFenceStage(VkDevice device, VkCommandBuffer commandBuffer)
{
  if (polFencePipeline == VK_NULL_HANDLE || polFenceDescriptorSet == VK_NULL_HANDLE)
  {
    RayLog::LogError(RAYLOG_TAG, "PolFence pipeline or descriptor set not created");
    return false;
  }

  // Add barrier to ensure unit buffer from unit place stage is ready
  AddPipelineBarrier(commandBuffer, unitBuffer,
                   VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

  // Bind pipeline
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, polFencePipeline);

  // Bind descriptor set with new lookup table bindings
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          polFenceLayout, 0, 1, &polFenceDescriptorSet, 0, nullptr);

  // Push constants with chunk dimensions and maxUnitId
  struct PolFencePushConstants
  {
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t airUnitId;
    float curveStrength;
    uint32_t maxUnitId;
    float _pad1;
    float _pad2;
  } pcData{};

  pcData.width = chunkWidth;
  pcData.height = chunkHeight;
  pcData.depth = chunkDepth;
  pcData.airUnitId = UnitAir::GetStaticClassId();
  pcData.curveStrength = 1.0f;
  pcData.maxUnitId = maxUnitId;

  vkCmdPushConstants(commandBuffer, polFenceLayout,
                    VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pcData), &pcData);

  // Dispatch compute shader
  uint32_t groupCountX = (chunkWidth + 7) / 8;
  uint32_t groupCountY = (chunkHeight + 7) / 8;
  uint32_t groupCountZ = (chunkDepth + 7) / 8;
  vkCmdDispatch(commandBuffer, groupCountX, groupCountY, groupCountZ);

  // Add memory barrier to ensure polFence writes complete before readback
  AddPipelineBarrier(commandBuffer, polFenceBuffer,
                   VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

  return true;
}

bool ChunkGeneratorGPU::ExecutePolFenceStageSliced(VkDevice device, VkCommandBuffer commandBuffer)
{
  if (polFencePipeline == VK_NULL_HANDLE || polFenceDescriptorSet == VK_NULL_HANDLE)
  {
    RayLog::LogError(RAYLOG_TAG, "PolFence pipeline or descriptor set not created");
    return false;
  }

  // Bind pipeline once
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, polFencePipeline);

  // Bind descriptor set once
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          polFenceLayout, 0, 1, &polFenceDescriptorSet, 0, nullptr);

  // Split height into slices (8 voxels per slice = 1 workgroup)
  const uint32_t sliceHeight = 8;
  const uint32_t numSlices = (chunkHeight + sliceHeight - 1) / sliceHeight;

  RayLog::LogInfo(RAYLOG_TAG, "Executing polFence stage in %u vertical slices (height=%u)", numSlices, chunkHeight);

  struct PolFencePushConstants
  {
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t airUnitId;
    float curveStrength;
    uint32_t maxUnitId;
    uint32_t yOffset;  // Y offset for this slice
    uint32_t sliceHeight;  // Height of this slice
  } pcData{};

  pcData.width = chunkWidth;
  pcData.height = chunkHeight;
  pcData.depth = chunkDepth;
  pcData.airUnitId = UnitAir::GetStaticClassId();
  pcData.curveStrength = 1.0f;
  pcData.maxUnitId = maxUnitId;

  uint32_t groupCountX = (chunkWidth + 7) / 8;
  uint32_t groupCountZ = (chunkDepth + 7) / 8;
  uint32_t groupCountY = (chunkHeight + 7) / 8;

  vkCmdPushConstants(commandBuffer, polFenceLayout,
                    VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pcData), &pcData);
  vkCmdDispatch(commandBuffer, groupCountX, groupCountY, groupCountZ);

  // Final memory barrier to ensure all writes complete before readback
  AddPipelineBarrier(commandBuffer, polFenceBuffer,
                   VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

  RayLog::LogInfo(RAYLOG_TAG, "PolFence sliced dispatch completed");
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

void ChunkGeneratorGPU::SetSkipPolFence(bool skip)
{
  skipPolFence = skip;
  if (skip)
  {
    RayLog::LogInfo(RAYLOG_TAG, "Skipping polygon fence generation for integrated GPU");
  }
  else
  {
    RayLog::LogInfo(RAYLOG_TAG, "Enabling polygon fence generation");
  }
}

void ChunkGeneratorGPU::SetSkipBiomeStage(bool skip)
{
  skipBiomeStage = skip;
  if (skip)
  {
    RayLog::LogInfo(RAYLOG_TAG, "Skipping biome stage for integrated GPU; simplify generation");
  }
  else
  {
    RayLog::LogInfo(RAYLOG_TAG, "Enabling biome stage");
  }
}

void ChunkGeneratorGPU::SetSkipHeightmapStage(bool skip)
{
  skipHeightmapStage = skip;
  if (skip)
  {
    RayLog::LogInfo(RAYLOG_TAG, "Skipping heightmap stage for integrated GPU (minimal generation)");
  }
  else
  {
    RayLog::LogInfo(RAYLOG_TAG, "Enabling heightmap stage");
  }
}

void ChunkGeneratorGPU::SetSkipNoiseStage(bool skip)
{
  skipNoiseStage = skip;
  if (skip)
  {
    RayLog::LogInfo(RAYLOG_TAG, "Skipping noise stage for integrated GPU (minimal generation)");
  }
  else
  {
    RayLog::LogInfo(RAYLOG_TAG, "Enabling noise stage");
  }
}

void ChunkGeneratorGPU::SetSkipUnitPlaceStage(bool skip)
{
  skipUnitPlaceStage = skip;
  if (skip)
  {
    RayLog::LogInfo(RAYLOG_TAG, "Skipping unit placement stage for integrated GPU; minimal generation");
  }
  else
  {
    RayLog::LogInfo(RAYLOG_TAG, "Enabling unit placement stage");
  }
}

void ChunkGeneratorGPU::SetSkipReadback(bool skip)
{
  skipReadback = skip;
  if (skip)
  {
    RayLog::LogInfo(RAYLOG_TAG, "Skipping GPU-CPU readback - keeping data in GPU buffers");
  }
  else
  {
    RayLog::LogInfo(RAYLOG_TAG, "Enabling GPU-CPU readback");
  }
}

void ChunkGeneratorGPU::SetUseSlicedDispatch(bool use)
{
  useSlicedDispatch = use;
  if (use)
  {
    RayLog::LogInfo(RAYLOG_TAG, "Enabling sliced dispatch for integrated GPUs to prevent timeout");
  }
  else
  {
    RayLog::LogInfo(RAYLOG_TAG, "Using standard dispatch (non-sliced)");
  }
}

} // namespace Rl::World::Chunk
