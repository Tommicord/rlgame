#include "Rl.Log/LogStackTraceWin32.h"

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <cstdio>
#include <windows.h>
#include <dbghelp.h>

namespace rl
{

namespace
{
bool _initialized = false;
}

bool LogStackTraceWin32::initialize() noexcept
{
        if (_initialized)
                return true;

        HANDLE process = GetCurrentProcess();

        SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);

        if (SymInitialize(process, nullptr, TRUE))
        {
                _initialized = true;
                return true;
        }

        return false;
}

int LogStackTraceWin32::capture(StackFrame* frames, int maxFrames) noexcept
{
        if (frames == nullptr || maxFrames <= 0)
                return 0;

        if (!initialize())
                return 0;

        HANDLE process = GetCurrentProcess();
        void*  stack[logMaxStackFrames];

        USHORT captured = CaptureStackBackTrace(
            0,
            static_cast<USHORT>(maxFrames < logMaxStackFrames ? maxFrames
                                                                 : logMaxStackFrames),
            stack, nullptr);

        int frameCount = 0;

        // Skip first 2 frames (CaptureStackBackTrace and this function)
        for (USHORT i = 2; i < captured && frameCount < maxFrames; i++)
        {
                frames[frameCount].address   = stack[i];
                frames[frameCount].symbol[0] = '\0';
                frameCount++;
        }

        return frameCount;
}

void LogStackTraceWin32::demangle(StackFrame* frame) noexcept
{
        if (frame == nullptr)
                return;

        HANDLE       process = GetCurrentProcess();
        char         symbolBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
        SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(symbolBuffer);

        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen   = MAX_SYM_NAME;

        DWORD64 displacement = 0;

        if (SymFromAddr(process, reinterpret_cast<DWORD64>(frame->address), &displacement, symbol))
        {
                snprintf(frame->symbol, sizeof(frame->symbol), "%s+0x%llx", symbol->Name,
                         static_cast<unsigned long long>(displacement));
        }
        else
        {
                snprintf(frame->symbol, sizeof(frame->symbol), "0x%p", frame->address);
        }
}

} // namespace rl

#endif // _WIN32
