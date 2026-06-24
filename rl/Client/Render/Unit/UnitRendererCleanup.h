#pragma once

#include "rl/Base/Game.h"
#include "rl/Client/Render/Unit/UnitRendererInfo.h"
#include <vulkan/vulkan.hpp>

namespace Rl::Client::Render
{

void UnitCleanupResources(VkDevice device, Providers::UnitStateDrawableVulkan& vk);

void UnitCleanupBuffers(VkDevice device, Providers::UnitStateDrawableVulkan& vk);

void UnitCleanupTextures(VkDevice device, Providers::UnitStateDrawableVulkan& vk);

void UnitCleanupSamplers(VkDevice device, Providers::UnitStateDrawableVulkan& vk);

void UnitCleanupPipelines(VkDevice device, Providers::UnitStateDrawableVulkan& vk);

void UnitCleanupDescriptorSets(VkDevice device, Providers::UnitStateDrawableVulkan& vk);

} // namespace Rl::Client::Render
