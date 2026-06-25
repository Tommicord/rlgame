#pragma once

#include <vulkan/vulkan.hpp>

namespace Rl::Client::Render
{

// Shadow map configuration
struct UnitShadowMapConfig
{
  uint32_t width     = 1024;
  uint32_t height    = 1024;
  float    nearPlane = 0.1f;
  float    farPlane  = 100.0f;
  float    orthoSize = 20.0f;
};

// Shadow map resources
struct UnitShadowMapResources
{
  VkImage        shadowMapImage       = VK_NULL_HANDLE;
  VkDeviceMemory shadowMapMemory      = VK_NULL_HANDLE;
  VkImageView    shadowMapView        = VK_NULL_HANDLE;
  VkSampler      shadowMapSampler     = VK_NULL_HANDLE;
  VkFramebuffer  shadowMapFramebuffer = VK_NULL_HANDLE;
  VkRenderPass   shadowMapRenderPass  = VK_NULL_HANDLE;
};

// Create shadow map resources
void UnitCreateShadowMapResources(VkDevice device,
    VkPhysicalDevice                       physicalDevice,
    UnitShadowMapConfig                    config,
    UnitShadowMapResources&                resources);

// Create shadow map render pass
void UnitCreateShadowMapRenderPass(VkDevice device, VkFormat depthFormat, VkRenderPass& renderPass);

// Create shadow map framebuffer
void UnitCreateShadowMapFramebuffer(VkDevice device,
    VkImageView                              depthImageView,
    VkRenderPass                             renderPass,
    uint32_t                                 width,
    uint32_t                                 height,
    VkFramebuffer&                           framebuffer);

// Create shadow map sampler with PCF support
void UnitCreateShadowMapSampler(VkDevice device, VkSampler& sampler);

// Begin shadow map rendering
void UnitBeginShadowMapRenderPass(VkCommandBuffer commandBuffer,
    VkRenderPass                                  renderPass,
    VkFramebuffer                                 framebuffer,
    uint32_t                                      width,
    uint32_t                                      height);

// End shadow map rendering
void UnitEndShadowMapRenderPass(VkCommandBuffer commandBuffer);

// Cleanup shadow map resources
void UnitCleanupShadowMapResources(VkDevice device, UnitShadowMapResources& resources);

} // namespace Rl::Client::Render
