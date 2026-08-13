#include "Rl.Base/GameVulkanAllocator.h"
#include "Rl.Base/GameError.h"

#include <algorithm>
#include <cstring>
#include <vulkan/vulkan.hpp>

namespace rl
{

uint32_t GameVulkanMemoryAllocator::findMemoryType(VkPhysicalDevice            physicalDevice,
                                                   const VkMemoryRequirements& memRequirements,
                                                   VkMemoryPropertyFlags       properties)
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
        return UINT32_MAX;
}

void bufferCopyData(VkDevice       device,
                    VkDeviceMemory bufferMemory,
                    VkDeviceSize   offset,
                    VkDeviceSize   size,
                    const void*    data)
{
        void* mapped;
        vkMapMemory(device, bufferMemory, offset, size, 0, &mapped);
        memcpy(mapped, data, size);
        vkUnmapMemory(device, bufferMemory);
}

GameVulkanMemoryBlock::GameVulkanMemoryBlock(VkDevice              device,
                                             VkPhysicalDevice      physicalDevice,
                                             VkDeviceSize          size,
                                             VkBufferUsageFlags    usage,
                                             VkMemoryPropertyFlags properties) :
    device(device), physicalDevice(physicalDevice), size(size), usage(usage),
    properties(properties), buffer(VK_NULL_HANDLE), memory(VK_NULL_HANDLE), usedSize(0)
{
        createBuffer();
        freeRegions.emplace_back(0, size);
}

GameVulkanMemoryBlock::~GameVulkanMemoryBlock()
{
        if (buffer != VK_NULL_HANDLE)
        {
                vkDestroyBuffer(device, buffer, nullptr);
                buffer = VK_NULL_HANDLE;
        }
        if (memory != VK_NULL_HANDLE)
        {
                vkFreeMemory(device, memory, nullptr);
                memory = VK_NULL_HANDLE;
        }
}

VkDeviceSize GameVulkanMemoryBlock::getSize() const
{
        return size;
}

VkDeviceSize GameVulkanMemoryBlock::getUsedSize() const
{
        return usedSize;
}

VkBuffer GameVulkanMemoryBlock::getBuffer() const
{
        return buffer;
}

VkDeviceMemory GameVulkanMemoryBlock::getMemory() const
{
        return memory;
}

VkBufferUsageFlags GameVulkanMemoryBlock::getUsage() const
{
        return usage;
}

VkMemoryPropertyFlags GameVulkanMemoryBlock::getProperties() const
{
        return properties;
}

bool GameVulkanMemoryBlock::allocate(VkDeviceSize             size,
                                     VkDeviceSize             alignment,
                                     GameVulkanSuballocation& outAllocation)
{
        for (size_t i = 0; i < freeRegions.size(); ++i)
        {
                VkDeviceSize alignedOffset =
                    (freeRegions[i].offset + alignment - 1) & ~(alignment - 1);
                VkDeviceSize padding = alignedOffset - freeRegions[i].offset;

                if (freeRegions[i].size >= size + padding)
                {
                        outAllocation.offset     = alignedOffset;
                        outAllocation.size       = size;
                        outAllocation.buffer     = buffer;
                        outAllocation.memory     = memory;
                        outAllocation.blockIndex = 0;

                        usedSize += size;

                        if (freeRegions[i].size == size + padding)
                        {
                                freeRegions.erase(freeRegions.begin() + i);
                        }
                        else
                        {
                                freeRegions[i].offset = alignedOffset + size;
                                freeRegions[i].size -= size + padding;
                        }

                        return true;
                }
        }
        return false;
}

void GameVulkanMemoryBlock::free(const GameVulkanSuballocation& allocation)
{
        VkDeviceSize allocEnd = allocation.offset + allocation.size;

        for (size_t i = 0; i < freeRegions.size(); ++i)
        {
                if (freeRegions[i].offset == allocEnd)
                {
                        freeRegions[i].offset = allocation.offset;
                        freeRegions[i].size += allocation.size;

                        if (i > 0 && freeRegions[i - 1].offset + freeRegions[i - 1].size ==
                                         freeRegions[i].offset)
                        {
                                freeRegions[i - 1].size += freeRegions[i].size;
                                freeRegions.erase(freeRegions.begin() + i);
                        }
                        usedSize -= allocation.size;
                        return;
                }
                if (freeRegions[i].offset + freeRegions[i].size == allocation.offset)
                {
                        freeRegions[i].size += allocation.size;

                        if (i + 1 < freeRegions.size() &&
                            freeRegions[i].offset + freeRegions[i].size ==
                                freeRegions[i + 1].offset)
                        {
                                freeRegions[i].size += freeRegions[i + 1].size;
                                freeRegions.erase(freeRegions.begin() + i + 1);
                        }
                        usedSize -= allocation.size;
                        return;
                }
        }

        freeRegions.emplace_back(allocation.offset, allocation.size);
        std::sort(freeRegions.begin(), freeRegions.end(),
                  [](const FreeRegion& a, const FreeRegion& b) { return a.offset < b.offset; });
        usedSize -= allocation.size;
}

