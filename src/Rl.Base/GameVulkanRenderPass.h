#ifndef RL_BASE_GAME_VULKAN_RENDER_PASS_H
#define RL_BASE_GAME_VULKAN_RENDER_PASS_H

#include <vulkan/vulkan.hpp>

namespace rl
{

/** @brief Create info for GameVulkanRenderPass */
struct GameVulkanRenderPassCreateInfo
{
    const VkRenderPassCreateInfo* pCreateInfo = nullptr;
};

/** @brief RAII wrapper for VkRenderPass */
class GameVulkanRenderPass
{
  public:
    /** @brief Default constructor */
    GameVulkanRenderPass() noexcept;
    /** @brief Copy constructor (deleted) */
    GameVulkanRenderPass(const GameVulkanRenderPass& other) = delete;
    /** @brief Move constructor */
    GameVulkanRenderPass(GameVulkanRenderPass&& other) noexcept;
    /** @brief Constructs with Vulkan device and create info
     * @param device Vulkan device
     * @param createInfo Create info for render pass */
    GameVulkanRenderPass(VkDevice device, const GameVulkanRenderPassCreateInfo& createInfo);
    /** @brief Destructor */
    ~GameVulkanRenderPass();

    /** @brief Move assignment operator */
    GameVulkanRenderPass& operator=(GameVulkanRenderPass&& other) noexcept;

    /** @brief Returns the render pass handle
     * @return VkRenderPass handle */
    VkRenderPass getRenderPass() const;

    /** @brief Sets the render pass (takes ownership)
     * @param other Render pass to take ownership of */
    void setRenderPass(VkRenderPass other);

    /** @brief Sets the render pass (non-owning)
     * @param other Render pass to reference */
    void setRenderPassNonOwning(VkRenderPass other);

  private:
    VkDevice     device;
    VkRenderPass renderPass;
    bool         ownsRenderPass;
};

} // namespace rl

#endif
