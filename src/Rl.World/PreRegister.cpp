#include "Rl.World/PreRegister.h"
#include "Rl.World/UnitHash.h"

#include <memory>
#include <typeinfo>
#if defined(_RL_COMPILER_GCC) || defined(_RL_COMPILER_CLANG)
#include <cxxabi.h>
#endif

namespace rl
{

#if defined(_RL_COMPILER_GCC) || defined(_RL_COMPILER_CLANG)

const char* demangleTypeName(const char* typeName)
{
  int                                    status = -1;
  std::unique_ptr<char, void (*)(void*)> demangled(
      abi::__cxa_demangle(typeName, nullptr, nullptr, &status), std::free);
  return (status == 0) ? demangled.get() : typeName;
}
#endif

} // namespace rl
