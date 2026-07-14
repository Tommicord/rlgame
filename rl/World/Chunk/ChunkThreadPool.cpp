import Rl.World.Chunk.ChunkThreadPool;

import <mutex>;
import <stdexcept>;

namespace Rl::World::Chunk
{

ChunkThreadPool::ChunkThreadPool(size_t numThreads) : stop(false), activeTasks(0)
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

ChunkThreadPool::ChunkThreadPool(ChunkThreadPool&& other) noexcept :
    workers(std::move(other.workers)), tasks(std::move(other.tasks)),
    stop(other.stop.load()), activeTasks(other.activeTasks.load())
{
    other.stop.store(true);
    other.activeTasks.store(0);
}

ChunkThreadPool& ChunkThreadPool::operator=(ChunkThreadPool&& other) noexcept
{
    if (this != &other)
    {
        Stop();
        std::lock(queueMutex, other.queueMutex);
        std::lock_guard lockThis(queueMutex, std::adopt_lock);
        std::lock_guard lockOther(other.queueMutex, std::adopt_lock);
        workers = std::move(other.workers);
        tasks   = std::move(other.tasks);
        stop.store(other.stop.load(std::memory_order_acquire), std::memory_order_release);
        activeTasks.store(other.activeTasks.load(std::memory_order_acquire),
                          std::memory_order_release);

        other.stop.store(true, std::memory_order_release);
        other.activeTasks.store(0, std::memory_order_release);
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

            condition.wait(lock, [this] { return stop.load() || !tasks.empty(); });

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
