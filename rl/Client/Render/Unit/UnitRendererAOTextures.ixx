export module Rl.Client.Render.Unit.UnitRendererAOTextures;

import Rl.Base.Game;
import Rl.World.Unit.Unit;
import <vulkan/vulkan.hpp>;

namespace Rl::Client::Render
{

export void UnitGenerateAOTextures(VkDevice    device,
    Game::VulkanContext&                context,
    Providers::UnitStateDrawableVulkan& vk,
    const World::UnitTextureMaterial&   textures);

export void UnitUpdateAOTextureDescriptor(VkDevice device,
    VkDescriptorSet                         descriptorSet,
    VkImageView                             aoTextureView[6],
    VkSampler                               sampler);

} // namespace Rl::Client::Render
