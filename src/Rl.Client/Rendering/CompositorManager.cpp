#include "Rl.Client/Rendering/CompositorManager.h"
#include "Rl.Client/Rendering/Compositor.h"
#include "Rl.Base/GameShaderModule.h"
#include "Rl.Base/GameVulkanAllocator.h"
#include "Rl.Base/GameError.h"
#include "Rl.Log/Log.h"

#include <stdexcept>

namespace rl
{

extern const uint32_t FullscreenQuadVert_data[];
extern const uint32_t FullscreenQuadVert_size;
extern const uint32_t FullscreenQuadFrag_data[];
extern const uint32_t FullscreenQuadFrag_size;

std::atomic<bool> CompositorManager::initializedFlag = false;

CompositorManager::CompositorManager() :
    device(VK_NULL_HANDLE), physicalDevice(VK_NULL_HANDLE), compositor(nullptr),
    syncManager(nullptr), pipelineLayout(VK_NULL_HANDLE), graphicsPipeline(VK_NULL_HANDLE),
    vertexBuffer(), sampler(), descriptorPool(VK_NULL_HANDLE), descriptorSetLayout(VK_NULL_HANDLE),
    initialized(false)
{
}

CompositorManager& CompositorManager::getInstance()
{
  static CompositorManager instance;
  return instance;
}

bool CompositorManager::isInitialized()
{
  return initializedFlag.load(std::memory_order_relaxed);
}

void CompositorManager::initialize(VkDevice         device,
                                   VkPhysicalDevice physicalDevice,
                                   uint32_t         maxFramesInFlight)
{
  auto&            instance = getInstance();
  std::scoped_lock lock(instance.mutex);

  if (!instance.initialized)
  {
    instance.device         = device;
    instance.physicalDevice = physicalDevice;
    instance.compositor     = std::make_unique<Compositor>();
    instance.syncManager =
        std::make_unique<CompositorSynchronizationHandler>(device, maxFramesInFlight);
    instance.compositor->registerObserver(&instance);
    instance.initialized = true;
    initializedFlag.store(true, std::memory_order_release);
  }
}

void CompositorManager::shutdown()
{
  auto&            instance = getInstance();
  std::scoped_lock lock(instance.mutex);

  if (instance.initialized)
  {
    if (instance.compositor)
    {
      instance.compositor->unregisterObserver(&instance);
    }
    instance.compositor.reset();
    instance.syncManager.reset();
    instance.initialized = false;
    initializedFlag.store(false, std::memory_order_release);
  }
}

Compositor& CompositorManager::getCompositor()
{
  std::scoped_lock lock(mutex);
  return *compositor;
}

void CompositorManager::addRenderTarget(IRenderTarget* target, int priority)
{
  std::scoped_lock lock(mutex);
  compositor->addRenderTarget(target, priority);
}

void CompositorManager::removeRenderTarget(IRenderTarget* target)
{
  std::scoped_lock lock(mutex);
  compositor->removeRenderTarget(target);
}

void CompositorManager::removeRenderTarget(uint64_t id)
{
  std::scoped_lock lock(mutex);
  compositor->removeRenderTarget(id);
}

IRenderTarget* CompositorManager::getRenderTarget(uint64_t id) const
{
  std::scoped_lock lock(mutex);
  return compositor->getRenderTarget(id);
}

std::vector<IRenderTarget*> CompositorManager::getAllRenderTargets() const
{
  std::scoped_lock lock(mutex);
  return compositor->getAllRenderTargets();
}

void CompositorManager::clearRenderTargets()
{
  std::scoped_lock lock(mutex);
  compositor->clear();
}

CompositorSynchronizationHandler& CompositorManager::getSynchronizationManager()
{
  std::scoped_lock lock(mutex);
  return *syncManager;
}

void CompositorManager::setup(GameDeviceInstance& device)
{
  std::scoped_lock lock(mutex);
  if (!initialized)
  {
    return;
  }

  setIsCompositing(true);
  setCanBeRendered(true);

  createFullscreenQuadPipeline(device);
  createDescriptorSets(device);
}

void CompositorManager::draw(GameDeviceInstance& device)
{
  std::scoped_lock lock(mutex);
  if (!initialized || !compositor->hasRenderTargets())
  {
    return;
  }

  auto renderTargets = compositor->getRenderTargetsByPriority();
  for (const auto& target : renderTargets)
  {
    if (target && target->isValid())
    {
      renderFullscreenQuad(device, target->getColorAttachmentView());
    }
  }
}

void CompositorManager::destroy(GameDeviceInstance& device)
{
  std::scoped_lock lock(mutex);
  if (graphicsPipeline != VK_NULL_HANDLE)
  {
    vkDestroyPipeline(device.getDevice(), graphicsPipeline, nullptr);
    graphicsPipeline = VK_NULL_HANDLE;
  }
  if (pipelineLayout != VK_NULL_HANDLE)
  {
    vkDestroyPipelineLayout(device.getDevice(), pipelineLayout, nullptr);
    pipelineLayout = VK_NULL_HANDLE;
  }
  if (descriptorPool != VK_NULL_HANDLE)
  {
    vkDestroyDescriptorPool(device.getDevice(), descriptorPool, nullptr);
    descriptorPool = VK_NULL_HANDLE;
  }
  if (descriptorSetLayout != VK_NULL_HANDLE)
  {
    vkDestroyDescriptorSetLayout(device.getDevice(), descriptorSetLayout, nullptr);
    descriptorSetLayout = VK_NULL_HANDLE;
  }
  descriptorSets.clear();
}

void CompositorManager::onRenderTargetAdded(IRenderTarget* target)
{
  updateDescriptorSets();
}

void CompositorManager::onRenderTargetRemoved(IRenderTarget* target)
{
  updateDescriptorSets();
}

void CompositorManager::onCompositorConfigurationChanged()
{
  updateDescriptorSets();
}

void CompositorManager::createFullscreenQuadPipeline(GameDeviceInstance& device)
{
  GameShaderModule vertShaderModule = GameShaderLoader::createShaderModule(
      device.getDevice(), FullscreenQuadVert_data, FullscreenQuadVert_size);
  GameShaderModule fragShaderModule = GameShaderLoader::createShaderModule(
      device.getDevice(), FullscreenQuadFrag_data, FullscreenQuadFrag_size);

  struct Vertex
  {
      float pos[2];
      float uv[2];
  };
  std::vector<Vertex> vertices = {{{-1.0f, -1.0f}, {0.0f, 0.0f}},
                                  {{1.0f, -1.0f}, {1.0f, 0.0f}},
                                  {{-1.0f, 1.0f}, {0.0f, 1.0f}},
                                  {{1.0f, 1.0f}, {1.0f, 1.0f}}};

  VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

  GameVulkanMemoryAllocator allocator(device.getDevice(), device.getPhysicalDevice());
  vertexBuffer =
      GameVulkanBuffer(&allocator, bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  void* data;
  vkMapMemory(device.getDevice(), vertexBuffer.getMemory(), vertexBuffer.getOffset(), bufferSize, 0,
              &data);
  memcpy(data, vertices.data(), bufferSize);
  vkUnmapMemory(device.getDevice(), vertexBuffer.getMemory());

  GameVulkanSamplerCreateInfo samplerInfo{};
  samplerInfo.magFilter               = VK_FILTER_LINEAR;
  samplerInfo.minFilter               = VK_FILTER_LINEAR;
  samplerInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.anisotropyEnable        = VK_FALSE;
  samplerInfo.maxAnisotropy           = 1.0f;
  samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable           = VK_FALSE;
  samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias              = 0.0f;
  samplerInfo.minLod                  = 0.0f;
  samplerInfo.maxLod                  = 0.0f;

  sampler = GameVulkanSampler(device.getDevice(), samplerInfo);

  VkDescriptorSetLayoutBinding samplerLayoutBinding{};
  samplerLayoutBinding.binding            = 0;
  samplerLayoutBinding.descriptorCount    = 1;
  samplerLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerLayoutBinding.pImmutableSamplers = nullptr;
  samplerLayoutBinding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = 1;
  layoutInfo.pBindings    = &samplerLayoutBinding;

  if (vkCreateDescriptorSetLayout(device.getDevice(), &layoutInfo, nullptr, &descriptorSetLayout) !=
      VK_SUCCESS)
  {
    GameError::exitWithError("vkCreateDescriptorSetLayout",
                             "Failed to create descriptor set layout");
  }

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

  VkVertexInputBindingDescription bindingDescription{};
  bindingDescription.binding   = 0;
  bindingDescription.stride    = sizeof(Vertex);
  bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
  attributeDescriptions[0].binding  = 0;
  attributeDescriptions[0].location = 0;
  attributeDescriptions[0].format   = VK_FORMAT_R32G32_SFLOAT;
  attributeDescriptions[0].offset   = offsetof(Vertex, pos);

  attributeDescriptions[1].binding  = 0;
  attributeDescriptions[1].location = 1;
  attributeDescriptions[1].format   = VK_FORMAT_R32G32_SFLOAT;
  attributeDescriptions[1].offset   = offsetof(Vertex, uv);

  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.pVertexBindingDescriptions    = &bindingDescription;
  vertexInputInfo.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(attributeDescriptions.size());
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
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
  pipelineLayoutInfo.setLayoutCount         = 1;
  pipelineLayoutInfo.pSetLayouts            = &descriptorSetLayout;
  pipelineLayoutInfo.pushConstantRangeCount = 0;

  if (vkCreatePipelineLayout(device.getDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) !=
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
  pipelineInfo.renderPass          = device.getRenderPass();
  pipelineInfo.subpass             = 0;
  pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE;
  pipelineInfo.basePipelineIndex   = -1;

  if (vkCreateGraphicsPipelines(device.getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
                                &graphicsPipeline) != VK_SUCCESS)
  {
    GameError::exitWithError("vkCreateGraphicsPipeline",
                             "Failed to create Vulkan Graphics Pipeline");
  }
}

void CompositorManager::createDescriptorSets(GameDeviceInstance& device)
{
  size_t targetCount = compositor->getRenderTargetCount();
  if (targetCount == 0)
  {
    return;
  }

  VkDescriptorPoolSize poolSize{};
  poolSize.type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSize.descriptorCount = static_cast<uint32_t>(targetCount);

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = 1;
  poolInfo.pPoolSizes    = &poolSize;
  poolInfo.maxSets       = static_cast<uint32_t>(targetCount);

  if (vkCreateDescriptorPool(device.getDevice(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create descriptor pool");
  }

  std::vector<VkDescriptorSetLayout> layouts(targetCount, descriptorSetLayout);
  VkDescriptorSetAllocateInfo        allocInfo{};
  allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool     = descriptorPool;
  allocInfo.descriptorSetCount = static_cast<uint32_t>(targetCount);
  allocInfo.pSetLayouts        = layouts.data();

  descriptorSets.resize(targetCount);
  if (vkAllocateDescriptorSets(device.getDevice(), &allocInfo, descriptorSets.data()) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to allocate descriptor sets");
  }

  updateDescriptorSets();
}

void CompositorManager::updateDescriptorSets()
{
  if (descriptorPool == VK_NULL_HANDLE)
  {
    return;
  }

  auto   renderTargets = compositor->getRenderTargetsByPriority();
  size_t i             = 0;
  for (const auto& target : renderTargets)
  {
    if (i >= descriptorSets.size())
    {
      break;
    }

    if (target && target->isValid())
    {
      VkDescriptorImageInfo imageInfo{};
      imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      imageInfo.imageView   = target->getColorAttachmentView().getImageView();
      imageInfo.sampler     = sampler.getSampler();

      VkWriteDescriptorSet descriptorWrite{};
      descriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrite.dstSet          = descriptorSets[i];
      descriptorWrite.dstBinding      = 0;
      descriptorWrite.dstArrayElement = 0;
      descriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      descriptorWrite.descriptorCount = 1;
      descriptorWrite.pImageInfo      = &imageInfo;

      vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
    }
    ++i;
  }
}

void CompositorManager::renderFullscreenQuad(GameDeviceInstance&  device,
                                             GameVulkanImageView& textureView)
{
  VkCommandBuffer commandBuffer = device.getCommandBuffer();
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

  VkBuffer     vertexBuffers[] = {vertexBuffer.getBuffer()};
  VkDeviceSize offsets[]       = {vertexBuffer.getOffset()};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

  VkViewport viewport{};
  viewport.x        = 0.0f;
  viewport.y        = 0.0f;
  viewport.width    = static_cast<float>(device.getExtent2d().width);
  viewport.height   = static_cast<float>(device.getExtent2d().height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = device.getExtent2d();
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  VkDescriptorImageInfo imageInfo{};
  imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfo.imageView   = textureView.getImageView();
  imageInfo.sampler     = sampler.getSampler();

  VkWriteDescriptorSet descriptorWrite{};
  descriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.dstSet          = descriptorSets[0];
  descriptorWrite.dstBinding      = 0;
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.pImageInfo      = &imageInfo;

  vkUpdateDescriptorSets(device.getDevice(), 1, &descriptorWrite, 0, nullptr);
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                          &descriptorSets[0], 0, nullptr);

  vkCmdDraw(commandBuffer, 4, 1, 0, 0);
}

} // namespace rl
