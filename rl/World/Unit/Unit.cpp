import Rl.World.Unit;
import Rl.World.Unit.UnitRegister;
import Rl.World.Unit.UnitRegistry;
import Rl.World.Unit.UnitResourceName;
import Rl.World.Unit.UnitTextureFactory;
import Rl.Base.Game;
import Rl.Base.Binding;
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
  this->name            = new char[maxSize];
  this->name[0]         = 0x00;
  this->nameLen         = 0;
  // Append to the start the prefix rl.unit
  std::vector<std::string_view> fullName;
  fullName.reserve(name.size() + 1);
  fullName.emplace_back(prefix);
  fullName.insert(fullName.end(), name.begin(), name.end());
  ConstructResourceName(fullName, maxSize);
  // Validate nameLen after construction
  if (this->nameLen >= maxSize)
  {
    this->nameLen             = maxSize - 1;
    this->name[this->nameLen] = 0x00;
  }
}

UnitResourceName::~UnitResourceName()
{ delete[] name; }

UnitResourceName::UnitResourceName(const UnitResourceName& other) : nameLen(other.nameLen)
{
  if (other.name && other.nameLen > 0)
  {
    name = new char[other.nameLen + 1];
    std::memcpy(name, other.name, other.nameLen);
    name[other.nameLen] = 0x00;
  }
  else
  {
    name = nullptr;
  }
}

UnitResourceName& UnitResourceName::operator=(const UnitResourceName& other)
{
  if (this != &other)
  {
    delete[] name;
    nameLen = other.nameLen;
    if (other.name && other.nameLen > 0)
    {
      name = new char[other.nameLen + 1];
      std::memcpy(name, other.name, other.nameLen);
      name[other.nameLen] = 0x00;
    }
    else
    {
      name = nullptr;
    }
  }
  return *this;
}

UnitResourceName::UnitResourceName(UnitResourceName&& other) noexcept :
    name(other.name), nameLen(other.nameLen)
{
  other.name    = nullptr;
  other.nameLen = 0;
}

UnitResourceName& UnitResourceName::operator=(UnitResourceName&& other) noexcept
{
  if (this != &other)
  {
    delete[] name;
    name          = other.name;
    nameLen       = other.nameLen;
    other.name    = nullptr;
    other.nameLen = 0;
  }
  return *this;
}

void UnitResourceName::ConstructResourceName(const std::vector<std::string_view>& base,
                                             const size_t maxSize) noexcept
{
  if (!this->name)
    return;
  this->nameLen = 0;
  this->name[0] = 0x00;
  for (size_t i = 0; i < base.size(); ++i)
  {
    std::string_view view  = base[i];
    size_t           count = view.length();
    if (count > maxSize)
    {
      count = maxSize;
    }
    if (this->nameLen + count + 1 > maxSize)
    {
      break;
    }
    std::memcpy(this->name + this->nameLen, view.data(), count);
    this->nameLen += count;
    if (i < base.size() - 1)
    {
      this->name[this->nameLen] = '.';
      this->nameLen += 1;
    }
  }
  this->name[this->nameLen] = 0x00;
}
std::vector<std::string> UnitResourceName::SplitResourceName() const
{
  if (!name || nameLen == 0)
    return {};
  constexpr size_t maxSafeLength = 1024;
  if (nameLen > maxSafeLength)
    return {};
  std::string              nm(name, nameLen);
  std::vector<std::string> res;
  // Reserve space to avoid reallocations
  const size_t dotcount =
      std::ranges::count_if(nm, [](const char c) { return c == '.'; });
  res.reserve(dotcount + 1);
  size_t start = 0;
  for (size_t i = 0; i < nm.size(); ++i)
  {
    if (nm[i] == '.' || i == nm.size() - 1)
    {
      const size_t length = (nm[i] == '.') ? (i - start) : (i - start + 1);
      if (length > 0)
      {
        res.push_back(nm.substr(start, length));
      }
      start = i + 1;
    }
  }
  return res;
}

char* UnitResourceName::GetResourceName() const
{ return name; }

size_t UnitResourceName::GetResourceNameLength() const
{ return nameLen; }

bool UnitResourceName::Equals(const UnitResourceName& resource) const
{
  if (&resource == this)
    return true;
  if (this->nameLen != resource.nameLen)
    return false;
  return std::memcmp(this->name, resource.name, this->nameLen);
}

UnitTextureMaterial::~UnitTextureMaterial()
{
  if (heap)
  {
    for (auto& face : textures.faces)
    {
      delete face;
    }
  }
}

template <typename Derived> void IUnit::RegistryDerivedGPU(Derived* ptr)
{
  if (!registryGPU->IsInitialized())
  {
    const Main::MainBinding bindings = Main::Game::GetInstance().GetMainBinding();
    registryGPU->Initialize(bindings.device, bindings.physicalDevice);
  }
  registryGPU->Register<Derived>(ptr);
}

void IUnit::RegisterDerivedCallback(unsigned short id)
{ UnitTextureFactory::FromUnit(id); }

} // namespace Rl::World
