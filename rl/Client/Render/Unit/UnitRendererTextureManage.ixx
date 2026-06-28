export module Rl.Client.Render.Unit.UnitRendererTextureManage;

import Rl.Base.Game;
import Rl.Base.Binding;
import Rl.World.Unit;
import <vulkan/vulkan.hpp>;

namespace Rl::Client::Render
{

// Update graphics descriptor set with unit textures
export void UnitUpdateUnitTextures(VkDevice  device,
    VkDescriptorSet                   descriptorSet,
    const World::UnitTextureMaterial& textures,
    Main::MainBinding&              context);

} // namespace Rl::Client::Render
