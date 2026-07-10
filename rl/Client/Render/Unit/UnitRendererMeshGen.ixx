export module Rl.Client.Render.Unit.UnitRendererMeshGen;

import Rl.Base.Binding;
import <vulkan/vulkan.hpp>;
import <vector>;

namespace Rl::Client::Render
{

/* Push constants for unit mesh generation compute shader */
export struct UnitMeshGenPushConstants
{
  uint32_t unitId;
  uint32_t startVertex;
  uint32_t padding1;
  uint32_t padding2;
};

/* Initialize unit mesh generation compute pipeline */
export void UnitCreateMeshGenPipeline(VkDevice device, VkDescriptorSetLayout descriptorSetLayout,
    VkPipelineLayout& pipelineLayout, VkPipeline& pipeline);

/* Create descriptor set layout for mesh generation */
export void UnitCreateMeshGenDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout& descriptorSetLayout);

/* Generate unit cube mesh using compute shader */
export void UnitGenerateMesh(VkDevice device, VkCommandBuffer commandBuffer, VkPipeline pipeline,
    VkPipelineLayout pipelineLayout, VkDescriptorSet descriptorSet, uint32_t unitId, uint32_t startVertex);

/* Cleanup mesh generation resources */
export void UnitCleanupMeshGenPipeline(VkDevice device, VkPipeline pipeline, VkPipelineLayout pipelineLayout);

/* Cleanup descriptor set layout */
export void UnitCleanupMeshGenDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout);

} // namespace Rl::Client::Render
