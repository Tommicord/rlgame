export module Rl.World.Unit.UnitMantle;

import Rl.World.Unit;
import Rl.World.Unit.UnitRegister;
import Rl.World.Chunk.UnitChunkAccessor;
import Rl.World.Unit.UnitGrassGrowBehavior;

import <type_traits>;
import <string_view>;
import <memory>;

namespace Rl::World
{

export class UnitDeepMantle final : public IUnit,
                                    public IUnitIdentifiable<UnitDeepMantle>
{
  public:
  explicit UnitDeepMantle() noexcept :
      IUnit(IUnitIdentifiable<UnitDeepMantle>::GetClassId()),
      IUnitIdentifiable<UnitDeepMantle>()
  { RegisterDerived<UnitDeepMantle>(*this); }

  ~UnitDeepMantle() override = default;

  /* Disable copy operations */
  UnitDeepMantle(const UnitDeepMantle&) = delete;
  UnitDeepMantle& operator=(const UnitDeepMantle&) = delete;

  /* Enable move operations */
  UnitDeepMantle(UnitDeepMantle&&) noexcept = delete;
  UnitDeepMantle& operator=(UnitDeepMantle&&) noexcept = delete;

  static constexpr float LIGHT_EMIT = 0.5f;
  static constexpr float LIGHT_OPACITY = 1.0f;
  static constexpr float AMBIENT_OCCLUSION = 1.0f;
  static constexpr float LIGHT_ABSORPTION = 0.8f;
  static constexpr float LIGHT_SCATTERING = 0.0f;
  static constexpr float ROUGHNESS = 0.7f;
  static constexpr float METALLIC = 0.3f;
  static constexpr float ALBEDO_R = 0.8f;
  static constexpr float ALBEDO_G = 0.3f;
  static constexpr float ALBEDO_B = 0.1f;
  static constexpr float REFLECTIVITY = 0.2f;
  static constexpr float REFRACTIVE_INDEX = 1.5f;
  static constexpr float DIRTINESS = 0.0f;
  static constexpr float MOISTURE = 0.0f;
  static constexpr float TEMPERATURE = 800.0f;
  static constexpr float HUMIDITY = 0.0f;
  static constexpr float HARDNESS = 10.0f;
  static constexpr float EXPLOSION_RESISTANCE = 5.0f;
  static constexpr float TRANSPARENCY = 0.0f;
  static constexpr float EMISSIVE_INTENSITY = 0.5f;
  static constexpr float SUBSURFACE_SCATTERING = 0.0f;
  static constexpr float FLAMMABILITY = 0.0f;

  [[nodiscard]]
  static constexpr float GetLightEmit()
  { return LIGHT_EMIT; }

  [[nodiscard]]
  static constexpr float GetLightOpacity()
  { return LIGHT_OPACITY; }

  [[nodiscard]]
  static constexpr float GetAmbientOcclusion()
  { return AMBIENT_OCCLUSION; }

  [[nodiscard]]
  static constexpr float GetLightAbsorption()
  { return LIGHT_ABSORPTION; }

  [[nodiscard]]
  static constexpr float GetLightScattering()
  { return LIGHT_SCATTERING; }

  [[nodiscard]]
  static constexpr float GetRoughness()
  { return ROUGHNESS; }

  [[nodiscard]]
  static constexpr float GetMetallic()
  { return METALLIC; }

  [[nodiscard]]
  static constexpr float GetAlbedoR()
  { return ALBEDO_R; }

  [[nodiscard]]
  static constexpr float GetAlbedoG()
  { return ALBEDO_G; }

  [[nodiscard]]
  static constexpr float GetAlbedoB()
  { return ALBEDO_B; }

  [[nodiscard]]
  static constexpr float GetReflectivity()
  { return REFLECTIVITY; }

  [[nodiscard]]
  static constexpr float GetRefractiveIndex()
  { return REFRACTIVE_INDEX; }

  [[nodiscard]]
  static constexpr float GetDirtiness()
  { return DIRTINESS; }

  [[nodiscard]]
  static constexpr float GetMoisture()
  { return MOISTURE; }

  [[nodiscard]]
  static constexpr float GetTemperature()
  { return TEMPERATURE; }

  [[nodiscard]]
  static constexpr float GetHumidity()
  { return HUMIDITY; }

  [[nodiscard]]
  static constexpr float GetHardness()
  { return HARDNESS; }

  [[nodiscard]]
  static constexpr float GetExplosionResistance()
  { return EXPLOSION_RESISTANCE; }

  [[nodiscard]]
  static constexpr float GetTransparency()
  { return TRANSPARENCY; }

  [[nodiscard]]
  static constexpr float GetEmissiveIntensity()
  { return EMISSIVE_INTENSITY; }

  [[nodiscard]]
  static constexpr float GetSubsurfaceScattering()
  { return SUBSURFACE_SCATTERING; }

  [[nodiscard]]
  static constexpr float GetFlammability()
  { return FLAMMABILITY; }

  [[nodiscard]]
  static constexpr bool IsLiquid()
  { return false; }

  [[nodiscard]]
  static constexpr bool IsGas()
  { return false; }

  [[nodiscard]]
  static constexpr bool IsSolid()
  { return true; }

  /* Update grass growth behavior */
  void Update(Chunk::UnitChunkAccessor& accessor) const;

  /* Get current growth configuration */
  [[nodiscard]]
  const Unit::GrassGrowConfig& GetConfig() const;

  protected:
  [[nodiscard]]
  unsigned short GetDerivedClassId() const override
  { return IUnitIdentifiable<UnitDeepMantle>::GetClassId(); }

  [[nodiscard]]
  std::string_view GetDerivedClassName() const override
  { return IUnitIdentifiable<UnitDeepMantle>::SimpleClassName(); }
};

export class UnitMantle final : public IUnit,
                                public IUnitIdentifiable<UnitMantle>
{
  public:
  explicit UnitMantle() noexcept :
      IUnit(IUnitIdentifiable<UnitMantle>::GetClassId()), IUnitIdentifiable<UnitMantle>()
  { RegisterDerived<UnitMantle>(*this); }

  ~UnitMantle() override = default;

  /* Disable copy operations */
  UnitMantle(const UnitMantle&) = delete;
  UnitMantle& operator=(const UnitMantle&) = delete;

  /* Enable move operations */
  UnitMantle(UnitMantle&&) noexcept = delete;
  UnitMantle& operator=(UnitMantle&&) noexcept = delete;

  static constexpr float LIGHT_EMIT = 0.1f;
  static constexpr float LIGHT_OPACITY = 1.0f;
  static constexpr float AMBIENT_OCCLUSION = 0.9f;
  static constexpr float LIGHT_ABSORPTION = 0.5f;
  static constexpr float LIGHT_SCATTERING = 0.0f;
  static constexpr float ROUGHNESS = 0.8f;
  static constexpr float METALLIC = 0.2f;
  static constexpr float ALBEDO_R = 0.6f;
  static constexpr float ALBEDO_G = 0.5f;
  static constexpr float ALBEDO_B = 0.4f;
  static constexpr float REFLECTIVITY = 0.1f;
  static constexpr float REFRACTIVE_INDEX = 1.45f;
  static constexpr float DIRTINESS = 0.1f;
  static constexpr float MOISTURE = 0.1f;
  static constexpr float TEMPERATURE = 200.0f;
  static constexpr float HUMIDITY = 0.1f;
  static constexpr float HARDNESS = 5.0f;
  static constexpr float EXPLOSION_RESISTANCE = 2.0f;
  static constexpr float TRANSPARENCY = 0.0f;
  static constexpr float EMISSIVE_INTENSITY = 0.1f;
  static constexpr float SUBSURFACE_SCATTERING = 0.0f;
  static constexpr float FLAMMABILITY = 0.0f;

  [[nodiscard]]
  static constexpr float GetLightEmit()
  { return LIGHT_EMIT; }

  [[nodiscard]]
  static constexpr float GetLightOpacity()
  { return LIGHT_OPACITY; }

  [[nodiscard]]
  static constexpr float GetAmbientOcclusion()
  { return AMBIENT_OCCLUSION; }

  [[nodiscard]]
  static constexpr float GetLightAbsorption()
  { return LIGHT_ABSORPTION; }

  [[nodiscard]]
  static constexpr float GetLightScattering()
  { return LIGHT_SCATTERING; }

  [[nodiscard]]
  static constexpr float GetRoughness()
  { return ROUGHNESS; }

  [[nodiscard]]
  static constexpr float GetMetallic()
  { return METALLIC; }

  [[nodiscard]]
  static constexpr float GetAlbedoR()
  { return ALBEDO_R; }

  [[nodiscard]]
  static constexpr float GetAlbedoG()
  { return ALBEDO_G; }

  [[nodiscard]]
  static constexpr float GetAlbedoB()
  { return ALBEDO_B; }

  [[nodiscard]]
  static constexpr float GetReflectivity()
  { return REFLECTIVITY; }

  [[nodiscard]]
  static constexpr float GetRefractiveIndex()
  { return REFRACTIVE_INDEX; }

  [[nodiscard]]
  static constexpr float GetDirtiness()
  { return DIRTINESS; }

  [[nodiscard]]
  static constexpr float GetMoisture()
  { return MOISTURE; }

  [[nodiscard]]
  static constexpr float GetTemperature()
  { return TEMPERATURE; }

  [[nodiscard]]
  static constexpr float GetHumidity()
  { return HUMIDITY; }

  [[nodiscard]]
  static constexpr float GetHardness()
  { return HARDNESS; }

  [[nodiscard]]
  static constexpr float GetExplosionResistance()
  { return EXPLOSION_RESISTANCE; }

  [[nodiscard]]
  static constexpr float GetTransparency()
  { return TRANSPARENCY; }

  [[nodiscard]]
  static constexpr float GetEmissiveIntensity()
  { return EMISSIVE_INTENSITY; }

  [[nodiscard]]
  static constexpr float GetSubsurfaceScattering()
  { return SUBSURFACE_SCATTERING; }

  [[nodiscard]]
  static constexpr float GetFlammability()
  { return FLAMMABILITY; }

  [[nodiscard]]
  static constexpr bool IsLiquid()
  { return false; }

  [[nodiscard]]
  static constexpr bool IsGas()
  { return false; }

  [[nodiscard]]
  static constexpr bool IsSolid()
  { return true; }

  /* Update grass growth behavior */
  void Update(Chunk::UnitChunkAccessor& accessor) const;

  /* Get current growth configuration */
  [[nodiscard]]
  const Unit::GrassGrowConfig& GetConfig() const;

  protected:
  [[nodiscard]]
  unsigned short GetDerivedClassId() const override
  { return IUnitIdentifiable<UnitMantle>::GetClassId(); }

  [[nodiscard]]
  std::string_view GetDerivedClassName() const override
  { return IUnitIdentifiable<UnitMantle>::SimpleClassName(); }
};

} // namespace Rl::World
