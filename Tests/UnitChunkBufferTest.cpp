#include <gtest/gtest.h>
#include "rl/World/Chunk/UnitChunkBuffer.h"

using namespace Rl::World::Chunk;

class UnitChunkBufferTest : public ::testing::Test
{
  protected:
  void SetUp() override
  {
    chunkBuffer = new UnitChunkBuffer();
  }

  void TearDown() override
  {
    delete chunkBuffer;
  }

  UnitChunkBuffer* chunkBuffer;
};

// Test constructor and basic initialization
TEST_F(UnitChunkBufferTest, ConstructorInitializesBuffer)
{
  ASSERT_NE(chunkBuffer, nullptr);
  EXPECT_TRUE(chunkBuffer->IsValid());
  EXPECT_EQ(chunkBuffer->GetTotalBlocks(), UnitChunkBuffer::W * UnitChunkBuffer::H * UnitChunkBuffer::D);
}

// Test destructor
TEST_F(UnitChunkBufferTest, DestructorCleansUp)
{
  UnitChunkBuffer* tempBuffer = new UnitChunkBuffer();
  delete tempBuffer;
  // If we reach here without crash, destructor worked
  SUCCEED();
}

// Test copy constructor
TEST_F(UnitChunkBufferTest, CopyConstructorCreatesIndependentCopy)
{
  // Set a value in the original buffer
  int* raw = chunkBuffer->GetRaw();
  raw[0] = 42;

  UnitChunkBuffer copy(*chunkBuffer);

  // Verify copy has its own buffer
  EXPECT_NE(copy.GetRaw(), chunkBuffer->GetRaw());
  EXPECT_TRUE(copy.IsValid());
  EXPECT_EQ(copy.GetRaw()[0], 42);
}

// Test move constructor
TEST_F(UnitChunkBufferTest, MoveConstructorTransfersOwnership)
{
  int* originalRaw = chunkBuffer->GetRaw();
  UnitChunkBuffer moved(std::move(*chunkBuffer));

  EXPECT_EQ(moved.GetRaw(), originalRaw);
  EXPECT_TRUE(moved.IsValid());
  EXPECT_FALSE(chunkBuffer->IsValid());
}

// Test copy assignment
TEST_F(UnitChunkBufferTest, CopyAssignmentCreatesIndependentCopy)
{
  UnitChunkBuffer other;
  int* raw = chunkBuffer->GetRaw();
  raw[0] = 123;

  other = *chunkBuffer;

  EXPECT_NE(other.GetRaw(), chunkBuffer->GetRaw());
  EXPECT_TRUE(other.IsValid());
  EXPECT_EQ(other.GetRaw()[0], 123);
}

// Test move assignment
TEST_F(UnitChunkBufferTest, MoveAssignmentTransfersOwnership)
{
  UnitChunkBuffer other;
  int* originalRaw = chunkBuffer->GetRaw();

  other = std::move(*chunkBuffer);

  EXPECT_EQ(other.GetRaw(), originalRaw);
  EXPECT_TRUE(other.IsValid());
  EXPECT_FALSE(chunkBuffer->IsValid());
}

// Test self-assignment
TEST_F(UnitChunkBufferTest, SelfAssignmentCopy)
{
  int* raw = chunkBuffer->GetRaw();
  raw[0] = 99;

  *chunkBuffer = *chunkBuffer;

  EXPECT_TRUE(chunkBuffer->IsValid());
  EXPECT_EQ(chunkBuffer->GetRaw()[0], 99);
}

// Test self-assignment move
TEST_F(UnitChunkBufferTest, SelfAssignmentMove)
{
  int* raw = chunkBuffer->GetRaw();
  raw[0] = 88;

  *chunkBuffer = std::move(*chunkBuffer);

  EXPECT_TRUE(chunkBuffer->IsValid());
  EXPECT_EQ(chunkBuffer->GetRaw()[0], 88);
}

// Test Clear function
TEST_F(UnitChunkBufferTest, ClearZerosBuffer)
{
  int* raw = chunkBuffer->GetRaw();
  for (int i = 0; i < chunkBuffer->GetTotalBlocks(); i++)
  {
    raw[i] = 42;
  }

  chunkBuffer->Clear();

  for (int i = 0; i < chunkBuffer->GetTotalBlocks(); i++)
  {
    EXPECT_EQ(raw[i], 0);
  }
}

// Test GetUnitIdXYZ with valid coordinates
TEST_F(UnitChunkBufferTest, GetUnitIdXYZValidCoordinates)
{
  int* raw = chunkBuffer->GetRaw();
  raw[0] = 5;
  raw[1] = 10;

  auto id0 = chunkBuffer->GetUnitIdXYZ(0, 0, 0);
  auto id1 = chunkBuffer->GetUnitIdXYZ(1, 0, 0);

  ASSERT_TRUE(id0.has_value());
  ASSERT_TRUE(id1.has_value());
  EXPECT_EQ(id0.value(), 5);
  EXPECT_EQ(id1.value(), 10);
}

