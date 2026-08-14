#ifndef RL_BASE_GAME_VULKAN_FENCE_H
#define RL_BASE_GAME_VULKAN_FENCE_H

#include <cstdint>
#include <vulkan/vulkan.hpp>

namespace rl
{

struct GameVulkanFenceCreateInfo
{
    VkFenceCreateFlags flags = 0;
};

class GameVulkanFence
{
  public:
    GameVulkanFence() noexcept;
    GameVulkanFence(VkDevice device, const GameVulkanFenceCreateInfo& createInfo);
    GameVulkanFence(GameVulkanFence&& other) noexcept;
    GameVulkanFence(const GameVulkanFence& other)            = delete;
    GameVulkanFence(const GameVulkanFence&& other)           = delete;
    GameVulkanFence& operator=(const GameVulkanFence& other) = delete;
    GameVulkanFence& operator=(GameVulkanFence&& other) noexcept;
    ~GameVulkanFence();

    VkFence  getFence() const;
    VkDevice getDevice() const;

    void wait(uint64_t timeout = UINT64_MAX) const;
    void reset();

  private:
    VkDevice device;
    VkFence  fence;
};

} // namespace rl

#endif
