export module Rl.World.Biome.BiomeGPUParams;

import Rl.World.Biome.IBiome;

import <cstdint>;

namespace Rl::World::Biome
{

/* GPU-compatible biome parameters structure for compute shaders */
export struct BiomeGPUParams
{
  BiomeType biomeType;
  float     temperatureBase;
  float     temperatureVariation;
  float     moistureBase;
  float     moistureVariation;
  float     elevationBase;
  float     elevationVariation;
  uint32_t  unitRuleCount;
  float     padding[3]; // Alignment padding to 16-byte boundary
};

/* GPU-compatible unit rule structure for compute shaders */
export struct BiomeUnitRuleGPU
{
  uint32_t unitId;
  float    minHeight;
  float    maxHeight;
  float    minTemperature;
  float    maxTemperature;
  float    minMoisture;
  float    maxMoisture;
  float    minElevation;
  float    maxElevation;
  float    probability;
  float    density;
  float    padding[2]; // Alignment padding
};

} // namespace Rl::World::Biome
