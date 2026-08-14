#ifndef RL_BASE_GAME_VULKAN_ALLOCATOR_H
#define RL_BASE_GAME_VULKAN_ALLOCATOR_H

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace rl
{

void bufferCopyData(VkDevice       device,
                    VkDeviceMemory bufferMemory,
                    VkDeviceSize   offset,
                    VkDeviceSize   size,
                    const void*    data);

struct GameVulkanSuballocation
{
                VkDeviceSize   offset;
                VkDeviceSize   size;
                VkBuffer       buffer;
                VkDeviceMemory memory;
                uint32_t       blockIndex;
};

class GameVulkanMemoryBlock
{
        public:
                GameVulkanMemoryBlock(VkDevice              device,
                                      VkPhysicalDevice      physicalDevice,
                                      VkDeviceSize          size,
                                      VkBufferUsageFlags    usage,
                                      VkMemoryPropertyFlags properties);
                ~GameVulkanMemoryBlock();

                VkDeviceSize          getSize() const;
                VkDeviceSize          getUsedSize() const;
                VkBuffer              getBuffer() const;
                VkDeviceMemory        getMemory() const;
                VkBufferUsageFlags    getUsage() const;
                VkMemoryPropertyFlags getProperties() const;

                bool allocate(VkDeviceSize             size,
                              VkDeviceSize             alignment,
                              GameVulkanSuballocation& outAllocation);
                void free(const GameVulkanSuballocation& allocation);

        private:
                struct FreeRegion
                {
                                VkDeviceSize offset;
                                VkDeviceSize size;
                                FreeRegion(VkDeviceSize offset, VkDeviceSize size) :
                                    offset(offset), size(size)
                                {
                                }
                                ~FreeRegion() = default;
                };

                VkDevice                device;
                VkPhysicalDevice        physicalDevice;
                VkDeviceSize            size;
                VkBufferUsageFlags      usage;
                VkMemoryPropertyFlags   properties;
                VkBuffer                buffer;
                VkDeviceMemory          memory;
                VkDeviceSize            usedSize;
                std::vector<FreeRegion> freeRegions;

                void createBuffer();
};

class GameVulkanMemoryAllocator
{
                static constexpr VkDeviceSize defaultMaxBlockSize =
                    64000000; // 64 MB max block size
                static constexpr VkDeviceSize minBlockSize = 256; // 256 bytes minimum block size
        public:
                GameVulkanMemoryAllocator();
                GameVulkanMemoryAllocator(VkDevice device, VkPhysicalDevice physicalDevice);
                ~GameVulkanMemoryAllocator();
                GameVulkanMemoryAllocator(const GameVulkanMemoryAllocator& other) = delete;
                GameVulkanMemoryAllocator(GameVulkanMemoryAllocator&& other) noexcept;
                GameVulkanMemoryAllocator& operator=(const GameVulkanMemoryAllocator& other);
                GameVulkanMemoryAllocator& operator=(GameVulkanMemoryAllocator&& other) noexcept;

                GameVulkanSuballocation allocate(VkDeviceSize          size,
                                                 VkDeviceSize          alignment,
                                                 VkBufferUsageFlags    usage,
                                                 VkMemoryPropertyFlags properties);
                void                    free(const GameVulkanSuballocation& allocation);

                VkDevice         getDevice() const;
                VkPhysicalDevice getPhysicalDevice() const;
                static uint32_t  findMemoryType(VkPhysicalDevice            physicalDevice,
                                                const VkMemoryRequirements& memRequirements,
                                                VkMemoryPropertyFlags       properties);

        private:
                VkDevice                            device;
                VkPhysicalDevice                    physicalDevice;
                std::vector<GameVulkanMemoryBlock*> blocks;

                VkDeviceSize getBlockSize(VkDeviceSize requestedSize) const;
                VkDeviceSize getAdjustedAlignment(VkDeviceSize       requestedAlignment,
                                                  VkBufferUsageFlags usage) const;
};

} // namespace rl

#endif
