export module Rl.Client.Render.Unit.UnitRendererShadowMap;

import <vulkan/vulkan.hpp>;
import <glm/glm.hpp>;

namespace Rl::Client::Render
{

export struct UnitCascadeShadowLevel
{
  VkImage        image = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;
  VkImageView    view = VK_NULL_HANDLE;
  VkFramebuffer  framebuffer = VK_NULL_HANDLE;
};

export struct UnitCascadeShadowLightingUniforms
{
  glm::mat4 lightSpaceMatrices[4];
};

export struct UnitCascadeShadowMapConfig
{
  uint32_t numCascades = 4;
  uint32_t width = 2048;
  uint32_t height = 2048;
};

export struct UnitCascadeShadowMapResources
{
  std::vector<UnitCascadeShadowLevel> shadowMapCascades{};
  VkSampler                           shadowMapSampler = VK_NULL_HANDLE;
  VkFramebuffer                       shadowMapFramebuffer = VK_NULL_HANDLE;
  VkRenderPass                        shadowMapRenderPass = VK_NULL_HANDLE;
};

export void UnitCreateCascadeShadowMapResources(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    UnitCascadeShadowMapConfig config,
    UnitCascadeShadowMapResources& resources);

export void UnitBeginCascadeShadowMapRenderPass(
    VkCommandBuffer commandBuffer,
    VkRenderPass renderPass,
    VkFramebuffer framebuffer,
    uint32_t width,
    uint32_t height);

export void UnitEndCascadeShadowMapRenderPass(VkCommandBuffer commandBuffer);

export void UnitTransitionCascadeShadowMapLayout(
    VkCommandBuffer commandBuffer,
    const UnitCascadeShadowMapResources& resources,
    VkImageLayout oldLayout,
    VkImageLayout newLayout);

export void UnitCleanupCascadeShadowMapResources(
    VkDevice device,UnitCascadeShadowMapResources& resources);

} // namespace Rl::Client::Render
