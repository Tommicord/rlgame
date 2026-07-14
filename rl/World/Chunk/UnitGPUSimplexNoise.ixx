export module Rl.World.Chunk.UnitGPUSimplexNoise;

import <cstddef>;
import <vulkan/vulkan.hpp>;

namespace Rl::World::Chunk
{

/* Push constants for the Simplex noise generation */
export struct alignas(16) SimplexNoisePushConstants
{
  uint32_t dimension; // 2 or 3
  float    scale; // Noise scale
  float    offsetX; // X offset
  float    offsetY; // Y offset
  float    offsetZ; // Z offset
  uint32_t width; // Grid width
  uint32_t height; // Grid height
  uint32_t depth; // Grid depth
  uint32_t time; // Time offset for animated noise
  uint32_t octaves; // Number of octaves for FBM (default: 1)
  float    persistence; // Amplitude multiplier per octave (default: 0.5)
  float    lacunarity; // Frequency multiplier per octave (default: 2.0)
  uint32_t noiseType; // 0: Standard, 1: FBM, 2: Ridged, 3: Turbulence
  uint32_t seed; // Seed for permutation initialization
};

/* GPU Simplex noise buffer manager for chunk generation */
export class UnitGPUSimplexNoise
{
  public:
  /* Default constructor for initialize a GPU powered Simplex Noise compute */
  UnitGPUSimplexNoise() = default;

  /* Destructs a UnitChunkBufferGPUSimplex instance (this don't free the Vulkan resources)
   */
  ~UnitGPUSimplexNoise() = default;

  /* Initialize Simplex noise resources like permutation tables, etc. */
  void Initialize(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t seed = 0);

  /* Create noise output buffer for a chunk */
  void CreateNoiseBuffer(VkDevice device,
      VkPhysicalDevice            physicalDevice,
      uint32_t                    width,
      uint32_t                    height,
      uint32_t                    depth);

  /* Create output buffers (temperature, moisture, elevation) for world mapping */
  void CreateWorldOutputBuffers(VkDevice device,
      VkPhysicalDevice                   physicalDevice,
      uint32_t                           width,
      uint32_t                           height,
      uint32_t                           depth);

  /* Push constants for world mapping shader */
  struct WorldNoisePushConstants
  {
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    float    globalScale;
    float    tempBase;
    float    tempVariation;
    float    moistBase;
    float    moistVariation;
    float    elevBase;
    float    elevVariation;
    float    _pad0;
  };

  /* Push constants for the Simplex noise generation */
  struct WorldNoisePushConstantArray
  {
    std::vector<WorldNoisePushConstants> data{};
  };

  /* Generate temperature/moisture/elevation from the existing noise buffer */
  void GenWorldNoise(VkDevice        device,
      VkCommandBuffer                commandBuffer,
      const WorldNoisePushConstants& params) const;

  /* Getters for world output buffers */
  [[nodiscard]]
  VkBuffer GetTemperatureBuffer() const
  { return temperatureBuffer; }
  [[nodiscard]]
  VkBuffer GetMoistureBuffer() const
  { return moistureBuffer; }
  [[nodiscard]]
  VkBuffer GetElevationBuffer() const
  { return elevationBuffer; }

  /* Generates noise for a chunk using a GPU compute shader */
  void GenNoise(VkDevice               device,
      VkCommandBuffer                  commandBuffer,
      const SimplexNoisePushConstants& params) const;

  /* Gets the noise buffer for reading results */
  [[nodiscard]]
  VkBuffer GetNoiseBuffer() const
  { return noiseBuffer; }
  [[nodiscard]]
  VkDeviceMemory GetNoiseBufferMemory() const
  { return noiseBufferMemory; }

  void Destroy(VkDevice device);

  private:
  /* Stores the permutation buffer */
  VkBuffer permBuffer = VK_NULL_HANDLE;

  /* Stores the permutation buffer device memory */
  VkDeviceMemory permBufferMemory = VK_NULL_HANDLE;

  /* Stores gradients 3D index buffer */
  VkBuffer permGradIndex3DBuffer = VK_NULL_HANDLE;

  /* Stores gradients 3D index buffer device memory */
  VkDeviceMemory permGradIndex3DBufferMemory = VK_NULL_HANDLE;

  /* Stores the init flag buffer */
  VkBuffer initFlagBuffer = VK_NULL_HANDLE;

  /* Stores the init flag buffer device memory */
  VkDeviceMemory initFlagBufferMemory = VK_NULL_HANDLE;

  /* Stores the noise buffer */
  VkBuffer noiseBuffer = VK_NULL_HANDLE;

  /* Stores the noise buffer device memory */
  VkDeviceMemory noiseBufferMemory = VK_NULL_HANDLE;

  /* The descriptor set layout */
  VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;

  /* The pipeline for compute shader */
  VkPipeline genPipeline;

  /* The pipeline layout for compute shader */
  VkPipelineLayout genPipelineLayout;

  /* The descriptor set for the pipeline */
  VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

  /* The descriptor pool for descriptor set assignment */
  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

  /* Seed for permutation initialization */
  uint32_t seed = 0;

  /* The output noise buffer width */
  uint32_t noiseWidth = 0;

  /* The output noise buffer height */
  uint32_t noiseHeight = 0;

  /* The output noise buffer depth */
  uint32_t noiseDepth = 0;

  VkBuffer       temperatureBuffer = VK_NULL_HANDLE;
  VkDeviceMemory temperatureBufferMemory = VK_NULL_HANDLE;

  VkBuffer       moistureBuffer = VK_NULL_HANDLE;
  VkDeviceMemory moistureBufferMemory = VK_NULL_HANDLE;

  VkBuffer       elevationBuffer = VK_NULL_HANDLE;
  VkDeviceMemory elevationBufferMemory = VK_NULL_HANDLE;

  VkDescriptorPool      mappingDescriptorPool = VK_NULL_HANDLE;
  VkDescriptorSetLayout mappingDescriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorSet       mappingDescriptorSet = VK_NULL_HANDLE;

  mutable VkPipeline       mappingPipeline = VK_NULL_HANDLE;
  mutable VkPipelineLayout mappingPipelineLayout = VK_NULL_HANDLE;

  /* Describe if initialized */
  bool isInitialized = false;
};

} // namespace Rl::World::Chunk
