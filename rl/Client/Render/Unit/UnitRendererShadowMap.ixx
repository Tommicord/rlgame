export module Rl.Client.Render.Unit.UnitRendererShadowMap;

import <vulkan/vulkan.hpp>;

namespace Rl::Client::Render
{

export struct UnitShadowMapConfig
{
  uint32_t width = 1024;
  uint32_t height = 1024;
  float    nearPlane = 0.1f;
  float    farPlane = 100.0f;
  float    orthoSize = 20.0f;
};

export struct UnitShadowMapResources
{
  VkImage        shadowMapImage = VK_NULL_HANDLE;
  VkDeviceMemory shadowMapMemory = VK_NULL_HANDLE;
  VkImageView    shadowMapView = VK_NULL_HANDLE;
  VkSampler      shadowMapSampler = VK_NULL_HANDLE;
  VkFramebuffer  shadowMapFramebuffer = VK_NULL_HANDLE;
  VkRenderPass   shadowMapRenderPass = VK_NULL_HANDLE;
};

export void UnitCreateShadowMapResources(VkDevice device,
    VkPhysicalDevice                              physicalDevice,
    UnitShadowMapConfig                           config,
    UnitShadowMapResources&                       resources);

export void UnitCreateShadowMapRenderPass(
    VkDevice device, VkFormat depthFormat, VkRenderPass& renderPass);

export void UnitCreateShadowMapFramebuffer(VkDevice device,
    VkImageView                                     depthImageView,
    VkRenderPass                                    renderPass,
    uint32_t                                        width,
    uint32_t                                        height,
    VkFramebuffer&                                  framebuffer);

export void UnitCreateShadowMapSampler(VkDevice device, VkSampler& sampler);

export void UnitBeginShadowMapRenderPass(VkCommandBuffer commandBuffer,
    VkRenderPass                                         renderPass,
    VkFramebuffer                                        framebuffer,
    uint32_t                                             width,
    uint32_t                                             height);

export void UnitEndShadowMapRenderPass(VkCommandBuffer commandBuffer);

export void UnitCleanupShadowMapResources(VkDevice device, UnitShadowMapResources& resources);

} // namespace Rl::Client::Render
