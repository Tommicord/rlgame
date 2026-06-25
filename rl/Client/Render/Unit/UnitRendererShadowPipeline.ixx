export module Rl.Client.Render.Unit.UnitRendererShadowPipeline;

import <vulkan/vulkan.hpp>;

namespace Rl::Client::Render
{

export void UnitCreateShadowPipelineLayout(VkDevice device, VkPipelineLayout& pipelineLayout);

export void UnitCreateShadowPipeline(VkDevice device,
    VkPipelineLayout                   pipelineLayout,
    VkRenderPass                       renderPass,
    uint32_t                           width,
    uint32_t                           height,
    VkPipeline&                        pipeline);

} // namespace Rl::Client::Render
