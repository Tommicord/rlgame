#include "UnitRendererCleanup.h"

namespace Rl::Client::Render
{

void UnitCleanupResources(VkDevice device, Providers::UnitStateDrawableVulkan& vk)
{
    UnitCleanupBuffers(device, vk);
    UnitCleanupTextures(device, vk);
    UnitCleanupSamplers(device, vk);
    UnitCleanupPipelines(device, vk);
    UnitCleanupDescriptorSets(device, vk);
}

void UnitCleanupBuffers(VkDevice device, Providers::UnitStateDrawableVulkan& vk)
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

    // Cleanup curvature compute buffers
    vkDestroyBuffer(device, vk.curvedVertexBuffer, nullptr);
    vkFreeMemory(device, vk.curvedVertexBufferMemory, nullptr);
    vkDestroyBuffer(device, vk.curvedIndexBuffer, nullptr);
    vkFreeMemory(device, vk.curvedIndexBufferMemory, nullptr);
    vkDestroyBuffer(device, vk.curveCountersBuffer, nullptr);
    vkFreeMemory(device, vk.curveCountersBufferMemory, nullptr);
    vkDestroyBuffer(device, vk.curveIndirectDrawBuffer, nullptr);
    vkFreeMemory(device, vk.curveIndirectDrawBufferMemory, nullptr);
}

void UnitCleanupTextures(VkDevice device, Providers::UnitStateDrawableVulkan& vk)
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

    // Cleanup AO textures
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

    // Cleanup normal textures
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
}

void UnitCleanupSamplers(VkDevice device, Providers::UnitStateDrawableVulkan& vk)
{
    // Cleanup global sampler
    if (vk.globalTextureSampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(device, vk.globalTextureSampler, nullptr);
    }
}

void UnitCleanupPipelines(VkDevice device, Providers::UnitStateDrawableVulkan& vk)
{
    vkDestroyPipeline(device, vk.computePipeline, nullptr);
    vkDestroyPipelineLayout(device, vk.computePipelineLayout, nullptr);
    vkDestroyPipeline(device, vk.curveComputePipeline, nullptr);
    vkDestroyPipelineLayout(device, vk.curveComputePipelineLayout, nullptr);
    vkDestroyPipeline(device, vk.pipeline, nullptr);
    vkDestroyPipelineLayout(device, vk.pipelineLayout, nullptr);
}

void UnitCleanupDescriptorSets(VkDevice device, Providers::UnitStateDrawableVulkan& vk)
{
    vkDestroyDescriptorSetLayout(device, vk.computeDescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, vk.curveComputeDescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, vk.descriptorSetLayout, nullptr);
    vkDestroyDescriptorPool(device, vk.descriptorPool, nullptr);
}

} // namespace Rl::Client::Render
