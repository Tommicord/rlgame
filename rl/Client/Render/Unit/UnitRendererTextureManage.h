#pragma once

#include "rl/Base/Game.h"
#include "rl/World/Unit/Unit.h"
#include <vulkan/vulkan.hpp>

namespace Rl::Client::Render
{

// Update graphics descriptor set with unit textures
void UnitUpdateUnitTextures(VkDevice device, VkDescriptorSet descriptorSet,
    const World::UnitTextureMaterial& textures, Game::VulkanContext& context);

} // namespace Rl::Client::Render
