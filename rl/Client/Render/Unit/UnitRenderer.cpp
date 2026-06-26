import Rl.Base.Game;
import Rl.Base.Shader;
import Rl.Base.Binding;
import Rl.Client.Render.Unit.UnitRendererInfo;
import Rl.Client.Render.Unit.UnitRendererAOTextures;
import Rl.Client.Render.Unit.UnitRendererBasicBuffer;
import Rl.Client.Render.Unit.UnitRendererCleanup;
import Rl.Client.Render.Unit.UnitRendererComputePipeline;
import Rl.Client.Render.Unit.UnitRendererCurvatureCompute;
import Rl.Client.Render.Unit.UnitRendererDescriptorSets;
import Rl.Client.Render.Unit.UnitRendererDraw;
import Rl.Client.Render.Unit.UnitRendererDrawCompute;
import Rl.Client.Render.Unit.UnitRendererFrustum;
import Rl.Client.Render.Unit.UnitRendererGraphicsPipeline;
import Rl.Client.Render.Unit.UnitRendererNormalTextures;
import Rl.Client.Render.Unit.UnitRendererPlaceholderResource;
import Rl.Client.Render.Unit.UnitRendererSampler;
import Rl.Client.Render.Unit.UnitRendererShadowMap;
import Rl.Client.Render.Unit.UnitRendererShadowPipeline;
import Rl.Client.Render.Unit.UnitRendererTextureManage;
import Rl.Client.Render.Unit.UnitRendererVertexInput;
import Rl.Client.Render.Unit.UnitRendererVertices;
import Rl.Client.State.UnitState;

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
import <glm/glm.hpp>;
import <glm/gtc/matrix_transform.hpp>;
import <vulkan/vulkan.hpp>;

namespace Rl::Providers
{

void UnitStateDrawable::OnCreate(
    UnitStateResource& resource, UnitStateBinding& vk, Game::MainBinding& context)
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
      vk.indirectDrawBuffer, vk.frustumBuffer,
      sizeof(Client::Render::UnitRenderVertex) * unitVertices.size());

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
      vk.curvedIndexBuffer, vk.curveCountersBuffer, vk.curveIndirectDrawBuffer,
      sizeof(Client::Render::UnitRenderVertex) * unitVertices.size(), unitIndices.size());

  // Create placeholder resources
  Client::Render::UnitCreatePlaceholderLightingTexture(context.device, context.physicalDevice,
      vk.placeholderLightingTexture, vk.placeholderLightingBufferMemory,
      vk.placeholderLightingTextureView, vk.placeholderLightingSampler);
  Client::Render::UnitCreatePlaceholderAOTexture(context.device, context.physicalDevice,
      vk.placeholderAOTexture, vk.placeholderAOTextureMemory, vk.placeholderAOTextureView,
      vk.placeholderAOSampler);
  Client::Render::UnitCreatePlaceholderNormalTexture(context.device, context.physicalDevice,
      vk.placeholderNormalTexture, vk.placeholderNormalTextureMemory,
      vk.placeholderNormalTextureView, vk.placeholderNormalSampler);
  Client::Render::UnitCreatePlaceholderSettingsBuffer(context.device, context.physicalDevice,
      vk.placeholderSettingsBuffer, vk.placeholderSettingsBufferMemory);
  Client::Render::UnitCreatePlaceholderLightingBuffer(context.device, context.physicalDevice,
      vk.placeholderLightingBuffer, vk.placeholderLightingBufferMemory);
  Client::Render::UnitCreateTriplanarSettingsBuffer(context.device, context.physicalDevice,
      vk.triplanarSettingsBuffer, vk.triplanarSettingsBufferMemory);

  // Create shadow map resources
  Client::Render::UnitShadowMapConfig    shadowConfig{};
  Client::Render::UnitShadowMapResources shadowResources{};
  Client::Render::UnitCreateShadowMapResources(
      context.device, context.physicalDevice, shadowConfig, shadowResources);

  // Assign shadow map resources to vk struct
  vk.shadowMapImage = shadowResources.shadowMapImage;
  vk.shadowMapMemory = shadowResources.shadowMapMemory;
  vk.shadowMapView = shadowResources.shadowMapView;
  vk.shadowMapSampler = shadowResources.shadowMapSampler;
  vk.shadowMapFramebuffer = shadowResources.shadowMapFramebuffer;
  vk.shadowMapRenderPass = shadowResources.shadowMapRenderPass;

  // Create shadow map pipeline layout and pipeline
  Client::Render::UnitCreateShadowPipelineLayout(context.device, vk.shadowPipelineLayout);
  Client::Render::UnitCreateShadowPipeline(context.device, vk.shadowPipelineLayout,
      vk.shadowMapRenderPass, shadowConfig.width, shadowConfig.height, vk.shadowPipeline);

  // Create global texture sampler
  Client::Render::UnitCreateGlobalTextureSampler(context.device, vk.globalTextureSampler);

  // Update graphics descriptor set with placeholder resources
  Client::Render::UnitUpdateGraphicsDescriptorSetWithPlaceholders(context.device, vk.descriptorSet,
      vk.placeholderLightingBuffer, vk.placeholderLightingTextureView,
      vk.placeholderLightingSampler, vk.placeholderSettingsBuffer, vk.placeholderAOTextureView,
      vk.placeholderAOSampler, vk.placeholderNormalTextureView, vk.placeholderNormalSampler,
      vk.triplanarSettingsBuffer, sizeof(Client::Render::UnitRenderLightingUniforms));

  // Update graphics descriptor set with shadow map
  Client::Render::UnitUpdateGraphicsDescriptorSetWithShadowMap(
      context.device, vk.descriptorSet, vk.shadowMapView, vk.shadowMapSampler);

  // Create vertex input state
  auto vertexInputBinding = Client::Render::UnitCreateVertexInputBindingDescription();
  auto vertexInputAttributes = Client::Render::UnitCreateVertexAttributeDescriptions();

  Client::Render::UnitCreateGraphicsPipelineLayout(
      context.device, vk.descriptorSetLayout, vk.pipelineLayout);
  Client::Render::UnitCreateGraphicsPipeline(context.device, vk.pipelineLayout, context.renderPass,
      context.swapChainExtent, vertexInputBinding, vertexInputAttributes, vk.pipeline);
}

