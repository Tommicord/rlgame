import Rl.World.Chunk.TransactionRingBuffer;
import Rl.World.Chunk.ChunkTransaction;

import <memory>;
import <thread>;
import <chrono>;
import <cmath>;

namespace Rl::World::Chunk
{

ChunkTransactionRingBuffer::ChunkTransactionRingBuffer(const uint32_t capacity) :
    capacity(capacity), head(0), tail(0), totalPushes(0), totalPops(0), failedPushes(0),
    failedPops(0), peakSize(0), overflows(0)
{
  if (!IsValidCapacity(capacity))
  {
    // Round up to next power of 2 for better performance
    uint32_t adjustedCapacity = 1;
    while (adjustedCapacity < capacity)
    {
      adjustedCapacity <<= 1;
    }
    const_cast<uint32_t&>(capacity) = adjustedCapacity;
  }

  buffer = std::make_unique<ChunkTransaction[]>(capacity);
}

ChunkTransactionRingBuffer::~ChunkTransactionRingBuffer()
{ buffer.reset(); }

ChunkTransactionRingBuffer::ChunkTransactionRingBuffer(
    ChunkTransactionRingBuffer&& other) noexcept :
    buffer(std::move(other.buffer)), capacity(other.capacity), head(other.head.load()),
    tail(other.tail.load()), totalPushes(other.totalPushes.load()),
    totalPops(other.totalPops.load()), failedPushes(other.failedPushes.load()),
    failedPops(other.failedPops.load()), peakSize(other.peakSize.load()),
    overflows(other.overflows.load())
{
  other.capacity = 0;
  other.head.store(0);
  other.tail.store(0);
  other.totalPushes.store(0);
  other.totalPops.store(0);
  other.failedPushes.store(0);
  other.failedPops.store(0);
  other.peakSize.store(0);
  other.overflows.store(0);
}

ChunkTransactionRingBuffer& ChunkTransactionRingBuffer::operator=(
    ChunkTransactionRingBuffer&& other) noexcept
{
  if (this != &other)
  {
    buffer.reset();
    buffer = std::move(other.buffer);
    capacity = other.capacity;
    head.store(other.head.load());
    tail.store(other.tail.load());
    totalPushes.store(other.totalPushes.load());
    totalPops.store(other.totalPops.load());
    failedPushes.store(other.failedPushes.load());
    failedPops.store(other.failedPops.load());
    peakSize.store(other.peakSize.load());
    overflows.store(other.overflows.load());

    other.capacity = 0;
    other.head.store(0);
    other.tail.store(0);
    other.totalPushes.store(0);
    other.totalPops.store(0);
    other.failedPushes.store(0);
    other.failedPops.store(0);
    other.peakSize.store(0);
    other.overflows.store(0);
  }
  return *this;
}

bool ChunkTransactionRingBuffer::Push(ChunkTransaction transaction)
{
  const uint32_t currentHead = head.load(std::memory_order_acquire);
  const uint32_t nextHead = NextIndex(currentHead);

  // Check if buffer is full
  if (nextHead == tail.load(std::memory_order_acquire))
  {
    failedPushes.fetch_add(1, std::memory_order_release);
    overflows.fetch_add(1, std::memory_order_release);
    return false;
  }

  buffer[currentHead] = std::move(transaction);
  head.store(nextHead, std::memory_order_release);
  totalPushes.fetch_add(1, std::memory_order_release);

  const uint32_t currentSize = Size();
  UpdatePeakSize(currentSize);

  return true;
}

bool ChunkTransactionRingBuffer::PushWithTimeout(
    ChunkTransaction transaction, const uint32_t timeoutMs)
{
  const auto startTime = std::chrono::steady_clock::now();
  const auto timeoutDuration = std::chrono::milliseconds(timeoutMs);

  while (true)
  {
    if (Push(std::move(transaction)))
    {
      return true;
    }

    const auto elapsed = std::chrono::steady_clock::now() - startTime;
    if (elapsed >= timeoutDuration)
    {
      return false;
    }

    std::this_thread::yield();
  }
}

bool ChunkTransactionRingBuffer::Pop(ChunkTransaction& transaction)
{
  const uint32_t currentTail = tail.load(std::memory_order_acquire);
  // Check if buffer is empty
  if (currentTail == head.load(std::memory_order_acquire))
  {
    failedPops.fetch_add(1, std::memory_order_release);
    return false;
  }

  transaction = std::move(buffer[currentTail]);
  tail.store(NextIndex(currentTail), std::memory_order_release);
  totalPops.fetch_add(1, std::memory_order_release);

  return true;
}

bool ChunkTransactionRingBuffer::PopWithTimeout(
    ChunkTransaction& transaction, const uint32_t timeoutMs)
{
  const auto startTime = std::chrono::steady_clock::now();
  const auto timeoutDuration = std::chrono::milliseconds(timeoutMs);

  while (true)
  {
    if (Pop(transaction))
    {
      return true;
    }

    const auto elapsed = std::chrono::steady_clock::now() - startTime;
    if (elapsed >= timeoutDuration)
    {
      return false;
    }

    std::this_thread::yield();
  }
}

bool ChunkTransactionRingBuffer::IsEmpty() const
{ return tail.load(std::memory_order_acquire) == head.load(std::memory_order_acquire); }

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
{ return capacity; }

void ChunkTransactionRingBuffer::Clear()
{
  head.store(0, std::memory_order_release);
  tail.store(0, std::memory_order_release);
  ResetStats();
}

RingBufferStats ChunkTransactionRingBuffer::GetStats() const
{
  RingBufferStats stats;
  stats.totalPushes = totalPushes.load(std::memory_order_acquire);
  stats.totalPops = totalPops.load(std::memory_order_acquire);
  stats.failedPushes = failedPushes.load(std::memory_order_acquire);
  stats.failedPops = failedPops.load(std::memory_order_acquire);
  stats.peakSize = peakSize.load(std::memory_order_acquire);
  stats.overflows = overflows.load(std::memory_order_acquire);

  const uint64_t totalOps = stats.totalPushes + stats.totalPops;
  if (totalOps > 0)
  {
    stats.averageSize = static_cast<double>(peakSize.load(std::memory_order_acquire)) /
                        static_cast<double>(totalOps);
  }
  else
  {
    stats.averageSize = 0.0;
  }

  return stats;
}

void ChunkTransactionRingBuffer::ResetStats()
{
  totalPushes.store(0, std::memory_order_release);
  totalPops.store(0, std::memory_order_release);
  failedPushes.store(0, std::memory_order_release);
  failedPops.store(0, std::memory_order_release);
  peakSize.store(0, std::memory_order_release);
  overflows.store(0, std::memory_order_release);
}

bool ChunkTransactionRingBuffer::IsHealthy() const
{
  const RingBufferStats stats = GetStats();

  // Check for excessive overflows (more than 1% of total pushes)
  if (stats.totalPushes > 0 &&
      static_cast<double>(stats.overflows) / static_cast<double>(stats.totalPushes) >
          0.01)
  {
    return false;
  }

  // Check for excessive failed operations (more than 5% of total operations)
  const uint64_t totalOps = stats.totalPushes + stats.totalPops;
  if (totalOps > 0)
  {
    const uint64_t totalFailed = stats.failedPushes + stats.failedPops;
    if (static_cast<double>(totalFailed) / static_cast<double>(totalOps) > 0.05)
    {
      return false;
    }
  }

  // Check if buffer is consistently near capacity
  if (stats.peakSize > 0 &&
      static_cast<double>(Size()) / static_cast<double>(capacity) > 0.9)
  {
    return false;
  }

  return true;
}

void ChunkTransactionRingBuffer::UpdatePeakSize(uint32_t currentSize)
{
  uint64_t currentPeak = peakSize.load(std::memory_order_acquire);
  while (currentSize > currentPeak)
  {
    if (peakSize.compare_exchange_weak(currentPeak, currentSize,
            std::memory_order_release, std::memory_order_acquire))
    {
      break;
    }
  }
}

bool ChunkTransactionRingBuffer::IsValidCapacity(uint32_t capacity)
{
  if (capacity == 0)
    return false;

  // Check if capacity is a power of 2
  return (capacity & (capacity - 1)) == 0;
}

} // namespace Rl::World::Chunk
