export module Rl.World.Biome.BaseBiome;

import Rl.World.Biome.IBiome;
import <algorithm>;
import <cmath>;

namespace Rl::World::Biome
{

/* Base biome implementation with common functionality */
export class BaseBiome : public IBiome
{
  public:
  BaseBiome(BiomeType type, const char* name) : biomeType(type), biomeName(name)
  {
  }

  virtual ~BaseBiome() = default;

  // IBiome interface implementations
  [[nodiscard]]
  BiomeType GetBiomeType() const override
  { return biomeType; }

  [[nodiscard]]
  const char* GetBiomeName() const override
  { return biomeName; }

  [[nodiscard]]
  BiomeNoiseLayer GetTemperatureNoiseLayer() const override
  { return temperatureLayer; }

  [[nodiscard]]
  BiomeNoiseLayer GetMoistureNoiseLayer() const override
  { return moistureLayer; }

  [[nodiscard]]
  BiomeNoiseLayer GetElevationNoiseLayer() const override
  { return elevationLayer; }

  [[nodiscard]]
  const std::vector<BiomeUnitRule>& GetUnitRules() const override
  { return unitRules; }

  /* Default classification based on threshold ranges */
  [[nodiscard]]
  bool BelongsToBiome(float temperature, float moisture, float elevation) const override
  {
    return (temperature >= minTemperature && temperature <= maxTemperature) &&
           (moisture >= minMoisture && moisture <= maxMoisture) &&
           (elevation >= minElevation && elevation <= maxElevation);
  }

  /* Default unit selection based on matching rules */
  [[nodiscard]]
  uint32_t GetDominantUnit(
      float temperature, float moisture, float elevation, float height) const override
  {
    uint32_t bestUnitId = 0; // Default to air/unknown
    float    bestScore = 0.0f;

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
        bestScore = score;
        bestUnitId = rule.unitId;
      }
    }

    return bestUnitId;
  }

  // Configuration methods
  void SetTemperatureNoiseLayer(const BiomeNoiseLayer& layer)
  { temperatureLayer = layer; }

  void SetMoistureNoiseLayer(const BiomeNoiseLayer& layer)
  { moistureLayer = layer; }

  void SetElevationNoiseLayer(const BiomeNoiseLayer& layer)
  { elevationLayer = layer; }

  void AddUnitRule(const BiomeUnitRule& rule)
  { unitRules.push_back(rule); }

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
  float minMoisture = 0.0f;
  float maxMoisture = 1.0f;
  float minElevation = 0.0f;
  float maxElevation = 1.0f;
};

} // namespace Rl::World::Biome
