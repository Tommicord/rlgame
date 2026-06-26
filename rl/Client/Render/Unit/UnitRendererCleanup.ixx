export module Rl.Client.Render.Unit.UnitRendererCleanup;

import Rl.Base.Game;
import Rl.Base.Binding;
import Rl.Client.State.UnitState;
import Rl.Client.Render.Unit.UnitRendererInfo;

import <vulkan/vulkan.hpp>;

namespace Rl::Client::Render
{

export void UnitCleanupResources(VkDevice device, Providers::UnitStateBinding& vk);

export void UnitCleanupBuffers(VkDevice device, Providers::UnitStateBinding& vk);

export void UnitCleanupTextures(VkDevice device, Providers::UnitStateBinding& vk);

export void UnitCleanupSamplers(VkDevice device, Providers::UnitStateBinding& vk);

export void UnitCleanupPipelines(VkDevice device, Providers::UnitStateBinding& vk);

export void UnitCleanupDescriptorSets(VkDevice device, Providers::UnitStateBinding& vk);

} // namespace Rl::Client::Render
