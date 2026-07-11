import Rl.Client.Render.Unit.UnitRendererBasicBuffer;
import Rl.Client.Render.Unit.UnitRendererInfo;
import Rl.Client.State.UnitState;

import <array>;
import <cstddef>;
import <glm/glm.hpp>;
import <vulkan/vulkan.hpp>;

namespace Rl::Client::Render
{

uint32_t UnitFindMemoryTypeIndex(VkPhysicalDevice physicalDevice,
    VkMemoryRequirements                          memRequirements,
    VkMemoryPropertyFlags                         properties)
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

void UnitCreateBuffer(VkDevice device,
    VkPhysicalDevice           physicalDevice,
    VkDeviceSize               size,
    VkBufferUsageFlags         usage,
    VkMemoryPropertyFlags      properties,
    VkBuffer&                  buffer,
    VkDeviceMemory&            bufferMemory)
{
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create buffer");
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex =
      UnitFindMemoryTypeIndex(physicalDevice, memRequirements, properties);

  if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to allocate buffer memory");
  }

  vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

void UnitCopyDataToBuffer(VkDevice device,
    VkDeviceMemory                 bufferMemory,
    VkDeviceSize                   offset,
    VkDeviceSize                   size,
    const void*                    data)
{
  void* mappedData;
  vkMapMemory(device, bufferMemory, offset, size, 0, &mappedData);
  memcpy(mappedData, data, size);
  vkUnmapMemory(device, bufferMemory);
}

// Create index buffer for the cube
void UnitCreateIndexBuffer(VkDevice device,
    VkPhysicalDevice                physicalDevice,
    uint32_t                        queueFamilyIndex,
    const std::vector<uint32_t>&    indices,
    VkBuffer&                       indexBuffer,
    VkDeviceMemory&                 indexBufferMemory)
{
  VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

  // Create staging buffer (host-visible)
  VkBuffer       stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  UnitCreateBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      stagingBuffer, stagingBufferMemory);

  // Copy data to staging buffer
  UnitCopyDataToBuffer(device, stagingBufferMemory, 0, bufferSize, indices.data());

  // Create device-local index buffer
  UnitCreateBuffer(device, physicalDevice, bufferSize,
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
          VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

  // Create temporary command pool for buffer copy
  VkCommandPoolCreateInfo commandPoolInfo{};
  commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
  commandPoolInfo.queueFamilyIndex = queueFamilyIndex;

  VkCommandPool commandPool;
  if (vkCreateCommandPool(device, &commandPoolInfo, nullptr, &commandPool) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create temporary command pool");
  }

  // Allocate command buffer
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
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
  copyRegion.size = bufferSize;
  vkCmdCopyBuffer(commandBuffer, stagingBuffer, indexBuffer, 1, &copyRegion);

  vkEndCommandBuffer(commandBuffer);

  // Submit command buffer
  VkQueue queue = nullptr;
  vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(queue);

  // Cleanup
  vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
  vkDestroyCommandPool(device, commandPool, nullptr);
  vkDestroyBuffer(device, stagingBuffer, nullptr);
  vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void UnitCreateUniformBuffers(
    VkDevice device, VkPhysicalDevice physicalDevice, Providers::UnitStateBinding& vk)
{
  // Create triplanar settings buffer
  UnitCreateBuffer(device, physicalDevice, sizeof(UnitRenderTriplanarSettings),
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      vk.triplanarSettingsBuffer, vk.triplanarSettingsBufferMemory);

  // Initialize triplanar settings
  UnitRenderTriplanarSettings initialTriplanar{};
  initialTriplanar.scale = 1.0f;
  initialTriplanar.sharpness = 4.0f;
  initialTriplanar.offsetX = 0.0f;
  initialTriplanar.offsetY = 0.0f;
  initialTriplanar.offsetZ = 0.0f;
  initialTriplanar.blendMix = 1.0f;
  UnitCopyDataToBuffer(device, vk.triplanarSettingsBufferMemory, 0,
      sizeof(UnitRenderTriplanarSettings), &initialTriplanar);
}

void UnitCreateUnitArrayBuffer(VkDevice device,
    VkPhysicalDevice                       physicalDevice,
    uint32_t                               queueFamilyIndex,
    const std::vector<UnitRenderUnitData>& unitData,
    VkBuffer&                              unitArrayBuffer,
    VkDeviceMemory&                        unitArrayMemory)
{
  // Calculate buffer size: count (uint32_t) + padding (3 * uint32_t) + unit data array
  VkDeviceSize bufferSize = sizeof(uint32_t) * 4 + sizeof(UnitRenderUnitData) * unitData.size();

  // Create staging buffer
  VkBuffer       stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  UnitCreateBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      stagingBuffer, stagingBufferMemory);

  // Prepare data with count header
  std::vector<uint8_t> stagingData(bufferSize);
  uint32_t* header = reinterpret_cast<uint32_t*>(stagingData.data());
  header[0] = static_cast<uint32_t>(unitData.size());
  header[1] = 0; // padding
  header[2] = 0; // padding
  header[3] = 0; // padding

  // Copy unit data
  if (!unitData.empty())
  {
    memcpy(stagingData.data() + sizeof(uint32_t) * 4, unitData.data(),
           sizeof(UnitRenderUnitData) * unitData.size());
  }

  // Copy to staging buffer
  UnitCopyDataToBuffer(device, stagingBufferMemory, 0, bufferSize, stagingData.data());

  // Create device-local buffer
  UnitCreateBuffer(device, physicalDevice, bufferSize,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, unitArrayBuffer, unitArrayMemory);

  // Copy from staging to device-local buffer (similar to index buffer)
  VkCommandPoolCreateInfo commandPoolInfo{};
  commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
  commandPoolInfo.queueFamilyIndex = queueFamilyIndex;

  VkCommandPool commandPool;
  if (vkCreateCommandPool(device, &commandPoolInfo, nullptr, &commandPool) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create temporary command pool");
  }

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS)
  {
    vkDestroyCommandPool(device, commandPool, nullptr);
    throw std::runtime_error("Failed to allocate command buffer");
  }

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  VkBufferCopy copyRegion{};
  copyRegion.srcOffset = 0;
  copyRegion.dstOffset = 0;
  copyRegion.size = bufferSize;
  vkCmdCopyBuffer(commandBuffer, stagingBuffer, unitArrayBuffer, 1, &copyRegion);

