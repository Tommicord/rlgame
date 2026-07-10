import Rl.Client.Render.Unit.UnitRendererMeshGen;
import Rl.Base.Shader;
import Rl.Base.Binding;

import <stdexcept>;
import <vulkan/vulkan.hpp>;

namespace Rl::Client::Render
{

void UnitCreateMeshGenDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout& descriptorSetLayout)
{
  VkDescriptorSetLayoutBinding bufferBinding{};
  bufferBinding.binding = 0;
  bufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  bufferBinding.descriptorCount = 1;
  bufferBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  bufferBinding.pImmutableSamplers = nullptr;

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = 1;
  layoutInfo.pBindings = &bufferBinding;

  if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create mesh generation descriptor set layout");
  }
}

void UnitCreateMeshGenPipeline(VkDevice device, VkDescriptorSetLayout descriptorSetLayout,
    VkPipelineLayout& pipelineLayout, VkPipeline& pipeline)
{
  // Create pipeline layout with descriptor set and push constants
  VkPipelineLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layoutInfo.setLayoutCount = 1;
  layoutInfo.pSetLayouts = &descriptorSetLayout;
  layoutInfo.pushConstantRangeCount = 1;

  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(UnitMeshGenPushConstants);
  layoutInfo.pPushConstantRanges = &pushConstantRange;

  if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create mesh generation pipeline layout");
  }

  // Load compute shader
  auto computeShaderCode = Providers::ShaderObject::Shader("unit.mesh.gen.comp.spv");
  auto computeShaderModule = Providers::ShaderObject::Module(device, computeShaderCode);

  VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
  computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  computeShaderStageInfo.module = computeShaderModule.module;
  computeShaderStageInfo.pName = "main";

  VkComputePipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.stage = computeShaderStageInfo;

  if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create mesh generation compute pipeline");
  }

  Providers::ShaderObject::DestroyShaderModule(device, computeShaderModule);
}

void UnitGenerateMesh(VkDevice device, VkCommandBuffer commandBuffer, VkPipeline pipeline,
    VkPipelineLayout pipelineLayout, VkDescriptorSet descriptorSet, uint32_t unitId, uint32_t startVertex)
{
  // Bind compute pipeline
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

  // Bind descriptor set
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout,
      0, 1, &descriptorSet, 0, nullptr);

  // Set push constants
  UnitMeshGenPushConstants pushConstants{};
  pushConstants.unitId = unitId;
  pushConstants.startVertex = startVertex;
  pushConstants.padding1 = 0;
  pushConstants.padding2 = 0;

  vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT,
      0, sizeof(UnitMeshGenPushConstants), &pushConstants);

  // Dispatch compute shader (6 faces * 4 vertices = 24 vertices total)
  vkCmdDispatch(commandBuffer, 1, 1, 1);
}

void UnitCleanupMeshGenPipeline(VkDevice device, VkPipeline pipeline, VkPipelineLayout pipelineLayout)
{
  if (pipeline != VK_NULL_HANDLE)
  {
    vkDestroyPipeline(device, pipeline, nullptr);
  }
  if (pipelineLayout != VK_NULL_HANDLE)
  {
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
  }
}

void UnitCleanupMeshGenDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout)
{
  if (descriptorSetLayout != VK_NULL_HANDLE)
  {
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
  }
}

} // namespace Rl::Client::Render
