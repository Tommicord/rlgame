import Rl.World.Chunk.ChunkSystem;

import Rl.World.Chunk.ChunkInRenderUnits;
import Rl.World.Chunk.UnitChunkBuffer;
import Rl.World.Chunk.UnitChunkAccessor;
import Rl.World.Chunk.ChunkGeneratorGPU;
import Rl.World.Chunk.ChunkSystemNotify;
import Rl.RayLog.Macro;
import Rl.RayLog.Logger;

import <algorithm>;
import <thread>;
import <stdexcept>;
import <utility>;
import <vector>;
import <mutex>;
import <ranges>;
import <vulkan/vulkan.hpp>;
import Rl.Base.UserInput;

namespace Rl::World::Chunk
{

static constexpr uint64_t CHUNK_KEY_MASK = 0x1FFFFFULL;

static uint64_t MakeChunkKey(const WorldChunkCoord& coord)
{
  return ((static_cast<uint64_t>(coord.chunkX) & CHUNK_KEY_MASK) << 42) |
         ((static_cast<uint64_t>(coord.chunkY) & CHUNK_KEY_MASK) << 21) |
         (static_cast<uint64_t>(coord.chunkZ) & CHUNK_KEY_MASK);
}

static int32_t DecodeChunkKeyField(uint64_t key, int shift)
{
  const uint64_t raw  = (key >> shift) & CHUNK_KEY_MASK;
  const uint64_t sign = 1ULL << 20;
  if (raw & sign)
  {
    return static_cast<int32_t>(raw | ~CHUNK_KEY_MASK);
  }
  return static_cast<int32_t>(raw);
}

static WorldChunkCoord WorldPositionToChunkCoord(const UnitPosition& position)
{
  WorldChunkCoord coord{};
  coord.chunkX = position.worldX / UnitChunkBuffer::W;
  coord.chunkY = position.worldY / UnitChunkBuffer::H;
  coord.chunkZ = position.worldZ / UnitChunkBuffer::D;

  if (position.worldX < 0)
    --coord.chunkX;
  if (position.worldY < 0)
    --coord.chunkY;
  if (position.worldZ < 0)
    --coord.chunkZ;

  return coord;
}

static bool IsInPlayerRange(const WorldChunkCoord& playerChunk,
                            const WorldChunkCoord& renderDistance,
                            const WorldChunkCoord& candidate)
{
  const int32_t startX = -(renderDistance.chunkX / 2);
  const int32_t endX   = (renderDistance.chunkX - 1) / 2;
  const int32_t startY = -(renderDistance.chunkY / 2);
  const int32_t endY   = (renderDistance.chunkY - 1) / 2;
  const int32_t startZ = -(renderDistance.chunkZ / 2);
  const int32_t endZ   = (renderDistance.chunkZ - 1) / 2;

  return candidate.chunkX >= playerChunk.chunkX + startX &&
         candidate.chunkX <= playerChunk.chunkX + endX &&
         candidate.chunkY >= playerChunk.chunkY + startY &&
         candidate.chunkY <= playerChunk.chunkY + endY &&
         candidate.chunkZ >= playerChunk.chunkZ + startZ &&
         candidate.chunkZ <= playerChunk.chunkZ + endZ;
}

bool ChunkSystem::GenerateChunkWithGpu(ChunkGeneratorGPU*     gpuGenerator,
                                       VkDevice               device,
                                       VkPhysicalDevice       physicalDevice,
                                       const WorldChunkCoord& coord,
                                       UnitChunkBuffer&       chunkBuffer)
{
  if (gpuGenerator == nullptr || device == VK_NULL_HANDLE ||
      physicalDevice == VK_NULL_HANDLE)
  {
    RayLog::LogError(RAYLOG_TAG, "GPU generation parameters invalid");
    return false;
  }

  if (!gpuGenerator->IsInitialized())
  {
    RayLog::LogError(RAYLOG_TAG, "GPU generator not initialized");
    return false;
  }

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                           queueFamilies.data());

  uint32_t queueFamilyIndex = 0xFFFFFFFFu;
  for (uint32_t i = 0; i < queueFamilyCount; ++i)
  {
    if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
    {
      queueFamilyIndex = i;
      break;
    }
  }

  if (queueFamilyIndex == 0xFFFFFFFFu)
  {
    RayLog::LogError(RAYLOG_TAG,
                     "No compute-capable queue family found for chunk GPU generation");
    return false;
  }

  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = queueFamilyIndex;
  poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

