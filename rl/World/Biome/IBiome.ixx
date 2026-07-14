export module Rl.World.Biome;

import Rl.Base.Game;
import Rl.Base.Binding;
import Rl.World.Biome.BiomeRegistryGPU;

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

  static BiomeRegistryGPU* GetGPUIDRegistry()
  {
    return registryGPU.get();
  }

  [[nodiscard]]
  BiomeNoiseLayer GetTemperatureNoiseLayer() const
  {
    return temperatureLayer;
  }

  [[nodiscard]]
  BiomeNoiseLayer GetMoistureNoiseLayer() const
  {
    return moistureLayer;
  }

  [[nodiscard]]
  BiomeNoiseLayer GetElevationNoiseLayer() const
  {
    return elevationLayer;
  }

  [[nodiscard]]
  const std::vector<BiomeUnitRule>& GetUnitRules() const
  {
    return unitRules;
  }

  /* Default classification based on threshold ranges */
  [[nodiscard]]
  bool BelongsToBiome(float temperature, float moisture, float elevation) const
  {
    return (temperature >= minTemperature && temperature <= maxTemperature) &&
           (moisture >= minMoisture && moisture <= maxMoisture) &&
           (elevation >= minElevation && elevation <= maxElevation);
  }

  /* Default unit selection based on matching rules */
  [[nodiscard]]
  uint32_t
  GetDominantUnit(float temperature, float moisture, float elevation, float height) const
  {
    uint32_t bestUnitId = 0; // Default to air/unknown
    float    bestScore  = 0.0f;

    for (const auto& rule : unitRules)
    {
      // Check if height matches
      if (height < rule.minHeight || height > rule.maxHeight)
        continue;

      // Check if temperature matches
      if (temperature < rule.minTemperature || temperature > rule.maxTemperature)
        continue;

      // Check if moisture matches
      if (moisture < rule.minMoisture || moisture > rule.maxMoisture)
        continue;

      // Check if elevation matches
      if (elevation < rule.minElevation || elevation > rule.maxElevation)
        continue;

      // Calculate score based on probability and density
      float score = rule.probability * rule.density;

      if (score > bestScore)
      {
        bestScore  = score;
        bestUnitId = rule.unitId;
      }
    }

    return bestUnitId;
  }

  // Configuration methods
  void SetTemperatureNoiseLayer(const BiomeNoiseLayer& layer)
  {
    temperatureLayer = layer;
  }

  void SetMoistureNoiseLayer(const BiomeNoiseLayer& layer)
  {
    moistureLayer = layer;
  }

  void SetElevationNoiseLayer(const BiomeNoiseLayer& layer)
  {
    elevationLayer = layer;
  }

  void AddUnitRule(const BiomeUnitRule& rule)
  {
    unitRules.push_back(rule);
  }

  void SetTemperatureThresholds(float minTemp, float maxTemp)
  {
    minTemperature = minTemp;
    maxTemperature = maxTemp;
  }

  void SetMoistureThresholds(float minMoist, float maxMoist)
  {
    minMoisture = minMoist;
    maxMoisture = maxMoist;
  }

  void SetElevationThresholds(float minElev, float maxElev)
  {
    minElevation = minElev;
    maxElevation = maxElev;
  }

  protected:
  BiomeType   biomeType;
  const char* biomeName;

  BiomeNoiseLayer            temperatureLayer{};
  BiomeNoiseLayer            moistureLayer{};
  BiomeNoiseLayer            elevationLayer{};
  std::vector<BiomeUnitRule> unitRules;

  // Threshold ranges for biome classification
  float minTemperature = 0.0f;
  float maxTemperature = 1.0f;
  float minMoisture    = 0.0f;
  float maxMoisture    = 1.0f;
  float minElevation   = 0.0f;
  float maxElevation   = 1.0f;
};

} // namespace Rl::World::Biome
