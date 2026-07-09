export module Rl.World.Chunk.ChunkLogger;

import Rl.RayLog.Macro;
import Rl.RayLog.Logger;
import Rl.RayLog.Config;
import Rl.RayLog.LevelPrinter;

import <cstdint>;
import <string>;
import <vector>;
import <atomic>;

namespace Rl::World::Chunk
{

/* Log severity levels - using RayLog's levels */
export using LogLevel = Rl::RayLog::RayLogLevel;

/* Log entry structure - wraps RayLogMessage with chunk coordinates */
export struct LogEntry
{
  LogLevel    level;
  uint64_t    timestamp;
  std::string category;
  std::string message;
  uint32_t    chunkIndex;
  int32_t     localX;
  int32_t     localY;
  int32_t     localZ;
};

/* Simple logger for chunk operations */
export class ChunkLogger
{
  public:
  ChunkLogger();
  ~ChunkLogger();

  /* Disable copy operations */
  ChunkLogger(const ChunkLogger&) = delete;
  ChunkLogger& operator=(const ChunkLogger&) = delete;

  /* Enable move operations */
  ChunkLogger(ChunkLogger&& other) noexcept;
  ChunkLogger& operator=(ChunkLogger&& other) noexcept;

  /* Log a message */
  void Log(LogLevel level, const std::string& category, const std::string& message);

  /* Log with chunk coordinates */
  void LogChunk(LogLevel level,
      const std::string& category,
      const std::string& message,
      uint32_t           chunkIndex,
      int32_t            localX,
      int32_t            localY,
      int32_t            localZ);

  /* Get recent log entries */
  [[nodiscard]]
  std::vector<LogEntry> GetRecentLogs(uint32_t maxEntries = 100) const;

  /* Clear all logs */
  void ClearLogs();

  /* Get log count by level */
  [[nodiscard]]
  uint32_t GetLogCount(LogLevel level) const;

  /* Enable/disable logging */
  void SetEnabled(bool enabled);

  /* Check if logging is enabled */
  [[nodiscard]]
  bool IsEnabled() const;

  private:
  static constexpr uint32_t MAX_LOG_ENTRIES = 1000;

  std::vector<LogEntry> logEntries;
  std::atomic<bool>     enabled;
  std::atomic<uint32_t> logCounts[6]; // One for each RayLogLevel

  /* Add log entry to local buffer for querying */
  void AddLogEntry(const LogEntry& entry);

  /* Convert RayLogLevel to index */
  [[nodiscard]]
  static uint32_t LevelToIndex(LogLevel level);

  /* Get current timestamp */
  [[nodiscard]]
  static uint64_t GetCurrentTimestamp();
};

} // namespace Rl::World::Chunk
