#pragma once

#include "rl/Base/Game.h"

#include <vulkan/vulkan.hpp>

namespace Rl::Client::Render
{

void UnitCreateComputeDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout& layout);

void UnitCreateGraphicsDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout& layout);

void UnitCreateCurvatureComputeDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout& layout);

void UnitCreateDescriptorPool(VkDevice device, VkDescriptorPool& pool);

void UnitAllocateComputeDescriptorSet(
    VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout layout, VkDescriptorSet& set);

void UnitAllocateCurvatureComputeDescriptorSet(
    VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout layout, VkDescriptorSet& set);

void UnitAllocateGraphicsDescriptorSet(
    VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout layout, VkDescriptorSet& set);

// Update compute descriptor set with SSBOs
void UnitUpdateComputeDescriptorSet(VkDevice device,
    VkDescriptorSet                          set,
    VkBuffer                                 vertexBuffer,
    VkBuffer                                 indexBuffer,
    VkBuffer                                 outputIndexBuffer,
    VkBuffer                                 visibleCountBuffer,
    VkBuffer                                 indirectDrawBuffer,
    VkBuffer                                 frustumBuffer,
    size_t                                   vertexBufferSize);

void UnitUpdateCurvatureComputeDescriptorSet(VkDevice device,
    VkDescriptorSet                                   set,
    VkBuffer                                          vertexBuffer,
    VkBuffer                                          indexBuffer,
    VkBuffer                                          curvedVertexBuffer,
    VkBuffer                                          curvedIndexBuffer,
    VkBuffer                                          countersBuffer,
    VkBuffer                                          indirectDrawBuffer,
    size_t                                            vertexBufferSize,
    size_t                                            indexBufferSize);

void UnitUpdateGraphicsDescriptorSetWithPlaceholders(VkDevice device,
    VkDescriptorSet                                           set,
    VkBuffer                                                  lightingBuffer,
    VkImageView                                               lightingTextureView,
    VkSampler                                                 lightingSampler,
    VkBuffer                                                  settingsBuffer,
    VkImageView                                               aoTextureView,
    VkSampler                                                 aoSampler,
    VkImageView                                               normalTextureView,
    VkSampler                                                 normalSampler,
    VkBuffer                                                  triplanarBuffer,
    size_t                                                    lightingBufferSize);

void UnitUpdateGraphicsDescriptorSetWithShadowMap(VkDevice device,
    VkDescriptorSet set,
    VkImageView shadowMapView,
    VkSampler shadowMapSampler);

} // namespace Rl::Client::Render
