#include "Rl.Base/GameVulkanBuffer.h"
#include "Rl.Base/GameError.h"

#include <vulkan/vulkan.hpp>

namespace rl
{

void GameVulkanBuffer::cleanupExistingResources()
{
  if (dedicated)
  {
    if (dedicatedMemory != VK_NULL_HANDLE && allocator != nullptr)
    {
      vkFreeMemory(allocator->getDevice(), dedicatedMemory, nullptr);
    }
    if (dedicatedBuffer != VK_NULL_HANDLE && allocator != nullptr)
    {
      vkDestroyBuffer(allocator->getDevice(), dedicatedBuffer, nullptr);
    }
  }
  else
  {
    if (allocator != nullptr)
    {
      allocator->free(allocation);
    }
  }
}

void GameVulkanBuffer::copyMetadata(const GameVulkanBuffer& other)
{
  allocator       = other.allocator;
  size            = other.size;
  usage           = other.usage;
  properties      = other.properties;
  dedicated       = other.dedicated;
  dedicatedBuffer = VK_NULL_HANDLE;
  dedicatedMemory = VK_NULL_HANDLE;
}

void GameVulkanBuffer::createDedicatedBuffer()
{
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size        = size;
  bufferInfo.usage       = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(allocator->getDevice(), &bufferInfo, nullptr, &dedicatedBuffer) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create dedicated buffer");
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(allocator->getDevice(), dedicatedBuffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex =
      allocator->findMemoryType(allocator->getPhysicalDevice(), memRequirements, properties);

  if (vkAllocateMemory(allocator->getDevice(), &allocInfo, nullptr, &dedicatedMemory) != VK_SUCCESS)
  {
    vkDestroyBuffer(allocator->getDevice(), dedicatedBuffer, nullptr);
    throw std::runtime_error("Failed to allocate dedicated buffer memory");
  }

  vkBindBufferMemory(allocator->getDevice(), dedicatedBuffer, dedicatedMemory, 0);
}

void GameVulkanBuffer::createSuballocatedBuffer()
{
  allocation = allocator->allocate(size, 16, usage, properties);
}

void GameVulkanBuffer::copyBufferData(const GameVulkanBuffer& other)
{
  VkBufferCreateInfo stagingBufferInfo{};
  stagingBufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  stagingBufferInfo.size        = size;
  stagingBufferInfo.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VkBuffer stagingBuffer;
  if (vkCreateBuffer(allocator->getDevice(), &stagingBufferInfo, nullptr, &stagingBuffer) !=
      VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create staging buffer");
  }

  VkMemoryRequirements stagingMemRequirements;
  vkGetBufferMemoryRequirements(allocator->getDevice(), stagingBuffer, &stagingMemRequirements);

  VkMemoryAllocateInfo stagingAllocInfo{};
  stagingAllocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  stagingAllocInfo.allocationSize  = stagingMemRequirements.size;
  stagingAllocInfo.memoryTypeIndex = allocator->findMemoryType(
      allocator->getPhysicalDevice(), stagingMemRequirements,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  VkDeviceMemory stagingMemory;
  if (vkAllocateMemory(allocator->getDevice(), &stagingAllocInfo, nullptr, &stagingMemory) !=
      VK_SUCCESS)
  {
    vkDestroyBuffer(allocator->getDevice(), stagingBuffer, nullptr);
    throw std::runtime_error("Failed to allocate staging buffer memory");
  }

  vkBindBufferMemory(allocator->getDevice(), stagingBuffer, stagingMemory, 0);

  VkDeviceSize srcOffset = dedicated ? 0 : other.allocation.offset;
  void*        srcData;
  vkMapMemory(allocator->getDevice(), dedicated ? other.dedicatedMemory : other.allocation.memory,
              srcOffset, size, 0, &srcData);
  void* stagingData;
  vkMapMemory(allocator->getDevice(), stagingMemory, 0, size, 0, &stagingData);
  memcpy(stagingData, srcData, size);
  vkUnmapMemory(allocator->getDevice(), stagingMemory);
  vkUnmapMemory(allocator->getDevice(),
                dedicated ? other.dedicatedMemory : other.allocation.memory);

  VkDeviceSize dstOffset = dedicated ? 0 : allocation.offset;
  void*        dstData;
  vkMapMemory(allocator->getDevice(), dedicated ? dedicatedMemory : allocation.memory, dstOffset,
              size, 0, &dstData);
  vkMapMemory(allocator->getDevice(), stagingMemory, 0, size, 0, &stagingData);
  memcpy(dstData, stagingData, size);
  vkUnmapMemory(allocator->getDevice(), dedicated ? dedicatedMemory : allocation.memory);
  vkUnmapMemory(allocator->getDevice(), stagingMemory);

  vkFreeMemory(allocator->getDevice(), stagingMemory, nullptr);
  vkDestroyBuffer(allocator->getDevice(), stagingBuffer, nullptr);
}

GameVulkanBuffer::GameVulkanBuffer() :
    allocator(nullptr), size(0), usage(0), properties(0), dedicated(false),
    dedicatedBuffer(VK_NULL_HANDLE), dedicatedMemory(VK_NULL_HANDLE)
{
}

static constexpr size_t bufferAlign = 16;

GameVulkanBuffer::GameVulkanBuffer(GameVulkanMemoryAllocator* allocator,
                                   VkDeviceSize               size,
                                   VkBufferUsageFlags         usage,
                                   VkMemoryPropertyFlags      properties,
                                   bool                       dedicated) :
    allocator(allocator), size(size), usage(usage), properties(properties), dedicated(dedicated),
    dedicatedBuffer(VK_NULL_HANDLE), dedicatedMemory(VK_NULL_HANDLE)
{
  if (dedicated)
  {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size        = size;
    bufferInfo.usage       = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(allocator->getDevice(), &bufferInfo, nullptr, &dedicatedBuffer) !=
        VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create dedicated buffer");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(allocator->getDevice(), dedicatedBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex =
        allocator->findMemoryType(allocator->getPhysicalDevice(), memRequirements, properties);

    if (vkAllocateMemory(allocator->getDevice(), &allocInfo, nullptr, &dedicatedMemory) !=
        VK_SUCCESS)
    {
      vkDestroyBuffer(allocator->getDevice(), dedicatedBuffer, nullptr);
      throw std::runtime_error("Failed to allocate dedicated buffer memory");
    }

    vkBindBufferMemory(allocator->getDevice(), dedicatedBuffer, dedicatedMemory, 0);
  }
  else
  {
    allocation = allocator->allocate(size, bufferAlign, usage, properties);
  }
}

GameVulkanBuffer::GameVulkanBuffer(GameVulkanMemoryAllocator* allocator, VkDeviceSize size) :
    allocator(allocator), size(size), usage(VK_BUFFER_USAGE_INDEX_BUFFER_BIT),
    properties(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
{
  allocation = allocator->allocate(size, bufferAlign, usage, properties);
}

GameVulkanBuffer::GameVulkanBuffer(GameVulkanBuffer&& other) noexcept :
    allocator(other.allocator), size(other.size), usage(other.usage), properties(other.properties),
    dedicated(other.dedicated), dedicatedBuffer(other.dedicatedBuffer),
    dedicatedMemory(other.dedicatedMemory), allocation(other.allocation)
{
  other.allocator       = nullptr;
  other.size            = 0;
  other.usage           = 0;
  other.properties      = 0;
  other.dedicated       = false;
  other.dedicatedBuffer = VK_NULL_HANDLE;
  other.dedicatedMemory = VK_NULL_HANDLE;
  other.allocation      = GameVulkanSuballocation{};
}

GameVulkanBuffer::~GameVulkanBuffer()
{
  if (dedicated)
  {
    if (dedicatedMemory != VK_NULL_HANDLE && allocator != nullptr)
    {
      vkFreeMemory(allocator->getDevice(), dedicatedMemory, nullptr);
    }
    if (dedicatedBuffer != VK_NULL_HANDLE && allocator != nullptr)
    {
      vkDestroyBuffer(allocator->getDevice(), dedicatedBuffer, nullptr);
    }
  }
  else
  {
    if (allocator != nullptr)
    {
      allocator->free(allocation);
    }
  }
}

GameVulkanBuffer& GameVulkanBuffer::operator=(const GameVulkanBuffer& other)
{
  if (this != &other)
  {
    cleanupExistingResources();
    copyMetadata(other);

    if (dedicated)
    {
      createDedicatedBuffer();
    }
    else
    {
      createSuballocatedBuffer();
    }
    bool hasSourceData = dedicated ? (other.dedicatedBuffer != VK_NULL_HANDLE &&
                                      other.dedicatedMemory != VK_NULL_HANDLE)
                                   : (other.allocation.buffer != VK_NULL_HANDLE &&
                                      other.allocation.memory != VK_NULL_HANDLE);

    if (hasSourceData)
    {
      copyBufferData(other);
    }
  }
  return *this;
}

GameVulkanBuffer& GameVulkanBuffer::operator=(GameVulkanBuffer&& other) noexcept
{
  if (this != &other)
  {
    if (dedicated)
    {
      if (dedicatedMemory != VK_NULL_HANDLE && allocator != nullptr)
      {
        vkFreeMemory(allocator->getDevice(), dedicatedMemory, nullptr);
      }
      if (dedicatedBuffer != VK_NULL_HANDLE && allocator != nullptr)
      {
        vkDestroyBuffer(allocator->getDevice(), dedicatedBuffer, nullptr);
      }
    }
    else
    {
      if (allocator != nullptr)
      {
        allocator->free(allocation);
      }
    }
    allocator       = other.allocator;
    size            = other.size;
    usage           = other.usage;
    properties      = other.properties;
    dedicated       = other.dedicated;
    dedicatedBuffer = other.dedicatedBuffer;
    dedicatedMemory = other.dedicatedMemory;
    allocation      = other.allocation;

    other.allocator       = nullptr;
    other.size            = 0;
    other.usage           = 0;
    other.properties      = 0;
    other.dedicated       = false;
    other.dedicatedBuffer = VK_NULL_HANDLE;
    other.dedicatedMemory = VK_NULL_HANDLE;
    other.allocation      = GameVulkanSuballocation{};
  }
  return *this;
}

VkDeviceSize GameVulkanBuffer::getSize() const
{
  return size;
}

VkBufferUsageFlags GameVulkanBuffer::getUsage() const
{
  return usage;
}

VkMemoryPropertyFlags GameVulkanBuffer::getProperties() const
{
  return properties;
}

VkBuffer GameVulkanBuffer::getBuffer() const
{
  return dedicated ? dedicatedBuffer : allocation.buffer;
}

VkDeviceSize GameVulkanBuffer::getOffset() const
{
  return dedicated ? 0 : allocation.offset;
}

VkDeviceMemory GameVulkanBuffer::getMemory() const
{
  return dedicated ? dedicatedMemory : allocation.memory;
}

void* GameVulkanBuffer::map(VkDeviceSize size, VkDeviceSize offset, VkMemoryMapFlags flags)
{
  VkDeviceMemory memory = getMemory();
  VkDeviceSize actualOffset = getOffset() + offset;
  void* data;
  VkResult result = vkMapMemory(allocator->getDevice(), memory, actualOffset, size, flags, &data);
  if (result != VK_SUCCESS)
  {
    GameError::exitWithError("vkMapMemory",
                             "Failed to map buffer memory (result = " +
                                 GameError::vulkanResultToString(result) + ")");
  }
  return data;
}

void GameVulkanBuffer::unmap()
{
  VkDeviceMemory memory = getMemory();
  vkUnmapMemory(allocator->getDevice(), memory);
}

} // namespace rl
