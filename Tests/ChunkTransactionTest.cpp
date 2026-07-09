import <gtest/gtest.h>;

import Rl.World.Chunk.ChunkTransaction;

using namespace Rl::World::Chunk;

TEST(ChunkTransactionTest, DefaultTransactionStartsPendingAndIncomplete)
{
  ChunkTransaction transaction;

  EXPECT_EQ(transaction.type, TransactionType::READ);
  EXPECT_EQ(transaction.state, TransactionState::PENDING);
  EXPECT_FALSE(transaction.completed.load());
  EXPECT_TRUE(transaction.IsValid());
}

TEST(ChunkTransactionTest, ValidationRejectsInvalidCoordinatesAndTypes)
{
  ChunkTransaction transaction;
  transaction.type = TransactionType::WRITE;
  transaction.localX = 64;

  EXPECT_EQ(transaction.Validate(64, 128, 64), ValidationResult::INVALID_COORDINATES);

  transaction.localX = 1;
  transaction.localY = 1;
  transaction.localZ = 1;
  transaction.type = TransactionType::INVALID;
  EXPECT_EQ(transaction.Validate(64, 128, 64), ValidationResult::INVALID_TYPE);
}

TEST(ChunkTransactionTest, RollbackCopySwapsOldAndNewUnitIds)
{
  ChunkTransaction transaction;
  transaction.oldUnitId = 1;
  transaction.newUnitId = 2;
  transaction.sequenceNumber = 7;

  const ChunkTransaction rollback = transaction.CreateRollbackCopy();

  EXPECT_EQ(rollback.oldUnitId, 2u);
  EXPECT_EQ(rollback.newUnitId, 1u);
  EXPECT_EQ(rollback.type, TransactionType::WRITE);
  EXPECT_EQ(rollback.sequenceNumber, 1000007u);
}

TEST(ChunkTransactionTest, CompleteMarksTransactionCompletedAndInvalid)
{
  ChunkTransaction transaction;
  transaction.Complete();

  EXPECT_EQ(transaction.state, TransactionState::COMPLETED);
  EXPECT_TRUE(transaction.completed.load());
  EXPECT_FALSE(transaction.IsValid());
}

TEST(ChunkTransactionTest, FailMarksTransactionFailedAndInvalid)
{
  ChunkTransaction transaction;
  transaction.Fail();

  EXPECT_EQ(transaction.state, TransactionState::FAILED);
  EXPECT_TRUE(transaction.completed.load());
  EXPECT_FALSE(transaction.IsValid());
}

TEST(ChunkTransactionTest, ResetRestoresInitialTransactionState)
{
  ChunkTransaction transaction;
  transaction.newUnitId = 3;
  transaction.type = TransactionType::WRITE;
  transaction.state = TransactionState::IN_PROGRESS;
  transaction.completed.store(true, std::memory_order_release);

  transaction.Reset();

  EXPECT_EQ(transaction.type, TransactionType::READ);
  EXPECT_EQ(transaction.state, TransactionState::PENDING);
  EXPECT_FALSE(transaction.completed.load());
  EXPECT_EQ(transaction.oldUnitId, 0u);
  EXPECT_EQ(transaction.newUnitId, 0u);
}
