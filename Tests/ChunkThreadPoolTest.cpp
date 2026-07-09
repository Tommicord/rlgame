import <chrono>;
import <stdexcept>;
import <thread>;
import <gtest/gtest.h>;

import Rl.World.Chunk.ChunkThreadPool;

using namespace Rl::World::Chunk;

TEST(ChunkThreadPoolTest, ConstructorCreatesWorkerThreadsAndStartsEmpty)
{
  ChunkThreadPool pool(2);

  EXPECT_GE(pool.GetThreadCount(), 1u);
  EXPECT_EQ(pool.GetPendingTaskCount(), 0u);
}

TEST(ChunkThreadPoolTest, SubmitAndWaitForWorkCompletesSuccessfully)
{
  ChunkThreadPool pool(2);

  auto future = pool.Submit([]() -> int {
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    return 17;
  });

  EXPECT_EQ(future.get(), 17);
  EXPECT_EQ(pool.GetPendingTaskCount(), 0u);
}

TEST(ChunkThreadPoolTest, StoppingPoolRejectsFurtherSubmission)
{
  ChunkThreadPool pool(1);
  pool.Stop();

  EXPECT_THROW(pool.Submit([]() { return 1; }), std::runtime_error);
}
