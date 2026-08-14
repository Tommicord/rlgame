#ifndef RL_BASE_GAME_VULKAN_COMMAND_BUFFER_H
#define RL_BASE_GAME_VULKAN_COMMAND_BUFFER_H

#include <vulkan/vulkan.hpp>

namespace rl
{

class GameVulkanCommandBuffer
{
  public:
    GameVulkanCommandBuffer();
    GameVulkanCommandBuffer(VkDevice device, VkCommandPool commandPool);
    GameVulkanCommandBuffer(const GameVulkanCommandBuffer& other)            = delete;
    GameVulkanCommandBuffer& operator=(const GameVulkanCommandBuffer& other) = delete;
    GameVulkanCommandBuffer(GameVulkanCommandBuffer&& other) noexcept;
    GameVulkanCommandBuffer& operator=(GameVulkanCommandBuffer&& other) noexcept;
    ~GameVulkanCommandBuffer();

    VkCommandBuffer getCommandBuffer() const;
    VkCommandPool   getCommandPool() const;
    VkDevice        getDevice() const;

    void begin(VkCommandBufferUsageFlags flags = 0);
    void end();
    void reset(VkCommandBufferResetFlags flags = 0);

  private:
    VkDevice        device;
    VkCommandPool   commandPool;
    VkCommandBuffer commandBuffer;
};

} // namespace rl

#endif
