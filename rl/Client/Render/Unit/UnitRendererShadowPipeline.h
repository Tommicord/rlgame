#pragma once

#include <vulkan/vulkan.hpp>

namespace Rl::Client::Render
{

void UnitCreateShadowPipelineLayout(VkDevice device, VkPipelineLayout& pipelineLayout);

void UnitCreateShadowPipeline(VkDevice device,
    VkPipelineLayout                   pipelineLayout,
    VkRenderPass                       renderPass,
    uint32_t                           width,
    uint32_t                           height,
    VkPipeline&                        pipeline);

} // namespace Rl::Client::Render
