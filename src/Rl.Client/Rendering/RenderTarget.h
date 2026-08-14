#ifndef RL_CLIENT_RENDERING_RENDER_TARGET_H
#define RL_CLIENT_RENDERING_RENDER_TARGET_H

#include "Rl.Client/Rendering/IRenderTarget.h"
#include "Rl.Base/GameVulkanFramebuffer.h"
#include "Rl.Base/GameVulkanRenderPass.h"
#include "Rl.Base/GameVulkanImage.h"
#include "Rl.Base/GameVulkanImageView.h"
#include <atomic>
#include <memory>

namespace rl
{

/**
 * @brief Concrete implementation of IRenderTarget
 *
 * Manages Vulkan resources for offscreen rendering including:
 * - Color attachment image with device memory
 * - Image view for the color attachment
 * - Render pass with proper layout transitions
 * - Framebuffer for rendering
 *
 * Thread-safe resource management using atomic operations for ID generation.
 */
class RenderTarget : public IRenderTarget
{
  public:
    /* @brief Default constructor */
    RenderTarget() = default;
    /**
     * @brief Constructor
     * @param device Vulkan device
     * @param physicalDevice Vulkan physical device
     * @param extent Extent of the render target
     * @param format Format of the color attachment
     * @param usageFlags Additional usage flags for the image
     */
    RenderTarget(VkDevice          device,
                 VkPhysicalDevice  physicalDevice,
                 VkExtent2D        extent,
                 VkFormat          format     = VK_FORMAT_B8G8R8A8_SRGB,
                 VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                                VK_IMAGE_USAGE_SAMPLED_BIT);

    /**
     * @brief Destructor, automatically clean ups the resources
     */
    ~RenderTarget() override = default;
    RenderTarget& operator=(RenderTarget& other) noexcept;
    RenderTarget(const RenderTarget& other) noexcept = delete;
    RenderTarget& operator=(RenderTarget&& other) noexcept;
    RenderTarget(RenderTarget& other) noexcept;
    RenderTarget(RenderTarget&& other) noexcept;

    const GameVulkanFramebuffer& getFramebuffer() const override
    {
      return framebuffer;
    }
    GameVulkanFramebuffer& getFramebuffer() override
    {
      return framebuffer;
    }
    const GameVulkanRenderPass& getRenderPass() const override
    {
      return renderPass;
    }
    GameVulkanRenderPass& getRenderPass() override
    {
      return renderPass;
    }
    const GameVulkanImageView& getColorAttachmentView() const override
    {
      return imageView;
    }
    GameVulkanImageView& getColorAttachmentView() override
    {
      return imageView;
    }
    const GameVulkanImage& getColorImage() const
    {
      return colorImage;
    }
    GameVulkanImage& getColorImage()
    {
      return colorImage;
    }

    VkExtent2D getExtent() const override
    {
      return extent;
    }
    VkFormat getFormat() const override
    {
      return format;
    }

    bool isValid() const override
    {
      return framebuffer.getFramebuffer() != VK_NULL_HANDLE;
    }
    uint64_t getId() const override
    {
      return id;
    }

    /**
     * @brief Transition the color attachment layout
     * @param computeCommandBuffer Command buffer to record barrier into
     * @param oldLayout Current layout
     * @param newLayout Target layout
     * @param srcStageMask Source pipeline stage mask
     * @param dstStageMask Destination pipeline stage mask
     * @param srcAccessMask Source access mask
     * @param dstAccessMask Destination access mask
     */
    void transitionLayout(VkCommandBuffer      commandBuffer,
                          VkImageLayout        oldLayout,
                          VkImageLayout        newLayout,
                          VkPipelineStageFlags srcStageMask,
                          VkPipelineStageFlags dstStageMask,
                          VkAccessFlags        srcAccessMask,
                          VkAccessFlags        dstAccessMask) const;

  private:
    void createColorAttachment();
    void createImageView();
    void createRenderPass();
    void createFramebuffer();

    VkDevice          device;
    VkPhysicalDevice  physicalDevice;
    VkExtent2D        extent;
    VkFormat          format;
    VkImageUsageFlags usageFlags;

    GameVulkanImage       colorImage;
    GameVulkanImageView   imageView;
    GameVulkanRenderPass  renderPass;
    GameVulkanFramebuffer framebuffer;

    uint64_t                     id;
    static std::atomic<uint64_t> nextId;
};

} // namespace rl

#endif
