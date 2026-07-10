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
  vkDestroyBuffer(device, vk.vertexBuffer, nullptr);
  vkFreeMemory(device, vk.vertexBufferMemory, nullptr);
  vkDestroyBuffer(device, vk.indexBuffer, nullptr);
  vkFreeMemory(device, vk.indexBufferMemory, nullptr);
  vkDestroyBuffer(device, vk.outputIndexBuffer, nullptr);
  vkFreeMemory(device, vk.outputIndexBufferMemory, nullptr);
  vkDestroyBuffer(device, vk.visibleCountBuffer, nullptr);
  vkFreeMemory(device, vk.visibleCountBufferMemory, nullptr);
  vkDestroyBuffer(device, vk.indirectDrawBuffer, nullptr);
  vkFreeMemory(device, vk.indirectDrawBufferMemory, nullptr);
  vkDestroyBuffer(device, vk.frustumBuffer, nullptr);
  vkFreeMemory(device, vk.frustumBufferMemory, nullptr);
  vkDestroyBuffer(device, vk.triplanarSettingsBuffer, nullptr);
  vkFreeMemory(device, vk.triplanarSettingsBufferMemory, nullptr);
  vkDestroyBuffer(device, vk.placeholderSettingsBuffer, nullptr);
  vkFreeMemory(device, vk.placeholderSettingsBufferMemory, nullptr);
  vkDestroyBuffer(device, vk.placeholderLightingBuffer, nullptr);
  vkFreeMemory(device, vk.placeholderLightingBufferMemory, nullptr);
  vkDestroyBuffer(device, vk.shadowCascadeMatricesBuffer, nullptr);
  vkFreeMemory(device, vk.shadowCascadeMatricesMemory, nullptr);

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

  vkDestroyBuffer(device, vk.curvedVertexBuffer, nullptr);
  vkFreeMemory(device, vk.curvedVertexBufferMemory, nullptr);
  vkDestroyBuffer(device, vk.curvedIndexBuffer, nullptr);
  vkFreeMemory(device, vk.curvedIndexBufferMemory, nullptr);
  vkDestroyBuffer(device, vk.curveCountersBuffer, nullptr);
  vkFreeMemory(device, vk.curveCountersBufferMemory, nullptr);
  vkDestroyBuffer(device, vk.curveIndirectDrawBuffer, nullptr);
  vkFreeMemory(device, vk.curveIndirectDrawBufferMemory, nullptr);

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
  vkDestroySampler(device, vk.placeholderLightingSampler, nullptr);
  vkDestroyImageView(device, vk.placeholderLightingTextureView, nullptr);
  vkDestroyImage(device, vk.placeholderLightingTexture, nullptr);
  vkFreeMemory(device, vk.placeholderLightingTextureMemory, nullptr);
  vkDestroySampler(device, vk.placeholderAOSampler, nullptr);
  vkDestroyImageView(device, vk.placeholderAOTextureView, nullptr);
  vkDestroyImage(device, vk.placeholderAOTexture, nullptr);
  vkFreeMemory(device, vk.placeholderAOTextureMemory, nullptr);

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
{ vkDestroySampler(device, vk.globalTextureSampler, nullptr); }

void UnitCleanupPipelines(VkDevice device, Providers::UnitStateBinding& vk)
{
  vkDestroyPipeline(device, vk.computePipeline, nullptr);
  vkDestroyPipelineLayout(device, vk.computePipelineLayout, nullptr);
  vkDestroyPipeline(device, vk.curveComputePipeline, nullptr);
  vkDestroyPipelineLayout(device, vk.curveComputePipelineLayout, nullptr);
  vkDestroyPipeline(device, vk.pipeline, nullptr);
  vkDestroyPipelineLayout(device, vk.pipelineLayout, nullptr);
  vkDestroyPipeline(device, vk.shadowPipeline, nullptr);
  vkDestroyPipelineLayout(device, vk.shadowPipelineLayout, nullptr);
}

void UnitCleanupDescriptorSets(VkDevice device, Providers::UnitStateBinding& vk)
{
  vkDestroyDescriptorSetLayout(device, vk.computeDescriptorSetLayout, nullptr);
  vkDestroyDescriptorSetLayout(device, vk.curveComputeDescriptorSetLayout, nullptr);
  vkDestroyDescriptorSetLayout(device, vk.descriptorSetLayout, nullptr);
  vkDestroyDescriptorPool(device, vk.descriptorPool, nullptr);
}

} // namespace Rl::Client::Render
