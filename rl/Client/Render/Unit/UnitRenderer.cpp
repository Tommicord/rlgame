#include "rl/Base/Game.h"
#include "rl/Base/ShaderFactory.h"
#include "rl/Client/Render/Unit/UnitRendererAOTextures.h"
#include "rl/Client/Render/Unit/UnitRendererBasicBuffer.h"
#include "rl/Client/Render/Unit/UnitRendererCleanup.h"
#include "rl/Client/Render/Unit/UnitRendererComputePipeline.h"
#include "rl/Client/Render/Unit/UnitRendererCurvatureCompute.h"
#include "rl/Client/Render/Unit/UnitRendererDescriptorSets.h"
#include "rl/Client/Render/Unit/UnitRendererDraw.h"
#include "rl/Client/Render/Unit/UnitRendererDrawCompute.h"
#include "rl/Client/Render/Unit/UnitRendererFrustum.h"
#include "rl/Client/Render/Unit/UnitRendererGraphicsPipeline.h"
#include "rl/Client/Render/Unit/UnitRendererNormalTextures.h"
#include "rl/Client/Render/Unit/UnitRendererPlaceholderResource.h"
#include "rl/Client/Render/Unit/UnitRendererSampler.h"
#include "rl/Client/Render/Unit/UnitRendererShadowMap.h"
#include "rl/Client/Render/Unit/UnitRendererTextureManage.h"
#include "rl/Client/Render/Unit/UnitRendererVertexInput.h"
#include "rl/Client/Render/Unit/UnitRendererVertices.h"
#include "rl/Client/State/UnitState.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan.hpp>

