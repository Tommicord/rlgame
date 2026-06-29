export module Rl.RayLog.ThreadPool;

import <thread>;
import <vector>;
import <functional>;
import <atomic>;
import <condition_variable>;
#include <future>
import <queue>;

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif

namespace Rl::RayLog
{

export class RayLogThreadPool
{
  std::vector<std::jthread>         workers;
  std::queue<std::function<void()>> tasks;
  std::mutex                        queueMutex;
  std::condition_variable           condition;
  std::atomic<bool>                 stop{false};

  public:
  /* Creates a thread pool with fixed size */
  explicit RayLogThreadPool(const size_t threads)
  {
    for (size_t i = 0; i < threads; ++i)
    {
      workers.emplace_back(
          [this]
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

  /* Joins all logging thread pool workers */
  ~RayLogThreadPool()
  {
    stop.store(true);
    condition.notify_all();
    for (auto& worker : workers)
    {
      auto future = std::async(std::launch::async, &std::thread::join, &worker);
      if (future.wait_for(std::chrono::seconds(4)) == std::future_status::timeout)
      {
#if defined(_WIN32) || defined(_WIN64)
        TerminateThread(worker.native_handle(), 1);
#else
        pthread_cancel(worker.native_handle());
#endif
      }
    }
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

} // namespace Rl::RayLog
