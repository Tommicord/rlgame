export module Rl.World.Chunk.ChunkTransaction;

import <cstdint>;
import <atomic>;

namespace Rl::World::Chunk
{

/* Transaction type for chunk modifications */
export enum class TransactionType : uint32_t
{
  READ = 0,
  WRITE = 1,
  READ_WRITE = 2
};

/* Single chunk modification transaction */
export struct ChunkTransaction
{
  uint32_t chunkIndex;      // Index of the chunk in the render distance
  int32_t localX;           // Local X coordinate within chunk
  int32_t localY;           // Local Y coordinate within chunk
  int32_t localZ;           // Local Z coordinate within chunk
  uint32_t oldUnitId;       // Previous unit ID (for rollback)
  uint32_t newUnitId;       // New unit ID to write
  TransactionType type;     // Transaction type
  uint64_t timestamp;       // Transaction timestamp
  std::atomic<bool> completed; // Transaction completion flag
  
  ChunkTransaction() 
      : chunkIndex(0), localX(0), localY(0), localZ(0), 
        oldUnitId(0), newUnitId(0), type(TransactionType::READ),
        timestamp(0), completed(false)
  {
  }
  
  /* Check if this transaction is valid */
  [[nodiscard]]
  bool IsValid() const
  {
    return completed.load(std::memory_order_acquire);
  }
  
  /* Mark transaction as completed */
  void Complete()
  {
    completed.store(true, std::memory_order_release);
  }
  
  /* Reset transaction for reuse */
  void Reset()
  {
    chunkIndex = 0;
    localX = 0;
    localY = 0;
    localZ = 0;
    oldUnitId = 0;
    newUnitId = 0;
    type = TransactionType::READ;
    timestamp = 0;
    completed.store(false, std::memory_order_release);
  }
};

/* Transaction result for operations */
export struct TransactionResult
{
  bool success;
  uint32_t readUnitId;
  const char* errorMessage;
  
  TransactionResult() 
      : success(false), readUnitId(0), errorMessage(nullptr)
  {
  }
  
  static TransactionResult Ok(uint32_t unitId = 0)
  {
    TransactionResult result;
    result.success = true;
    result.readUnitId = unitId;
    return result;
  }
  
  static TransactionResult Error(const char* error)
  {
    TransactionResult result;
    result.success = false;
    result.errorMessage = error;
    return result;
  }
};

} // namespace Rl::World::Chunk
