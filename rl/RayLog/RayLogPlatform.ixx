export module Rl.RayLog.Platform;

export namespace Rl::RayLog::Platform
{
  inline constexpr bool IsAndroid = 
#if defined(__ANDROID__)
    true;
#else
    false;
#endif

  inline constexpr bool IsIOS = 
#if defined(__APPLE__) && TARGET_OS_IPHONE
    true;
#else
    false;
#endif

  inline constexpr bool IsMacOS = 
#if defined(__APPLE__) && !TARGET_OS_IPHONE
    true;
#else
    false;
#endif

  inline constexpr bool IsLinux = 
#if defined(__linux__) && !defined(__ANDROID__)
    true;
#else
    false;
#endif

  inline constexpr bool IsWindows = 
#if defined(_WIN32)
    true;
#else
    false;
#endif
}
