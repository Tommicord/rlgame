#include "Rl.World/PreUnitRegister.h"
#include "Rl.World/PreRegister.h"

#include <memory>
#include <typeinfo>
#if defined(_RL_COMPILER_GCC) || defined(_RL_COMPILER_CLANG)
#include <cxxabi.h>
#endif

namespace rl
{

#if defined(_RL_COMPILER_GCC) || defined(_RL_COMPILER_CLANG)

namespace
{

const char* demangleTypeName(const char* typeName)
{
  int                                    status = -1;
  std::unique_ptr<char, void (*)(void*)> demangled(
      abi::__cxa_demangle(typeName, nullptr, nullptr, &status), std::free);
  return (status == 0) ? demangled.get() : typeName;
}
#endif

PreUnitRegister PreUnitRegisterFactory::create(const char* typeName)
{
#if defined(_RL_COMPILER_GCC) || defined(_RL_COMPILER_CLANG)
  const char* demangledName = demangleTypeName(typeName);
#else
  const char* demangledName = typeName;
#endif
  size_t nameLength = 0;

  while (demangledName[nameLength] != '\0')
    nameLength++;

  uint64_t runtimeHash = hashTypeName(demangledName, nameLength);
  uint32_t typeId      = generateTypeIdFromHash(runtimeHash);

  return PreUnitRegister(runtimeHash, typeId);
}

} // namespace rl
