import Rl.Client.Render.Unit.UnitRendererCleanup;
import Rl.Client.State.UnitState;
import Rl.Base.Binding;

import <vulkan/vulkan.hpp>;

namespace Rl::Client::Render
{

void UnitCleanupResources(VkDevice device, Providers::UnitStateBinding& vk)
{
  UnitCleanupBuffers(device, vk);
  UnitCleanupTextures(device, vk);
  UnitCleanupSamplers(device, vk);
  UnitCleanupPipelines(device, vk);
  UnitCleanupDescriptorSets(device, vk);
}

void UnitCleanupBuffers(VkDevice device, Providers::UnitStateBinding& vk)
{
  if (vk.vertexBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device, vk.vertexBuffer, nullptr);
    vkFreeMemory(device, vk.vertexBufferMemory, nullptr);
  }
  if (vk.indexBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device, vk.indexBuffer, nullptr);
    vkFreeMemory(device, vk.indexBufferMemory, nullptr);
  }
  if (vk.outputIndexBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device, vk.outputIndexBuffer, nullptr);
    vkFreeMemory(device, vk.outputIndexBufferMemory, nullptr);
  }
  if (vk.visibleCountBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device, vk.visibleCountBuffer, nullptr);
    vkFreeMemory(device, vk.visibleCountBufferMemory, nullptr);
  }
  if (vk.indirectDrawBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device, vk.indirectDrawBuffer, nullptr);
    vkFreeMemory(device, vk.indirectDrawBufferMemory, nullptr);
  }
  if (vk.frustumBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device, vk.frustumBuffer, nullptr);
    vkFreeMemory(device, vk.frustumBufferMemory, nullptr);
  }
  if (vk.triplanarSettingsBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device, vk.triplanarSettingsBuffer, nullptr);
    vkFreeMemory(device, vk.triplanarSettingsBufferMemory, nullptr);
  }
  if (vk.placeholderSettingsBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device, vk.placeholderSettingsBuffer, nullptr);
    vkFreeMemory(device, vk.placeholderSettingsBufferMemory, nullptr);
  }
  if (vk.placeholderLightingBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device, vk.placeholderLightingBuffer, nullptr);
    vkFreeMemory(device, vk.placeholderLightingBufferMemory, nullptr);
  }
  if (vk.shadowCascadeMatricesBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device, vk.shadowCascadeMatricesBuffer, nullptr);
    vkFreeMemory(device, vk.shadowCascadeMatricesMemory, nullptr);
  }

  // Cleanup unit array buffers
  if (vk.unitArrayBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device, vk.unitArrayBuffer, nullptr);
    vkFreeMemory(device, vk.unitArrayMemory, nullptr);
  }
  if (vk.polFenceArrayBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device, vk.polFenceArrayBuffer, nullptr);
    vkFreeMemory(device, vk.polFenceArrayMemory, nullptr);
  }

  if (vk.curvedVertexBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device, vk.curvedVertexBuffer, nullptr);
    vkFreeMemory(device, vk.curvedVertexBufferMemory, nullptr);
  }
  if (vk.curvedIndexBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device, vk.curvedIndexBuffer, nullptr);
    vkFreeMemory(device, vk.curvedIndexBufferMemory, nullptr);
  }
  if (vk.curveCountersBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device, vk.curveCountersBuffer, nullptr);
    vkFreeMemory(device, vk.curveCountersBufferMemory, nullptr);
  }
  if (vk.curveIndirectDrawBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device, vk.curveIndirectDrawBuffer, nullptr);
    vkFreeMemory(device, vk.curveIndirectDrawBufferMemory, nullptr);
  }

  if (vk.meshGenPipeline != VK_NULL_HANDLE)
  {
    vkDestroyPipeline(device, vk.meshGenPipeline, nullptr);
  }
  if (vk.meshGenPipelineLayout != VK_NULL_HANDLE)
  {
    vkDestroyPipelineLayout(device, vk.meshGenPipelineLayout, nullptr);
  }
  if (vk.meshGenDescriptorSetLayout != VK_NULL_HANDLE)
  {
    vkDestroyDescriptorSetLayout(device, vk.meshGenDescriptorSetLayout, nullptr);
  }
  if (vk.meshGenDescriptorPool != VK_NULL_HANDLE)
  {
    vkDestroyDescriptorPool(device, vk.meshGenDescriptorPool, nullptr);
  }
}