void UnitStateDrawable::OnUpdate(
    UnitStateResource& resource, UnitStateBinding& vk, Game::MainBinding& context)
{
  if (!resource.camera.has_value())
  {
    return;
  }
  // Visible count reset is now handled in OnDrawCompute using vkCmdFillBuffer (GPU-side)
  Client::Render::UnitRenderFrustumPlanes frustum{};
  Client::Render::UnitCameraToFrustumPlanes(frustum, resource.camera.value().GetObject());

  // Update graphics descriptor set with textures from unit (only if textures changed)
  Client::Render::UnitUpdateUnitTextures(
      context.device, vk.descriptorSet, resource.unit.GetMaterial(), context);

  // Generate AO textures from unit textures (only if not already generated)
  if (vk.aoTexturesView[0] == VK_NULL_HANDLE)
  {
    Client::Render::UnitGenerateAOTextures(
        context.device, context, vk, resource.unit.GetMaterial());
    Client::Render::UnitUpdateAOTextureDescriptor(
        context.device, vk.descriptorSet, vk.aoTexturesView, vk.globalTextureSampler);
  }

  // Generate normal textures from unit textures (only if not already generated)
  if (vk.normalTexturesView[0] == VK_NULL_HANDLE)
  {
    Client::Render::UnitGenerateNormalTextures(
        context.device, context, vk, resource.unit.GetMaterial());
    Client::Render::UnitUpdateNormalTextureDescriptor(
        context.device, vk.descriptorSet, vk.normalTexturesView, vk.globalTextureSampler);
  }
}

void UnitStateDrawable::OnDraw(
    UnitStateResource& resource, UnitStateBinding& vk, Game::MainBinding& context)
{
  if (!resource.camera.has_value())
  {
    return;
  }
  Client::Render::UnitRender(resource, vk, context);
}

void UnitStateDrawable::OnDrawCompute(
    UnitStateResource& resource, UnitStateBinding& vk, Game::MainBinding& context)
{
  if (!resource.camera.has_value())
  {
    return;
  }
  Client::Render::UnitDispatchComputeShaders(resource, vk, context);

  // Although this has nothing to do with a
  // compute shader, we need to render the
  // shadows outside the render pass.
  Client::Render::UnitRenderShadowMap(resource, vk, context);
}

void UnitStateDrawable::OnDestroy(
    UnitStateResource& resource, UnitStateBinding& vk, Game::MainBinding& context)
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
