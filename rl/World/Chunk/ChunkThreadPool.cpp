import Rl.World.Chunk.ChunkThreadPool;

import <stdexcept>;

namespace Rl::World::Chunk
{

ChunkThreadPool::ChunkThreadPool(size_t numThreads) 
    : stop(false), activeTasks(0)
{
  for (size_t i = 0; i < numThreads; ++i)
  {
    workers.emplace_back([this] { WorkerThread(); });
  }
}

ChunkThreadPool::~ChunkThreadPool()
{
  Stop();
}

ChunkThreadPool::ChunkThreadPool(ChunkThreadPool&& other) noexcept
    : workers(std::move(other.workers)),
      tasks(std::move(other.tasks)),
      stop(other.stop.load()),
      activeTasks(other.activeTasks.load())
{
  other.stop.store(true);
  other.activeTasks.store(0);
}

ChunkThreadPool& ChunkThreadPool::operator=(ChunkThreadPool&& other) noexcept
{
  if (this != &other)
  {
    Stop();
    
    workers = std::move(other.workers);
    tasks = std::move(other.tasks);
    stop.store(other.stop.load());
    activeTasks.store(other.activeTasks.load());
    
    other.stop.store(true);
    other.activeTasks.store(0);
  }
  return *this;
}

void ChunkThreadPool::WorkerThread()
{
  while (true)
  {
    std::function<void()> task;
    
    {
      std::unique_lock<std::mutex> lock(queueMutex);
      
      condition.wait(lock, [this] 
      { 
        return stop.load() || !tasks.empty(); 
      });
      
      if (stop.load() && tasks.empty())
        return;
      
      task = std::move(tasks.front());
      tasks.pop();
    }
    
    activeTasks.fetch_add(1, std::memory_order_release);
    task();
    activeTasks.fetch_sub(1, std::memory_order_release);
  }
}

void ChunkThreadPool::WaitForAll()
{
  while (activeTasks.load(std::memory_order_acquire) > 0)
  {
    std::this_thread::yield();
  }
}

size_t ChunkThreadPool::GetThreadCount() const
{
  return workers.size();
}

size_t ChunkThreadPool::GetPendingTaskCount() const
{
  std::unique_lock<std::mutex> lock(queueMutex);
  return tasks.size();
}

void ChunkThreadPool::Stop()
{
  {
    std::unique_lock<std::mutex> lock(queueMutex);
    stop.store(true, std::memory_order_release);
  }
  
  condition.notify_all();
  
  for (std::thread& worker : workers)
  {
    if (worker.joinable())
      worker.join();
  }
  
  workers.clear();
}

} // namespace Rl::World::Chunk
