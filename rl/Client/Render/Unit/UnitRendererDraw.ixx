export module Rl.Client.Render.Unit.UnitRendererDraw;

import Rl.Base.Game;
import Rl.Base.Binding;
import Rl.Client.Render.Unit.UnitRendererInfo;
import Rl.Client.State.UnitState;
import <vulkan/vulkan.hpp>;

namespace Rl::Client::Render
{

export void UnitRenderShadowMap(Providers::UnitStateResource& resource,
    Providers::UnitStateBinding&                              vk,
    Main::MainBinding&                                        context);

export void UnitRender(Providers::UnitStateResource& resource,
    Providers::UnitStateBinding&                     vk,
    Main::MainBinding&                               context);

} // namespace Rl::Client::Render
