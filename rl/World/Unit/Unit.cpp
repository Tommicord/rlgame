import Rl.World.Unit;
import Rl.World.Unit.UnitRegister;
import Rl.World.Unit.UnitRegistry;
import Rl.World.Unit.UnitResourceName;
import Rl.Base.Texture2;

import <algorithm>;
import <cstring>;
import <string>;
import <memory>;
import <vector>;
import <optional>;

namespace Rl::World
{

UnitResourceName::UnitResourceName(const std::vector<std::string_view>& name) noexcept
{
  constexpr int maxSize = 255;
  this->name = new char[maxSize];
  this->name[0] = 0x00;
  this->nameLen = 0;
  ConstructResourceName(name, maxSize);
}

UnitResourceName::~UnitResourceName()
{
  delete[] name;
}

void UnitResourceName::ConstructResourceName(
    const std::vector<std::string_view>& base, const size_t maxSize) noexcept
{
  if (!this->name)
    return;
  this->nameLen = 0;
  this->name[0] = 0x00;
  for (size_t i = 0; i < base.size(); ++i)
  {
    std::string_view view = base[i];
    size_t           count = std::strlen(view.data());
    if (count > 255)
    {
      count = 255;
    }
    if (this->nameLen + count + 1 >= maxSize)
    {
      break;
    }
    this->name[count] = '.';
    std::strcat(this->name, view.data());
    this->nameLen += count;
    if (i < base.size() - 1)
    {
      std::strcat(this->name, ".");
      this->nameLen += 1;
    }
  }
}
std::vector<char*> UnitResourceName::SplitResourceName() const
{
  std::string        nm(name);
  std::vector<char*> res;
  // Reserve space to avoid reallocations
  const size_t dotcount = std::ranges::count_if(nm, [](const char c) { return c == '.'; });
  res.reserve(dotcount + 1);
  size_t start = 0;
  for (size_t i = 0; i < nm.size(); ++i)
  {
    if (nm[i] == '.' || i == nm.size() - 1)
    {
      size_t length = (nm[i] == '.') ? (i - start) : (i - start + 1);
      if (length > 0)
      {
        auto segment = new char[length + 1];
        std::memcpy(segment, nm.data() + start, length);
        segment[length] = 0x00;
        res.push_back(segment);
      }
      start = i + 1;
    }
  }

  return res;
}

char* UnitResourceName::GetResourceName() const
{
  return name;
}

size_t UnitResourceName::GetResourceNameLength() const
{
  return nameLen;
}

bool UnitResourceName::Equals(const UnitResourceName& resource) const
{
  if (&resource == this)
    return true;
  if (this->nameLen != resource.nameLen)
    return false;
  return std::memcmp(this->name, resource.name, this->nameLen);
}

template <class K, class V> void UnitRegisters<K, V>::PutPair(UnitRegistryPair3<K, V>& reg) noexcept
{
  registry.emplace_back(reg);
}

template <class K, class V> size_t UnitRegisters<K, V>::GetRegistrySize()
{
  return registry.size();
}

template <class K, class V>
const std::vector<UnitRegistryPair3<K, V>>& UnitRegisters<K, V>::GetRegistry()
{
  return registry;
}

template <class K, class V>
UnitRegistryPair3<K, V>::UnitRegistryPair3(const K& defaultRegKey) :
    regId(-1), regKey(defaultRegKey), regValue()
{
}

template <class K, class V>
void UnitRegistryPair3<K, V>::Register(unsigned short id, K& key, V& value)
{
  if (&key == &this->regKey)
  {
    this->regValue = value;
  }
  this->regId = id;

  /* Finally register the pair */
  UnitRegisters<K, V>::PutPair(*this);
}

template <class K, class V> std::optional<K> UnitRegistryPair3<K, V>::GetNameForObject(V& value)
{
  // Access the static registry through UnitRegisters
  auto& registry = UnitRegisters<K, V>::GetRegistry();

  for (const auto& pair : registry)
  {
    if (pair.regValue == value)
    {
      return pair.regKey;
    }
  }
  return std::nullopt;
}

template <class K, class V> std::optional<V> UnitRegistryPair3<K, V>::GetObject(K name)
{
  // Access the static registry through UnitRegisters
  auto& registry = UnitRegisters<K, V>::GetRegistry();

  for (const auto& pair : registry)
  {
    if (&pair.regKey == &name)
    {
      return pair.regValue;
    }
  }
  return std::nullopt;
}

template <class K, class V>
std::optional<V> UnitRegistryPair3<K, V>::GetObjectById(unsigned short id)
{
  // Access the static registry through UnitRegisters
  auto& registry = UnitRegisters<K, V>::GetRegistry();
  for (const auto& reg : registry)
  {
    if (reg.regId == id)
    {
      return reg;
    }
  }
  return std::nullopt;
}

UnitTextureMaterial::UnitTextureMaterial(Providers::Texture2* top,
    Providers::Texture2*                                      down,
    Providers::Texture2*                                      left,
    Providers::Texture2*                                      right,
    Providers::Texture2*                                      front,
    Providers::Texture2*                                      back)
{
  this->top = top;
  this->down = down;
  this->left = left;
  this->right = right;
  this->front = front;
  this->back = back;
  hasMaterial = true;
}

UnitTextureMaterial::~UnitTextureMaterial()
{
  if (!hasMaterial)
    return;
  delete top;
  top = nullptr;
  delete down;
  down = nullptr;
  delete left;
  left = nullptr;
  delete right;
  right = nullptr;
  delete front;
  front = nullptr;
  delete back;
  back = nullptr;
}
template <class Derived> IUnit<Derived>::~IUnit()
{
  textures.reset();
}

template <class Derived> UnitTextureMaterial& IUnit<Derived>::GetMaterial() const
{
  return *textures;
}

template <class Derived> void IUnit<Derived>::SetResistance(const float resistance)
{
  this->unitResistance = resistance;
}

template <class Derived> void IUnit<Derived>::SetLightEmit(const float emit)
{
  this->lightEmit = emit;
}

template <class Derived> void IUnit<Derived>::SetLightOpacity(const float opacity)
{
  this->lightOpacity = opacity;
}

template <class Derived> void IUnit<Derived>::SetUnitHardness(const float hardness)
{
  this->unitHardness = hardness;
}

template <class Derived> void IUnit<Derived>::SetPolFenceRight(const PolFence& fence)
{
  // Copies registry from polTr address
  std::memcpy(&polTr, &fence, sizeof(fence));
}

template <class Derived> void IUnit<Derived>::SetPolFenceLeft(const PolFence& fence)
{
  // Copies registry from polTl address
  std::memcpy(&polTl, &fence, sizeof(fence));
}

template <class Derived> void IUnit<Derived>::SetPolCurve(const float curve)
{
  this->polCurveV = curve;
}

template <class Derived> void IUnit<Derived>::EnableCollision()
{
  mustCollide = true;
}

template <class Derived> void IUnit<Derived>::DisableCollision()
{
  mustCollide = false;
}

template <class Derived> bool IUnit<Derived>::IsCollisionEnabled() const
{
  return mustCollide;
}

template <class Derived> bool IUnit<Derived>::IsVisible() const
{
  return mustVisible;
}

template <class Derived> void IUnit<Derived>::Update()
{
}

} // namespace Rl::World
