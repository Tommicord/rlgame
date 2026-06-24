#pragma once

#include "rl/Client/Render/Unit/UnitRendererBasicBuffer.h"
#include "rl/Client/Render/Unit/UnitRendererInfo.h"
#include "rl/Client/State/UnitState.h"

#include <vulkan/vulkan.hpp>

namespace Rl::Client::Render
{

void CreateSSBOBuffers(VkDevice device,
    VkPhysicalDevice            physicalDevice,
    size_t                      vertexCount,
    Providers::UnitStateDrawableVulkan&    vk)
{
  // Create output index buffer (host-visible for compute shader writes)
  VkDeviceSize outputIndexBufferSize = sizeof(uint32_t) * 36; // Max 36 indices for cube
  CreateBuffer(device, physicalDevice, outputIndexBufferSize,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      vk.outputIndexBuffer, vk.outputIndexBufferMemory);

  std::vector<uint32_t> zeroIndices(36, 0);
  CopyDataToBuffer(
      device, vk.outputIndexBufferMemory, 0, outputIndexBufferSize, zeroIndices.data());

  // Create visible count buffer
  CreateBuffer(device, physicalDevice, sizeof(uint32_t),
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
          VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      vk.visibleCountBuffer, vk.visibleCountBufferMemory);

  // Initialize visible count to 0
  uint32_t initialCount = 0;
  CopyDataToBuffer(device, vk.visibleCountBufferMemory, 0, sizeof(uint32_t), &initialCount);

  // Create indirect draw buffer
  CreateBuffer(device, physicalDevice, sizeof(UnitDrawIndexedParams),
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
          VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      vk.indirectDrawBuffer, vk.indirectDrawBufferMemory);

  // Initialize indirect draw buffer for indexed drawing
  UnitDrawIndexedParams initialDrawParams{};
  initialDrawParams.indexCount    = 36; // 6 faces × 6 indices per face
  initialDrawParams.instanceCount = 1;
  initialDrawParams.firstIndex    = 0;
  initialDrawParams.vertexOffset  = 0;
  initialDrawParams.firstInstance = 0;
  CopyDataToBuffer(
      device, vk.indirectDrawBufferMemory, 0, sizeof(UnitDrawIndexedParams), &initialDrawParams);

  // Create frustum buffer
  CreateBuffer(device, physicalDevice, sizeof(UnitRenderFrustumPlanes),
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vk.frustumBuffer,
      vk.frustumBufferMemory);
}

} // namespace Rl::Client::Render
