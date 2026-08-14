#ifndef RL_WORLD_PRE_BIOME_REGISTER_H
#define RL_WORLD_PRE_BIOME_REGISTER_H

/**
 * @file PreBiomeRegister.h
 * @brief Biome type registration with deterministic hash-based identification
 *
 * This file implements a hash-based identification system for Biome types,
 * similar to the PreRegister system for Units.
 *
 * BIOME TYPE ID GENERATION:
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

/** Register containing biome type information */
class PreBiomeRegister final : public PreRegister
{
  public:
    /** Constructs a PreBiomeRegister with type information
     * @param hash The 8-byte hash block for biome identification
     * @param typeId The unique type identifier */
    constexpr PreBiomeRegister(uint64_t hash, uint32_t typeId) : PreRegister(hash, typeId)
    {
    }
};

/** Factory class that generates hash-based PreBiomeRegister */
class PreBiomeRegisterFactory final
{
  public:
    /** Creates a PreBiomeRegister by generating deterministic hashes from type
     * name
     * @param typeName The type name from typeid(T).name() or a string literal
     * @return PreBiomeRegister with hash-based type information */
    static PreBiomeRegister create(const char* typeName);
};

} // namespace rl

#endif
