#pragma once

#include <vulkan/vulkan.hpp>
#include "rl/Base/Game.h"
#include "rl/Base/ShaderFactory.h"
#include "rl/Client/Render/Unit/UnitRendererInfo.h"

namespace Rl::Client::Render
{

// Create compute pipeline layout
void UnitCreateComputePipelineLayout(
    VkDevice device, VkDescriptorSetLayout descriptorSetLayout, VkPipelineLayout& pipelineLayout);

// Create compute pipeline for face culling
void UnitCreateComputePipeline(VkDevice device, VkPipelineLayout pipelineLayout, VkPipeline& pipeline);

} // namespace Rl::Client::Render
