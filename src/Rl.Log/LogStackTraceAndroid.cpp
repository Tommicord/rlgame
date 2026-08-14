#include "Rl.Log/LogStackTraceAndroid.h"

#ifdef __ANDROID__

#include <cstdio>
#include <cstring>
#include <unwind.h>

namespace rl
{

namespace
{
struct BacktraceState
{
                void** current;
                void** end;
};

static _Unwind_Reason_Code unwindCallback(struct _Unwind_Context* context, void* arg)
{
        BacktraceState* state = static_cast<BacktraceState*>(arg);
        uintptr_t       pc    = _Unwind_GetIP(context);

        if (pc)
        {
                if (state->current < state->end)
                {
                        *state->current++ = reinterpret_cast<void*>(pc);
                }
                else
                {
                        return _URC_END_OF_STACK;
                }
        }

        return _URC_NO_REASON;
}
} // namespace

int LogStackTraceAndroid::capture(StackFrame* frames, int maxFrames) noexcept
{
        if (frames == nullptr || maxFrames <= 0)
                return 0;

        void* buffer[logMaxStackFrames];

        BacktraceState state = {buffer, buffer + logMaxStackFrames};
        _Unwind_Backtrace(unwindCallback, &state);

        int captured = static_cast<int>(state.current - buffer);

        // Skip first 2 frames (unwind and this function)
        int frameCount = 0;
        for (int i = 2; i < captured && frameCount < maxFrames; i++)
        {
                frames[frameCount].address   = buffer[i];
                frames[frameCount].symbol[0] = '\0';
                frameCount++;
        }

        return frameCount;
}

void LogStackTraceAndroid::demangle(StackFrame* frame) noexcept
{
        if (frame == nullptr)
                return;

        // Android doesn't have easy symbol resolution without external libraries
        // Just store the address
        snprintf(frame->symbol, sizeof(frame->symbol), "0x%p", frame->address);
}

} // namespace rl

#endif // __ANDROID__
