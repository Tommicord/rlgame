#pragma once

#include <array>
#include <vulkan/vulkan.hpp>
#include "rl/Base/Game.h"
#include "rl/Base/ShaderFactory.h"

namespace Rl::Client::Render
{

void UnitCreateGraphicsPipelineLayout(
    VkDevice device, VkDescriptorSetLayout descriptorSetLayout, VkPipelineLayout& pipelineLayout);

void UnitCreateGraphicsPipeline(VkDevice                     device,
    VkPipelineLayout                                         pipelineLayout,
    VkRenderPass                                             renderPass,
    VkExtent2D                                               swapChainExtent,
    const VkVertexInputBindingDescription&                   bindingDescription,
    const std::array<VkVertexInputAttributeDescription, 14>& attributeDescriptions,
    VkPipeline&                                              pipeline);

} // namespace Rl::Client::Render
