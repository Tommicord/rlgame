#pragma once

#include <vulkan/vulkan.hpp>

namespace Rl::Client::Render
{

void UnitBeginCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags flags);

void UnitEndCommandBuffer(VkCommandBuffer commandBuffer);

void UnitSubmitCommandBuffer(VkQueue queue, VkCommandBuffer commandBuffer);

VkCommandPool UnitCreateTempCommandPool(VkDevice device, uint32_t queueFamilyIndex);

VkCommandBuffer UnitAllocateTempCommandBuffer(VkDevice device, VkCommandPool commandPool);

void UnitFreeTempCommandBuffer(
    VkDevice device, VkCommandPool commandPool, VkCommandBuffer commandBuffer);

void UnitDestroyTempCommandPool(VkDevice device, VkCommandPool commandPool);

} // namespace Rl::Client::Render
