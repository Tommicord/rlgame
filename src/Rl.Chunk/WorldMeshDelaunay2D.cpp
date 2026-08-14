#include "Rl.Chunk/WorldMeshDelaunay2D.h"
#include "Rl.Chunk/WorldMeshGen.h"

#include "Rl.Base/GameError.h"
#include "Rl.Base/GameVulkanAllocator.h"
#include "Rl.Base/GameVulkanQueueSubmitter.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vulkan/vulkan.hpp>

namespace rl
{

extern const uint32_t WorldMeshDelaunay2DComp_data[];
extern const uint32_t WorldMeshDelaunay2DComp_size;

WorldMeshDelaunay2D::WorldMeshDelaunay2D(const WorldMeshDelaunay2DData& data,
                                         IMeshGen&                      meshGen,
                                         GameDeviceInstance&            instance) :
    device(instance.getDevice()), physicalDevice(instance.getPhysicalDevice()),
    graphicsQueue(instance.getGraphicsQueue()), commandPool(instance.getCommandPool()),
    maxIndices(data.maxIndices), subdivisions(data.subdivisions), faceCount(data.faceCount),
    maxIterations(data.maxIterations), meshGen(meshGen), pipeline(VK_NULL_HANDLE),
    pipelineLayout(VK_NULL_HANDLE), descriptorSet(VK_NULL_HANDLE),
    descriptorSetLayout(VK_NULL_HANDLE), descriptorPool(VK_NULL_HANDLE),
    completionSemaphore(device, GameVulkanSemaphoreCreateInfo{VK_SEMAPHORE_TYPE_BINARY}),
    completionFence(device, GameVulkanFenceCreateInfo{VK_FENCE_CREATE_SIGNALED_BIT}),
    memoryAllocator(device, physicalDevice),
    indexBuffer(&memoryAllocator,
                sizeof(uint32_t) * maxIndices,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                    VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
    countBuffer(&memoryAllocator,
                sizeof(uint32_t),
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
    faceStartBuffer(&memoryAllocator,
                    sizeof(uint32_t) * faceCount,
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
    edgeFlipBuffer(&memoryAllocator,
                   sizeof(uint32_t),
                   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    computeCommandBuffer(device, commandPool)
{
        indexBufferHandle.setHandle(&indexBuffer);
        indexBufferHandle.setSize(sizeof(uint32_t) * maxIndices);
        indexBufferHandle.setOffset(0);
        indexBufferHandle.setUsage(GameOpaqueBufferUsage::StorageBuffer | GameOpaqueBufferUsage::TransferSrc | GameOpaqueBufferUsage::TransferDst);
        indexBufferHandle.setMemoryProperty(GameOpaqueBufferMemoryProperty::HostVisible | GameOpaqueBufferMemoryProperty::HostCoherent);

        countBufferHandle.setHandle(&countBuffer);
        countBufferHandle.setSize(sizeof(uint32_t));
        countBufferHandle.setOffset(0);
        countBufferHandle.setUsage(GameOpaqueBufferUsage::StorageBuffer | GameOpaqueBufferUsage::TransferDst);
        countBufferHandle.setMemoryProperty(GameOpaqueBufferMemoryProperty::HostVisible | GameOpaqueBufferMemoryProperty::HostCoherent);

        completionHandle.setHandle(&completionSemaphore);
        completionHandle.setType(GameOpaqueSyncHandleType::Semaphore);
        completionHandle.setState(GameOpaqueSyncHandleState::Unsignaled);

        createFaceStartBuffer();
        createDescriptorSets();
        createPipeline();
}

WorldMeshDelaunay2D::~WorldMeshDelaunay2D()
{
        completionFence.wait();

        if (pipeline != VK_NULL_HANDLE)
        {
                vkDestroyPipeline(device, pipeline, nullptr);
        }
        if (pipelineLayout != VK_NULL_HANDLE)
        {
                vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        }
        if (descriptorPool != VK_NULL_HANDLE)
        {
                vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        }
        if (descriptorSetLayout != VK_NULL_HANDLE)
        {
                vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        }
}

void WorldMeshDelaunay2D::createFaceStartBuffer()
{
        uint32_t vertsPerEdge    = subdivisions + 1;
        uint32_t faceVertexCount = vertsPerEdge * vertsPerEdge;

        std::vector<uint32_t> faceStartIndices(faceCount);
        for (uint32_t i = 0; i < faceCount; ++i)
        {
                faceStartIndices[i] = i * faceVertexCount;
        }

        VkDeviceMemory memory = faceStartBuffer.getMemory();
        VkDeviceSize offset = faceStartBuffer.getOffset();
        void* data;
        VkResult result = vkMapMemory(device, memory, offset, sizeof(uint32_t) * faceCount, 0, &data);
        if (result != VK_SUCCESS)
        {
                GameError::exitWithError("vkMapMemory",
                                         "Failed to map face start buffer (result = " +
                                             GameError::vulkanResultToString(result) + ")");
        }
        memcpy(data, faceStartIndices.data(), sizeof(uint32_t) * faceCount);
        vkUnmapMemory(device, memory);
}

void WorldMeshDelaunay2D::createDescriptorSets()
{
        std::array<VkDescriptorPoolSize, 1> poolSizes{};
        poolSizes[0].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSizes[0].descriptorCount = 5;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.maxSets       = 1;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes    = poolSizes.data();

        VkResult result = vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool);
        if (result != VK_SUCCESS)
        {
                GameError::exitWithError("vkCreateDescriptorPool",
                                         "Failed to create descriptor pool (result = " +
                                             GameError::vulkanResultToString(result) + ")");
        }

        std::array<VkDescriptorSetLayoutBinding, 5> bindings{};
        bindings[0].binding         = 0;
        bindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

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

        bindings[4].binding         = 4;
        bindings[4].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[4].descriptorCount = 1;
        bindings[4].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings    = bindings.data();

        result = vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout);
        if (result != VK_SUCCESS)
        {
                GameError::exitWithError("vkCreateDescriptorSetLayout",
                                         "Failed to create descriptor set layout (result = " +
                                             GameError::vulkanResultToString(result) + ")");
        }

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool     = descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts        = &descriptorSetLayout;

        result = vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet);
        if (result != VK_SUCCESS)
        {
                GameError::exitWithError("vkAllocateDescriptorSets",
                                         "Failed to allocate descriptor sets (result = " +
                                             GameError::vulkanResultToString(result) + ")");
        }

        VkDescriptorBufferInfo inputVertexBufferInfo{};
#if defined(_RL_CHUNK_VULKAN_BACKEND)
        const GameVulkanBuffer* vertexBufferPtr =
            reinterpret_cast<GameVulkanBuffer*>(meshGen.getVertexBuffer().handle);
        if (vertexBufferPtr == nullptr)
        {
                GameError::exitWithError("WorldMeshDelaunay2D", "Opaque handle is null");
        }
        const GameVulkanBuffer& vertexBuffer = *vertexBufferPtr;
#else
        const GameOpaqueBufferHandle& vbHandle = meshGen.getVertexBuffer();
        const GameVulkanBuffer*       vertexBufferPtr =
            reinterpret_cast<const GameVulkanBuffer*>(vbHandle.handle);
        if (vertexBufferPtr == nullptr)
        {
                GameError::exitWithError("WorldMeshDelaunay2D", "Opaque handle is null");
        }
        const GameVulkanBuffer& vertexBuffer = *vertexBufferPtr;
#endif
        inputVertexBufferInfo.buffer = vertexBuffer.getBuffer();
        inputVertexBufferInfo.offset = 0;
        inputVertexBufferInfo.range  = VK_WHOLE_SIZE;

        VkDescriptorBufferInfo indexBufferInfo{};
        indexBufferInfo.buffer = indexBuffer.getBuffer();
        indexBufferInfo.offset = indexBuffer.getOffset();
        indexBufferInfo.range  = VK_WHOLE_SIZE;

        VkDescriptorBufferInfo countBufferInfo{};
        countBufferInfo.buffer = countBuffer.getBuffer();
        countBufferInfo.offset = countBuffer.getOffset();
        countBufferInfo.range  = sizeof(uint32_t);

        VkDescriptorBufferInfo faceStartBufferInfo{};
        faceStartBufferInfo.buffer = faceStartBuffer.getBuffer();
        faceStartBufferInfo.offset = faceStartBuffer.getOffset();
        faceStartBufferInfo.range  = VK_WHOLE_SIZE;

        VkDescriptorBufferInfo edgeFlipBufferInfo{};
        edgeFlipBufferInfo.buffer = edgeFlipBuffer.getBuffer();
        edgeFlipBufferInfo.offset = edgeFlipBuffer.getOffset();
        edgeFlipBufferInfo.range  = sizeof(uint32_t);

        std::array<VkWriteDescriptorSet, 5> descriptorWrites{};
        descriptorWrites[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet          = descriptorSet;
        descriptorWrites[0].dstBinding      = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo     = &inputVertexBufferInfo;

        descriptorWrites[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet          = descriptorSet;
        descriptorWrites[1].dstBinding      = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pBufferInfo     = &indexBufferInfo;

        descriptorWrites[2].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet          = descriptorSet;
        descriptorWrites[2].dstBinding      = 2;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].pBufferInfo     = &countBufferInfo;

        descriptorWrites[3].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[3].dstSet          = descriptorSet;
        descriptorWrites[3].dstBinding      = 3;
        descriptorWrites[3].dstArrayElement = 0;
        descriptorWrites[3].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[3].descriptorCount = 1;
        descriptorWrites[3].pBufferInfo     = &faceStartBufferInfo;

        descriptorWrites[4].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[4].dstSet          = descriptorSet;
        descriptorWrites[4].dstBinding      = 4;
        descriptorWrites[4].dstArrayElement = 0;
        descriptorWrites[4].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[4].descriptorCount = 1;
        descriptorWrites[4].pBufferInfo     = &edgeFlipBufferInfo;

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()),
                               descriptorWrites.data(), 0, nullptr);
}

void WorldMeshDelaunay2D::createPipeline()
{
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        pushConstantRange.offset     = 0;
        pushConstantRange.size       = sizeof(WorldMeshDelaunay2DPushConstants);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount         = 1;
        pipelineLayoutInfo.pSetLayouts            = &descriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges    = &pushConstantRange;

        VkResult result =
            vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout);
        if (result != VK_SUCCESS)
        {
                GameError::exitWithError("vkCreatePipelineLayout",
                                         "Failed to create pipeline layout (result = " +
                                             GameError::vulkanResultToString(result) + ")");
        }

