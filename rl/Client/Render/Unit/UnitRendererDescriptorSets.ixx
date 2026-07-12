export module Rl.Client.Render.Unit.UnitRendererDescriptorSets;

import Rl.Base.Game;
import Rl.Client.Render.Unit.UnitRendererShadowMap;
import <vector>;
import <vulkan/vulkan.hpp>;

namespace Rl::Client::Render
{

export void UnitCreateComputeDescriptorSetLayout(
    VkDevice device, VkDescriptorSetLayout& layout);

export void UnitCreateGraphicsDescriptorSetLayout(
    VkDevice device, VkDescriptorSetLayout& layout);

export void UnitCreateCurvatureComputeDescriptorSetLayout(
    VkDevice device, VkDescriptorSetLayout& layout);

export void UnitCreateDescriptorPool(VkDevice device, VkDescriptorPool& pool);

export void UnitAllocateComputeDescriptorSet(VkDevice device,
    VkDescriptorPool                                  pool,
    VkDescriptorSetLayout                             layout,
    VkDescriptorSet&                                  set);

export void UnitAllocateCurvatureComputeDescriptorSet(VkDevice device,
    VkDescriptorPool                                           pool,
    VkDescriptorSetLayout                                      layout,
    VkDescriptorSet&                                           set);

export void UnitAllocateGraphicsDescriptorSet(VkDevice device,
    VkDescriptorPool                                   pool,
    VkDescriptorSetLayout                              layout,
    VkDescriptorSet&                                   set);

// Update compute descriptor set with SSBOs
export void UnitUpdateComputeDescriptorSet(VkDevice device,
    VkDescriptorSet                                 set,
    VkBuffer                                        vertexBuffer,
    VkBuffer                                        indexBuffer,
    VkBuffer                                        outputIndexBuffer,
    VkBuffer                                        visibleCountBuffer,
    VkBuffer                                        indirectDrawBuffer,
    VkBuffer                                        frustumBuffer,
    VkBuffer                                        mvpBuffer,
    size_t                                          vertexBufferSize);

export void UnitUpdateCurvatureComputeDescriptorSet(VkDevice device,
    VkDescriptorSet                                          set,
    VkBuffer                                                 vertexBuffer,
    VkBuffer                                                 indexBuffer,
    VkBuffer                                                 curvedVertexBuffer,
    VkBuffer                                                 curvedIndexBuffer,
    VkBuffer                                                 countersBuffer,
    VkBuffer                                                 indirectDrawBuffer,
    size_t                                                   vertexBufferSize,
    size_t                                                   indexBufferSize);

export void UnitUpdateGraphicsDescriptorSetWithPlaceholders(VkDevice device,
    VkDescriptorSet                                                  set,
    VkBuffer                                                         lightingBuffer,
    VkImageView                                                      lightingTextureView,
    VkSampler                                                        lightingSampler,
    VkBuffer                                                         settingsBuffer,
    VkImageView                                                      aoTextureView,
    VkSampler                                                        aoSampler,
    VkImageView                                                      normalTextureView,
    VkSampler                                                        normalSampler,
    VkBuffer                                                         triplanarBuffer,
    VkBuffer                                                         mvpBuffer,
    size_t                                                           lightingBufferSize);

export void UnitUpdateGraphicsDescriptorSetWithShadowMap(VkDevice device,
    VkDescriptorSet                                               set,
    VkSampler                                                     shadowMapSampler,
    const std::vector<UnitCascadeShadowLevel>&                    cascades,
    VkBuffer                                                      cascadeMatricesBuffer);

/* Update graphics descriptor set with unit array buffers */
export void UnitUpdateGraphicsDescriptorSetWithUnitArrays(VkDevice device,
    VkDescriptorSet                                               set,
    VkBuffer                                                      unitArrayBuffer,
    VkBuffer                                                      polFenceArrayBuffer);

} // namespace Rl::Client::Render
