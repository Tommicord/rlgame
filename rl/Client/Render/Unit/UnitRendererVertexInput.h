#pragma once

#include <array>
#include <vulkan/vulkan.hpp>
#include "rl/Client/Render/Unit/UnitRendererInfo.h"

namespace Rl::Client::Render
{

// Create vertex input binding description
VkVertexInputBindingDescription UnitCreateVertexBindingDescription();

// Create vertex input attribute descriptions for UnitRenderVertex
std::array<VkVertexInputAttributeDescription, 13> UnitCreateVertexAttributeDescriptions();

} // namespace Rl::Client::Render
