#ifndef RL_BASE_GAME_VULKAN_FRAMEBUFFER_H
#define RL_BASE_GAME_VULKAN_FRAMEBUFFER_H

#include <vulkan/vulkan.hpp>

namespace rl
{

/** @brief Create info for GameVulkanFramebuffer */
struct GameVulkanFramebufferCreateInfo
{
    const VkFramebufferCreateInfo* pCreateInfo = nullptr;
};

/** @brief RAII wrapper for VkFramebuffer */
class GameVulkanFramebuffer
{
  public:
    /** @brief Default constructor */
    GameVulkanFramebuffer() noexcept;
    /** @brief Copy constructor (deleted) */
    GameVulkanFramebuffer(const GameVulkanFramebuffer& other) = delete;
    /** @brief Move constructor */
    GameVulkanFramebuffer(GameVulkanFramebuffer&& other) noexcept;
    /** @brief Constructs with Vulkan device and create info
     * @param device Vulkan device
     * @param createInfo Create info for framebuffer */
    GameVulkanFramebuffer(VkDevice device, const GameVulkanFramebufferCreateInfo& createInfo);
    /** @brief Destructor */
    ~GameVulkanFramebuffer();

    /** @brief Move assignment operator */
    GameVulkanFramebuffer& operator=(GameVulkanFramebuffer&& other) noexcept;

    /** @brief Returns the framebuffer handle
     * @return VkFramebuffer handle */
    VkFramebuffer getFramebuffer() const;

    /** @brief Sets the framebuffer (takes ownership)
     * @param other Framebuffer to take ownership of */
    void setFramebuffer(VkFramebuffer other);

    /** @brief Sets the framebuffer (non-owning)
     * @param other Framebuffer to reference */
    void setFramebufferNonOwning(VkFramebuffer other);

  private:
    VkDevice      device;
    VkFramebuffer framebuffer;
    bool          ownsFramebuffer;
};

} // namespace rl

#endif
