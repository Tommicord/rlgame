#ifndef RL_BASE_GAME_VULKAN_SAMPLER_H
#define RL_BASE_GAME_VULKAN_SAMPLER_H

#include <vulkan/vulkan.hpp>

namespace rl
{

/** @brief Create info for sampler creation */
struct GameVulkanSamplerCreateInfo
{
    VkFilter             magFilter               = VK_FILTER_LINEAR;
    VkFilter             minFilter               = VK_FILTER_LINEAR;
    VkSamplerMipmapMode  mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    VkSamplerAddressMode addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkSamplerAddressMode addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkSamplerAddressMode addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    float                mipLodBias              = 0.0f;
    VkBool32             anisotropyEnable        = VK_FALSE;
    float                maxAnisotropy           = 1.0f;
    VkBool32             compareEnable           = VK_FALSE;
    VkCompareOp          compareOp               = VK_COMPARE_OP_ALWAYS;
    float                minLod                  = 0.0f;
    float                maxLod                  = 0.0f;
    VkBorderColor        borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    VkBool32             unnormalizedCoordinates = VK_FALSE;
};

/** @brief RAII wrapper for Vulkan sampler objects */
class GameVulkanSampler
{
  public:
    /**
     * @brief Constructs a sampler (VK_NULL_HANDLE by default)
     */
    GameVulkanSampler() noexcept;

    /**
     * @brief Constructs a sampler by createInfo
     * @param device Vulkan device
     * @param createInfo Sampler creation info
     */
    GameVulkanSampler(VkDevice device, const GameVulkanSamplerCreateInfo& createInfo);

    /** @brief Destroys the sampler */
    ~GameVulkanSampler();

    GameVulkanSampler(GameVulkanSampler& other);
    GameVulkanSampler(const GameVulkanSampler& other) = delete;
    GameVulkanSampler(GameVulkanSampler&& other) noexcept;
    GameVulkanSampler& operator=(const GameVulkanSampler& other) = delete;
    GameVulkanSampler& operator=(GameVulkanSampler&& other) noexcept;

    /**
     * @brief Returns the sampler handle
     * @return Vulkan sampler handle
     */
    VkSampler getSampler() const;

    /**
     * @brief Sets the sampler to the current state (takes ownership)
     * @param other Vulkan sampler handle to take ownership of
     */
    void setSampler(VkSampler other);

    /**
     * @brief Sets the sampler without taking ownership (non-owning reference)
     * @param other Vulkan sampler handle to reference without ownership
     */
    void setSamplerNonOwning(VkSampler other);

  private:
    VkDevice  device      = VK_NULL_HANDLE;
    VkSampler sampler     = VK_NULL_HANDLE;
    bool      ownsSampler = true;
};

} // namespace rl

#endif // RL_BASE_GAME_VULKAN_SAMPLER_H
