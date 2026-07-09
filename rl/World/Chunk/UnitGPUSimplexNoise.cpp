import Rl.World.Chunk.UnitGPUSimplexNoise;
import Rl.Base.Shader;
import Rl.Client.Render.Unit.UnitRendererBasicBuffer;

import <cstring>;
import <stdexcept>;
import <vulkan/vulkan.hpp>;

namespace Rl::World::Chunk
{

void UnitGPUSimplexNoise::Create(
    VkDevice device, VkPhysicalDevice physicalDevice, uint32_t seed)
{
  if (isInitialized)
  {
    return; // Already initialized
  }

  const VkDeviceSize permBufferSize = 256 * sizeof(int32_t);
  Rl::Client::Render::UnitCreateBuffer(device, physicalDevice, permBufferSize,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, permBuffer, permBufferMemory);

  Rl::Client::Render::UnitCreateBuffer(device, physicalDevice, permBufferSize,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, permGradIndex3DBuffer,
      permGradIndex3DBufferMemory);

  VkDescriptorPoolSize poolSizes[3] = {};
  poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  poolSizes[0].descriptorCount = 3; // perm, permGradIndex3D, noise output
  poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  poolSizes[1].descriptorCount = 1;
  poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  poolSizes[2].descriptorCount = 1;

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = 3;
  poolInfo.pPoolSizes = poolSizes;
  poolInfo.maxSets = 2; // One for init, one for generation

  if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create descriptor pool for Simplex noise");
  }

  // Create descriptor set layout
  VkDescriptorSetLayoutBinding bindings[3] = {};
  bindings[0].binding = 0;
  bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  bindings[0].descriptorCount = 1;
  bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  bindings[1].binding = 1;
  bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  bindings[1].descriptorCount = 1;
  bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  bindings[2].binding = 2;
  bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  bindings[2].descriptorCount = 1;
  bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = 3;
  layoutInfo.pBindings = bindings;

  if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) !=
      VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create descriptor set layout for Simplex noise");
  }

  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = &descriptorSetLayout;

  if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to allocate descriptor set for Simplex noise");
  }

  VkPushConstantRange initPushConstantRange{};
  initPushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  initPushConstantRange.offset = 0;
  initPushConstantRange.size = sizeof(uint32_t);

  VkPipelineLayoutCreateInfo initLayoutInfo{};
  initLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  initLayoutInfo.setLayoutCount = 1;
  initLayoutInfo.pSetLayouts = &descriptorSetLayout;
  initLayoutInfo.pushConstantRangeCount = 1;
  initLayoutInfo.pPushConstantRanges = &initPushConstantRange;

  if (vkCreatePipelineLayout(device, &initLayoutInfo, nullptr, &initPipelineLayout) !=
      VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create init pipeline layout for Simplex noise");
  }

  // Create init compute pipeline
  auto initShaderCode = Providers::ShaderObject::Shader("simplex.init.comp.spv");
  auto initShaderModule = Providers::ShaderObject::Module(device, initShaderCode);

  VkPipelineShaderStageCreateInfo initShaderStageInfo{};
  initShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  initShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  initShaderStageInfo.module = initShaderModule.module;
  initShaderStageInfo.pName = "main";

  VkComputePipelineCreateInfo initPipelineInfo{};
  initPipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  initPipelineInfo.stage = initShaderStageInfo;
  initPipelineInfo.layout = initPipelineLayout;

  if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &initPipelineInfo, nullptr,
          &initPipeline) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create init compute pipeline for Simplex noise");
  }
  Providers::ShaderObject::DestroyShaderModule(device, initShaderModule);

  VkDescriptorBufferInfo permBufferInfo{};
  permBufferInfo.buffer = permBuffer;
  permBufferInfo.offset = 0;
  permBufferInfo.range = VK_WHOLE_SIZE;

  VkDescriptorBufferInfo permGradIndexBufferInfo{};
  permGradIndexBufferInfo.buffer = permGradIndex3DBuffer;
  permGradIndexBufferInfo.offset = 0;
  permGradIndexBufferInfo.range = VK_WHOLE_SIZE;

  VkDescriptorBufferInfo dummyNoiseBufferInfo{};
  dummyNoiseBufferInfo.buffer = permBuffer; // Use any valid buffer as placeholder
  dummyNoiseBufferInfo.offset = 0;
  dummyNoiseBufferInfo.range = sizeof(float);

  VkWriteDescriptorSet initWrites[3] = {};
  initWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  initWrites[0].dstSet = descriptorSet;
  initWrites[0].dstBinding = 0;
  initWrites[0].dstArrayElement = 0;
  initWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  initWrites[0].descriptorCount = 1;
  initWrites[0].pBufferInfo = &permBufferInfo;

  initWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  initWrites[1].dstSet = descriptorSet;
  initWrites[1].dstBinding = 1;
  initWrites[1].dstArrayElement = 0;
  initWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  initWrites[1].descriptorCount = 1;
  initWrites[1].pBufferInfo = &permGradIndexBufferInfo;

  initWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  initWrites[2].dstSet = descriptorSet;
  initWrites[2].dstBinding = 2;
  initWrites[2].dstArrayElement = 0;
  initWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  initWrites[2].descriptorCount = 1;
  initWrites[2].pBufferInfo = &dummyNoiseBufferInfo;

  vkUpdateDescriptorSets(device, 3, initWrites, 0, nullptr);

  // Dispatch init shader to populate permutation tables
  // Note: This requires a command buffer, which should be provided by the caller
  // For now, we'll mark as initialized and the caller should dispatch the init shader
  isInitialized = true;
}

