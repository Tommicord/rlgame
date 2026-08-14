#include "Rl.Chunk/WorldMeshIndexDedup.h"
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

extern const uint32_t WorldMeshIndexDedupComp_data[];
extern const uint32_t WorldMeshIndexDedupComp_size;

WorldMeshIndexDedup::WorldMeshIndexDedup(const WorldMeshIndexDedupData& data,
                                         IMeshGen&                      meshGen,
                                         GameDeviceInstance&            instance) :
    device(instance.getDevice()), physicalDevice(instance.getPhysicalDevice()),
    graphicsQueue(instance.getGraphicsQueue()), commandPool(instance.getCommandPool()),
    maxVertices(data.maxVertices), maxIndices(data.maxIndices), hashTableSize(data.hashTableSize),
    subdivisions(data.subdivisions), meshGen(meshGen), pipeline(VK_NULL_HANDLE),
    pipelineLayout(VK_NULL_HANDLE), descriptorSet(VK_NULL_HANDLE),
    descriptorSetLayout(VK_NULL_HANDLE), descriptorPool(VK_NULL_HANDLE),
    completionSemaphore(device, GameVulkanSemaphoreCreateInfo{VK_SEMAPHORE_TYPE_BINARY}),
    completionFence(device, GameVulkanFenceCreateInfo{VK_FENCE_CREATE_SIGNALED_BIT}),
    memoryAllocator(device, physicalDevice),
    outputVertexBuffer(&memoryAllocator,
                       sizeof(MeshVertex) * maxVertices,
                       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                           VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
    indexBuffer(&memoryAllocator,
                sizeof(uint32_t) * maxIndices,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                    VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
    countBuffer(&memoryAllocator,
                sizeof(uint32_t) * 2,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
    hashTableBuffer(&memoryAllocator,
                    sizeof(uint32_t) * data.hashTableSize,
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    indexMappingBuffer(&memoryAllocator,
                       sizeof(uint32_t) * maxVertices,
                       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    computeCommandBuffer(device, commandPool)
{
        createDescriptorSets();
        createPipeline();
}

WorldMeshIndexDedup::~WorldMeshIndexDedup()
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

void WorldMeshIndexDedup::createDescriptorSets()
{
        std::array<VkDescriptorPoolSize, 1> poolSizes{};
        poolSizes[0].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSizes[0].descriptorCount = 6;

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

        std::array<VkDescriptorSetLayoutBinding, 6> bindings{};
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

        bindings[5].binding         = 5;
        bindings[5].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[5].descriptorCount = 1;
        bindings[5].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

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
                GameError::exitWithError("WorldMeshIndexDedup", "Opaque handle is null");
        }
        const GameVulkanBuffer& vertexBuffer = *vertexBufferPtr;
#else
        // CPU backend: meshGen may provide CPU-side buffers; fall back to using
        // the opaque handle metadata or throw if not supported.
        const GameOpaqueBufferHandle& vbHandle = meshGen.getVertexBuffer();
        const GameVulkanBuffer*       vertexBufferPtr =
            reinterpret_cast<const GameVulkanBuffer*>(vbHandle.handle);
        if (vertexBufferPtr == nullptr)
        {
                GameError::exitWithError("WorldMeshIndexDedup", "Opaque handle is null");
        }
        const GameVulkanBuffer& vertexBuffer = *vertexBufferPtr;
#endif
        inputVertexBufferInfo.buffer = vertexBuffer.getBuffer();
        inputVertexBufferInfo.offset = 0;
        inputVertexBufferInfo.range  = VK_WHOLE_SIZE;

        VkDescriptorBufferInfo outputVertexBufferInfo{};
        outputVertexBufferInfo.buffer = outputVertexBuffer.getBuffer();
        outputVertexBufferInfo.offset = outputVertexBuffer.getOffset();
        outputVertexBufferInfo.range  = VK_WHOLE_SIZE;

        VkDescriptorBufferInfo indexBufferInfo{};
        indexBufferInfo.buffer = indexBuffer.getBuffer();
        indexBufferInfo.offset = indexBuffer.getOffset();
        indexBufferInfo.range  = VK_WHOLE_SIZE;

        VkDescriptorBufferInfo countBufferInfo{};
        countBufferInfo.buffer = countBuffer.getBuffer();
        countBufferInfo.offset = countBuffer.getOffset();
        countBufferInfo.range  = sizeof(uint32_t) * 2;

        VkDescriptorBufferInfo hashTableBufferInfo{};
        hashTableBufferInfo.buffer = hashTableBuffer.getBuffer();
        hashTableBufferInfo.offset = hashTableBuffer.getOffset();
        hashTableBufferInfo.range  = VK_WHOLE_SIZE;

        VkDescriptorBufferInfo indexMappingBufferInfo{};
        indexMappingBufferInfo.buffer = indexMappingBuffer.getBuffer();
        indexMappingBufferInfo.offset = indexMappingBuffer.getOffset();
        indexMappingBufferInfo.range  = VK_WHOLE_SIZE;

        std::array<VkWriteDescriptorSet, 6> descriptorWrites{};
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
        descriptorWrites[1].pBufferInfo     = &outputVertexBufferInfo;

        descriptorWrites[2].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet          = descriptorSet;
        descriptorWrites[2].dstBinding      = 2;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].pBufferInfo     = &indexBufferInfo;

        descriptorWrites[3].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[3].dstSet          = descriptorSet;
        descriptorWrites[3].dstBinding      = 3;
        descriptorWrites[3].dstArrayElement = 0;
        descriptorWrites[3].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[3].descriptorCount = 1;
        descriptorWrites[3].pBufferInfo     = &countBufferInfo;

        descriptorWrites[4].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[4].dstSet          = descriptorSet;
        descriptorWrites[4].dstBinding      = 4;
        descriptorWrites[4].dstArrayElement = 0;
        descriptorWrites[4].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[4].descriptorCount = 1;
        descriptorWrites[4].pBufferInfo     = &hashTableBufferInfo;

        descriptorWrites[5].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[5].dstSet          = descriptorSet;
        descriptorWrites[5].dstBinding      = 5;
        descriptorWrites[5].dstArrayElement = 0;
        descriptorWrites[5].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[5].descriptorCount = 1;
        descriptorWrites[5].pBufferInfo     = &indexMappingBufferInfo;

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()),
                               descriptorWrites.data(), 0, nullptr);
}

void WorldMeshIndexDedup::createPipeline()
{
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        pushConstantRange.offset     = 0;
        pushConstantRange.size       = sizeof(WorldMeshIndexDedupPushConstants);

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

        computeShader = GameShaderLoader::createShaderModule(device, WorldMeshIndexDedupComp_data,
                                                             WorldMeshIndexDedupComp_size);

        VkPipelineShaderStageCreateInfo shaderStageInfo{};
        shaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageInfo.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
        shaderStageInfo.module = computeShader.shaderModule;
        shaderStageInfo.pName  = "main";

        VkComputePipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.stage  = shaderStageInfo;

        result =
            vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline);
        if (result != VK_SUCCESS)
        {
                GameError::exitWithError("vkCreateComputePipelines",
                                         "Failed to create compute pipeline (result = " +
                                             GameError::vulkanResultToString(result) + ")");
        }
}

