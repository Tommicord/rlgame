#pragma once

#include <vulkan/vulkan.hpp>

namespace Rl::Client::Render
{

// Fill indirect draw buffer parameters
void UnitFillIndirectDrawBuffer(VkCommandBuffer commandBuffer,
    VkBuffer                                    indirectDrawBuffer,
    uint32_t                                    vertexCount,
    uint32_t                                    instanceCount,
    uint32_t                                    firstVertex,
    uint32_t                                    firstInstance);

// Copy visible count to indirect draw buffer
void UnitCopyVisibleCountToIndirectDraw(
    VkCommandBuffer commandBuffer, VkBuffer visibleCountBuffer, VkBuffer indirectDrawBuffer);

} // namespace Rl::Client::Render
