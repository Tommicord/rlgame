import Rl.World.Chunk.UnitGPUSimplexNoise;
import Rl.Base.Shader;
import Rl.Client.Render.Buffer;
import Rl.RayLog.Macro;
import Rl.RayLog.Logger;

import Rl.Base.Game;

import <array>;
import <cstdint>;
import <cstring>;
import <limits>;
import <numeric>;
import <stdexcept>;
import <vector>;
import <vulkan/vulkan.hpp>;
namespace Rl::World::Chunk
{

namespace
{

void BuildPermutationTables(uint32_t                  seed,
                            std::array<int32_t, 256>& perm,
                            std::array<int32_t, 256>& permGradIndex3d)
{
    std::array<int32_t, 256> source{};
    std::iota(source.begin(), source.end(), 0);

    uint64_t state =
        static_cast<uint64_t>(seed) * 6364136223846793005ull + 1442695040888963407ull;

    for (int i = 255; i >= 0; --i)
    {
        state     = state * 6364136223846793005ull + 1442695040888963407ull;
        int32_t r = static_cast<int32_t>((state + 31) % static_cast<uint64_t>(i + 1));
        if (r < 0)
        {
            r += (i + 1);
        }

        perm[i]   = source[r];
        source[r] = source[i];
    }

    const int32_t gradientCount = 24;
    for (int i = 0; i < 256; ++i)
    {
        permGradIndex3d[i] = (perm[i] % gradientCount) * 3;
    }
}

void UploadBufferData(VkDevice       device,
                      VkDeviceMemory bufferMemory,
                      VkDeviceSize   size,
                      const void*    data)
{
    void* mapped = nullptr;
    if (vkMapMemory(device, bufferMemory, 0, size, 0, &mapped) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to map Vulkan buffer memory");
    }

    std::memcpy(mapped, data, static_cast<size_t>(size));
    vkUnmapMemory(device, bufferMemory);
}

void CreateWorldMappingPipeline(VkDevice              device,
                                VkDescriptorSetLayout descriptorSetLayout,
                                VkPipelineLayout&     pipelineLayout,
                                VkPipeline&           pipeline)
{
    if (pipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        pipelineLayout = VK_NULL_HANDLE;
    }

    if (pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(device, pipeline, nullptr);
        pipeline = VK_NULL_HANDLE;
    }

    VkPushConstantRange pcRange{};
    pcRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pcRange.offset     = 0;
    pcRange.size       = sizeof(UnitGPUSimplexNoise::WorldNoisePushConstants);

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount         = 1;
    layoutInfo.pSetLayouts            = &descriptorSetLayout;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges    = &pcRange;

    if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &pipelineLayout) !=
        VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create pipeline layout for world mapping");
    }

    auto shaderCode   = Providers::ShaderObject::Shader("world.noise.comp.spv");
    auto shaderModule = Providers::ShaderObject::Module(device, shaderCode);

    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
    stageInfo.module = shaderModule.module;
    stageInfo.pName  = "main";

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage  = stageInfo;
    pipelineInfo.layout = pipelineLayout;

    if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
                                 &pipeline) != VK_SUCCESS)
    {
        Providers::ShaderObject::DestroyShaderModule(device, shaderModule);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        pipelineLayout = VK_NULL_HANDLE;
        throw std::runtime_error("Failed to create compute pipeline for world mapping");
    }

    Providers::ShaderObject::DestroyShaderModule(device, shaderModule);
}
} // namespace

