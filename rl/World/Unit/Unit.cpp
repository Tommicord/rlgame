import Rl.World.Unit.Unit;
import Rl.World.Unit.UnitRegister;
import Rl.World.Unit.UnitRegistry;
import Rl.World.Unit.UnitResourceName;

import <algorithm>;
import <cstring>;
import <memory>;
import <vector>;

namespace Rl::World
{

UnitResourceName::UnitResourceName(const std::vector<const char*>& name) noexcept
{
  constexpr int maxSize = 255;
  this->name            = new char[maxSize];
  this->name[0]         = 0x00;
  ConstructResourceName(name, maxSize);
}

UnitResourceName::~UnitResourceName()
{
  delete[] name;
}

void UnitResourceName::ConstructResourceName(
    const std::vector<const char*>& base, const size_t maxSize) noexcept
{
  if (!this->name)
    return;

  this->nameLen = 0;
  this->name[0] = 0x00;
  for (size_t i = 0; i < base.size(); ++i)
  {
    size_t count = std::strlen(base[i]);
    if (count > 255)
    {
      count = 255;
    }
    if (this->nameLen + count + 1 >= maxSize)
    {
      break;
    }
    // Append the string
    std::strcat(this->name, base[i]);
    this->nameLen += count;

    // Add dot separator if not the last element
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

const char* UnitResourceName::GetBaseResourceString()
{
  return BASE;
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

template <class K, class V>
void UnitRegisters<K, V>::PutKV(UnitRegistryKVPair<K, V>& reg) noexcept
{
  registry.push_back(reg);
}

template <class K, class V>
size_t UnitRegisters<K, V>::GetRegistrySize()
{
  return registry.size();
}

template <class K, class V>
std::vector<UnitRegistryKVPair<K, V>>& UnitRegisters<K, V>::GetRegistry()
{
  return registry;
}

template <class K, class V>
UnitRegistryKVPair<K, V>::UnitRegistryKVPair(const K& defaultRegKey) :
    regKey(defaultRegKey), regValue()
{
}

template <class K, class V>
void UnitRegistryKVPair<K, V>::Register(int id, K& key, V& value)
{
  if (&key == &this->regKey)
  {
    this->regValue = value;
  }
  UnitRegisters<K, V>::PutKV(*this);
}

template <class K, class V>
std::optional<K> UnitRegistryKVPair<K, V>::GetNameForObject(V& value)
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

template <class K, class V>
std::optional<V> UnitRegistryKVPair<K, V>::GetObject(K name)
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
std::optional<V> UnitRegistryKVPair<K, V>::GetObjectById(int id)
{
  // Access the static registry through UnitRegisters
  auto& registry = UnitRegisters<K, V>::GetRegistry();
  if (id >= 0 && static_cast<size_t>(id) < registry.size())
  {
    return registry[id].regValue;
  }
  return std::nullopt;
}

UnitTextureMaterial::UnitTextureMaterial(
    Texture2* top, Texture2* down, Texture2* left, Texture2* right, Texture2* front, Texture2* back)
{
  this->top   = top;
  this->down  = down;
  this->left  = left;
  this->right = right;
  this->front = front;
  this->back  = back;
  hasTexture  = true;
}

UnitTextureMaterial::~UnitTextureMaterial()
{
  if (!hasTexture)
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

BaseUnit::~BaseUnit()
{
  textures.reset();
}

UnitTextureMaterial& BaseUnit::GetMaterial() const
{
  return *textures;
}

void BaseUnit::SetResistance(const float resistance)
{
  this->unitResistance = resistance;
}

void BaseUnit::SetLightEmit(const float emit)
{
  this->lightEmit = emit;
}

void BaseUnit::SetLightOpacity(const float opacity)
{
  this->lightOpacity = opacity;
}

void BaseUnit::SetUnitHardness(const float hardness)
{
  this->unitHardness = hardness;
}

void BaseUnit::SetPolFenceRight(PolFence& fence)
{
  // Copies start from polTr address
  std::memcpy(&polTr, &fence, sizeof(fence));
}

void BaseUnit::SetPolFenceLeft(PolFence& fence)
{
  // Copies start from polTl address
  std::memcpy(&polTl, &fence, sizeof(fence));
}

void BaseUnit::SetPolCurve(float curve)
{
  this->polCurveV = curve;
}

void BaseUnit::EnableCollision()
{
  mustCollide = true;
}

void BaseUnit::DisableCollision()
{
  mustCollide = false;
}

bool BaseUnit::IsCollisionEnabled() const
{
  return mustCollide;
}

bool BaseUnit::IsVisible() const
{
  return mustVisible;
}

void BaseUnit::Update()
{
}

} // namespace Rl::World
