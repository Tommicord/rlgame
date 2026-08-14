#include "Rl.Base/GameVulkanPipelineLayout.h"
#include "Rl.Base/GameError.h"

namespace rl
{

GameVulkanPipelineLayout::GameVulkanPipelineLayout() noexcept :
    device(VK_NULL_HANDLE), pipelineLayout(VK_NULL_HANDLE), ownsPipelineLayout(true)
{
}

GameVulkanPipelineLayout::GameVulkanPipelineLayout(GameVulkanPipelineLayout&& other) noexcept :
    device(other.device), pipelineLayout(other.pipelineLayout),
    ownsPipelineLayout(other.ownsPipelineLayout)
{
        other.device             = VK_NULL_HANDLE;
        other.pipelineLayout     = VK_NULL_HANDLE;
        other.ownsPipelineLayout = false;
}

GameVulkanPipelineLayout::GameVulkanPipelineLayout(
    VkDevice device, const GameVulkanPipelineLayoutCreateInfo& createInfo) :
    device(device), pipelineLayout(VK_NULL_HANDLE), ownsPipelineLayout(true)
{
        VkResult result =
            vkCreatePipelineLayout(device, createInfo.pCreateInfo, nullptr, &pipelineLayout);
        if (result != VK_SUCCESS)
        {
                GameError::exitWithError("vkCreatePipelineLayout",
                                         "Failed to create pipeline layout (result = " +
                                             GameError::vulkanResultToString(result) + ")",
                                         device, VK_NULL_HANDLE, VK_NULL_HANDLE);
        }
}

GameVulkanPipelineLayout::~GameVulkanPipelineLayout()
{
        if (pipelineLayout != VK_NULL_HANDLE && ownsPipelineLayout)
        {
                vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
                pipelineLayout = VK_NULL_HANDLE;
        }
}

GameVulkanPipelineLayout&
GameVulkanPipelineLayout::operator=(GameVulkanPipelineLayout&& other) noexcept
{
        if (this != &other)
        {
                if (pipelineLayout != VK_NULL_HANDLE && ownsPipelineLayout)
                {
                        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
                }
                device                   = other.device;
                pipelineLayout           = other.pipelineLayout;
                ownsPipelineLayout       = other.ownsPipelineLayout;
                other.device             = VK_NULL_HANDLE;
                other.pipelineLayout     = VK_NULL_HANDLE;
                other.ownsPipelineLayout = false;
        }
        return *this;
}

VkPipelineLayout GameVulkanPipelineLayout::getPipelineLayout() const
{
        return pipelineLayout;
}

void GameVulkanPipelineLayout::setPipelineLayout(VkPipelineLayout other)
{
        if (pipelineLayout != VK_NULL_HANDLE && ownsPipelineLayout)
        {
                vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        }
        pipelineLayout     = other;
        ownsPipelineLayout = true;
}

void GameVulkanPipelineLayout::setPipelineLayoutNonOwning(VkPipelineLayout other)
{
        if (pipelineLayout != VK_NULL_HANDLE && ownsPipelineLayout)
        {
                vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        }
        pipelineLayout     = other;
        ownsPipelineLayout = false;
}

} // namespace rl
