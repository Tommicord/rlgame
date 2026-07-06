import Rl.World.Chunk.ChunkInRenderUnits;

import <algorithm>;
import <stdexcept>;

namespace Rl::World::Chunk
{

ChunkInRenderUnits::ChunkInRenderUnits(int32_t renderDistanceX, int32_t renderDistanceY, int32_t renderDistanceZ)
    : renderDistanceX(renderDistanceX), 
      renderDistanceY(renderDistanceY), 
      renderDistanceZ(renderDistanceZ),
      totalChunks(0),
      transactionBuffer(4096),
      frameCounter(0),
      initialized(false)
{
  totalChunks = static_cast<uint32_t>(renderDistanceX * renderDistanceY * renderDistanceZ);
}

ChunkInRenderUnits::~ChunkInRenderUnits()
{
  Shutdown();
}

ChunkInRenderUnits::ChunkInRenderUnits(ChunkInRenderUnits&& other) noexcept
    : renderDistanceX(other.renderDistanceX),
      renderDistanceY(other.renderDistanceY),
      renderDistanceZ(other.renderDistanceZ),
      totalChunks(other.totalChunks),
      chunkBuffers(std::move(other.chunkBuffers)),
      chunkCoords(std::move(other.chunkCoords)),
      chunkActive(std::move(other.chunkActive)),
      transactionBuffer(std::move(other.transactionBuffer)),
      pendingDeltas(std::move(other.pendingDeltas)),
      frameCounter(other.frameCounter.load()),
      initialized(other.initialized.load())
{
  other.totalChunks = 0;
  other.frameCounter.store(0);
  other.initialized.store(false);
}

ChunkInRenderUnits& ChunkInRenderUnits::operator=(ChunkInRenderUnits&& other) noexcept
{
  if (this != &other)
  {
    Shutdown();
    
    renderDistanceX = other.renderDistanceX;
    renderDistanceY = other.renderDistanceY;
    renderDistanceZ = other.renderDistanceZ;
    totalChunks = other.totalChunks;
    chunkBuffers = std::move(other.chunkBuffers);
    chunkCoords = std::move(other.chunkCoords);
    chunkActive = std::move(other.chunkActive);
    transactionBuffer = std::move(other.transactionBuffer);
    pendingDeltas = std::move(other.pendingDeltas);
    frameCounter.store(other.frameCounter.load());
    initialized.store(other.initialized.load());
    
    other.totalChunks = 0;
    other.frameCounter.store(0);
    other.initialized.store(false);
  }
  return *this;
}

void ChunkInRenderUnits::Initialize()
{
  if (initialized.load())
    return;
  
  std::lock_guard<std::mutex> lock(chunkMutex);
  
  chunkBuffers = std::make_unique<UnitChunkBuffer[]>(totalChunks);
  chunkCoords = std::make_unique<WorldChunkCoord[]>(totalChunks);
  chunkActive = std::make_unique<std::atomic<bool>[]>(totalChunks);
  
  for (uint32_t i = 0; i < totalChunks; ++i)
  {
    chunkActive[i].store(false, std::memory_order_release);
  }
  
  initialized.store(true, std::memory_order_release);
}

void ChunkInRenderUnits::Shutdown()
{
  if (!initialized.load())
    return;
  
  std::lock_guard<std::mutex> lock(chunkMutex);
  
  chunkBuffers.reset();
  chunkCoords.reset();
  chunkActive.reset();
  pendingDeltas.clear();
  
  initialized.store(false, std::memory_order_release);
}

bool ChunkInRenderUnits::AddChunk(const WorldChunkCoord& coord, UnitChunkBuffer& chunkBuffer)
{
  if (!initialized.load())
    return false;
  
  if (!IsInRenderDistance(coord))
    return false;
  
  uint32_t index = WorldCoordToIndex(coord);
  
  std::lock_guard<std::mutex> lock(chunkMutex);
  
  if (chunkActive[index].load(std::memory_order_acquire))
    return false; // Chunk already exists at this location
  
  chunkBuffers[index] = std::move(chunkBuffer);
  chunkCoords[index] = coord;
  chunkActive[index].store(true, std::memory_order_release);
  
  return true;
}

bool ChunkInRenderUnits::RemoveChunk(const WorldChunkCoord& coord)
{
  if (!initialized.load())
    return false;
  
  if (!IsInRenderDistance(coord))
    return false;
  
  uint32_t index = WorldCoordToIndex(coord);
  
  std::lock_guard<std::mutex> lock(chunkMutex);
  
  if (!chunkActive[index].load(std::memory_order_acquire))
    return false; // Chunk doesn't exist at this location
  
  chunkActive[index].store(false, std::memory_order_release);
  
  return true;
}

UnitChunkBuffer* ChunkInRenderUnits::GetChunkBuffer(const WorldChunkCoord& coord)
{
  if (!initialized.load())
    return nullptr;
  
  if (!IsInRenderDistance(coord))
    return nullptr;
  
  uint32_t index = WorldCoordToIndex(coord);
  
  std::lock_guard<std::mutex> lock(chunkMutex);
  
  if (!chunkActive[index].load(std::memory_order_acquire))
    return nullptr;
  
  return &chunkBuffers[index];
}

UnitChunkBuffer* ChunkInRenderUnits::GetChunkBuffer(uint32_t index)
{
  if (!initialized.load())
    return nullptr;
  
  if (index >= totalChunks)
    return nullptr;
  
  std::lock_guard<std::mutex> lock(chunkMutex);
  
  if (!chunkActive[index].load(std::memory_order_acquire))
    return nullptr;
  
  return &chunkBuffers[index];
}

TransactionResult ChunkInRenderUnits::ReadUnitId(const WorldChunkCoord& coord, int32_t localX, int32_t localY, int32_t localZ)
{
  if (!initialized.load())
    return TransactionResult::Error("System not initialized");
  
  if (!IsInChunkBounds(localX, localY, localZ))
    return TransactionResult::Error("Local coordinates out of bounds");
  
  UnitChunkBuffer* chunk = GetChunkBuffer(coord);
  if (!chunk)
    return TransactionResult::Error("Chunk not found");
  
  auto unitId = chunk->GetUnitIdXYZ(localX, localY, localZ);
  if (!unitId)
    return TransactionResult::Error("Failed to read unit ID");
  
  return TransactionResult::Ok(static_cast<uint32_t>(unitId.value()));
}

TransactionResult ChunkInRenderUnits::WriteUnitId(const WorldChunkCoord& coord, int32_t localX, int32_t localY, int32_t localZ, uint32_t newUnitId)
{
  if (!initialized.load())
    return TransactionResult::Error("System not initialized");
  
  if (!IsInChunkBounds(localX, localY, localZ))
    return TransactionResult::Error("Local coordinates out of bounds");
  
  UnitChunkBuffer* chunk = GetChunkBuffer(coord);
  if (!chunk)
    return TransactionResult::Error("Chunk not found");
  
  // Read current value for delta
  auto oldUnitId = chunk->GetUnitIdXYZ(localX, localY, localZ);
  if (!oldUnitId)
    return TransactionResult::Error("Failed to read current unit ID");
  
  uint32_t chunkIndex = WorldCoordToIndex(coord);
  
  // Create transaction
  ChunkTransaction transaction;
  transaction.chunkIndex = chunkIndex;
  transaction.localX = localX;
  transaction.localY = localY;
  transaction.localZ = localZ;
  transaction.oldUnitId = static_cast<uint32_t>(oldUnitId.value());
  transaction.newUnitId = newUnitId;
  transaction.type = TransactionType::WRITE;
  transaction.timestamp = frameCounter.load();
  
  // Enqueue transaction
  if (!EnqueueTransaction(transaction))
    return TransactionResult::Error("Failed to enqueue transaction");
  
  return TransactionResult::Ok(newUnitId);
}

bool ChunkInRenderUnits::EnqueueTransaction(const ChunkTransaction& transaction)
{
  return transactionBuffer.Push(transaction);
}

void ChunkInRenderUnits::ProcessTransactions()
{
  if (!initialized.load())
    return;
  
  ChunkTransaction transaction;
  while (transactionBuffer.Pop(transaction))
  {
    // Apply the transaction to the chunk
    UnitChunkBuffer* chunk = GetChunkBuffer(transaction.chunkIndex);
    if (chunk && chunkActive[transaction.chunkIndex].load())
    {
      // Get the raw buffer and write the new unit ID
      int* buffer = chunk->GetRaw();
      int index = IndexMap3d2<UnitChunkBuffer::W, UnitChunkBuffer::H>(
          transaction.localX, transaction.localY, transaction.localZ);
      
      if (index >= 0 && index < UnitChunkBuffer::GetTotalBlocks())
      {
        buffer[index] = static_cast<int>(transaction.newUnitId);
        
        // Add delta for GPU synchronization
        ChunkDelta delta;
        delta.chunkIndex = transaction.chunkIndex;
        delta.localX = transaction.localX;
        delta.localY = transaction.localY;
        delta.localZ = transaction.localZ;
        delta.oldUnitId = transaction.oldUnitId;
        delta.newUnitId = transaction.newUnitId;
        delta.frameNumber = frameCounter.load();
        
        AddDelta(delta);
      }
    }
    
    transaction.Complete();
  }
  
  // Increment frame counter
  frameCounter.fetch_add(1, std::memory_order_release);
}

std::vector<ChunkDelta> ChunkInRenderUnits::GetPendingDeltas()
{
  std::lock_guard<std::mutex> lock(deltaMutex);
  return pendingDeltas;
}

void ChunkInRenderUnits::ClearProcessedDeltas()
{
  std::lock_guard<std::mutex> lock(deltaMutex);
  pendingDeltas.clear();
}

uint32_t ChunkInRenderUnits::GetChunkCount() const
{
  if (!initialized.load())
    return 0;
  
  uint32_t count = 0;
  for (uint32_t i = 0; i < totalChunks; ++i)
  {
    if (chunkActive[i].load(std::memory_order_acquire))
      count++;
  }
  return count;
}

WorldChunkCoord ChunkInRenderUnits::GetRenderDistance() const
{
  return {renderDistanceX, renderDistanceY, renderDistanceZ};
}

bool ChunkInRenderUnits::IsInRenderDistance(const WorldChunkCoord& coord) const
{
  return coord.chunkX >= 0 && coord.chunkX < renderDistanceX &&
         coord.chunkY >= 0 && coord.chunkY < renderDistanceY &&
         coord.chunkZ >= 0 && coord.chunkZ < renderDistanceZ;
}

uint32_t ChunkInRenderUnits::GetPendingTransactionCount() const
{
  return transactionBuffer.Size();
}

uint32_t ChunkInRenderUnits::WorldCoordToIndex(const WorldChunkCoord& coord) const
{
  return static_cast<uint32_t>(
      coord.chunkX + 
      coord.chunkY * renderDistanceX + 
      coord.chunkZ * renderDistanceX * renderDistanceY);
}

WorldChunkCoord ChunkInRenderUnits::IndexToWorldCoord(uint32_t index) const
{
  WorldChunkCoord coord;
  coord.chunkX = index % renderDistanceX;
  coord.chunkY = (index / renderDistanceX) % renderDistanceY;
  coord.chunkZ = index / (renderDistanceX * renderDistanceY);
  return coord;
}

bool ChunkInRenderUnits::IsInChunkBounds(int32_t localX, int32_t localY, int32_t localZ) const
{
  return localX >= 0 && localX < UnitChunkBuffer::W &&
         localY >= 0 && localY < UnitChunkBuffer::H &&
         localZ >= 0 && localZ < UnitChunkBuffer::D;
}

void ChunkInRenderUnits::AddDelta(const ChunkDelta& delta)
{
  std::lock_guard<std::mutex> lock(deltaMutex);
  pendingDeltas.push_back(delta);
}

} // namespace Rl::World::Chunk