void UnitGPUSimplexNoise::Initialize(VkDevice         device,
                                     VkPhysicalDevice physicalDevice,
                                     uint32_t         seed)
{
    if (isInitialized)
    {
        return; // Already initialized
    }

    this->seed = seed;

    const VkDeviceSize permBufferSize = 256 * sizeof(int32_t);
    Client::Render::CreateBuffer(
        device, physicalDevice, permBufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, permBuffer, permBufferMemory);

    Client::Render::CreateBuffer(device, physicalDevice, permBufferSize,
                                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                     VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                 permGradIndex3DBuffer, permGradIndex3DBufferMemory);

    std::array<int32_t, 256> permData{};
    std::array<int32_t, 256> permGradIndexData{};
    BuildPermutationTables(seed, permData, permGradIndexData);

    VkBuffer       stagingPermBuffer       = VK_NULL_HANDLE;
    VkDeviceMemory stagingPermBufferMemory = VK_NULL_HANDLE;
    Client::Render::CreateBuffer(
        device, physicalDevice, permBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingPermBuffer, stagingPermBufferMemory);

    UploadBufferData(device, stagingPermBufferMemory, permBufferSize, permData.data());

    VkBuffer       stagingPermGradBuffer       = VK_NULL_HANDLE;
    VkDeviceMemory stagingPermGradBufferMemory = VK_NULL_HANDLE;
    Client::Render::CreateBuffer(
        device, physicalDevice, permBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingPermGradBuffer, stagingPermGradBufferMemory);

    UploadBufferData(device, stagingPermGradBufferMemory, permBufferSize,
                     permGradIndexData.data());

    auto& gameBinding    = Rl::Main::Game::GetInstance().GetMainBinding();
    auto  graphicsFamily = gameBinding.queueFamilyIndices.graphicsFamily;
    if (!graphicsFamily.has_value())
    {
        throw std::runtime_error(
            "No graphics queue family is available for permutation uploads");
    }
    const uint32_t          queueFamilyIndex = graphicsFamily.value();
    VkCommandPoolCreateInfo commandPoolInfo{};
    commandPoolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolInfo.flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    commandPoolInfo.queueFamilyIndex = queueFamilyIndex;

    VkCommandPool tempCommandPool = VK_NULL_HANDLE;
    if (vkCreateCommandPool(device, &commandPoolInfo, nullptr, &tempCommandPool) !=
        VK_SUCCESS)
    {
        vkDestroyBuffer(device, stagingPermBuffer, nullptr);
        vkFreeMemory(device, stagingPermBufferMemory, nullptr);
        vkDestroyBuffer(device, stagingPermGradBuffer, nullptr);
        vkFreeMemory(device, stagingPermGradBufferMemory, nullptr);
        throw std::runtime_error(
            "Failed to create temporary command pool for permutation upload");
    }

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool        = tempCommandPool;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer uploadCommandBuffer = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(device, &allocInfo, &uploadCommandBuffer) != VK_SUCCESS)
    {
        vkDestroyCommandPool(device, tempCommandPool, nullptr);
        vkDestroyBuffer(device, stagingPermBuffer, nullptr);
        vkFreeMemory(device, stagingPermBufferMemory, nullptr);
        vkDestroyBuffer(device, stagingPermGradBuffer, nullptr);
        vkFreeMemory(device, stagingPermGradBufferMemory, nullptr);
        throw std::runtime_error("Failed to allocate upload command buffer");
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vkBeginCommandBuffer(uploadCommandBuffer, &beginInfo) != VK_SUCCESS)
    {
        vkFreeCommandBuffers(device, tempCommandPool, 1, &uploadCommandBuffer);
        vkDestroyCommandPool(device, tempCommandPool, nullptr);
        vkDestroyBuffer(device, stagingPermBuffer, nullptr);
        vkFreeMemory(device, stagingPermBufferMemory, nullptr);
        vkDestroyBuffer(device, stagingPermGradBuffer, nullptr);
        vkFreeMemory(device, stagingPermGradBufferMemory, nullptr);
        throw std::runtime_error("Failed to begin upload command buffer");
    }

    VkBufferCopy permCopy{};
    permCopy.size = permBufferSize;
    vkCmdCopyBuffer(uploadCommandBuffer, stagingPermBuffer, permBuffer, 1, &permCopy);

    VkBufferCopy permGradCopy{};
    permGradCopy.size = permBufferSize;
    vkCmdCopyBuffer(uploadCommandBuffer, stagingPermGradBuffer, permGradIndex3DBuffer, 1,
                    &permGradCopy);

    if (vkEndCommandBuffer(uploadCommandBuffer) != VK_SUCCESS)
    {
        vkFreeCommandBuffers(device, tempCommandPool, 1, &uploadCommandBuffer);
        vkDestroyCommandPool(device, tempCommandPool, nullptr);
        vkDestroyBuffer(device, stagingPermBuffer, nullptr);
        vkFreeMemory(device, stagingPermBufferMemory, nullptr);
        vkDestroyBuffer(device, stagingPermGradBuffer, nullptr);
        vkFreeMemory(device, stagingPermGradBufferMemory, nullptr);
        throw std::runtime_error("Failed to record upload command buffer");
    }

    VkQueue queue = gameBinding.graphicsQueue;
    if (queue == VK_NULL_HANDLE)
    {
        vkFreeCommandBuffers(device, tempCommandPool, 1, &uploadCommandBuffer);
        vkDestroyCommandPool(device, tempCommandPool, nullptr);
        vkDestroyBuffer(device, stagingPermBuffer, nullptr);
        vkFreeMemory(device, stagingPermBufferMemory, nullptr);
        vkDestroyBuffer(device, stagingPermGradBuffer, nullptr);
        vkFreeMemory(device, stagingPermGradBufferMemory, nullptr);
        throw std::runtime_error(
            "No graphics queue is available for permutation uploads");
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &uploadCommandBuffer;

    if (vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
    {
        vkFreeCommandBuffers(device, tempCommandPool, 1, &uploadCommandBuffer);
        vkDestroyCommandPool(device, tempCommandPool, nullptr);
        vkDestroyBuffer(device, stagingPermBuffer, nullptr);
        vkFreeMemory(device, stagingPermBufferMemory, nullptr);
        vkDestroyBuffer(device, stagingPermGradBuffer, nullptr);
        vkFreeMemory(device, stagingPermGradBufferMemory, nullptr);
        throw std::runtime_error("Failed to submit permutation upload");
    }

    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(device, tempCommandPool, 1, &uploadCommandBuffer);
    vkDestroyCommandPool(device, tempCommandPool, nullptr);
    vkDestroyBuffer(device, stagingPermBuffer, nullptr);
    vkFreeMemory(device, stagingPermBufferMemory, nullptr);
    vkDestroyBuffer(device, stagingPermGradBuffer, nullptr);
    vkFreeMemory(device, stagingPermGradBufferMemory, nullptr);

    // Create init flag buffer
    const VkDeviceSize initFlagSize = sizeof(uint32_t);
    Client::Render::CreateBuffer(
        device, physicalDevice, initFlagSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, initFlagBuffer, initFlagBufferMemory);

    VkDescriptorPoolSize poolSizes[1] = {};
    poolSizes[0].type                 = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[0].descriptorCount = 4; // perm, permGradIndex3D, noise output, init flag

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes    = poolSizes;
    poolInfo.maxSets       = 1;

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor pool for Simplex noise");
    }

    // Create descriptor set layout with 4 bindings
    VkDescriptorSetLayoutBinding bindings[4] = {};
    bindings[0].binding                      = 0;
    bindings[0].descriptorType               = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[0].descriptorCount              = 1;
    bindings[0].stageFlags                   = VK_SHADER_STAGE_COMPUTE_BIT;

    bindings[1].binding         = 1;
    bindings[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

    bindings[2].binding         = 2;
    bindings[2].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[2].descriptorCount = 1;
    bindings[2].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

    bindings[3].binding         = 3;
    bindings[3].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[3].descriptorCount = 1;
    bindings[3].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 4;
    layoutInfo.pBindings    = bindings;

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) !=
        VK_SUCCESS)
    {
        throw std::runtime_error(
            "Failed to create descriptor set layout for Simplex noise");
    }

    VkDescriptorSetAllocateInfo allocInfo2{};
    allocInfo2.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo2.descriptorPool     = descriptorPool;
    allocInfo2.descriptorSetCount = 1;
    allocInfo2.pSetLayouts        = &descriptorSetLayout;

    if (vkAllocateDescriptorSets(device, &allocInfo2, &descriptorSet) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate descriptor set for Simplex noise");
    }

    // Update descriptor set with permutation buffers
    VkDescriptorBufferInfo permBufferInfo{};
    permBufferInfo.buffer = permBuffer;
    permBufferInfo.offset = 0;
    permBufferInfo.range  = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo permGradIndex3DBufferInfo{};
    permGradIndex3DBufferInfo.buffer = permGradIndex3DBuffer;
    permGradIndex3DBufferInfo.offset = 0;
    permGradIndex3DBufferInfo.range  = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo initFlagBufferInfo{};
    initFlagBufferInfo.buffer = initFlagBuffer;
    initFlagBufferInfo.offset = 0;
    initFlagBufferInfo.range  = VK_WHOLE_SIZE;

    VkWriteDescriptorSet writes[3] = {};

    writes[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet          = descriptorSet;
    writes[0].dstBinding      = 0;
    writes[0].dstArrayElement = 0;
    writes[0].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[0].descriptorCount = 1;
    writes[0].pBufferInfo     = &permBufferInfo;

    writes[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet          = descriptorSet;
    writes[1].dstBinding      = 1;
    writes[1].dstArrayElement = 0;
    writes[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[1].descriptorCount = 1;
    writes[1].pBufferInfo     = &permGradIndex3DBufferInfo;

    writes[2].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[2].dstSet          = descriptorSet;
    writes[2].dstBinding      = 3;
    writes[2].dstArrayElement = 0;
    writes[2].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[2].descriptorCount = 1;
    writes[2].pBufferInfo     = &initFlagBufferInfo;

    vkUpdateDescriptorSets(device, 3, writes, 0, nullptr);

    RayLog::LogInfo("UnitGPUSimplexNoise", "GenNoise: Creating pipeline layout");

    VkPushConstantRange genPushConstantRange{};
    genPushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    genPushConstantRange.offset     = 0;
    genPushConstantRange.size       = sizeof(SimplexNoisePushConstants);

    VkPipelineLayoutCreateInfo genLayoutInfo{};
    genLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    genLayoutInfo.setLayoutCount         = 1;
    genLayoutInfo.pSetLayouts            = &descriptorSetLayout;
    genLayoutInfo.pushConstantRangeCount = 1;
    genLayoutInfo.pPushConstantRanges    = &genPushConstantRange;

    if (vkCreatePipelineLayout(device, &genLayoutInfo, nullptr, &genPipelineLayout) !=
        VK_SUCCESS)
    {
        throw std::runtime_error(
            "Failed to create generation pipeline layout for Simplex noise");
    }

    RayLog::LogInfo("UnitGPUSimplexNoise", "GenNoise: Loading shader");

    auto genShaderCode   = Providers::ShaderObject::Shader("simplex.comp.spv");
    auto genShaderModule = Providers::ShaderObject::Module(device, genShaderCode);

    VkPipelineShaderStageCreateInfo genShaderStageInfo{};
    genShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    genShaderStageInfo.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
    genShaderStageInfo.module = genShaderModule.module;
    genShaderStageInfo.pName  = "main";

    VkComputePipelineCreateInfo genPipelineInfo{};
    genPipelineInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    genPipelineInfo.stage  = genShaderStageInfo;
    genPipelineInfo.layout = genPipelineLayout;
    RayLog::LogInfo("UnitGPUSimplexNoise", "GenNoise: Creating compute pipeline");
    if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &genPipelineInfo, nullptr,
                                 &genPipeline) != VK_SUCCESS)
    {
        throw std::runtime_error(
            "Failed to create generation compute pipeline for Simplex noise");
    }
    Providers::ShaderObject::DestroyShaderModule(device, genShaderModule);
    RayLog::LogInfo("UnitGPUSimplexNoise",
                    "GenNoise: Binding pipeline and descriptor sets");
    isInitialized = true;
}

