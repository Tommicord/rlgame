#pragma once

#include <vulkan/vulkan.hpp>
#include "rl/Base/Game.h"
#include "rl/Client/Render/Unit/UnitRendererInfo.h"

namespace Rl::Client::Render
{

// Dispatch compute shaders for unit rendering
void UnitDispatchComputeShaders(Providers::UnitStateResource& resource,
    Providers::UnitStateDrawableVulkan&                       vk,
    Game::VulkanContext&                                      context);

} // namespace Rl::Client::Render