void WorldMeshIndexDedup::dispatch(void*                      pResource,
                                   const GameVulkanSemaphore& waitSemaphore,
                                   GameVulkanFence&           fence)
{
        WorldMeshIndexDedupPResource* resource =
            static_cast<WorldMeshIndexDedupPResource*>(pResource);
        WorldMeshIndexDedupPushConstants* params = resource->pParams;

        GameVulkanCommandBuffer readCmd(device, commandPool);
        readCmd.reset();
        readCmd.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        VkBufferMemoryBarrier countReadBarrier{};
        countReadBarrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        countReadBarrier.srcAccessMask       = VK_ACCESS_SHADER_WRITE_BIT;
        countReadBarrier.dstAccessMask       = VK_ACCESS_HOST_READ_BIT;
        countReadBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        countReadBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

#if defined(_RL_CHUNK_VULKAN_BACKEND)
        const GameVulkanBuffer& meshCountBuffer =
            *reinterpret_cast<GameVulkanBuffer*>(meshGen.getCountBuffer().handle);
#else
        const GameOpaqueBufferHandle& cbHandle = meshGen.getCountBuffer();
        const GameVulkanBuffer*       meshCountBufferPtr =
            reinterpret_cast<const GameVulkanBuffer*>(cbHandle.handle);
        const GameVulkanBuffer& meshCountBuffer =
            meshCountBufferPtr ? *meshCountBufferPtr
                               : *reinterpret_cast<const GameVulkanBuffer*>(nullptr);
#endif
        countReadBarrier.buffer = meshCountBuffer.getBuffer();
        countReadBarrier.offset = 0;
        countReadBarrier.size   = sizeof(uint32_t);

        vkCmdPipelineBarrier(readCmd.getCommandBuffer(), VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             VK_PIPELINE_STAGE_HOST_BIT, 0, 0, nullptr, 1, &countReadBarrier, 0,
                             nullptr);
        readCmd.end();

        VkDeviceSize countBufferSize = sizeof(uint32_t) * 2;

        VkSubmitInfo submitInfo1{};
        submitInfo1.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo1.commandBufferCount = 1;
        const VkCommandBuffer cmdBuff  = readCmd.getCommandBuffer();
        submitInfo1.pCommandBuffers    = &cmdBuff;

        GameVulkanQueueSubmitter::submit(graphicsQueue, &submitInfo1, fence.getFence());
        fence.wait();
        fence.reset();

        uint32_t actualInputVertexCount = 0;
        meshGen.readVertexCount(actualInputVertexCount);

        VkDeviceSize bufferSize = sizeof(uint32_t);
        void*        data       = nullptr;

        params->inputVertexCount = actualInputVertexCount;
        params->maxVertices      = maxVertices;
        params->maxIndices       = maxIndices;
        params->hashTableSize    = hashTableSize;
        params->subdivisions     = subdivisions;

        std::scoped_lock lock(generateMutex);

        completionFence.wait();
        completionFence.reset();

        computeCommandBuffer.reset();
        computeCommandBuffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        vkCmdFillBuffer(computeCommandBuffer.getCommandBuffer(), countBuffer.getBuffer(), 0,
                        countBufferSize, 0);

        vkCmdFillBuffer(computeCommandBuffer.getCommandBuffer(), outputVertexBuffer.getBuffer(),
                        outputVertexBuffer.getOffset(), outputVertexBuffer.getSize(), 0);

        vkCmdFillBuffer(computeCommandBuffer.getCommandBuffer(), indexBuffer.getBuffer(),
                        indexBuffer.getOffset(), indexBuffer.getSize(), 0);

        vkCmdFillBuffer(computeCommandBuffer.getCommandBuffer(), hashTableBuffer.getBuffer(),
                        hashTableBuffer.getOffset(), hashTableBuffer.getSize(), 0xFFFFFFFF);

        if (indexMappingBuffer.getSize() > 0)
        {
                vkCmdFillBuffer(computeCommandBuffer.getCommandBuffer(),
                                indexMappingBuffer.getBuffer(), indexMappingBuffer.getOffset(),
                                indexMappingBuffer.getSize(), 0);
        }

        std::array<VkBufferMemoryBarrier, 5> fillBarriers{};
        // Count buffer barrier
        fillBarriers[0].sType         = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        fillBarriers[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        fillBarriers[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        fillBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        fillBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        fillBarriers[0].buffer              = countBuffer.getBuffer();
        fillBarriers[0].offset              = 0;
        fillBarriers[0].size                = countBufferSize;

        // Output vertex buffer barrier
        fillBarriers[1].sType         = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        fillBarriers[1].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        fillBarriers[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        fillBarriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        fillBarriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        fillBarriers[1].buffer              = outputVertexBuffer.getBuffer();
        fillBarriers[1].offset              = outputVertexBuffer.getOffset();
        fillBarriers[1].size                = outputVertexBuffer.getSize();

        // Index buffer barrier
        fillBarriers[2].sType         = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        fillBarriers[2].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        fillBarriers[2].dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        fillBarriers[2].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        fillBarriers[2].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        fillBarriers[2].buffer              = indexBuffer.getBuffer();
        fillBarriers[2].offset              = indexBuffer.getOffset();
        fillBarriers[2].size                = indexBuffer.getSize();

        // Hash table buffer barrier
        fillBarriers[3].sType         = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        fillBarriers[3].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        fillBarriers[3].dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        fillBarriers[3].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        fillBarriers[3].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        fillBarriers[3].buffer              = hashTableBuffer.getBuffer();
        fillBarriers[3].offset              = hashTableBuffer.getOffset();
        fillBarriers[3].size                = hashTableBuffer.getSize();

        uint32_t barrierCount = 4;
        if (indexMappingBuffer.getSize() > 0)
        {
                fillBarriers[4].sType         = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                fillBarriers[4].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                fillBarriers[4].dstAccessMask =
                    VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
                fillBarriers[4].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                fillBarriers[4].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                fillBarriers[4].buffer              = indexMappingBuffer.getBuffer();
                fillBarriers[4].offset              = indexMappingBuffer.getOffset();
                fillBarriers[4].size                = indexMappingBuffer.getSize();
                barrierCount                        = 5;
        }

        vkCmdPipelineBarrier(computeCommandBuffer.getCommandBuffer(),
                             VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             0, 0, nullptr, barrierCount, fillBarriers.data(), 0, nullptr);

        vkCmdBindPipeline(computeCommandBuffer.getCommandBuffer(), VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipeline);
        vkCmdBindDescriptorSets(computeCommandBuffer.getCommandBuffer(),
                                VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1,
                                &descriptorSet, 0, nullptr);
        vkCmdPushConstants(computeCommandBuffer.getCommandBuffer(), pipelineLayout,
                           VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(WorldMeshIndexDedupPushConstants),
                           params);

        uint32_t groupCountX = (actualInputVertexCount + 63) / 64;
        if (groupCountX == 0)
                groupCountX = 1;

        vkCmdDispatch(computeCommandBuffer.getCommandBuffer(), groupCountX, 1, 1);

        computeCommandBuffer.end();

        VkSubmitInfo submitInfo2{};
        submitInfo2.sType               = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo2.commandBufferCount  = 1;
        const VkCommandBuffer cmdBuffer = computeCommandBuffer.getCommandBuffer();
        submitInfo2.pCommandBuffers     = &cmdBuffer;
        VkPipelineStageFlags waitStage  = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        const VkSemaphore    waitSem    = waitSemaphore.getSemaphore();
        if (waitSem != VK_NULL_HANDLE)
        {
                submitInfo2.waitSemaphoreCount = 1;
                submitInfo2.pWaitSemaphores    = &waitSem;
                submitInfo2.pWaitDstStageMask  = &waitStage;
        }
        else
        {
                submitInfo2.waitSemaphoreCount = 0;
                submitInfo2.pWaitSemaphores    = nullptr;
                submitInfo2.pWaitDstStageMask  = nullptr;
        }
        const VkSemaphore semaphore = completionSemaphore.getSemaphore();
        if (semaphore != VK_NULL_HANDLE)
        {
                submitInfo2.signalSemaphoreCount = 1;
                submitInfo2.pSignalSemaphores    = &semaphore;
        }
        else
        {
                submitInfo2.signalSemaphoreCount = 0;
                submitInfo2.pSignalSemaphores    = nullptr;
        }

        completionFence.reset();
        GameVulkanQueueSubmitter::submit(graphicsQueue, &submitInfo2, completionFence.getFence());
}

std::recursive_mutex& WorldMeshIndexDedup::getGenerateMutex()
{
        return generateMutex;
}

const GameVulkanSemaphore& WorldMeshIndexDedup::getCompletionSemaphore() const
{
        return completionSemaphore;
}

GameVulkanSemaphore& WorldMeshIndexDedup::getCompletionSemaphore()
{
        return completionSemaphore;
}

const GameVulkanFence& WorldMeshIndexDedup::getCompletionFence() const
{
        return completionFence;
}

GameVulkanFence& WorldMeshIndexDedup::getCompletionFence()
{
        return completionFence;
}

GameVulkanBuffer& WorldMeshIndexDedup::getOutputVertexBuffer()
{
        return outputVertexBuffer;
}

GameVulkanBuffer& WorldMeshIndexDedup::getIndexBuffer()
{
        return indexBuffer;
}

GameVulkanBuffer& WorldMeshIndexDedup::getCountBuffer()
{
        return countBuffer;
}

void WorldMeshIndexDedup::readIndices(VkDevice         device,
                                      VkPhysicalDevice physicalDevice,
                                      uint32_t*        pOutput,
                                      const size_t     outputSize)
{
        void* data = nullptr;
        vkMapMemory(device, indexBuffer.getMemory(), indexBuffer.getOffset(), outputSize, 0, &data);
        memcpy(pOutput, data, outputSize);
        vkUnmapMemory(device, indexBuffer.getMemory());
}

void WorldMeshIndexDedup::readCounts(VkDevice         device,
                                     VkPhysicalDevice physicalDevice,
                                     uint32_t&        pVertexCount,
                                     uint32_t&        pIndexCount)
{
        void*    data      = nullptr;
        uint32_t counts[2] = {0, 0};
        vkMapMemory(device, indexBuffer.getMemory(), indexBuffer.getOffset(), sizeof(counts), 0,
                    &data);
        memcpy(counts, data, sizeof(counts));
        pVertexCount = counts[0];
        pIndexCount  = counts[1];
        vkUnmapMemory(device, indexBuffer.getMemory());
}

} // namespace rl
