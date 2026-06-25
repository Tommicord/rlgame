export module Rl.Client.Render.Unit.UnitRendererGraphicsPipeline;

import Rl.Base.Game;
import Rl.Base.Shader;
import <array>;
import <vulkan/vulkan.hpp>;

namespace Rl::Client::Render
{

export void UnitCreateGraphicsPipelineLayout(
    VkDevice device, VkDescriptorSetLayout descriptorSetLayout, VkPipelineLayout& pipelineLayout);

export void UnitCreateGraphicsPipeline(VkDevice                     device,
    VkPipelineLayout                                         pipelineLayout,
    VkRenderPass                                             renderPass,
    VkExtent2D                                               swapChainExtent,
    const VkVertexInputBindingDescription&                   bindingDescription,
    const std::array<VkVertexInputAttributeDescription, 14>& attributeDescriptions,
    VkPipeline&                                              pipeline);

} // namespace Rl::Client::Render
