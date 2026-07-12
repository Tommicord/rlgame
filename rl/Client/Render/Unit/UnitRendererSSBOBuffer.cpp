import Rl.Client.Render.Unit.UnitRendererBasicBuffer;
import Rl.Client.Render.Unit.UnitRendererInfo;
import Rl.Client.State.UnitState;
import <vulkan/vulkan.hpp>;

namespace Rl::Client::Render
{

void UnitCreateSSBOBuffers(VkDevice device,
    VkPhysicalDevice                physicalDevice,
    size_t                          vertexCount,
    Providers::UnitStateBinding&    vk)
{
  // Create output index buffer (host-visible for compute shader writes)
  // Size for maximum tessellated geometry: tessellation level 8 = 9x9 vertices per face
  // Max indices = 8 * 8 * 6 triangles per face * 6 faces = 2304 indices
  // Use conservative maximum of 32768 indices to match unit.tessel.comp MAX_INDICES
  VkDeviceSize outputIndexBufferSize = sizeof(uint32_t) * 32768;
  UnitCreateBuffer(device, physicalDevice, outputIndexBufferSize,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      vk.outputIndexBuffer, vk.outputIndexBufferMemory);

  std::vector<uint32_t> zeroIndices(32768, 0);
  UnitCopyDataToBuffer(
      device, vk.outputIndexBufferMemory, 0, outputIndexBufferSize, zeroIndices.data());

  // Create visible count buffer
  UnitCreateBuffer(device, physicalDevice, sizeof(uint32_t),
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
          VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      vk.visibleCountBuffer, vk.visibleCountBufferMemory);

  // Initialize visible count to 0
  uint32_t initialCount = 0;
  UnitCopyDataToBuffer(
      device, vk.visibleCountBufferMemory, 0, sizeof(uint32_t), &initialCount);

  // Create indirect draw buffer
  UnitCreateBuffer(device, physicalDevice, sizeof(UnitRenderDrawIndexedParams),
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
          VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      vk.indirectDrawBuffer, vk.indirectDrawBufferMemory);

  // Initialize indirect draw buffer for indexed drawing
  UnitRenderDrawIndexedParams initialDrawParams{};
  initialDrawParams.indexCount = 36; // 6 faces × 6 indices per face
  initialDrawParams.instanceCount = 1;
  initialDrawParams.firstIndex = 0;
  initialDrawParams.vertexOffset = 0;
  initialDrawParams.firstInstance = 0;
  UnitCopyDataToBuffer(device, vk.indirectDrawBufferMemory, 0,
      sizeof(UnitRenderDrawIndexedParams), &initialDrawParams);

  // Create frustum buffer
  UnitCreateBuffer(device, physicalDevice, sizeof(UnitRenderFrustumPlanes),
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      vk.frustumBuffer, vk.frustumBufferMemory);
}

} // namespace Rl::Client::Render
