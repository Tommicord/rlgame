import <gtest/gtest.h>;

import Rl.World.Chunk.ChunkInRenderUnits;
import Rl.World.Chunk.UnitChunkBuffer;
import Rl.World.Chunk.ChunkTransaction;

import <stdexcept>;

using namespace Rl::World::Chunk;

TEST(ChunkInRenderUnitsTest, ConstructorRejectsNonPositiveRenderDistance)
{
  EXPECT_THROW(ChunkInRenderUnits invalid(0, 1, 1), std::invalid_argument);
  EXPECT_THROW(ChunkInRenderUnits invalid(1, 0, 1), std::invalid_argument);
  EXPECT_THROW(ChunkInRenderUnits invalid(1, 1, 0), std::invalid_argument);
}

TEST(ChunkInRenderUnitsTest, AddRemoveAndGetChunkBufferWorkAsExpected)
{
  ChunkInRenderUnits system(1, 1, 1);
  ASSERT_TRUE(system.Initialize());

  UnitChunkBuffer chunkBuffer;
  chunkBuffer.GetRaw()[0] = 9;

  WorldChunkCoord coord{0, 0, 0};
  EXPECT_TRUE(system.AddChunk(coord, chunkBuffer));

  UnitChunkBuffer* fetched = system.GetChunkBuffer(coord);
  ASSERT_NE(fetched, nullptr);
  EXPECT_EQ(fetched->GetRaw()[0], 9);

  EXPECT_TRUE(system.RemoveChunk(coord));
  EXPECT_EQ(system.GetChunkBuffer(coord), nullptr);

  EXPECT_TRUE(system.Shutdown());
}

TEST(ChunkInRenderUnitsTest, AddChunkFailsOutsideRenderDistance)
{
  ChunkInRenderUnits system(1, 1, 1);
  ASSERT_TRUE(system.Initialize());

  UnitChunkBuffer chunkBuffer;
  EXPECT_FALSE(system.AddChunk(WorldChunkCoord{1, 0, 0}, chunkBuffer));
  EXPECT_TRUE(system.Shutdown());
}

TEST(ChunkInRenderUnitsTest, AddChunkCannotOverwriteExistingChunk)
{
  ChunkInRenderUnits system(1, 1, 1);
  ASSERT_TRUE(system.Initialize());

  UnitChunkBuffer firstChunk;
  ASSERT_TRUE(system.AddChunk(WorldChunkCoord{0, 0, 0}, firstChunk));

  UnitChunkBuffer secondChunk;
  EXPECT_FALSE(system.AddChunk(WorldChunkCoord{0, 0, 0}, secondChunk));
  EXPECT_TRUE(system.Shutdown());
}

TEST(ChunkInRenderUnitsTest, ReadAndWriteRejectInvalidLocalCoordinates)
{
  ChunkInRenderUnits system(1, 1, 1);
  ASSERT_TRUE(system.Initialize());

  UnitChunkBuffer chunkBuffer;
  ASSERT_TRUE(system.AddChunk(WorldChunkCoord{0, 0, 0}, chunkBuffer));

  const TransactionResult invalidRead = system.ReadUnitId(WorldChunkCoord{0, 0, 0}, UnitChunkBuffer::W, 0, 0);
  EXPECT_FALSE(invalidRead.success);

  const TransactionResult invalidWrite = system.WriteUnitId(WorldChunkCoord{0, 0, 0}, 0, 0, UnitChunkBuffer::D, 5);
  EXPECT_FALSE(invalidWrite.success);

  EXPECT_TRUE(system.Shutdown());
}

TEST(ChunkInRenderUnitsTest, WriteAndProcessTransactionsUpdateChunkData)
{
  ChunkInRenderUnits system(1, 1, 1);
  ASSERT_TRUE(system.Initialize());

  UnitChunkBuffer chunkBuffer;
  chunkBuffer.GetRaw()[0] = 1;

  WorldChunkCoord coord{0, 0, 0};
  ASSERT_TRUE(system.AddChunk(coord, chunkBuffer));

  const TransactionResult writeResult = system.WriteUnitId(coord, 0, 0, 0, 7);
  EXPECT_TRUE(writeResult.success);
  EXPECT_EQ(writeResult.readUnitId, 7u);

  EXPECT_TRUE(system.ProcessTransactions());

  const TransactionResult readResult = system.ReadUnitId(coord, 0, 0, 0);
  EXPECT_TRUE(readResult.success);
  EXPECT_EQ(readResult.readUnitId, 7u);

  EXPECT_TRUE(system.Shutdown());
}
