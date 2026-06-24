#pragma once

#include <array>
#include <cstddef>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

namespace Rl::Client::Render
{

/* Defines the Vertex data structure for unit rendering */
struct UnitRenderVertex
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
struct UnitRenderLight
{
  alignas(16) glm::vec3 direction; // For directional lights
  alignas(16) glm::vec3 color;
  alignas(4) float intensity;
  alignas(4) float padding;
};

/* Defines the lighting uniforms for the unit render info */
struct UnitRenderLightingUniforms
{
  // Primary sun light
  alignas(16) glm::vec3 sunDirection;
  alignas(16) glm::vec3 sunColor;
  alignas(4) float sunIntensity;

  UnitRenderLight additionalLights[4];
  alignas(4) uint32_t additionalLightCount;

  // Ambient and environment
  alignas(4) float ambientStrength;
  alignas(16) glm::vec3 cameraPosition;
  alignas(4) float exposure;

  // Spherical harmonics for GI (9 coefficients for RGB = 27 floats)
  alignas(16) glm::vec3 shCoefficients[9];

  // Environment colors
  alignas(16) glm::vec3 groundColor;
  alignas(16) glm::vec3 skyColor;

  alignas(4) float padding1;
  alignas(4) float padding2;
};

/* Defines the triplanar settings */
struct UnitRenderTriplanarSettings
{
  alignas(4) float scale;
  alignas(4) float sharpness;
  alignas(4) float offsetX;
  alignas(4) float offsetY;
  alignas(4) float offsetZ;
  alignas(4) float blendMix;
};

/* Defines the UBO for Projection-View-Model */
struct UnitRenderUBO
{
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 projection;
};

/* Defines the frustum planes for frustum culling */
struct UnitRenderFrustumPlanes
{
  std::array<glm::vec4, 6> planes;
};

/* Defines draw parameters for indirect drawing */
struct UnitRenderDrawIndexedParams
{
  uint32_t indexCount;
  uint32_t instanceCount;
  uint32_t firstIndex;
  int32_t  vertexOffset;
  uint32_t firstInstance;
};

} // namespace Rl::Client::Render
