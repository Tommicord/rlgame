export module Rl.World.Biome.IBiome;

import <cstdint>;
import <string>;
import <vector>;

namespace Rl::World::Biome
{
/* Noise layer configuration for biome generation */
export struct BiomeNoiseLayer
{
  float scale;           // Noise frequency scale
  float offsetX;         // X offset for noise sampling
  float offsetY;         // Y offset for noise sampling
  float offsetZ;         // Z offset for noise sampling
  uint32_t octaves;      // FBM octaves
  float persistence;     // FBM persistence
  float lacunarity;      // FBM lacunarity
  uint32_t noiseType;    // 0: Standard, 1: FBM, 2: Ridged, 3: Turbulence
  float weight;          // Layer weight in final classification
};

/* Unit generation rule for a biome */
export struct BiomeUnitRule
{
  uint32_t unitId;       // Unit ID to place
  float minHeight;       // Minimum height for this unit
  float maxHeight;       // Maximum height for this unit
  float minTemperature;  // Minimum temperature threshold
  float maxTemperature;  // Maximum temperature threshold
  float minMoisture;     // Minimum moisture threshold
  float maxMoisture;     // Maximum moisture threshold
  float minElevation;    // Minimum elevation threshold
  float maxElevation;    // Maximum elevation threshold
  float probability;     // Probability of placement (0.0-1.0)
  float density;         // Density of placement (0.0-1.0)
};

/* GPU-compatible biome parameters structure */
export struct BiomeGPUParams
{
  uint32_t biomeType;
  float temperatureBase;
  float temperatureVariation;
  float moistureBase;
  float moistureVariation;
  float elevationBase;
  float elevationVariation;
  uint32_t unitRuleCount;
  float padding[3]; // Alignment padding
};

/* Pure virtual interface for biome definition */
export class IBiome
{
  public:
  virtual ~IBiome() = default;

  /* Gets the biome type identifier */
  [[nodiscard]]
  virtual uint32_t GetBiomeType() const = 0;

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
  virtual bool BelongsToBiome(float temperature, float moisture, float elevation) const = 0;

  /* Gets the dominant unit ID for a given position in this biome */
  [[nodiscard]]
  virtual uint32_t GetDominantUnit(float temperature, float moisture, float elevation, float height) const = 0;
};

} // namespace Rl::World::Biome