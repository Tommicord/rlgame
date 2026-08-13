#ifndef RL_BASE_GAME_VULKAN_DESCRIPTOR_POOL_H
#define RL_BASE_GAME_VULKAN_DESCRIPTOR_POOL_H

#include <vulkan/vulkan.hpp>

namespace rl
{

/** @brief Create info for GameVulkanDescriptorPool */
struct GameVulkanDescriptorPoolCreateInfo
{
                const VkDescriptorPoolCreateInfo* pCreateInfo = nullptr;
};

/** @brief RAII wrapper for VkDescriptorPool */
class GameVulkanDescriptorPool
{
        public:
                /** @brief Default constructor */
                GameVulkanDescriptorPool() noexcept;
                /** @brief Copy constructor (deleted) */
                GameVulkanDescriptorPool(const GameVulkanDescriptorPool& other) = delete;
                /** @brief Move constructor */
                GameVulkanDescriptorPool(GameVulkanDescriptorPool&& other) noexcept;
                /** @brief Constructs with Vulkan device and create info
                 * @param device Vulkan device
                 * @param createInfo Create info for descriptor pool */
                GameVulkanDescriptorPool(VkDevice                                  device,
                                         const GameVulkanDescriptorPoolCreateInfo& createInfo);
                /** @brief Destructor */
                ~GameVulkanDescriptorPool();

                /** @brief Move assignment operator */
                GameVulkanDescriptorPool& operator=(GameVulkanDescriptorPool&& other) noexcept;

                /** @brief Returns the descriptor pool handle
                 * @return VkDescriptorPool handle */
                VkDescriptorPool getDescriptorPool() const;

                /** @brief Sets the descriptor pool (takes ownership)
                 * @param other Descriptor pool to take ownership of */
                void setDescriptorPool(VkDescriptorPool other);

                /** @brief Sets the descriptor pool (non-owning)
                 * @param other Descriptor pool to reference */
                void setDescriptorPoolNonOwning(VkDescriptorPool other);

        private:
                VkDevice         device;
                VkDescriptorPool descriptorPool;
                bool             ownsDescriptorPool;
};

} // namespace rl

#endif