void UnitGPUSimplexNoise::CreateNoiseBuffer(VkDevice device,
    VkPhysicalDevice                                 physicalDevice,
    uint32_t                                         width,
    uint32_t                                         height,
    uint32_t                                         depth)
{
  if (noiseBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device, noiseBuffer, nullptr);
    vkFreeMemory(device, noiseBufferMemory, nullptr);
  }

  noiseWidth = width;
  noiseHeight = height;
  noiseDepth = depth;

  const uint32_t     totalElements = width * height * depth;
  const VkDeviceSize noiseBufferSize = totalElements * sizeof(float);

  Client::Render::UnitCreateBuffer(device, physicalDevice, noiseBufferSize,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, noiseBuffer, noiseBufferMemory);

  // Update descriptor set for noise buffer
  VkDescriptorBufferInfo noiseBufferInfo{};
  noiseBufferInfo.buffer = noiseBuffer;
  noiseBufferInfo.offset = 0;
  noiseBufferInfo.range = VK_WHOLE_SIZE;

  VkWriteDescriptorSet noiseWrite{};
  noiseWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  noiseWrite.dstSet = descriptorSet;
  noiseWrite.dstBinding = 2;
  noiseWrite.dstArrayElement = 0;
  noiseWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  noiseWrite.descriptorCount = 1;
  noiseWrite.pBufferInfo = &noiseBufferInfo;

  vkUpdateDescriptorSets(device, 1, &noiseWrite, 0, nullptr);
}

