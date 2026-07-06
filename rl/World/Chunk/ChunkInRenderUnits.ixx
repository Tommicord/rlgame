export module Rl.World.Chunk.ChunkInRenderUnits;

import Rl.World.Chunk.UnitChunkBuffer;
import Rl.World.Chunk.ChunkTransaction;
import Rl.World.Chunk.TransactionRingBuffer;
import <cstdint>;
import <memory>;
import <vector>;
import <mutex>;
import <atomic>;

namespace Rl::World::Chunk
{

/* Delta for GPU synchronization - represents a single unit change */
export struct ChunkDelta
{
  uint32_t chunkIndex;      // Index of the chunk
  int32_t localX;           // Local X coordinate
  int32_t localY;           // Local Y coordinate
  int32_t localZ;           // Local Z coordinate
  uint32_t oldUnitId;       // Previous unit ID
  uint32_t newUnitId;       // New unit ID
  uint64_t frameNumber;     // Frame number for synchronization
};

/* Chunk coordinate in world space */
export struct WorldChunkCoord
{
  int32_t chunkX;
  int32_t chunkY;
  int32_t chunkZ;
};

/* Global manager for chunks in render distance */
export class ChunkInRenderUnits
{
  public:
  /* Constructor with render distance parameters */
  ChunkInRenderUnits(int32_t renderDistanceX, int32_t renderDistanceY, int32_t renderDistanceZ);
  
  /* Destructor */
  ~ChunkInRenderUnits();
  
  /* Disable copy operations */
  ChunkInRenderUnits(const ChunkInRenderUnits&) = delete;
  ChunkInRenderUnits& operator=(const ChunkInRenderUnits&) = delete;
  
  /* Enable move operations */
  ChunkInRenderUnits(ChunkInRenderUnits&& other) noexcept;
  ChunkInRenderUnits& operator=(ChunkInRenderUnits&& other) noexcept;
  
  /* Initialize the chunk system */
  void Initialize();
  
  /* Shutdown the chunk system */
  void Shutdown();
  
  /* Add a chunk to the render distance */
  [[nodiscard]]
  bool AddChunk(const WorldChunkCoord& coord, UnitChunkBuffer& chunkBuffer);
  
  /* Remove a chunk from the render distance */
  [[nodiscard]]
  bool RemoveChunk(const WorldChunkCoord& coord);
  
  /* Get chunk buffer at world coordinates */
  [[nodiscard]]
  UnitChunkBuffer* GetChunkBuffer(const WorldChunkCoord& coord);
  
  /* Get chunk buffer at index */
  [[nodiscard]]
  UnitChunkBuffer* GetChunkBuffer(uint32_t index);
  
  /* Safe read operation with bounds checking */
  [[nodiscard]]
  TransactionResult ReadUnitId(const WorldChunkCoord& coord, int32_t localX, int32_t localY, int32_t localZ);
  
  /* Safe write operation with bounds checking and transaction */
  [[nodiscard]]
  TransactionResult WriteUnitId(const WorldChunkCoord& coord, int32_t localX, int32_t localY, int32_t localZ, uint32_t newUnitId);
  
  /* Enqueue a transaction for processing */
  [[nodiscard]]
  bool EnqueueTransaction(const ChunkTransaction& transaction);
  
  /* Process all pending transactions */
  void ProcessTransactions();
  
  /* Get all pending deltas for GPU synchronization */
  [[nodiscard]]
  std::vector<ChunkDelta> GetPendingDeltas();
  
  /* Clear processed deltas */
  void ClearProcessedDeltas();
  
  /* Get the number of chunks in render distance */
  [[nodiscard]]
  uint32_t GetChunkCount() const;
  
  /* Get render distance dimensions */
  [[nodiscard]]
  WorldChunkCoord GetRenderDistance() const;
  
  /* Check if a world coordinate is within render distance */
  [[nodiscard]]
  bool IsInRenderDistance(const WorldChunkCoord& coord) const;
  
  /* Get the total number of pending transactions */
  [[nodiscard]]
  uint32_t GetPendingTransactionCount() const;

  private:
  /* Convert world chunk coordinates to flat index */
  [[nodiscard]]
  uint32_t WorldCoordToIndex(const WorldChunkCoord& coord) const;
  
  /* Convert flat index to world chunk coordinates */
  [[nodiscard]]
  WorldChunkCoord IndexToWorldCoord(uint32_t index) const;
  
  /* Check if local coordinates are within chunk bounds */
  [[nodiscard]]
  bool IsInChunkBounds(int32_t localX, int32_t localY, int32_t localZ) const;
  
  /* Add a delta to the pending list */
  void AddDelta(const ChunkDelta& delta);
  
  /* Render distance dimensions */
  int32_t renderDistanceX;
  int32_t renderDistanceY;
  int32_t renderDistanceZ;
  
  /* Total number of chunks in render distance */
  uint32_t totalChunks;
  
  /* Flat array of chunk buffers */
  std::unique_ptr<UnitChunkBuffer[]> chunkBuffers;
  
  /* Array of chunk coordinates for each index */
  std::unique_ptr<WorldChunkCoord[]> chunkCoords;
  
  /* Array of active flags for each chunk slot */
  std::unique_ptr<std::atomic<bool>[]> chunkActive;
  
  /* Transaction ring buffer for concurrent access */
  ChunkTransactionRingBuffer transactionBuffer;
  
  /* Pending deltas for GPU synchronization */
  std::vector<ChunkDelta> pendingDeltas;
  
  /* Mutex for delta operations */
  std::mutex deltaMutex;
  
  /* Mutex for chunk array operations */
  mutable std::mutex chunkMutex;
  
  /* Frame counter for delta synchronization */
  std::atomic<uint64_t> frameCounter;
  
  /* Flag indicating if system is initialized */
  std::atomic<bool> initialized;
};

} // namespace Rl::World::Chunk