  VkCommandPool commandPool = VK_NULL_HANDLE;
  if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
  {
    RayLog::LogError(RAYLOG_TAG,
                     "Failed to create command pool for chunk GPU generation");
    return false;
  }

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool        = commandPool;
  allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
  if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS)
  {
    vkDestroyCommandPool(device, commandPool, nullptr);
    RayLog::LogError(RAYLOG_TAG,
                     "Failed to allocate command buffer for chunk GPU generation");
    return false;
  }

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  bool success = false;
  VkResult beginResult = vkBeginCommandBuffer(commandBuffer, &beginInfo);
  if (beginResult == VK_SUCCESS)
  {
    success = gpuGenerator->GenerateChunk(device, commandBuffer, coord, chunkBuffer);
    if (success)
    {
      VkResult endResult = vkEndCommandBuffer(commandBuffer);
      if (endResult != VK_SUCCESS)
      {
        RayLog::LogError(RAYLOG_TAG, "Failed to end command buffer: %d", endResult);
        success = false;
      }
    }
    else
    {
      RayLog::LogError(RAYLOG_TAG, "GPU generator failed to generate chunk");
    }
  }
  else
  {
    RayLog::LogError(RAYLOG_TAG, "Failed to begin command buffer: %d", beginResult);
  }

  if (success)
  {
    VkQueue queue = VK_NULL_HANDLE;
    vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);

    VkSubmitInfo submitInfo{};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &commandBuffer;

    VkResult submitResult = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    if (submitResult == VK_SUCCESS)
    {
      // Use a fence with timeout instead of vkQueueWaitIdle to prevent indefinite hangs
      VkFenceCreateInfo fenceInfo{};
      fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
      VkFence fence = VK_NULL_HANDLE;
      if (vkCreateFence(device, &fenceInfo, nullptr, &fence) == VK_SUCCESS)
      {
        submitInfo.pSignalSemaphores = nullptr;
        submitInfo.signalSemaphoreCount = 0;
        
        VkResult submitWithFence = vkQueueSubmit(queue, 1, &submitInfo, fence);
        if (submitWithFence == VK_SUCCESS)
        {
          // Wait with 5 second timeout
          VkResult waitResult = vkWaitForFences(device, 1, &fence, VK_TRUE, 5000000000ULL);
          if (waitResult != VK_SUCCESS)
          {
            RayLog::LogError(RAYLOG_TAG, "GPU generation timeout or error: %d", waitResult);
            success = false;
          }
        }
        else
        {
          RayLog::LogError(RAYLOG_TAG, "Failed to submit with fence: %d", submitWithFence);
          success = false;
        }
        vkDestroyFence(device, fence, nullptr);
      }
      else
      {
        RayLog::LogError(RAYLOG_TAG, "Failed to create fence for GPU generation");
        success = false;
      }
    }
    else
    {
      RayLog::LogError(RAYLOG_TAG, "Failed to submit GPU generation: %d", submitResult);
      success = false;
    }
  }

  vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
  vkDestroyCommandPool(device, commandPool, nullptr);

  if (!success)
    RayLog::LogError(RAYLOG_TAG, "GPU chunk generation failed for chunk [%d, %d, %d]",
                     coord.chunkX, coord.chunkY, coord.chunkZ);

  return success;
}

ChunkSystem::ChunkSystem(ChunkInRenderUnits& chunkStore, WorldChunkCoord renderDistance) :
    chunkStore(chunkStore),
    renderDistance(renderDistance),
    workerPool(4),
    running(false),
    refreshRequested(false),
    generationInProgress(false),
    stopRequested(false)
{
  if (renderDistance.chunkX <= 0 || renderDistance.chunkY <= 0 ||
      renderDistance.chunkZ <= 0)
  {
    throw std::invalid_argument("Render distance must be positive");
  }
  chunkGenerator = [](const WorldChunkCoord&, UnitChunkBuffer& chunkBuffer)
  {
    chunkBuffer.Clear();
    return true;
  };
  const bool initialized = chunkStore.Initialize();
  if (!initialized) {
    RayLog::LogError(RAYLOG_TAG, "Failed to initialize Chunk System storage");
  }
  running.store(true, std::memory_order_release);
  // Create the notifier
  notifier = std::make_unique<ChunkSystemNotify>(*this);
  // Finally start the generation thread
  generationThread = std::jthread([this] { RunGenerationLoop(); });
}

