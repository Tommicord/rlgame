export module Rl.Client.Render.Unit.UnitRendererDraw;

import Rl.Base.Game;
import Rl.Client.Render.Unit.UnitRendererInfo;
import Rl.Client.State.UnitState;
import <vulkan/vulkan.hpp>;

namespace Rl::Client::Render
{

export void UnitRenderShadowMap(Providers::UnitStateResource& resource,
    Providers::UnitStateDrawableVulkan&                vk,
    Game::VulkanContext&                               context);

export void UnitRender(Providers::UnitStateResource& resource,
    Providers::UnitStateDrawableVulkan&       vk,
    Game::VulkanContext&                      context);

} // namespace Rl::Client::Render
