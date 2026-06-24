#pragma once

#include "rl/Base/Game.h"

#include <vulkan/vulkan.hpp>

namespace Rl::Client::Render
{

/* Create global texture sampler for all textures */
void UnitCreateGlobalTextureSampler(VkDevice device, VkSampler& sampler);

/* Creates placeholder lighting texture sampler */
void UnitCreateLightingSampler(VkDevice device, VkSampler& sampler);

/* Creates AO sampler */
void UnitCreateAOSampler(VkDevice device, VkSampler& sampler);

} // namespace Rl::Client::Render
