#include "Rl.Base/GameVulkanImageView.h"
#include "Rl.Base/GameError.h"

namespace rl
{

GameVulkanImageView::GameVulkanImageView() noexcept :
    device(VK_NULL_HANDLE), imageView(VK_NULL_HANDLE), ownsImageView(true)
{
}

GameVulkanImageView::GameVulkanImageView(GameVulkanImageView& other) :
    device(other.device), imageView(other.imageView), ownsImageView(other.ownsImageView)
{
        other.device        = VK_NULL_HANDLE;
        other.imageView     = VK_NULL_HANDLE;
        other.ownsImageView = false;
}

GameVulkanImageView::GameVulkanImageView(VkDevice                             device,
                                         const GameVulkanImageViewCreateInfo& createInfo) :
    device(device), imageView(VK_NULL_HANDLE), ownsImageView(true)
{
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.flags            = createInfo.flags;
        viewInfo.image            = createInfo.image;
        viewInfo.viewType         = createInfo.viewType;
        viewInfo.format           = createInfo.format;
        viewInfo.components       = createInfo.components;
        viewInfo.subresourceRange = createInfo.subresourceRange;

        VkResult result = vkCreateImageView(device, &viewInfo, nullptr, &imageView);
        if (result != VK_SUCCESS)
        {
                GameError::exitWithError("vkCreateImageView",
                                         "Failed to create image view (result = " +
                                             GameError::vulkanResultToString(result) + ")",
                                         device, VK_NULL_HANDLE, VK_NULL_HANDLE);
        }
}

GameVulkanImageView::GameVulkanImageView(GameVulkanImageView&& other) noexcept :
    device(other.device), imageView(other.imageView), ownsImageView(other.ownsImageView)
{
        other.device        = VK_NULL_HANDLE;
        other.imageView     = VK_NULL_HANDLE;
        other.ownsImageView = false;
}

GameVulkanImageView& GameVulkanImageView::operator=(GameVulkanImageView&& other) noexcept
{
        if (this != &other)
        {
                if (imageView != VK_NULL_HANDLE && ownsImageView)
                {
                        vkDestroyImageView(device, imageView, nullptr);
                }
                device              = other.device;
                imageView           = other.imageView;
                ownsImageView       = other.ownsImageView;
                other.device        = VK_NULL_HANDLE;
                other.imageView     = VK_NULL_HANDLE;
                other.ownsImageView = false;
        }
        return *this;
}

GameVulkanImageView::~GameVulkanImageView()
{
        if (imageView != VK_NULL_HANDLE && ownsImageView)
        {
                vkDestroyImageView(device, imageView, nullptr);
                imageView = VK_NULL_HANDLE;
        }
}

VkImageView GameVulkanImageView::getImageView() const
{
        return imageView;
}

void GameVulkanImageView::setImageView(VkImageView other)
{
        if (imageView != VK_NULL_HANDLE && ownsImageView)
        {
                vkDestroyImageView(device, imageView, nullptr);
        }
        imageView     = other;
        ownsImageView = true;
}

void GameVulkanImageView::setImageViewNonOwning(VkImageView other)
{
        if (imageView != VK_NULL_HANDLE && ownsImageView)
        {
                vkDestroyImageView(device, imageView, nullptr);
        }
        imageView     = other;
        ownsImageView = false;
}

} // namespace rl
