import Rl.Client.Render.Unit.UnitRendererComputePipeline;
import Rl.Client.Render.Unit.UnitRendererInfo;
import Rl.Base.Shader;

import <stdexcept>;
import <vulkan/vulkan.hpp>;

namespace Rl::Client::Render
{

void UnitCreateComputePipelineLayout(
    VkDevice device, VkDescriptorSetLayout descriptorSetLayout, VkPipelineLayout& pipelineLayout)
{
  VkPipelineLayoutCreateInfo computePipelineLayoutInfo{};
  computePipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  computePipelineLayoutInfo.setLayoutCount         = 1;
  computePipelineLayoutInfo.pSetLayouts            = &descriptorSetLayout;
  computePipelineLayoutInfo.pushConstantRangeCount = 1;

  VkPushConstantRange computePushConstantRange{};
  computePushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT;
  computePushConstantRange.offset     = 0;
  computePushConstantRange.size       = sizeof(UnitRenderUBO);
  computePipelineLayoutInfo.pPushConstantRanges = &computePushConstantRange;

  if (vkCreatePipelineLayout(device, &computePipelineLayoutInfo, nullptr, &pipelineLayout) !=
      VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create compute pipeline layout");
  }
}

void UnitCreateComputePipeline(
    VkDevice device, VkPipelineLayout pipelineLayout, VkPipeline& pipeline)
{
  auto computeShaderCode   = Providers::ShaderObject::Shader("unit.face.comp.spv");
  auto computeShaderModule = Providers::ShaderObject::Module(device, computeShaderCode);

  VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
  computeShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  computeShaderStageInfo.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
  computeShaderStageInfo.module = computeShaderModule.module;
  computeShaderStageInfo.pName  = "main";

  VkComputePipelineCreateInfo computePipelineInfo{};
  computePipelineInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  computePipelineInfo.stage  = computeShaderStageInfo;
  computePipelineInfo.layout = pipelineLayout;

  if (vkCreateComputePipelines(
          device, VK_NULL_HANDLE, 1, &computePipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create compute pipeline");
  }
  Providers::ShaderObject::DestroyShaderModule(device, computeShaderModule);
}

} // namespace Rl::Client::Render
