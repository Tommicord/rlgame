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

  alignas(16) glm::vec3 cameraPosition;

  UnitRenderLight additionalLights[4];
  // Spherical harmonics for GI (9 coefficients for RGB = 27 floats)
  alignas(16) glm::vec4 shCoefficients[9];
  alignas(16) glm::vec4 groundColor;
  alignas(16) glm::vec4 skyColor;
  alignas(16) glm::mat4 lightSpaceMatrix;

  // LOD settings
  float    lodDistanceNear; // Distance threshold for high quality
  float    lodDistanceFar; // Distance threshold for low quality
  uint32_t qualityLevel; // 0=low, 1=medium, 2=high
  float    _padding; // Alignment padding
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

} // namespace Rl::Client::Render
