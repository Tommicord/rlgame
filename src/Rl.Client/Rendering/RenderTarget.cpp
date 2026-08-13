#include "Rl.Client/Rendering/RenderTarget.h"
#include "Rl.Base/GameVulkanAllocator.h"
#include <stdexcept>

namespace rl
{

std::atomic<uint64_t> RenderTarget::nextId{1};

RenderTarget::RenderTarget(VkDevice          device,
                           VkPhysicalDevice  physicalDevice,
                           VkExtent2D        extent,
                           VkFormat          format,
                           VkImageUsageFlags usageFlags) :
    device(device), physicalDevice(physicalDevice), extent(extent), format(format),
    usageFlags(usageFlags), id(nextId.fetch_add(1))
{
        createColorAttachment();
        createImageView();
        createRenderPass();
        createFramebuffer();
}

RenderTarget& RenderTarget::operator=(RenderTarget& other) noexcept
{
        if (this != &other)
        {
                device         = other.device;
                physicalDevice = other.physicalDevice;
                extent         = other.extent;
                format         = other.format;
                usageFlags     = other.usageFlags;
                id             = other.id;

                colorImage  = std::move(other.colorImage);
                imageView   = std::move(other.imageView);
                renderPass  = std::move(other.renderPass);
                framebuffer = std::move(other.framebuffer);
        }
        return *this;
}

RenderTarget& RenderTarget::operator=(RenderTarget&& other) noexcept
{
        if (this != &other)
        {
                device         = other.device;
                physicalDevice = other.physicalDevice;
                extent         = other.extent;
                format         = other.format;
                usageFlags     = other.usageFlags;
                id             = other.id;

                colorImage  = std::move(other.colorImage);
                imageView   = std::move(other.imageView);
                renderPass  = std::move(other.renderPass);
                framebuffer = std::move(other.framebuffer);
        }
        return *this;
}

RenderTarget::RenderTarget(RenderTarget& other) noexcept :
    device(other.device), physicalDevice(other.physicalDevice), extent(other.extent),
    format(other.format), usageFlags(other.usageFlags), id(other.id),
    colorImage(std::move(other.colorImage)), imageView(std::move(other.imageView)),
    renderPass(std::move(other.renderPass)), framebuffer(std::move(other.framebuffer))
{
}

RenderTarget::RenderTarget(RenderTarget&& other) noexcept :
    device(other.device), physicalDevice(other.physicalDevice), extent(other.extent),
    format(other.format), usageFlags(other.usageFlags), id(other.id),
    colorImage(std::move(other.colorImage)), imageView(std::move(other.imageView)),
    renderPass(std::move(other.renderPass)), framebuffer(std::move(other.framebuffer))
{
}

void RenderTarget::createColorAttachment()
{
        GameVulkanImageCreateInfo imageInfo{};
        imageInfo.imageType        = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width     = extent.width;
        imageInfo.extent.height    = extent.height;
        imageInfo.extent.depth     = 1;
        imageInfo.mipLevels        = 1;
        imageInfo.arrayLayers      = 1;
        imageInfo.format           = format;
        imageInfo.tiling           = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout    = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage            = usageFlags;
        imageInfo.samples          = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        colorImage = GameVulkanImage(device, physicalDevice, imageInfo);
}

void RenderTarget::createImageView()
{
        GameVulkanImageViewCreateInfo viewInfo{};
        viewInfo.image                           = colorImage.getImage();
        viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format                          = format;
        viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel   = 0;
        viewInfo.subresourceRange.levelCount     = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount     = 1;

        imageView = GameVulkanImageView(device, viewInfo);
}

void RenderTarget::createRenderPass()
{
        VkAttachmentDescription colorAttachmentDesc{};
        colorAttachmentDesc.format         = format;
        colorAttachmentDesc.samples        = VK_SAMPLE_COUNT_1_BIT;
        colorAttachmentDesc.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachmentDesc.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentDesc.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentDesc.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentDesc.finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments    = &colorAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass    = 0;
        dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments    = &colorAttachmentDesc;
        renderPassInfo.subpassCount    = 1;
        renderPassInfo.pSubpasses      = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies   = &dependency;

        GameVulkanRenderPassCreateInfo rpCreateInfo{};
        rpCreateInfo.pCreateInfo = &renderPassInfo;
        renderPass               = GameVulkanRenderPass(device, rpCreateInfo);
}

void RenderTarget::createFramebuffer()
{
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass      = renderPass.getRenderPass();
        framebufferInfo.attachmentCount = 1;
        VkImageView attachments[]       = {imageView.getImageView()};
        framebufferInfo.pAttachments    = attachments;
        framebufferInfo.width           = extent.width;
        framebufferInfo.height          = extent.height;
        framebufferInfo.layers          = 1;

        GameVulkanFramebufferCreateInfo fbCreateInfo{};
        fbCreateInfo.pCreateInfo = &framebufferInfo;
        framebuffer              = GameVulkanFramebuffer(device, fbCreateInfo);
}

void RenderTarget::transitionLayout(VkCommandBuffer      commandBuffer,
                                    VkImageLayout        oldLayout,
                                    VkImageLayout        newLayout,
                                    VkPipelineStageFlags srcStageMask,
                                    VkPipelineStageFlags dstStageMask,
                                    VkAccessFlags        srcAccessMask,
                                    VkAccessFlags        dstAccessMask) const
{
        VkImageMemoryBarrier barrier{};
        barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout                       = oldLayout;
        barrier.newLayout                       = newLayout;
        barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.image                           = colorImage.getImage();
        barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel   = 0;
        barrier.subresourceRange.levelCount     = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount     = 1;
        barrier.srcAccessMask                   = srcAccessMask;
        barrier.dstAccessMask                   = dstAccessMask;

        vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr,
                             1, &barrier);
}

} // namespace rl
