#include "rl/Client/Render/Unit/UnitRendererVertexInput.h"
#include "rl/Client/Render/Unit/UnitRendererInfo.h"

namespace Rl::Client::Render
{

VkVertexInputBindingDescription UnitCreateVertexInputBindingDescription()
{
  VkVertexInputBindingDescription inputBindingDescription{};
  inputBindingDescription.binding   = 0;
  inputBindingDescription.stride    = sizeof(UnitRenderVertex);
  inputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  return inputBindingDescription;
}

std::array<VkVertexInputAttributeDescription, 14> UnitCreateVertexAttributeDescriptions()
{
  std::array<VkVertexInputAttributeDescription, 14> inputAttributeDescriptions{};

  // Position (vec4)
  inputAttributeDescriptions[0].binding  = 0;
  inputAttributeDescriptions[0].location = 0;
  inputAttributeDescriptions[0].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
  inputAttributeDescriptions[0].offset   = offsetof(UnitRenderVertex, position);

  // PolRight (vec4)
  inputAttributeDescriptions[1].binding  = 0;
  inputAttributeDescriptions[1].location = 1;
  inputAttributeDescriptions[1].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
  inputAttributeDescriptions[1].offset   = offsetof(UnitRenderVertex, polRight);

  // PolLeft (vec4)
  inputAttributeDescriptions[2].binding  = 0;
  inputAttributeDescriptions[2].location = 2;
  inputAttributeDescriptions[2].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
  inputAttributeDescriptions[2].offset   = offsetof(UnitRenderVertex, polLeft);

  // TexCoords (vec2)
  inputAttributeDescriptions[3].binding  = 0;
  inputAttributeDescriptions[3].location = 3;
  inputAttributeDescriptions[3].format   = VK_FORMAT_R32G32_SFLOAT;
  inputAttributeDescriptions[3].offset   = offsetof(UnitRenderVertex, texCoords);

  // LightingEmit (uint)
  inputAttributeDescriptions[4].binding  = 0;
  inputAttributeDescriptions[4].location = 4;
  inputAttributeDescriptions[4].format   = VK_FORMAT_R32_UINT;
  inputAttributeDescriptions[4].offset   = offsetof(UnitRenderVertex, lightingEmit);

  // TransparencyLevel (uint)
  inputAttributeDescriptions[5].binding  = 0;
  inputAttributeDescriptions[5].location = 5;
  inputAttributeDescriptions[5].format   = VK_FORMAT_R32_UINT;
  inputAttributeDescriptions[5].offset   = offsetof(UnitRenderVertex, transparencyLevel);

  // FaceIndex (uint)
  inputAttributeDescriptions[6].binding  = 0;
  inputAttributeDescriptions[6].location = 6;
  inputAttributeDescriptions[6].format   = VK_FORMAT_R32_UINT;
  inputAttributeDescriptions[6].offset   = offsetof(UnitRenderVertex, faceIndex);

  // Roughness (float)
  inputAttributeDescriptions[7].binding  = 0;
  inputAttributeDescriptions[7].location = 7;
  inputAttributeDescriptions[7].format   = VK_FORMAT_R32_SFLOAT;
  inputAttributeDescriptions[7].offset   = offsetof(UnitRenderVertex, roughness);

  // Metallic (float)
  inputAttributeDescriptions[8].binding  = 0;
  inputAttributeDescriptions[8].location = 8;
  inputAttributeDescriptions[8].format   = VK_FORMAT_R32_SFLOAT;
  inputAttributeDescriptions[8].offset   = offsetof(UnitRenderVertex, metallic);

  // PolCurve (vec4)
  inputAttributeDescriptions[9].binding  = 0;
  inputAttributeDescriptions[9].location = 9;
  inputAttributeDescriptions[9].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
  inputAttributeDescriptions[9].offset   = offsetof(UnitRenderVertex, polCurve);

  // Albedo (vec4)
  inputAttributeDescriptions[10].binding  = 0;
  inputAttributeDescriptions[10].location = 10;
  inputAttributeDescriptions[10].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
  inputAttributeDescriptions[10].offset   = offsetof(UnitRenderVertex, albedo);

  // Tangent (vec4)
  inputAttributeDescriptions[11].binding  = 0;
  inputAttributeDescriptions[11].location = 11;
  inputAttributeDescriptions[11].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
  inputAttributeDescriptions[11].offset   = offsetof(UnitRenderVertex, tangent);

  // Bitangent (vec4)
  inputAttributeDescriptions[12].binding  = 0;
  inputAttributeDescriptions[12].location = 12;
  inputAttributeDescriptions[12].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
  inputAttributeDescriptions[12].offset   = offsetof(UnitRenderVertex, bitangent);

  // Normal (vec4)
  inputAttributeDescriptions[13].binding  = 0;
  inputAttributeDescriptions[13].location = 13;
  inputAttributeDescriptions[13].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
  inputAttributeDescriptions[13].offset   = offsetof(UnitRenderVertex, normal);

  return inputAttributeDescriptions;
}

} // namespace Rl::Client::Render
