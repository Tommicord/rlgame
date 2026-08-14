#ifndef RL_WORLD_PRE_UNIT_REGISTER_H
#define RL_WORLD_PRE_UNIT_REGISTER_H

/**
 * @file PreUnitRegister.h
 * @brief Unit type registration with deterministic hash-based identification
 *
 * This file implements a hash-based identification system for Unit types,
 * similar to the PreBiomeRegister system for Biomes.
 *
 * UNIT TYPE ID GENERATION:
 * =========================
 *
 * Type IDs are generated deterministically from the type name using FNV-1a:
 * - Same type name always produces the same type ID
 * - Consistent across different compilation units
 * - Consistent across different game sessions
 * - Enables reliable file validation without storing additional metadata
 */

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>
#include <typeinfo>

#include "Rl.World/PreRegister.h"

namespace rl
{

/** Register containing unit type information */
class PreUnitRegister final : public PreRegister
{
  public:
    /** Constructs a PreUnitRegister with type information
     * @param hash The 8-byte hash block for unit identification
     * @param typeId The unique type identifier */
    constexpr PreUnitRegister(uint64_t hash, uint32_t typeId) : PreRegister(hash, typeId)
    {
    }
};

/** Factory class that generates hash-based PreUnitRegister */
class PreUnitRegisterFactory final
{
  public:
    /** Creates a PreUnitRegister by generating deterministic hashes from type
     * name
     * @param typeName The type name from typeid(T).name() or a string literal
     * @return PreUnitRegister with hash-based type information */
    static PreUnitRegister create(const char* typeName);
};

} // namespace rl

#endif
