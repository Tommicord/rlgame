import Rl.Base.Game;
import Rl.Base.Shader;
import Rl.Base.Binding;
import Rl.World.ServiceLocator;
import Rl.World.Unit.UnitRegistryGPU;
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
import Rl.Client.Render.Unit.UnitRendererMeshGen;
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
import Rl.Player.PlayerCamera;

namespace Rl::Providers
{

void UnitStateDrawable::EnableUnitArrayMode(UnitStateBinding& vk, Main::MainBinding& context,
    const std::vector<Client::Render::UnitRenderUnitData>& unitData,
    const std::vector<Client::Render::UnitRenderPolFence>& fenceData)
{
  // Create unit array buffer
  Client::Render::UnitCreateUnitArrayBuffer(context.device, context.physicalDevice,
      unitData, vk.unitArrayBuffer, vk.unitArrayMemory);

  // Create polygon fence array buffer
  Client::Render::UnitCreatePolFenceArrayBuffer(context.device, context.physicalDevice,
      fenceData, vk.polFenceArrayBuffer, vk.polFenceArrayMemory);

  // Update descriptor set with unit arrays
  Client::Render::UnitUpdateGraphicsDescriptorSetWithUnitArrays(context.device,
      vk.descriptorSet, vk.unitArrayBuffer, vk.polFenceArrayBuffer);

  vk.useUnitArrayMode = true;
}

void UnitStateDrawable::DisableUnitArrayMode(UnitStateBinding& vk, Main::MainBinding& context)
{
  // Cleanup unit array buffers
  if (vk.unitArrayBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(context.device, vk.unitArrayBuffer, nullptr);
    vkFreeMemory(context.device, vk.unitArrayMemory, nullptr);
    vk.unitArrayBuffer = VK_NULL_HANDLE;
    vk.unitArrayMemory = VK_NULL_HANDLE;
  }

  if (vk.polFenceArrayBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(context.device, vk.polFenceArrayBuffer, nullptr);
    vkFreeMemory(context.device, vk.polFenceArrayMemory, nullptr);
    vk.polFenceArrayBuffer = VK_NULL_HANDLE;
    vk.polFenceArrayMemory = VK_NULL_HANDLE;
  }

  vk.useUnitArrayMode = false;
}

void UnitStateDrawable::EnableSingleUnitMode(UnitStateBinding& vk, uint32_t unitId)
{
  vk.singleUnitMode = true;
  vk.singleUnitId = unitId;
}

void UnitStateDrawable::DisableSingleUnitMode(UnitStateBinding& vk)
{
  vk.singleUnitMode = false;
  vk.singleUnitId = 0;
}

void UnitStateDrawable::EnableUnitArrayModeFromRegistry(UnitStateBinding& vk, Main::MainBinding& context,
    const World::UnitRegistryGPU& unitRegistry)
{
  if (!unitRegistry.IsInitialized())
  {
    return;
  }

  // Get CPU unit data from registry
  const auto& gpuUnits = unitRegistry.GetCPUUnits();
  
  // Convert UnitGPUParams to UnitRenderUnitData
  std::vector<Client::Render::UnitRenderUnitData> unitData;
  unitData.reserve(gpuUnits.size());
  
  for (const auto& gpuUnit : gpuUnits)
  {
    unitData.push_back(Client::Render::ConvertGPUParamsToRenderData(gpuUnit));
  }

  // Create default polygon fence data (all zeros for now)
  // In a full implementation, this would come from unit instances or biome data
  std::vector<Client::Render::UnitRenderPolFence> fenceData(gpuUnits.size());
  for (auto& fence : fenceData)
  {
    fence.t = 0.0f;
    fence.d = 0.0f;
    fence.b = 0.0f;
    fence.f = 0.0f;
  }

  // Enable unit array mode with the converted data
  EnableUnitArrayMode(vk, context, unitData, fenceData);
}

void UnitStateDrawable::GenerateUnitMesh(UnitStateBinding& vk, Main::MainBinding& context, uint32_t unitId, uint32_t startVertex)
{
  if (vk.meshGenPipeline == VK_NULL_HANDLE)
  {
    // Initialize mesh generation pipeline on first use
    Client::Render::UnitCreateMeshGenDescriptorSetLayout(context.device, vk.meshGenDescriptorSetLayout);
    Client::Render::UnitCreateMeshGenPipeline(context.device, vk.meshGenDescriptorSetLayout,
        vk.meshGenPipelineLayout, vk.meshGenPipeline);
    
    // Create descriptor pool for mesh generation
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSize.descriptorCount = 1;
    
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1;
    
    if (vkCreateDescriptorPool(context.device, &poolInfo, nullptr, &vk.meshGenDescriptorPool) != VK_SUCCESS)
    {
      return;
    }
    
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = vk.meshGenDescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &vk.meshGenDescriptorSetLayout;
    
    if (vkAllocateDescriptorSets(context.device, &allocInfo, &vk.meshGenDescriptorSet) != VK_SUCCESS)
    {
      return;
    }
    
    // Update descriptor set with vertex buffer
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = vk.vertexBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = VK_WHOLE_SIZE;
    
    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = vk.meshGenDescriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;
    
    vkUpdateDescriptorSets(context.device, 1, &descriptorWrite, 0, nullptr);
  }
  
  // Generate mesh using compute shader
  Client::Render::UnitGenerateMesh(context.device, context.commandBuffers[0],
      vk.meshGenPipeline, vk.meshGenPipelineLayout, vk.meshGenDescriptorSet, unitId, startVertex);
}

void UnitStateDrawable::OnCreate(
    UnitStateResource& resource, UnitStateBinding& vk, Main::MainBinding& context)
{
  const auto& unitVertices = Client::Render::UnitGetTestVertices();
  Client::Render::UnitCreateVertexBuffer(context.device, context.physicalDevice,
      unitVertices, vk.vertexBuffer, vk.vertexBufferMemory);

  // Create index buffer for indexed drawing
  std::vector<uint32_t> unitIndices =
      Client::Render::UnitGenerateIndices(4, 6); // 4 vertices per face, 6 faces
  Client::Render::UnitCreateIndexBuffer(context.device, context.physicalDevice,
      unitIndices, vk.indexBuffer, vk.indexBufferMemory);

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
  Client::Render::UnitCreateGraphicsDescriptorSetLayout(
      context.device, vk.descriptorSetLayout);

  // Create descriptor pool
  Client::Render::UnitCreateDescriptorPool(context.device, vk.descriptorPool);

  // Allocate descriptor sets
  Client::Render::UnitAllocateComputeDescriptorSet(context.device, vk.descriptorPool,
      vk.computeDescriptorSetLayout, vk.computeDescriptorSet);
  Client::Render::UnitAllocateGraphicsDescriptorSet(
      context.device, vk.descriptorPool, vk.descriptorSetLayout, vk.descriptorSet);

  // Create compute pipeline layout and pipeline
  Client::Render::UnitCreateComputePipelineLayout(
      context.device, vk.computeDescriptorSetLayout, vk.pipelineLayout);
  Client::Render::UnitCreateComputePipeline(
      context.device, vk.pipelineLayout, vk.computePipeline);

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
  Client::Render::UnitAllocateCurvatureComputeDescriptorSet(context.device,
      vk.descriptorPool, vk.curveComputeDescriptorSetLayout,
      vk.curveComputeDescriptorSet);

  // Update curvature compute descriptor set
  Client::Render::UnitUpdateCurvatureComputeDescriptorSet(context.device,
      vk.curveComputeDescriptorSet, vk.vertexBuffer, vk.indexBuffer,
      vk.curvedVertexBuffer, vk.curvedIndexBuffer, vk.curveCountersBuffer,
      vk.curveIndirectDrawBuffer,
      sizeof(Client::Render::UnitRenderVertex) * unitVertices.size(), unitIndices.size());

  // Create placeholder resources
  Client::Render::UnitCreatePlaceholderLightingTexture(context.device,
      context.physicalDevice, vk.placeholderLightingTexture,
      vk.placeholderLightingBufferMemory, vk.placeholderLightingTextureView,
      vk.placeholderLightingSampler);
  Client::Render::UnitCreatePlaceholderAOTexture(context.device, context.physicalDevice,
      vk.placeholderAOTexture, vk.placeholderAOTextureMemory, vk.placeholderAOTextureView,
      vk.placeholderAOSampler);
  Client::Render::UnitCreatePlaceholderNormalTexture(context.device,
      context.physicalDevice, vk.placeholderNormalTexture,
      vk.placeholderNormalTextureMemory, vk.placeholderNormalTextureView,
      vk.placeholderNormalSampler);
  Client::Render::UnitCreatePlaceholderSettingsBuffer(context.device,
      context.physicalDevice, vk.placeholderSettingsBuffer,
      vk.placeholderSettingsBufferMemory);
  Client::Render::UnitCreatePlaceholderLightingBuffer(context.device,
      context.physicalDevice, vk.placeholderLightingBuffer,
      vk.placeholderLightingBufferMemory);
  Client::Render::UnitCreateTriplanarSettingsBuffer(context.device,
      context.physicalDevice, vk.triplanarSettingsBuffer,
      vk.triplanarSettingsBufferMemory);

  // Create shadow map resources
  Client::Render::UnitCascadeShadowMapConfig    shadowConfig{};
  Client::Render::UnitCascadeShadowMapResources shadowResources{};
  Client::Render::UnitCreateCascadeShadowMapResources(
      context.device, context.physicalDevice, shadowConfig, shadowResources);

  Client::Render::UnitCreateBuffer(context.device, context.physicalDevice,
      sizeof(Client::Render::UnitCascadeShadowLightingUniforms),
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      vk.shadowCascadeMatricesBuffer, vk.shadowCascadeMatricesMemory);

  vk.shadowMapCascades = shadowResources.shadowMapCascades;
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
  Client::Render::UnitUpdateGraphicsDescriptorSetWithPlaceholders(context.device,
      vk.descriptorSet, vk.placeholderLightingBuffer, vk.placeholderLightingTextureView,
      vk.placeholderLightingSampler, vk.placeholderSettingsBuffer,
      vk.placeholderAOTextureView, vk.placeholderAOSampler,
      vk.placeholderNormalTextureView, vk.placeholderNormalSampler,
      vk.triplanarSettingsBuffer, sizeof(Client::Render::UnitRenderLightingUniforms));

  // Update graphics descriptor set with shadow map
  Client::Render::UnitUpdateGraphicsDescriptorSetWithShadowMap(context.device,
      vk.descriptorSet, vk.shadowMapSampler, vk.shadowMapCascades,
      vk.shadowCascadeMatricesBuffer);

  // Create vertex input state
  auto vertexInputBinding = Client::Render::UnitCreateVertexInputBindingDescription();
  auto vertexInputAttributes = Client::Render::UnitCreateVertexAttributeDescriptions();

  Client::Render::UnitCreateGraphicsPipelineLayout(
      context.device, vk.descriptorSetLayout, vk.pipelineLayout);
  Client::Render::UnitCreateGraphicsPipeline(context.device, vk.pipelineLayout,
      context.renderPass, context.swapChainExtent, vertexInputBinding,
      vertexInputAttributes, vk.pipeline);
}

void UnitStateDrawable::OnUpdate(
    UnitStateResource& resource, UnitStateBinding& vk, Main::MainBinding& context)
{
  // Visible count reset is now handled in OnDrawCompute using vkCmdFillBuffer (GPU-side)
  const Player::IPlayerCamera&            camera = *resource.player.camera;
  Client::Render::UnitRenderFrustumPlanes frustum{};
  Client::Render::UnitCameraToFrustumPlanes(frustum, camera);

  // Update graphics descriptor set with textures from unit (only if textures changed)
  Client::Render::UnitUpdateUnitTextures(
      context.device, vk.descriptorSet, resource.unit.GetMaterial(), context);
#pragma push_macro("near")

#pragma push_macro("far")

#undef near

#undef far
  const auto skyboxSun =
      World::WorldServiceLocator::GetSkyboxSystem()->GetSunProperties();
  Client::Render::UnitUpdateCascadeShadowMaps(camera.GetViewMatrix(),
      camera.GetProjectionMatrix(), skyboxSun.direction, camera.aspectRatio, camera.near,
      camera.far, static_cast<uint32_t>(vk.shadowMapCascades.size()),
      vk.shadowCascadeLighting, vk.shadowCascadeSplits, vk.shadowCascadeCount);
#pragma pop_macro("near")

#pragma pop_macro("far")
  Client::Render::UnitCopyDataToBuffer(context.device, vk.shadowCascadeMatricesMemory, 0,
      sizeof(Client::Render::UnitCascadeShadowLightingUniforms),
      &vk.shadowCascadeLighting);

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
    UnitStateResource& resource, UnitStateBinding& vk, Main::MainBinding& context)
{ Client::Render::UnitRender(resource, vk, context); }

void UnitStateDrawable::OnDrawCompute(
    UnitStateResource& resource, UnitStateBinding& vk, Main::MainBinding& context)
{
  Client::Render::UnitDispatchComputeShaders(resource, vk, context);

  // Although this has nothing to do with a
  // compute shader, we need to render the
  // shadows outside the render pass.
  Client::Render::UnitRenderCascadeShadowMap(resource, vk, context);
}

void UnitStateDrawable::OnDestroy(
    UnitStateResource& resource, UnitStateBinding& vk, Main::MainBinding& context)
{ Client::Render::UnitCleanupResources(context.device, vk); }

void UnitStateDrawable::OnPause()
{
}

void UnitStateDrawable::OnResume()
{
}

} // namespace Rl::Providers