void UnitGPUSimplexNoise::CreateNoiseBuffer(VkDevice         device,
                                            VkPhysicalDevice physicalDevice,
                                            uint32_t         width,
                                            uint32_t         height,
                                            uint32_t         depth)
{
    if (noiseBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(device, noiseBuffer, nullptr);
        vkFreeMemory(device, noiseBufferMemory, nullptr);
    }

    noiseWidth  = width;
    noiseHeight = height;
    noiseDepth  = depth;

    const uint32_t     totalElements   = width * height * depth;
    const VkDeviceSize noiseBufferSize = totalElements * sizeof(float);

    Client::Render::CreateBuffer(
        device, physicalDevice, noiseBufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, noiseBuffer, noiseBufferMemory);

    // Update descriptor set for noise buffer
    VkDescriptorBufferInfo noiseBufferInfo{};
    noiseBufferInfo.buffer = noiseBuffer;
    noiseBufferInfo.offset = 0;
    noiseBufferInfo.range  = VK_WHOLE_SIZE;

    VkWriteDescriptorSet noiseWrite{};
    noiseWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    noiseWrite.dstSet          = descriptorSet;
    noiseWrite.dstBinding      = 2;
    noiseWrite.dstArrayElement = 0;
    noiseWrite.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    noiseWrite.descriptorCount = 1;
    noiseWrite.pBufferInfo     = &noiseBufferInfo;

    vkUpdateDescriptorSets(device, 1, &noiseWrite, 0, nullptr);

    if (mappingDescriptorSet != VK_NULL_HANDLE)
    {
        VkDescriptorBufferInfo noiseInfo{};
        noiseInfo.buffer = noiseBuffer;
        noiseInfo.offset = 0;
        noiseInfo.range  = VK_WHOLE_SIZE;

        VkWriteDescriptorSet mappingNoiseWrite{};
        mappingNoiseWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        mappingNoiseWrite.dstSet          = mappingDescriptorSet;
        mappingNoiseWrite.dstBinding      = 0;
        mappingNoiseWrite.dstArrayElement = 0;
        mappingNoiseWrite.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        mappingNoiseWrite.descriptorCount = 1;
        mappingNoiseWrite.pBufferInfo     = &noiseInfo;

        vkUpdateDescriptorSets(device, 1, &mappingNoiseWrite, 0, nullptr);
    }
}

