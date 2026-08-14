#include "Rl.Client/Rendering/Testing/TestTriangleDrawable.h"
#include "Rl.Base/GameShaderModule.h"

#include <vector>
#include <vulkan/vulkan.hpp>

namespace rl
{

extern const uint32_t TriangleVert_data[];
extern const uint32_t TriangleVert_size;
extern const uint32_t TriangleFrag_data[];
extern const uint32_t TriangleFrag_size;

TestTriangleDrawable::TestTriangleDrawable() noexcept
{
  setIsTest(true);
  setTestCanRender(false);
  setIsCompositing(true);
  GameDeviceInstance::registerDrawable(this);
}

void TestTriangleDrawable::setup(GameDeviceInstance& instance)
{
  VkExtent2D       extent         = instance.getExtent2d();
  VkDevice         device         = instance.getDevice();
  VkPhysicalDevice physicalDevice = instance.getPhysicalDevice();

  renderTarget = RenderTarget(device, physicalDevice, extent);

  vertShaderModule = GameShaderLoader::createShaderModule(instance.getDevice(), TriangleVert_data,
                                                          TriangleVert_size);
  fragShaderModule = GameShaderLoader::createShaderModule(instance.getDevice(), TriangleFrag_data,
                                                          TriangleFrag_size);

  VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
  vertShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertShaderModule.shaderModule;
  vertShaderStageInfo.pName  = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
  fragShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragShaderModule.shaderModule;
  fragShaderStageInfo.pName  = "main";

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount   = 0;
  vertexInputInfo.vertexAttributeDescriptionCount = 0;

  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkPipelineViewportStateCreateInfo viewportState{};
  viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.scissorCount  = 1;

  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable        = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth               = 1.0f;
  rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace               = VK_FRONT_FACE_CLOCKWISE;
  rasterizer.depthBiasEnable         = VK_FALSE;

  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable  = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo colorBlending{};
  colorBlending.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable     = VK_FALSE;
  colorBlending.logicOp           = VK_LOGIC_OP_COPY;
  colorBlending.attachmentCount   = 1;
  colorBlending.pAttachments      = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0f;
  colorBlending.blendConstants[1] = 0.0f;
  colorBlending.blendConstants[2] = 0.0f;
  colorBlending.blendConstants[3] = 0.0f;

  std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynamicState{};
  dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
  dynamicState.pDynamicStates    = dynamicStates.data();

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount         = 0;
  pipelineLayoutInfo.pushConstantRangeCount = 0;

  if (vkCreatePipelineLayout(instance.getDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) !=
      VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create Vulkan Pipeline Layout");
  }

  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount          = 2;
  pipelineInfo.pStages             = shaderStages;
  pipelineInfo.pVertexInputState   = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState      = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState   = &multisampling;
  pipelineInfo.pDepthStencilState  = nullptr;
  pipelineInfo.pColorBlendState    = &colorBlending;
  pipelineInfo.pDynamicState       = &dynamicState;
  pipelineInfo.layout              = pipelineLayout;

  const GameVulkanRenderPass& renderPass = renderTarget.getRenderPass();
  pipelineInfo.renderPass                = renderPass.getRenderPass();
  pipelineInfo.subpass                   = 0;
  pipelineInfo.basePipelineHandle        = VK_NULL_HANDLE;
  pipelineInfo.basePipelineIndex         = -1;

  if (vkCreateGraphicsPipelines(instance.getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
                                &graphicsPipeline) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create Vulkan Graphics Pipeline");
  }
}

void TestTriangleDrawable::draw(GameDeviceInstance& instance)
{
  VkCommandBuffer commandBuffer = instance.getCommandBuffer();

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType                     = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  const GameVulkanRenderPass& renderPass   = renderTarget.getRenderPass();
  renderPassInfo.renderPass                = renderPass.getRenderPass();
  const GameVulkanFramebuffer& framebuffer = renderTarget.getFramebuffer();
  renderPassInfo.framebuffer               = framebuffer.getFramebuffer();
  renderPassInfo.renderArea.offset         = {0, 0};
  renderPassInfo.renderArea.extent         = renderTarget.getExtent();

  VkClearValue clearColor        = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues    = &clearColor;

  VkViewport viewport{};
  viewport.x        = 0.0f;
  viewport.y        = 0.0f;
  viewport.width    = static_cast<float>(renderTarget.getExtent().width);
  viewport.height   = static_cast<float>(renderTarget.getExtent().height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = renderTarget.getExtent();
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
  vkCmdDraw(commandBuffer, 3, 1, 0, 0);
}

void TestTriangleDrawable::destroy(GameDeviceInstance& instance)
{
  if (graphicsPipeline != VK_NULL_HANDLE)
  {
    vkDestroyPipeline(instance.getDevice(), graphicsPipeline, nullptr);
    graphicsPipeline = VK_NULL_HANDLE;
  }
  if (pipelineLayout != VK_NULL_HANDLE)
  {
    vkDestroyPipelineLayout(instance.getDevice(), pipelineLayout, nullptr);
    pipelineLayout = VK_NULL_HANDLE;
  }
}

} // namespace rl
