#include "Rl.Base/GameVulkanDescriptorSetLayout.h"
#include "Rl.Base/GameError.h"

namespace rl
{

GameVulkanDescriptorSetLayout::GameVulkanDescriptorSetLayout() noexcept :
    device(VK_NULL_HANDLE), descriptorSetLayout(VK_NULL_HANDLE), ownsDescriptorSetLayout(true)
{
}

GameVulkanDescriptorSetLayout::GameVulkanDescriptorSetLayout(
    GameVulkanDescriptorSetLayout&& other) noexcept :
    device(other.device), descriptorSetLayout(other.descriptorSetLayout),
    ownsDescriptorSetLayout(other.ownsDescriptorSetLayout)
{
        other.device                  = VK_NULL_HANDLE;
        other.descriptorSetLayout     = VK_NULL_HANDLE;
        other.ownsDescriptorSetLayout = false;
}

GameVulkanDescriptorSetLayout::GameVulkanDescriptorSetLayout(
    VkDevice device, const GameVulkanDescriptorSetLayoutCreateInfo& createInfo) :
    device(device), descriptorSetLayout(VK_NULL_HANDLE), ownsDescriptorSetLayout(true)
{
        VkResult result = vkCreateDescriptorSetLayout(device, createInfo.pCreateInfo, nullptr,
                                                      &descriptorSetLayout);
        if (result != VK_SUCCESS)
        {
                GameError::exitWithError(
                    "vkCreateDescriptorSetLayout",
                    "Failed to create descriptor set layout (result = " +
                        GameError::vulkanResultToString(result) + ")",
                    device, VK_NULL_HANDLE, VK_NULL_HANDLE);
        }
}

GameVulkanDescriptorSetLayout::~GameVulkanDescriptorSetLayout()
{
        if (descriptorSetLayout != VK_NULL_HANDLE && ownsDescriptorSetLayout)
        {
                vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
                descriptorSetLayout = VK_NULL_HANDLE;
        }
}

GameVulkanDescriptorSetLayout&
GameVulkanDescriptorSetLayout::operator=(GameVulkanDescriptorSetLayout&& other) noexcept
{
        if (this != &other)
        {
                if (descriptorSetLayout != VK_NULL_HANDLE && ownsDescriptorSetLayout)
                {
                        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
                }
                device                        = other.device;
                descriptorSetLayout           = other.descriptorSetLayout;
                ownsDescriptorSetLayout       = other.ownsDescriptorSetLayout;
                other.device                  = VK_NULL_HANDLE;
                other.descriptorSetLayout     = VK_NULL_HANDLE;
                other.ownsDescriptorSetLayout = false;
        }
        return *this;
}

VkDescriptorSetLayout GameVulkanDescriptorSetLayout::getDescriptorSetLayout() const
{
        return descriptorSetLayout;
}

void GameVulkanDescriptorSetLayout::setDescriptorSetLayout(VkDescriptorSetLayout other)
{
        if (descriptorSetLayout != VK_NULL_HANDLE && ownsDescriptorSetLayout)
        {
                vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        }
        descriptorSetLayout     = other;
        ownsDescriptorSetLayout = true;
}

void GameVulkanDescriptorSetLayout::setDescriptorSetLayoutNonOwning(VkDescriptorSetLayout other)
{
        if (descriptorSetLayout != VK_NULL_HANDLE && ownsDescriptorSetLayout)
        {
                vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        }
        descriptorSetLayout     = other;
        ownsDescriptorSetLayout = false;
}

} // namespace rl
