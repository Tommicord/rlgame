#ifndef RL_CHUNK_WORLD_MESH_DELAUNAY_2D_H
#define RL_CHUNK_WORLD_MESH_DELAUNAY_2D_H

#include "Rl.Base/GameVulkanBuffer.h"
#include "Rl.Base/GameVulkanCommandBuffer.h"
#include "Rl.Base/GameVulkanSemaphore.h"
#include "Rl.Base/GameVulkanFence.h"
#include "Rl.Base/GameShaderModule.h"
#include "Rl.Base/IGameComputeDispatch.h"
#include "Rl.Base/GameDevice.h"
#include "Rl.Base/GameOpaqueBufferHandle.h"
#include "Rl.Base/GameOpaqueSyncHandle.h"

#include "Rl.Chunk/IMeshGen.h"
#include "Rl.Chunk/IMeshDelaunay2D.h"

#include <cstdint>
#include <mutex>
#include <vulkan/vulkan.hpp>

namespace rl
{

/** @brief Configuration parameters for WorldMeshDelaunay2D */
struct WorldMeshDelaunay2DData
{
    uint32_t maxIndices; /**< Maximum indices in output */
    uint32_t subdivisions; /**< Subdivision level for triangulation */
    uint32_t faceCount; /**< Number of faces to triangulate */
    uint32_t maxIterations; /**< Maximum edge flip iterations */
};

/** @brief Push constants for 2D Delaunay triangulation compute shader */
struct WorldMeshDelaunay2DPushConstants
{
    uint32_t inputVertexCount;
    uint32_t maxIndices;
    uint32_t subdivisions;
    uint32_t faceCount;
    uint32_t maxIterations;
    uint32_t _padding[3];
};

/** @brief Resource pointer when dispatching WorldMeshDelaunay2D */
struct WorldMeshDelaunay2DPResource
{
    WorldMeshDelaunay2DPushConstants* pParams;
};

/** @brief GPU-accelerated 2D Delaunay triangulation using Vulkan compute shaders */
class WorldMeshDelaunay2D : public IGameComputeDispatch,
                            public IMeshDelaunay2D
{
  public:
    /** @brief Constructs a 2D Delaunay triangulation processor
     * @param data Configuration parameters
     * @param meshGen Reference to IMeshGen for vertex buffer
     * @param instance Vulkan device instance */
    WorldMeshDelaunay2D(const WorldMeshDelaunay2DData& data,
                        IMeshGen&                      meshGen,
                        GameDeviceInstance&            instance);
    /** @brief Destroys the triangulation processor */
    ~WorldMeshDelaunay2D();
    WorldMeshDelaunay2D(const WorldMeshDelaunay2D& other)            = delete;
    WorldMeshDelaunay2D& operator=(const WorldMeshDelaunay2D& other) = delete;

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

    GameVulkanBuffer& getIndexBuffer();
    GameVulkanBuffer& getCountBuffer();

    const GameOpaqueBufferHandle& getIndexBuffer() const override;
    const GameOpaqueBufferHandle& getCountBuffer() const override;
    const GameOpaqueSyncHandle&   getCompletionHandle() const override;
    void                          readIndices(uint32_t* pOutput, const size_t outputSize) override;
    void                          readCounts(uint32_t& pIndexCount) override;

#if defined(_RL_CHUNK_VULKAN_BACKEND)
    void* getIndexBufferPtr() const override;
    void* getCountBufferPtr() const override;
#endif

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
     * @param pIndexCount Output pointer for index count */
    void readCounts(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t& pIndexCount);

  protected:
    void dispatch(void*                      pResource,
                  const GameVulkanSemaphore& waitSemaphore,
                  GameVulkanFence&           fence) override;

  private:
    void createDescriptorSets();
    void createPipeline();
    void createFaceStartBuffer();

    VkDevice         device;
    VkPhysicalDevice physicalDevice;
    VkQueue          graphicsQueue;
    VkCommandPool    commandPool;

    uint32_t maxIndices;
    uint32_t subdivisions;
    uint32_t faceCount;
    uint32_t maxIterations;

    IMeshGen& meshGen;

    GameShaderModule      computeShader;
    VkPipeline            pipeline;
    VkPipelineLayout      pipelineLayout;
    VkDescriptorSet       descriptorSet;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorPool      descriptorPool;

    GameVulkanMemoryAllocator memoryAllocator;
    GameVulkanBuffer          indexBuffer;
    GameVulkanBuffer          countBuffer;
    GameVulkanBuffer          faceStartBuffer;
    GameVulkanBuffer          edgeFlipBuffer;

    GameVulkanCommandBuffer computeCommandBuffer;

    GameVulkanSemaphore completionSemaphore;
    GameVulkanFence     completionFence;

    GameOpaqueBufferHandle indexBufferHandle;
    GameOpaqueBufferHandle countBufferHandle;
    GameOpaqueSyncHandle   completionHandle;

    std::recursive_mutex generateMutex;
};

} // namespace rl

#endif // RL_CHUNK_WORLD_MESH_DELAUNAY_2D_H
