import Rl.Client.Render.Unit.UnitRendererCurvatureCompute;
import Rl.Base.Shader;

import <stdexcept>;
import <vulkan/vulkan.hpp>;

namespace Rl::Client::Render
{

void UnitCreateCurvatureComputePushConstantRange(VkPushConstantRange& pushConstantRange)
{
  pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  pushConstantRange.offset     = 0;
  pushConstantRange.size       = 8; // 2 uint32_t (tessellationLevel + faceCount)
}

void UnitCreateCurvatureComputePipelineLayout(
    VkDevice device, VkDescriptorSetLayout descriptorSetLayout, VkPipelineLayout& pipelineLayout)
{
  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  pushConstantRange.offset     = 0;
  pushConstantRange.size       = 8;

  VkPipelineLayoutCreateInfo curvePipelineLayoutInfo{};
  curvePipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  curvePipelineLayoutInfo.setLayoutCount         = 1;
  curvePipelineLayoutInfo.pSetLayouts            = &descriptorSetLayout;
  curvePipelineLayoutInfo.pushConstantRangeCount = 1;
  curvePipelineLayoutInfo.pPushConstantRanges    = &pushConstantRange;

  if (vkCreatePipelineLayout(device, &curvePipelineLayoutInfo, nullptr, &pipelineLayout) !=
      VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create curvature compute pipeline layout");
  }
}

void UnitCreateCurvatureComputePipeline(
    VkDevice device, VkPipelineLayout pipelineLayout, VkPipeline& pipeline)
{
  auto curveComputeShaderCode   = Providers::ShaderObject::Shader("unit.curve.comp.spv");
  auto curveComputeShaderModule = Providers::ShaderObject::Module(device, curveComputeShaderCode);

  VkPipelineShaderStageCreateInfo curveComputeShaderStageInfo{};
  curveComputeShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  curveComputeShaderStageInfo.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
  curveComputeShaderStageInfo.module = curveComputeShaderModule.module;
  curveComputeShaderStageInfo.pName  = "main";

  VkComputePipelineCreateInfo curveComputePipelineInfo{};
  curveComputePipelineInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  curveComputePipelineInfo.stage  = curveComputeShaderStageInfo;
  curveComputePipelineInfo.layout = pipelineLayout;

  if (vkCreateComputePipelines(
          device, VK_NULL_HANDLE, 1, &curveComputePipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create curvature compute pipeline");
  }
  Providers::ShaderObject::DestroyShaderModule(device, curveComputeShaderModule);
}

} // namespace Rl::Client::Render