void UnitCleanupTextures(VkDevice device, Providers::UnitStateBinding& vk)
{
  // Cleanup placeholder textures
  if (vk.placeholderLightingSampler != VK_NULL_HANDLE)
  {
    vkDestroySampler(device, vk.placeholderLightingSampler, nullptr);
  }
  if (vk.placeholderLightingTextureView != VK_NULL_HANDLE)
  {
    vkDestroyImageView(device, vk.placeholderLightingTextureView, nullptr);
  }
  if (vk.placeholderLightingTexture != VK_NULL_HANDLE)
  {
    vkDestroyImage(device, vk.placeholderLightingTexture, nullptr);
  }
  if (vk.placeholderLightingTextureMemory != VK_NULL_HANDLE)
  {
    vkFreeMemory(device, vk.placeholderLightingTextureMemory, nullptr);
  }
  if (vk.placeholderAOSampler != VK_NULL_HANDLE)
  {
    vkDestroySampler(device, vk.placeholderAOSampler, nullptr);
  }
  if (vk.placeholderAOTextureView != VK_NULL_HANDLE)
  {
    vkDestroyImageView(device, vk.placeholderAOTextureView, nullptr);
  }
  if (vk.placeholderAOTexture != VK_NULL_HANDLE)
  {
    vkDestroyImage(device, vk.placeholderAOTexture, nullptr);
  }
  if (vk.placeholderAOTextureMemory != VK_NULL_HANDLE)
  {
    vkFreeMemory(device, vk.placeholderAOTextureMemory, nullptr);
  }

  for (int i = 0; i < 6; ++i)
  {
    if (vk.aoTexturesView[i] != VK_NULL_HANDLE)
    {
      vkDestroyImageView(device, vk.aoTexturesView[i], nullptr);
    }
    if (vk.aoTextures[i] != VK_NULL_HANDLE)
    {
      vkDestroyImage(device, vk.aoTextures[i], nullptr);
    }
    if (vk.aoTexturesMemory[i] != VK_NULL_HANDLE)
    {
      vkFreeMemory(device, vk.aoTexturesMemory[i], nullptr);
    }
  }

  for (int i = 0; i < 6; ++i)
  {
    if (vk.normalTexturesView[i] != VK_NULL_HANDLE)
    {
      vkDestroyImageView(device, vk.normalTexturesView[i], nullptr);
    }
    if (vk.normalTextures[i] != VK_NULL_HANDLE)
    {
      vkDestroyImage(device, vk.normalTextures[i], nullptr);
    }
    if (vk.normalTexturesMemory[i] != VK_NULL_HANDLE)
    {
      vkFreeMemory(device, vk.normalTexturesMemory[i], nullptr);
    }
  }

  if (vk.shadowMapSampler != VK_NULL_HANDLE) {
    vkDestroySampler(device, vk.shadowMapSampler, nullptr);
  }
  if (vk.shadowMapRenderPass != VK_NULL_HANDLE) {
    vkDestroyRenderPass(device, vk.shadowMapRenderPass, nullptr);
  }
  for (auto& cascade : vk.shadowMapCascades) {
    if (cascade.framebuffer != VK_NULL_HANDLE) {
      vkDestroyFramebuffer(device, cascade.framebuffer, nullptr);
    }
    if (cascade.view != VK_NULL_HANDLE) {
      vkDestroyImageView(device, cascade.view, nullptr);
    }
    if (cascade.image != VK_NULL_HANDLE) {
      vkDestroyImage(device, cascade.image, nullptr);
    }
    if (cascade.memory != VK_NULL_HANDLE) {
      vkFreeMemory(device, cascade.memory, nullptr);
    }
  }
  vk.shadowMapCascades.clear();
  if (vk.shadowMapFramebuffer != VK_NULL_HANDLE) {
    vkDestroyFramebuffer(device, vk.shadowMapFramebuffer, nullptr);
  }
}

void UnitCleanupSamplers(VkDevice device, Providers::UnitStateBinding& vk)
{
  if (vk.globalTextureSampler != VK_NULL_HANDLE)
  {
    vkDestroySampler(device, vk.globalTextureSampler, nullptr);
  }
}

void UnitCleanupPipelines(VkDevice device, Providers::UnitStateBinding& vk)
{
  if (vk.computePipeline != VK_NULL_HANDLE)
  {
    vkDestroyPipeline(device, vk.computePipeline, nullptr);
  }
  if (vk.computePipelineLayout != VK_NULL_HANDLE)
  {
    vkDestroyPipelineLayout(device, vk.computePipelineLayout, nullptr);
  }
  if (vk.curveComputePipeline != VK_NULL_HANDLE)
  {
    vkDestroyPipeline(device, vk.curveComputePipeline, nullptr);
  }
  if (vk.curveComputePipelineLayout != VK_NULL_HANDLE)
  {
    vkDestroyPipelineLayout(device, vk.curveComputePipelineLayout, nullptr);
  }
  if (vk.pipeline != VK_NULL_HANDLE)
  {
    vkDestroyPipeline(device, vk.pipeline, nullptr);
  }
  if (vk.pipelineLayout != VK_NULL_HANDLE)
  {
    vkDestroyPipelineLayout(device, vk.pipelineLayout, nullptr);
  }
  if (vk.shadowPipeline != VK_NULL_HANDLE)
  {
    vkDestroyPipeline(device, vk.shadowPipeline, nullptr);
  }
  if (vk.shadowPipelineLayout != VK_NULL_HANDLE)
  {
    vkDestroyPipelineLayout(device, vk.shadowPipelineLayout, nullptr);
  }
}

void UnitCleanupDescriptorSets(VkDevice device, Providers::UnitStateBinding& vk)
{
  if (vk.computeDescriptorSetLayout != VK_NULL_HANDLE)
  {
    vkDestroyDescriptorSetLayout(device, vk.computeDescriptorSetLayout, nullptr);
  }
  if (vk.curveComputeDescriptorSetLayout != VK_NULL_HANDLE)
  {
    vkDestroyDescriptorSetLayout(device, vk.curveComputeDescriptorSetLayout, nullptr);
  }
  if (vk.descriptorSetLayout != VK_NULL_HANDLE)
  {
    vkDestroyDescriptorSetLayout(device, vk.descriptorSetLayout, nullptr);
  }
  if (vk.descriptorPool != VK_NULL_HANDLE)
  {
    vkDestroyDescriptorPool(device, vk.descriptorPool, nullptr);
  }
}

} // namespace Rl::Client::Render