// Test GetUnitIdXYZ with invalid coordinates
TEST_F(UnitChunkBufferTest, GetUnitIdXYZInvalidCoordinates)
{
  auto idNegative = chunkBuffer->GetUnitIdXYZ(-1, 0, 0);
  auto idTooLarge = chunkBuffer->GetUnitIdXYZ(UnitChunkBuffer::W, 0, 0);

  EXPECT_FALSE(idNegative.has_value());
  EXPECT_FALSE(idTooLarge.has_value());
}

// Test GetUnitId with ChunkCoord
TEST_F(UnitChunkBufferTest, GetUnitIdWithChunkCoord)
{
  int* raw = chunkBuffer->GetRaw();
  raw[0] = 7;

  UnitChunkBuffer::ChunkCoord coord{0, 0, 0};
  auto id = chunkBuffer->GetUnitId(coord);

  ASSERT_TRUE(id.has_value());
  EXPECT_EQ(id.value(), 7);
}

// Test IsInBounds with valid coordinates
TEST_F(UnitChunkBufferTest, IsInBoundsValidCoordinates)
{
  EXPECT_TRUE(chunkBuffer->IsInBounds(0, 0, 0));
  EXPECT_TRUE(chunkBuffer->IsInBounds(UnitChunkBuffer::W - 1, UnitChunkBuffer::H - 1, UnitChunkBuffer::D - 1));
  EXPECT_TRUE(chunkBuffer->IsInBounds(32, 64, 32));
}

// Test IsInBounds with invalid coordinates
TEST_F(UnitChunkBufferTest, IsInBoundsInvalidCoordinates)
{
  EXPECT_FALSE(chunkBuffer->IsInBounds(-1, 0, 0));
  EXPECT_FALSE(chunkBuffer->IsInBounds(UnitChunkBuffer::W, 0, 0));
  EXPECT_FALSE(chunkBuffer->IsInBounds(0, -1, 0));
  EXPECT_FALSE(chunkBuffer->IsInBounds(0, UnitChunkBuffer::H, 0));
  EXPECT_FALSE(chunkBuffer->IsInBounds(0, 0, -1));
  EXPECT_FALSE(chunkBuffer->IsInBounds(0, 0, UnitChunkBuffer::D));
}

// Test IsInBounds with ChunkCoord
TEST_F(UnitChunkBufferTest, IsInBoundsWithChunkCoord)
{
  UnitChunkBuffer::ChunkCoord valid{32, 64, 32};
  UnitChunkBuffer::ChunkCoord invalid{-1, 0, 0};

  EXPECT_TRUE(chunkBuffer->IsInBounds(valid));
  EXPECT_FALSE(chunkBuffer->IsInBounds(invalid));
}

// Test GetTotalBlocks
TEST_F(UnitChunkBufferTest, GetTotalBlocksReturnsCorrectValue)
{
  constexpr int expected = UnitChunkBuffer::W * UnitChunkBuffer::H * UnitChunkBuffer::D;
  EXPECT_EQ(chunkBuffer->GetTotalBlocks(), expected);
}

// Test GetRaw
TEST_F(UnitChunkBufferTest, GetRawReturnsValidPointer)
{
  int* raw = chunkBuffer->GetRaw();
  ASSERT_NE(raw, nullptr);
  EXPECT_TRUE(chunkBuffer->IsValid());
}

// Test GetRaw const version
TEST_F(UnitChunkBufferTest, GetRawConstReturnsValidPointer)
{
  const UnitChunkBuffer* constBuffer = chunkBuffer;
  const int* raw = constBuffer->GetRaw();
  ASSERT_NE(raw, nullptr);
}

// Test GetBufferSizeBytes
TEST_F(UnitChunkBufferTest, GetBufferSizeBytesReturnsCorrectValue)
{
  constexpr size_t expected = static_cast<size_t>(UnitChunkBuffer::W * UnitChunkBuffer::H * UnitChunkBuffer::D) * sizeof(int);
  EXPECT_EQ(chunkBuffer->GetBufferSizeBytes(), expected);
}

// Test IsValid
TEST_F(UnitChunkBufferTest, IsValidReturnsTrueForValidBuffer)
{
  EXPECT_TRUE(chunkBuffer->IsValid());
}

// Test IsValid after move
TEST_F(UnitChunkBufferTest, IsValidReturnsFalseAfterMove)
{
  UnitChunkBuffer moved(std::move(*chunkBuffer));
  EXPECT_FALSE(chunkBuffer->IsValid());
  EXPECT_TRUE(moved.IsValid());
}

// Test GetDimensions
TEST_F(UnitChunkBufferTest, GetDimensionsReturnsCorrectDimensions)
{
  UnitChunkBuffer::ChunkCoord dims = chunkBuffer->GetDimensions();
  EXPECT_EQ(dims.x, UnitChunkBuffer::W);
  EXPECT_EQ(dims.y, UnitChunkBuffer::H);
  EXPECT_EQ(dims.z, UnitChunkBuffer::D);
}