ChunkSystem::~ChunkSystem()
{ Stop(); }

void ChunkSystem::SetPlayerPosition(const UnitPosition& position)
{
  std::scoped_lock lock(stateMutex);
  playerPosition = position;
  refreshRequested.store(true, std::memory_order_release);
}

void ChunkSystem::SetRenderDistance(WorldChunkCoord distance)
{
  std::scoped_lock lock(stateMutex);
  renderDistance = distance;
  refreshRequested.store(true, std::memory_order_release);
}

void ChunkSystem::SetChunkGenerator(ChunkGenerator generator)
{
  std::scoped_lock lock(stateMutex);
  chunkGenerator = std::move(generator);
}

void ChunkSystem::EnableGPUGeneration(ChunkGeneratorGPU* gpuGenerator,
                                      VkDevice           device,
                                      VkPhysicalDevice   physicalDevice)
{
  std::scoped_lock lock(stateMutex);
  this->gpuGenerator         = gpuGenerator;
  this->gpuDevice            = device;
  this->gpuPhysicalDevice    = physicalDevice;
  this->gpuGenerationEnabled = true;
  RayLog::LogInfo(RAYLOG_TAG, "GPU generation enabled");
}

void ChunkSystem::DisableGPUGeneration()
{
  std::scoped_lock lock(stateMutex);
  gpuGenerator         = nullptr;
  gpuDevice            = VK_NULL_HANDLE;
  gpuPhysicalDevice    = VK_NULL_HANDLE;
  gpuGenerationEnabled = false;
  RayLog::LogInfo(RAYLOG_TAG, "GPU generation disabled");
}

bool ChunkSystem::IsRunning() const
{ return running.load(std::memory_order_acquire); }

bool ChunkSystem::IsChunkGenerated(const WorldChunkCoord& coord) const
{
  std::scoped_lock lock(stateMutex);
  return generatedChunkStorageCoords.contains(MakeChunkKey(coord));
}

bool ChunkSystem::HasPendingWork() const
{
  std::scoped_lock lock(stateMutex);
  return refreshRequested.load(std::memory_order_acquire) ||
         generationInProgress.load(std::memory_order_acquire);
}

void ChunkSystem::RefreshNow()
{
  refreshRequested.store(true, std::memory_order_release);
}

void ChunkSystem::RefreshAndWait()
{
  refreshRequested.store(true, std::memory_order_release);
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(512);
  while (generationInProgress.load(std::memory_order_acquire) &&
         std::chrono::steady_clock::now() < deadline)
  {
    std::this_thread::yield();
  }
}

void ChunkSystem::Stop()
{
  if (stopRequested.exchange(true, std::memory_order_acq_rel))
    return;
  refreshRequested.store(true, std::memory_order_release);
  if (generationThread.joinable())
    generationThread.join();
}

void ChunkSystem::RunGenerationLoop()
{
  while (!stopRequested.load(std::memory_order_acquire))
  {
    if (refreshRequested.exchange(false, std::memory_order_acq_rel))
    {
      RefreshInternal();
    }
    else
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
  }
}

void ChunkSystem::RefreshInternal()
{
  if (generationInProgress.exchange(true, std::memory_order_acq_rel))
    return;
  try
  {
    std::vector<WorldChunkCoord> coordsToGenerate;
    {
      std::scoped_lock      lock(stateMutex);
      const WorldChunkCoord playerChunk = WorldPositionToChunkCoord(playerPosition);
      const int32_t         startX      = -(renderDistance.chunkX / 2);
      const int32_t         endX        = (renderDistance.chunkX - 1) / 2;
      const int32_t         startY      = -(renderDistance.chunkY / 2);
      const int32_t         endY        = (renderDistance.chunkY - 1) / 2;
      const int32_t         startZ      = -(renderDistance.chunkZ / 2);
      const int32_t         endZ        = (renderDistance.chunkZ - 1) / 2;

      for (int32_t z = startZ; z <= endZ; ++z)
      {
        for (int32_t y = startY; y <= endY; ++y)
        {
          for (int32_t x = startX; x <= endX; ++x)
          {
            WorldChunkCoord coord{playerChunk.chunkX + x, playerChunk.chunkY + y,
                                  playerChunk.chunkZ + z};
            if (!ShouldGenerate(coord))
              continue;
            coordsToGenerate.push_back(coord);
          }
        }
      }
    }
    for (const auto& coord : coordsToGenerate)
    {
      GenerateChunk(coord);
    }
    RemoveOutOfRangeChunks();
  }
  catch (...)
  {
    generationInProgress.store(false, std::memory_order_release);
    throw;
  }
  generationInProgress.store(false, std::memory_order_release);
}

