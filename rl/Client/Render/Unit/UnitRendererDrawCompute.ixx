export module Rl.Client.Render.Unit.UnitRendererDrawCompute;

import Rl.Base.Game;
import Rl.Base.Binding;
import Rl.Client.Render.Unit.UnitRendererInfo;
import Rl.Client.State.UnitState;
import <vulkan/vulkan.hpp>;

namespace Rl::Client::Render
{

export void UnitDispatchComputeShaders(Providers::UnitStateResource& resource,
    Providers::UnitStateBinding&                                     vk,
    Game::MainBinding&                                               context);

} // namespace Rl::Client::Render
