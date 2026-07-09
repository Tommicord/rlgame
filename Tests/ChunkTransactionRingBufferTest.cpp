import <gtest/gtest.h>;

import Rl.World.Chunk.TransactionRingBuffer;
import Rl.World.Chunk.ChunkTransaction;

using namespace Rl::World::Chunk;

TEST(ChunkTransactionRingBufferTest, PushPopAndSizeReflectBufferState)
{
  ChunkTransactionRingBuffer buffer(8);

  ChunkTransaction first;
  first.newUnitId = 10;
  first.sequenceNumber = 1;
  EXPECT_TRUE(buffer.Push(std::move(first)));

  ChunkTransaction second;
  second.newUnitId = 20;
  second.sequenceNumber = 2;
  EXPECT_TRUE(buffer.Push(std::move(second)));

  EXPECT_EQ(buffer.Size(), 2u);
  EXPECT_FALSE(buffer.IsEmpty());

  ChunkTransaction popped;
  EXPECT_TRUE(buffer.Pop(popped));
  EXPECT_EQ(popped.newUnitId, 10u);
  EXPECT_EQ(popped.sequenceNumber, 1u);

  EXPECT_TRUE(buffer.Pop(popped));
  EXPECT_EQ(popped.newUnitId, 20u);
  EXPECT_EQ(popped.sequenceNumber, 2u);
  EXPECT_TRUE(buffer.IsEmpty());
}

TEST(ChunkTransactionRingBufferTest, FullBufferReportsFailuresAndStats)
{
  ChunkTransactionRingBuffer buffer(2);

  ChunkTransaction one;
  one.sequenceNumber = 1;
  ChunkTransaction two;
  two.sequenceNumber = 2;
  ChunkTransaction three;
  three.sequenceNumber = 3;

  EXPECT_TRUE(buffer.Push(std::move(one)));
  EXPECT_TRUE(buffer.Push(std::move(two)));
  EXPECT_FALSE(buffer.Push(std::move(three)));

  const RingBufferStats stats = buffer.GetStats();
  EXPECT_EQ(stats.totalPushes, 2u);
  EXPECT_EQ(stats.failedPushes, 1u);
  EXPECT_EQ(stats.overflows, 1u);
}

TEST(ChunkTransactionRingBufferTest, ClearAndHealthChecksResetState)
{
  ChunkTransactionRingBuffer buffer(4);
  ChunkTransaction transaction;
  transaction.sequenceNumber = 7;
  EXPECT_TRUE(buffer.Push(std::move(transaction)));

  buffer.Clear();
  EXPECT_TRUE(buffer.IsEmpty());
  EXPECT_EQ(buffer.Size(), 0u);
  EXPECT_EQ(buffer.GetStats().totalPushes, 0u);
  EXPECT_TRUE(buffer.IsHealthy());
}

TEST(ChunkTransactionRingBufferTest, IsFullReportsFullWhenCapacityReached)
{
  ChunkTransactionRingBuffer buffer(2);

  ChunkTransaction one;
  one.sequenceNumber = 1;
  ChunkTransaction two;
  two.sequenceNumber = 2;

  EXPECT_TRUE(buffer.Push(std::move(one)));
  EXPECT_TRUE(buffer.Push(std::move(two)));
  EXPECT_TRUE(buffer.IsFull());
}

TEST(ChunkTransactionRingBufferTest, PopWithTimeoutReturnsFalseForEmptyBuffer)
{
  ChunkTransactionRingBuffer buffer(4);
  ChunkTransaction transaction;

  EXPECT_FALSE(buffer.PopWithTimeout(transaction, 5u));
  EXPECT_EQ(buffer.GetStats().failedPops, 1u);
}

TEST(ChunkTransactionRingBufferTest, PushWithTimeoutReturnsFalseWhenBufferStaysFull)
{
  ChunkTransactionRingBuffer buffer(2);
  ChunkTransaction one;
  one.sequenceNumber = 1;
  ChunkTransaction two;
  two.sequenceNumber = 2;
  ChunkTransaction three;
  three.sequenceNumber = 3;

  EXPECT_TRUE(buffer.Push(std::move(one)));
  EXPECT_TRUE(buffer.Push(std::move(two)));
  EXPECT_FALSE(buffer.PushWithTimeout(std::move(three), 5u));
}
