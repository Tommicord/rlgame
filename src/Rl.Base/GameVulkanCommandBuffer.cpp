#include "Rl.Base/GameVulkanCommandBuffer.h"
#include "Rl.Base/GameError.h"

#include <stdexcept>
#include <vulkan/vulkan.hpp>

namespace rl
{

GameVulkanCommandBuffer::GameVulkanCommandBuffer() :
    device(VK_NULL_HANDLE), commandPool(VK_NULL_HANDLE), commandBuffer(VK_NULL_HANDLE)
{
}

GameVulkanCommandBuffer::GameVulkanCommandBuffer(VkDevice device, VkCommandPool commandPool) :
    device(device), commandPool(commandPool), commandBuffer(VK_NULL_HANDLE)
{
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool        = commandPool;
  allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;

  try
  {
    if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to allocate command buffer");
    }
  }
  catch (std::runtime_error& e)
  {
    GameError::exitWithError("Vulkan Command Buffer Allocation Error", e.what());
  }
}

GameVulkanCommandBuffer::GameVulkanCommandBuffer(GameVulkanCommandBuffer&& other) noexcept :
    device(other.device), commandPool(other.commandPool), commandBuffer(other.commandBuffer)
{
  other.device        = VK_NULL_HANDLE;
  other.commandPool   = VK_NULL_HANDLE;
  other.commandBuffer = VK_NULL_HANDLE;
}

GameVulkanCommandBuffer&
GameVulkanCommandBuffer::operator=(GameVulkanCommandBuffer&& other) noexcept
{
  if (this != &other)
  {
    if (commandBuffer != VK_NULL_HANDLE)
    {
      vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }

    device        = other.device;
    commandPool   = other.commandPool;
    commandBuffer = other.commandBuffer;

    other.device        = VK_NULL_HANDLE;
    other.commandPool   = VK_NULL_HANDLE;
    other.commandBuffer = VK_NULL_HANDLE;
  }
  return *this;
}

GameVulkanCommandBuffer::~GameVulkanCommandBuffer()
{
  if (commandBuffer != VK_NULL_HANDLE)
  {
    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
  }
}

VkCommandBuffer GameVulkanCommandBuffer::getCommandBuffer() const
{
  return commandBuffer;
}

VkCommandPool GameVulkanCommandBuffer::getCommandPool() const
{
  return commandPool;
}

VkDevice GameVulkanCommandBuffer::getDevice() const
{
  return device;
}

void GameVulkanCommandBuffer::begin(VkCommandBufferUsageFlags flags)
{
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = flags;

  try
  {
    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to begin command buffer");
    }
  }
  catch (std::runtime_error& e)
  {
    GameError::exitWithError("Vulkan Command Buffer Begin Error", e.what());
  }
}

void GameVulkanCommandBuffer::end()
{
  try
  {
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to end command buffer");
    }
  }
  catch (std::runtime_error& e)
  {
    GameError::exitWithError("Vulkan Command Buffer End Error", e.what());
  }
}

void GameVulkanCommandBuffer::reset(VkCommandBufferResetFlags flags)
{
  try
  {
    if (vkResetCommandBuffer(commandBuffer, flags) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to reset command buffer");
    }
  }
  catch (std::runtime_error& e)
  {
    GameError::exitWithError("Vulkan Command Buffer Reset Error", e.what());
  }
}

} // namespace rl
