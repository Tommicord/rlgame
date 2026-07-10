export module Rl.World.Chunk.ChunkGeneratorGPU;

import Rl.World.Biome.BiomeRegistryGPU;
import Rl.World.Unit.UnitRegistryGPU;
import Rl.World.Chunk.ChunkInRenderUnits;
import Rl.World.Chunk.UnitChunkBuffer;
import Rl.World.Chunk.UnitGPUSimplexNoise;
import Rl.RayLog.Macro;

import <cstdint>;
import <memory>;
import <vector>;
import <functional>;
import <mutex>;
import <atomic>;
import <queue>;
import <future>;
import <vulkan/vulkan.hpp>;

namespace Rl::World::Chunk
{

/* Chunk generation request for async processing */
export struct ChunkGenRequest
{
  WorldChunkCoord coord;
  std::function<void(const WorldChunkCoord&, UnitChunkBuffer&)> callback;
  uint32_t priority; // Higher = more important
};

/* Pipeline stage enumeration for barrier management */
export enum class PipelineStage : uint32_t
{
  Noise = 0,
  Heightmap = 1,
  Biome = 2,
  UnitPlace = 3,
  PolFence = 4,
  Count = 5
};

/* GPU world generation orchestrator that manages the complete pipeline */
export class ChunkGeneratorGPU
{
  protected:
  static constexpr auto RAYLOG_TAG = "WorldGeneratorGPU";

  static constexpr auto comparator = [](const ChunkGenRequest& a, const ChunkGenRequest& b) {
    return a.priority < b.priority;
  };

  public:
  ChunkGeneratorGPU() = default;
  ~ChunkGeneratorGPU();

  /* Initialize the GPU world generation system */
  bool Initialize(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t seed = 0);

  /* Shutdown and cleanup GPU resources */
  void Shutdown(VkDevice device);

  /* Register biome and unit registries for GPU access */
  void SetBiomeRegistry(Biome::BiomeRegistryGPU* biomeRegistry);
  void SetUnitRegistry(UnitRegistryGPU* unitRegistry);

  /* Set non-curable unit IDs for polygon fence generation */
  void SetNonCurableUnits(const std::vector<uint32_t>& nonCurableIds);

  /* Generate a single chunk synchronously */
  bool GenerateChunk(VkDevice device, VkCommandBuffer commandBuffer,
                    const WorldChunkCoord& coord, UnitChunkBuffer& outChunk);

  /* Enqueue chunk for async generation */
  void EnqueueChunkGeneration(const ChunkGenRequest& request);

  /* Process async generation queue */
  void ProcessAsyncQueue(VkDevice device, VkCommandBuffer commandBuffer);

  /* Check if async queue has pending work */
  [[nodiscard]]
  bool HasPendingWork() const;

  /* Get number of pending chunks in queue */
  [[nodiscard]]
  uint32_t GetPendingCount() const;

  /* Disable copy/move operations */
  ChunkGeneratorGPU(const ChunkGeneratorGPU&) = delete;
  ChunkGeneratorGPU& operator=(const ChunkGeneratorGPU&) = delete;
  ChunkGeneratorGPU(ChunkGeneratorGPU&&) = delete;
  ChunkGeneratorGPU& operator=(ChunkGeneratorGPU&&) = delete;

  private:
  /* Create compute pipelines for each stage */
  bool CreateComputePipelines(VkDevice device, VkPhysicalDevice physicalDevice);

  /* Create descriptor sets for each pipeline */
  bool CreateDescriptorSets(VkDevice device);

  /* Create buffers for intermediate pipeline stages */
  bool CreateIntermediateBuffers(VkDevice device, VkPhysicalDevice physicalDevice,
                                  uint32_t width, uint32_t height, uint32_t depth);

  /* Execute noise generation stage */
  bool ExecuteNoiseStage(VkDevice device, VkCommandBuffer commandBuffer,
                        const WorldChunkCoord& coord);

  /* Execute heightmap generation stage */
  bool ExecuteHeightmapStage(VkDevice device, VkCommandBuffer commandBuffer);

  /* Execute biome classification stage */
  bool ExecuteBiomeStage(VkDevice device, VkCommandBuffer commandBuffer);

  /* Execute unit placement stage */
  bool ExecuteUnitPlaceStage(VkDevice device, VkCommandBuffer commandBuffer);

