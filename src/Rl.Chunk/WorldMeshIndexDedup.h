#ifndef RL_CHUNK_WORLD_MESH_INDEX_DEDUP_H
#define RL_CHUNK_WORLD_MESH_INDEX_DEDUP_H

#include "Rl.Base/GameVulkanBuffer.h"
#include "Rl.Base/GameVulkanCommandBuffer.h"
#include "Rl.Base/GameVulkanSemaphore.h"
#include "Rl.Base/GameVulkanFence.h"
#include "Rl.Base/GameShaderModule.h"
#include "Rl.Base/IGameComputeDispatch.h"
#include "Rl.Base/GameDevice.h"

#include "Rl.Chunk/IMeshGen.h"

#include <cstdint>
#include <mutex>
#include <vulkan/vulkan.hpp>

namespace rl
{

/** @brief Configuration parameters for WorldMeshIndexDedup */
struct WorldMeshIndexDedupData
{
                uint32_t maxVertices; /**< Maximum vertices in output */
                uint32_t maxIndices; /**< Maximum indices in output */
                uint32_t hashTableSize; /**< Size of hash table (should be power of 2) */
                uint32_t subdivisions; /**< Subdivision level for triangulation */
};

/** @brief Push constants for index deduplication compute shader */
struct WorldMeshIndexDedupPushConstants
{
                uint32_t inputVertexCount;
                uint32_t maxVertices;
                uint32_t maxIndices;
                uint32_t hashTableSize;
                uint32_t subdivisions;
                uint32_t _padding[3];
};

/** @brief Resource pointer when dispatching WorldMeshIndexDedup */
struct WorldMeshIndexDedupPResource
{
                WorldMeshIndexDedupPushConstants* pParams;
};

/** @brief GPU-accelerated vertex deduplication and index generation using Vulkan compute shaders */
class WorldMeshIndexDedup : public IGameComputeDispatch
{
        public:
                /** @brief Constructs an index deduplication processor
                 * @param data Configuration parameters
                 * @param meshGen Reference to IMeshGen for vertex buffer and count
                 * @param instance Vulkan device instance */
                WorldMeshIndexDedup(const WorldMeshIndexDedupData& data,
                                    IMeshGen&                      meshGen,
                                    GameDeviceInstance&        instance);
                /** @brief Destroys the deduplication processor */
                ~WorldMeshIndexDedup();
                WorldMeshIndexDedup(const WorldMeshIndexDedup& other)            = delete;
                WorldMeshIndexDedup& operator=(const WorldMeshIndexDedup& other) = delete;

                /** @brief Returns the generate mutex for external synchronization
                 * @return Reference to the generate mutex */
                std::recursive_mutex& getGenerateMutex() override;

                /** @brief Returns the completion semaphore
                 * @return Reference to the completion semaphore */
                const GameVulkanSemaphore& getCompletionSemaphore() const override;
                GameVulkanSemaphore&       getCompletionSemaphore() override;

                /** @brief Returns the completion fence
                 * @return Reference to the completion fence */
                const GameVulkanFence& getCompletionFence() const;
                GameVulkanFence&       getCompletionFence();

                GameVulkanBuffer& getOutputVertexBuffer();
                GameVulkanBuffer& getIndexBuffer();
                GameVulkanBuffer& getCountBuffer();

                /** @brief Reads the index buffer data from GPU
                 * @param device Vulkan device
                 * @param physicalDevice Physical device
                 * @param pOutput Output pointer for index data
                 * @param outputSize Size of output buffer in bytes */
                void readIndices(VkDevice         device,
                                 VkPhysicalDevice physicalDevice,
                                 uint32_t*        pOutput,
                                 const size_t     outputSize);
                /** @brief Reads the count buffer data from GPU
                 * @param device Vulkan device
                 * @param physicalDevice Physical device
                 * @param pVertexCount Output pointer for vertex count
                 * @param pIndexCount Output pointer for index count */
                void readCounts(VkDevice         device,
                                VkPhysicalDevice physicalDevice,
                                uint32_t&        pVertexCount,
                                uint32_t&        pIndexCount);

        protected:
                void dispatch(void*                      pResource,
                              const GameVulkanSemaphore& waitSemaphore,
                              GameVulkanFence&           fence) override;

        private:
                void createDescriptorSets();
                void createPipeline();

                VkDevice         device;
                VkPhysicalDevice physicalDevice;
                VkQueue          graphicsQueue;
                VkCommandPool    commandPool;

                uint32_t maxVertices;
                uint32_t maxIndices;
                uint32_t hashTableSize;
                uint32_t subdivisions;

                IMeshGen& meshGen;

                GameShaderModule      computeShader;
                VkPipeline            pipeline;
                VkPipelineLayout      pipelineLayout;
                VkDescriptorSet       descriptorSet;
                VkDescriptorSetLayout descriptorSetLayout;
                VkDescriptorPool      descriptorPool;

                GameVulkanMemoryAllocator memoryAllocator;
                GameVulkanBuffer          outputVertexBuffer;
                GameVulkanBuffer          indexBuffer;
                GameVulkanBuffer          countBuffer;
                GameVulkanBuffer          hashTableBuffer;
                GameVulkanBuffer          indexMappingBuffer;

                GameVulkanCommandBuffer computeCommandBuffer;

                GameVulkanSemaphore completionSemaphore;
                GameVulkanFence     completionFence;

                std::recursive_mutex generateMutex;
};

} // namespace rl

#endif // RL_CHUNK_WORLD_MESH_INDEX_DEDUP_H
