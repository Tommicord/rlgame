#pragma once

#include <vector>
#include <optional>
#include <map>

namespace Rl::World {

// Some forward references
class BaseUnit;
template<class K, class V>
class UnitRegistryKVPair;
class UnitResourceName;

template<class K, class V>
class UnitRegisters
{
    inline static std::vector<UnitRegistryKVPair<K, V>> registry;
public:
    /* Puts a Key-Value pair of Unit register */
    static void PutKV(UnitRegistryKVPair<K, V>& reg) noexcept;

    /* Returns the registry size */
    [[nodiscard]]
    size_t GetRegistrySize();

    /* Returns the current registry */
    [[nodiscard]]
    static
    std::vector<UnitRegistryKVPair<K, V>>& GetRegistry();

    /* This is only to the KV Pair access the PutKV method
     * When a register is created, automatically
     * Adds the KV pair to the registry */
    friend class UnitRegistryKVPair<K, V>;
};

template<class K, class V>
class UnitRegistryKVPair
{
protected:
    K regKey;
    V regValue;
public:
    /* Creates a basic register of world unit */
    explicit UnitRegistryKVPair(const K& defaultRegKey);

    /* Registers a Unit into the registry */
    void Register(int id, K& key, V& value);

    /* Gets the name we use to identify the object */
    [[nodiscard]]
    static std::optional<K> GetNameForObject(V& value);

    /* Gets the object from the name identifier */
    [[nodiscard]]
    static std::optional<V> GetObject(K name);

    /* Gets the object from the id identifier */
    [[nodiscard]]
    static std::optional<V> GetObjectById(int id);
};
template class UnitRegistryKVPair<UnitResourceName, BaseUnit *>;

} // namespace Rl::World