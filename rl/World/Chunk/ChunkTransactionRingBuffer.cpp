import Rl.World.Chunk.TransactionRingBuffer;
import Rl.World.Chunk.ChunkTransaction;

import <memory>;

namespace Rl::World::Chunk
{

ChunkTransactionRingBuffer::ChunkTransactionRingBuffer(const uint32_t capacity)
    : capacity(capacity), head(0), tail(0)
{
  buffer = std::make_unique<ChunkTransaction[]>(capacity);
}

ChunkTransactionRingBuffer::~ChunkTransactionRingBuffer()
{
  buffer.reset();
}

ChunkTransactionRingBuffer::ChunkTransactionRingBuffer(ChunkTransactionRingBuffer&& other) noexcept
    : buffer(std::move(other.buffer)), 
      capacity(other.capacity), 
      head(other.head.load()), 
      tail(other.tail.load())
{
  other.capacity = 0;
  other.head.store(0);
  other.tail.store(0);
}

ChunkTransactionRingBuffer& ChunkTransactionRingBuffer::operator=(ChunkTransactionRingBuffer&& other) noexcept
{
  if (this != &other)
  {
    buffer.reset();
    buffer = std::move(other.buffer);
    capacity = other.capacity;
    head.store(other.head.load());
    tail.store(other.tail.load());
    
    other.capacity = 0;
    other.head.store(0);
    other.tail.store(0);
  }
  return *this;
}

bool ChunkTransactionRingBuffer::Push(const ChunkTransaction& transaction)
{
  const uint32_t currentHead = head.load(std::memory_order_acquire);
  const uint32_t nextHead = NextIndex(currentHead);
  
  // Check if buffer is full
  if (nextHead == tail.load(std::memory_order_acquire))
  {
    return false;
  }
  
  buffer[currentHead] = transaction;
  head.store(nextHead, std::memory_order_release);
  return true;
}

bool ChunkTransactionRingBuffer::Pop(ChunkTransaction& transaction)
{
  const uint32_t currentTail = tail.load(std::memory_order_acquire);
  // Check if buffer is empty
  if (currentTail == head.load(std::memory_order_acquire))
  {
    return false;
  }
  transaction = buffer[currentTail];
  tail.store(NextIndex(currentTail), std::memory_order_release);
  return true;
}

bool ChunkTransactionRingBuffer::IsEmpty() const
{
  return tail.load(std::memory_order_acquire) == head.load(std::memory_order_acquire);
}

bool ChunkTransactionRingBuffer::IsFull() const
{
  const uint32_t nextHead = NextIndex(head.load(std::memory_order_acquire));
  return nextHead == tail.load(std::memory_order_acquire);
}

uint32_t ChunkTransactionRingBuffer::Size() const
{
  const uint32_t currentHead = head.load(std::memory_order_acquire);
  const uint32_t currentTail = tail.load(std::memory_order_acquire);
  
  if (currentHead >= currentTail)
  {
    return currentHead - currentTail;
  }
  else
  {
    return capacity - currentTail + currentHead;
  }
}

uint32_t ChunkTransactionRingBuffer::Capacity() const
{
  return capacity;
}

void ChunkTransactionRingBuffer::Clear()
{
  head.store(0, std::memory_order_release);
  tail.store(0, std::memory_order_release);
}

} // namespace Rl::World::Chunk
