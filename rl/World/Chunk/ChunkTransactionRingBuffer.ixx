export module Rl.World.Chunk.TransactionRingBuffer;

import Rl.World.Chunk.ChunkTransaction;
import <atomic>;
import <memory>;
import <cstdint>;
import <chrono>;

namespace Rl::World::Chunk
{

/* Statistics for ring buffer monitoring */
export struct RingBufferStats
{
  uint64_t totalPushes;
  uint64_t totalPops;
  uint64_t failedPushes;
  uint64_t failedPops;
  uint64_t peakSize;
  double averageSize;
  uint64_t overflows;
};

/* Lock-free ring buffer for concurrent transaction processing with robustness features */
export class ChunkTransactionRingBuffer
{
  public:
  explicit ChunkTransactionRingBuffer(uint32_t capacity = 1024);
  ~ChunkTransactionRingBuffer();
  
  /* Disable copy operations */
  ChunkTransactionRingBuffer(const ChunkTransactionRingBuffer&) = delete;
  ChunkTransactionRingBuffer& operator=(const ChunkTransactionRingBuffer&) = delete;
  
  /* Enable move operations */
  ChunkTransactionRingBuffer(ChunkTransactionRingBuffer&& other) noexcept;
  ChunkTransactionRingBuffer& operator=(ChunkTransactionRingBuffer&& other) noexcept;
  
  /* Push a transaction to the ring buffer with retry logic */
  [[nodiscard]]
  bool Push(const ChunkTransaction& transaction);
  
  /* Push with timeout, returns false if timeout expires */
  [[nodiscard]]
  bool PushWithTimeout(const ChunkTransaction& transaction, uint32_t timeoutMs);
  
  /* Pop a transaction from the ring buffer */
  [[nodiscard]]
  bool Pop(ChunkTransaction& transaction);
  
  /* Pop with timeout, returns false if timeout expires */
  [[nodiscard]]
  bool PopWithTimeout(ChunkTransaction& transaction, uint32_t timeoutMs);
  
  /* Check if the buffer is empty */
  [[nodiscard]]
  bool IsEmpty() const;
  
  /* Check if the buffer is full */
  [[nodiscard]]
  bool IsFull() const;
  
  /* Get the current size of the buffer */
  [[nodiscard]]
  uint32_t Size() const;
  
  /* Get the capacity of the buffer */
  [[nodiscard]]
  uint32_t Capacity() const;
  
  /* Clear the buffer and reset statistics */
  void Clear();
  
  /* Get current statistics */
  [[nodiscard]]
  RingBufferStats GetStats() const;
  
  /* Reset statistics */
  void ResetStats();
  
  /* Check if buffer is healthy (no overflows, reasonable stats) */
  [[nodiscard]]
  bool IsHealthy() const;

  private:
  std::unique_ptr<ChunkTransaction[]> buffer;
  uint32_t capacity;
  std::atomic<uint32_t> head;
  std::atomic<uint32_t> tail;
  
  /* Statistics tracking */
  mutable std::atomic<uint64_t> totalPushes;
  mutable std::atomic<uint64_t> totalPops;
  mutable std::atomic<uint64_t> failedPushes;
  mutable std::atomic<uint64_t> failedPops;
  mutable std::atomic<uint64_t> peakSize;
  mutable std::atomic<uint64_t> overflows;
  
  /* Get next index with wraparound */
  [[nodiscard]]
  uint32_t NextIndex(const uint32_t current) const
  {
    return (current + 1) % capacity;
  }
  
  /* Update peak size statistics */
  void UpdatePeakSize(uint32_t currentSize);
  
  /* Validate capacity is power of 2 for better performance */
  [[nodiscard]]
  static bool IsValidCapacity(uint32_t capacity);
};

} // namespace Rl::World::Chunk
