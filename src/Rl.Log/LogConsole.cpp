#include "Rl.Log/LogConsole.h"

#ifdef _WIN32
#include "Rl.Log/LogConsoleWin32.h"
#elif defined(__ANDROID__)
#include "Rl.Log/LogConsoleAndroid.h"
#else
#include "Rl.Log/LogConsolePosix.h"
#endif

namespace rl
{

LogConsole* createConsole(const LogHandle& handle) noexcept
{
#ifdef _WIN32
  static LogConsoleWin32 console(handle);
  return &console;
#elif defined(__ANDROID__)
  static LogConsoleAndroid console(handle);
  return &console;
#else
  static LogConsolePosix console(handle);
  return &console;
#endif
}

} // namespace rl
