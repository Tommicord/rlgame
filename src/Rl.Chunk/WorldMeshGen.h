#ifndef RL_CHUNK_WORLD_MESH_GEN_H
#define RL_CHUNK_WORLD_MESH_GEN_H

#include "Rl.Base/GameMatrix.h"
#include "Rl.Base/GameShaderModule.h"
#include "Rl.Base/GameVulkanBuffer.h"
#include "Rl.Base/GameVulkanCommandBuffer.h"
#include "Rl.Base/GameVulkanSemaphore.h"
#include "Rl.Base/GameVulkanCommandPool.h"
#include "Rl.Base/IGameComputeDispatch.h"
#include "Rl.Base/GameVulkanFence.h"
#include "Rl.Base/GameDevice.h"
#include "Rl.Base/GameOpaqueBufferHandle.h"
#include "Rl.Base/GameOpaqueSyncHandle.h"

#include "Rl.Chunk/IMeshGen.h"
#include "Rl.Chunk/IMeshTess.h"
#include "Rl.World/PreUnitRegister.h"
#include "Rl.World/PreUnitRegistry.h"
#include "Rl.World/Unit.h"

#include <cstdint>
#include <mutex>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace rl
{

/** @brief Output vertex for triangulation */
struct MeshVertex
{
                Vec3 position;
                Vec3 normal;
                Vec2 uv;
};

/** @brief Configuration parameters for WorldMeshGen */
struct WorldMeshGenData
{
                uint32_t width; /**< Output width */
                uint32_t height; /**< Output height */
                uint32_t depth; /**< Output depth */
                uint32_t maxVertices; /**< Maximum vertices in output */
                uint32_t maxIndices; /**< Maximum indices in output */
                uint32_t subdivisions; /**< Face subdivision level */
                uint32_t vertexBufferSize; /**< Vertex buffer size including count field */
                uint32_t indexBufferSize; /**< Index buffer size including count field */
};

/** @brief Push constants for mesh generation compute shader */
struct WorldMeshGenPushConstants
{
                uint32_t width;
                uint32_t height;
                uint32_t depth;
                uint32_t maxVertices;
                uint32_t maxIndices;
                uint32_t subdivisions;
                uint32_t _padding[2];
};

/** @brief Resource pointer when dispatching WorldMeshGen */
struct WorldMeshGenPResource
{
                WorldMeshGenPushConstants* pParams;
};
class WorldMeshTess;

/** @brief GPU-accelerated mesh generation using Vulkan compute shaders */
class WorldMeshGen : public IGameComputeDispatch,
                     public IMeshGen
{
        public:
                /** @brief Constructs a mesh generator that reads from IMeshTess GPU output
                 * @param data Configuration parameters
                 * @param meshTess Reference to IMeshTess to read PostUnit data from GPU
                 * @param instance Vulkan device instance */
                WorldMeshGen(const WorldMeshGenData& data,
                             IMeshTess&              meshTess,
                             GameDeviceInstance&     instance);
                /** @brief Destroys the mesh generator */
                ~WorldMeshGen();
                WorldMeshGen(const WorldMeshGen& other)            = delete;
                WorldMeshGen& operator=(const WorldMeshGen& other) = delete;

                /** @brief Reads the vertex data
                 * @param device Vulkan device
                 * @param physicalDevice Physical device
                 * @param pOutput Output pointer for vertex data
                 * @param outputSize Output size of vertex data */
                void readVertices(VkDevice         device,
                                  VkPhysicalDevice physicalDevice,
                                  MeshVertex*      pOutput,
                                  const size_t     outputSize);
                /** @brief Reads the index data
                 * @param device Vulkan device
                 * @param physicalDevice Physical device
                 * @param pOutput Output pointer for index data
                 * @param outputSize Output size of index data */
                void readIndices(VkDevice         device,
                                 VkPhysicalDevice physicalDevice,
                                 uint32_t*        pOutput,
                                 const size_t     outputSize);
                /** @brief Reads the vertex count from count buffer
                 * @param vertexCount Output for vertex count */
                void readVertexCount(uint32_t& vertexCount) override;

                /** @brief Returns the generate mutex for external synchronization
                 * @return Reference to the generate mutex */
                std::recursive_mutex& getGenerateMutex() override;

                /** @brief Returns the completion sync handle (API-agnostic)
                 * @return Reference to the completion sync handle */
                const GameOpaqueSyncHandle& getCompletionHandle() const override;

                /** Get the completion semaphore for this dispatch
                 * @return The semaphore that will be signaled when the dispatch completes */
                const GameVulkanSemaphore& getCompletionSemaphore() const override;
                GameVulkanSemaphore&       getCompletionSemaphore() override;

                /** Get the completion fence for this dispatch
                 * @return The fence that will be signaled when the dispatch completes */
                const GameVulkanFence& getCompletionFence() const override;
                GameVulkanFence&       getCompletionFence() override;

                /** @brief Returns the vertex buffer handle (API-agnostic)
                 * @return Reference to the vertex buffer handle struct */
                GameOpaqueBufferHandle& getVertexBuffer() override;

                /** @brief Returns the index buffer handle (API-agnostic)
                 * @return Reference to the index buffer handle struct */
                GameOpaqueBufferHandle& getIndexBuffer() override;

                /** @brief Returns the count buffer handle (API-agnostic)
                 * @return Reference to the count buffer handle struct */
                GameOpaqueBufferHandle& getCountBuffer() override;

#if defined(_RL_CHUNK_VULKAN_BACKEND)
                /** Vulkan backend: return native buffer pointers */
                void* getVertexBufferPtr();
                void* getIndexBufferPtr();
                void* getCountBufferPtr();
#endif

                uint32_t getSubdivisions() const override;
                uint32_t getWidth() const;
                uint32_t getHeight() const;
                uint32_t getDepth() const;

        protected:
                void dispatch(void*                      pResource,
                              const GameVulkanSemaphore& waitSemaphore,
                              GameVulkanFence&           fence) override;

        private:
                void createDescriptorSets();
                void createPipeline();
                void initializeCountBuffer();

                VkDevice         device;
                VkPhysicalDevice physicalDevice;
                VkQueue          graphicsQueue;
                VkCommandPool    commandPool;

                uint32_t width;
                uint32_t height;
                uint32_t depth;
                uint32_t maxVertices;
                uint32_t maxIndices;
                uint32_t subdivisions;

                IMeshTess& meshTess;

                GameShaderModule      computeShader;
                VkPipeline            pipeline;
                VkPipelineLayout      pipelineLayout;
                VkDescriptorSet       descriptorSet;
                VkDescriptorSetLayout descriptorSetLayout;
                VkDescriptorPool      descriptorPool;

                GameVulkanMemoryAllocator memoryAllocator;
                GameVulkanBuffer          vertexBuffer;
                GameVulkanBuffer          indexBuffer;
                GameVulkanBuffer          countBuffer;

                GameOpaqueBuffer<GameVulkanBuffer>  vertexBufferHandle;
                GameOpaqueBuffer<GameVulkanBuffer>  indexBufferHandle;
                GameOpaqueBuffer<GameVulkanBuffer>  countBufferHandle;
                GameOpaqueSync<GameVulkanSemaphore> completionHandle;

                GameVulkanCommandBuffer commandBuffer;

                GameVulkanSemaphore completionSemaphore;
                GameVulkanFence     completionFence;

                std::recursive_mutex generateMutex;
};

} // namespace rl

#endif // RL_CHUNK_WORLD_MESH_GEN_H
