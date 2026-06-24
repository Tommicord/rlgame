#pragma once

#include <array>
#include <vulkan/vulkan.hpp>

namespace Rl::Client::Render
{

/* Creates a binding for a vertex input structure */
VkVertexInputBindingDescription UnitCreateVertexInputBindingDescription();

/* Creates a binding attributes for a vertex input structure */
std::array<VkVertexInputAttributeDescription, 14> UnitCreateVertexAttributeDescriptions();
} // namespace Rl::Client::Render
