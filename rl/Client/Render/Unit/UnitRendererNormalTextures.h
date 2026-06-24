#pragma once

#include <vulkan/vulkan.hpp>
#include "rl/Base/Game.h"
#include "rl/World/Unit/Unit.h"

namespace Rl::Client::Render
{

// Generate normal textures from unit textures
void UnitGenerateNormalTextures(VkDevice device,
    Game::VulkanContext&                 context,
    Providers::UnitStateDrawableVulkan&  vk,
    const World::UnitTextureMaterial&    textures);

// Update normal texture descriptor
void UnitUpdateNormalTextureDescriptor(VkDevice device,
    VkDescriptorSet                             descriptorSet,
    VkImageView                                 normalTextureView[6],
    VkSampler                                   sampler);

} // namespace Rl::Client::Render
