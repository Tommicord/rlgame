export module Rl.RayLog.ThreadPool;

import <thread>;
import <vector>;
import <functional>;
import <atomic>;
import <condition_variable>;
import <queue>;

namespace Rl::RayLog
{

export class RayLogThreadPool
{
  std::vector<std::thread> workers;
  std::queue<std::function<void()>> tasks;
  std::mutex queueMutex;
  std::condition_variable condition;
  std::atomic<bool> stop{false};

  public:
  explicit RayLogThreadPool(size_t threads)
  {
    for (size_t i = 0; i < threads; ++i)
    {
      workers.emplace_back([this]
      {
        while (true)
        {
          std::function<void()> task;
          {
            std::unique_lock lock(queueMutex);
            condition.wait(lock, [this] { return stop.load() || !tasks.empty(); });
            
            if (stop.load() && tasks.empty())
              return;

            task = std::move(tasks.front());
            tasks.pop();
          }
          task();
        }
      });
    }
  }

  ~RayLogThreadPool()
  {
    stop.store(true);
    condition.notify_all();
    for (auto& worker : workers)
      worker.join();
  }

  void Enqueue(std::function<void()> task)
  {
    {
      std::scoped_lock lock(queueMutex);
      tasks.push(std::move(task));
    }
    condition.notify_one();
  }

  [[nodiscard]]
  size_t WorkerCount() const
  {
    return workers.size();
  }
};

}
