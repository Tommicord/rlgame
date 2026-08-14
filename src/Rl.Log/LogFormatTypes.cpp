#include "Rl.Log/LogFormatTypes.h"
#include <cstdio>
#include <cstring>

namespace rl
{

void LogFormatTypes::formatint(
    char* buffer, size_t& pos, size_t bufferSize, int64_t value, const FormatSpec& spec) noexcept
{
  if (pos >= bufferSize)
    return;

  int written = snprintf(buffer + pos, bufferSize - pos, "%lld", static_cast<long long>(value));
  if (written > 0 && pos + static_cast<size_t>(written) < bufferSize)
  {
    pos += static_cast<size_t>(written);
  }
}

void LogFormatTypes::formatUint(
    char* buffer, size_t& pos, size_t bufferSize, uint64_t value, const FormatSpec& spec) noexcept
{
  if (pos >= bufferSize)
    return;

  int written =
      snprintf(buffer + pos, bufferSize - pos, "%llu", static_cast<unsigned long long>(value));
  if (written > 0 && pos + static_cast<size_t>(written) < bufferSize)
  {
    pos += static_cast<size_t>(written);
  }
}

void LogFormatTypes::formatFloat(
    char* buffer, size_t& pos, size_t bufferSize, double value, const FormatSpec& spec) noexcept
{
  if (pos >= bufferSize)
    return;

  int written = snprintf(buffer + pos, bufferSize - pos, "%f", value);
  if (written > 0 && pos + static_cast<size_t>(written) < bufferSize)
  {
    pos += static_cast<size_t>(written);
  }
}

void LogFormatTypes::formatString(
    char* buffer, size_t& pos, size_t bufferSize, const char* str, const FormatSpec& spec) noexcept
{
  if (str == nullptr || pos >= bufferSize)
    return;

  size_t len       = strlen(str);
  size_t available = bufferSize - pos - 1;
  size_t toCopy    = (len < available) ? len : available;

  memcpy(buffer + pos, str, toCopy);
  pos += toCopy;
  buffer[pos] = '\0';
}

void LogFormatTypes::formatPointer(char*       buffer,
                                   size_t&     pos,
                                   size_t      bufferSize,
                                   const void* ptr) noexcept
{
  if (pos >= bufferSize)
    return;

  int written = snprintf(buffer + pos, bufferSize - pos, "%p", ptr);
  if (written > 0 && pos + static_cast<size_t>(written) < bufferSize)
  {
    pos += static_cast<size_t>(written);
  }
}

} // namespace rl