  vkEndCommandBuffer(commandBuffer);

  VkQueue queue;
  vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;
  vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(queue);

  vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
  vkDestroyCommandPool(device, commandPool, nullptr);
  vkDestroyBuffer(device, stagingBuffer, nullptr);
  vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void UnitCreatePolFenceArrayBuffer(VkDevice device,
    VkPhysicalDevice                       physicalDevice,
    uint32_t                               queueFamilyIndex,
    const std::vector<UnitRenderPolFence>& fenceData,
    VkBuffer&                              fenceArrayBuffer,
    VkDeviceMemory&                        fenceArrayMemory)
{
  // Calculate buffer size: count (uint32_t) + padding (3 * uint32_t) + fence data array
  VkDeviceSize bufferSize = sizeof(uint32_t) * 4 + sizeof(UnitRenderPolFence) * fenceData.size();

  // Create staging buffer
  VkBuffer       stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  UnitCreateBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      stagingBuffer, stagingBufferMemory);

  // Prepare data with count header
  std::vector<uint8_t> stagingData(bufferSize);
  uint32_t* header = reinterpret_cast<uint32_t*>(stagingData.data());
  header[0] = static_cast<uint32_t>(fenceData.size());
  header[1] = 0; // padding
  header[2] = 0; // padding
  header[3] = 0; // padding

  // Copy fence data
  if (!fenceData.empty())
  {
    memcpy(stagingData.data() + sizeof(uint32_t) * 4, fenceData.data(),
           sizeof(UnitRenderPolFence) * fenceData.size());
  }

  // Copy to staging buffer
  UnitCopyDataToBuffer(device, stagingBufferMemory, 0, bufferSize, stagingData.data());

  // Create device-local buffer
  UnitCreateBuffer(device, physicalDevice, bufferSize,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, fenceArrayBuffer, fenceArrayMemory);

  // Copy from staging to device-local buffer
  VkCommandPoolCreateInfo commandPoolInfo{};
  commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
  commandPoolInfo.queueFamilyIndex = queueFamilyIndex;

  VkCommandPool commandPool;
  if (vkCreateCommandPool(device, &commandPoolInfo, nullptr, &commandPool) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create temporary command pool");
  }

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS)
  {
    vkDestroyCommandPool(device, commandPool, nullptr);
    throw std::runtime_error("Failed to allocate command buffer");
  }

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  VkBufferCopy copyRegion{};
  copyRegion.srcOffset = 0;
  copyRegion.dstOffset = 0;
  copyRegion.size = bufferSize;
  vkCmdCopyBuffer(commandBuffer, stagingBuffer, fenceArrayBuffer, 1, &copyRegion);

  vkEndCommandBuffer(commandBuffer);

  VkQueue queue;
  vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;
  vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(queue);

  vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
  vkDestroyCommandPool(device, commandPool, nullptr);
  vkDestroyBuffer(device, stagingBuffer, nullptr);
  vkFreeMemory(device, stagingBufferMemory, nullptr);
}

} // namespace Rl::Client::Render
