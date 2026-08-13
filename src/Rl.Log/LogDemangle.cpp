#include "Rl.Log/LogDemangle.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dbghelp.h>
#elif defined(__ANDROID__)
#include <cxxabi.h>
#else
#include <cxxabi.h>
#include <execinfo.h>
#endif

#include <cstdio>
#include <cstring>

namespace rl
{

void LogDemangle::demangleSymbol(char* buffer, size_t bufferSize, const void* address) noexcept
{
        if (buffer == nullptr || bufferSize == 0)
                return;

        buffer[0] = '\0';

#ifdef _WIN32
        HANDLE       process = GetCurrentProcess();
        char         symbolBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
        SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(symbolBuffer);

        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen   = MAX_SYM_NAME;

        if (SymFromAddr(process, reinterpret_cast<DWORD64>(address), nullptr, symbol))
        {
                snprintf(buffer, bufferSize, "%s", symbol->Name);
        }
        else
        {
                snprintf(buffer, bufferSize, "0x%p", address);
        }
#elif defined(__ANDROID__)
        snprintf(buffer, bufferSize, "0x%p", address);
#else
        char** strings = backtrace_symbols(static_cast<void* const*>(&address), 1);
        if (strings != nullptr && strings[0] != nullptr)
        {
                // Try to demangle the symbol
                size_t len       = strlen(strings[0]);
                char*  demangled = abi::__cxa_demangle(strings[0], nullptr, nullptr, nullptr);
                if (demangled != nullptr)
                {
                        snprintf(buffer, bufferSize, "%s", demangled);
                        free(demangled);
                }
                else
                {
                        snprintf(buffer, bufferSize, "%s", strings[0]);
                }
                free(strings);
        }
        else
        {
                snprintf(buffer, bufferSize, "0x%p", address);
        }
#endif
}

bool LogDemangle::isSystemLibrary(const char* symbol) noexcept
{
        if (symbol == nullptr)
                return false;
        const char* systemLibs[] = {"ntdll.dll", "kernel32.dll", "libc++", "libstdc++", nullptr};
        for (int i = 0; systemLibs[i] != nullptr; i++)
        {
                if (strstr(symbol, systemLibs[i]) != nullptr)
                {
                        return true;
                }
        }

        return false;
}

} // namespace rl
