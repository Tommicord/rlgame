export module Rl.Client.Render.Unit.UnitRendererCleanup;

import Rl.Base.Game;
import Rl.Client.Render.Unit.UnitRendererInfo;
import <vulkan/vulkan.hpp>;

namespace Rl::Client::Render
{

export void UnitCleanupResources(VkDevice device, Providers::UnitStateDrawableVulkan& vk);

export void UnitCleanupBuffers(VkDevice device, Providers::UnitStateDrawableVulkan& vk);

export void UnitCleanupTextures(VkDevice device, Providers::UnitStateDrawableVulkan& vk);

export void UnitCleanupSamplers(VkDevice device, Providers::UnitStateDrawableVulkan& vk);

export void UnitCleanupPipelines(VkDevice device, Providers::UnitStateDrawableVulkan& vk);

export void UnitCleanupDescriptorSets(VkDevice device, Providers::UnitStateDrawableVulkan& vk);

} // namespace Rl::Client::Render
