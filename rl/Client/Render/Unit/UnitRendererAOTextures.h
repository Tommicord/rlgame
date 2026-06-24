#pragma once

#include "rl/Base/Game.h"
#include "rl/World/Unit/Unit.h"

#include <vulkan/vulkan.hpp>

namespace Rl::Client::Render
{

void UnitGenerateAOTextures(VkDevice device, Game::VulkanContext& context,
    Providers::UnitStateDrawableVulkan& vk, const World::UnitTextureMaterial& textures);

void UnitUpdateAOTextureDescriptor(VkDevice device, VkDescriptorSet descriptorSet,
    VkImageView aoTextureView, VkSampler sampler);

} // namespace Rl::Client::Render
