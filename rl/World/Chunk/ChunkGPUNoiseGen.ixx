export module Rl.World.Chunk.ChunkGPUNoiseGen;

import Rl.World.Chunk.ChunkInRenderUnits;
import Rl.World.Chunk.UnitChunkBuffer;
import Rl.World.Chunk.UnitGPUSimplexNoise;
import Rl.RayLog.Macro;

import <cstdint>;
import <memory>;
import <stdexcept>;
import <vector>;
import <vulkan/vulkan.hpp>;

namespace Rl::World::Chunk
{

/* Generates the full render-distance set of chunk noise buffers using the current GPU
 * pipeline. */
export class ChunkGPUNoiseGen
{
  protected:
  static constexpr auto RAYLOG_TAG = "ChunkGPUNoiseGen";
  public:
  ChunkGPUNoiseGen()  = default;
  ~ChunkGPUNoiseGen() = default;

  void Initialize(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t seed = 0)
  {
    noiseGenerator.Create(device, physicalDevice, seed);
    initialized = true;
  }

  void Shutdown(VkDevice device)
  {
    noiseGenerator.Destroy(device);
    initialized = false;
  }

  /*
   * Generates chunks with the render distance limit
   */
  bool
  GenerateForRenderDistance(ChunkInRenderUnits& chunkSystem,
                            VkDevice            device,
                            VkPhysicalDevice    physicalDevice,
                            VkCommandBuffer     commandBuffer,
                            int32_t             renderDistanceX,
                            int32_t             renderDistanceY,
                            int32_t             renderDistanceZ,
                            UnitGPUSimplexNoise::WorldNoisePushConstantArray worldParams)
  {
    if (!initialized)
      return false;

    const WorldChunkCoord renderDistance{renderDistanceX, renderDistanceY,
                                         renderDistanceZ};
    if (renderDistance.chunkX <= 0 || renderDistance.chunkY <= 0 ||
        renderDistance.chunkZ <= 0)
      return false;

    const int32_t totalChunks =
        renderDistance.chunkX * renderDistance.chunkY * renderDistance.chunkZ;
    if (totalChunks <= 0)
      return false;
    const auto size = worldParams.data.size();
    if (size != totalChunks)
    {
      RayLog::LogError(
        RAYLOG_TAG,
        "Cannot generate chunks due incorrect "
        "constants array length; "
        "Expected length: %d, found: %d", totalChunks, size
      );
    }

    if (!chunkSystem.Initialize())
      return false;

    for (int32_t z = 0; z < renderDistance.chunkZ; ++z)
    {
      for (int32_t y = 0; y < renderDistance.chunkY; ++y)
      {
        for (int32_t x = 0; x < renderDistance.chunkX; ++x)
        {
          WorldChunkCoord coord{x, y, z};
          if (!chunkSystem.IsInRenderDistance(coord))
            continue;
          long long pos = x * (y * renderDistance.chunkX) +
                     (z * renderDistance.chunkX * renderDistance.chunkY);
          UnitChunkBuffer chunkBuffer{};
          if (!GenerateChunkNoise(device, physicalDevice, commandBuffer, coord,
                                  chunkBuffer, worldParams.data[pos]))
            continue;
          chunkSystem.AddChunk(coord, chunkBuffer);
        }
      }
    }

    return true;
  }

  bool GenerateChunkNoise(VkDevice               device,
                          VkPhysicalDevice       physicalDevice,
                          VkCommandBuffer        commandBuffer,
                          const WorldChunkCoord& coord,
                          UnitChunkBuffer&       outChunk,
                          const UnitGPUSimplexNoise::WorldNoisePushConstants& worldParams)
  {
    if (!initialized)
      return false;

    constexpr uint32_t kChunkWidth  = UnitChunkBuffer::W;
    constexpr uint32_t kChunkHeight = UnitChunkBuffer::H;
    constexpr uint32_t kChunkDepth  = UnitChunkBuffer::D;

    noiseGenerator.CreateNoiseBuffer(device, physicalDevice, kChunkWidth, kChunkHeight,
                                     kChunkDepth);
    noiseGenerator.CreateWorldOutputBuffers(device, physicalDevice, kChunkWidth,
                                            kChunkHeight, kChunkDepth);

    SimplexNoisePushConstants simplexParams{};
    simplexParams.dimension   = 3;
    simplexParams.scale       = 0.05f;
    simplexParams.offsetX     = static_cast<float>(coord.chunkX);
    simplexParams.offsetY     = static_cast<float>(coord.chunkY);
    simplexParams.offsetZ     = static_cast<float>(coord.chunkZ);
    simplexParams.width       = kChunkWidth;
    simplexParams.height      = kChunkHeight;
    simplexParams.depth       = kChunkDepth;
    simplexParams.time        = 0;
    simplexParams.octaves     = 1;
    simplexParams.persistence = 0.5f;
    simplexParams.lacunarity  = 2.0f;
    simplexParams.noiseType   = 0;

    noiseGenerator.GenNoise(device, commandBuffer, simplexParams);
    noiseGenerator.GenWorldNoise(device, commandBuffer, worldParams);

    outChunk.Clear();
    return true;
  }

  private:
  UnitGPUSimplexNoise noiseGenerator;
  bool                initialized = false;
};

} // namespace Rl::World::Chunk