// Test IndexMap3d2
TEST_F(UnitChunkBufferTest, IndexMap3d2CalculatesCorrectIndex)
{
  // Test origin
  int v1 = IndexMap3d2<UnitChunkBuffer::W, UnitChunkBuffer::H>(0, 0, 0);
  EXPECT_EQ(v1, 0);
  // Test (1,0,0)
  int v2 = IndexMap3d2<UnitChunkBuffer::W, UnitChunkBuffer::H>(1, 0, 0);
  EXPECT_EQ(v2, 1);
  // Test (0,1,0) - should be W
  int v3 = IndexMap3d2<UnitChunkBuffer::W, UnitChunkBuffer::H>(0, 1, 0);
  EXPECT_EQ(v3, UnitChunkBuffer::W);
  // Test (0,0,1) - should be W*H
  int v4 = IndexMap3d2<UnitChunkBuffer::W, UnitChunkBuffer::H>(0, 0, 1);
  EXPECT_EQ(v4, UnitChunkBuffer::W * UnitChunkBuffer::H);
}

// Test IndexMap3d2 with ChunkCoord
TEST_F(UnitChunkBufferTest, IndexMap3d2WithChunkCoord)
{
  UnitChunkBuffer::ChunkCoord coord{5, 10, 2};
  int expected = 5 + (10 * UnitChunkBuffer::W) + (2 * UnitChunkBuffer::W * UnitChunkBuffer::H);
  int v = IndexMap3d2<UnitChunkBuffer::W, UnitChunkBuffer::H>(coord);
  EXPECT_EQ(v, expected);
}

// Test IndexVal
TEST_F(UnitChunkBufferTest, IndexValValidatesCoordinates)
{
  int x = 5;
  EXPECT_TRUE(IndexVal(x, UnitChunkBuffer::W));

  x = -1;
  EXPECT_FALSE(IndexVal(x, UnitChunkBuffer::W));

  x = UnitChunkBuffer::W;
  EXPECT_FALSE(IndexVal(x, UnitChunkBuffer::W));
}

// Test buffer initialization to zero
TEST_F(UnitChunkBufferTest, BufferInitializedToZero)
{
  int* raw = chunkBuffer->GetRaw();
  for (int i = 0; i < chunkBuffer->GetTotalBlocks(); i++)
  {
    EXPECT_EQ(raw[i], 0);
  }
}

// Test setting and getting values
TEST_F(UnitChunkBufferTest, SetAndGetValue)
{
  int* raw = chunkBuffer->GetRaw();
  raw[100] = 42;

  auto id = chunkBuffer->GetUnitIdXYZ(100 % UnitChunkBuffer::W, (100 / UnitChunkBuffer::W) % UnitChunkBuffer::H, 100 / (UnitChunkBuffer::W * UnitChunkBuffer::H));

  ASSERT_TRUE(id.has_value());
  EXPECT_EQ(id.value(), 42);
}

// Test corner coordinates
TEST_F(UnitChunkBufferTest, CornerCoordinates)
{
  int* raw = chunkBuffer->GetRaw();

  // Set values at corners
  raw[0] = 1; // (0,0,0)
  raw[UnitChunkBuffer::W - 1] = 2; // (W-1,0,0)
  raw[(UnitChunkBuffer::W * UnitChunkBuffer::H) - 1] = 3; // (W-1,H-1,0)
  raw[(UnitChunkBuffer::W * UnitChunkBuffer::H * UnitChunkBuffer::D) - 1] = 4; // (W-1,H-1,D-1)

  EXPECT_EQ(chunkBuffer->GetUnitIdXYZ(0, 0, 0).value(), 1);
  EXPECT_EQ(chunkBuffer->GetUnitIdXYZ(UnitChunkBuffer::W - 1, 0, 0).value(), 2);
  EXPECT_EQ(chunkBuffer->GetUnitIdXYZ(UnitChunkBuffer::W - 1, UnitChunkBuffer::H - 1, 0).value(), 3);
  EXPECT_EQ(chunkBuffer->GetUnitIdXYZ(UnitChunkBuffer::W - 1, UnitChunkBuffer::H - 1, UnitChunkBuffer::D - 1).value(), 4);
}

// Test Clear on invalid buffer
TEST(UnitChunkBufferEdgeCases, ClearOnInvalidBuffer)
{
  UnitChunkBuffer buffer;
  UnitChunkBuffer moved(std::move(buffer));

  // Should not crash on invalid buffer
  buffer.Clear();
  SUCCEED();
}

// Test GetUnitIdXYZ on invalid buffer
TEST(UnitChunkBufferEdgeCases, GetUnitIdXYZOnInvalidBuffer)
{
  UnitChunkBuffer buffer;
  UnitChunkBuffer moved(std::move(buffer));

  auto id = buffer.GetUnitIdXYZ(0, 0, 0);
  EXPECT_FALSE(id.has_value());
}
