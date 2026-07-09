import Rl.World.Chunk.ChunkSystem;
import Rl.World.Chunk.ChunkInRenderUnits;
import Rl.World.Chunk.UnitChunkBuffer;
import Rl.World.Chunk.UnitChunkAccessor;

#include <gtest/gtest.h>

using namespace Rl::World::Chunk;

TEST(ChunkSystemTest, RefreshGeneratesChunksAroundPlayerAndTracksThem)
{
  ChunkInRenderUnits backing(3, 3, 3);
  ASSERT_TRUE(backing.Initialize());

  ChunkSystem system(backing, WorldChunkCoord{3, 3, 3});
  int generationCalls = 0;

  system.SetChunkGenerator([&](const WorldChunkCoord& coord, UnitChunkBuffer& chunkBuffer) {
    ++generationCalls;
    chunkBuffer.Clear();
    return true;
  });

  system.SetPlayerPosition(UnitPosition{0, 0, 0});
  system.RefreshNow();

  EXPECT_EQ(generationCalls, 27);
  EXPECT_TRUE(system.IsChunkGenerated(WorldChunkCoord{0, 0, 0}));
  EXPECT_EQ(backing.GetChunkCount(), 27u);

  system.SetPlayerPosition(UnitPosition{100, 0, 0});
  system.RefreshNow();

  EXPECT_FALSE(system.IsChunkGenerated(WorldChunkCoord{0, 0, 0}));
}
