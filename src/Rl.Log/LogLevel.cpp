#include "Rl.Log/LogLevel.h"

namespace rl
{

const char* logLevelToString(LogLevel level) noexcept
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

} // namespace rl
