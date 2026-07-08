export module Rl.World.Unit.UnitRegistry;

import Rl.World.Unit.UnitResourceName;

import <map>;
import <optional>;
import <vector>;

namespace Rl::World
{

// Some forward references
class IUnit;

export template <class K, class V>
class UnitRegistryPair3;

class UnitResourceName;

export template <class K, class V>
class UnitRegisters
{
  public:
  /* The pair type for the registry */
  using Pair = UnitRegistryPair3<K, V>;

  /* Start of pairs */
  static std::vector<Pair> registry;

  /* Puts a Key-Value pair of Unit register */
  static void PutPair(UnitRegistryPair3<K, V>& reg) noexcept;

  /* Returns the registry size */
  [[nodiscard]]
  size_t GetRegistrySize();

  /* Returns the current registry */
  [[nodiscard]]
  static const std::vector<UnitRegistryPair3<K, V>>& GetRegistry();

  /* This is only to the KV Pair access the PutPair method
   * When a register is created, automatically
   * Adds the KV pair to the registry */
  friend class UnitRegistryPair3<K, V>;
};

export template <class K, class V>
class UnitRegistryPair3
{
  protected:
  short    regId;
  K        regKey;
  V        regValue;

  public:
  /* Creates a basic register of world unit */
  explicit UnitRegistryPair3(const K& defaultRegKey);

  /* Registers a Unit into the registry */
  void Register(unsigned short id, const K& key, V& value);

  /* Gets the name we use to identify the object */
  [[nodiscard]]
  static std::optional<K> GetNameForObject(V& value);

  /* Gets the object from the name identifier */
  [[nodiscard]]
  static std::optional<V> GetObject(K name);

  /* Gets the object from the id identifier */
  [[nodiscard]]
  static std::optional<V> GetObjectById(unsigned short id);
};

template <class K, class V>
UnitRegistryPair3<K, V>::UnitRegistryPair3(const K& defaultRegKey) : regId(0), regKey(defaultRegKey), regValue(V{})
{
  UnitRegisters<K, V>::PutPair(*this);
}

template <class K, class V>
void UnitRegistryPair3<K, V>::Register(unsigned short id, const K& key, V& value)
{
  regId    = id;
  regKey   = key;
  regValue = value;
  UnitRegisters<K, V>::PutPair(*this);
}

template <class K, class V>
std::optional<K> UnitRegistryPair3<K, V>::GetNameForObject(V& value)
{
  for (const auto& pair : UnitRegisters<K, V>::GetRegistry())
  {
    if (pair.regValue == value)
    {
      return std::optional<K>(pair.regKey);
    }
  }
  return std::nullopt;
}

template <class K, class V>
std::optional<V> UnitRegistryPair3<K, V>::GetObject(K name)
{
  for (const auto& pair : UnitRegisters<K, V>::GetRegistry())
  {
    if (pair.regKey == name)
    {
      return std::optional<V>(pair.regValue);
    }
  }
  return std::nullopt;
}

template <class K, class V>
std::optional<V> UnitRegistryPair3<K, V>::GetObjectById(unsigned short id)
{
  for (const auto& pair : UnitRegisters<K, V>::GetRegistry())
  {
    if (pair.regId == id)
    {
      return pair.regValue;
    }
  }
  return std::nullopt;
}

template <class K, class V>
std::vector<UnitRegistryPair3<K, V>> UnitRegisters<K, V>::registry;

template <class K, class V>
void UnitRegisters<K, V>::PutPair(UnitRegistryPair3<K, V>& reg) noexcept
{
  registry.push_back(reg);
}

template <class K, class V>
size_t UnitRegisters<K, V>::GetRegistrySize()
{
  return registry.size();
}

template <class K, class V>
const std::vector<UnitRegistryPair3<K, V>>& UnitRegisters<K, V>::GetRegistry()
{
  return registry;
}

template class UnitRegistryPair3<UnitResourceName, IUnit*>;

} // namespace Rl::World
