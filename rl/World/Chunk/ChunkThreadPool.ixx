export module Rl.World.Chunk.ChunkThreadPool;

import <functional>;
import <vector>;
import <thread>;
import <mutex>;
import <condition_variable>;
import <queue>;
import <atomic>;
import <future>;

namespace Rl::World::Chunk
{

/* Thread pool for concurrent chunk operations */
export class ChunkThreadPool
{
  public:
  /* Constructor with specified number of threads */
  explicit ChunkThreadPool(size_t numThreads = std::thread::hardware_concurrency());

  /* Destructor - waits for all tasks to complete */
  ~ChunkThreadPool();

  /* Disable copy operations */
  ChunkThreadPool(const ChunkThreadPool&) = delete;
  ChunkThreadPool& operator=(const ChunkThreadPool&) = delete;

  /* Enable move operations */
  ChunkThreadPool(ChunkThreadPool&& other) noexcept;
  ChunkThreadPool& operator=(ChunkThreadPool&& other) noexcept;

  /* Submit a task to the thread pool */
  template <typename F, typename... Args>
  auto Submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>
  {
    using ReturnType = std::invoke_result_t<F, Args...>;

    auto task = std::make_shared<std::packaged_task<ReturnType()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    std::future<ReturnType> result = task->get_future();

    {
      std::unique_lock<std::mutex> lock(queueMutex);
      if (stop)
      {
        throw std::runtime_error("Cannot submit task to stopped thread pool");
      }
      tasks.emplace([task]() { (*task)(); });
    }

    condition.notify_one();
    return result;
  }

  /* Wait for all tasks to complete */
  void WaitForAll();

  /* Get the number of worker threads */
  [[nodiscard]]
  size_t GetThreadCount() const;

  /* Get the number of pending tasks */
  [[nodiscard]]
  size_t GetPendingTaskCount() const;

  /* Stop the thread pool */
  void Stop();

  private:
  /* Worker thread function */
  void WorkerThread();

  std::vector<std::thread>          workers;
  std::queue<std::function<void()>> tasks;

  mutable std::mutex      queueMutex;
  std::condition_variable condition;
  std::atomic<bool>       stop;
  std::atomic<size_t>     activeTasks;
};

} // namespace Rl::World::Chunk
