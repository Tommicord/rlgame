import Rl.Client.Render.Unit.UnitRendererShadowMap;
import Rl.Client.Render.Unit.UnitRendererBasicBuffer;

import <vulkan/vulkan.hpp>;
import <stdexcept>;

namespace Rl::Client::Render
{

void UnitCreateCascadeShadowMapResources(VkDevice device,
    VkPhysicalDevice                              physicalDevice,
    UnitCascadeShadowMapConfig                    config,
    UnitCascadeShadowMapResources&                resources)
{
  resources.shadowMapCascades.resize(config.numCascades);

  VkAttachmentDescription depthAttachment{};
  depthAttachment.format = VK_FORMAT_D32_SFLOAT;
  depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depthAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  VkAttachmentReference depthAttachmentRef{};
  depthAttachmentRef.attachment = 0;
  depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.pDepthStencilAttachment = &depthAttachmentRef;

  VkSubpassDependency dependencies[2]{};
  // Input dependency
  dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[0].dstSubpass = 0;
  dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
  dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  // Output dependency
  dependencies[1].srcSubpass = 0;
  dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
  dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &depthAttachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 2;
  renderPassInfo.pDependencies = dependencies;

  if (vkCreateRenderPass(
          device, &renderPassInfo, nullptr, &resources.shadowMapRenderPass) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create cascade shadow map render pass");
  }
  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
  samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  samplerInfo.compareEnable = VK_TRUE;
  samplerInfo.compareOp = VK_COMPARE_OP_LESS;
  samplerInfo.anisotropyEnable = VK_FALSE;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = 1.0f;

  if (vkCreateSampler(device, &samplerInfo, nullptr, &resources.shadowMapSampler) !=
      VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create cascade shadow map sampler");
  }

  for (uint32_t i = 0; i < config.numCascades; ++i)
  {
    // Create Depth image
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = config.width;
    imageInfo.extent.height = config.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_D32_SFLOAT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage =
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &imageInfo, nullptr,
            &resources.shadowMapCascades[i].image) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create cascade shadow map image");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(
        device, resources.shadowMapCascades[i].image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = UnitFindMemoryTypeIndex(
        physicalDevice, memRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(device, &allocInfo, nullptr,
            &resources.shadowMapCascades[i].memory) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to allocate cascade shadow map memory");
    }
    vkBindImageMemory(device, resources.shadowMapCascades[i].image,
        resources.shadowMapCascades[i].memory, 0);

    // Image view
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = resources.shadowMapCascades[i].image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_D32_SFLOAT;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device, &viewInfo, nullptr,
            &resources.shadowMapCascades[i].view) != VK_SUCCESS)
    {
      throw std::runtime_error(
          "Failed to create cascade shadow map image view");
    }

    // Framebuffer
    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = resources.shadowMapRenderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = &resources.shadowMapCascades[i].view;
    framebufferInfo.width = config.width;
    framebufferInfo.height = config.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr,
            &resources.shadowMapCascades[i].framebuffer) != VK_SUCCESS)
    {
      throw std::runtime_error(
          "Failed to create cascade shadow map framebuffer");
    }
  }
}

void UnitBeginCascadeShadowMapRenderPass(VkCommandBuffer commandBuffer,
    VkRenderPass                                         renderPass,
    VkFramebuffer                                        framebuffer,
    uint32_t                                             width,
    uint32_t                                             height)
{
  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = renderPass;
  renderPassInfo.framebuffer = framebuffer;
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = {width, height};

  VkClearValue clearValue{};
  clearValue.depthStencil = {1.0f, 0};
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clearValue;

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

  // Viewport
  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(width);
  viewport.height = static_cast<float>(height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  // Scissor
  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = {width, height};
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void UnitEndCascadeShadowMapRenderPass(VkCommandBuffer commandBuffer)
{ vkCmdEndRenderPass(commandBuffer); }

void UnitTransitionCascadeShadowMapLayout(VkCommandBuffer commandBuffer,
    const UnitCascadeShadowMapResources&                  resources,
    VkImageLayout                                         oldLayout,
    VkImageLayout                                         newLayout)
{
  for (const auto& cascade : resources.shadowMapCascades)
  {
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = cascade.image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    VkPipelineStageFlags srcStage, dstStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
        newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
      barrier.srcAccessMask = 0;
      barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else
    {
      barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      srcStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
      dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    vkCmdPipelineBarrier(
        commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
  }
}

void UnitCleanupCascadeShadowMapResources(
    VkDevice device, UnitCascadeShadowMapResources& resources)
{
  if (resources.shadowMapSampler != VK_NULL_HANDLE)
  {
    vkDestroySampler(device, resources.shadowMapSampler, nullptr);
  }

  if (resources.shadowMapRenderPass != VK_NULL_HANDLE)
  {
    vkDestroyRenderPass(device, resources.shadowMapRenderPass, nullptr);
  }

  for (auto& cascade : resources.shadowMapCascades)
  {
    if (cascade.framebuffer != VK_NULL_HANDLE)
    {
      vkDestroyFramebuffer(device, cascade.framebuffer, nullptr);
    }
    if (cascade.view != VK_NULL_HANDLE)
    {
      vkDestroyImageView(device, cascade.view, nullptr);
    }
    if (cascade.image != VK_NULL_HANDLE)
    {
      vkDestroyImage(device, cascade.image, nullptr);
    }
    if (cascade.memory != VK_NULL_HANDLE)
    {
      vkFreeMemory(device, cascade.memory, nullptr);
    }
  }
  resources.shadowMapCascades.clear();
}

} // namespace Rl::Client::Render