void UnitGPUSimplexNoise::CreateWorldOutputBuffers(VkDevice         device,
                                                   VkPhysicalDevice physicalDevice,
                                                   uint32_t         width,
                                                   uint32_t         height,
                                                   uint32_t         depth)
{
    // Destroy existing outputs if any
    if (temperatureBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(device, temperatureBuffer, nullptr);
        vkFreeMemory(device, temperatureBufferMemory, nullptr);
        temperatureBuffer       = VK_NULL_HANDLE;
        temperatureBufferMemory = VK_NULL_HANDLE;
    }
    if (moistureBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(device, moistureBuffer, nullptr);
        vkFreeMemory(device, moistureBufferMemory, nullptr);
        moistureBuffer       = VK_NULL_HANDLE;
        moistureBufferMemory = VK_NULL_HANDLE;
    }
    if (elevationBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(device, elevationBuffer, nullptr);
        vkFreeMemory(device, elevationBufferMemory, nullptr);
        elevationBuffer       = VK_NULL_HANDLE;
        elevationBufferMemory = VK_NULL_HANDLE;
    }

    const uint32_t     totalElements = width * height * depth;
    const VkDeviceSize bufferSize    = totalElements * sizeof(float);

    Client::Render::CreateBuffer(
        device, physicalDevice, bufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, temperatureBuffer, temperatureBufferMemory);

    Client::Render::CreateBuffer(
        device, physicalDevice, bufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, moistureBuffer, moistureBufferMemory);

    Client::Render::CreateBuffer(
        device, physicalDevice, bufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, elevationBuffer, elevationBufferMemory);

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

    // Create descriptor pool and layout for mapping shader
    VkDescriptorPoolSize poolSizes[4] = {};
    for (int i = 0; i < 4; ++i)
    {
        poolSizes[i].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSizes[i].descriptorCount = 1;
    }

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 4;
    poolInfo.pPoolSizes    = poolSizes;
    poolInfo.maxSets       = 1;

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &mappingDescriptorPool) !=
        VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor pool for world mapping");
    }

    // descriptor set layout: binding 0 = noise input, 1=temp,2=moist,3=elev
    VkDescriptorSetLayoutBinding bindings[4] = {};
    for (uint32_t i = 0; i < 4; ++i)
    {
        bindings[i].binding         = i;
        bindings[i].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[i].descriptorCount = 1;
        bindings[i].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 4;
    layoutInfo.pBindings    = bindings;

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr,
                                    &mappingDescriptorSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error(
            "Failed to create descriptor set layout for world mapping");
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = mappingDescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts        = &mappingDescriptorSetLayout;

    if (vkAllocateDescriptorSets(device, &allocInfo, &mappingDescriptorSet) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate descriptor set for world mapping");
    }

    VkDescriptorBufferInfo noiseInfo{};
    noiseInfo.buffer = noiseBuffer;
    noiseInfo.offset = 0;
    noiseInfo.range  = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo tempInfo{};
    tempInfo.buffer = temperatureBuffer;
    tempInfo.offset = 0;
    tempInfo.range  = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo moistInfo{};
    moistInfo.buffer = moistureBuffer;
    moistInfo.offset = 0;
    moistInfo.range  = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo elevInfo{};
    elevInfo.buffer = elevationBuffer;
    elevInfo.offset = 0;
    elevInfo.range  = VK_WHOLE_SIZE;

    VkWriteDescriptorSet writes[4] = {};
    writes[0].sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet               = mappingDescriptorSet;
    writes[0].dstBinding           = 0;
    writes[0].descriptorType       = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[0].descriptorCount      = 1;
    writes[0].pBufferInfo          = &noiseInfo;

    writes[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet          = mappingDescriptorSet;
    writes[1].dstBinding      = 1;
    writes[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[1].descriptorCount = 1;
    writes[1].pBufferInfo     = &tempInfo;

    writes[2].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[2].dstSet          = mappingDescriptorSet;
    writes[2].dstBinding      = 2;
    writes[2].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[2].descriptorCount = 1;
    writes[2].pBufferInfo     = &moistInfo;

    writes[3].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[3].dstSet          = mappingDescriptorSet;
    writes[3].dstBinding      = 3;
    writes[3].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[3].descriptorCount = 1;
    writes[3].pBufferInfo     = &elevInfo;

    vkUpdateDescriptorSets(device, 4, writes, 0, nullptr);

    CreateWorldMappingPipeline(device, mappingDescriptorSetLayout, mappingPipelineLayout,
                               mappingPipeline);
}

void UnitGPUSimplexNoise::GenNoise(VkDevice                         device,
                                   VkCommandBuffer                  commandBuffer,
                                   const SimplexNoisePushConstants& params) const
{
    if (!isInitialized)
    {
        throw std::runtime_error(
            "Simplex noise not initialized. Call Initialize() first");
    }
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, genPipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                            genPipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

    // Push constants (include seed)
    SimplexNoisePushConstants paramsWithSeed = params;
    paramsWithSeed.seed                      = seed;
    vkCmdPushConstants(commandBuffer, genPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
                       sizeof(SimplexNoisePushConstants), &paramsWithSeed);

    // Dispatch compute shader
    const uint32_t workgroupSize = 4;
    uint32_t       workgroupsX   = (params.width + workgroupSize - 1) / workgroupSize;
    uint32_t       workgroupsY   = (params.height + workgroupSize - 1) / workgroupSize;
    uint32_t       workgroupsZ   = (params.depth + workgroupSize - 1) / workgroupSize;

    vkCmdDispatch(commandBuffer, workgroupsX, workgroupsY, workgroupsZ);

    // Add memory barrier to ensure shader writes are complete before next compute stage
    VkBufferMemoryBarrier barrier{};
    barrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.srcAccessMask       = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask       = VK_ACCESS_SHADER_READ_BIT;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.buffer              = noiseBuffer;
    barrier.offset              = 0;
    barrier.size                = VK_WHOLE_SIZE;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 1, &barrier,
                         0, nullptr);
}

void UnitGPUSimplexNoise::GenWorldNoise(VkDevice                       device,
                                        VkCommandBuffer                commandBuffer,
                                        const WorldNoisePushConstants& params) const
{
    if (noiseBuffer == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Noise buffer not created for world mapping");
    }

    if (mappingDescriptorSetLayout == VK_NULL_HANDLE)
    {
        throw std::runtime_error("World mapping descriptor set not created");
    }

    if (mappingPipelineLayout == VK_NULL_HANDLE || mappingPipeline == VK_NULL_HANDLE)
    {
        CreateWorldMappingPipeline(device, mappingDescriptorSetLayout,
                                   mappingPipelineLayout, mappingPipeline);
    }

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mappingPipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                            mappingPipelineLayout, 0, 1, &mappingDescriptorSet, 0,
                            nullptr);

    vkCmdPushConstants(commandBuffer, mappingPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT,
                       0, sizeof(WorldNoisePushConstants), &params);

    const uint32_t workgroupSize = 4;
    uint32_t       workgroupsX   = (params.width + workgroupSize - 1) / workgroupSize;
    uint32_t       workgroupsY   = (params.height + workgroupSize - 1) / workgroupSize;
    uint32_t       workgroupsZ   = (params.depth + workgroupSize - 1) / workgroupSize;

    vkCmdDispatch(commandBuffer, workgroupsX, workgroupsY, workgroupsZ);

    // Barrier to ensure shader writes are visible for next compute stage
    VkBufferMemoryBarrier barriers[3] = {};
    barriers[0].sType                 = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barriers[0].srcAccessMask         = VK_ACCESS_SHADER_WRITE_BIT;
    barriers[0].dstAccessMask         = VK_ACCESS_SHADER_READ_BIT;
    barriers[0].srcQueueFamilyIndex   = VK_QUEUE_FAMILY_IGNORED;
    barriers[0].dstQueueFamilyIndex   = VK_QUEUE_FAMILY_IGNORED;
    barriers[0].buffer                = temperatureBuffer;
    barriers[0].offset                = 0;
    barriers[0].size                  = VK_WHOLE_SIZE;

    barriers[1]        = barriers[0];
    barriers[1].buffer = moistureBuffer;

    barriers[2]        = barriers[0];
    barriers[2].buffer = elevationBuffer;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 3, barriers,
                         0, nullptr);
}

