export module Rl.Client.Render.Unit.UnitRendererInfo;

import <array>;
import <cstddef>;
import <glm/glm.hpp>;
import <vulkan/vulkan.hpp>;

namespace Rl::Client::Render
{

/* Defines the Vertex data structure for unit rendering */
export struct UnitRenderVertex
{
  glm::vec4 position; // 16 bytes
  glm::vec4 polRight; // 16 bytes
  glm::vec4 polLeft; // 16 bytes
  glm::vec2 texCoords; // 8 bytes
  uint32_t  lightingEmit; // 4 bytes
  uint32_t  transparencyLevel; // 4 bytes

  uint32_t faceIndex; // 4 bytes
  float    roughness; // 4 bytes
  float    metallic; // 4 bytes
  float    padding;

  glm::vec4 polCurve;
  glm::vec4 albedo; // 16 bytes (albR, albG, albB + padding)
  glm::vec4 tangent; // 16 bytes (tanX, tanY, tanZ + padding)
  glm::vec4 bitangent; // 16 bytes (bitanX, bitanY, bitanZ + padding)
  glm::vec4 normal; // 16 bytes (normX, normY, normZ + padding)
  uint32_t  unitId; // 4 bytes - Unit ID for array lookup
};

/* Unit data structure for GPU array (matches UnitGPUParams) */
export struct alignas(16) UnitRenderUnitData
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
  float    padding[3];
};

/* Polygon fence structure for GPU array */
export struct alignas(16) UnitRenderPolFence
{
  float t, d, b, f; // Top, Down, Back, Front
};

/* Defines a single light source */
export struct UnitRenderLight
{
  glm::vec3 direction;
  float     padding0;
  glm::vec3 color;
  float     intensity;
  float     padding1[4];
};

/* Defines the lighting uniforms for the unit render info */
export struct alignas(16) UnitRenderLightingUniforms
{
  // Primary sunlight
  glm::vec4 sunDirection;
  glm::vec4 sunColor;
  float     sunIntensity;
  uint32_t  additionalLightCount;
  float     ambientStrength;
  float     exposure;
  float     _padding;

  alignas(16) glm::vec3 cameraPosition;

  UnitRenderLight additionalLights[4];
  // Spherical harmonics for GI (9 coefficients for RGB = 27 floats)
  alignas(16) glm::vec4 shCoefficients[9];
  alignas(16) glm::vec4 groundColor;
  alignas(16) glm::vec4 skyColor;
  alignas(16) glm::mat4 u_LightSpaceMatrix;

  alignas(16) glm::vec4 u_CascadeSplits;

  // LOD settings
  float    lodDistanceNear; // Distance threshold for high quality
  float    lodDistanceFar; // Distance threshold for low quality
  uint32_t qualityLevel; // 0=low, 1=medium, 2=high
  uint32_t numCascades;
};

/* Defines the triplanar settings */
export struct UnitRenderTriplanarSettings
{
  alignas(4) float scale;
  alignas(4) float sharpness;
  alignas(4) float offsetX;
  alignas(4) float offsetY;
  alignas(4) float offsetZ;
  alignas(4) float blendMix;
};

/* Defines the UBO for Projection-View-Model */
export struct UnitRenderUBO
{
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 projection;
};

/* Defines the frustum planes for frustum culling */
export struct UnitRenderFrustumPlanes
{
  std::array<glm::vec4, 6> planes;
};

/* Defines draw parameters for indirect drawing */
export struct UnitRenderDrawIndexedParams
{
  uint32_t indexCount;
  uint32_t instanceCount;
  uint32_t firstIndex;
  int32_t  vertexOffset;
  uint32_t firstInstance;
};

/* Defines push constants for unit rendering */
export struct UnitRenderPushConstants
{
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 projection;
  uint32_t  useUnitArray; // 0 = use vertex data, 1 = use unit array
  uint32_t  singleUnitMode; // 1 = render single unit only
  uint32_t  singleUnitId; // unit ID for single unit mode
  uint32_t  padding1;
};

/* Convert UnitGPUParams to UnitRenderUnitData for rendering */
export inline UnitRenderUnitData ConvertGPUParamsToRenderData(const World::UnitGPUParams& params)
{
  UnitRenderUnitData renderData{};
  renderData.unitId = params.unitId;
  renderData.temperature = params.temperature;
  renderData.moisture = params.moisture;
  renderData.roughness = params.roughness;
  renderData.metallic = params.metallic;
  renderData.albedoR = params.albedoR;
  renderData.albedoG = params.albedoG;
  renderData.albedoB = params.albedoB;
  renderData.reflectivity = params.reflectivity;
  renderData.refractiveIndex = params.refractiveIndex;
  renderData.dirtiness = params.dirtiness;
  renderData.hardness = params.hardness;
  renderData.explosionResistance = params.explosionResistance;
  renderData.transparency = params.transparency;
  renderData.emissiveIntensity = params.emissiveIntensity;
  renderData.subsurfaceScattering = params.subsurfaceScattering;
  renderData.flammability = params.flammability;
  renderData.lightEmit = params.lightEmit;
  renderData.lightOpacity = params.lightOpacity;
  renderData.ambientOcclusion = params.ambientOcclusion;
  renderData.lightAbsorption = params.lightAbsorption;
  renderData.lightScattering = params.lightScattering;
  renderData.humidity = params.humidity;
  renderData.isLiquid = params.isLiquid;
  renderData.isGas = params.isGas;
  renderData.isSolid = params.isSolid;
  renderData.padding[0] = params.padding[0];
  renderData.padding[1] = params.padding[1];
  renderData.padding[2] = params.padding[2];
  return renderData;
}

} // namespace Rl::Client::Render