void UnitGPUSimplexNoise::CreateWorldOutputBuffers(VkDevice device,
    VkPhysicalDevice                                 physicalDevice,
    uint32_t                                         width,
    uint32_t                                         height,
    uint32_t                                         depth)
{
  // Destroy existing outputs if any
  if (temperatureBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device, temperatureBuffer, nullptr);
    vkFreeMemory(device, temperatureBufferMemory, nullptr);
    temperatureBuffer = VK_NULL_HANDLE;
    temperatureBufferMemory = VK_NULL_HANDLE;
  }
  if (moistureBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device, moistureBuffer, nullptr);
    vkFreeMemory(device, moistureBufferMemory, nullptr);
    moistureBuffer = VK_NULL_HANDLE;
    moistureBufferMemory = VK_NULL_HANDLE;
  }
  if (elevationBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device, elevationBuffer, nullptr);
    vkFreeMemory(device, elevationBufferMemory, nullptr);
    elevationBuffer = VK_NULL_HANDLE;
    elevationBufferMemory = VK_NULL_HANDLE;
  }

  const uint32_t totalElements = width * height * depth;
  const VkDeviceSize bufferSize = totalElements * sizeof(float);

  Client::Render::UnitCreateBuffer(device, physicalDevice, bufferSize,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, temperatureBuffer, temperatureBufferMemory);

  Client::Render::UnitCreateBuffer(device, physicalDevice, bufferSize,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, moistureBuffer, moistureBufferMemory);

  Client::Render::UnitCreateBuffer(device, physicalDevice, bufferSize,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, elevationBuffer, elevationBufferMemory);

  // Create descriptor pool and layout for mapping shader
  VkDescriptorPoolSize poolSizes[4] = {};
  for (int i = 0; i < 4; ++i)
  {
    poolSizes[i].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[i].descriptorCount = 1;
  }

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = 4;
  poolInfo.pPoolSizes = poolSizes;
  poolInfo.maxSets = 1;

  if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &mappingDescriptorPool) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create descriptor pool for world mapping");
  }

  // descriptor set layout: binding 0 = noise input, 1=temp,2=moist,3=elev
  VkDescriptorSetLayoutBinding bindings[4] = {};
  for (uint32_t i = 0; i < 4; ++i)
  {
    bindings[i].binding = i;
    bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[i].descriptorCount = 1;
    bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  }

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = 4;
  layoutInfo.pBindings = bindings;

  if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &mappingDescriptorSetLayout) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create descriptor set layout for world mapping");
  }

  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = mappingDescriptorPool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = &mappingDescriptorSetLayout;

  if (vkAllocateDescriptorSets(device, &allocInfo, &mappingDescriptorSet) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to allocate descriptor set for world mapping");
  }

  VkDescriptorBufferInfo noiseInfo{};
  noiseInfo.buffer = noiseBuffer;
  noiseInfo.offset = 0;
  noiseInfo.range = VK_WHOLE_SIZE;

  VkDescriptorBufferInfo tempInfo{};
  tempInfo.buffer = temperatureBuffer;
  tempInfo.offset = 0;
  tempInfo.range = VK_WHOLE_SIZE;

  VkDescriptorBufferInfo moistInfo{};
  moistInfo.buffer = moistureBuffer;
  moistInfo.offset = 0;
  moistInfo.range = VK_WHOLE_SIZE;

  VkDescriptorBufferInfo elevInfo{};
  elevInfo.buffer = elevationBuffer;
  elevInfo.offset = 0;
  elevInfo.range = VK_WHOLE_SIZE;

  VkWriteDescriptorSet writes[4] = {};
  writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writes[0].dstSet = mappingDescriptorSet;
  writes[0].dstBinding = 0;
  writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  writes[0].descriptorCount = 1;
  writes[0].pBufferInfo = &noiseInfo;

  writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writes[1].dstSet = mappingDescriptorSet;
  writes[1].dstBinding = 1;
  writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  writes[1].descriptorCount = 1;
  writes[1].pBufferInfo = &tempInfo;

  writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writes[2].dstSet = mappingDescriptorSet;
  writes[2].dstBinding = 2;
  writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  writes[2].descriptorCount = 1;
  writes[2].pBufferInfo = &moistInfo;

  writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writes[3].dstSet = mappingDescriptorSet;
  writes[3].dstBinding = 3;
  writes[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  writes[3].descriptorCount = 1;
  writes[3].pBufferInfo = &elevInfo;

  vkUpdateDescriptorSets(device, 4, writes, 0, nullptr);
}

