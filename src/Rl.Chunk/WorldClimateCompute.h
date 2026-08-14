#ifndef RL_CHUNK_WORLD_CLIMATE_COMPUTE_H
#define RL_CHUNK_WORLD_CLIMATE_COMPUTE_H

#include "Rl.Base/GameMatrix.h"
#include "Rl.Base/GameShaderModule.h"
#include "Rl.Base/GameVulkanBuffer.h"
#include "Rl.Base/GameVulkanCommandBuffer.h"
#include "Rl.Base/GameVulkanSemaphore.h"
#include "Rl.Base/GameVulkanCommandPool.h"
#include "Rl.Base/GameVulkanImage.h"
#include "Rl.Base/GameVulkanImageView.h"
#include "Rl.Base/IGameComputeDispatch.h"
#include "Rl.Base/GameVulkanFence.h"
#include "Rl.Base/GameDevice.h"
#include "Rl.Base/GameOpaqueImageHandle.h"
#include "Rl.Base/GameOpaqueSyncHandle.h"
#include "Rl.Chunk/IClimateCompute.h"

#include <cstdint>
#include <mutex>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace rl
{

/** @brief Planet data structure matching the compute shader */
struct alignas(16) WorldPlanetData
{
    Vec3     center; /**< Center of the planet in world space */
    float    radius; /**< Radius of the planet */
    Vec3     axis; /**< Rotation axis of the planet */
    float    mass; /**< Mass of the planet */
    Vec3     velocity; /**< Velocity vector */
    float    gravity; /**< Surface gravity */
    float    baseTemperature; /**< Base temperature at equator */
    float    baseMoisture; /**< Base moisture at equator */
    float    atmosphereHeight; /**< Height of atmosphere */
    uint32_t seed; /**< Random seed */
    uint32_t shapeType; /**< Shape type identifier */
    uint32_t _padding[2]; /**< Padding for alignment */
};

/** @brief Equator field data structure matching the compute shader output */
struct WorldClimateData
{
    float latitude; /**< Latitude: -1.0 (south pole) to 1.0 (north pole) */
    float darkening; /**< Equator darkening factor */
    float temperature; /**< Temperature modifier */
    float moisture; /**< Moisture modifier */
};

/** @brief Push constants for equator compute shader */
struct WorldClimateComputePushConstants
{
    Vec3     worldOrigin; /**< Origin of the region to process */
    uint32_t width; /**< Width of the region in texels */
    uint32_t height; /**< Height of the region in texels */
    float    texelSize; /**< Size of each texel in world units */
    uint32_t planetIndex; /**< Index of the planet to use */
    uint32_t _padding[3]; /**< Padding for alignment */
};

/** @brief Resource pointer when dispatching WorldClimateCompute */
struct WorldClimateComputePResource
{
    WorldClimateComputePushConstants* pParams;
    WorldPlanetData*                  pPlanet;
};

/** @brief GPU-accelerated equator field generator using Vulkan compute shaders */
class WorldClimateCompute : public IGameComputeDispatch,
                            public IClimateCompute
{
  public:
    /** @brief Constructs an equator field generator
     * @param width Output width
     * @param height Output height
     * @param instance Vulkan device instance */
    WorldClimateCompute(uint32_t width, uint32_t height, GameDeviceInstance& instance);
    /** @brief Destroys the equator field generator */
    ~WorldClimateCompute();
    WorldClimateCompute(const WorldClimateCompute& other)            = delete;
    WorldClimateCompute& operator=(const WorldClimateCompute& other) = delete;

    /** @brief Reads the equator field data from GPU to CPU
     * @param device Vulkan device
     * @param physicalDevice Physical device
     * @param output Output vector for equator data */
    void
    read(VkDevice device, VkPhysicalDevice physicalDevice, std::vector<WorldClimateData>& output);

    /** @brief Returns the equator field image (API-agnostic)
     * @return Image handle for equator field */
    const GameOpaqueImageHandle& getEquatorImage() const override;

    /** @brief Returns the generate mutex for external synchronization
     * @return Reference to the generate mutex */
    std::recursive_mutex& getGenerateMutex() override;

    /**
     * @brief Get the completion sync handle for this dispatch (API-agnostic)
     * @return The sync handle that will be signaled when the dispatch completes
     */
    const GameOpaqueSyncHandle& getCompletionHandle() const override;

    /** Get the completion semaphore for this dispatch
     * @return The semaphore that will be signaled when the dispatch completes */
    const GameVulkanSemaphore& getCompletionSemaphore() const override;
    GameVulkanSemaphore&       getCompletionSemaphore() override;

    /** Get the completion fence for this dispatch
     * @return The fence that will be signaled when the dispatch completes */
    const GameVulkanFence& getCompletionFence() const override;
    GameVulkanFence&       getCompletionFence() override;

#if defined(_RL_CHUNK_VULKAN_BACKEND)
    /** Vulkan backend: opaque pointer to equator image view wrapper */
    void* getEquatorImageViewPtr() const;
#endif

  protected:
    /** internal dispatch method called by GameComputeDispatch
     * @param pResource The pointer to the resource when dispatching, must be a instance
     * of WorldClimateComputePResource
     * @param waitSemaphore Semaphore to wait on before starting generation
     * @param fence Fence to signal when dispatch completes */
    void dispatch(void*                      pResource,
                  const GameVulkanSemaphore& waitSemaphore,
                  GameVulkanFence&           fence) override;

  private:
    /** @brief Creates the descriptor set layout
     * @param device Vulkan device */
    void createDescriptorSetLayout(VkDevice device);
    /** @brief Creates the descriptor pool
     * @param device Vulkan device */
    void createDescriptorPool(VkDevice device);
    /** @brief Creates the descriptor sets
     * @param device Vulkan device */
    void createDescriptorSets(VkDevice device);
    /** @brief Creates the compute pipeline
     * @param device Vulkan device */
    void createComputePipeline(VkDevice device);
    /** @brief Creates the equator image
     * @param device Vulkan device
     * @param physicalDevice Physical device */
    void createEquatorImage(VkDevice device, VkPhysicalDevice physicalDevice);
    /** @brief Creates the image view for the equator
     * @param device Vulkan device */
    void createEquatorImageView(VkDevice device);
    /** @brief Updates the planet buffer with new planet data
     * @param planet Planet data to upload to GPU */
    void updatePlanetBuffer(const WorldPlanetData& planet);

    VkDevice         device         = VK_NULL_HANDLE;
    VkQueue          computeQueue   = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkInstance       instance       = VK_NULL_HANDLE;

    GameVulkanImage     equatorImage;
    GameVulkanImageView equatorImageView;

    GameOpaqueImage<GameVulkanImage>    equatorImageHandle;
    GameOpaqueSync<GameVulkanSemaphore> completionHandle;

    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet       descriptorSet       = VK_NULL_HANDLE;
    VkDescriptorPool      descriptorPool      = VK_NULL_HANDLE;
    VkPipelineLayout      pipelineLayout      = VK_NULL_HANDLE;
    VkPipeline            pipeline            = VK_NULL_HANDLE;

    GameVulkanMemoryAllocator memoryAllocator;
    GameVulkanBuffer          planetBuffer;
    GameVulkanSemaphore       completionSemaphore;
    GameVulkanFence           completionFence;
    GameShaderModule          computeShaderModule;

    GameVulkanCommandPool   computeCommandPool;
    GameVulkanCommandBuffer computeCommandBuffer;
    GameVulkanFence         readFence;

    std::recursive_mutex generateMutex;

    uint32_t width;
    uint32_t height;
};

} // namespace rl

#endif
