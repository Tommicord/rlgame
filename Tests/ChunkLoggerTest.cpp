import <gtest/gtest.h>;
import <vector>;

import Rl.World.Chunk.ChunkLogger;

using namespace Rl::World::Chunk;

TEST(ChunkLoggerTest, LoggerStartsEnabledAndEmpty)
{
  ChunkLogger logger;

  EXPECT_TRUE(logger.IsEnabled());
  EXPECT_TRUE(logger.GetRecentLogs().empty());
  EXPECT_EQ(logger.GetLogCount(LogLevel::Info), 0u);
}

TEST(ChunkLoggerTest, ChunkSpecificLogsAreStoredWithCoordinates)
{
  ChunkLogger logger;

  logger.LogChunk(LogLevel::Info, "world", "generated chunk", 12, 3, 4, 5);

  const std::vector<LogEntry> logs = logger.GetRecentLogs();
  ASSERT_EQ(logs.size(), 1u);
  EXPECT_EQ(logs[0].chunkIndex, 12u);
  EXPECT_EQ(logs[0].localX, 3);
  EXPECT_EQ(logs[0].localY, 4);
  EXPECT_EQ(logs[0].localZ, 5);
  EXPECT_EQ(logs[0].category, "world");
  EXPECT_EQ(logs[0].message, "generated chunk");
  EXPECT_EQ(logger.GetLogCount(LogLevel::Info), 1u);
}

TEST(ChunkLoggerTest, DisablingAndClearingStopsAndRemovesEntries)
{
  ChunkLogger logger;
  logger.Log(LogLevel::Warning, "debug", "warning message");
  EXPECT_EQ(logger.GetLogCount(LogLevel::Warning), 1u);

  logger.SetEnabled(false);
  logger.Log(LogLevel::Error, "debug", "suppressed");
  EXPECT_EQ(logger.GetLogCount(LogLevel::Error), 0u);

  logger.ClearLogs();
  EXPECT_TRUE(logger.GetRecentLogs().empty());
  EXPECT_EQ(logger.GetLogCount(LogLevel::Warning), 0u);
}
