export module Rl.Client.Render.Unit.UnitRendererCommandBuffers;

import <vulkan/vulkan.hpp>;

namespace Rl::Client::Render
{

export void UnitBeginCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags flags);

export void UnitEndCommandBuffer(VkCommandBuffer commandBuffer);

export void UnitSubmitCommandBuffer(VkQueue queue, VkCommandBuffer commandBuffer);

export VkCommandPool UnitCreateTempCommandPool(VkDevice device, uint32_t queueFamilyIndex);

export VkCommandBuffer UnitAllocateTempCommandBuffer(VkDevice device, VkCommandPool commandPool);

export void UnitFreeTempCommandBuffer(
    VkDevice device, VkCommandPool commandPool, VkCommandBuffer commandBuffer);

export void UnitDestroyTempCommandPool(VkDevice device, VkCommandPool commandPool);

} // namespace Rl::Client::Render