void UnitGPUSimplexNoise::Destroy(VkDevice device)
{
    if (noiseBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(device, noiseBuffer, nullptr);
        vkFreeMemory(device, noiseBufferMemory, nullptr);
        noiseBuffer       = VK_NULL_HANDLE;
        noiseBufferMemory = VK_NULL_HANDLE;
    }

    if (permBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(device, permBuffer, nullptr);
        vkFreeMemory(device, permBufferMemory, nullptr);
        permBuffer       = VK_NULL_HANDLE;
        permBufferMemory = VK_NULL_HANDLE;
    }

    if (permGradIndex3DBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(device, permGradIndex3DBuffer, nullptr);
        vkFreeMemory(device, permGradIndex3DBufferMemory, nullptr);
        permGradIndex3DBuffer       = VK_NULL_HANDLE;
        permGradIndex3DBufferMemory = VK_NULL_HANDLE;
    }

    if (this->mappingPipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(device, mappingPipeline, nullptr);
        mappingPipeline = VK_NULL_HANDLE;
    }

    if (mappingPipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(device, mappingPipelineLayout, nullptr);
        mappingPipelineLayout = VK_NULL_HANDLE;
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
        temperatureBuffer       = VK_NULL_HANDLE;
        temperatureBufferMemory = VK_NULL_HANDLE;
    }
    if (moistureBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(device, moistureBuffer, nullptr);
        vkFreeMemory(device, moistureBufferMemory, nullptr);
        moistureBuffer       = VK_NULL_HANDLE;
        moistureBufferMemory = VK_NULL_HANDLE;
    }
    if (elevationBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(device, elevationBuffer, nullptr);
        vkFreeMemory(device, elevationBufferMemory, nullptr);
        elevationBuffer       = VK_NULL_HANDLE;
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
    if (genPipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(device, genPipeline, nullptr);
        genPipeline = VK_NULL_HANDLE;
    }
    if (genPipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(device, genPipelineLayout, nullptr);
        genPipelineLayout = VK_NULL_HANDLE;
    }
    isInitialized = false;
}

} // namespace Rl::World::Chunk
