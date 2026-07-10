export module Rl.World.Unit.UnitGPUParams;

import <cstdint>;

namespace Rl::World
{

/* GPU-compatible unit parameters structure for compute shaders */
export struct UnitGPUParams
{
  uint32_t unitId;
  float    temperature;
  float    moisture;
  float    roughness;
  float    metallic;
  float    albedoR;
  float    albedoG;
  float    albedoB;
  float    reflectivity;
  float    refractiveIndex;
  float    dirtiness;
  float    hardness;
  float    explosionResistance;
  float    transparency;
  float    emissiveIntensity;
  float    subsurfaceScattering;
  float    flammability;
  float    lightEmit;
  float    lightOpacity;
  float    ambientOcclusion;
  float    lightAbsorption;
  float    lightScattering;
  float    humidity;
  uint32_t isLiquid;
  uint32_t isGas;
  uint32_t isSolid;
  float    padding[3]; // Alignment padding to 16-byte boundary
};

} // namespace Rl::World
