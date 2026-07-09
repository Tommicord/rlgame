import Rl.World.Unit.UnitTextureFactory;

import Rl.Base.Texture2;
import Rl.World.Unit.UnitResourceName;
import Rl.World.Unit.UnitRegistry;
import Rl.World.Unit;
import Rl.RayLog.Macro;

import <string>;
import <vector>;
import <array>;

namespace Rl::World
{

void UnitTextureFactory::FromUnit(unsigned short id)
{
  // Load default texture
  static Providers::Texture2 texture{};
  const auto                 pixels(GetDefaultUnitTexture());
  texture.FromData(pixels.data(), 16, 16, Providers::Texture2Format::RGBA8, {});

  auto object = IUnit::Registry::GetObjectById(id);
  if (object.has_value())
  {
    const auto value_object = object.value();
    auto&      material = value_object->GetMaterial().GetTextures2();
    const auto wrapped_name = IUnit::Registry::GetNameForObject(object.value());

    if (wrapped_name.has_value())
    {
      const UnitResourceName& name = wrapped_name.value();
      const bool              isValid = IsValidResourceName(name);

      if (isValid)
      {
        // The IUnit owns the texture, don't need to free the memory
        auto unitTexture = new Providers::Texture2(name.GetResourceName());
        for (int i = 0; i < 6; ++i)
        {
          material.faces[i] = unitTexture;
        }
        value_object->GetMaterial().SetHeap(true);
        return;
      }
    }

    for (int i = 0; i < 6; ++i)
    {
      material.faces[i] = &texture;
    }
    value_object->GetMaterial().SetHeap(false);
  }
}

bool UnitTextureFactory::IsValidResourceName(const UnitResourceName& name)
{
  const std::vector<std::string>& parts = name.SplitResourceName();
  bool                            failure = false;
  if (!name.GetResourceName())
  {
    RayLog::LogWarning(RAY_LOG_TAG, "A Unit resource name cannot be null");
    return false;
  }
  bool expect = false; // false = separator, true = split item name
  for (int i = 0; i < parts.size(); ++i)
  {
    auto       str = parts[i];
    const bool isDot = str.starts_with('.');
    if (isDot)
    {
      if (i == 0)
      {
        RayLog::LogWarning(RAY_LOG_TAG,
            "Invalid resource name '%s', "
            "This should not happen!, A Unit resource name "
            "must not start with a dot",
            name.GetResourceName());
        failure = true;
        break;
      }
      expect = true;
    }
    else if (expect == true && !isDot)
    {
      RayLog::LogWarning(
          RAY_LOG_TAG, "Invalid Unit resource name '%s'", name.GetResourceName());
      failure = true;
    }
    else
    {
      expect = false;
    }
  }
  return !failure;
}

std::array<uint8_t, UnitTextureFactory::DEFAULT_TEXTURE2_LENGTH>
UnitTextureFactory::GetDefaultUnitTexture()
{
  std::array<uint8_t, DEFAULT_TEXTURE2_LENGTH> pixels{};
  constexpr int                                sidePixels = 8 * 4;
  bool                                         side = false; // false = left, true = right
  int                                          pixelIndex = 0;
  for (int i = 0; i < 16; ++i)
  {
    if (i == 7) // The row 8
    {
      side = true;
    }
    std::array<uint8_t, sidePixels> left{};
    constexpr uint8_t               black[4] = {0x00, 0x00, 0x00, 0xFF};
    for (int j = 0; j < 8; ++j)
    {
      int offset = j * 4;
      std::memcpy(left.data() + offset, black, sizeof(black));
    }

    std::array<uint8_t, sidePixels> right{};
    constexpr uint8_t               purple[4] = {0xFF, 0x00, 0xFF, 0xFF}; // RGBA
    for (int j = 0; j < 8; ++j)
    {
      int offset = j * 4;
      std::memcpy(right.data() + offset, purple, sizeof(purple));
    }
    if (side)
    {
      std::memcpy(pixels.data() + pixelIndex, left.data(), sizeof(left));
      pixelIndex += sidePixels;
      std::memcpy(pixels.data() + pixelIndex, right.data(), sizeof(right));
      pixelIndex += sidePixels;
    }
    else
    {
      std::memcpy(pixels.data() + pixelIndex, right.data(), sizeof(right));
      pixelIndex += sidePixels;
      std::memcpy(pixels.data() + pixelIndex, left.data(), sizeof(left));
      pixelIndex += sidePixels;
    }
  }
  return pixels;
}

} // namespace Rl::World
