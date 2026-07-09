import Rl.World.Chunk.UnitChunkAccessor;
import Rl.World.Chunk.ChunkInRenderUnits;
import Rl.World.Chunk.UnitChunkBuffer;
import Rl.World.Chunk.ChunkTransaction;

import <gtest/gtest.h>;
import <cstdint>;

using namespace Rl::World::Chunk;

TEST(UnitChunkAccessorTest, RelativeOffsetHelpersProduceExpectedPositions)
{
  const UnitPosition base{4, 5, 6};

  const UnitPosition above = RelativeOffset::Above().ToAbsolute(base);
  const UnitPosition below = RelativeOffset::Below().ToAbsolute(base);
  const UnitPosition northEast = RelativeOffset::NorthEast().ToAbsolute(base);

  EXPECT_EQ(above.worldX, 4);
  EXPECT_EQ(above.worldY, 6);
  EXPECT_EQ(above.worldZ, 6);
  EXPECT_EQ(below.worldY, 4);
  EXPECT_EQ(northEast.worldX, 5);
  EXPECT_EQ(northEast.worldZ, 5);
}

TEST(UnitChunkAccessorTest, AccessorTracksPositionAndConvertsToChunkLocal)
{
  ChunkInRenderUnits system(1, 1, 1);
  ASSERT_TRUE(system.Initialize());

  UnitChunkBuffer chunkBuffer{};
  chunkBuffer.GetRaw()[0] = 3;
  ASSERT_TRUE(system.AddChunk(WorldChunkCoord{0, 0, 0}, chunkBuffer));

  UnitChunkAccessor accessor(system, UnitPosition{0, 0, 0});
  EXPECT_EQ(accessor.GetPosition().worldX, 0);
  accessor.UpdatePosition(UnitPosition{5, 10, 2});
  EXPECT_EQ(accessor.GetPosition().worldY, 10);

  const int targetIndex = IndexMap3d2<UnitChunkBuffer::W, UnitChunkBuffer::H>(5, 10, 2);
  // Modify the chunk already added to the system
  UnitChunkBuffer* existing = system.GetChunkBuffer(WorldChunkCoord{0, 0, 0});
  ASSERT_NE(existing, nullptr);
  existing->GetRaw()[targetIndex] = 3;

  const TransactionResult result = accessor.ReadCurrent();
  EXPECT_TRUE(result.success) << result.GetErrorMessage() << " (code="
      << static_cast<int>(result.validationCode) << ")";
  EXPECT_EQ(result.readUnitId, 3u);

  EXPECT_TRUE(system.Shutdown());
}

TEST(UnitChunkAccessorTest, AccessorReportsGroundAndEmptySpaceBasedOnNeighborBlocks)
{
  ChunkInRenderUnits system(1, 1, 1);
  ASSERT_TRUE(system.Initialize());

  UnitChunkBuffer chunkBuffer;
  const int currentIndex = IndexMap3d2<UnitChunkBuffer::W, UnitChunkBuffer::H>(0, 0, 0);
  const int aboveIndex = IndexMap3d2<UnitChunkBuffer::W, UnitChunkBuffer::H>(0, 1, 0);
  chunkBuffer.GetRaw()[currentIndex] = 0;
  chunkBuffer.GetRaw()[aboveIndex] = 0;
  ASSERT_TRUE(system.AddChunk(WorldChunkCoord{0, 0, 0}, chunkBuffer));

  UnitChunkAccessor accessor(system, UnitPosition{0, 0, 0});
  EXPECT_FALSE(accessor.IsOnGround());
  EXPECT_TRUE(accessor.IsSpaceAboveEmpty());

  const TransactionResult writeResult = accessor.WriteAbove(4);
  EXPECT_TRUE(writeResult.success);
  EXPECT_TRUE(accessor.IsSpaceAboveEmpty());

  EXPECT_TRUE(system.Shutdown());
}

TEST(UnitChunkAccessorTest, WriteBelowUpdatesGroundStateAndBelowValue)
{
  ChunkInRenderUnits system(1, 1, 1);
  ASSERT_TRUE(system.Initialize());

  UnitChunkBuffer chunkBuffer;
  ASSERT_TRUE(system.AddChunk(WorldChunkCoord{0, 0, 0}, chunkBuffer));

  UnitChunkAccessor accessor(system, UnitPosition{0, 1, 0});
  EXPECT_FALSE(accessor.IsOnGround());

  const TransactionResult writeResult = accessor.WriteBelow(8);
  EXPECT_TRUE(writeResult.success);
  EXPECT_TRUE(system.ProcessTransactions());

  const TransactionResult belowResult = accessor.ReadBelow();
  EXPECT_TRUE(belowResult.success);
  EXPECT_EQ(belowResult.readUnitId, 8u);
  EXPECT_TRUE(accessor.IsOnGround());

  EXPECT_TRUE(system.Shutdown());
}

TEST(UnitChunkAccessorTest, InvalidRelativePositionIsRejected)
{
  ChunkInRenderUnits system(1, 1, 1);
  ASSERT_TRUE(system.Initialize());

  UnitChunkBuffer chunkBuffer;
  ASSERT_TRUE(system.AddChunk(WorldChunkCoord{0, 0, 0}, chunkBuffer));

  UnitChunkAccessor accessor(system, UnitPosition{0, 0, 0});
  const TransactionResult result = accessor.WriteBelow(7);

  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.validationCode, ValidationResult::INVALID_COORDINATES);
  EXPECT_TRUE(system.Shutdown());
}

TEST(UnitChunkAccessorTest, GetAbsolutePositionAppliesRelativeOffsets)
{
  ChunkInRenderUnits system(1, 1, 1);
  ASSERT_TRUE(system.Initialize());

  UnitChunkBuffer chunkBuffer;
  ASSERT_TRUE(system.AddChunk(WorldChunkCoord{0, 0, 0}, chunkBuffer));

  UnitChunkAccessor accessor(system, UnitPosition{2, 3, 4});
  const UnitPosition position = accessor.GetAbsolutePosition(RelativeOffset::NorthEast());

  UnitPosition pos = {3, 3, 3};
  EXPECT_EQ(position, pos);
  EXPECT_TRUE(system.Shutdown());
}
