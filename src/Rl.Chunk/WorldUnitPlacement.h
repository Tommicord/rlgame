#ifndef RL_CHUNK_WORLD_UNIT_PLACEMENT_H
#define RL_CHUNK_WORLD_UNIT_PLACEMENT_H

#include "Rl.Base/GameMatrix.h"
#include "Rl.Base/GameShaderModule.h"
#include "Rl.Base/GameVulkanBuffer.h"
#include "Rl.Base/GameVulkanCommandBuffer.h"
#include "Rl.Base/GameVulkanSemaphore.h"
#include "Rl.Base/GameVulkanCommandPool.h"
#include "Rl.Base/GameVulkanFence.h"
#include "Rl.Base/GameVulkanImage.h"
#include "Rl.Base/GameVulkanImageView.h"
#include "Rl.Base/GameDevice.h"
#include "Rl.Base/IGameComputeDispatch.h"
#include "Rl.Base/GameOpaqueImageHandle.h"
#include "Rl.Base/GameOpaqueSyncHandle.h"

#include "Rl.Chunk/IUnitPlacement.h"
#include "Rl.Chunk/IHeightmap.h"
#include "Rl.Chunk/IClimateCompute.h"
#include "Rl.Chunk/ChunkNoiseGenerator.h"

#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace rl
{

class PreUnitRegistry;
class PreBiomeRegistry;

/** @brief Unit data structure matching the compute shader */
struct alignas(16) WorldUnitData
{
                uint32_t typeId; /**< PreUnit::IType */
                float    elevationMin; /**< Minimum elevation for generation */
                float    elevationMax; /**< Maximum elevation for generation */
                float    moistureMin; /**< Minimum moisture for generation */
                float    moistureMax; /**< Maximum moisture for generation */
                float    temperatureMin; /**< Minimum temperature for generation */
                float    temperatureMax; /**< Maximum temperature for generation */
                float    equatorMin; /**< Minimum equator distance for generation */
                float    equatorMax; /**< Maximum equator distance for generation */
                uint32_t _padding0; /**< Padding for GLSL std430 alignment */
                uint32_t _padding1; /**< Padding for GLSL std430 alignment */
                uint32_t _padding2; /**< Padding for GLSL std430 alignment */
};

/** Biome data structure matching the compute shader */
struct alignas(16) WorldBiomeData
{
                uint32_t typeId; /**< Biome type ID */
                float    elevationMin; /**< Minimum elevation for generation */
                float    elevationMax; /**< Maximum elevation for generation */
                float    moistureMin; /**< Minimum moisture for generation */
                float    moistureMax; /**< Maximum moisture for generation */
                float    temperatureMin; /**< Minimum temperature for generation */
                float    temperatureMax; /**< Maximum temperature for generation */
                float    equatorMin; /**< Minimum equator distance for generation */
                float    equatorMax; /**< Maximum equator distance for generation */
                uint32_t _padding0; /**< Padding for GLSL std430 alignment */
                uint32_t _padding1; /**< Padding for GLSL std430 alignment */
                uint32_t _padding2; /**< Padding for GLSL std430 alignment */
};

/** @brief Push constants for unit placement compute shader */
struct WorldUnitPlacementPushConstants
{
                Vec3     worldOrigin; /**< Origin of the region to process */
                uint32_t _padding0; /**< GLSL std140 vec3 alignment (16 bytes) */
                uint32_t width; /**< Width of the region in texels */
                uint32_t height; /**< Height of the region in texels */
                uint32_t depth; /**< Depth of the region in texels */
                float    texelSize; /**< Size of each texel in world units */
                uint32_t airUnitId; /**< ID to skip for air units */
                uint32_t unitRegistryCount; /**< Number of units in registry */
                uint32_t biomeRegistryCount; /**< Number of biomes in registry */
                float    groundLevel; /**< Ground level (0-1) where deep terrain starts */
                uint32_t _padding[1]; /**< Padding for alignment */
};

struct WorldPlanetData;
/** @brief Resource pointer when dispatching WorldUnitPlacement */
struct WorldUnitPlacementComputePResource
{
                WorldUnitPlacementPushConstants* pParams;
                PreUnitRegistry*                 pUnitRegistry;
                WorldPlanetData*                 pPlanet;
                PreBiomeRegistry*                pBiomeRegistry;
};

/** @brief GPU-accelerated unit and biome placement using Vulkan compute shaders */
class WorldUnitPlacement : public IGameComputeDispatch, public IUnitPlacement
{
        public:
                /** @brief Constructs a unit placement generator
                 * @param width Output width
                 * @param height Output height
                 * @param depth Output depth
                 * @param seed Random seed for permutation generation
                 * @param instance Vulkan device instance
                 * @param heightmapGenerator Heightmap generator instance
                 * @param climateCompute Climate compute instance for equator data*/
                WorldUnitPlacement(uint32_t                width,
                                   uint32_t                height,
                                   uint32_t                depth,
                                   uint32_t                seed,
                                   GameDeviceInstance& instance,
                                   IHeightmap&             heightmapGenerator,
                                   IClimateCompute&        climateCompute);
                /** @brief Destroys the unit placement generator */
                ~WorldUnitPlacement();
                WorldUnitPlacement(const WorldUnitPlacement& other)            = delete;
                WorldUnitPlacement& operator=(const WorldUnitPlacement& other) = delete;

                /** @brief Returns the unit output image (API-agnostic)
                 * @return Image handle struct for unit output */
                const GameOpaqueImageHandle& getUnitOutputImage() const override;

                /** @brief Returns the biome output image (API-agnostic)
                 * @return Image handle struct for biome output */
                const GameOpaqueImageHandle& getBiomeOutputImage() const override;
                
                /** @brief Internal method to read unit output data
                 * @param device Vulkan device
                 * @param physicalDevice Physical device
                 * @param pOutput Output pointer to fill
                 * @param outputSize size of the output */
                void readUnitOutput(VkDevice         device,
                                    VkPhysicalDevice physicalDevice,
                                    uint32_t*        pOutput,
                                    size_t           outputSize);

                /** @brief Internal method to read biome output data
                 * @param device Vulkan device
                 * @param physicalDevice Physical device
                 * @param pOutput Output pointer to fill
                 * @param outputSize size of the output */
                void readBiomeOutput(VkDevice         device,
                                     VkPhysicalDevice physicalDevice,
                                     uint32_t*        pOutput,
                                     size_t           outputSize);

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
                GameVulkanSemaphore& getCompletionSemaphore() override;

                /** Get the completion fence for this dispatch
                 * @return The fence that will be signaled when the dispatch completes */
                const GameVulkanFence& getCompletionFence() const override;
                GameVulkanFence& getCompletionFence() override;

        protected:
                /** internal dispatch method called by GameComputeDispatch
                 * @param pResource The pointer to the resource when dispatching, must be a instance
                 * of WorldUnitPlacementComputePResource
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
                /** @brief Updates descriptor set without reallocating
                 * Called after registry buffers are created/updated */
                void updateDescriptorSets();
                /** @brief Initializes registry buffers with empty data
                 * Called during construction to ensure descriptors are valid */
                void initRegistryBuffers(VkDevice device, VkPhysicalDevice physicalDevice);
                /** Updates biome registry data on GPU
                 * @param device Vulkan device
                 * @param physicalDevice Physical device
                 * @param biomeRegistry Biome registry to upload */
                void createComputePipeline(VkDevice device);
                /** @brief Updates unit registry data on GPU
                 * @param device Vulkan device
                 * @param physicalDevice Physical device
                 * @param unitRegistry Unit registry to upload */
                void updateUnitRegistryData(VkDevice               device,
                                            VkPhysicalDevice       physicalDevice,
                                            const PreUnitRegistry& unitRegistry);
                /** @brief Updates biome registry data on GPU
                 * @param device Vulkan device
                 * @param physicalDevice Physical device
                 * @param biomeRegistry Biome registry to upload */
                void updateBiomeRegistryData(VkDevice                device,
                                             VkPhysicalDevice        physicalDevice,
                                             const PreBiomeRegistry& biomeRegistry);
                /** @brief Updates planet data on GPU
                 * @param device Vulkan device
                 * @param physicalDevice Physical device
                 * @param planet Planet data to upload */
                void updatePlanetBuffer(VkDevice               device,
                                        VkPhysicalDevice       physicalDevice,
                                        const WorldPlanetData& planet);
                /** @brief Initializes Simplex noise permutation tables
                 * @param device Vulkan device
                 * @param physicalDevice Physical device */
                void initPermutationTables(VkDevice device, VkPhysicalDevice physicalDevice);
                /** @brief Creates the unit output image
                 * @param device Vulkan device
                 * @param physicalDevice Physical device */
                void createUnitOutputImage(VkDevice device, VkPhysicalDevice physicalDevice);
                /** @brief Creates the image view for the unit output
                 * @param device Vulkan device */
                void createUnitOutputImageView(VkDevice device);
                /** @brief Creates the biome output image
                 * @param device Vulkan device
                 * @param physicalDevice Physical device */
                void createBiomeOutputImage(VkDevice device, VkPhysicalDevice physicalDevice);
                /** @brief Creates the image view for the biome output
                 * @param device Vulkan device */
                void createBiomeOutputImageView(VkDevice device);

                GameVulkanMemoryAllocator         memoryAllocator;
                GameVulkanBuffer                  planetBuffer;
                std::unique_ptr<GameVulkanBuffer> unitRegistryBuffer;
                std::unique_ptr<GameVulkanBuffer> biomeRegistryBuffer;
                std::unique_ptr<GameVulkanBuffer> permBuffer;
                std::unique_ptr<GameVulkanBuffer> permGradIndex3DBuffer;

                IHeightmap&      heightmapGenerator;
                IClimateCompute& climateCompute;
                GameShaderModule computeShaderModule;

                GameVulkanImage     unitOutputImage;
                GameVulkanImageView unitOutputImageView;
                GameVulkanImage     biomeOutputImage;
                GameVulkanImageView biomeOutputImageView;

                GameOpaqueImage<GameVulkanImage> unitOutputImageHandle;
                GameOpaqueImage<GameVulkanImage> biomeOutputImageHandle;
                GameOpaqueSync<GameVulkanSemaphore> completionHandle;

                VkDescriptorSetLayout   descriptorSetLayout = VK_NULL_HANDLE;
                VkDescriptorSet         descriptorSet       = VK_NULL_HANDLE;
                VkDescriptorPool        descriptorPool      = VK_NULL_HANDLE;
                VkPipelineLayout        pipelineLayout      = VK_NULL_HANDLE;
                VkPipeline              pipeline            = VK_NULL_HANDLE;
                GameVulkanCommandPool   computeCommandPool;
                GameVulkanCommandBuffer computeCommandBuffer;
                GameVulkanSemaphore     completionSemaphore;
                GameVulkanFence         completionFence;

                std::recursive_mutex generateMutex;

                VkDevice         device         = VK_NULL_HANDLE;
                VkQueue          computeQueue   = VK_NULL_HANDLE;
                VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
                VkInstance       instance       = VK_NULL_HANDLE;

                uint32_t width;
                uint32_t height;
                uint32_t depth;
                uint32_t seed;
};

} // namespace rl

#endif
