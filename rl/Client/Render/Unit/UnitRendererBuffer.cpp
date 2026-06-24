#include "rl/Client/Render/Unit/UnitRendererBasicBuffer.h"

#include <array>
#include <cstddef>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

namespace Rl::Client::Render
{

static uint32_t UnitFindMemoryTypeIndex(VkPhysicalDevice physicalDevice,
    VkMemoryRequirements                                 memRequirements,
    VkMemoryPropertyFlags                                properties)
{
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
  {
    if ((memRequirements.memoryTypeBits & (1 << i)) &&
        (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
    {
      return i;
    }
  }
  throw std::runtime_error("Failed to find suitable memory type");
}

// Helper function to create a buffer with specified properties
void UnitCreateBuffer(VkDevice device,
    VkPhysicalDevice       physicalDevice,
    VkDeviceSize           size,
    VkBufferUsageFlags     usage,
    VkMemoryPropertyFlags  properties,
    VkBuffer&              buffer,
    VkDeviceMemory&        bufferMemory)
{
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size        = size;
  bufferInfo.usage       = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create buffer");
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize  = memRequirements.size;
  allocInfo.memoryTypeIndex = UnitFindMemoryTypeIndex(physicalDevice, memRequirements, properties);

  if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to allocate buffer memory");
  }

  vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

void UnitCopyDataToBuffer(VkDevice device,
    VkDeviceMemory             bufferMemory,
    VkDeviceSize               offset,
    VkDeviceSize               size,
    const void*                data)
{
  void* mappedData;
  vkMapMemory(device, bufferMemory, offset, size, 0, &mappedData);
  memcpy(mappedData, data, size);
  vkUnmapMemory(device, bufferMemory);
}

// Create index buffer for the cube
void UnitCreateIndexBuffer(VkDevice  device,
    VkPhysicalDevice             physicalDevice,
    const std::vector<uint32_t>& indices,
    VkBuffer&                    indexBuffer,
    VkDeviceMemory&              indexBufferMemory)
{
  VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

  // Create staging buffer (host-visible)
  VkBuffer       stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  UnitCreateBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
      stagingBufferMemory);

  // Copy data to staging buffer
  UnitCopyDataToBuffer(device, stagingBufferMemory, 0, bufferSize, indices.data());

  // Create device-local index buffer
  UnitCreateBuffer(device, physicalDevice, bufferSize,
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
          VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

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
  vkCmdCopyBuffer(commandBuffer, stagingBuffer, indexBuffer, 1, &copyRegion);

  vkEndCommandBuffer(commandBuffer);

  // Submit command buffer
  VkQueue queue = nullptr;
  // For now, we'll assume queue 0 exists
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

void UnitCreateUniformBuffers(
    VkDevice device, VkPhysicalDevice physicalDevice, Providers::UnitStateDrawableVulkan& vk)
{
  // Create triplanar settings buffer
  UnitCreateBuffer(device, physicalDevice, sizeof(UnitRenderTriplanarSettings),
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      vk.triplanarSettingsBuffer, vk.triplanarSettingsBufferMemory);

  // Initialize triplanar settings
  UnitRenderTriplanarSettings initialTriplanar{};
  initialTriplanar.scale     = 1.0f;
  initialTriplanar.sharpness = 2.0f;
  initialTriplanar.offsetX   = 0.0f;
  initialTriplanar.offsetY   = 0.0f;
  initialTriplanar.offsetZ   = 0.0f;
  initialTriplanar.blendMix  = 1.0f;
  CopyDataToBuffer(device, vk.triplanarSettingsBufferMemory, 0, sizeof(UnitRenderTriplanarSettings),
      &initialTriplanar);
}

} // namespace Rl::Client::Render