bool ChunkSystem::ShouldGenerate(const WorldChunkCoord& coord) const
{
  WorldChunkCoord playerChunk{};
  WorldChunkCoord localRenderDistance{};
  {
    std::scoped_lock lock(stateMutex);
    playerChunk         = WorldPositionToChunkCoord(playerPosition);
    localRenderDistance = renderDistance;
  }

  if (!IsInPlayerRange(playerChunk, localRenderDistance, coord))
    return false;

  if (IsChunkGenerated(coord))
    return false;

  return true;
}

bool ChunkSystem::GenerateChunk(const WorldChunkCoord& coord)
{
  WorldChunkCoord    playerChunk{};
  WorldChunkCoord    localRenderDistance{};
  bool               useGPU                 = true;
  ChunkGeneratorGPU* localGpuGenerator      = nullptr;
  VkDevice           localGpuDevice         = VK_NULL_HANDLE;
  VkPhysicalDevice   localGpuPhysicalDevice = VK_NULL_HANDLE;
  {
    std::scoped_lock lock(stateMutex);
    playerChunk            = WorldPositionToChunkCoord(playerPosition);
    localRenderDistance    = renderDistance;
    useGPU                 = gpuGenerationEnabled;
    localGpuGenerator      = gpuGenerator;
    localGpuDevice         = gpuDevice;
    localGpuPhysicalDevice = gpuPhysicalDevice;
  }

  if (!IsInPlayerRange(playerChunk, localRenderDistance, coord))
    return false;
  if (IsChunkGenerated(coord))
    return true;

  const WorldChunkCoord storageCoord{
      coord.chunkX - playerChunk.chunkX + (localRenderDistance.chunkX / 2),
      coord.chunkY - playerChunk.chunkY + (localRenderDistance.chunkY / 2),
      coord.chunkZ - playerChunk.chunkZ + (localRenderDistance.chunkZ / 2)};

  UnitChunkBuffer chunkBuffer{};

  if (useGPU && localGpuGenerator && localGpuDevice != VK_NULL_HANDLE)
  {
    RayLog::LogInfo(RAYLOG_TAG, "Using GPU generation for chunk [%d, %d, %d]",
                    coord.chunkX, coord.chunkY, coord.chunkZ);

    if (!GenerateChunkWithGpu(localGpuGenerator, localGpuDevice, localGpuPhysicalDevice,
                              coord, chunkBuffer))
      return false;
  }
  else
  {
    RayLog::LogInfo(RAYLOG_TAG, "GPU generation disabled for chunk [%d, %d, %d]",
                    coord.chunkX, coord.chunkY, coord.chunkZ);
    return false;
  }

  if (!chunkStore.AddChunk(storageCoord, chunkBuffer))
    return false;

  std::scoped_lock lock(stateMutex);
  generatedChunkStorageCoords[MakeChunkKey(coord)] = storageCoord;
  return true;
}

void ChunkSystem::RemoveOutOfRangeChunks()
{
  std::vector<std::pair<uint64_t, WorldChunkCoord>> chunksToRemove;
  {
    std::scoped_lock      lock(stateMutex);
    const WorldChunkCoord playerChunk = WorldPositionToChunkCoord(playerPosition);
    for (auto it = generatedChunkStorageCoords.begin();
         it != generatedChunkStorageCoords.end();)
    {
      const uint64_t        key          = it->first;
      const WorldChunkCoord storageCoord = it->second;
      const int32_t         chunkX       = DecodeChunkKeyField(key, 42);
      const int32_t         chunkY       = DecodeChunkKeyField(key, 21);
      const int32_t         chunkZ       = DecodeChunkKeyField(key, 0);

      WorldChunkCoord coord{chunkX, chunkY, chunkZ};
      if (!IsInPlayerRange(playerChunk, renderDistance, coord))
      {
        chunksToRemove.push_back({key, storageCoord});
        it = generatedChunkStorageCoords.erase(it);
      }
      else
      {
        ++it;
      }
    }
  }
  for (const auto& storageCoord : chunksToRemove | std::views::values)
  {
    chunkStore.RemoveChunk(storageCoord);
  }
}

} // namespace Rl::World::Chunk
