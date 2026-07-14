export module Rl.World.Unit.UnitGPUParams;

import <cstdint>;
import <glm/glm.hpp>;

namespace Rl::World
{

/* GPU-compatible unit parameters structure for compute shaders */
// Must match the UnitData structure in unit.vert shader exactly
// Aligned to 16 bytes for std430/std140 compatibility
export struct alignas(16) UnitGPUParams
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
    float    padding[3]; // Pad to 16-byte boundary (total 120 bytes)
};

} // namespace Rl::World