  /* Execute polygon fence generation stage */
  bool ExecutePolFenceStage(VkDevice device, VkCommandBuffer commandBuffer);

  /* Add pipeline barrier between stages */
  void AddPipelineBarrier(VkCommandBuffer commandBuffer, VkBuffer buffer,
                         VkAccessFlags srcAccess, VkAccessFlags dstAccess,
                         VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage);

  /* Read back generated unit data to CPU */
  bool ReadbackUnitData(VkDevice device, VkCommandBuffer commandBuffer,
                       UnitChunkBuffer& outChunk);

  /* Vulkan buffer creation helper */
  bool CreateBuffer(VkDevice device, VkPhysicalDevice physicalDevice,
                   VkDeviceSize size, VkBufferUsageFlags usage,
                   VkMemoryPropertyFlags properties,
                   VkBuffer& buffer, VkDeviceMemory& memory);

  /* Memory type finder */
  uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter,
                         VkMemoryPropertyFlags properties) const;

  /* Simplex noise generator */
  UnitGPUSimplexNoise noiseGenerator;

  /* Biome and unit registries */
  Biome::BiomeRegistryGPU* biomeRegistry = nullptr;
  UnitRegistryGPU* unitRegistry = nullptr;

  /* Non-curable unit IDs */
  std::vector<uint32_t> nonCurableUnitIds;

  /* Vulkan device handles */
  VkDevice device = VK_NULL_HANDLE;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

  /* Compute pipelines */
  VkPipeline heightmapPipeline = VK_NULL_HANDLE;
  VkPipeline biomePipeline = VK_NULL_HANDLE;
  VkPipeline unitPlacePipeline = VK_NULL_HANDLE;
  VkPipeline polFencePipeline = VK_NULL_HANDLE;

  /* Pipeline layouts */
  VkPipelineLayout heightmapLayout = VK_NULL_HANDLE;
  VkPipelineLayout biomeLayout = VK_NULL_HANDLE;
  VkPipelineLayout unitPlaceLayout = VK_NULL_HANDLE;
  VkPipelineLayout polFenceLayout = VK_NULL_HANDLE;

  /* Descriptor pools and sets */
  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
  VkDescriptorSet heightmapDescriptorSet = VK_NULL_HANDLE;
  VkDescriptorSet biomeDescriptorSet = VK_NULL_HANDLE;
  VkDescriptorSet unitPlaceDescriptorSet = VK_NULL_HANDLE;
  VkDescriptorSet polFenceDescriptorSet = VK_NULL_HANDLE;

  /* Intermediate buffers */
  VkBuffer heightmapBuffer = VK_NULL_HANDLE;
  VkDeviceMemory heightmapMemory = VK_NULL_HANDLE;
  VkBuffer biomeBuffer = VK_NULL_HANDLE;
  VkDeviceMemory biomeMemory = VK_NULL_HANDLE;
  VkBuffer unitBuffer = VK_NULL_HANDLE;
  VkDeviceMemory unitMemory = VK_NULL_HANDLE;
  VkBuffer polFenceBuffer = VK_NULL_HANDLE;
  VkDeviceMemory polFenceMemory = VK_NULL_HANDLE;

  /* Staging buffer for readback */
  VkBuffer stagingBuffer = VK_NULL_HANDLE;
  VkDeviceMemory stagingMemory = VK_NULL_HANDLE;

  /* Non-curable units GPU buffer */
  VkBuffer nonCurableBuffer = VK_NULL_HANDLE;
  VkDeviceMemory nonCurableMemory = VK_NULL_HANDLE;

  /* Buffer sizes */
  VkDeviceSize heightmapBufferSize = 0;
  VkDeviceSize biomeBufferSize = 0;
  VkDeviceSize unitBufferSize = 0;
  VkDeviceSize polFenceBufferSize = 0;

  /* Chunk dimensions */
  uint32_t chunkWidth = 0;
  uint32_t chunkHeight = 0;
  uint32_t chunkDepth = 0;

  /* Async generation queue */
  std::priority_queue<ChunkGenRequest, std::vector<ChunkGenRequest>, decltype(comparator)> asyncQueue;

  /* Queue mutex */
  mutable std::mutex queueMutex;

  /* Initialization state */
  bool initialized = false;
  bool registriesSet = false;
};

} // namespace Rl::World::Chunk