namespace Rl::Providers
{

void UnitStateDrawable::OnCreate(
    UnitStateResource& resource, UnitStateDrawableVulkan& vk, Game::VulkanContext& context)
{
  const auto& unitVertices = Client::Render::UnitGetTestVertices();
  Client::Render::UnitCreateVertexBuffer(
      context.device, context.physicalDevice, unitVertices, vk.vertexBuffer, vk.vertexBufferMemory);

  // Create index buffer for indexed drawing
  std::vector<uint32_t> unitIndices =
      Client::Render::UnitGenerateIndices(4, 6); // 4 vertices per face, 6 faces
  Client::Render::UnitCreateIndexBuffer(
      context.device, context.physicalDevice, unitIndices, vk.indexBuffer, vk.indexBufferMemory);

  // Create SSBO buffers
  Client::Render::UnitCreateSSBOBuffers(
      context.device, context.physicalDevice, unitVertices.size(), vk);

  // Create curvature compute shader buffers
  Client::Render::UnitCreateCurvatureComputeBuffers(
      context.device, context.physicalDevice, unitVertices.size(), vk);

  // Create uniform buffers
  Client::Render::UnitCreateUniformBuffers(context.device, context.physicalDevice, vk);

  // Create descriptor set layouts
  Client::Render::UnitCreateComputeDescriptorSetLayout(
      context.device, vk.computeDescriptorSetLayout);
  Client::Render::UnitCreateGraphicsDescriptorSetLayout(context.device, vk.descriptorSetLayout);

  // Create descriptor pool
  Client::Render::UnitCreateDescriptorPool(context.device, vk.descriptorPool);

  // Allocate descriptor sets
  Client::Render::UnitAllocateComputeDescriptorSet(
      context.device, vk.descriptorPool, vk.computeDescriptorSetLayout, vk.computeDescriptorSet);
  Client::Render::UnitAllocateGraphicsDescriptorSet(
      context.device, vk.descriptorPool, vk.descriptorSetLayout, vk.descriptorSet);

  // Create compute pipeline layout and pipeline
  Client::Render::UnitCreateComputePipelineLayout(
      context.device, vk.computeDescriptorSetLayout, vk.pipelineLayout);
  Client::Render::UnitCreateComputePipeline(context.device, vk.pipelineLayout, vk.computePipeline);

  // Update compute descriptor set
  Client::Render::UnitUpdateComputeDescriptorSet(context.device, vk.computeDescriptorSet,
      vk.vertexBuffer, vk.indexBuffer, vk.outputIndexBuffer, vk.visibleCountBuffer,
      vk.indirectDrawBuffer, vk.frustumBuffer, sizeof(Client::Render::UnitRenderVertex) * unitVertices.size());

  Client::Render::UnitCreateCurvatureComputeDescriptorSetLayout(
      context.device, vk.curveComputeDescriptorSetLayout);
  Client::Render::UnitCreateCurvatureComputePipelineLayout(
      context.device, vk.curveComputeDescriptorSetLayout, vk.curveComputePipelineLayout);
  Client::Render::UnitCreateCurvatureComputePipeline(
      context.device, vk.curveComputePipelineLayout, vk.curveComputePipeline);

  // Allocate curvature compute descriptor set
  Client::Render::UnitAllocateCurvatureComputeDescriptorSet(context.device, vk.descriptorPool,
      vk.curveComputeDescriptorSetLayout, vk.curveComputeDescriptorSet);

  // Update curvature compute descriptor set
  Client::Render::UnitUpdateCurvatureComputeDescriptorSet(context.device,
      vk.curveComputeDescriptorSet, vk.vertexBuffer, vk.indexBuffer, vk.curvedVertexBuffer,
      vk.curvedIndexBuffer, vk.curveCountersBuffer, vk.curveIndirectDrawBuffer, sizeof(Client::Render::UnitRenderVertex) * unitVertices.size(),
      unitIndices.size());

  // Create placeholder resources
  Client::Render::UnitCreatePlaceholderLightingTexture(context.device, context.physicalDevice,
      vk.placeholderLightingTexture, vk.placeholderLightingBufferMemory,
      vk.placeholderLightingTextureView, vk.placeholderLightingSampler);
  Client::Render::UnitCreatePlaceholderAOTexture(context.device, context.physicalDevice,
      vk.placeholderAOTexture, vk.placeholderAOTextureMemory, vk.placeholderAOTextureView,
      vk.placeholderAOSampler);
  Client::Render::UnitCreatePlaceholderNormalTexture(context.device, context.physicalDevice,
      vk.placeholderNormalTexture, vk.placeholderNormalTextureMemory, vk.placeholderNormalTextureView,
      vk.placeholderNormalSampler);
  Client::Render::UnitCreatePlaceholderSettingsBuffer(context.device, context.physicalDevice,
      vk.placeholderSettingsBuffer, vk.placeholderSettingsBufferMemory);
  Client::Render::UnitCreatePlaceholderLightingBuffer(context.device, context.physicalDevice,
      vk.placeholderLightingBuffer, vk.placeholderLightingBufferMemory);
  Client::Render::UnitCreateTriplanarSettingsBuffer(context.device, context.physicalDevice,
      vk.triplanarSettingsBuffer, vk.triplanarSettingsBufferMemory);

  // Create shadow map resources
  Client::Render::UnitShadowMapConfig shadowConfig{};
  Client::Render::UnitShadowMapResources shadowResources{};
  Client::Render::UnitCreateShadowMapResources(context.device, context.physicalDevice,
      shadowConfig, shadowResources);

  // Assign shadow map resources to vk struct
  vk.shadowMapImage = shadowResources.shadowMapImage;
  vk.shadowMapMemory = shadowResources.shadowMapMemory;
  vk.shadowMapView = shadowResources.shadowMapView;
  vk.shadowMapSampler = shadowResources.shadowMapSampler;
  vk.shadowMapFramebuffer = shadowResources.shadowMapFramebuffer;
  vk.shadowMapRenderPass = shadowResources.shadowMapRenderPass;

  // Create global texture sampler
  Client::Render::UnitCreateGlobalTextureSampler(context.device, vk.globalTextureSampler);

  // Update graphics descriptor set with placeholder resources
  Client::Render::UnitUpdateGraphicsDescriptorSetWithPlaceholders(context.device, vk.descriptorSet,
      vk.placeholderLightingBuffer, vk.placeholderLightingTextureView,
      vk.placeholderLightingSampler, vk.placeholderSettingsBuffer, vk.placeholderAOTextureView,
      vk.placeholderAOSampler, vk.placeholderNormalTextureView, vk.placeholderNormalSampler,
      vk.triplanarSettingsBuffer, sizeof(Client::Render::UnitRenderLightingUniforms));

  // Update graphics descriptor set with shadow map
  Client::Render::UnitUpdateGraphicsDescriptorSetWithShadowMap(context.device, vk.descriptorSet,
      vk.shadowMapView, vk.shadowMapSampler);

  // Create vertex input state
  auto vertexInputBinding    = Client::Render::UnitCreateVertexInputBindingDescription();
  auto vertexInputAttributes = Client::Render::UnitCreateVertexAttributeDescriptions();

  Client::Render::UnitCreateGraphicsPipelineLayout(
      context.device, vk.descriptorSetLayout, vk.pipelineLayout);
  Client::Render::UnitCreateGraphicsPipeline(context.device, vk.pipelineLayout, context.renderPass,
      context.swapChainExtent, vertexInputBinding, vertexInputAttributes, vk.pipeline);
}

void UnitStateDrawable::OnUpdate(
    UnitStateResource& resource, UnitStateDrawableVulkan& vk, Game::VulkanContext& context)
{
  if (!resource.cameraModel)
    return;

  // Visible count reset is now handled in OnDrawCompute using vkCmdFillBuffer (GPU-side)
  Client::Render::UnitRenderFrustumPlanes frustum{};
  Client::Render::UnitCameraToFrustumPlanes(frustum, resource.cameraModel->GetObject());

  // Update graphics descriptor set with textures from unit (only if textures changed)
  if (resource.unit)
  {
    Client::Render::UnitUpdateUnitTextures(
        context.device, vk.descriptorSet, resource.unit->GetMaterial(), context);

    // Generate AO textures from unit textures (only if not already generated)
    if (vk.aoTexturesView[0] == VK_NULL_HANDLE)
    {
      Client::Render::UnitGenerateAOTextures(
          context.device, context, vk, resource.unit->GetMaterial());
      Client::Render::UnitUpdateAOTextureDescriptor(
          context.device, vk.descriptorSet, vk.aoTexturesView, vk.globalTextureSampler);
    }

    // Generate normal textures from unit textures (only if not already generated)
    if (vk.normalTexturesView[0] == VK_NULL_HANDLE)
    {
      Client::Render::UnitGenerateNormalTextures(
          context.device, context, vk, resource.unit->GetMaterial());
      Client::Render::UnitUpdateNormalTextureDescriptor(
          context.device, vk.descriptorSet, vk.normalTexturesView, vk.globalTextureSampler);
    }
  }
}

void UnitStateDrawable::OnDraw(
    UnitStateResource& resource, UnitStateDrawableVulkan& vk, Game::VulkanContext& context)
{
  if (!resource.cameraModel)
    return;
  Client::Render::UnitRender(resource, vk, context);
}

void UnitStateDrawable::OnDrawCompute(
    UnitStateResource& resource, UnitStateDrawableVulkan& vk, Game::VulkanContext& context)
{
  if (!resource.cameraModel)
    return;
  Client::Render::UnitDispatchComputeShaders(resource, vk, context);
}

void UnitStateDrawable::OnDestroy(
    UnitStateResource& resource, UnitStateDrawableVulkan& vk, Game::VulkanContext& context)
{
  Client::Render::UnitCleanupResources(context.device, vk);
}

void UnitStateDrawable::OnPause()
{
}

void UnitStateDrawable::OnResume()
{
}

} // namespace Rl::Providers
