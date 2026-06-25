export module Rl.Client.Render.Unit.UnitRendererPlaceholderResource;

import Rl.Base.Game;
import <vulkan/vulkan.hpp>;

namespace Rl::Client::Render
{

export void UnitCreatePlaceholderLightingTexture(VkDevice device,
    VkPhysicalDevice                               physicalDevice,
    VkImage&                                       texture,
    VkDeviceMemory&                                textureMemory,
    VkImageView&                                   textureView,
    VkSampler&                                     sampler);

export void UnitCreatePlaceholderSettingsBuffer(VkDevice device,
    VkPhysicalDevice                              physicalDevice,
    VkBuffer&                                     buffer,
    VkDeviceMemory&                               bufferMemory);

export void UnitCreatePlaceholderLightingBuffer(VkDevice device,
    VkPhysicalDevice                              physicalDevice,
    VkBuffer&                                     buffer,
    VkDeviceMemory&                               bufferMemory);

export void UnitCreatePlaceholderAOTexture(VkDevice device,
    VkPhysicalDevice                         physicalDevice,
    VkImage&                                 texture,
    VkDeviceMemory&                          textureMemory,
    VkImageView&                             textureView,
    VkSampler&                               sampler);

export void UnitCreateTriplanarSettingsBuffer(VkDevice device,
    VkPhysicalDevice                            physicalDevice,
    VkBuffer&                                   buffer,
    VkDeviceMemory&                             bufferMemory);

export void UnitCreatePlaceholderNormalTexture(VkDevice device,
    VkPhysicalDevice                             physicalDevice,
    VkImage&                                     texture,
    VkDeviceMemory&                              textureMemory,
    VkImageView&                                 textureView,
    VkSampler&                                   sampler);

} // namespace Rl::Client::Render
