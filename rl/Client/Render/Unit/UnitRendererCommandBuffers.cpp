import Rl.Client.Render.Unit.UnitRendererCommandBuffers;
import <stdexcept>;

namespace Rl::Client::Render
{

void UnitBeginCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags flags)
{
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = flags;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to begin command buffer");
  }
}

void UnitEndCommandBuffer(VkCommandBuffer commandBuffer)
{
  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to end command buffer");
  }
}

void UnitSubmitCommandBuffer(VkQueue queue, VkCommandBuffer commandBuffer)
{
  VkSubmitInfo submitInfo{};
  submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers    = &commandBuffer;

  if (vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to submit command buffer");
  }
}

VkCommandPool UnitCreateTempCommandPool(VkDevice device, uint32_t queueFamilyIndex)
{
  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
  poolInfo.queueFamilyIndex = queueFamilyIndex;

  VkCommandPool commandPool;
  if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create temporary command pool");
  }

  return commandPool;
}

VkCommandBuffer UnitAllocateTempCommandBuffer(VkDevice device, VkCommandPool commandPool)
{
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool        = commandPool;
  allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to allocate temporary command buffer");
  }

  return commandBuffer;
}

void UnitFreeTempCommandBuffer(
    VkDevice device, VkCommandPool commandPool, VkCommandBuffer commandBuffer)
{
  vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void UnitDestroyTempCommandPool(VkDevice device, VkCommandPool commandPool)
{
  vkDestroyCommandPool(device, commandPool, nullptr);
}

} // namespace Rl::Client::Render
