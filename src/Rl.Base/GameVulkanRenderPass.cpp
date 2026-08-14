#include "Rl.Base/GameVulkanRenderPass.h"
#include "Rl.Base/GameError.h"

namespace rl
{

GameVulkanRenderPass::GameVulkanRenderPass() noexcept :
    device(VK_NULL_HANDLE), renderPass(VK_NULL_HANDLE), ownsRenderPass(true)
{
}

GameVulkanRenderPass::GameVulkanRenderPass(GameVulkanRenderPass&& other) noexcept :
    device(other.device), renderPass(other.renderPass), ownsRenderPass(other.ownsRenderPass)
{
        other.device         = VK_NULL_HANDLE;
        other.renderPass     = VK_NULL_HANDLE;
        other.ownsRenderPass = false;
}

GameVulkanRenderPass::GameVulkanRenderPass(VkDevice                              device,
                                           const GameVulkanRenderPassCreateInfo& createInfo) :
    device(device), renderPass(VK_NULL_HANDLE), ownsRenderPass(true)
{
        VkResult result = vkCreateRenderPass(device, createInfo.pCreateInfo, nullptr, &renderPass);
        if (result != VK_SUCCESS)
        {
                GameError::exitWithError("vkCreateRenderPass",
                                         "Failed to create render pass (result = " +
                                             GameError::vulkanResultToString(result) + ")",
                                         device, VK_NULL_HANDLE, VK_NULL_HANDLE);
        }
}

GameVulkanRenderPass::~GameVulkanRenderPass()
{
        if (renderPass != VK_NULL_HANDLE && ownsRenderPass)
        {
                vkDestroyRenderPass(device, renderPass, nullptr);
                renderPass = VK_NULL_HANDLE;
        }
}

GameVulkanRenderPass& GameVulkanRenderPass::operator=(GameVulkanRenderPass&& other) noexcept
{
        if (this != &other)
        {
                if (renderPass != VK_NULL_HANDLE && ownsRenderPass)
                {
                        vkDestroyRenderPass(device, renderPass, nullptr);
                }
                device               = other.device;
                renderPass           = other.renderPass;
                ownsRenderPass       = other.ownsRenderPass;
                other.device         = VK_NULL_HANDLE;
                other.renderPass     = VK_NULL_HANDLE;
                other.ownsRenderPass = false;
        }
        return *this;
}

VkRenderPass GameVulkanRenderPass::getRenderPass() const
{
        return renderPass;
}

void GameVulkanRenderPass::setRenderPass(VkRenderPass other)
{
        if (renderPass != VK_NULL_HANDLE && ownsRenderPass)
        {
                vkDestroyRenderPass(device, renderPass, nullptr);
        }
        renderPass     = other;
        ownsRenderPass = true;
}

void GameVulkanRenderPass::setRenderPassNonOwning(VkRenderPass other)
{
        if (renderPass != VK_NULL_HANDLE && ownsRenderPass)
        {
                vkDestroyRenderPass(device, renderPass, nullptr);
        }
        renderPass     = other;
        ownsRenderPass = false;
}

} // namespace rl
