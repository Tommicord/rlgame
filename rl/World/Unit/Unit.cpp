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
  // Append to the start the prefix rl.unit
  std::vector<std::string_view> fullName;
  fullName.reserve(name.size() + 1);
  fullName.emplace_back(prefix);
  fullName.insert(fullName.end(), name.begin(), name.end());
  ConstructResourceName(fullName, maxSize);
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
    size_t           count = view.length();
    if (count > 255)
    {
      count = 255;
    }
    if (this->nameLen + count + 1 >= maxSize)
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

} // namespace Rl::World
