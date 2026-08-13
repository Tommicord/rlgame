#ifndef RL_LOG_LOG_STACK_TRACE_WIN32_H
#define RL_LOG_LOG_STACK_TRACE_WIN32_H

#include "Rl.Log/LogStackTrace.h"
#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>

namespace rl
{

class LogStackTraceWin32
{
        public:
                static int  capture(StackFrame* frames, int maxFrames) noexcept;
                static void demangle(StackFrame* frame) noexcept;

        private:
                static bool initialize() noexcept;
};

} // namespace rl

#endif // _WIN32

#endif // RL_LOG_LOG_STACK_TRACE_WIN32_H
