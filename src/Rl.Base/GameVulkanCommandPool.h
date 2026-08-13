#ifndef RL_BASE_GAME_VULKAN_COMMAND_POOL_H
#define RL_BASE_GAME_VULKAN_COMMAND_POOL_H

#include <cstdint>
#include <vulkan/vulkan.hpp>

namespace rl
{

class GameVulkanCommandPool
{
        public:
                GameVulkanCommandPool() noexcept;
                GameVulkanCommandPool(VkDevice device, uint32_t queueFamilyIndex);
                GameVulkanCommandPool(VkDevice      device,
                                      VkCommandPool commandPool,
                                      bool          ownsCommandPool = true);
                GameVulkanCommandPool(GameVulkanCommandPool&& other) noexcept;
                GameVulkanCommandPool& operator=(GameVulkanCommandPool&& other) noexcept;
                GameVulkanCommandPool(const GameVulkanCommandPool& other)            = delete;
                GameVulkanCommandPool& operator=(const GameVulkanCommandPool& other) = delete;
                ~GameVulkanCommandPool();

                VkCommandPool getCommandPool() const;

        private:
                VkDevice      device;
                VkCommandPool commandPool;
                bool          ownsCommandPool;
};

} // namespace rl

#endif
