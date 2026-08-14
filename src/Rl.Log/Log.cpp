#include "Rl.Log/Log.h"

#include "Rl.Log/LogCommon.h"
#include "Rl.Log/LogConfig.h"
#include "Rl.Log/LogConsole.h"
#include "Rl.Log/LogFormatter.h"
#include "Rl.Log/LogLevel.h"
#include "Rl.Log/LogMutex.h"
#include "Rl.Log/LogStackTrace.h"

#include <cstdio>
#include <cstring>
#include <vector>

namespace rl
{

namespace
{

LogConfig   _config;
LogConsole* _console = nullptr;
LogMutex    _mutex;
LogHandle   _handle;
bool        _initialized = false;

void initializeConsole() noexcept
{
  if (_console == nullptr && _initialized)
  {
    _console = createConsole(_handle);
  }
}

bool shouldLog(LogLevel level) noexcept
{
  return static_cast<int>(level) >= static_cast<int>(_config.minLevel);
}

} // namespace

void Log::initialize(const LogHandle& handle) noexcept
{
  LogLock lock(_mutex);
  _handle      = handle;
  _initialized = true;
  initializeConsole();
}

void Log::shutdown() noexcept
{
  LogLock lock(_mutex);
  delete _console;
  _console     = nullptr;
  _initialized = false;
}

void Log::setConfig(const LogConfig& config) noexcept
{
  LogLock lock(_mutex);
  _config = config;
}

const LogConfig& Log::getConfig() noexcept
{
  return _config;
}

bool Log::shouldLog(LogLevel level) noexcept
{
  return static_cast<int>(level) >= static_cast<int>(_config.minLevel);
}

void Log::initializeConsole() noexcept
{
  if (_console == nullptr && _initialized)
  {
    _console = createConsole(_handle);
  }
}

const char* Log::logLevelToString(LogLevel level) noexcept
{
  switch (level)
  {
  case LogLevel::Trace:
    return "TRACE";
  case LogLevel::Debug:
    return "DEBUG";
  case LogLevel::Info:
    return "INFO";
  case LogLevel::Warning:
    return "WARN";
  case LogLevel::Error:
    return "ERROR";
  default:
    return "UNKNOWN";
  }
}

LogConfig& Log::getConfigInternal() noexcept
{
  return _config;
}

LogConsole* Log::getConsole() noexcept
{
  return _console;
}

LogMutex& Log::getMutex() noexcept
{
  return _mutex;
}

} // namespace rl
