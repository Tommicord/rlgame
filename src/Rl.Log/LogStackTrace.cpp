#include "Rl.Log/LogStackTrace.h"

#ifdef _WIN32
#include "Rl.Log/LogStackTraceWin32.h"
#elif defined(__ANDROID__)
#include "Rl.Log/LogStackTraceAndroid.h"
#else
#include "Rl.Log/LogStackTracePosix.h"
#endif

#include <cstdio>
#include <cstring>

#include "Rl.Log/LogDemangle.h"

namespace rl
{

int LogStackTrace::capture(StackFrame* frames, int maxFrames) noexcept
{
  if (frames == nullptr || maxFrames <= 0)
    return 0;

#ifdef _WIN32
  return LogStackTraceWin32::capture(frames, maxFrames);
#elif defined(__ANDROID__)
  return LogStackTraceAndroid::capture(frames, maxFrames);
#else
  return LogStackTracePosix::capture(frames, maxFrames);
#endif
}

void LogStackTrace::demangle(StackFrame* frame) noexcept
{
  if (frame == nullptr)
    return;

  LogDemangle::demangleSymbol(frame->symbol, sizeof(frame->symbol), frame->address);
}

bool LogStackTrace::shouldSkipFrame(const char* symbol) noexcept
{
  if (symbol == nullptr)
    return false;

  // Skip logging functions
  if (strstr(symbol, "Rl::Log::") != nullptr)
    return true;
  // Skip system libraries
  if (LogDemangle::isSystemLibrary(symbol))
    return true;

  return false;
}

void LogStackTrace::formatStackTrace(char*             buffer,
                                     size_t            bufferSize,
                                     const StackFrame* frames,
                                     int               frameCount) noexcept
{
  if (buffer == nullptr || bufferSize == 0 || frames == nullptr)
    return;
  size_t pos = 0;
  for (int i = 0; i < frameCount && pos < bufferSize - 1; i++)
  {
    if (shouldSkipFrame(frames[i].symbol))
      continue;

    int written = snprintf(buffer + pos, bufferSize - pos, "  #%d: %s\n", i, frames[i].symbol);
    if (written > 0 && pos + static_cast<size_t>(written) < bufferSize)
    {
      pos += static_cast<size_t>(written);
    }
    else
    {
      break;
    }
  }

  buffer[pos] = '\0';
}

} // namespace rl
