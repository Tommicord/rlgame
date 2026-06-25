export module Rl.World.Unit.UnitPropertyStrategy;

import <type_traits>;

namespace Rl::World
{

class BaseUnit;

template <class T>
export inline constexpr bool IsDerivedUnit = std::is_base_of_v<BaseUnit, T>;

template <class T>
  requires IsDerivedUnit<T>
export class UnitPropertyStrategy
{
  public:
  /* Gets the base light emission value for this unit type */
  [[nodiscard]]
  static constexpr float GetLightEmit()
  {
    return 0.0f;
  }

  /* Gets the light opacity (how much light is blocked) */
  [[nodiscard]]
  static constexpr float GetLightOpacity()
  {
    return 1.0f;
  }

  /* Gets the ambient occlusion factor */
  [[nodiscard]]
  static constexpr float GetAmbientOcclusion()
  {
    return 1.0f;
  }

  /* Gets the light absorption coefficient */
  [[nodiscard]]
  static constexpr float GetLightAbsorption()
  {
    return 0.0f;
  }

  /* Gets the light scattering coefficient */
  [[nodiscard]]
  static constexpr float GetLightScattering()
  {
    return 0.0f;
  }

  /* Gets the roughness value (0.0 = smooth, 1.0 = rough) */
  [[nodiscard]]
  static constexpr float GetRoughness()
  {
    return 0.5f;
  }

  /* Gets the metallic value (0.0 = dielectric, 1.0 = metallic) */
  [[nodiscard]]
  static constexpr float GetMetallic()
  {
    return 0.0f;
  }

  /* Gets the albedo color (RGB) */
  [[nodiscard]]
  static constexpr float GetAlbedoR()
  {
    return 1.0f;
  }
  [[nodiscard]]
  static constexpr float GetAlbedoG()
  {
    return 1.0f;
  }
  [[nodiscard]]
  static constexpr float GetAlbedoB()
  {
    return 1.0f;
  }

  /* Gets the reflectivity value */
  [[nodiscard]]
  static constexpr float GetReflectivity()
  {
    return 0.0f;
  }

  /* Gets the refractive index */
  [[nodiscard]]
  static constexpr float GetRefractiveIndex()
  {
    return 1.0f;
  }

  /* Gets the dirtiness level (0.0 = clean, 1.0 = very dirty) */
  [[nodiscard]]
  static constexpr float GetDirtiness()
  {
    return 0.0f;
  }

  /* Gets the wetness level (0.0 = dry, 1.0 = saturated) */
  [[nodiscard]]
  static constexpr float GetWetness()
  {
    return 0.0f;
  }

  /* Gets the temperature in Celsius */
  [[nodiscard]]
  static constexpr float GetTemperature()
  {
    return 20.0f;
  }

  /* Gets the humidity level */
  [[nodiscard]]
  static constexpr float GetHumidity()
  {
    return 0.5f;
  }

  /* Gets the hardness value (resistance to breaking) */
  [[nodiscard]]
  static constexpr float GetHardness()
  {
    return 1.0f;
  }

  /* Gets the explosion resistance */
  [[nodiscard]]
  static constexpr float GetExplosionResistance()
  {
    return 0.0f;
  }

  /* Gets the transparency level (0.0 = opaque, 1.0 = transparent) */
  [[nodiscard]]
  static constexpr float GetTransparency()
  {
    return 0.0f;
  }

  /* Gets the emissive intensity */
  [[nodiscard]]
  static constexpr float GetEmissiveIntensity()
  {
    return 0.0f;
  }

  /* Gets the subsurface scattering factor */
  [[nodiscard]]
  static constexpr float GetSubsurfaceScattering()
  {
    return 0.0f;
  }

  /* Gets the flammability (0.0 = non-flammable, 1.0 = highly flammable) */
  [[nodiscard]]
  static constexpr float GetFlammability()
  {
    return 0.0f;
  }

  /* Calculates combined light attenuation */
  [[nodiscard]]
  static constexpr float CalculateLightAttenuation()
  {
    return GetLightOpacity() + GetLightAbsorption();
  }

  /* Checks if unit is liquid */
  [[nodiscard]]
  static constexpr bool IsLiquid()
  {
    return false;
  }

  /* Checks if unit is gas */
  [[nodiscard]]
  static constexpr bool IsGas()
  {
    return false;
  }

  /* Checks if unit is solid */
  [[nodiscard]]
  static constexpr bool IsSolid()
  {
    return true;
  }
};

} // namespace Rl::World
