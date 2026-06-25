export module Rl.Client.Render.Unit.UnitRendererDrawCompute;

import Rl.Base.Game;
import Rl.Client.Render.Unit.UnitRendererInfo;
import Rl.Client.State.UnitState;
import <vulkan/vulkan.hpp>;

namespace Rl::Client::Render
{

// Dispatch compute shaders for unit rendering
export void UnitDispatchComputeShaders(Providers::UnitStateResource& resource,
    Providers::UnitStateDrawableVulkan&                       vk,
    Game::VulkanContext&                                      context);

} // namespace Rl::Client::Render
