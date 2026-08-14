#ifndef RL_LOG_LOG_DEMANGLE_H
#define RL_LOG_LOG_DEMANGLE_H

#include <cstddef>

namespace rl
{

/** Symbol demangling utilities for stack traces */
class LogDemangle
{
  public:
    /** Demangles a C++ symbol from an address
     * @param buffer Output buffer for demangled symbol
     * @param bufferSize Size of output buffer
     * @param address Address to demangle */
    static void demangleSymbol(char* buffer, size_t bufferSize, const void* address) noexcept;
    /** Checks if a symbol belongs to a system library
     * @param symbol The symbol to check
     * @return true if system library, false otherwise */
    static bool isSystemLibrary(const char* symbol) noexcept;
};

} // namespace rl

#endif // RL_LOG_LOG_DEMANGLE_H
