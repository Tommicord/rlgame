#include "Rl.Base/GameVulkanFramebuffer.h"
#include "Rl.Base/GameError.h"

namespace rl
{

GameVulkanFramebuffer::GameVulkanFramebuffer() noexcept :
    device(VK_NULL_HANDLE), framebuffer(VK_NULL_HANDLE), ownsFramebuffer(true)
{
}

GameVulkanFramebuffer::GameVulkanFramebuffer(GameVulkanFramebuffer&& other) noexcept :
    device(other.device), framebuffer(other.framebuffer), ownsFramebuffer(other.ownsFramebuffer)
{
        other.device          = VK_NULL_HANDLE;
        other.framebuffer     = VK_NULL_HANDLE;
        other.ownsFramebuffer = false;
}

GameVulkanFramebuffer::GameVulkanFramebuffer(VkDevice                               device,
                                             const GameVulkanFramebufferCreateInfo& createInfo) :
    device(device), framebuffer(VK_NULL_HANDLE), ownsFramebuffer(true)
{
        VkResult result =
            vkCreateFramebuffer(device, createInfo.pCreateInfo, nullptr, &framebuffer);
        if (result != VK_SUCCESS)
        {
                GameError::exitWithError(
                    "vkCreateFramebuffer",
                    "Failed to create framebuffer (result = " +
                        GameError::vulkanResultToString(result) + ")",
                    device, VK_NULL_HANDLE, VK_NULL_HANDLE);
        }
}

GameVulkanFramebuffer::~GameVulkanFramebuffer()
{
        if (framebuffer != VK_NULL_HANDLE && ownsFramebuffer)
        {
                vkDestroyFramebuffer(device, framebuffer, nullptr);
                framebuffer = VK_NULL_HANDLE;
        }
}

GameVulkanFramebuffer& GameVulkanFramebuffer::operator=(GameVulkanFramebuffer&& other) noexcept
{
        if (this != &other)
        {
                if (framebuffer != VK_NULL_HANDLE && ownsFramebuffer)
                {
                        vkDestroyFramebuffer(device, framebuffer, nullptr);
                }
                device                = other.device;
                framebuffer           = other.framebuffer;
                ownsFramebuffer       = other.ownsFramebuffer;
                other.device          = VK_NULL_HANDLE;
                other.framebuffer     = VK_NULL_HANDLE;
                other.ownsFramebuffer = false;
        }
        return *this;
}

VkFramebuffer GameVulkanFramebuffer::getFramebuffer() const
{
        return framebuffer;
}

void GameVulkanFramebuffer::setFramebuffer(VkFramebuffer other)
{
        if (framebuffer != VK_NULL_HANDLE && ownsFramebuffer)
        {
                vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
        framebuffer     = other;
        ownsFramebuffer = true;
}

void GameVulkanFramebuffer::setFramebufferNonOwning(VkFramebuffer other)
{
        if (framebuffer != VK_NULL_HANDLE && ownsFramebuffer)
        {
                vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
        framebuffer     = other;
        ownsFramebuffer = false;
}

} // namespace rl
