export module Rl.World.Unit.UnitRegistry;

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
  const K& regKey;
  V        regValue;

  public:
  /* Creates a basic register of world unit */
  explicit UnitRegistryPair3(const K& defaultRegKey);

  /* Registers a Unit into the registry */
  void Register(unsigned short id, K& key, V& value);

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
template class UnitRegistryPair3<UnitResourceName, IUnit*>;

} // namespace Rl::World
