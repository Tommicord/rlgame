#pragma once

#include "rl/Base/Game.h"
#include "rl/Client/Render/Unit/UnitRendererInfo.h"
#include <vulkan/vulkan.hpp>

namespace Rl::Client::Render
{

void UnitRender(Providers::UnitStateResource& resource,
    Providers::UnitStateDrawableVulkan&       vk,
    Game::VulkanContext&                      context);

} // namespace Rl::Client::Render
