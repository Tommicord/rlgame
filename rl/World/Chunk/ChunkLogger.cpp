import Rl.World.Chunk.ChunkLogger;

import <chrono>;
import <algorithm>;
import <format>;

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
{
  ClearLogs();
}

ChunkLogger::ChunkLogger(ChunkLogger&& other) noexcept
    : logEntries(std::move(other.logEntries)),
      enabled(other.enabled.load())
{
  for (uint32_t i = 0; i < 6; ++i)
  {
    logCounts[i].store(other.logCounts[i].load(std::memory_order_acquire), std::memory_order_release);
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
      logCounts[i].store(other.logCounts[i].load(std::memory_order_acquire), std::memory_order_release);
      other.logCounts[i].store(0, std::memory_order_release);
    }
    other.enabled.store(false, std::memory_order_release);
  }
  return *this;
}

void ChunkLogger::Log(LogLevel level, const std::string& category, const std::string& message)
{
  if (!enabled.load(std::memory_order_acquire))
    return;
  
  // Use RayLog for actual logging
  switch (level)
  {
    case LogLevel::Trace:
      Rl::RayLog::LogTrace(category, "{}", message);
      break;
    case LogLevel::Debug:
      Rl::RayLog::LogDebug(category, "{}", message);
      break;
    case LogLevel::Info:
      Rl::RayLog::LogInfo(category, "{}", message);
      break;
    case LogLevel::Warning:
      Rl::RayLog::LogWarning(category, "{}", message);
      break;
    case LogLevel::Error:
      Rl::RayLog::LogError(category, "{}", message);
      break;
    case LogLevel::Fatal:
      Rl::RayLog::LogFatal(category, "{}", message);
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

void ChunkLogger::LogChunk(LogLevel level, const std::string& category, const std::string& message,
                         uint32_t chunkIndex, int32_t localX, int32_t localY, int32_t localZ)
{
  if (!enabled.load(std::memory_order_acquire))
    return;
  
  // Use RayLog for actual logging with chunk coordinates in message
  std::string chunkMsg = std::format("[Chunk {} @ ({},{},{})] {}", chunkIndex, localX, localY, localZ, message);
  switch (level)
  {
    case LogLevel::Trace:
      Rl::RayLog::LogTrace(category, "{}", chunkMsg);
      break;
    case LogLevel::Debug:
      Rl::RayLog::LogDebug(category, "{}", chunkMsg);
      break;
    case LogLevel::Info:
      Rl::RayLog::LogInfo(category, "{}", chunkMsg);
      break;
    case LogLevel::Warning:
      Rl::RayLog::LogWarning(category, "{}", chunkMsg);
      break;
    case LogLevel::Error:
      Rl::RayLog::LogError(category, "{}", chunkMsg);
      break;
    case LogLevel::Fatal:
      Rl::RayLog::LogFatal(category, "{}", chunkMsg);
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
  uint32_t startIdx = 0;
  
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

uint32_t ChunkLogger::GetLogCount(LogLevel level) const
{
  uint32_t levelIndex = LevelToIndex(level);
  if (levelIndex >= 6)
    return 0;
  
  return logCounts[levelIndex].load(std::memory_order_acquire);
}

void ChunkLogger::SetEnabled(bool enabled)
{
  this->enabled.store(enabled, std::memory_order_release);
}

bool ChunkLogger::IsEnabled() const
{
  return enabled.load(std::memory_order_acquire);
}

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

uint32_t ChunkLogger::LevelToIndex(LogLevel level)
{
  switch (level)
  {
    case LogLevel::Trace:
      return 0;
    case LogLevel::Debug:
      return 1;
    case LogLevel::Info:
      return 2;
    case LogLevel::Warning:
      return 3;
    case LogLevel::Error:
      return 4;
    case LogLevel::Fatal:
      return 5;
    default:
      return 0;
  }
}

} // namespace Rl::World::Chunk
