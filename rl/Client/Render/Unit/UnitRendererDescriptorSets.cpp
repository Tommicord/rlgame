#include "rl/Client/Render/Unit/UnitRendererDescriptorSets.h"
#include "rl/Client/Render/Unit/UnitRendererInfo.h"

#include <stdexcept>

namespace Rl::Client::Render
{
void UnitCreateComputeDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout& layout)
{
  VkDescriptorSetLayoutBinding computeBindings[6]{};
  // Input vertices SSBO (binding 0)
  computeBindings[0].binding         = 0;
  computeBindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  computeBindings[0].descriptorCount = 1;
  computeBindings[0].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;
  // Index buffer (binding 1)
  computeBindings[1].binding         = 1;
  computeBindings[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  computeBindings[1].descriptorCount = 1;
  computeBindings[1].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

  computeBindings[2].binding         = 2;
  computeBindings[2].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  computeBindings[2].descriptorCount = 1;
  computeBindings[2].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;
  // Visible count SSBO (binding 3)
  computeBindings[3].binding         = 3;
  computeBindings[3].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  computeBindings[3].descriptorCount = 1;
  computeBindings[3].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;
  // Indirect draw SSBO (binding 4)
  computeBindings[4].binding         = 4;
  computeBindings[4].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  computeBindings[4].descriptorCount = 1;
  computeBindings[4].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;
  // Frustum planes uniform buffer (binding 5)
  computeBindings[5].binding         = 5;
  computeBindings[5].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  computeBindings[5].descriptorCount = 1;
  computeBindings[5].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

  VkDescriptorSetLayoutCreateInfo computeLayoutInfo{};
  computeLayoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  computeLayoutInfo.bindingCount = 6;
  computeLayoutInfo.pBindings    = computeBindings;

  if (vkCreateDescriptorSetLayout(device, &computeLayoutInfo, nullptr, &layout) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create compute descriptor set layout");
  }
}

void UnitCreateGraphicsDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout& layout)
{
  VkDescriptorSetLayoutBinding graphicsBindings[8]{};
  // Texture array (binding 2)
  graphicsBindings[0].binding         = 2;
  graphicsBindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  graphicsBindings[0].descriptorCount = 6; // 6 textures for 6 faces
  graphicsBindings[0].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
  // Lighting block uniform buffer (binding 4)
  graphicsBindings[1].binding         = 4;
  graphicsBindings[1].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  graphicsBindings[1].descriptorCount = 1;
  graphicsBindings[1].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
  // Lighting texture (binding 8)
  graphicsBindings[2].binding         = 8;
  graphicsBindings[2].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  graphicsBindings[2].descriptorCount = 1;
  graphicsBindings[2].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
  // Settings uniform buffer (binding 9)
  graphicsBindings[3].binding         = 9;
  graphicsBindings[3].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  graphicsBindings[3].descriptorCount = 1;
  graphicsBindings[3].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
  // AO texture (binding 10)
  graphicsBindings[4].binding         = 10;
  graphicsBindings[4].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  graphicsBindings[4].descriptorCount = 1;
  graphicsBindings[4].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
  // Normal texture (binding 11)
  graphicsBindings[5].binding         = 11;
  graphicsBindings[5].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  graphicsBindings[5].descriptorCount = 1;
  graphicsBindings[5].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
  // Triplanar settings uniform buffer (binding 12)
  graphicsBindings[6].binding         = 12;
  graphicsBindings[6].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  graphicsBindings[6].descriptorCount = 1;
  graphicsBindings[6].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
  // Shadow map sampler (binding 13)
  graphicsBindings[7].binding         = 13;
  graphicsBindings[7].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  graphicsBindings[7].descriptorCount = 1;
  graphicsBindings[7].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutCreateInfo graphicsLayoutInfo{};
  graphicsLayoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  graphicsLayoutInfo.bindingCount = 8;
  graphicsLayoutInfo.pBindings    = graphicsBindings;

  if (vkCreateDescriptorSetLayout(device, &graphicsLayoutInfo, nullptr, &layout) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create graphics descriptor set layout");
  }
}

void UnitCreateCurvatureComputeDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout& layout)
{
  VkDescriptorSetLayoutBinding curveComputeBindings[6]{};
  // Input vertices SSBO (binding 0)
  curveComputeBindings[0].binding         = 0;
  curveComputeBindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  curveComputeBindings[0].descriptorCount = 1;
  curveComputeBindings[0].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;
  // Index buffer (binding 1)
  curveComputeBindings[1].binding         = 1;
  curveComputeBindings[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  curveComputeBindings[1].descriptorCount = 1;
  curveComputeBindings[1].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;
  // Curved vertices SSBO (binding 2)
  curveComputeBindings[2].binding         = 2;
  curveComputeBindings[2].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  curveComputeBindings[2].descriptorCount = 1;
  curveComputeBindings[2].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;
  // Curved indices SSBO (binding 3)
  curveComputeBindings[3].binding         = 3;
  curveComputeBindings[3].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  curveComputeBindings[3].descriptorCount = 1;
  curveComputeBindings[3].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;
  // Counters SSBO (binding 4)
  curveComputeBindings[4].binding         = 4;
  curveComputeBindings[4].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  curveComputeBindings[4].descriptorCount = 1;
  curveComputeBindings[4].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;
  // Indirect draw SSBO (binding 5)
  curveComputeBindings[5].binding         = 5;
  curveComputeBindings[5].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  curveComputeBindings[5].descriptorCount = 1;
  curveComputeBindings[5].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

  VkDescriptorSetLayoutCreateInfo curveComputeLayoutInfo{};
  curveComputeLayoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  curveComputeLayoutInfo.bindingCount = 6;
  curveComputeLayoutInfo.pBindings    = curveComputeBindings;

  if (vkCreateDescriptorSetLayout(device, &curveComputeLayoutInfo, nullptr, &layout) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create curvature compute descriptor set layout");
  }
}

void UnitCreateDescriptorPool(VkDevice device, VkDescriptorPool& pool)
{
  VkDescriptorPoolSize poolSizes[3]{};
  poolSizes[0].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  poolSizes[0].descriptorCount = 11; // 5 SSBOs for face culling + 6 SSBOs for curvature compute
  poolSizes[1].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[1].descriptorCount =
      10; // 6 textures + 1 lighting texture + 1 AO texture + 1 normal texture + 1 shadow map
  poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[2].descriptorCount =
      6; // 1 frustum + 1 lighting block + 1 settings + 1 triplanar + 2 extra

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = 3;
  poolInfo.pPoolSizes    = poolSizes;
  poolInfo.maxSets       = 3; // 1 for face culling + 1 for curvature + 1 for graphics
  poolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

  if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &pool) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create descriptor pool");
  }
}

void UnitAllocateComputeDescriptorSet(
    VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout layout, VkDescriptorSet& set)
{
  VkDescriptorSetAllocateInfo computeAllocInfo{};
  computeAllocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  computeAllocInfo.descriptorPool     = pool;
  computeAllocInfo.descriptorSetCount = 1;
  computeAllocInfo.pSetLayouts        = &layout;

  if (vkAllocateDescriptorSets(device, &computeAllocInfo, &set) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to allocate compute descriptor set");
  }
}

void UnitAllocateCurvatureComputeDescriptorSet(
    VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout layout, VkDescriptorSet& set)
{
  VkDescriptorSetAllocateInfo curveComputeAllocInfo{};
  curveComputeAllocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  curveComputeAllocInfo.descriptorPool     = pool;
  curveComputeAllocInfo.descriptorSetCount = 1;
  curveComputeAllocInfo.pSetLayouts        = &layout;

  if (vkAllocateDescriptorSets(device, &curveComputeAllocInfo, &set) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to allocate curvature compute descriptor set");
  }
}

void UnitAllocateGraphicsDescriptorSet(
    VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout layout, VkDescriptorSet& set)
{
  VkDescriptorSetAllocateInfo graphicsAllocInfo{};
  graphicsAllocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  graphicsAllocInfo.descriptorPool     = pool;
  graphicsAllocInfo.descriptorSetCount = 1;
  graphicsAllocInfo.pSetLayouts        = &layout;

  if (vkAllocateDescriptorSets(device, &graphicsAllocInfo, &set) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to allocate graphics descriptor set");
  }
}

