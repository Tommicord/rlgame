export module Rl.RayLog.StackTrace;

import <vector>;
import <string>;

namespace Rl::RayLog
{

export class RayLogStackTrace
{
  public:
  [[nodiscard]]
  static std::vector<void*> Capture(int skipFrames = 0)
  {
#if defined(__linux__) || defined(__APPLE__)
    #include <execinfo.h>
    constexpr int MaxFrames = 64;
    void* buffer[MaxFrames];
    int count = backtrace(buffer, MaxFrames);
    std::vector<void*> result;
    for (int i = skipFrames; i < count; ++i)
      result.push_back(buffer[i]);
    return result;
#elif defined(_WIN32)
    #include <windows.h>
    #include <DbgHelp.h>
    constexpr int MaxFrames = 64;
    void* buffer[MaxFrames];
    USHORT count = CaptureStackBackTrace(skipFrames + 1, MaxFrames, buffer, nullptr);
    std::vector<void*> result(buffer, buffer + count);
    return result;
#else
    return {};
#endif
  }

  [[nodiscard]]
  static std::string ToString(const std::vector<void*>& frames)
  {
    std::string result = "Stack Trace:\n";
    for (size_t i = 0; i < frames.size(); ++i)
    {
      result += "  #" + std::to_string(i) + " " + PointerToString(frames[i]) + "\n";
    }
    return result;
  }

  private:
  [[nodiscard]]
  static std::string PointerToString(void* ptr)
  {
    char buffer[20];
    snprintf(buffer, sizeof(buffer), "%p", ptr);
    return std::string(buffer);
  }
};

}
