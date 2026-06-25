export module Rl.Client.Render.Unit.UnitRendererComputePipeline;

import Rl.Base.Game;
import Rl.Base.Shader;
import Rl.Client.Render.Unit.UnitRendererInfo;
import <vulkan/vulkan.hpp>;

namespace Rl::Client::Render
{

// Create compute pipeline layout
export void UnitCreateComputePipelineLayout(
    VkDevice device, VkDescriptorSetLayout descriptorSetLayout, VkPipelineLayout& pipelineLayout);

// Create compute pipeline for face culling
export void UnitCreateComputePipeline(
    VkDevice device, VkPipelineLayout pipelineLayout, VkPipeline& pipeline);

} // namespace Rl::Client::Render