void UnitUpdateComputeDescriptorSet(VkDevice device,
    VkDescriptorSet                          set,
    VkBuffer                                 vertexBuffer,
    VkBuffer                                 indexBuffer,
    VkBuffer                                 outputIndexBuffer,
    VkBuffer                                 visibleCountBuffer,
    VkBuffer                                 indirectDrawBuffer,
    VkBuffer                                 frustumBuffer,
    size_t                                   vertexBufferSize)
{
  VkDescriptorBufferInfo inputBufferInfo{};
  inputBufferInfo.buffer = vertexBuffer;
  inputBufferInfo.offset = 0;
  inputBufferInfo.range  = vertexBufferSize;

  VkDescriptorBufferInfo indexBufferInfo{};
  indexBufferInfo.buffer = indexBuffer;
  indexBufferInfo.offset = 0;
  indexBufferInfo.range  = sizeof(uint32_t) * 36; // 6 faces × 6 indices

  VkDescriptorBufferInfo outputBufferInfo{};
  outputBufferInfo.buffer = outputIndexBuffer;
  outputBufferInfo.offset = 0;
  outputBufferInfo.range  = sizeof(uint32_t) * 36; // Max 36 indices for cube

  VkDescriptorBufferInfo countBufferInfo{};
  countBufferInfo.buffer = visibleCountBuffer;
  countBufferInfo.offset = 0;
  countBufferInfo.range  = sizeof(uint32_t);

  VkDescriptorBufferInfo indirectBufferInfo{};
  indirectBufferInfo.buffer = indirectDrawBuffer;
  indirectBufferInfo.offset = 0;
  indirectBufferInfo.range  = sizeof(UnitRenderDrawIndexedParams);

  VkDescriptorBufferInfo frustumBufferInfo{};
  frustumBufferInfo.buffer = frustumBuffer;
  frustumBufferInfo.offset = 0;
  frustumBufferInfo.range  = sizeof(UnitRenderFrustumPlanes);

  VkWriteDescriptorSet computeWrites[6]{};
  computeWrites[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  computeWrites[0].dstSet          = set;
  computeWrites[0].dstBinding      = 0;
  computeWrites[0].dstArrayElement = 0;
  computeWrites[0].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  computeWrites[0].descriptorCount = 1;
  computeWrites[0].pBufferInfo     = &inputBufferInfo;

  computeWrites[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  computeWrites[1].dstSet          = set;
  computeWrites[1].dstBinding      = 1;
  computeWrites[1].dstArrayElement = 0;
  computeWrites[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  computeWrites[1].descriptorCount = 1;
  computeWrites[1].pBufferInfo     = &indexBufferInfo;

  computeWrites[2].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  computeWrites[2].dstSet          = set;
  computeWrites[2].dstBinding      = 2;
  computeWrites[2].dstArrayElement = 0;
  computeWrites[2].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  computeWrites[2].descriptorCount = 1;
  computeWrites[2].pBufferInfo     = &outputBufferInfo;

  computeWrites[3].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  computeWrites[3].dstSet          = set;
  computeWrites[3].dstBinding      = 3;
  computeWrites[3].dstArrayElement = 0;
  computeWrites[3].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  computeWrites[3].descriptorCount = 1;
  computeWrites[3].pBufferInfo     = &countBufferInfo;

  computeWrites[4].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  computeWrites[4].dstSet          = set;
  computeWrites[4].dstBinding      = 4;
  computeWrites[4].dstArrayElement = 0;
  computeWrites[4].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  computeWrites[4].descriptorCount = 1;
  computeWrites[4].pBufferInfo     = &indirectBufferInfo;

  computeWrites[5].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  computeWrites[5].dstSet          = set;
  computeWrites[5].dstBinding      = 5;
  computeWrites[5].dstArrayElement = 0;
  computeWrites[5].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  computeWrites[5].descriptorCount = 1;
  computeWrites[5].pBufferInfo     = &frustumBufferInfo;

  vkUpdateDescriptorSets(device, 6, computeWrites, 0, nullptr);
}

void UnitUpdateCurvatureComputeDescriptorSet(VkDevice device,
    VkDescriptorSet                                   set,
    VkBuffer                                          vertexBuffer,
    VkBuffer                                          indexBuffer,
    VkBuffer                                          curvedVertexBuffer,
    VkBuffer                                          curvedIndexBuffer,
    VkBuffer                                          countersBuffer,
    VkBuffer                                          indirectDrawBuffer,
    size_t                                            vertexBufferSize,
    size_t                                            indexBufferSize)
{
  VkDescriptorBufferInfo curveInputBufferInfo{};
  curveInputBufferInfo.buffer = vertexBuffer;
  curveInputBufferInfo.offset = 0;
  curveInputBufferInfo.range  = vertexBufferSize;

  VkDescriptorBufferInfo curveIndexBufferInfo{};
  curveIndexBufferInfo.buffer = indexBuffer;
  curveIndexBufferInfo.offset = 0;
  curveIndexBufferInfo.range  = indexBufferSize;

  VkDescriptorBufferInfo curveOutputVertexBufferInfo{};
  curveOutputVertexBufferInfo.buffer = curvedVertexBuffer;
  curveOutputVertexBufferInfo.offset = 0;
  curveOutputVertexBufferInfo.range  = VK_WHOLE_SIZE;

  VkDescriptorBufferInfo curveOutputIndexBufferInfo{};
  curveOutputIndexBufferInfo.buffer = curvedIndexBuffer;
  curveOutputIndexBufferInfo.offset = 0;
  curveOutputIndexBufferInfo.range  = VK_WHOLE_SIZE;

  VkDescriptorBufferInfo curveCountersBufferInfo{};
  curveCountersBufferInfo.buffer = countersBuffer;
  curveCountersBufferInfo.offset = 0;
  curveCountersBufferInfo.range  = 2 * sizeof(uint32_t);

  VkDescriptorBufferInfo curveIndirectBufferInfo{};
  curveIndirectBufferInfo.buffer = indirectDrawBuffer;
  curveIndirectBufferInfo.offset = 0;
  curveIndirectBufferInfo.range  = sizeof(UnitRenderDrawIndexedParams);

  VkWriteDescriptorSet curveComputeWrites[6]{};
  curveComputeWrites[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  curveComputeWrites[0].dstSet          = set;
  curveComputeWrites[0].dstBinding      = 0;
  curveComputeWrites[0].dstArrayElement = 0;
  curveComputeWrites[0].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  curveComputeWrites[0].descriptorCount = 1;
  curveComputeWrites[0].pBufferInfo     = &curveInputBufferInfo;

  curveComputeWrites[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  curveComputeWrites[1].dstSet          = set;
  curveComputeWrites[1].dstBinding      = 1;
  curveComputeWrites[1].dstArrayElement = 0;
  curveComputeWrites[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  curveComputeWrites[1].descriptorCount = 1;
  curveComputeWrites[1].pBufferInfo     = &curveIndexBufferInfo;

  curveComputeWrites[2].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  curveComputeWrites[2].dstSet          = set;
  curveComputeWrites[2].dstBinding      = 2;
  curveComputeWrites[2].dstArrayElement = 0;
  curveComputeWrites[2].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  curveComputeWrites[2].descriptorCount = 1;
  curveComputeWrites[2].pBufferInfo     = &curveOutputVertexBufferInfo;

  curveComputeWrites[3].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  curveComputeWrites[3].dstSet          = set;
  curveComputeWrites[3].dstBinding      = 3;
  curveComputeWrites[3].dstArrayElement = 0;
  curveComputeWrites[3].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  curveComputeWrites[3].descriptorCount = 1;
  curveComputeWrites[3].pBufferInfo     = &curveOutputIndexBufferInfo;

  curveComputeWrites[4].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  curveComputeWrites[4].dstSet          = set;
  curveComputeWrites[4].dstBinding      = 4;
  curveComputeWrites[4].dstArrayElement = 0;
  curveComputeWrites[4].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  curveComputeWrites[4].descriptorCount = 1;
  curveComputeWrites[4].pBufferInfo     = &curveCountersBufferInfo;

  curveComputeWrites[5].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  curveComputeWrites[5].dstSet          = set;
  curveComputeWrites[5].dstBinding      = 5;
  curveComputeWrites[5].dstArrayElement = 0;
  curveComputeWrites[5].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  curveComputeWrites[5].descriptorCount = 1;
  curveComputeWrites[5].pBufferInfo     = &curveIndirectBufferInfo;

  vkUpdateDescriptorSets(device, 6, curveComputeWrites, 0, nullptr);
}

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
    size_t                                                    lightingBufferSize)
{
  VkDescriptorImageInfo textureArrayInfo[6]{};
  for (int i = 0; i < 6; ++i)
  {
    textureArrayInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    textureArrayInfo[i].imageView   = lightingTextureView;
    textureArrayInfo[i].sampler     = lightingSampler;
  }

  VkWriteDescriptorSet textureArrayWrite{};
  textureArrayWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  textureArrayWrite.dstSet          = set;
  textureArrayWrite.dstBinding      = 2;
  textureArrayWrite.dstArrayElement = 0;
  textureArrayWrite.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  textureArrayWrite.descriptorCount = 6;
  textureArrayWrite.pImageInfo      = textureArrayInfo;

  VkDescriptorBufferInfo lightingBufferInfo{};
  lightingBufferInfo.buffer = lightingBuffer;
  lightingBufferInfo.offset = 0;
  lightingBufferInfo.range  = lightingBufferSize;

  VkWriteDescriptorSet lightingBufferWrite{};
  lightingBufferWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  lightingBufferWrite.dstSet          = set;
  lightingBufferWrite.dstBinding      = 4;
  lightingBufferWrite.dstArrayElement = 0;
  lightingBufferWrite.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  lightingBufferWrite.descriptorCount = 1;
  lightingBufferWrite.pBufferInfo     = &lightingBufferInfo;

  VkDescriptorImageInfo lightingImageInfo{};
  lightingImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  lightingImageInfo.imageView   = lightingTextureView;
  lightingImageInfo.sampler     = lightingSampler;

  VkWriteDescriptorSet lightingTextureWrite{};
  lightingTextureWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  lightingTextureWrite.dstSet          = set;
  lightingTextureWrite.dstBinding      = 8;
  lightingTextureWrite.dstArrayElement = 0;
  lightingTextureWrite.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  lightingTextureWrite.descriptorCount = 1;
  lightingTextureWrite.pImageInfo      = &lightingImageInfo;

  VkDescriptorBufferInfo settingsBufferInfo{};
  settingsBufferInfo.buffer = settingsBuffer;
  settingsBufferInfo.offset = 0;
  settingsBufferInfo.range  = VK_WHOLE_SIZE;

  VkWriteDescriptorSet settingsWrite{};
  settingsWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  settingsWrite.dstSet          = set;
  settingsWrite.dstBinding      = 9;
  settingsWrite.dstArrayElement = 0;
  settingsWrite.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  settingsWrite.descriptorCount = 1;
  settingsWrite.pBufferInfo     = &settingsBufferInfo;

  VkDescriptorImageInfo aoImageInfo{};
  aoImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  aoImageInfo.imageView   = aoTextureView;
  aoImageInfo.sampler     = aoSampler;

  VkWriteDescriptorSet aoTextureWrite{};
  aoTextureWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  aoTextureWrite.dstSet          = set;
  aoTextureWrite.dstBinding      = 10;
  aoTextureWrite.dstArrayElement = 0;
  aoTextureWrite.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  aoTextureWrite.descriptorCount = 1;
  aoTextureWrite.pImageInfo      = &aoImageInfo;

  VkDescriptorImageInfo normalImageInfo{};
  normalImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  normalImageInfo.imageView   = normalTextureView;
  normalImageInfo.sampler     = normalSampler;

  VkWriteDescriptorSet normalTextureWrite{};
  normalTextureWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  normalTextureWrite.dstSet          = set;
  normalTextureWrite.dstBinding      = 11;
  normalTextureWrite.dstArrayElement = 0;
  normalTextureWrite.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  normalTextureWrite.descriptorCount = 1;
  normalTextureWrite.pImageInfo      = &normalImageInfo;

  // Triplanar settings buffer descriptor (binding 12)
  VkDescriptorBufferInfo triplanarBufferInfo{};
  triplanarBufferInfo.buffer = triplanarBuffer;
  triplanarBufferInfo.offset = 0;
  triplanarBufferInfo.range  = sizeof(UnitRenderTriplanarSettings);

  VkWriteDescriptorSet triplanarSettingsWrite{};
  triplanarSettingsWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  triplanarSettingsWrite.dstSet          = set;
  triplanarSettingsWrite.dstBinding      = 12;
  triplanarSettingsWrite.dstArrayElement = 0;
  triplanarSettingsWrite.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  triplanarSettingsWrite.descriptorCount = 1;
  triplanarSettingsWrite.pBufferInfo     = &triplanarBufferInfo;

  std::array descriptorWrites = {textureArrayWrite, lightingBufferWrite, lightingTextureWrite,
      settingsWrite, aoTextureWrite, normalTextureWrite, triplanarSettingsWrite};

  vkUpdateDescriptorSets(
      device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void UnitUpdateGraphicsDescriptorSetWithShadowMap(
    VkDevice device, VkDescriptorSet set, VkImageView shadowMapView, VkSampler shadowMapSampler)
{
  VkDescriptorImageInfo shadowMapInfo{};
  shadowMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  shadowMapInfo.imageView   = shadowMapView;
  shadowMapInfo.sampler     = shadowMapSampler;

  VkWriteDescriptorSet shadowMapWrite{};
  shadowMapWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  shadowMapWrite.dstSet          = set;
  shadowMapWrite.dstBinding      = 13;
  shadowMapWrite.dstArrayElement = 0;
  shadowMapWrite.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  shadowMapWrite.descriptorCount = 1;
  shadowMapWrite.pImageInfo      = &shadowMapInfo;

  vkUpdateDescriptorSets(device, 1, &shadowMapWrite, 0, nullptr);
}

} // namespace Rl::Client::Render
