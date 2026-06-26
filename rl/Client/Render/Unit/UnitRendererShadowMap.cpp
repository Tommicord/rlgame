import Rl.Client.Render.Unit.UnitRendererShadowMap;
import Rl.Client.Render.Unit.UnitRendererBasicBuffer;

import <vulkan/vulkan.h>;
import <stdexcept>;

namespace Rl::Client::Render
{

void UnitCreateShadowMapRenderPass(VkDevice device, VkFormat depthFormat, VkRenderPass& renderPass)
{
  VkAttachmentDescription depthAttachment{};
  depthAttachment.format = depthFormat;
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

  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dependency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
  dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &depthAttachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create shadow map render pass");
  }
}

void UnitCreateShadowMapFramebuffer(VkDevice device,
    VkImageView                              depthImageView,
    VkRenderPass                             renderPass,
    uint32_t                                 width,
    uint32_t                                 height,
    VkFramebuffer&                           framebuffer)
{
  VkFramebufferCreateInfo framebufferInfo{};
  framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  framebufferInfo.renderPass = renderPass;
  framebufferInfo.attachmentCount = 1;
  framebufferInfo.pAttachments = &depthImageView;
  framebufferInfo.width = width;
  framebufferInfo.height = height;
  framebufferInfo.layers = 1;

  if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create shadow map framebuffer");
  }
}

void UnitCreateShadowMapSampler(VkDevice device, VkSampler& sampler)
{
  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
  samplerInfo.anisotropyEnable = VK_FALSE;
  samplerInfo.maxAnisotropy = 1.0f;
  samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_TRUE;
  samplerInfo.compareOp = VK_COMPARE_OP_LESS;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = 1.0f;

  if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create shadow map sampler");
  }
}

void UnitCreateShadowMapResources(VkDevice device,
    VkPhysicalDevice                       physicalDevice,
    UnitShadowMapConfig                    config,
    UnitShadowMapResources&                resources)
{
  // Create depth image for shadow map
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
  imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateImage(device, &imageInfo, nullptr, &resources.shadowMapImage) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create shadow map image");
  }

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(device, resources.shadowMapImage, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex =
      UnitFindMemoryTypeIndex(physicalDevice, memRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  if (vkAllocateMemory(device, &allocInfo, nullptr, &resources.shadowMapMemory) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to allocate shadow map memory");
  }

  vkBindImageMemory(device, resources.shadowMapImage, resources.shadowMapMemory, 0);

  // Create image view
  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = resources.shadowMapImage;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = VK_FORMAT_D32_SFLOAT;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  if (vkCreateImageView(device, &viewInfo, nullptr, &resources.shadowMapView) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create shadow map image view");
  }

  // Create render pass
  UnitCreateShadowMapRenderPass(device, VK_FORMAT_D32_SFLOAT, resources.shadowMapRenderPass);

  // Create framebuffer
  UnitCreateShadowMapFramebuffer(device, resources.shadowMapView, resources.shadowMapRenderPass,
      config.width, config.height, resources.shadowMapFramebuffer);

  // Create sampler with PCF support
  UnitCreateShadowMapSampler(device, resources.shadowMapSampler);
}

void UnitBeginShadowMapRenderPass(VkCommandBuffer commandBuffer,
    VkRenderPass                                  renderPass,
    VkFramebuffer                                 framebuffer,
    uint32_t                                      width,
    uint32_t                                      height)
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
}

void UnitEndShadowMapRenderPass(VkCommandBuffer commandBuffer)
{
  vkCmdEndRenderPass(commandBuffer);
}

void UnitCleanupShadowMapResources(VkDevice device, UnitShadowMapResources& resources)
{
  if (resources.shadowMapSampler != VK_NULL_HANDLE)
  {
    vkDestroySampler(device, resources.shadowMapSampler, nullptr);
  }
  if (resources.shadowMapFramebuffer != VK_NULL_HANDLE)
  {
    vkDestroyFramebuffer(device, resources.shadowMapFramebuffer, nullptr);
  }
  if (resources.shadowMapRenderPass != VK_NULL_HANDLE)
  {
    vkDestroyRenderPass(device, resources.shadowMapRenderPass, nullptr);
  }
  if (resources.shadowMapView != VK_NULL_HANDLE)
  {
    vkDestroyImageView(device, resources.shadowMapView, nullptr);
  }
  if (resources.shadowMapImage != VK_NULL_HANDLE)
  {
    vkDestroyImage(device, resources.shadowMapImage, nullptr);
  }
  if (resources.shadowMapMemory != VK_NULL_HANDLE)
  {
    vkFreeMemory(device, resources.shadowMapMemory, nullptr);
  }
}

} // namespace Rl::Client::Render
