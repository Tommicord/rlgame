import Rl.Client.Render.Unit.UnitRendererShadowPipeline;
import Rl.Base.Shader;
import Rl.Client.Render.Unit.UnitRendererInfo;
import Rl.Client.Render.Unit.UnitRendererVertexInput;

import <stdexcept>;

namespace Rl::Client::Render
{

void UnitCreateShadowPipelineLayout(VkDevice device, VkPipelineLayout& pipelineLayout)
{
  VkPipelineLayoutCreateInfo shadowPipelineLayoutInfo{};
  shadowPipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  shadowPipelineLayoutInfo.setLayoutCount         = 0;
  shadowPipelineLayoutInfo.pSetLayouts            = nullptr;
  shadowPipelineLayoutInfo.pushConstantRangeCount = 1;

  VkPushConstantRange shadowPushConstantRange{};
  shadowPushConstantRange.stageFlags           = VK_SHADER_STAGE_VERTEX_BIT;
  shadowPushConstantRange.offset               = 0;
  shadowPushConstantRange.size                 = sizeof(glm::mat4); // light space matrix
  shadowPipelineLayoutInfo.pPushConstantRanges = &shadowPushConstantRange;

  if (vkCreatePipelineLayout(device, &shadowPipelineLayoutInfo, nullptr, &pipelineLayout) !=
      VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create shadow pipeline layout");
  }
}

void UnitCreateShadowPipeline(VkDevice device,
    VkPipelineLayout                   pipelineLayout,
    VkRenderPass                       renderPass,
    uint32_t                           width,
    uint32_t                           height,
    VkPipeline&                        pipeline)
{
  auto vertShaderCode   = Providers::ShaderObject::Shader("unit.shadow.vert.spv");
  auto fragShaderCode   = Providers::ShaderObject::Shader("unit.shadow.frag.spv");
  auto vertShaderModule = Providers::ShaderObject::Module(device, vertShaderCode);
  auto fragShaderModule = Providers::ShaderObject::Module(device, fragShaderCode);

  VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
  vertShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertShaderModule.module;
  vertShaderStageInfo.pName  = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
  fragShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragShaderModule.module;
  fragShaderStageInfo.pName  = "main";

  VkPipelineShaderStageCreateInfo shadowShaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

  VkVertexInputBindingDescription bindingDescription{};
  bindingDescription.binding   = 0;
  bindingDescription.stride    = sizeof(UnitRenderVertex);
  bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  VkVertexInputAttributeDescription positionAttribute{};
  positionAttribute.binding  = 0;
  positionAttribute.location = 0;
  positionAttribute.format   = VK_FORMAT_R32G32B32A32_SFLOAT;
  positionAttribute.offset   = offsetof(UnitRenderVertex, position);

  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount   = 1;
  vertexInputInfo.pVertexBindingDescriptions      = &bindingDescription;
  vertexInputInfo.vertexAttributeDescriptionCount = 1;
  vertexInputInfo.pVertexAttributeDescriptions    = &positionAttribute;

  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport{};
  viewport.x        = 0.0f;
  viewport.y        = 0.0f;
  viewport.width    = static_cast<float>(width);
  viewport.height   = static_cast<float>(height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = {width, height};

  VkPipelineViewportStateCreateInfo viewportState{};
  viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.pViewports    = &viewport;
  viewportState.scissorCount  = 1;
  viewportState.pScissors     = &scissor;

  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable        = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth               = 1.0f;
  rasterizer.cullMode                = VK_CULL_MODE_FRONT_BIT; // Cull front faces for shadow bias
  rasterizer.frontFace               = VK_FRONT_FACE_CLOCKWISE;
  rasterizer.depthBiasEnable         = VK_TRUE;
  rasterizer.depthBiasConstantFactor = 1.25f;
  rasterizer.depthBiasSlopeFactor    = 1.75f;

  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable  = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineDepthStencilStateCreateInfo depthStencil{};
  depthStencil.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.depthTestEnable       = VK_TRUE;
  depthStencil.depthWriteEnable      = VK_TRUE;
  depthStencil.depthCompareOp        = VK_COMPARE_OP_LESS_OR_EQUAL;
  depthStencil.depthBoundsTestEnable = VK_FALSE;
  depthStencil.stencilTestEnable     = VK_FALSE;

  // No color attachment for shadow pass
  VkPipelineColorBlendStateCreateInfo colorBlending{};
  colorBlending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable   = VK_FALSE;
  colorBlending.attachmentCount = 0;
  colorBlending.pAttachments    = nullptr;

  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount          = 2;
  pipelineInfo.pStages             = shadowShaderStages;
  pipelineInfo.pVertexInputState   = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState      = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState   = &multisampling;
  pipelineInfo.pDepthStencilState  = &depthStencil;
  pipelineInfo.pColorBlendState    = &colorBlending;
  pipelineInfo.pDynamicState       = nullptr;
  pipelineInfo.layout              = pipelineLayout;
  pipelineInfo.renderPass          = renderPass;
  pipelineInfo.subpass             = 0;
  pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE;

  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) !=
      VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create shadow graphics pipeline");
  }

  Providers::ShaderObject::DestroyShaderModule(device, vertShaderModule);
  Providers::ShaderObject::DestroyShaderModule(device, fragShaderModule);
}

} // namespace Rl::Client::Render
