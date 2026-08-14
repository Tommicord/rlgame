#ifndef RL_LOG_LOG_STACK_TRACE_ANDROID_H
#define RL_LOG_LOG_STACK_TRACE_ANDROID_H

#ifdef __ANDROID__

#include "Rl.Log/LogStackTrace.h"

namespace rl
{

class LogStackTraceAndroid
{
  public:
    static int  capture(StackFrame* frames, int maxFrames) noexcept;
    static void demangle(StackFrame* frame) noexcept;
};

} // namespace rl

#endif // __ANDROID__

#endif // RL_LOG_LOG_STACK_TRACE_ANDROID_H
