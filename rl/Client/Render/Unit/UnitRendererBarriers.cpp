#include "UnitRendererBarriers.h"

namespace Rl::Client::Render
{

VkMemoryBarrier UnitCreateMemoryBarrier(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask)
{
  VkMemoryBarrier barrier{};
  barrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  barrier.srcAccessMask = srcAccessMask;
  barrier.dstAccessMask = dstAccessMask;
  return barrier;
}

VkBufferMemoryBarrier UnitCreateBufferMemoryBarrier(
    VkBuffer buffer, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask)
{
  VkBufferMemoryBarrier barrier{};
  barrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  barrier.srcAccessMask       = srcAccessMask;
  barrier.dstAccessMask       = dstAccessMask;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.buffer              = buffer;
  barrier.offset              = 0;
  barrier.size                = VK_WHOLE_SIZE;
  return barrier;
}

VkImageMemoryBarrier UnitCreateImageMemoryBarrier(VkImage image,
    VkAccessFlags                                         srcAccessMask,
    VkAccessFlags                                         dstAccessMask,
    VkImageLayout                                         oldLayout,
    VkImageLayout                                         newLayout)
{
  VkImageMemoryBarrier barrier{};
  barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.srcAccessMask                   = srcAccessMask;
  barrier.dstAccessMask                   = dstAccessMask;
  barrier.oldLayout                       = oldLayout;
  barrier.newLayout                       = newLayout;
  barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.image                           = image;
  barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel   = 0;
  barrier.subresourceRange.levelCount     = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount     = 1;
  return barrier;
}

} // namespace Rl::Client::Render
