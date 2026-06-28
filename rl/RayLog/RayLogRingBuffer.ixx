export module Rl.RayLog.RingBuffer;

import <atomic>;
import <vector>;
import <mutex>;

namespace Rl::RayLog
{

template<typename T>
class RayLogRingBuffer
{
  std::vector<T> buffer;
  std::atomic<size_t> head{0};
  std::atomic<size_t> tail{0};
  std::mutex mutex;
  size_t capacity;

  public:
  explicit RayLogRingBuffer(size_t size) : buffer(size), capacity(size) {}

  [[nodiscard]]
  bool Push(const T& item)
  {
    std::scoped_lock lock(mutex);
    const size_t nextHead = (head.load() + 1) % capacity;
    
    if (nextHead == tail.load())
      return false;

    buffer[head.load()] = item;
    head.store(nextHead);
    return true;
  }

  [[nodiscard]]
  bool Pop(T& item)
  {
    std::scoped_lock lock(mutex);
    if (tail.load() == head.load())
      return false;

    item = buffer[tail.load()];
    tail.store((tail.load() + 1) % capacity);
    return true;
  }

  [[nodiscard]]
  size_t Size() const
  {
    if (head.load() >= tail.load())
      return head.load() - tail.load();
    return capacity - tail.load() + head.load();
  }

  [[nodiscard]]
  bool IsEmpty() const
  {
    return head.load() == tail.load();
  }

  [[nodiscard]]
  bool IsFull() const
  {
    return (head.load() + 1) % capacity == tail.load();
  }
};

}
