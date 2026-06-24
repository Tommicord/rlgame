#pragma once

#include "rl/Base/Game.h"
#include "rl/Base/ShaderFactory.h"

#include <vulkan/vulkan.hpp>

namespace Rl::Client::Render
{

// Create push constant range for curvature compute shader
void UnitCreateCurvatureComputePushConstantRange(VkPushConstantRange& pushConstantRange);

// Create pipeline layout for curvature compute shader
void UnitCreateCurvatureComputePipelineLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, VkPipelineLayout& pipelineLayout);

// Create curvature compute pipeline
void UnitCreateCurvatureComputePipeline(VkDevice device, VkPipelineLayout pipelineLayout,
    VkPipeline& pipeline);

} // namespace Rl::Client::Render
