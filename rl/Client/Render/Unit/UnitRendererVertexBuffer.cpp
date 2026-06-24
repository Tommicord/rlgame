#include "rl/Client/Render/Unit/UnitRendererBasicBuffer.h"
#include "rl/Client/Render/Unit/UnitRendererInfo.h"

#include <array>
#include <cstddef>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

namespace Rl::Client::Render
{

void CreateVertexBuffer(VkDevice         device,
    VkPhysicalDevice                     physicalDevice,
    const std::vector<UnitRenderVertex>& vertices,
    VkBuffer&                            vertexBuffer,
    VkDeviceMemory&                      vertexBufferMemory)
{
  VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

  // Create staging buffer (host-visible)
  VkBuffer       stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  CreateBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
      stagingBufferMemory);

  // Copy data to staging buffer
  CopyDataToBuffer(device, stagingBufferMemory, 0, bufferSize, vertices.data());

  // Create device-local vertex buffer
  CreateBuffer(device, physicalDevice, bufferSize,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
          VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

  // Create temporary command pool for buffer copy
  VkCommandPoolCreateInfo commandPoolInfo{};
  commandPoolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  commandPoolInfo.flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
  commandPoolInfo.queueFamilyIndex = 0; // Assuming graphics queue family is 0

  VkCommandPool commandPool;
  if (vkCreateCommandPool(device, &commandPoolInfo, nullptr, &commandPool) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create temporary command pool");
  }

  // Allocate command buffer
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool        = commandPool;
  allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS)
  {
    vkDestroyCommandPool(device, commandPool, nullptr);
    throw std::runtime_error("Failed to allocate command buffer");
  }

  // Begin command buffer
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  // Copy from staging to device-local buffer
  VkBufferCopy copyRegion{};
  copyRegion.srcOffset = 0;
  copyRegion.dstOffset = 0;
  copyRegion.size      = bufferSize;
  vkCmdCopyBuffer(commandBuffer, stagingBuffer, vertexBuffer, 1, &copyRegion);

  vkEndCommandBuffer(commandBuffer);

  // Submit command buffer
  VkQueue queue = nullptr;
  vkGetDeviceQueue(device, 0, 0, &queue);

  VkSubmitInfo submitInfo{};
  submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers    = &commandBuffer;

  vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(queue);

  // Cleanup
  vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
  vkDestroyCommandPool(device, commandPool, nullptr);
  vkDestroyBuffer(device, stagingBuffer, nullptr);
  vkFreeMemory(device, stagingBufferMemory, nullptr);
}

} // namespace Rl::Client::Render
