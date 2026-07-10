export module Rl.World.Chunk.ChunkSystem;

import Rl.World.Chunk.ChunkInRenderUnits;
import Rl.World.Chunk.ChunkThreadPool;
import Rl.World.Chunk.UnitChunkAccessor;
import Rl.World.Chunk.UnitChunkBuffer;
import Rl.World.Chunk.ChunkGeneratorGPU;

import <atomic>;
import <chrono>;
import <cstdint>;
import <functional>;
import <future>;
import <memory>;
import <mutex>;
import <optional>;
import <thread>;
import <unordered_map>;
import <utility>;
import <vector>;
import <vulkan/vulkan.hpp>;

namespace Rl::World::Chunk
{

export class ChunkSystem
{
protected:
  static constexpr auto RAYLOG_TAG = "ChunkSystem";
  public:
  using ChunkGenerator = std::function<bool(const WorldChunkCoord&, UnitChunkBuffer&)>;

  ChunkSystem(ChunkInRenderUnits& chunkStore, WorldChunkCoord renderDistance);
  ~ChunkSystem();

  ChunkSystem(const ChunkSystem&) = delete;
  ChunkSystem& operator=(const ChunkSystem&) = delete;

  ChunkSystem(ChunkSystem&&) noexcept = delete;
  ChunkSystem& operator=(ChunkSystem&&) noexcept = delete;

  void SetPlayerPosition(const UnitPosition& position);
  void SetRenderDistance(WorldChunkCoord distance);
  void SetChunkGenerator(ChunkGenerator generator);

  /* Enable GPU-based world generation */
  void EnableGPUGeneration(ChunkGeneratorGPU* gpuGenerator, VkDevice device, VkPhysicalDevice physicalDevice);
  void DisableGPUGeneration();

  [[nodiscard]] bool IsRunning() const;
  [[nodiscard]] bool IsChunkGenerated(const WorldChunkCoord& coord) const;
  [[nodiscard]] bool HasPendingWork() const;

  void RefreshNow();
  void Stop();

  private:
  void RunGenerationLoop();
  void RefreshInternal();
  bool ShouldGenerate(const WorldChunkCoord& coord) const;
  bool GenerateChunk(const WorldChunkCoord& coord);
  void RemoveOutOfRangeChunks();

  ChunkInRenderUnits& chunkStore;
  WorldChunkCoord renderDistance;
  UnitPosition playerPosition;
  ChunkGenerator chunkGenerator;
  ChunkThreadPool workerPool;
  std::atomic<bool> running;
  std::atomic<bool> refreshRequested;
  std::atomic<bool> generationInProgress;
  std::atomic<bool> stopRequested;
  mutable std::mutex stateMutex;
  std::unordered_map<uint64_t, WorldChunkCoord> generatedChunkStorageCoords;
  std::jthread generationThread;

  /* GPU generation support */
  ChunkGeneratorGPU* gpuGenerator = nullptr;
  VkDevice gpuDevice = VK_NULL_HANDLE;
  VkPhysicalDevice gpuPhysicalDevice = VK_NULL_HANDLE;
  bool gpuGenerationEnabled = false;
};

} // namespace Rl::World::Chunk
