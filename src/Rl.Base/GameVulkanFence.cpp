#include "Rl.Base/GameVulkanFence.h"
#include "Rl.Base/GameError.h"

#include <vulkan/vulkan.hpp>

namespace rl
{

GameVulkanFence::GameVulkanFence() noexcept : device(VK_NULL_HANDLE), fence(VK_NULL_HANDLE)
{
}

GameVulkanFence::GameVulkanFence(VkDevice device, const GameVulkanFenceCreateInfo& createInfo) :
    device(device), fence(VK_NULL_HANDLE)
{
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = createInfo.flags;

        VkResult result = vkCreateFence(device, &fenceInfo, nullptr, &fence);
        if (result != VK_SUCCESS)
        {
                GameError::exitWithError("vkCreateFence",
                                         "Failed to create fence (result = " +
                                             GameError::vulkanResultToString(result) + ")",
                                         device, VK_NULL_HANDLE, VK_NULL_HANDLE);
        }
}

GameVulkanFence::GameVulkanFence(GameVulkanFence&& other) noexcept :
    device(other.device), fence(other.fence)
{
        other.device = VK_NULL_HANDLE;
        other.fence  = VK_NULL_HANDLE;
}

GameVulkanFence::~GameVulkanFence()
{
        if (fence != VK_NULL_HANDLE)
        {
                vkDestroyFence(device, fence, nullptr);
                fence = VK_NULL_HANDLE;
        }
}

VkFence GameVulkanFence::getFence() const
{
        return fence;
}

VkDevice GameVulkanFence::getDevice() const
{
        return device;
}

void GameVulkanFence::wait(uint64_t timeout) const
{
        VkResult result = vkWaitForFences(device, 1, &fence, VK_TRUE, timeout);
        if (result == VK_TIMEOUT)
        {
                GameError::exitWithError("Timeout waiting for fence");
        }
        else if (result != VK_SUCCESS)
        {
                GameError::exitWithError("Failed to wait for fence (result = " +
                                             GameError::vulkanResultToString(result) + ")",
                                         device, VK_NULL_HANDLE, VK_NULL_HANDLE);
        }
}

void GameVulkanFence::reset()
{
        VkResult result = vkResetFences(device, 1, &fence);
        if (result != VK_SUCCESS)
        {
                GameError::exitWithError("Failed to reset fence (result = " +
                                             GameError::vulkanResultToString(result) + ")",
                                         device, VK_NULL_HANDLE, VK_NULL_HANDLE);
        }
}

GameVulkanFence& GameVulkanFence::operator=(GameVulkanFence&& other) noexcept
{
        if (this != &other)
        {
                if (fence != VK_NULL_HANDLE)
                {
                        vkDestroyFence(device, fence, nullptr);
                }
                device       = other.device;
                fence        = other.fence;
                other.device = VK_NULL_HANDLE;
                other.fence  = VK_NULL_HANDLE;
        }
        return *this;
}

} // namespace rl
