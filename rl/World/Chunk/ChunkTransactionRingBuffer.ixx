export module Rl.World.Chunk.TransactionRingBuffer;

import Rl.World.Chunk.ChunkTransaction;
import <atomic>;
import <memory>;
import <cstdint>;

namespace Rl::World::Chunk
{

/* Lock-free ring buffer for concurrent transaction processing */
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
  
  /* Push a transaction to the ring buffer */
  [[nodiscard]]
  bool Push(const ChunkTransaction& transaction);
  
  /* Pop a transaction from the ring buffer */
  [[nodiscard]]
  bool Pop(ChunkTransaction& transaction);
  
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
  
  /* Clear the buffer */
  void Clear();

  private:
  std::unique_ptr<ChunkTransaction[]> buffer;
  uint32_t capacity;
  std::atomic<uint32_t> head;
  std::atomic<uint32_t> tail;
  
  /* Get next index with wraparound */
  [[nodiscard]]
  uint32_t NextIndex(const uint32_t current) const
  {
    return (current + 1) % capacity;
  }
};

} // namespace Rl::World::Chunk
