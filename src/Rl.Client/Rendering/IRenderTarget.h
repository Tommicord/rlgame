#ifndef RL_CLIENT_RENDERING_I_RENDER_TARGET_H
#define RL_CLIENT_RENDERING_I_RENDER_TARGET_H

#include <vulkan/vulkan.hpp>

#include "Rl.Base/GameVulkanFramebuffer.h"
#include "Rl.Base/GameVulkanRenderPass.h"
#include "Rl.Base/GameVulkanImage.h"
#include "Rl.Base/GameVulkanImageView.h"

namespace rl
{

/**
 * @brief Interface for offscreen render targets
 *
 * This interface defines the contract for render targets that can be used
 * for offscreen rendering and subsequent compositing. Implementations manage
 * their own Vulkan resources (framebuffer, render pass, images, image views).
 */
class IRenderTarget
{
        public:
                virtual ~IRenderTarget() = default;

                /**
                 * @brief Get the framebuffer handle
                 * @return VkFramebuffer handle
                 */
                virtual const GameVulkanFramebuffer& getFramebuffer() const = 0;

                /**
                 * @brief Get the framebuffer handle
                 * @return VkFramebuffer handle
                 */
                virtual GameVulkanFramebuffer& getFramebuffer() = 0;

                /**
                 * @brief Get the render pass handle
                 * @return VkRenderPass handle
                 */
                virtual const GameVulkanRenderPass& getRenderPass() const = 0;

                /**
                 * @brief Get the render pass handle
                 * @return VkRenderPass handle
                 */
                virtual GameVulkanRenderPass& getRenderPass() = 0;

                /**
                 * @brief Get the color attachment image view for sampling
                 * @return VkImageView handle
                 */
                virtual const GameVulkanImageView& getColorAttachmentView() const = 0;

                /**
                 * @brief Get the color attachment image view for sampling
                 * @return VkImageView handle
                 */
                virtual GameVulkanImageView& getColorAttachmentView() = 0;

                /**
                 * @brief Get the underlying color image
                 * @return VkImage handle
                 */
                virtual const GameVulkanImage& getColorImage() const = 0;

                /**
                 * @brief Get the underlying color image
                 * @return VkImage handle
                 */
                virtual GameVulkanImage& getColorImage() = 0;

                /**
                 * @brief Get the extent of the render target
                 * @return VkExtent2D extent
                 */
                virtual VkExtent2D getExtent() const = 0;

                /**
                 * @brief Get the format of the color attachment
                 * @return VkFormat format
                 */
                virtual VkFormat getFormat() const = 0;

                /**
                 * @brief Check if the render target is valid and ready for use
                 * @return true if valid, false otherwise
                 */
                virtual bool isValid() const = 0;

                /**
                 * @brief Get a unique identifier for this render target
                 * @return uint64_t unique ID
                 */
                virtual uint64_t getId() const = 0;
};

} // namespace rl

#endif
