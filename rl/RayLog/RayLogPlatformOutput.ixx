module;

#if defined(__ANDROID__)
#include <android/log.h>
#elif defined(__APPLE__) && TARGET_OS_IPHONE
#include <os/log.h>
#elif defined(__linux__) && !defined(__ANDROID__)
#include <syslog.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

export module Rl.RayLog.PlatformOutput;

import Rl.RayLog.Platform;
import <string>;
import <iostream>;

namespace Rl::RayLog
{

#if defined(_WIN32)
namespace
{
struct ConsoleInitializer
{
  ConsoleInitializer()
  {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole != INVALID_HANDLE_VALUE)
    {
      DWORD mode = 0;
      if (GetConsoleMode(hConsole, &mode))
      {
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hConsole, mode);
      }
    }
  }
};
static ConsoleInitializer consoleInit;
} // namespace
#endif

export class RayLogPlatformOutput
{
  public:
  static void Write(const std::string& message)
  {
#if defined(__ANDROID__)
    __android_log_print(ANDROID_LOG_INFO, "RayLog", "%s", message.c_str());
#elif defined(__APPLE__) && TARGET_OS_IPHONE
    os_log_with_type(OS_LOG_DEFAULT, OS_LOG_TYPE_INFO, "%{public}s", message.c_str());
#elif defined(__linux__) && !defined(__ANDROID__)
    syslog(LOG_INFO, "%s", message.c_str());
#elif defined(_WIN32)
    OutputDebugStringA(message.c_str());
    OutputDebugStringA("\n");
    std::cout << message << std::endl;
#endif
  }

  static void WriteError(const std::string& message)
  {
#if defined(__ANDROID__)
    __android_log_print(ANDROID_LOG_ERROR, "RayLog", "%s", message.c_str());
#elif defined(__APPLE__) && TARGET_OS_IPHONE
    os_log_with_type(OS_LOG_DEFAULT, OS_LOG_TYPE_ERROR, "%{public}s", message.c_str());
#elif defined(__linux__) && !defined(__ANDROID__)
    syslog(LOG_ERR, "%s", message.c_str());
#elif defined(_WIN32)
    OutputDebugStringA(message.c_str());
    OutputDebugStringA("\n");
    std::cerr << message << std::endl;
#endif
  }
};

} // namespace Rl::RayLog
