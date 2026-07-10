export module Rl.World.Unit.UnitGrass;

import Rl.World.Unit;
import Rl.World.Unit.UnitRegister;
import Rl.World.Unit.UnitGrassGrowBehavior;
import Rl.World.Chunk.UnitChunkAccessor;
import Rl.World.Unit.UnitGrassGrowBehavior;

import <type_traits>;
import <string_view>;
import <memory>;

namespace Rl::World
{

export class IUnitGrowable
{
  public:
  /* Destructs a IUnitGrowable object */
  virtual ~IUnitGrowable() = default;

  /* Returns if the Grass unit can grow */
  virtual bool InGrowState() = 0;
};

export class UnitGrass final : public IUnit,
                               public IUnitGrowable,
                               public IUnitIdentifiable<UnitGrass>
{
  public:
  explicit UnitGrass(
      const Unit::GrassGrowConfig& config = Unit::GetGrassConfig()) noexcept;
  ~UnitGrass() override = default;

  /* Disable copy operations */
  UnitGrass(const UnitGrass&) = delete;
  UnitGrass& operator=(const UnitGrass&) = delete;

  /* Enable move operations */
  UnitGrass(UnitGrass&&) noexcept = delete;
  UnitGrass& operator=(UnitGrass&&) noexcept = delete;

  static constexpr float LIGHT_EMIT = 0.0f;
  static constexpr float LIGHT_OPACITY = 0.3f;
  static constexpr float AMBIENT_OCCLUSION = 0.6f;
  static constexpr float LIGHT_ABSORPTION = 0.1f;
  static constexpr float LIGHT_SCATTERING = 0.2f;
  static constexpr float ROUGHNESS = 0.9f;
  static constexpr float METALLIC = 0.0f;
  static constexpr float ALBEDO_R = 0.4f;
  static constexpr float ALBEDO_G = 0.7f;
  static constexpr float ALBEDO_B = 0.2f;
  static constexpr float REFLECTIVITY = 0.05f;
  static constexpr float REFRACTIVE_INDEX = 1.33f;
  static constexpr float DIRTINESS = 0.2f;
  static constexpr float MOISTURE = 0.7f;
  static constexpr float TEMPERATURE = 20.0f;
  static constexpr float HUMIDITY = 0.7f;
  static constexpr float HARDNESS = 0.1f;
  static constexpr float EXPLOSION_RESISTANCE = 0.0f;
  static constexpr float TRANSPARENCY = 0.0f;
  static constexpr float EMISSIVE_INTENSITY = 0.0f;
  static constexpr float SUBSURFACE_SCATTERING = 0.3f;
  static constexpr float FLAMMABILITY = 0.8f;

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

  /* Update growth configuration */
  void UpdateConfig(const Unit::GrassGrowConfig& newConfig);

  /* Describes if the block is in grow state */
  bool UnitGrass::InGrowState() override;

  /* Get current growth configuration */
  [[nodiscard]]
  const Unit::GrassGrowConfig& GetConfig() const;

  protected:
  [[nodiscard]]
  unsigned short GetDerivedClassId() const override
  { return IUnitIdentifiable<UnitGrass>::GetClassId(); }

  [[nodiscard]]
  std::string_view GetDerivedClassName() const override
  { return IUnitIdentifiable<UnitGrass>::SimpleClassName(); }

  std::unique_ptr<Unit::UnitGrassGrowBehavior> growBehavior;
};

} // namespace Rl::World
