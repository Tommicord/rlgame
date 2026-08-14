#ifndef RL_CHUNK_CHUNK_OPEN_SIMPLEX_H
#define RL_CHUNK_CHUNK_OPEN_SIMPLEX_H

#include "Rl.Base/GameShaderModule.h"
#include "Rl.Base/GameVulkanBuffer.h"
#include "Rl.Base/GameVulkanSemaphore.h"
#include "Rl.Base/GameVulkanFence.h"
#include "Rl.Base/GameDevice.h"
#include "Rl.Base/IGameComputeDispatch.h"
#include "Rl.Chunk/ChunkNoiseGenerator.h"

#include <cstddef>
#include <cstdint>
#include <vulkan/vulkan.hpp>

namespace rl
{

/** Push constants for simplex noise compute shader */
struct ChunkOpenSimplexPushConstants
{
                uint32_t dimension; /**< Noise dimension (2D or 3D) */
                float    scale; /**< Noise scale multiplier */
                float    persistence; /**< Persistence for fractal noise */
                float    offsetX; /**< X offset for noise sampling */
                float    offsetY; /**< Y offset for noise sampling */
                float    offsetZ; /**< Z offset for noise sampling */
                uint32_t width; /**< Output width */
                uint32_t height; /**< Output height */
                uint32_t depth; /**< Output depth */
                uint32_t time; /**< Time for animated noise */
                uint32_t octaves; /**< Number of octaves for fractal noise */
                uint32_t seed; /**< Random seed */
                uint32_t _padding; /**< Padding for alignment */
};

/** @brief Resource pointer when dispatching ChunkSimplex */
struct ChunkOpenSimplexComputePResource
{
                ChunkOpenSimplexPushConstants* pParams;
};

/** GPU-accelerated simplex noise generator using Vulkan compute shaders */
class ChunkOpenSimplex : public ChunkNoiseGenerator,
                         public IGameComputeDispatch
{
        public:
                /** Constructs a OpenSimplex noise generator
                 * @param width Output width
                 * @param height Output height
                 * @param depth Output depth
                 * @param seed Random seed
                 * @param instance Vulkan device instance */
                ChunkOpenSimplex(uint32_t            width,
                                 uint32_t            height,
                                 uint32_t            depth,
                                 uint32_t            seed,
                                 GameDeviceInstance& instance);
                /** Destroys the noise generator */
                ~ChunkOpenSimplex();
                ChunkOpenSimplex(const ChunkOpenSimplex& other)            = delete;
                ChunkOpenSimplex& operator=(const ChunkOpenSimplex& other) = delete;

                /** Sets the output width
                 * @param newWidth The new width */
                void setWidth(uint32_t newWidth);
                /** Returns the output width
                 * @return Current width */
                uint32_t getWidth() const;
                /** Sets the output height
                 * @param newHeight The new height */
                void setHeight(uint32_t newHeight);
                /** Returns the output height
                 * @return Current height */
                uint32_t getHeight() const;
                /** Sets the output depth
                 * @param newDepth The new depth */
                void setDepth(uint32_t newDepth);
                /** Returns the output depth
                 * @return Current depth */
                uint32_t getDepth() const;
                /** Sets the random seed
                 * @param newSeed The new seed */
                void setSeed(uint32_t newSeed);
                /** Updates permutation tables with new seed
                 * @param device Vulkan device
                 * @param physicalDevice Physical device */
                void updateSeed(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t newSeed);
                /** Returns the random seed
                 * @return Current seed */
                uint32_t getSeed() const;

                /** Returns the noise buffer
                 * @return Vulkan buffer handle */
                VkBuffer getNoiseBuffer() const;
                /** Returns the noise buffer size
                 * @return Buffer size in bytes */
                VkDeviceSize getNoiseBufferSize() const;
                /** Reads the noise data from GPU to CPU
                 * @param device Vulkan device
                 * @param physicalDevice Physical device
                 * @param output Output vector for noise data */
                void
                read(VkDevice device, VkPhysicalDevice physicalDevice, std::vector<float>& output);

                /**
                 * @brief Get the generate mutex for external synchronization
                 * @return Reference to the generate mutex
                 */
                std::recursive_mutex& getGenerateMutex() override;
                /**
                 * @brief Get a const reference the completion semaphore for this dispatch
                 * @return The semaphore that will be signaled when the dispatch completes
                 */
                const GameVulkanSemaphore& getCompletionSemaphore() const override;

                /* @brief Get a mutable reference to the completion semaphore for this dispatch
                 * @return The semaphore that will be signaled when the dispatch completes
                 */
                GameVulkanSemaphore& getCompletionSemaphore() override;

                /**
                 * @brief Get a const reference to the completion fence for this dispatch
                 * @return The fence that will be signaled when the dispatch completes
                 */
                const GameVulkanFence& getCompletionFence() const override;

                /**
                 * @brief Get a mutable reference to the completion fence for this dispatch
                 * @return The fence that will be signaled when the dispatch completes
                 */
                GameVulkanFence& getCompletionFence() override;

        protected:
                /** internal dispatch method called by GameComputeDispatch
                 * @param pResource The pointer to the resource when dispatching, must be a instance
                 * of ChunkOpenSimplexComputePResource
                 * @param waitSemaphore Semaphore to wait on before starting generation
                 * @param fence Fence to signal when dispatch is complete
                 */
                void dispatch(void*                      pResource,
                              const GameVulkanSemaphore& waitSemaphore,
                              GameVulkanFence&           fence) override;

        private:
                /** Creates the descriptor set layout
                 * @param device Vulkan device */
                void createDescriptorSetLayout(VkDevice device);
                /** Creates the descriptor pool
                 * @param device Vulkan device */
                void createDescriptorPool(VkDevice device);
                /** Creates the descriptor sets
                 * @param device Vulkan device */
                void createDescriptorSets(VkDevice device);
                /** Creates the compute pipeline
                 * @param device Vulkan device */
                void createComputePipeline(VkDevice device);

                GameVulkanBuffer    noiseBuffer; /**< Noise output buffer */
                GameVulkanSemaphore completionSemaphore; /**< Semaphore for signaling completion */
                GameVulkanFence     completionFence; /**< Fence for signaling completion */
                GameShaderModule    computeShaderModule;

                VkInstance            instance = VK_NULL_HANDLE;
                VkDescriptorSetLayout descriptorSetLayout =
                    VK_NULL_HANDLE; /**< Descriptor set layout */
                VkDescriptorSet  descriptorSet  = VK_NULL_HANDLE; /**< Descriptor set */
                VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
                VkPipelineLayout pipelineLayout = VK_NULL_HANDLE; /**< Pipeline layout */
                VkPipeline       pipeline       = VK_NULL_HANDLE; /**< Compute pipeline */

                std::recursive_mutex generateMutex;

                /** internal state */
                struct State
                {
                                uint32_t seed        = 0; /**< Random seed */
                                uint32_t noiseWidth  = 0; /**< Noise width */
                                uint32_t noiseHeight = 0; /**< Noise height */
                                uint32_t noiseDepth  = 0; /**< Noise depth */
                } state;
};

} // namespace rl

#endif
