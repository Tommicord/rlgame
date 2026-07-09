export module Rl.World.Chunk.ChunkSystem;

import Rl.World.Chunk.ChunkInRenderUnits;
import Rl.World.Chunk.ChunkThreadPool;
import Rl.World.Chunk.UnitChunkAccessor;
import Rl.World.Chunk.UnitChunkBuffer;

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
};

} // namespace Rl::World::Chunk
