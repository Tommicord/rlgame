#include "Rl.Base/GameVulkanSemaphore.h"
#include "Rl.Base/GameError.h"

namespace rl
{

GameVulkanSemaphore::GameVulkanSemaphore() noexcept :
    device(VK_NULL_HANDLE), semaphore(VK_NULL_HANDLE), ownsSemaphore(true)
{
}

GameVulkanSemaphore::GameVulkanSemaphore(GameVulkanSemaphore& other) :
    device(other.device), semaphore(other.semaphore), ownsSemaphore(other.ownsSemaphore)
{
        other.device        = VK_NULL_HANDLE;
        other.semaphore     = VK_NULL_HANDLE;
        other.ownsSemaphore = false;
}

GameVulkanSemaphore::GameVulkanSemaphore(VkDevice                             device,
                                         const GameVulkanSemaphoreCreateInfo& createInfo) :
    device(device), semaphore(VK_NULL_HANDLE), ownsSemaphore(true)
{
        VkSemaphoreTypeCreateInfo typeCreateInfo{};
        typeCreateInfo.sType         = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
        typeCreateInfo.semaphoreType = createInfo.semaphoreType;
        typeCreateInfo.initialValue  = createInfo.initialValue;

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphoreInfo.flags = createInfo.flags;

        if (createInfo.semaphoreType != VK_SEMAPHORE_TYPE_BINARY)
        {
                semaphoreInfo.pNext = &typeCreateInfo;
        }

        VkResult result = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore);
        if (result != VK_SUCCESS)
        {
                GameError::exitWithError("vkCreateSemaphore",
                                         "Failed to create semaphore (result = " +
                                             GameError::vulkanResultToString(result) + ")",
                                         device, VK_NULL_HANDLE, VK_NULL_HANDLE);
        }
}

GameVulkanSemaphore::GameVulkanSemaphore(GameVulkanSemaphore&& other) noexcept :
    device(other.device), semaphore(other.semaphore), ownsSemaphore(other.ownsSemaphore)
{
        other.device        = VK_NULL_HANDLE;
        other.semaphore     = VK_NULL_HANDLE;
        other.ownsSemaphore = false;
}

GameVulkanSemaphore& GameVulkanSemaphore::operator=(GameVulkanSemaphore&& other) noexcept
{
        if (this != &other)
        {
                if (semaphore != VK_NULL_HANDLE && ownsSemaphore)
                {
                        vkDestroySemaphore(device, semaphore, nullptr);
                }
                device              = other.device;
                semaphore           = other.semaphore;
                ownsSemaphore       = other.ownsSemaphore;
                other.device        = VK_NULL_HANDLE;
                other.semaphore     = VK_NULL_HANDLE;
                other.ownsSemaphore = false;
        }
        return *this;
}

GameVulkanSemaphore::~GameVulkanSemaphore()
{
        if (semaphore != VK_NULL_HANDLE && ownsSemaphore)
        {
                vkDestroySemaphore(device, semaphore, nullptr);
                semaphore = VK_NULL_HANDLE;
        }
}

VkSemaphore GameVulkanSemaphore::getSemaphore() const
{
        return semaphore;
}

void GameVulkanSemaphore::setSemaphore(VkSemaphore other)
{
        if (semaphore != VK_NULL_HANDLE && ownsSemaphore)
        {
                vkDestroySemaphore(device, semaphore, nullptr);
        }
        semaphore     = other;
        ownsSemaphore = true;
}

void GameVulkanSemaphore::setSemaphoreNonOwning(VkSemaphore other)
{
        if (semaphore != VK_NULL_HANDLE && ownsSemaphore)
        {
                vkDestroySemaphore(device, semaphore, nullptr);
        }
        semaphore     = other;
        ownsSemaphore = false;
}

} // namespace rl