void UnitGPUSimplexNoise::GenNoise(VkDevice device,
    VkCommandBuffer                         commandBuffer,
    const SimplexNoisePushConstants&        params) const
{
  if (!isInitialized)
  {
    throw std::runtime_error("Simplex noise not initialized. Call Initialize() first");
  }

  VkPushConstantRange genPushConstantRange{};
  genPushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  genPushConstantRange.offset = 0;
  genPushConstantRange.size = sizeof(SimplexNoisePushConstants);

  VkPipelineLayoutCreateInfo genLayoutInfo{};
  genLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  genLayoutInfo.setLayoutCount = 1;
  genLayoutInfo.pSetLayouts = &descriptorSetLayout;
  genLayoutInfo.pushConstantRangeCount = 1;
  genLayoutInfo.pPushConstantRanges = &genPushConstantRange;

  VkPipelineLayout genPipelineLayout;
  if (vkCreatePipelineLayout(device, &genLayoutInfo, nullptr, &genPipelineLayout) !=
      VK_SUCCESS)
  {
    throw std::runtime_error(
        "Failed to create generation pipeline layout for Simplex noise");
  }

  auto genShaderCode = Providers::ShaderObject::Shader("simplex.comp.spv");
  auto genShaderModule = Providers::ShaderObject::Module(device, genShaderCode);

  VkPipelineShaderStageCreateInfo genShaderStageInfo{};
  genShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  genShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  genShaderStageInfo.module = genShaderModule.module;
  genShaderStageInfo.pName = "main";

  VkComputePipelineCreateInfo genPipelineInfo{};
  genPipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  genPipelineInfo.stage = genShaderStageInfo;
  genPipelineInfo.layout = genPipelineLayout;

  VkPipeline genPipeline;
  if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &genPipelineInfo, nullptr,
          &genPipeline) != VK_SUCCESS)
  {
    throw std::runtime_error(
        "Failed to create generation compute pipeline for Simplex noise");
  }
  Providers::ShaderObject::DestroyShaderModule(device, genShaderModule);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, genPipeline);
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
      genPipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

  // Push constants
  vkCmdPushConstants(commandBuffer, genPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
      sizeof(SimplexNoisePushConstants), &params);

  // Dispatch compute shader
  const uint32_t workgroupSize = 8;
  uint32_t       workgroupsX = (params.width + workgroupSize - 1) / workgroupSize;
  uint32_t       workgroupsY = (params.height + workgroupSize - 1) / workgroupSize;
  uint32_t       workgroupsZ = (params.depth + workgroupSize - 1) / workgroupSize;

  vkCmdDispatch(commandBuffer, workgroupsX, workgroupsY, workgroupsZ);

  // Add memory barrier to ensure shader writes are complete
  VkBufferMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.buffer = noiseBuffer;
  barrier.offset = 0;
  barrier.size = VK_WHOLE_SIZE;

  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);

  vkDestroyPipeline(device, genPipeline, nullptr);
  vkDestroyPipelineLayout(device, genPipelineLayout, nullptr);
}

void UnitGPUSimplexNoise::GenWorldNoise(VkDevice device,
    VkCommandBuffer                         commandBuffer,
    const WorldNoisePushConstants&         params) const
{
  if (noiseBuffer == VK_NULL_HANDLE)
  {
    throw std::runtime_error("Noise buffer not created for world mapping");
  }

  if (mappingDescriptorSetLayout == VK_NULL_HANDLE)
  {
    throw std::runtime_error("World mapping descriptor set not created");
  }

  VkPushConstantRange pcRange{};
  pcRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  pcRange.offset = 0;
  pcRange.size = sizeof(WorldNoisePushConstants);

  VkPipelineLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layoutInfo.setLayoutCount = 1;
  layoutInfo.pSetLayouts = &mappingDescriptorSetLayout;
  layoutInfo.pushConstantRangeCount = 1;
  layoutInfo.pPushConstantRanges = &pcRange;

  VkPipelineLayout localLayout;
  if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &localLayout) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create pipeline layout for world mapping");
  }

  auto shaderCode = Providers::ShaderObject::Shader("world.noise.comp.spv");
  auto shaderModule = Providers::ShaderObject::Module(device, shaderCode);

  VkPipelineShaderStageCreateInfo stageInfo{};
  stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  stageInfo.module = shaderModule.module;
  stageInfo.pName = "main";

  VkComputePipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipelineInfo.stage = stageInfo;
  pipelineInfo.layout = localLayout;

  VkPipeline localPipeline;
  if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &localPipeline) != VK_SUCCESS)
  {
    Providers::ShaderObject::DestroyShaderModule(device, shaderModule);
    vkDestroyPipelineLayout(device, localLayout, nullptr);
    throw std::runtime_error("Failed to create compute pipeline for world mapping");
  }

  Providers::ShaderObject::DestroyShaderModule(device, shaderModule);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, localPipeline);
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, localLayout, 0, 1,
      &mappingDescriptorSet, 0, nullptr);

  vkCmdPushConstants(commandBuffer, localLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
      sizeof(WorldNoisePushConstants), &params);

  const uint32_t workgroupSize = 8;
  uint32_t workgroupsX = (params.width + workgroupSize - 1) / workgroupSize;
  uint32_t workgroupsY = (params.height + workgroupSize - 1) / workgroupSize;
  uint32_t workgroupsZ = (params.depth + workgroupSize - 1) / workgroupSize;

  vkCmdDispatch(commandBuffer, workgroupsX, workgroupsY, workgroupsZ);

  // Barrier to ensure shader writes are visible for transfer/read operations
  VkBufferMemoryBarrier barriers[3] = {};
  barriers[0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  barriers[0].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  barriers[0].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
  barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barriers[0].buffer = temperatureBuffer;
  barriers[0].offset = 0;
  barriers[0].size = VK_WHOLE_SIZE;

  barriers[1] = barriers[0];
  barriers[1].buffer = moistureBuffer;

  barriers[2] = barriers[0];
  barriers[2].buffer = elevationBuffer;

  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 3, barriers, 0, nullptr);

  vkDestroyPipeline(device, localPipeline, nullptr);
  vkDestroyPipelineLayout(device, localLayout, nullptr);
}