void GameVulkanMemoryBlock::createBuffer()
{
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size        = size;
        bufferInfo.usage       = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        try
        {
                if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
                {
                        throw std::runtime_error("Failed to create buffer");
                }
        }
        catch (std::runtime_error& e)
        {
                GameError::exitWithError("Vulkan Buffer Creation Error", e.what());
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = size;
        allocInfo.memoryTypeIndex =
            GameVulkanMemoryAllocator::findMemoryType(physicalDevice, memRequirements, properties);

        try
        {
                if (vkAllocateMemory(device, &allocInfo, nullptr, &memory) != VK_SUCCESS)
                {
                        throw std::runtime_error("Failed to allocate buffer memory");
                }
        }
        catch (std::runtime_error& e)
        {
                GameError::exitWithError("Vulkan Buffer Memory Allocation Error",
                                                        e.what());
        }
        vkBindBufferMemory(device, buffer, memory, 0);
}

GameVulkanMemoryAllocator::GameVulkanMemoryAllocator() :
    device(VK_NULL_HANDLE), physicalDevice(VK_NULL_HANDLE)
{
}

GameVulkanMemoryAllocator::GameVulkanMemoryAllocator(VkDevice         device,
                                                     VkPhysicalDevice physicalDevice) :
    device(device), physicalDevice(physicalDevice)
{
}

GameVulkanMemoryAllocator::~GameVulkanMemoryAllocator()
{
        for (auto* block : blocks)
        {
                delete block;
        }
        blocks.clear();
}

GameVulkanMemoryAllocator&
GameVulkanMemoryAllocator::operator=(const GameVulkanMemoryAllocator& other)
{
        if (this != &other)
        {
                for (auto* block : blocks)
                {
                        delete block;
                }
                blocks.clear();

                device         = other.device;
                physicalDevice = other.physicalDevice;
        }
        return *this;
}

GameVulkanMemoryAllocator&
GameVulkanMemoryAllocator::operator=(GameVulkanMemoryAllocator&& other) noexcept
{
        if (this != &other)
        {
                for (auto* block : blocks)
                {
                        delete block;
                }
                blocks.clear();

                device         = other.device;
                physicalDevice = other.physicalDevice;
                blocks         = std::move(other.blocks);

                other.device         = VK_NULL_HANDLE;
                other.physicalDevice = VK_NULL_HANDLE;
        }
        return *this;
}

GameVulkanSuballocation GameVulkanMemoryAllocator::allocate(VkDeviceSize          size,
                                                            VkDeviceSize          alignment,
                                                            VkBufferUsageFlags    usage,
                                                            VkMemoryPropertyFlags properties)
{
        VkDeviceSize adjustedAlignment = getAdjustedAlignment(alignment, usage);

        for (size_t i = 0; i < blocks.size(); ++i)
        {
                if (blocks[i]->getUsage() == usage && blocks[i]->getProperties() == properties)
                {
                        GameVulkanSuballocation allocation;
                        if (blocks[i]->allocate(size, adjustedAlignment, allocation))
                        {
                                allocation.blockIndex = static_cast<uint32_t>(i);
                                return allocation;
                        }
                }
        }

        VkDeviceSize blockSize = getBlockSize(size);

        GameVulkanMemoryBlock* newBlock =
            new GameVulkanMemoryBlock(device, physicalDevice, blockSize, usage, properties);
        blocks.emplace_back(newBlock);

        GameVulkanSuballocation allocation;
        if (!newBlock->allocate(size, adjustedAlignment, allocation))
        {
                throw;
        }
        allocation.blockIndex = static_cast<uint32_t>(blocks.size() - 1);
        return allocation;
}

VkDeviceSize GameVulkanMemoryAllocator::getBlockSize(VkDeviceSize requestedSize) const
{
        if (requestedSize < 65536)
        {
                VkDeviceSize blockSize = requestedSize;
                if (blockSize < minBlockSize)
                {
                        blockSize = minBlockSize;
                }
                blockSize = (blockSize + 255) & ~255;
                return blockSize;
        }
        VkDeviceSize alignedSize = requestedSize;
        if (alignedSize < minBlockSize)
        {
                alignedSize = minBlockSize;
        }
        VkDeviceSize powerOf2 = minBlockSize;
        while (powerOf2 < alignedSize && powerOf2 < defaultMaxBlockSize)
        {
                powerOf2 *= 2;
        }
        VkDeviceSize blockSize = (powerOf2 < defaultMaxBlockSize) ? powerOf2 : defaultMaxBlockSize;
        if (requestedSize > blockSize)
        {
                blockSize = ((requestedSize + 4095) / 4096) * 4096; // Round up to 4KB
        }

        return blockSize;
}

void GameVulkanMemoryAllocator::free(const GameVulkanSuballocation& allocation)
{
        if (allocation.blockIndex < blocks.size())
        {
                blocks[allocation.blockIndex]->free(allocation);
        }
}

VkDevice GameVulkanMemoryAllocator::getDevice() const
{
        return device;
}

VkPhysicalDevice GameVulkanMemoryAllocator::getPhysicalDevice() const
{
        return physicalDevice;
}

VkDeviceSize GameVulkanMemoryAllocator::getAdjustedAlignment(VkDeviceSize       requestedAlignment,
                                                             VkBufferUsageFlags usage) const
{
        VkDeviceSize deviceAlignment = requestedAlignment;
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
        VkPhysicalDeviceLimits limits = deviceProperties.limits;

        if (usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
        {
                deviceAlignment = (std::max)(deviceAlignment, limits.minUniformBufferOffsetAlignment);
        }
        if (usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
        {
                deviceAlignment = (std::max)(deviceAlignment, limits.minStorageBufferOffsetAlignment);
        }

        return deviceAlignment;
}

} // namespace rl
