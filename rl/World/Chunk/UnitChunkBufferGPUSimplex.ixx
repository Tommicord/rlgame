export module Rl.World.Chunk.UnitChunkBufferGPUSimplex;

import <cstddef>;
import <vulkan/vulkan.hpp>;

namespace Rl::World::Chunk
{

/* Push constants for the Simplex noise generation */
export struct SimplexNoisePushConstants
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
};

/* GPU Simplex noise buffer manager for chunk generation */
export class UnitChunkBufferGPUSimplex
{
  public:
  /* Default constructor for initialize a GPU powered Simplex Noise compute */
  UnitChunkBufferGPUSimplex() = default;

  /* Destructs a UnitChunkBufferGPUSimplex instance (this don't free the Vulkan resources) */
  ~UnitChunkBufferGPUSimplex() = default;

  /* Initialize Simplex noise resources like permutation tables, etc. */
  void Create(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t seed = 0);

  /* Create noise output buffer for a chunk */
  void CreateNoiseBuffer(VkDevice device,
      VkPhysicalDevice            physicalDevice,
      uint32_t                    width,
      uint32_t                    height,
      uint32_t                    depth);

  /* Generates noise for a chunk using a GPU compute shader */
  void GenNoise(
      VkDevice device, VkCommandBuffer commandBuffer, const SimplexNoisePushConstants& params) const;

  /* Gets the noise buffer for reading results */
  VkBuffer GetNoiseBuffer() const
  {
    return noiseBuffer;
  }
  VkDeviceMemory GetNoiseBufferMemory() const
  {
    return noiseBufferMemory;
  }
  
  /* Helper to create standard FBM noise parameters */
  static SimplexNoisePushConstants CreateFBMParams(
      uint32_t dimension,
      float scale,
      float offsetX,
      float offsetY,
      float offsetZ,
      uint32_t width,
      uint32_t height,
      uint32_t depth,
      uint32_t octaves = 6,
      float persistence = 0.5f,
      float lacunarity = 2.0f)
  {
    SimplexNoisePushConstants params{};
    params.dimension = dimension;
    params.scale = scale;
    params.offsetX = offsetX;
    params.offsetY = offsetY;
    params.offsetZ = offsetZ;
    params.width = width;
    params.height = height;
    params.depth = depth;
    params.time = 0;
    params.octaves = octaves;
    params.persistence = persistence;
    params.lacunarity = lacunarity;
    params.noiseType = 1; // FBM
    return params;
  }
  
  /* Helper to create ridged noise parameters */
  static SimplexNoisePushConstants CreateRidgedParams(
      uint32_t dimension,
      float scale,
      float offsetX,
      float offsetY,
      float offsetZ,
      uint32_t width,
      uint32_t height,
      uint32_t depth,
      uint32_t octaves = 6,
      float persistence = 0.5f,
      float lacunarity = 2.0f)
  {
    SimplexNoisePushConstants params{};
    params.dimension = dimension;
    params.scale = scale;
    params.offsetX = offsetX;
    params.offsetY = offsetY;
    params.offsetZ = offsetZ;
    params.width = width;
    params.height = height;
    params.depth = depth;
    params.time = 0;
    params.octaves = octaves;
    params.persistence = persistence;
    params.lacunarity = lacunarity;
    params.noiseType = 2; // Ridged
    return params;
  }
  
  /* Helper to create turbulence noise parameters */
  static SimplexNoisePushConstants CreateTurbulenceParams(
      uint32_t dimension,
      float scale,
      float offsetX,
      float offsetY,
      float offsetZ,
      uint32_t width,
      uint32_t height,
      uint32_t depth,
      uint32_t octaves = 6,
      float persistence = 0.5f,
      float lacunarity = 2.0f)
  {
    SimplexNoisePushConstants params{};
    params.dimension = dimension;
    params.scale = scale;
    params.offsetX = offsetX;
    params.offsetY = offsetY;
    params.offsetZ = offsetZ;
    params.width = width;
    params.height = height;
    params.depth = depth;
    params.time = 0;
    params.octaves = octaves;
    params.persistence = persistence;
    params.lacunarity = lacunarity;
    params.noiseType = 3; // Turbulence
    return params;
  }
  
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

  /* Stores the noise buffer */
  VkBuffer noiseBuffer = VK_NULL_HANDLE;

  /* Stores the noise buffer device memory */
  VkDeviceMemory noiseBufferMemory = VK_NULL_HANDLE;

  /* The descriptor set layout */
  VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;

  /* The descriptor set for the pipeline */
  VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

  /* The pipeline layout */
  VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

  /* The pipeline for the compute shader */
  VkPipeline pipeline = VK_NULL_HANDLE;

  /* The initial pipeline */
  VkPipeline initPipeline = VK_NULL_HANDLE;

  /* The initial pipeline layout */
  VkPipelineLayout initPipelineLayout = VK_NULL_HANDLE;

  /* The descriptor pool for descriptor set assignment */
  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

  /* The output noise buffer width */
  uint32_t noiseWidth = 0;

  /* The output noise buffer height */
  uint32_t noiseHeight = 0;

  /* The output noise buffer depth */
  uint32_t noiseDepth = 0;

  /* Describe if initialized */
  bool isInitialized = false;
};

} // namespace Rl::World::Chunk
