export module Rl.Client.Render.Unit.UnitRendererNormalTextures;

import Rl.Base.Game;
import Rl.World.Unit.Unit;
import <vulkan/vulkan.hpp>;

namespace Rl::Client::Render
{

// Generate normal textures from unit textures
export void UnitGenerateNormalTextures(VkDevice device,
    Game::VulkanContext&                 context,
    Providers::UnitStateDrawableVulkan&  vk,
    const World::UnitTextureMaterial&    textures);

// Update normal texture descriptor
export void UnitUpdateNormalTextureDescriptor(VkDevice device,
    VkDescriptorSet                             descriptorSet,
    VkImageView                                 normalTextureView[6],
    VkSampler                                   sampler);

} // namespace Rl::Client::Render
