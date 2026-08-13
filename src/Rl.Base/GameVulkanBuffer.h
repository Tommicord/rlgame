#ifndef RL_BASE_GAME_VULKAN_BUFFER_H
#define RL_BASE_GAME_VULKAN_BUFFER_H

#include "Rl.Base/GameVulkanAllocator.h"

#include <cstddef>
#include <cstdint>
#include <vulkan/vulkan.hpp>

namespace rl
{

class GameVulkanBuffer
{
        public:
                GameVulkanBuffer();
                GameVulkanBuffer(GameVulkanMemoryAllocator* allocator,
                                 VkDeviceSize               size,
                                 VkBufferUsageFlags         usage,
                                 VkMemoryPropertyFlags      properties,
                                 bool                       dedicated = false);
                GameVulkanBuffer(GameVulkanMemoryAllocator* allocator, VkDeviceSize size);
                GameVulkanBuffer(const GameVulkanBuffer& other) = delete;
                GameVulkanBuffer(GameVulkanBuffer&& other) noexcept;
                GameVulkanBuffer& operator=(const GameVulkanBuffer& other);
                GameVulkanBuffer& operator=(GameVulkanBuffer&& other) noexcept;
                ~GameVulkanBuffer();

                VkDeviceSize          getSize() const;
                VkBufferUsageFlags    getUsage() const;
                VkMemoryPropertyFlags getProperties() const;
                VkBuffer              getBuffer() const;
                VkDeviceSize          getOffset() const;
                VkDeviceMemory        getMemory() const;

        private:
                void cleanupExistingResources();
                void copyMetadata(const GameVulkanBuffer& other);
                void createDedicatedBuffer();
                void createSuballocatedBuffer();
                void copyBufferData(const GameVulkanBuffer& other);

                GameVulkanMemoryAllocator* allocator;
                GameVulkanSuballocation    allocation;
                VkDeviceSize               size;
                VkBufferUsageFlags         usage;
                VkMemoryPropertyFlags      properties;
                bool                       dedicated;
                VkBuffer                   dedicatedBuffer;
                VkDeviceMemory             dedicatedMemory;
};

} // namespace rl

#endif
