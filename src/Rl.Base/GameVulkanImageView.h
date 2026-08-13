#ifndef RL_BASE_GAME_VULKAN_IMAGE_VIEW_H
#define RL_BASE_GAME_VULKAN_IMAGE_VIEW_H

#include <vulkan/vulkan.hpp>

namespace rl
{

/** @brief Create info for image view creation */
struct GameVulkanImageViewCreateInfo
{
                VkImageViewCreateFlags flags      = 0;
                VkImage                image      = VK_NULL_HANDLE;
                VkImageViewType        viewType   = VK_IMAGE_VIEW_TYPE_2D;
                VkFormat               format     = VK_FORMAT_R8G8B8A8_UNORM;
                VkComponentMapping     components = {
                    VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                    VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
                VkImageSubresourceRange subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
};

/** @brief RAII wrapper for Vulkan image view objects */
class GameVulkanImageView
{
        public:
                /**
                 * @brief Constructs an image view (VK_NULL_HANDLE by default)
                 */
                GameVulkanImageView() noexcept;

                /**
                 * @brief Constructs an image view by createInfo
                 * @param device Vulkan device
                 * @param createInfo Image view creation info
                 */
                GameVulkanImageView(VkDevice                             device,
                                    const GameVulkanImageViewCreateInfo& createInfo);

                /** @brief Destroys the image view */
                ~GameVulkanImageView();

                GameVulkanImageView(GameVulkanImageView& other);
                GameVulkanImageView(const GameVulkanImageView& other) = delete;
                GameVulkanImageView(GameVulkanImageView&& other) noexcept;
                GameVulkanImageView& operator=(const GameVulkanImageView& other) = delete;
                GameVulkanImageView& operator=(GameVulkanImageView&& other) noexcept;

                /** @brief Returns the image view handle
                 * @return Vulkan image view handle */
                VkImageView getImageView() const;

                /** @brief Sets the image view to the current state (takes ownership)
                 * @param other Vulkan image view handle to take ownership of */
                void setImageView(VkImageView other);

                /** @brief Sets the image view without taking ownership (non-owning reference)
                 * @param other Vulkan image view handle to reference without ownership */
                void setImageViewNonOwning(VkImageView other);

        private:
                VkDevice    device        = VK_NULL_HANDLE;
                VkImageView imageView     = VK_NULL_HANDLE;
                bool        ownsImageView = true;
};

} // namespace rl

#endif // RL_BASE_GAME_VULKAN_IMAGE_VIEW_H
