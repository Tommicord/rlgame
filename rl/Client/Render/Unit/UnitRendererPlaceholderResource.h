#pragma once

#include "rl/Base/Game.h"

#include <vulkan/vulkan.hpp>

namespace Rl::Client::Render
{

void UnitCreatePlaceholderLightingTexture(VkDevice device,
    VkPhysicalDevice                               physicalDevice,
    VkImage&                                       texture,
    VkDeviceMemory&                                textureMemory,
    VkImageView&                                   textureView,
    VkSampler&                                     sampler);

void UnitCreatePlaceholderSettingsBuffer(VkDevice device,
    VkPhysicalDevice                              physicalDevice,
    VkBuffer&                                     buffer,
    VkDeviceMemory&                               bufferMemory);

void UnitCreatePlaceholderLightingBuffer(VkDevice device,
    VkPhysicalDevice                              physicalDevice,
    VkBuffer&                                     buffer,
    VkDeviceMemory&                               bufferMemory);

void UnitCreatePlaceholderAOTexture(VkDevice device,
    VkPhysicalDevice                         physicalDevice,
    VkImage&                                 texture,
    VkDeviceMemory&                          textureMemory,
    VkImageView&                             textureView,
    VkSampler&                               sampler);

void UnitCreateTriplanarSettingsBuffer(VkDevice device,
    VkPhysicalDevice                            physicalDevice,
    VkBuffer&                                   buffer,
    VkDeviceMemory&                             bufferMemory);

void UnitCreatePlaceholderNormalTexture(VkDevice device,
    VkPhysicalDevice                             physicalDevice,
    VkImage&                                     texture,
    VkDeviceMemory&                              textureMemory,
    VkImageView&                                 textureView,
    VkSampler&                                   sampler);

} // namespace Rl::Client::Render
