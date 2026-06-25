export module Rl.Client.Render.Unit.UnitRendererSampler;

import Rl.Base.Game;
import <vulkan/vulkan.hpp>;

namespace Rl::Client::Render
{

/* Create global texture sampler for all textures */
export void UnitCreateGlobalTextureSampler(VkDevice device, VkSampler& sampler);

/* Creates placeholder lighting texture sampler */
export void UnitCreateLightingSampler(VkDevice device, VkSampler& sampler);

/* Creates AO sampler */
export void UnitCreateAOSampler(VkDevice device, VkSampler& sampler);

} // namespace Rl::Client::Render
