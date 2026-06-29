export module Rl.RayLog.SymbolDemangler;

import <string>;
import <vector>;
#if defined(_WIN32)
#include <windows.h>
#include <dbghelp.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <cxxabi.h>
#include <execinfo.h>
#endif

namespace Rl::RayLog
{

export class RayLogSymbolDemangler
{
  public:
  [[nodiscard]]
  static std::string Demangle(const char* mangledName)
  {
#if defined(__linux__) || defined(__APPLE__)
    int   status = 0;
    char* demangled = abi::__cxa_demangle(mangledName, nullptr, nullptr, &status);
    if (demangled && status == 0)
    {
      std::string result(demangled);
      free(demangled);
      return result;
    }
    return mangledName ? mangledName : "??";
#elif defined(_WIN32)
    static bool initialized = false;
if (!initialized)
{
  SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
  SymInitialize(GetCurrentProcess(), nullptr, TRUE);
  initialized = true;
}
char buffer[1024];
if (UnDecorateSymbolName(mangledName, buffer, sizeof(buffer), UNDNAME_COMPLETE))
  return {buffer};
return mangledName ? mangledName : "??";
#else
    return mangledName ? mangledName : "??";
#endif
  }
  [[nodiscard]]
  static std::string DemangleStackTrace(const std::vector<void*>& frames)
  {
    std::string result = "Stack Trace:\n";

#if defined(__linux__) || defined(__APPLE__)
    auto symbols = backtrace_symbols(frames.data(), frames.size());
    if (symbols)
    {
      for (size_t i = 0; i < frames.size(); ++i)
      {
        result += "  #" + std::to_string(i) + " ";
        result += Demangle(symbols[i]) + "\n";
      }
      free(symbols);
    }
#elif defined(_WIN32)
  HANDLE process = GetCurrentProcess();
  SymInitialize(process, nullptr, TRUE);

  for (size_t i = 0; i < frames.size(); ++i)
  {
    char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
    const auto symbol = reinterpret_cast<SYMBOL_INFO*>(buffer);
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = MAX_SYM_NAME;

    DWORD64 displacement = 0;
    if (SymFromAddr(process, reinterpret_cast<DWORD64>(frames[i]), &displacement, symbol))
    {
      result += "  #" + std::to_string(i) + " " + std::string(symbol->Name) + "\n";
    }
    else
    {
      result += "  #" + std::to_string(i) + " ??\n";
    }
  }
#else
    for (size_t i = 0; i < frames.size(); ++i)
    {
      result += "  #" + std::to_string(i) + " ??" + "\n";
    }
#endif
    return result;
  }
};

} // namespace Rl::RayLog
