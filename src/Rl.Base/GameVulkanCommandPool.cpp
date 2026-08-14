#include "Rl.Base/GameVulkanCommandPool.h"
#include "Rl.Base/GameError.h"

#include <vulkan/vulkan.hpp>

namespace rl
{

GameVulkanCommandPool::GameVulkanCommandPool() noexcept :
    device(VK_NULL_HANDLE), commandPool(VK_NULL_HANDLE), ownsCommandPool(true)
{
}

GameVulkanCommandPool::GameVulkanCommandPool(VkDevice device, uint32_t queueFamilyIndex) :
    device(device), commandPool(VK_NULL_HANDLE), ownsCommandPool(true)
{
  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = queueFamilyIndex;

  VkResult result = vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool);
  if (result != VK_SUCCESS)
  {
    GameError::exitWithError(
        "vkCreateCommandPool",
        "Failed to create command pool (result = " + GameError::vulkanResultToString(result) + ")",
        device, VK_NULL_HANDLE, VK_NULL_HANDLE);
  }
}

GameVulkanCommandPool::GameVulkanCommandPool(VkDevice      device,
                                             VkCommandPool commandPool,
                                             bool          ownsCommandPool) :
    device(device), commandPool(commandPool), ownsCommandPool(ownsCommandPool)
{
}

GameVulkanCommandPool::GameVulkanCommandPool(GameVulkanCommandPool&& other) noexcept :
    device(other.device), commandPool(other.commandPool), ownsCommandPool(other.ownsCommandPool)
{
  other.device          = VK_NULL_HANDLE;
  other.commandPool     = VK_NULL_HANDLE;
  other.ownsCommandPool = true;
}

GameVulkanCommandPool& GameVulkanCommandPool::operator=(GameVulkanCommandPool&& other) noexcept
{
  if (this != &other)
  {
    if (commandPool != VK_NULL_HANDLE && ownsCommandPool)
    {
      vkDestroyCommandPool(device, commandPool, nullptr);
    }

    device                = other.device;
    commandPool           = other.commandPool;
    ownsCommandPool       = other.ownsCommandPool;
    other.commandPool     = VK_NULL_HANDLE;
    other.ownsCommandPool = true;
  }
  return *this;
}

GameVulkanCommandPool::~GameVulkanCommandPool()
{
  if (commandPool != VK_NULL_HANDLE && ownsCommandPool)
  {
    vkDestroyCommandPool(device, commandPool, nullptr);
    commandPool = VK_NULL_HANDLE;
  }
}

VkCommandPool GameVulkanCommandPool::getCommandPool() const
{
  return commandPool;
}

} // namespace rl
