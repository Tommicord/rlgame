#ifndef RL_WORLD_PRE_REGISTER_H
#define RL_WORLD_PRE_REGISTER_H

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>
#include <typeinfo>

namespace rl
{

namespace
{

/** Simple FNV-1a hash for runtime unit identification */
constexpr uint64_t hashTypeName(const char* str, size_t length)
{
        uint64_t hash = 14695981039346656037ULL;
        for (size_t i = 0; i < length; ++i)
        {
                hash ^= static_cast<uint64_t>(str[i]);
                hash *= 1099511628211ULL;
        }
        return hash;
}

/** Generate consistent type ID from type name hash */
inline uint32_t generateTypeIdFromHash(uint64_t hash)
{
        uint32_t typeId = static_cast<uint32_t>(hash ^ (hash >> 32));
        // Ensure non-zero
        if (typeId == 0)
                typeId = 1;
        return typeId;
}

} // namespace

/** Register containing unit type information */
class PreRegister
{
        public:
                /** Constructs a PreRegister with type information
                 * @param hash The 8-byte hash block for unit identification
                 * @param typeId The unique type identifier */
                constexpr PreRegister(uint64_t hash, uint32_t typeId) : hashValue(hash), id(typeId)
                {
                }

                /** Returns the unique type identifier
                 * @return Type ID */
                constexpr uint32_t getId() const
                {
                        return id;
                }

                /** Returns the unit hash
                 * @return Hash value */
                constexpr uint64_t getHash() const
                {
                        return hashValue;
                }

        private:
                uint64_t hashValue; /**< 8-byte hash block */
                uint32_t id; /**< Type identifier */
};

} // namespace rl

#endif
