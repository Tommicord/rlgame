#ifndef RL_CHUNK_WORLD_HEIGHTMAP_H
#define RL_CHUNK_WORLD_HEIGHTMAP_H

#include "Rl.Base/GameVulkanBuffer.h"
#include "Rl.Base/GameVulkanCommandBuffer.h"
#include "Rl.Base/GameVulkanCommandPool.h"
#include "Rl.Base/GameVulkanSemaphore.h"
#include "Rl.Base/GameVulkanFence.h"
#include "Rl.Base/GameVulkanImage.h"
#include "Rl.Base/GameVulkanImageView.h"
#include "Rl.Base/GameShaderModule.h"
#include "Rl.Base/IGameComputeDispatch.h"
#include "Rl.Base/GameDevice.h"
#include "Rl.Base/GameOpaqueImageHandle.h"
#include "Rl.Base/GameOpaqueSyncHandle.h"
#include "Rl.Chunk/ChunkNoiseGenerator.h"
#include "Rl.Chunk/IHeightmap.h"

#include <cstddef>
#include <cstdint>
#include <vulkan/vulkan.hpp>

namespace rl
{

/** Push constants for heightmap generation compute shader */
struct WorldHeightmapPushConstants
{
    uint32_t width; /**< Output width */
    uint32_t height; /**< Output height */
    uint32_t depth; /**< Output depth */
    float    scale; /**< Noise scale */
    float    heightScale; /**< Vertical exaggeration */
    float    seaLevel; /**< Sea level (0-1) */
    uint32_t seed; /**< Random seed */
    uint32_t octaves; /**< Number of octaves for fractal noise */
    float    persistence; /**< Persistence for fractal noise */
    float    groundLevel; /**< Ground level (0-1) where deep terrain starts */
    uint32_t _padding; /**< Padding for alignment */
};

/** Heightmap data containing elevation, temperature, and moisture */
struct WorldHeightmapData
{
    float elevation; /**< Terrain elevation (0-1) */
    float temperature; /**< Temperature based on elevation (0-1) */
    float moisture; /**< Moisture based on elevation (0-1) */
};

/** @brief Resource pointer when dispatching ChunkHeightmap */
struct WorldHeightmapComputePResource
{
    WorldHeightmapPushConstants* pParams;
};

/** GPU-accelerated heightmap generator using Vulkan compute shaders */
class WorldHeightmap : public ChunkNoiseGenerator,
                       public IGameComputeDispatch,
                       public IHeightmap
{
  public:
    /** Constructs a heightmap generator
     * @param width Output width
     * @param height Output height
     * @param depth Output depth
     * @param seed Random seed
     * @param instance Vulkan device instance */
    WorldHeightmap(uint32_t            width,
                   uint32_t            height,
                   uint32_t            depth,
                   uint32_t            seed,
                   GameDeviceInstance& instance);
    /** Destroys the heightmap generator */
    ~WorldHeightmap();
    WorldHeightmap(const WorldHeightmap& other)            = delete;
    WorldHeightmap& operator=(const WorldHeightmap& other) = delete;

    /** Returns the basemap elevation image (API-agnostic)
     * @return Image handle for basemap elevation */
    const GameOpaqueImageHandle& getBasemapElevationImage() const override;

    /** Returns the basemap temperature image (API-agnostic)
     * @return Image handle for basemap temperature */
    const GameOpaqueImageHandle& getBasemapTemperatureImage() const override;

    /** Returns the basemap moisture image (API-agnostic)
     * @return Image handle for basemap moisture */
    const GameOpaqueImageHandle& getBasemapMoistureImage() const override;

    /** Returns the deepmap elevation image (API-agnostic)
     * @return Image handle for deepmap elevation */
    const GameOpaqueImageHandle& getDeepmapElevationImage() const override;

    /** Returns the deepmap temperature image (API-agnostic)
     * @return Image handle for deepmap temperature */
    const GameOpaqueImageHandle& getDeepmapTemperatureImage() const override;

    /** Returns the deepmap moisture image (API-agnostic)
     * @return Image handle for deepmap moisture */
    const GameOpaqueImageHandle& getDeepmapMoistureImage() const override;

    /** Returns the generate mutex for external synchronization
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

    /** Get the basemap elevation image view
     * @return Vulkan image view for basemap elevation */
    VkImageView getBasemapElevationImageView() const;

    /** Get the basemap temperature image view
     * @return Vulkan image view for basemap temperature */
    VkImageView getBasemapTemperatureImageView() const;

    /** Get the basemap moisture image view
     * @return Vulkan image view for basemap moisture */
    VkImageView getBasemapMoistureImageView() const;

    /** Get the deepmap elevation image view
     * @return Vulkan image view for deepmap elevation */
    VkImageView getDeepmapElevationImageView() const;

    /** Get the deepmap temperature image view
     * @return Vulkan image view for deepmap temperature */
    VkImageView getDeepmapTemperatureImageView() const;

    /** Get the deepmap moisture image view
     * @return Vulkan image view for deepmap moisture */
    VkImageView getDeepmapMoistureImageView() const;

#if defined(_RL_CHUNK_VULKAN_BACKEND)
    /** Vulkan backend: return opaque pointer to the GameVulkanImageView */
    void* getBasemapElevationImageViewPtr() const;
    void* getBasemapTemperatureImageViewPtr() const;
    void* getBasemapMoistureImageViewPtr() const;
    void* getDeepmapElevationImageViewPtr() const;
    void* getDeepmapTemperatureImageViewPtr() const;
    void* getDeepmapMoistureImageViewPtr() const;
#endif

    /** Reads the heightmap data from GPU to CPU
     * @param device Vulkan device
     * @param physicalDevice Physical device
     * @param output Output vector for heightmap data */
    void
    read(VkDevice device, VkPhysicalDevice physicalDevice, std::vector<WorldHeightmapData>& output);

    /** Generates compressed grayscale image from heightmap
     * @param device Vulkan device
     * @param physicalDevice Physical device
     * @param output Output vector for compressed grayscale data */
    void generateCompressedGrayscale(VkDevice              device,
                                     VkPhysicalDevice      physicalDevice,
                                     std::vector<uint8_t>& output);

  protected:
    /** internal dispatch method called by GameComputeDispatch
     * @param pResource The pointer to the resource when dispatching, must be a instance
     * of WorldHeightmapComputePResource
     * @param waitSemaphore Semaphore to wait on before starting generation
     * @param fence Fence to signal when dispatch completes */
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
    /** Creates the basemap image
     * @param device Vulkan device
     * @param physicalDevice Physical device */
    void createBasemapImage(VkDevice device, VkPhysicalDevice physicalDevice);
    /** Creates the deepmap image
     * @param device Vulkan device
     * @param physicalDevice Physical device */
    void createDeepmapImage(VkDevice device, VkPhysicalDevice physicalDevice);
    /** Creates the image view for the basemap
     * @param device Vulkan device */
    void createBasemapImageView(VkDevice device);
    /** Creates the image view for the deepmap
     * @param device Vulkan device */
    void createDeepmapImageView(VkDevice device);
    /** Helper function to create a single channel image
     * @param device Vulkan device
     * @param physicalDevice Physical device
     * @param image Output RAII image wrapper
     * @param format Image format */
    void createBasemapChannelImage(VkDevice         device,
                                   VkPhysicalDevice physicalDevice,
                                   GameVulkanImage& image,
                                   VkFormat         format);
    /** Helper function to create a single deepmap channel image
     * @param device Vulkan device
     * @param physicalDevice Physical device
     * @param image Output RAII image wrapper
     * @param format Image format */
    void createDeepmapChannelImage(VkDevice         device,
                                   VkPhysicalDevice physicalDevice,
                                   GameVulkanImage& image,
                                   VkFormat         format);
    /** Helper function to create a single channel image view
     * @param device Vulkan device
     * @param image RAII image wrapper
     * @param imageView Output RAII image view wrapper
     * @param format Image format */
    void createBasemapChannelImageView(VkDevice             device,
                                       GameVulkanImage&     image,
                                       GameVulkanImageView& imageView,
                                       VkFormat             format);
    /** Helper function to create a single deepmap channel image view
     * @param device Vulkan device
     * @param image RAII image wrapper
     * @param imageView Output RAII image view wrapper
     * @param format Image format */
    void createDeepmapChannelImageView(VkDevice             device,
                                       GameVulkanImage&     image,
                                       GameVulkanImageView& imageView,
                                       VkFormat             format);

    GameVulkanBuffer    heightmapBuffer;
    GameVulkanSemaphore completionSemaphore;
    GameVulkanFence     completionFence;

    GameVulkanImage     basemapElevationImage;
    GameVulkanImageView basemapElevationImageView;

    GameVulkanImage     basemapTemperatureImage;
    GameVulkanImageView basemapTemperatureImageView;

    GameVulkanImage     basemapMoistureImage;
    GameVulkanImageView basemapMoistureImageView;

    GameVulkanImage     deepmapElevationImage;
    GameVulkanImageView deepmapElevationImageView;

    GameVulkanImage     deepmapTemperatureImage;
    GameVulkanImageView deepmapTemperatureImageView;

    GameVulkanImage     deepmapMoistureImage;
    GameVulkanImageView deepmapMoistureImageView;

    GameOpaqueImage<GameVulkanImage>    basemapElevationImageHandle;
    GameOpaqueImage<GameVulkanImage>    basemapTemperatureImageHandle;
    GameOpaqueImage<GameVulkanImage>    basemapMoistureImageHandle;
    GameOpaqueImage<GameVulkanImage>    deepmapElevationImageHandle;
    GameOpaqueImage<GameVulkanImage>    deepmapTemperatureImageHandle;
    GameOpaqueImage<GameVulkanImage>    deepmapMoistureImageHandle;
    GameOpaqueSync<GameVulkanSemaphore> completionHandle;

    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet       descriptorSet       = VK_NULL_HANDLE;
    VkDescriptorPool      descriptorPool      = VK_NULL_HANDLE;
    VkPipelineLayout      pipelineLayout      = VK_NULL_HANDLE;
    VkPipeline            pipeline            = VK_NULL_HANDLE;

    GameShaderModule        computeShaderModule;
    GameVulkanCommandPool   commandPool;
    GameVulkanCommandBuffer commandBuffer;

    std::recursive_mutex generateMutex;

    VkDevice         device         = VK_NULL_HANDLE;
    VkQueue          computeQueue   = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkInstance       instance       = VK_NULL_HANDLE;

    uint32_t width;
    uint32_t height;
    uint32_t depth;
};

} // namespace rl

#endif