        computeShader = GameShaderLoader::createShaderModule(device, WorldMeshDelaunay2DComp_data,
                                                             WorldMeshDelaunay2DComp_size);

        VkPipelineShaderStageCreateInfo shaderStageInfo{};
        shaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageInfo.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
        shaderStageInfo.module = computeShader.shaderModule;
        shaderStageInfo.pName  = "main";

        VkComputePipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.stage  = shaderStageInfo;
        pipelineInfo.layout = pipelineLayout;

        result =
            vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline);
        if (result != VK_SUCCESS)
        {
                GameError::exitWithError("vkCreateComputePipelines",
                                         "Failed to create compute pipeline (result = " +
                                             GameError::vulkanResultToString(result) + ")");
        }
}

void WorldMeshDelaunay2D::dispatch(void*                      pResource,
                                   const GameVulkanSemaphore& waitSemaphore,
                                   GameVulkanFence&           fence)
{
        WorldMeshDelaunay2DPResource* resource =
            static_cast<WorldMeshDelaunay2DPResource*>(pResource);
        WorldMeshDelaunay2DPushConstants* params = resource->pParams;

        completionFence.wait();
        completionFence.reset();

        uint32_t zero = 0;
        VkDeviceMemory countMemory = countBuffer.getMemory();
        VkDeviceSize countOffset = countBuffer.getOffset();
        void* countData;
        VkResult result = vkMapMemory(device, countMemory, countOffset, sizeof(uint32_t), 0, &countData);
        if (result != VK_SUCCESS)
        {
                GameError::exitWithError("vkMapMemory",
                                         "Failed to map count buffer (result = " +
                                             GameError::vulkanResultToString(result) + ")");
        }
        memcpy(countData, &zero, sizeof(uint32_t));
        vkUnmapMemory(device, countMemory);

        VkDeviceMemory edgeFlipMemory = edgeFlipBuffer.getMemory();
        VkDeviceSize edgeFlipOffset = edgeFlipBuffer.getOffset();
        void* edgeFlipData;
        result = vkMapMemory(device, edgeFlipMemory, edgeFlipOffset, sizeof(uint32_t), 0, &edgeFlipData);
        if (result != VK_SUCCESS)
        {
                GameError::exitWithError("vkMapMemory",
                                         "Failed to map edge flip buffer (result = " +
                                             GameError::vulkanResultToString(result) + ")");
        }
        memcpy(edgeFlipData, &zero, sizeof(uint32_t));
        vkUnmapMemory(device, edgeFlipMemory);

        computeCommandBuffer.begin();
        VkCommandBuffer cmdBuffer = computeCommandBuffer.getCommandBuffer();
        vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0,
                                                1, &descriptorSet, 0, nullptr);
        vkCmdPushConstants(cmdBuffer, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
                                           sizeof(WorldMeshDelaunay2DPushConstants), params);
        vkCmdDispatch(cmdBuffer, faceCount, 1, 1);
        computeCommandBuffer.end();

        GameVulkanQueueSubmitter::submit(graphicsQueue, computeCommandBuffer.getCommandBuffer(),
                                         waitSemaphore, completionSemaphore,
                                         completionFence.getFence());
}

