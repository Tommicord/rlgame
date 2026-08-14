#include "Rl.Log/LogConsoleWin32.h"
#include "Rl.Log/LogLevel.h"

#ifdef _WIN32
#include <cstdio>
#include <io.h>

namespace rl
{

LogConsoleWin32::LogConsoleWin32(const LogHandle& handle) noexcept
{
  if (!GetConsoleWindow())
  {
    AllocConsole();
    freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
    freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
    SetConsoleTitleA("Debug Console for rlgame");
  }

  consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  if (GetConsoleScreenBufferInfo(consoleHandle, &csbi))
  {
    originalAttributes = csbi.wAttributes;
  }
  else
  {
    originalAttributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
  }
}

void LogConsoleWin32::write(LogLevel level, const char* message) noexcept
{
  if (message == nullptr)
    return;
  printf("%s\n", message);
}

void LogConsoleWin32::setColor(LogLevel level) noexcept
{
  WORD attributes = originalAttributes;
  switch (level)
  {
  case LogLevel::Trace:
    attributes |= FOREGROUND_INTENSITY;
    break;
  case LogLevel::Debug:
    attributes |= FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    break;
  case LogLevel::Info:
    attributes |= FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    break;
  case LogLevel::Warning:
    attributes |= FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    break;
  case LogLevel::Error:
    attributes |= FOREGROUND_RED | FOREGROUND_INTENSITY;
    break;
  }

  SetConsoleTextAttribute(consoleHandle, attributes);
}

void LogConsoleWin32::resetColor() noexcept
{
  SetConsoleTextAttribute(consoleHandle, originalAttributes);
}

} // namespace rl

#endif // _WIN32
