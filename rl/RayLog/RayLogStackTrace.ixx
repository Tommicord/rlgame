export module Rl.RayLog.StackTrace;

import <vector>;
import <string>;
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <DbgHelp.h>
#include <windows.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <execinfo.h>
#endif

namespace Rl::RayLog
{

export class RayLogStackTrace
{
public:
    [[nodiscard]]
    static std::vector<void*> Capture(const int skipFrames = 0)
    {
#if defined(__linux__) || defined(__APPLE__)
        constexpr int      MaxFrames = 64;
        void*              buffer[MaxFrames];
        int                count = backtrace(buffer, MaxFrames);
        std::vector<void*> result;
        for (int i = skipFrames; i < count; ++i)
            result.push_back(buffer[i]);
        return result;
#elif defined(_WIN32)
        constexpr int MaxFrames = 64;
        void*         buffer[MaxFrames];
        const USHORT  count =
            CaptureStackBackTrace(skipFrames + 1, MaxFrames, buffer, nullptr);
        std::vector result(buffer, buffer + count);
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
        return {buffer};
    }
};

} // namespace Rl::RayLog
