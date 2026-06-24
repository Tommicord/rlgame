#include "rl/Client/Render/Unit/UnitRendererBasicBuffer.h"

#include <array>
#include <cstddef>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

namespace Rl::Client::Render
{

void CreateCurvatureComputeBuffers(VkDevice device,
    VkPhysicalDevice                   physicalDevice,
    size_t                             vertexCount,
    Providers::UnitStateDrawableVulkan&           vk)
{
  // Calculate maximum curved vertex count (tessellation level 8 = 9x9 = 81 vertices per curved face)
  const uint32_t tessellationLevel = 8;
  const uint32_t verticesPerEdge = tessellationLevel + 1;
  const uint32_t maxVerticesPerCurvedFace = verticesPerEdge * verticesPerEdge;
  const uint32_t maxCurvedVertices = maxVerticesPerCurvedFace * 6; // 6 faces
  const uint32_t maxCurvedIndices = tessellationLevel * tessellationLevel * 6 * 6; // 6 faces

  // Create curved vertex buffer
  VkDeviceSize curvedVertexBufferSize = sizeof(UnitRenderVertex) * maxCurvedVertices;
  UnitCreateBuffer(device, physicalDevice, curvedVertexBufferSize,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
          VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vk.curvedVertexBuffer, vk.curvedVertexBufferMemory);

  // Create curved index buffer
  VkDeviceSize curvedIndexBufferSize = sizeof(uint32_t) * maxCurvedIndices;
  UnitCreateBuffer(device, physicalDevice, curvedIndexBufferSize,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
          VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vk.curvedIndexBuffer, vk.curvedIndexBufferMemory);

  // Create counters buffer (vertexCount and indexCount)
  UnitCreateBuffer(device, physicalDevice, 2 * sizeof(uint32_t),
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      vk.curveCountersBuffer, vk.curveCountersBufferMemory);

  // Initialize counters to 0
  uint32_t zeroCounters[2] = {0, 0};
  UnitCopyDataToBuffer(device, vk.curveCountersBufferMemory, 0, 2 * sizeof(uint32_t), zeroCounters);

  // Create indirect draw buffer for curved geometry
  UnitCreateBuffer(device, physicalDevice, sizeof(UnitDrawIndexedParams),
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
          VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      vk.curveIndirectDrawBuffer, vk.curveIndirectDrawBufferMemory);

  // Initialize indirect draw buffer
  UnitDrawIndexedParams initialDrawParams{};
  initialDrawParams.indexCount = 0;
  initialDrawParams.instanceCount = 1;
  initialDrawParams.firstIndex = 0;
  initialDrawParams.vertexOffset = 0;
  initialDrawParams.firstInstance = 0;
  UnitCopyDataToBuffer(device, vk.curveIndirectDrawBufferMemory, 0, sizeof(UnitDrawIndexedParams), &initialDrawParams);
}

} // namespace Rl::Client::Render