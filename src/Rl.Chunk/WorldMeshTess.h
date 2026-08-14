#ifndef RL_CHUNK_WORLD_MESH_TESS_H
#define RL_CHUNK_WORLD_MESH_TESS_H

#include "Rl.Base/GameVulkanShaderModule.h"
#include "Rl.Base/GameVulkanBuffer.h"
#include "Rl.Base/GameVulkanCommandBuffer.h"
#include "Rl.Base/GameVulkanSemaphore.h"
#include "Rl.Base/GameVulkanCommandPool.h"
#include "Rl.Base/IGameComputeDispatch.h"
#include "Rl.Base/GameVulkanFence.h"
#include "Rl.Base/GameDevice.h"
#include "Rl.Base/GameOpaqueBufferHandle.h"
#include "Rl.Base/GameOpaqueSyncHandle.h"

#include "Rl.Chunk/IMeshTess.h"
#include "Rl.Chunk/WorldUnitPlacement.h"
#include "Rl.World/Unit.h"

#include <cstdint>
#include <mutex>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace rl
{

/** @brief Configuration parameters for WorldMeshTess */
struct WorldMeshTessData
{
    uint32_t width; /**< Output width */
    uint32_t height; /**< Output height */
    uint32_t depth; /**< Output depth */
    uint32_t seed; /**< Random seed */
    uint32_t airUnitId; /**< ID to skip for air units */
};

/** @brief Push constants for tessellation compute shader */
struct WorldMeshTessPushConstants
{
    uint32_t seed;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t airUnitId;
    uint32_t _padding[3];
};

/** @brief Resource pointer when dispatching WorldMeshTess */
struct WorldMeshTessPResource
{
    WorldMeshTessPushConstants* pParams;
};
class WorldOcclusionCull;

/** @brief GPU-accelerated unit tessellation using Vulkan compute shaders */
class WorldMeshTess : public IGameComputeDispatch,
                      public IMeshTess
{
  public:
    /** @brief Constructs a unit tessellation generator that reads from
     * IUnitPlacement GPU output
     * @param data Configuration parameters
     * @param unitPlacement Reference to IUnitPlacement to read unit data from GPU
     * @param instance Vulkan device instance */
    WorldMeshTess(const WorldMeshTessData& data,
                  IUnitPlacement&          unitPlacement,
                  GameDeviceInstance&      instance);
    /** @brief Constructs a unit tessellation generator with occlusion culling
     * @param data Configuration parameters
     * @param unitPlacement Reference to IUnitPlacement to read unit data from GPU
     * @param occlusionCull Reference to WorldOcclusionCull for visibility mask
     * @param instance Vulkan device instance */
    WorldMeshTess(const WorldMeshTessData& data,
                  IUnitPlacement&          unitPlacement,
                  WorldOcclusionCull&      occlusionCull,
                  GameDeviceInstance&      instance);
    /** @brief Destroys the unit tessellation generator */
    ~WorldMeshTess();
    WorldMeshTess(const WorldMeshTess& other)            = delete;
    WorldMeshTess& operator=(const WorldMeshTess& other) = delete;

    /** @brief Reads the post-unit data from GPU to CPU
     * @param device Vulkan device
     * @param physicalDevice Physical device
     * @param output Output vector for post-unit data */
    void read(VkDevice         device,
              VkPhysicalDevice physicalDevice,
              PostUnit*        pOutput,
              const size_t     outputSize);

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

    /** @brief Returns the output buffer containing PostUnit data (API-agnostic)
     * @return Reference to the output buffer handle */
    const GameOpaqueBufferHandle& getOutputBuffer() const override;

#if defined(_RL_CHUNK_VULKAN_BACKEND)
    /** Vulkan backend: opaque pointer to native output buffer (GameVulkanBuffer*) */
    void* getOutputBufferPtr() const;
#endif

  protected:
    void dispatch(void*                      pResource,
                  const GameVulkanSemaphore& waitSemaphore,
                  GameVulkanFence&           fence) override;

  private:
    void createDescriptorSets();
    void createPipeline();

    VkDevice         device;
    VkPhysicalDevice physicalDevice;
    VkQueue          computeQueue;
    VkCommandPool    commandPool;

    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t seed;
    uint32_t airUnitId;

    IUnitPlacement&     unitPlacement;
    WorldOcclusionCull* occlusionCull; // Optional occlusion culling reference

    GameVulkanShaderModule computeShader;
    VkPipeline             pipeline;
    VkPipelineLayout       pipelineLayout;
    VkDescriptorSet        descriptorSet;
    VkDescriptorSetLayout  descriptorSetLayout;
    VkDescriptorPool       descriptorPool;

    GameVulkanMemoryAllocator memoryAllocator;
    GameVulkanBuffer          outputBuffer;
    GameVulkanCommandBuffer   commandBuffer;

    GameOpaqueBuffer<GameVulkanBuffer>  outputBufferHandle;
    GameOpaqueSync<GameVulkanSemaphore> completionHandle;

    GameVulkanSemaphore completionSemaphore;
    GameVulkanFence     completionFence;

    std::recursive_mutex generateMutex;
};

} // namespace rl

#endif // RL_CHUNK_WORLD_MESH_TESS_H
