import Rl.Client.Render.Unit.UnitRendererIndirectDraw;

import <vulkan/vulkan.hpp>;

namespace Rl::Client::Render
{

void FillIndirectDrawBuffer(VkCommandBuffer commandBuffer,
    VkBuffer                                indirectDrawBuffer,
    uint32_t                                vertexCount,
    uint32_t                                instanceCount,
    uint32_t                                firstVertex,
    uint32_t                                firstInstance)
{
  vkCmdFillBuffer(commandBuffer, indirectDrawBuffer, 0, sizeof(uint32_t), vertexCount);
  vkCmdFillBuffer(
      commandBuffer, indirectDrawBuffer, sizeof(uint32_t), sizeof(uint32_t), instanceCount);
  vkCmdFillBuffer(
      commandBuffer, indirectDrawBuffer, sizeof(uint32_t) * 2, sizeof(uint32_t), firstVertex);
  vkCmdFillBuffer(
      commandBuffer, indirectDrawBuffer, sizeof(uint32_t) * 3, sizeof(uint32_t), firstInstance);
}

void CopyVisibleCountToIndirectDraw(
    VkCommandBuffer commandBuffer, VkBuffer visibleCountBuffer, VkBuffer indirectDrawBuffer)
{
  VkBufferCopy copyRegion{};
  copyRegion.srcOffset = 0;
  copyRegion.dstOffset = 0;
  copyRegion.size      = sizeof(uint32_t);
  vkCmdCopyBuffer(commandBuffer, visibleCountBuffer, indirectDrawBuffer, 1, &copyRegion);
}

} // namespace Rl::Client::Render
