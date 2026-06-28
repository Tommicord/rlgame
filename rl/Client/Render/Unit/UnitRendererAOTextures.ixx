export module Rl.Client.Render.Unit.UnitRendererAOTextures;

import Rl.Base.Game;
import Rl.Base.Binding;
import Rl.World.Unit;
import Rl.Client.State.UnitState;

import <vulkan/vulkan.hpp>;

namespace Rl::Client::Render
{

export void UnitGenerateAOTextures(VkDevice    device,
    Main::MainBinding&                context,
    Providers::UnitStateBinding& vk,
    const World::UnitTextureMaterial&   textures);

export void UnitUpdateAOTextureDescriptor(VkDevice device,
    VkDescriptorSet                         descriptorSet,
    VkImageView                             aoTextureView[6],
    VkSampler                               sampler);

} // namespace Rl::Client::Render
