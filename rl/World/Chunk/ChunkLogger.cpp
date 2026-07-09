import Rl.World.Chunk.ChunkLogger;

import <chrono>;
import <algorithm>;
import <format>;
import <atomic>;

import Rl.RayLog.LevelPrinter;
import Rl.RayLog.Logger;
import Rl.RayLog.Macro;

namespace Rl::World::Chunk
{

ChunkLogger::ChunkLogger() : enabled(true)
{
  logEntries.reserve(MAX_LOG_ENTRIES);
  for (uint32_t i = 0; i < 6; ++i)
  {
    logCounts[i].store(0, std::memory_order_release);
  }
}

ChunkLogger::~ChunkLogger()
{ ClearLogs(); }

ChunkLogger::ChunkLogger(ChunkLogger&& other) noexcept :
    logEntries(std::move(other.logEntries)), enabled(other.enabled.load())
{
  for (uint32_t i = 0; i < 6; ++i)
  {
    logCounts[i].store(
        other.logCounts[i].load(std::memory_order_acquire), std::memory_order_release);
    other.logCounts[i].store(0, std::memory_order_release);
  }
  other.enabled.store(false, std::memory_order_release);
}

ChunkLogger& ChunkLogger::operator=(ChunkLogger&& other) noexcept
{
  if (this != &other)
  {
    ClearLogs();

    logEntries = std::move(other.logEntries);
    enabled.store(other.enabled.load(), std::memory_order_release);

    for (uint32_t i = 0; i < 6; ++i)
    {
      logCounts[i].store(
          other.logCounts[i].load(std::memory_order_acquire), std::memory_order_release);
      other.logCounts[i].store(0, std::memory_order_release);
    }
    other.enabled.store(false, std::memory_order_release);
  }
  return *this;
}

void ChunkLogger::Log(
    RayLog::RayLogLevel level, const std::string& category, const std::string& message)
{
  if (!enabled.load(std::memory_order_acquire))
    return;

  // Use RayLog for actual logging
  switch (level)
  {
  case RayLog::RayLogLevel::Trace:
    Rl::RayLog::LogTrace(category, "%s", message);
    break;
  case RayLog::RayLogLevel::Debug:
    Rl::RayLog::LogDebug(category, "%s", message);
    break;
  case RayLog::RayLogLevel::Info:
    Rl::RayLog::LogInfo(category, "%s", message);
    break;
  case RayLog::RayLogLevel::Warning:
    Rl::RayLog::LogWarning(category, "%s", message);
    break;
  case RayLog::RayLogLevel::Error:
    Rl::RayLog::LogError(category, "%s", message);
    break;
  case RayLog::RayLogLevel::Fatal:
    Rl::RayLog::LogFatal(category, "%s", message);
    break;
  }

  // Also store in local buffer for querying
  LogEntry entry;
  entry.level = level;
  entry.timestamp = GetCurrentTimestamp();
  entry.category = category;
  entry.message = message;
  entry.chunkIndex = 0;
  entry.localX = 0;
  entry.localY = 0;
  entry.localZ = 0;

  AddLogEntry(entry);
}

void ChunkLogger::LogChunk(RayLog::RayLogLevel level,
    const std::string&                         category,
    const std::string&                         message,
    uint32_t                                   chunkIndex,
    int32_t                                    localX,
    int32_t                                    localY,
    int32_t                                    localZ)
{
  if (!enabled.load(std::memory_order_acquire))
    return;
  const std::string chunkMsg = std::format(
      "[Chunk {} @({}, {}, {})] %s", chunkIndex, localX, localY, localZ, message);
  switch (level)
  {
  case RayLog::RayLogLevel::Trace:
    Rl::RayLog::LogTrace(category, "%s", chunkMsg);
    break;
  case RayLog::RayLogLevel::Debug:
    Rl::RayLog::LogDebug(category, "%s", chunkMsg);
    break;
  case RayLog::RayLogLevel::Info:
    Rl::RayLog::LogInfo(category, "%s", chunkMsg);
    break;
  case RayLog::RayLogLevel::Warning:
    Rl::RayLog::LogWarning(category, "%s", chunkMsg);
    break;
  case RayLog::RayLogLevel::Error:
    Rl::RayLog::LogError(category, "%s", chunkMsg);
    break;
  case RayLog::RayLogLevel::Fatal:
    Rl::RayLog::LogFatal(category, "%s", chunkMsg);
    break;
  }

  // Also store in local buffer for querying with original message
  LogEntry entry;
  entry.level = level;
  entry.timestamp = GetCurrentTimestamp();
  entry.category = category;
  entry.message = message;
  entry.chunkIndex = chunkIndex;
  entry.localX = localX;
  entry.localY = localY;
  entry.localZ = localZ;

  AddLogEntry(entry);
}

std::vector<LogEntry> ChunkLogger::GetRecentLogs(uint32_t maxEntries) const
{
  std::vector<LogEntry> result;
  uint32_t              startIdx = 0;

  if (logEntries.size() > maxEntries)
  {
    startIdx = static_cast<uint32_t>(logEntries.size()) - maxEntries;
  }

  for (uint32_t i = startIdx; i < logEntries.size(); ++i)
  {
    result.push_back(logEntries[i]);
  }

  return result;
}

void ChunkLogger::ClearLogs()
{
  logEntries.clear();

  for (uint32_t i = 0; i < 6; ++i)
  {
    logCounts[i].store(0, std::memory_order_release);
  }
}

uint32_t ChunkLogger::GetLogCount(RayLog::RayLogLevel level) const
{
  uint32_t levelIndex = LevelToIndex(level);
  if (levelIndex >= 6)
    return 0;

  return logCounts[levelIndex].load(std::memory_order_acquire);
}

void ChunkLogger::SetEnabled(bool enabled)
{ this->enabled.store(enabled, std::memory_order_release); }

bool ChunkLogger::IsEnabled() const
{ return enabled.load(std::memory_order_acquire); }

void ChunkLogger::AddLogEntry(const LogEntry& entry)
{
  // Remove oldest entry if buffer is full
  if (logEntries.size() >= MAX_LOG_ENTRIES)
  {
    // Decrement count for the removed entry's level
    uint32_t removedLevelIndex = LevelToIndex(logEntries.front().level);
    if (removedLevelIndex < 6)
    {
      logCounts[removedLevelIndex].fetch_sub(1, std::memory_order_release);
    }
    logEntries.erase(logEntries.begin());
  }

  logEntries.push_back(entry);

  // Increment count for the new entry's level
  uint32_t levelIndex = LevelToIndex(entry.level);
  if (levelIndex < 6)
  {
    logCounts[levelIndex].fetch_add(1, std::memory_order_release);
  }
}

uint64_t ChunkLogger::GetCurrentTimestamp()
{
  auto now = std::chrono::steady_clock::now();
  auto duration = now.time_since_epoch();
  return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

uint32_t ChunkLogger::LevelToIndex(Rl::RayLog::RayLogLevel level)
{
  switch (level)
  {
  case RayLog::RayLogLevel::Trace:
    return 0;
  case RayLog::RayLogLevel::Debug:
    return 1;
  case RayLog::RayLogLevel::Info:
    return 2;
  case RayLog::RayLogLevel::Warning:
    return 3;
  case RayLog::RayLogLevel::Error:
    return 4;
  case RayLog::RayLogLevel::Fatal:
    return 5;
  default:
    return 0;
  }
}

} // namespace Rl::World::Chunk
