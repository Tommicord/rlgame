#include "Rl.Base/GameVulkanSampler.h"
#include "Rl.Base/GameError.h"

namespace rl
{

GameVulkanSampler::GameVulkanSampler() noexcept :
    device(VK_NULL_HANDLE), sampler(VK_NULL_HANDLE), ownsSampler(true)
{
}

GameVulkanSampler::GameVulkanSampler(GameVulkanSampler& other) :
    device(other.device), sampler(other.sampler), ownsSampler(other.ownsSampler)
{
        other.device      = VK_NULL_HANDLE;
        other.sampler     = VK_NULL_HANDLE;
        other.ownsSampler = false;
}

GameVulkanSampler::GameVulkanSampler(VkDevice                           device,
                                     const GameVulkanSamplerCreateInfo& createInfo) :
    device(device), sampler(VK_NULL_HANDLE), ownsSampler(true)
{
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter               = createInfo.magFilter;
        samplerInfo.minFilter               = createInfo.minFilter;
        samplerInfo.mipmapMode              = createInfo.mipmapMode;
        samplerInfo.addressModeU            = createInfo.addressModeU;
        samplerInfo.addressModeV            = createInfo.addressModeV;
        samplerInfo.addressModeW            = createInfo.addressModeW;
        samplerInfo.mipLodBias              = createInfo.mipLodBias;
        samplerInfo.anisotropyEnable        = createInfo.anisotropyEnable;
        samplerInfo.maxAnisotropy           = createInfo.maxAnisotropy;
        samplerInfo.compareEnable           = createInfo.compareEnable;
        samplerInfo.compareOp               = createInfo.compareOp;
        samplerInfo.minLod                  = createInfo.minLod;
        samplerInfo.maxLod                  = createInfo.maxLod;
        samplerInfo.borderColor             = createInfo.borderColor;
        samplerInfo.unnormalizedCoordinates = createInfo.unnormalizedCoordinates;

        VkResult result = vkCreateSampler(device, &samplerInfo, nullptr, &sampler);
        if (result != VK_SUCCESS)
        {
                GameError::exitWithError(
                    "vkCreateSampler",
                    "Failed to create sampler (result = " +
                        GameError::vulkanResultToString(result) + ")",
                    device, VK_NULL_HANDLE, VK_NULL_HANDLE);
        }
}

GameVulkanSampler::GameVulkanSampler(GameVulkanSampler&& other) noexcept :
    device(other.device), sampler(other.sampler), ownsSampler(other.ownsSampler)
{
        other.device      = VK_NULL_HANDLE;
        other.sampler     = VK_NULL_HANDLE;
        other.ownsSampler = false;
}

GameVulkanSampler& GameVulkanSampler::operator=(GameVulkanSampler&& other) noexcept
{
        if (this != &other)
        {
                if (sampler != VK_NULL_HANDLE && ownsSampler)
                {
                        vkDestroySampler(device, sampler, nullptr);
                }
                device            = other.device;
                sampler           = other.sampler;
                ownsSampler       = other.ownsSampler;
                other.device      = VK_NULL_HANDLE;
                other.sampler     = VK_NULL_HANDLE;
                other.ownsSampler = false;
        }
        return *this;
}

GameVulkanSampler::~GameVulkanSampler()
{
        if (sampler != VK_NULL_HANDLE && ownsSampler)
        {
                vkDestroySampler(device, sampler, nullptr);
                sampler = VK_NULL_HANDLE;
        }
}

VkSampler GameVulkanSampler::getSampler() const
{
        return sampler;
}

void GameVulkanSampler::setSampler(VkSampler other)
{
        if (sampler != VK_NULL_HANDLE && ownsSampler)
        {
                vkDestroySampler(device, sampler, nullptr);
        }
        sampler     = other;
        ownsSampler = true;
}

void GameVulkanSampler::setSamplerNonOwning(VkSampler other)
{
        if (sampler != VK_NULL_HANDLE && ownsSampler)
        {
                vkDestroySampler(device, sampler, nullptr);
        }
        sampler     = other;
        ownsSampler = false;
}

} // namespace rl
