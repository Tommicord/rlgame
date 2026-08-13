#include "Rl.Log/LogStackTracePosix.h"

#if !defined(_WIN32) && !defined(__ANDROID__)

#include <cstdio>
#include <cstring>
#include <execinfo.h>

namespace rl
{

int LogStackTracePosix::capture(StackFrame* frames, int maxFrames) noexcept
{
        if (frames == nullptr || maxFrames <= 0)
                return 0;

        void* buffer[LOG_MAX_STACK_FRAMES];
        int   captured =
            backtrace(buffer, maxFrames < LOG_MAX_STACK_FRAMES ? maxFrames : LOG_MAX_STACK_FRAMES);

        // Skip first 2 frames (backtrace and this function)
        int frameCount = 0;
        for (int i = 2; i < captured && frameCount < maxFrames; i++)
        {
                frames[frameCount].address   = buffer[i];
                frames[frameCount].symbol[0] = '\0';
                frameCount++;
        }

        return frameCount;
}

void LogStackTracePosix::demangle(StackFrame* frame) noexcept
{
        if (frame == nullptr)
                return;

        char** strings = backtrace_symbols(&frame->address, 1);
        if (strings != nullptr && strings[0] != nullptr)
        {
                // Copy the raw symbol (full demangling happens in LogDemangle)
                strncpy(frame->symbol, strings[0], sizeof(frame->symbol) - 1);
                frame->symbol[sizeof(frame->symbol) - 1] = '\0';
                free(strings);
        }
        else
        {
                snprintf(frame->symbol, sizeof(frame->symbol), "0x%p", frame->address);
        }
}

} // namespace rl

#endif // !defined(_WIN32) && !defined(__ANDROID__)