void UnitGPUSimplexNoise::Destroy(VkDevice device)
{
  if (noiseBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device, noiseBuffer, nullptr);
    vkFreeMemory(device, noiseBufferMemory, nullptr);
    noiseBuffer = VK_NULL_HANDLE;
    noiseBufferMemory = VK_NULL_HANDLE;
  }

  if (permBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device, permBuffer, nullptr);
    vkFreeMemory(device, permBufferMemory, nullptr);
    permBuffer = VK_NULL_HANDLE;
    permBufferMemory = VK_NULL_HANDLE;
  }

  if (permGradIndex3DBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device, permGradIndex3DBuffer, nullptr);
    vkFreeMemory(device, permGradIndex3DBufferMemory, nullptr);
    permGradIndex3DBuffer = VK_NULL_HANDLE;
    permGradIndex3DBufferMemory = VK_NULL_HANDLE;
  }

  if (initPipeline != VK_NULL_HANDLE)
  {
    vkDestroyPipeline(device, initPipeline, nullptr);
    initPipeline = VK_NULL_HANDLE;
  }

  if (initPipelineLayout != VK_NULL_HANDLE)
  {
    vkDestroyPipelineLayout(device, initPipelineLayout, nullptr);
    initPipelineLayout = VK_NULL_HANDLE;
  }

  if (pipeline != VK_NULL_HANDLE)
  {
    vkDestroyPipeline(device, pipeline, nullptr);
    pipeline = VK_NULL_HANDLE;
  }

  if (pipelineLayout != VK_NULL_HANDLE)
  {
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    pipelineLayout = VK_NULL_HANDLE;
  }

  if (descriptorSetLayout != VK_NULL_HANDLE)
  {
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
    descriptorSetLayout = VK_NULL_HANDLE;
  }

  if (descriptorPool != VK_NULL_HANDLE)
  {
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    descriptorPool = VK_NULL_HANDLE;
  }
  // mapping outputs cleanup
  if (temperatureBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device, temperatureBuffer, nullptr);
    vkFreeMemory(device, temperatureBufferMemory, nullptr);
    temperatureBuffer = VK_NULL_HANDLE;
    temperatureBufferMemory = VK_NULL_HANDLE;
  }
  if (moistureBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device, moistureBuffer, nullptr);
    vkFreeMemory(device, moistureBufferMemory, nullptr);
    moistureBuffer = VK_NULL_HANDLE;
    moistureBufferMemory = VK_NULL_HANDLE;
  }
  if (elevationBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device, elevationBuffer, nullptr);
    vkFreeMemory(device, elevationBufferMemory, nullptr);
    elevationBuffer = VK_NULL_HANDLE;
    elevationBufferMemory = VK_NULL_HANDLE;
  }

  if (mappingPipeline != VK_NULL_HANDLE)
  {
    vkDestroyPipeline(device, mappingPipeline, nullptr);
    mappingPipeline = VK_NULL_HANDLE;
  }

  if (mappingPipelineLayout != VK_NULL_HANDLE)
  {
    vkDestroyPipelineLayout(device, mappingPipelineLayout, nullptr);
    mappingPipelineLayout = VK_NULL_HANDLE;
  }

  if (mappingDescriptorSetLayout != VK_NULL_HANDLE)
  {
    vkDestroyDescriptorSetLayout(device, mappingDescriptorSetLayout, nullptr);
    mappingDescriptorSetLayout = VK_NULL_HANDLE;
  }

  if (mappingDescriptorPool != VK_NULL_HANDLE)
  {
    vkDestroyDescriptorPool(device, mappingDescriptorPool, nullptr);
    mappingDescriptorPool = VK_NULL_HANDLE;
  }
  isInitialized = false;
}

} // namespace Rl::World::Chunk
