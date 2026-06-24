#pragma once

#include <vulkan/vulkan.hpp>

namespace Rl::Client::Render
{

// Create memory barrier for buffer operations
VkMemoryBarrier UnitCreateMemoryBarrier(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask);

// Create buffer memory barrier
VkBufferMemoryBarrier UnitCreateBufferMemoryBarrier(
    VkBuffer buffer, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask);

// Create image memory barrier
VkImageMemoryBarrier UnitCreateImageMemoryBarrier(VkImage image,
    VkAccessFlags                                         srcAccessMask,
    VkAccessFlags                                         dstAccessMask,
    VkImageLayout                                         oldLayout,
    VkImageLayout                                         newLayout);

} // namespace Rl::Client::Render
