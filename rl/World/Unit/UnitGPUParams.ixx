export module Rl.World.Unit.UnitGPUParams;

import <cstdint>;
import <glm/glm.hpp>;

namespace Rl::World
{

/* GPU-compatible unit parameters structure for compute shaders */
export struct UnitGPUParams
{
  uint32_t  unitId;
  float     temperature;
  float     moisture;
  float     roughness;
  float     metallic;
  float     albedoR;
  float     albedoG;
  float     albedoB;
  float     reflectivity;
  float     refractiveIndex;
  float     dirtiness;
  float     hardness;
  float     explosionResistance;
  float     transparency;
  float     emissiveIntensity;
  float     subsurfaceScattering;
  float     flammability;
  float     lightEmit;
  float     lightOpacity;
  float     ambientOcclusion;
  float     lightAbsorption;
  float     lightScattering;
  float     humidity;
  glm::vec4 polCurve;
  glm::vec4 polLeft;
  glm::vec4 polRight;
  uint32_t  isLiquid;
  uint32_t  isGas;
  uint32_t  isSolid;
};

} // namespace Rl::World
