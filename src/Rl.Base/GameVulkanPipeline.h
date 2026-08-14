#ifndef RL_BASE_GAME_VULKAN_PIPELINE_H
#define RL_BASE_GAME_VULKAN_PIPELINE_H

#include <vulkan/vulkan.hpp>

namespace rl
{

/** @brief Create info for GameVulkanPipeline */
struct GameVulkanPipelineCreateInfo
{
    const VkGraphicsPipelineCreateInfo* pGraphicsCreateInfo = nullptr;
    const VkComputePipelineCreateInfo*  pComputeCreateInfo  = nullptr;
    VkPipelineCache                     pipelineCache       = VK_NULL_HANDLE;
};

/** @brief RAII wrapper for VkPipeline */
class GameVulkanPipeline
{
  public:
    /** @brief Default constructor */
    GameVulkanPipeline() noexcept;
    /** @brief Copy constructor (deleted) */
    GameVulkanPipeline(const GameVulkanPipeline& other) = delete;
    /** @brief Move constructor */
    GameVulkanPipeline(GameVulkanPipeline&& other) noexcept;
    /** @brief Constructs with Vulkan device and create info
     * @param device Vulkan device
     * @param createInfo Create info for pipeline */
    GameVulkanPipeline(VkDevice device, const GameVulkanPipelineCreateInfo& createInfo);
    /** @brief Destructor */
    ~GameVulkanPipeline();

    /** @brief Move assignment operator */
    GameVulkanPipeline& operator=(GameVulkanPipeline&& other) noexcept;

    /** @brief Returns the pipeline handle
     * @return VkPipeline handle */
    VkPipeline getPipeline() const;

    /** @brief Sets the pipeline (takes ownership)
     * @param other Pipeline to take ownership of */
    void setPipeline(VkPipeline other);

    /** @brief Sets the pipeline (non-owning)
     * @param other Pipeline to reference */
    void setPipelineNonOwning(VkPipeline other);

  private:
    VkDevice   device;
    VkPipeline pipeline;
    bool       ownsPipeline;
};

} // namespace rl

#endif
