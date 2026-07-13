export module Rl.World.Biome;

import Rl.World.Biome.BiomeRegistryGPU;
import Rl.Base.Game;
import Rl.Base.Binding;

import <cstdint>;
import <string>;
import <vector>;
import <memory>;

namespace Rl::World::Biome
{
/* Noise layer configuration for biome generation */
export struct BiomeNoiseLayer
{
  float    scale; // Noise frequency scale
  float    offsetX; // X offset for noise sampling
  float    offsetY; // Y offset for noise sampling
  float    offsetZ; // Z offset for noise sampling
  uint32_t octaves; // FBM octaves
  float    persistence; // FBM persistence
  float    lacunarity; // FBM lacunarity
  uint32_t noiseType; // 0: Standard, 1: FBM, 2: Ridged, 3: Turbulence
  float    weight; // Layer weight in final classification
};

export struct BiomeType
{
  uint32_t value;
};

/* Unit generation rule for a biome */
export struct BiomeUnitRule
{
  uint32_t unitId; // Unit ID to place
  float    minHeight; // Minimum height for this unit
  float    maxHeight; // Maximum height for this unit
  float    minTemperature; // Minimum temperature threshold
  float    maxTemperature; // Maximum temperature threshold
  float    minMoisture; // Minimum moisture threshold
  float    maxMoisture; // Maximum moisture threshold
  float    minElevation; // Minimum elevation threshold
  float    maxElevation; // Maximum elevation threshold
  float    probability; // Probability of placement (0.0-1.0)
  float    density; // Density of placement (0.0-1.0)
};

/* Pure virtual interface for biome definition */
export class IBiome
{
  private:
  /* The biome GPU registry */
  inline static auto registryGPU = std::make_shared<BiomeRegistryGPU>();
  public:
  template <typename Derived>
    requires std::is_base_of_v<IBiome, Derived>
  explicit IBiome(Derived* ptr) noexcept
  {
    if (!registryGPU->IsInitialized())
    {
      const Main::MainBinding& binding = Main::Game::GetInstance().GetMainBinding();
      registryGPU->Initialize(binding.device, binding.physicalDevice);
    }
    registryGPU->RegisterBiome(ptr);
  }
  virtual ~IBiome() = default;

  /* Gets the biome type identifier */
  [[nodiscard]]
  virtual BiomeType GetBiomeType() const = 0;

  /* Gets the biome name for debugging */
  [[nodiscard]]
  virtual const char* GetBiomeName() const = 0;

  /* Gets the temperature noise layer configuration */
  [[nodiscard]]
  virtual BiomeNoiseLayer GetTemperatureNoiseLayer() const = 0;

  /* Gets the moisture noise layer configuration */
  [[nodiscard]]
  virtual BiomeNoiseLayer GetMoistureNoiseLayer() const = 0;

  /* Gets the elevation noise layer configuration */
  [[nodiscard]]
  virtual BiomeNoiseLayer GetElevationNoiseLayer() const = 0;

  /* Gets the unit generation rules for this biome */
  [[nodiscard]]
  virtual const std::vector<BiomeUnitRule>& GetUnitRules() const = 0;

  /* Classifies a position as belonging to this biome based on noise values */
  [[nodiscard]]
  virtual bool
  BelongsToBiome(float temperature, float moisture, float elevation) const = 0;

  /* Gets the dominant unit ID for a given position in this biome */
  [[nodiscard]]
  virtual uint32_t GetDominantUnit(float temperature,
                                   float moisture,
                                   float elevation,
                                   float height) const = 0;

  static BiomeRegistryGPU& GetGPUIDRegistry() {
    return registryGPU.get()
  }
};

} // namespace Rl::World::Biome
