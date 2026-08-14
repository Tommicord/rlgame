#include "Rl.Base/GameVulkanDescriptorPool.h"
#include "Rl.Base/GameError.h"

namespace rl
{

GameVulkanDescriptorPool::GameVulkanDescriptorPool() noexcept :
    device(VK_NULL_HANDLE), descriptorPool(VK_NULL_HANDLE), ownsDescriptorPool(true)
{
}

GameVulkanDescriptorPool::GameVulkanDescriptorPool(GameVulkanDescriptorPool&& other) noexcept :
    device(other.device), descriptorPool(other.descriptorPool),
    ownsDescriptorPool(other.ownsDescriptorPool)
{
  other.device             = VK_NULL_HANDLE;
  other.descriptorPool     = VK_NULL_HANDLE;
  other.ownsDescriptorPool = false;
}

GameVulkanDescriptorPool::GameVulkanDescriptorPool(
    VkDevice device, const GameVulkanDescriptorPoolCreateInfo& createInfo) :
    device(device), descriptorPool(VK_NULL_HANDLE), ownsDescriptorPool(true)
{
  VkResult result =
      vkCreateDescriptorPool(device, createInfo.pCreateInfo, nullptr, &descriptorPool);
  if (result != VK_SUCCESS)
  {
    GameError::exitWithError("vkCreateDescriptorPool",
                             "Failed to create descriptor pool (result = " +
                                 GameError::vulkanResultToString(result) + ")",
                             device, VK_NULL_HANDLE, VK_NULL_HANDLE);
  }
}

GameVulkanDescriptorPool::~GameVulkanDescriptorPool()
{
  if (descriptorPool != VK_NULL_HANDLE && ownsDescriptorPool)
  {
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    descriptorPool = VK_NULL_HANDLE;
  }
}

GameVulkanDescriptorPool&
GameVulkanDescriptorPool::operator=(GameVulkanDescriptorPool&& other) noexcept
{
  if (this != &other)
  {
    if (descriptorPool != VK_NULL_HANDLE && ownsDescriptorPool)
    {
      vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    }
    device                   = other.device;
    descriptorPool           = other.descriptorPool;
    ownsDescriptorPool       = other.ownsDescriptorPool;
    other.device             = VK_NULL_HANDLE;
    other.descriptorPool     = VK_NULL_HANDLE;
    other.ownsDescriptorPool = false;
  }
  return *this;
}

VkDescriptorPool GameVulkanDescriptorPool::getDescriptorPool() const
{
  return descriptorPool;
}

void GameVulkanDescriptorPool::setDescriptorPool(VkDescriptorPool other)
{
  if (descriptorPool != VK_NULL_HANDLE && ownsDescriptorPool)
  {
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
  }
  descriptorPool     = other;
  ownsDescriptorPool = true;
}

void GameVulkanDescriptorPool::setDescriptorPoolNonOwning(VkDescriptorPool other)
{
  if (descriptorPool != VK_NULL_HANDLE && ownsDescriptorPool)
  {
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
  }
  descriptorPool     = other;
  ownsDescriptorPool = false;
}

} // namespace rl
