#ifndef RL_LOG_LOG_STACK_TRACE_H
#define RL_LOG_LOG_STACK_TRACE_H

#include "Rl.Log/LogCommon.h"

namespace rl
{

/** Represents a single frame in a stack trace */
struct StackFrame
{
    void* address; /**< Instruction address */
    char  symbol[256]; /**< Demangled symbol name */
};

/** Stack trace capture and formatting utilities */
class LogStackTrace
{
  public:
    /** Captures the current stack trace
     * @param frames Output array for stack frames
     * @param maxFrames Maximum number of frames to capture
     * @return Number of frames captured */
    static int capture(StackFrame* frames, int maxFrames) noexcept;
    /** Demangles a symbol in a stack frame
     * @param frame The frame to demangle */
    static void demangle(StackFrame* frame) noexcept;
    /** Checks if a frame should be skipped in output
     * @param symbol The symbol to check
     * @return true if should skip, false otherwise */
    static bool shouldSkipFrame(const char* symbol) noexcept;
    /** Formats a stack trace into a string
     * @param buffer Output buffer
     * @param bufferSize Size of output buffer
     * @param frames Array of stack frames
     * @param frameCount Number of frames */
    static void formatStackTrace(char*             buffer,
                                 size_t            bufferSize,
                                 const StackFrame* frames,
                                 int               frameCount) noexcept;
};

} // namespace rl

#endif // RL_LOG_LOG_STACK_TRACE_H
