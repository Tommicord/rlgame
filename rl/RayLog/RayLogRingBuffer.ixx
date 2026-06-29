export module Rl.RayLog.RingBuffer;

import <vector>;
import <mutex>;
import <cstddef>;

namespace Rl::RayLog
{

export template<typename T>
class RayLogRingBuffer
{
  std::vector<T> buffer;
  size_t head{0};
  size_t tail{0};
  mutable std::mutex mutex;
  size_t capacity;

  public:
  explicit RayLogRingBuffer(size_t size) : buffer(size), capacity(size) {}

  bool Push(const T& item)
  {
    std::scoped_lock lock(mutex);
    const size_t nextHead = (head + 1) % capacity;
    
    if (nextHead == tail)
      return false;

    buffer[head] = item;
    head = nextHead;
    return true;
  }

  [[nodiscard]]
  bool Pop(T& item)
  {
    std::scoped_lock lock(mutex);
    if (tail == head)
      return false;
    item = buffer[tail];
    tail = (tail + 1) % capacity;
    return true;
  }

  [[nodiscard]]
  size_t Size() const
  {
    std::scoped_lock lock(mutex);
    if (head >= tail)
      return head - tail;
    return capacity - tail + head;
  }

  [[nodiscard]]
  bool IsEmpty() const
  {
    std::scoped_lock lock(mutex);
    return head == tail;
  }

  [[nodiscard]]
  bool IsFull() const
  {
    std::scoped_lock lock(mutex);
    return (head + 1) % capacity == tail;
  }
};

}
