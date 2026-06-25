export module Rl.Client.Render.Unit.UnitRendererCurvatureCompute;

import Rl.Base.Game;
import Rl.Base.Shader;
import <vulkan/vulkan.hpp>;

namespace Rl::Client::Render
{

// Create push constant range for curvature compute shader
export void UnitCreateCurvatureComputePushConstantRange(VkPushConstantRange& pushConstantRange);

// Create pipeline layout for curvature compute shader
export void UnitCreateCurvatureComputePipelineLayout(
    VkDevice device, VkDescriptorSetLayout descriptorSetLayout, VkPipelineLayout& pipelineLayout);

// Create curvature compute pipeline
export void UnitCreateCurvatureComputePipeline(
    VkDevice device, VkPipelineLayout pipelineLayout, VkPipeline& pipeline);

} // namespace Rl::Client::Render
