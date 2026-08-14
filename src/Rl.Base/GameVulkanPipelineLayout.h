#ifndef RL_BASE_GAME_VULKAN_PIPELINE_LAYOUT_H
#define RL_BASE_GAME_VULKAN_PIPELINE_LAYOUT_H

#include <vulkan/vulkan.hpp>

namespace rl
{

/** @brief Create info for GameVulkanPipelineLayout */
struct GameVulkanPipelineLayoutCreateInfo
{
    const VkPipelineLayoutCreateInfo* pCreateInfo = nullptr;
};

/** @brief RAII wrapper for VkPipelineLayout */
class GameVulkanPipelineLayout
{
  public:
    /** @brief Default constructor */
    GameVulkanPipelineLayout() noexcept;
    /** @brief Copy constructor (deleted) */
    GameVulkanPipelineLayout(const GameVulkanPipelineLayout& other) = delete;
    /** @brief Move constructor */
    GameVulkanPipelineLayout(GameVulkanPipelineLayout&& other) noexcept;
    /** @brief Constructs with Vulkan device and create info
     * @param device Vulkan device
     * @param createInfo Create info for pipeline layout */
    GameVulkanPipelineLayout(VkDevice device, const GameVulkanPipelineLayoutCreateInfo& createInfo);
    /** @brief Destructor */
    ~GameVulkanPipelineLayout();

    /** @brief Move assignment operator */
    GameVulkanPipelineLayout& operator=(GameVulkanPipelineLayout&& other) noexcept;

    /** @brief Returns the pipeline layout handle
     * @return VkPipelineLayout handle */
    VkPipelineLayout getPipelineLayout() const;

    /** @brief Sets the pipeline layout (takes ownership)
     * @param other Pipeline layout to take ownership of */
    void setPipelineLayout(VkPipelineLayout other);

    /** @brief Sets the pipeline layout (non-owning)
     * @param other Pipeline layout to reference */
    void setPipelineLayoutNonOwning(VkPipelineLayout other);

  private:
    VkDevice         device;
    VkPipelineLayout pipelineLayout;
    bool             ownsPipelineLayout;
};

} // namespace rl

#endif
