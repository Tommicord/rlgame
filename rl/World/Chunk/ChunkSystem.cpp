import Rl.World.Chunk.ChunkSystem;

import Rl.World.Chunk.ChunkInRenderUnits;
import Rl.World.Chunk.UnitChunkBuffer;
import Rl.World.Chunk.UnitChunkAccessor;
import Rl.World.Chunk.ChunkGeneratorGPU;
import Rl.RayLog.Macro;
import Rl.RayLog.Logger;

import <algorithm>;
import <thread>;
import <stdexcept>;
import <utility>;
import <vector>;
import <mutex>;
import <vulkan/vulkan.hpp>;

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
  const uint64_t raw = (key >> shift) & CHUNK_KEY_MASK;
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
  const int32_t endX = (renderDistance.chunkX - 1) / 2;
  const int32_t startY = -(renderDistance.chunkY / 2);
  const int32_t endY = (renderDistance.chunkY - 1) / 2;
  const int32_t startZ = -(renderDistance.chunkZ / 2);
  const int32_t endZ = (renderDistance.chunkZ - 1) / 2;

  return candidate.chunkX >= playerChunk.chunkX + startX &&
         candidate.chunkX <= playerChunk.chunkX + endX &&
         candidate.chunkY >= playerChunk.chunkY + startY &&
         candidate.chunkY <= playerChunk.chunkY + endY &&
         candidate.chunkZ >= playerChunk.chunkZ + startZ &&
         candidate.chunkZ <= playerChunk.chunkZ + endZ;
}

ChunkSystem::ChunkSystem(ChunkInRenderUnits& chunkStore, WorldChunkCoord renderDistance) :
    chunkStore(chunkStore), renderDistance(renderDistance),
    workerPool(4), running(false), refreshRequested(false),
    generationInProgress(false), stopRequested(false)
{
  if (renderDistance.chunkX <= 0 || renderDistance.chunkY <= 0 || renderDistance.chunkZ <= 0)
  {
    throw std::invalid_argument("Render distance must be positive");
  }

  chunkGenerator =
    [](const WorldChunkCoord&, UnitChunkBuffer& chunkBuffer) {
      chunkBuffer.Clear();
      return true;
    };

  const bool initialized = chunkStore.Initialize();
  if (!initialized)
  {
    RayLog::LogError(RAYLOG_TAG, "Failed to initialize Chunk System storage");
  }

  running.store(true, std::memory_order_release);
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

void ChunkSystem::EnableGPUGeneration(ChunkGeneratorGPU* gpuGenerator, VkDevice device, VkPhysicalDevice physicalDevice)
{
  std::scoped_lock lock(stateMutex);
  this->gpuGenerator = gpuGenerator;
  this->gpuDevice = device;
  this->gpuPhysicalDevice = physicalDevice;
  this->gpuGenerationEnabled = true;
  RayLog::LogInfo(RAYLOG_TAG, "GPU generation enabled");
}

void ChunkSystem::DisableGPUGeneration()
{
  std::scoped_lock lock(stateMutex);
  gpuGenerator = nullptr;
  gpuDevice = VK_NULL_HANDLE;
  gpuPhysicalDevice = VK_NULL_HANDLE;
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
      std::scoped_lock lock(stateMutex);
      const WorldChunkCoord playerChunk = WorldPositionToChunkCoord(playerPosition);
      const int32_t startX = -(renderDistance.chunkX >> 1);
      const int32_t endX = (renderDistance.chunkX - 1) >> 1;
      const int32_t startY = -(renderDistance.chunkY >> 1);
      const int32_t endY = (renderDistance.chunkY - 1) >> 1;
      const int32_t startZ = -(renderDistance.chunkZ >> 1);
      const int32_t endZ = (renderDistance.chunkZ - 1) >> 1;

      for (int32_t z = startZ; z <= endZ; ++z)
      {
        for (int32_t y = startY; y <= endY; ++y)
        {
          for (int32_t x = startX; x <= endX; ++x)
          {
            WorldChunkCoord coord{playerChunk.chunkX + x, playerChunk.chunkY + y, playerChunk.chunkZ + z};
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
    playerChunk = WorldPositionToChunkCoord(playerPosition);
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
  WorldChunkCoord playerChunk{};
  WorldChunkCoord localRenderDistance{};
  bool useGPU = false;
  ChunkGeneratorGPU* localGpuGenerator = nullptr;
  VkDevice localGpuDevice = VK_NULL_HANDLE;
  VkPhysicalDevice localGpuPhysicalDevice = VK_NULL_HANDLE;
  {
    std::scoped_lock lock(stateMutex);
    playerChunk = WorldPositionToChunkCoord(playerPosition);
    localRenderDistance = renderDistance;
    useGPU = gpuGenerationEnabled;
    localGpuGenerator = gpuGenerator;
    localGpuDevice = gpuDevice;
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
    // Use GPU generation
    // Note: This requires a command buffer - in production, this would be integrated
    // with the rendering command buffer recording. For now, we'll use a fallback.
    RayLog::LogInfo(RAYLOG_TAG, "Using GPU generation for chunk [%d, %d, %d]",
                   coord.chunkX, coord.chunkY, coord.chunkZ);
    
    // Fallback to CPU generation for now - GPU generation needs command buffer integration
    if (!chunkGenerator(coord, chunkBuffer))
      return false;
  }
  else
  {
    // Use CPU generation
    if (!chunkGenerator(coord, chunkBuffer))
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
  std::scoped_lock lock(stateMutex);
  const WorldChunkCoord playerChunk = WorldPositionToChunkCoord(playerPosition);
  for (auto it = generatedChunkStorageCoords.begin(); it != generatedChunkStorageCoords.end();)
  {
    const uint64_t key = it->first;
    const WorldChunkCoord storageCoord = it->second;
    const int32_t chunkX = DecodeChunkKeyField(key, 42);
    const int32_t chunkY = DecodeChunkKeyField(key, 21);
    const int32_t chunkZ = DecodeChunkKeyField(key, 0);

    WorldChunkCoord coord{chunkX, chunkY, chunkZ};
    if (!IsInPlayerRange(playerChunk, renderDistance, coord))
    {
      chunkStore.RemoveChunk(storageCoord);
      it = generatedChunkStorageCoords.erase(it);
    }
    else
    {
      ++it;
    }
  }
}

} // namespace Rl::World::Chunk
