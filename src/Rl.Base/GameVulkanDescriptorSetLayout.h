#ifndef RL_BASE_GAME_VULKAN_DESCRIPTOR_SET_LAYOUT_H
#define RL_BASE_GAME_VULKAN_DESCRIPTOR_SET_LAYOUT_H

#include <vulkan/vulkan.hpp>

namespace rl
{

/** @brief Create info for GameVulkanDescriptorSetLayout */
struct GameVulkanDescriptorSetLayoutCreateInfo
{
    const VkDescriptorSetLayoutCreateInfo* pCreateInfo = nullptr;
};

/** @brief RAII wrapper for VkDescriptorSetLayout */
class GameVulkanDescriptorSetLayout
{
  public:
    /** @brief Default constructor */
    GameVulkanDescriptorSetLayout() noexcept;
    /** @brief Copy constructor (deleted) */
    GameVulkanDescriptorSetLayout(const GameVulkanDescriptorSetLayout& other) = delete;
    /** @brief Move constructor */
    GameVulkanDescriptorSetLayout(GameVulkanDescriptorSetLayout&& other) noexcept;
    /** @brief Constructs with Vulkan device and create info
     * @param device Vulkan device
     * @param createInfo Create info for descriptor set layout */
    GameVulkanDescriptorSetLayout(VkDevice                                       device,
                                  const GameVulkanDescriptorSetLayoutCreateInfo& createInfo);
    /** @brief Destructor */
    ~GameVulkanDescriptorSetLayout();

    /** @brief Move assignment operator */
    GameVulkanDescriptorSetLayout& operator=(GameVulkanDescriptorSetLayout&& other) noexcept;

    /** @brief Returns the descriptor set layout handle
     * @return VkDescriptorSetLayout handle */
    VkDescriptorSetLayout getDescriptorSetLayout() const;

    /** @brief Sets the descriptor set layout (takes ownership)
     * @param other Descriptor set layout to take ownership of */
    void setDescriptorSetLayout(VkDescriptorSetLayout other);

    /** @brief Sets the descriptor set layout (non-owning)
     * @param other Descriptor set layout to reference */
    void setDescriptorSetLayoutNonOwning(VkDescriptorSetLayout other);

  private:
    VkDevice              device;
    VkDescriptorSetLayout descriptorSetLayout;
    bool                  ownsDescriptorSetLayout;
};

} // namespace rl

#endif
