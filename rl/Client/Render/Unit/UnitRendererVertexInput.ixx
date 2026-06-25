export module Rl.Client.Render.Unit.UnitRendererVertexInput;

import <array>;
import <vulkan/vulkan.hpp>;

namespace Rl::Client::Render
{

/* Creates a binding for a vertex input structure */
export VkVertexInputBindingDescription UnitCreateVertexInputBindingDescription();

/* Creates a binding attributes for a vertex input structure */
export std::array<VkVertexInputAttributeDescription, 14> UnitCreateVertexAttributeDescriptions();

} // namespace Rl::Client::Render
