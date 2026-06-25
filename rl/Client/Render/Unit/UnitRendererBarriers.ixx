export module Rl.Client.Render.Unit.UnitRendererBarriers;

import <vulkan/vulkan.hpp>;

namespace Rl::Client::Render
{

// Create memory barrier for buffer operations
export VkMemoryBarrier UnitCreateMemoryBarrier(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask);

// Create buffer memory barrier
export VkBufferMemoryBarrier UnitCreateBufferMemoryBarrier(
    VkBuffer buffer, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask);

// Create image memory barrier
export VkImageMemoryBarrier UnitCreateImageMemoryBarrier(VkImage image,
    VkAccessFlags                                         srcAccessMask,
    VkAccessFlags                                         dstAccessMask,
    VkImageLayout                                         oldLayout,
    VkImageLayout                                         newLayout);

} // namespace Rl::Client::Render