std::recursive_mutex& WorldMeshDelaunay2D::getGenerateMutex()
{
        return generateMutex;
}

const GameVulkanSemaphore& WorldMeshDelaunay2D::getCompletionSemaphore() const
{
        return completionSemaphore;
}

GameVulkanSemaphore& WorldMeshDelaunay2D::getCompletionSemaphore()
{
        return completionSemaphore;
}

const GameVulkanFence& WorldMeshDelaunay2D::getCompletionFence() const
{
        return completionFence;
}

GameVulkanFence& WorldMeshDelaunay2D::getCompletionFence()
{
        return completionFence;
}

GameVulkanBuffer& WorldMeshDelaunay2D::getIndexBuffer()
{
        return indexBuffer;
}

GameVulkanBuffer& WorldMeshDelaunay2D::getCountBuffer()
{
        return countBuffer;
}

void WorldMeshDelaunay2D::readIndices(VkDevice         device,
                                      VkPhysicalDevice physicalDevice,
                                      uint32_t*        pOutput,
                                      const size_t     outputSize)
{
        VkDeviceMemory memory = indexBuffer.getMemory();
        VkDeviceSize offset = indexBuffer.getOffset();
        void* data;
        VkResult result = vkMapMemory(device, memory, offset, outputSize, 0, &data);
        if (result != VK_SUCCESS)
        {
                GameError::exitWithError("vkMapMemory",
                                         "Failed to map index buffer (result = " +
                                             GameError::vulkanResultToString(result) + ")");
        }
        memcpy(pOutput, data, outputSize);
        vkUnmapMemory(device, memory);
}

