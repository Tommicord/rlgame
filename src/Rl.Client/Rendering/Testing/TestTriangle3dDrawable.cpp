#include "Rl.Client/Rendering/Testing/TestTriangle3dDrawable.h"
#include "Rl.Base/GameVulkanShaderModule.h"
#include "Rl.Base/MainGame.h"
#include "Rl.Log/Log.h"

#include <cstring>
#include <vector>
#include <vulkan/vulkan.hpp>

using rl::makeLogArray;

namespace rl
{

extern const uint32_t Triangle3dVert_data[];
extern const uint32_t Triangle3dVert_size;
extern const uint32_t Triangle3dFrag_data[];
extern const uint32_t Triangle3dFrag_size;

struct Vertex
{
    Vec3 pos;
    Vec3 color;
};

TestTriangle3dDrawable::TestTriangle3dDrawable() noexcept
{
  setIsTest(true);
  setTestCanRender(true);
  setIsCompositing(true);
  GameDeviceInstance::registerDrawable(this);
}

void TestTriangle3dDrawable::setup(GameDeviceInstance& instance)
{
  VkExtent2D       extent         = instance.getExtent2d();
  VkDevice         device         = instance.getDevice();
  VkPhysicalDevice physicalDevice = instance.getPhysicalDevice();

  renderTarget = RenderTarget(device, physicalDevice, extent);

  std::vector<Vertex> vertices = {{{0.0f, -0.5f, -5.0f}, {1.0f, 0.0f, 0.0f}},
                                  {{0.5f, 0.5f, -5.0f}, {0.0f, 1.0f, 0.0f}},
                                  {{-0.5f, 0.5f, -5.0f}, {0.0f, 0.0f, 1.0f}}};

  memoryAllocator = GameVulkanMemoryAllocator(instance.getDevice(), instance.getPhysicalDevice());

  vertexBuffer =
      GameVulkanBuffer(&memoryAllocator, sizeof(vertices[0]) * vertices.size(),
                       VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  void* data;
  vkMapMemory(instance.getDevice(), vertexBuffer.getMemory(), vertexBuffer.getOffset(),
              vertexBuffer.getSize(), 0, &data);
  memcpy(data, vertices.data(), vertexBuffer.getSize());
  vkUnmapMemory(instance.getDevice(), vertexBuffer.getMemory());

  vertShaderModule = GameVulkanShader::shader(instance.getDevice(), Triangle3dVert_data,
                                                          Triangle3dVert_size);
  fragShaderModule = GameVulkanShader::shader(instance.getDevice(), Triangle3dFrag_data,
                                                          Triangle3dFrag_size);

  VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
  vertShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertShaderModule.getShaderModule();
  vertShaderStageInfo.pName  = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
  fragShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragShaderModule.getShaderModule();
  fragShaderStageInfo.pName  = "main";

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

  VkVertexInputBindingDescription bindingDescription{};
  bindingDescription.binding   = 0;
  bindingDescription.stride    = sizeof(Vertex);
  bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
  attributeDescriptions[0].binding  = 0;
  attributeDescriptions[0].location = 0;
  attributeDescriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[0].offset   = offsetof(Vertex, pos);

  attributeDescriptions[1].binding  = 0;
  attributeDescriptions[1].location = 1;
  attributeDescriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[1].offset   = offsetof(Vertex, color);

  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.pVertexBindingDescriptions    = &bindingDescription;
  vertexInputInfo.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(attributeDescriptions.size());
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

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

  VkPipelineDepthStencilStateCreateInfo depthStencil{};
  depthStencil.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.depthTestEnable       = VK_TRUE;
  depthStencil.depthWriteEnable      = VK_TRUE;
  depthStencil.depthCompareOp        = VK_COMPARE_OP_LESS_OR_EQUAL;
  depthStencil.depthBoundsTestEnable = VK_FALSE;
  depthStencil.stencilTestEnable     = VK_FALSE;

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

  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  pushConstantRange.offset     = 0;
  pushConstantRange.size       = sizeof(Mat4);

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount         = 0;
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges    = &pushConstantRange;

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
  pipelineInfo.pDepthStencilState  = &depthStencil;
  pipelineInfo.pColorBlendState    = &colorBlending;
  pipelineInfo.pDynamicState       = &dynamicState;
  pipelineInfo.layout              = pipelineLayout;
  pipelineInfo.renderPass          = instance.getRenderPass();
  pipelineInfo.subpass             = 0;
  pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE;
  pipelineInfo.basePipelineIndex   = -1;

  if (vkCreateGraphicsPipelines(instance.getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
                                &graphicsPipeline) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create Vulkan Graphics Pipeline");
  }
}

void TestTriangle3dDrawable::draw(GameDeviceInstance& instance)
{
  VkCommandBuffer commandBuffer = instance.getCommandBuffer();

  auto playerController = instance.getGameResources().getPlayer().getController();
  auto camera           = playerController.getCamera();

  Mat4 model = camera.m;
  Mat4 view  = camera.v;
  Mat4 proj  = camera.p;
  Mat4 mvp   = model * view * proj;

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
  vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Mat4),
                     &mvp);

  VkBuffer     vertexBuffers[] = {vertexBuffer.getBuffer()};
  VkDeviceSize offsets[]       = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

  vkCmdDraw(commandBuffer, 3, 1, 0, 0);
}

void TestTriangle3dDrawable::destroy(GameDeviceInstance& instance)
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
