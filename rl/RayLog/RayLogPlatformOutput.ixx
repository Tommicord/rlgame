export module Rl.RayLog.PlatformOutput;

import Rl.RayLog.Platform;
import <string>;

namespace Rl::RayLog
{

export class RayLogPlatformOutput
{
  public:
  static void Write(const std::string& message)
  {
#if Platform::IsAndroid
    #include <android/log.h>
    __android_log_print(ANDROID_LOG_INFO, "RayLog", "%s", message.c_str());
#elif Platform::IsIOS
    #include <os/log.h>
    os_log_with_type(OS_LOG_DEFAULT, OS_LOG_TYPE_INFO, "%{public}s", message.c_str());
#elif Platform::IsLinux || Platform::IsMacOS
    #include <syslog.h>
    syslog(LOG_INFO, "%s", message.c_str());
#elif Platform::IsWindows
    #include <windows.h>
    OutputDebugStringA(message.c_str());
    OutputDebugStringA("\n");
#endif
  }

  static void WriteError(const std::string& message)
  {
#if Platform::IsAndroid
    #include <android/log.h>
    __android_log_print(ANDROID_LOG_ERROR, "RayLog", "%s", message.c_str());
#elif Platform::IsIOS
    #include <os/log.h>
    os_log_with_type(OS_LOG_DEFAULT, OS_LOG_TYPE_ERROR, "%{public}s", message.c_str());
#elif Platform::IsLinux || Platform::IsMacOS
    #include <syslog.h>
    syslog(LOG_ERR, "%s", message.c_str());
#elif Platform::IsWindows
    #include <windows.h>
    OutputDebugStringA(message.c_str());
    OutputDebugStringA("\n");
#endif
  }
};

}