void WorldMeshDelaunay2D::readCounts(VkDevice         device,
                                     VkPhysicalDevice physicalDevice,
                                     uint32_t&        pIndexCount)
{
        VkDeviceMemory memory = countBuffer.getMemory();
        VkDeviceSize offset = countBuffer.getOffset();
        void* data;
        VkResult result = vkMapMemory(device, memory, offset, sizeof(uint32_t), 0, &data);
        if (result != VK_SUCCESS)
        {
                GameError::exitWithError("vkMapMemory",
                                         "Failed to map count buffer (result = " +
                                             GameError::vulkanResultToString(result) + ")");
        }
        memcpy(&pIndexCount, data, sizeof(uint32_t));
        vkUnmapMemory(device, memory);
}

// IMeshDelaunay2D interface implementation
const GameOpaqueBufferHandle& WorldMeshDelaunay2D::getIndexBuffer() const
{
        return indexBufferHandle;
}

const GameOpaqueBufferHandle& WorldMeshDelaunay2D::getCountBuffer() const
{
        return countBufferHandle;
}

const GameOpaqueSyncHandle& WorldMeshDelaunay2D::getCompletionHandle() const
{
        return completionHandle;
}

void WorldMeshDelaunay2D::readIndices(uint32_t* pOutput, const size_t outputSize)
{
        readIndices(device, physicalDevice, pOutput, outputSize);
}

void WorldMeshDelaunay2D::readCounts(uint32_t& pIndexCount)
{
        readCounts(device, physicalDevice, pIndexCount);
}

#if defined(_RL_CHUNK_VULKAN_BACKEND)
void* WorldMeshDelaunay2D::getIndexBufferPtr() const
{
        return const_cast<GameVulkanBuffer*>(&indexBuffer);
}

void* WorldMeshDelaunay2D::getCountBufferPtr() const
{
        return const_cast<GameVulkanBuffer*>(&countBuffer);
}
#endif

} // namespace rl
