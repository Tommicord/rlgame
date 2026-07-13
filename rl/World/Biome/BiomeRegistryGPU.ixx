export module Rl.World.Biome.BiomeRegistryGPU;

import Rl.World.Biome.BiomeGPUParams;
import Rl.RayLog.Macro;

import <cstdint>;
import <vector>;
import <memory>;
import <vulkan/vulkan.hpp>;

namespace Rl::World::Biome
{

class IBiome;

/* GPU biome registry for compute shader access */
export class BiomeRegistryGPU
{
  protected:
  static constexpr auto RAYLOG_TAG = "BiomeRegistryGPU";

  public:
  BiomeRegistryGPU() = default;
  ~BiomeRegistryGPU();

  /* Initialize GPU resources for biome registry */
  bool Initialize(VkDevice device, VkPhysicalDevice physicalDevice);

  /* Cleanup GPU resources */
  void Shutdown(VkDevice device);

  /* Register a biome for GPU access */
  void RegisterBiome(const IBiome& biome);

  /* Update GPU buffer with registered biomes (call after RegisterBiome) */
  bool UpdateGPUBuffer(VkDevice device, VkCommandBuffer commandBuffer);

  /* Get the GPU buffer handle for compute shader binding */
  [[nodiscard]]
  VkBuffer GetBiomeBuffer() const
  {
    return biomeBuffer;
  }

  /* Get the GPU buffer handle for unit rules */
  [[nodiscard]]
  VkBuffer GetUnitRulesBuffer() const
  {
    return unitRulesBuffer;
  }

  /* Get the total number of registered biomes */
  [[nodiscard]]
  uint32_t GetBiomeCount() const
  {
    return static_cast<uint32_t>(cpuBiomes.size());
  }

  /* Get the total number of unit rules across all biomes */
  [[nodiscard]]
  uint32_t GetUnitRuleCount() const
  {
    return static_cast<uint32_t>(cpuUnitRules.size());
  }

  /* Check if registry is initialized */
  [[nodiscard]]
  bool IsInitialized() const
  {
    return initialized;
  }

  /* Disable copy operations */
  BiomeRegistryGPU(const BiomeRegistryGPU&)            = delete;
  BiomeRegistryGPU& operator=(const BiomeRegistryGPU&) = delete;

  /* Disable move operations */
  BiomeRegistryGPU(BiomeRegistryGPU&&)            = delete;
  BiomeRegistryGPU& operator=(BiomeRegistryGPU&&) = delete;

  private:
  /* Create Vulkan buffer with proper memory allocation */
  bool CreateBuffer(VkDevice              device,
                    VkPhysicalDevice      physicalDevice,
                    VkDeviceSize          size,
                    VkBufferUsageFlags    usage,
                    VkMemoryPropertyFlags properties,
                    VkBuffer&             buffer,
                    VkDeviceMemory&       memory);

  /* Copy data from CPU to GPU buffer */
  bool CopyBufferToGPU(VkDevice        device,
                       VkCommandBuffer commandBuffer,
                       VkBuffer        srcBuffer,
                       VkBuffer        dstBuffer,
                       VkDeviceSize    size);

  /* Find memory type index for buffer allocation */
  uint32_t FindMemoryType(VkPhysicalDevice      physicalDevice,
                          uint32_t              typeFilter,
                          VkMemoryPropertyFlags properties);

  /* CPU-side biome data */
  std::vector<BiomeGPUParams>   cpuBiomes;
  std::vector<BiomeUnitRuleGPU> cpuUnitRules;

  /* GPU-side buffers */
  VkBuffer       biomeBuffer     = VK_NULL_HANDLE;
  VkDeviceMemory biomeMemory     = VK_NULL_HANDLE;
  VkBuffer       unitRulesBuffer = VK_NULL_HANDLE;
  VkDeviceMemory unitRulesMemory = VK_NULL_HANDLE;

  /* Staging buffers for data transfer */
  VkBuffer       stagingBuffer = VK_NULL_HANDLE;
  VkDeviceMemory stagingMemory = VK_NULL_HANDLE;

  /* Buffer sizes */
  VkDeviceSize biomeBufferSize     = 0;
  VkDeviceSize unitRulesBufferSize = 0;

  /* Initialization state */
  bool initialized = false;
  bool gpuDirty    = false; // Flag to indicate GPU needs update

  /* Vulkan device handles */
  VkDevice         device         = VK_NULL_HANDLE;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
};

} // namespace Rl::World::Biome
