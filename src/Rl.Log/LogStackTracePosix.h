#ifndef RL_LOG_LOG_STACK_TRACE_POSIX_H
#define RL_LOG_LOG_STACK_TRACE_POSIX_H

#if !defined(_WIN32) && !defined(__ANDROID__)

#include "Rl.Log/LogStackTrace.h"

namespace rl
{

class LogStackTracePosix
{
        public:
                static int  capture(StackFrame* frames, int maxFrames) noexcept;
                static void demangle(StackFrame* frame) noexcept;
};

} // namespace rl

#endif // !defined(_WIN32) && !defined(__ANDROID__)

#endif // RL_LOG_LOG_STACK_TRACE_POSIX_H
