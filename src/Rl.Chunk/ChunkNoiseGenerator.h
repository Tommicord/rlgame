#ifndef RL_CHUNK_CHUNK_NOISE_GENERATOR_H
#define RL_CHUNK_CHUNK_NOISE_GENERATOR_H

#include "Rl.Base/GameVulkanBuffer.h"
#include "Rl.Base/GameVulkanCommandBuffer.h"
#include "Rl.Base/GameVulkanCommandPool.h"
#include "Rl.Base/GameVulkanFence.h"
#include "Rl.Base/GameDevice.h"

#include <array>
#include <cstdint>
#include <mutex>
#include <vulkan/vulkan.hpp>

namespace rl
{

/** @brief Base class for GPU-accelerated noise generation using Vulkan compute shaders
 * Provides common Vulkan infrastructure and permutation buffer management */
class ChunkNoiseGenerator
{
  public:
    static constexpr size_t permBufferSize = 256;

    /** @brief Constructs a noise generator base
     * @param seed Random seed for permutation generation
     * @param instance Vulkan device instance */
    ChunkNoiseGenerator(uint32_t seed, GameDeviceInstance& instance);
    virtual ~ChunkNoiseGenerator() = default;

    ChunkNoiseGenerator(const ChunkNoiseGenerator& other)            = delete;
    ChunkNoiseGenerator& operator=(const ChunkNoiseGenerator& other) = delete;

    /** @brief Generates permutation tables for noise
     * @param seed Random seed
     * @param perm Output permutation array
     * @param permGradIndex3d Output gradient index array */
    static void genPermutations(const uint32_t                       seed,
                                std::array<int32_t, permBufferSize>& perm,
                                std::array<int32_t, permBufferSize>& permGradIndex3d);

    /** @brief Updates permutation buffers on GPU
     * @param device Vulkan device
     * @param physicalDevice Physical device */
    void updatePermutationBuffers(VkDevice device, VkPhysicalDevice physicalDevice);

    /** @brief Updates permutation buffers with a new seed
     * @param device Vulkan device
     * @param physicalDevice Physical device
     * @param newSeed New seed value */
    void
    updatePermutationBuffers(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t newSeed);

    VkDevice         device         = VK_NULL_HANDLE;
    VkQueue          computeQueue   = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkInstance       instance       = VK_NULL_HANDLE;

    GameVulkanCommandPool   computeCommandPool;
    GameVulkanCommandBuffer computeCommandBuffer;
    GameVulkanFence         fence;

    std::recursive_mutex generateMutex;

    GameVulkanMemoryAllocator memoryAllocator;
    GameVulkanBuffer          permBuffer;
    GameVulkanBuffer          permGradBuffer;

    uint32_t seed;
};

} // namespace rl

#endif
