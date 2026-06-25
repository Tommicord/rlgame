export module Rl.Client.Render.Unit.UnitRendererIndirectDraw;

import <vulkan/vulkan.hpp>;

namespace Rl::Client::Render
{

// Fill indirect draw buffer parameters
export void UnitFillIndirectDrawBuffer(VkCommandBuffer commandBuffer,
    VkBuffer                                    indirectDrawBuffer,
    uint32_t                                    vertexCount,
    uint32_t                                    instanceCount,
    uint32_t                                    firstVertex,
    uint32_t                                    firstInstance);

// Copy visible count to indirect draw buffer
export void UnitCopyVisibleCountToIndirectDraw(
    VkCommandBuffer commandBuffer, VkBuffer visibleCountBuffer, VkBuffer indirectDrawBuffer);

} // namespace Rl::Client::Render
