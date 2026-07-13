import Rl.World.Chunk.ChunkGeneratorGPU;
import Rl.World.Chunk.ChunkInRenderUnits;
import Rl.World.Chunk.UnitChunkBuffer;
import Rl.World.Biome.BiomeRegistryGPU;
import Rl.World.Unit.UnitRegistryGPU;
import Rl.Base.Binding;
import Rl.Base.Game;
import Rl.RayLog.Macro;
import Rl.World.Unit;

// Disable validation layers for GPU tests to avoid fatal cleanup errors
extern bool disableValidationLayers;
bool disableValidationLayers = true;

#include <gtest/gtest.h>
#include <vulkan/vulkan.hpp>
#include <memory>
#include <vector>

using namespace Rl::World::Chunk;
using namespace Rl::Main;

class ChunkGeneratorGPUTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    // Use Game class with headless mode for proper Vulkan initialization
    Game& game = Game::GetInstance();
    game.SetHeadless(true);
    game.Init();

    // Get Vulkan resources from Game
    device = game.GetMainBinding().device;
    physicalDevice = game.GetMainBinding().physicalDevice;
    commandPool = game.GetMainBinding().commandPool;

    // Initialize chunk generator
    generator = std::make_unique<ChunkGeneratorGPU>();
    if (!generator->Initialize(device, physicalDevice, 42))
    {
      GTEST_SKIP() << "Failed to initialize chunk generator, skipping GPU tests";
      return;
    }

    // Create test registries
    biomeRegistry = std::make_unique<Rl::World::Biome::BiomeRegistryGPU>();

    // Skip stages that require registry data for testing
    generator->SetUnitRegistry(Rl::World::IUnit::GetGPUIDRegistry());
    generator->SetBiomeRegistry(biomeRegistry.get());
    generator->SetSkipPolFence(true);
  }

  void TearDown() override
  {
    if (generator)
    {
      generator->Shutdown(device);
    }

    if (biomeRegistry)
    {
      biomeRegistry->Shutdown(device);
    }

    // Don't destroy Game resources here, let the singleton handle cleanup at exit
    // The generator shutdown should clean up all GPU resources before device destruction
  }

  // Helper to execute command buffer and wait
  bool ExecuteCommandBuffer()
  {
    Game& game = Game::GetInstance();
    VkCommandBuffer cmdBuffer = game.GetMainBinding().commandBuffers[0];

    // Reset command buffer before recording
    if (vkResetCommandBuffer(cmdBuffer, 0) != VK_SUCCESS)
    {
      return false;
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(cmdBuffer, &beginInfo) != VK_SUCCESS)
    {
      return false;
    }

    return true;
  }

  bool SubmitAndWait()
  {
    Game& game = Game::GetInstance();
    VkCommandBuffer cmdBuffer = game.GetMainBinding().commandBuffers[0];
    vkEndCommandBuffer(cmdBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuffer;

    if (vkQueueSubmit(game.GetMainBinding().graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
    {
      return false;
    }

    vkQueueWaitIdle(game.GetMainBinding().graphicsQueue);
    return true;
  }

  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkDevice device = VK_NULL_HANDLE;
  VkCommandPool commandPool = VK_NULL_HANDLE;

  std::unique_ptr<ChunkGeneratorGPU> generator;
  std::unique_ptr<Rl::World::Biome::BiomeRegistryGPU> biomeRegistry;
};

TEST_F(ChunkGeneratorGPUTest, InitializeWithValidVulkanContext)
{
  ASSERT_NE(generator, nullptr);
  EXPECT_TRUE(generator->IsInitialized());
}

TEST_F(ChunkGeneratorGPUTest, GenerateChunkWithCPUReadback)
{
  if (!generator || !generator->IsInitialized())
  {
    return;
  }

  UnitChunkBuffer chunkBuffer;

  WorldChunkCoord coord{0, 0, 0};

  Game& game = Game::GetInstance();
  VkCommandBuffer cmdBuffer = game.GetMainBinding().commandBuffers[0];

  // Record command buffer
  ASSERT_TRUE(ExecuteCommandBuffer());

  // Generate chunk (records commands to cmdBuffer)
  bool generated = generator->GenerateChunk(device, cmdBuffer, coord, chunkBuffer);

  // Record readback command
  ASSERT_TRUE(generator->ReadbackUnitData(device, cmdBuffer, chunkBuffer));

  // Submit and wait for GPU to finish
  ASSERT_TRUE(SubmitAndWait());

  // Now complete the readback after GPU execution
  ASSERT_TRUE(generator->CompleteReadback(device, chunkBuffer));

  EXPECT_TRUE(generated);
}

TEST_F(ChunkGeneratorGPUTest, SkipStagesForPerformance)
{
  if (!generator || !generator->IsInitialized())
  {
    return;
  }

  UnitChunkBuffer chunkBuffer;
  WorldChunkCoord coord{0, 0, 0};

  Game& game = Game::GetInstance();
  VkCommandBuffer cmdBuffer = game.GetMainBinding().commandBuffers[0];

  ASSERT_TRUE(ExecuteCommandBuffer());

  bool generated = generator->GenerateChunk(device, cmdBuffer, coord, chunkBuffer);

  ASSERT_TRUE(SubmitAndWait());

  EXPECT_TRUE(generated);

  // Verify chunk has data
  uint32_t unitCount = 0;
  uint32_t airCount = 0;

  for (uint32_t y = 0; y < UnitChunkBuffer::H; ++y)
  {
    for (uint32_t x = 0; x < UnitChunkBuffer::W; ++x)
    {
      for (uint32_t z = 0; z < UnitChunkBuffer::D; ++z)
      {
        auto unitIdOpt = chunkBuffer.GetUnitIdXYZ(x, y, z);
        if (unitIdOpt.has_value() && unitIdOpt.value() != 0)
        {
          unitCount++;
        }
        else
        {
          airCount++;
        }
      }
    }
  }

  // Should have some units placed (not all air)
  EXPECT_GT(unitCount, 0u);
  EXPECT_GT(airCount, 0u);
}

TEST_F(ChunkGeneratorGPUTest, MultipleChunkGenerationConsistency)
{
  if (!generator || !generator->IsInitialized())
  {
    return;
  }

  UnitChunkBuffer chunk1, chunk2;

  WorldChunkCoord coord1{0, 0, 0};
  WorldChunkCoord coord2{0, 0, 0}; // Same coordinates

  Game& game = Game::GetInstance();
  VkCommandBuffer cmdBuffer = game.GetMainBinding().commandBuffers[0];

  ASSERT_TRUE(ExecuteCommandBuffer());
  bool generated1 = generator->GenerateChunk(device, cmdBuffer, coord1, chunk1);
  ASSERT_TRUE(generator->ReadbackUnitData(device, cmdBuffer, chunk1));
  ASSERT_TRUE(SubmitAndWait());
  ASSERT_TRUE(generator->CompleteReadback(device, chunk1));

  ASSERT_TRUE(ExecuteCommandBuffer());
  bool generated2 = generator->GenerateChunk(device, cmdBuffer, coord2, chunk2);
  ASSERT_TRUE(generator->ReadbackUnitData(device, cmdBuffer, chunk2));
  ASSERT_TRUE(SubmitAndWait());
  ASSERT_TRUE(generator->CompleteReadback(device, chunk2));

  EXPECT_TRUE(generated1);
  EXPECT_TRUE(generated2);

  // Same seed and coordinates should produce same results
  bool identical = true;
  for (uint32_t y = 0; y < UnitChunkBuffer::H && identical; ++y)
  {
    for (uint32_t x = 0; x < UnitChunkBuffer::W && identical; ++x)
    {
      for (uint32_t z = 0; z < UnitChunkBuffer::D && identical; ++z)
      {
        auto unitId1 = chunk1.GetUnitIdXYZ(x, y, z);
        auto unitId2 = chunk2.GetUnitIdXYZ(x, y, z);
        if (unitId1 != unitId2)
        {
          identical = false;
        }
      }
    }
  }

  EXPECT_TRUE(identical);
}

TEST_F(ChunkGeneratorGPUTest, DifferentCoordinatesProduceDifferentChunks)
{
  if (!generator || !generator->IsInitialized())
  {
    return;
  }

  UnitChunkBuffer chunk1, chunk2;

  WorldChunkCoord coord1{0, 0, 0};
  WorldChunkCoord coord2{10, 10, 10}; // Different coordinates

  Game& game = Game::GetInstance();
  VkCommandBuffer cmdBuffer = game.GetMainBinding().commandBuffers[0];

  ASSERT_TRUE(ExecuteCommandBuffer());
  bool generated1 = generator->GenerateChunk(device, cmdBuffer, coord1, chunk1);
  ASSERT_TRUE(generator->ReadbackUnitData(device, cmdBuffer, chunk1));
  ASSERT_TRUE(SubmitAndWait());
  ASSERT_TRUE(generator->CompleteReadback(device, chunk1));

  ASSERT_TRUE(ExecuteCommandBuffer());
  bool generated2 = generator->GenerateChunk(device, cmdBuffer, coord2, chunk2);
  ASSERT_TRUE(generator->ReadbackUnitData(device, cmdBuffer, chunk2));
  ASSERT_TRUE(SubmitAndWait());
  ASSERT_TRUE(generator->CompleteReadback(device, chunk2));

  EXPECT_TRUE(generated1);
  EXPECT_TRUE(generated2);

  // Different coordinates should produce different results
  bool different = false;
  for (uint32_t y = 0; y < UnitChunkBuffer::H && !different; ++y)
  {
    for (uint32_t x = 0; x < UnitChunkBuffer::W && !different; ++x)
    {
      for (uint32_t z = 0; z < UnitChunkBuffer::D && !different; ++z)
      {
        auto unitId1 = chunk1.GetUnitIdXYZ(x, y, z);
        auto unitId2 = chunk2.GetUnitIdXYZ(x, y, z);
        if (unitId1 != unitId2)
        {
          different = true;
        }
      }
    }
  }

  EXPECT_TRUE(different);
}

TEST_F(ChunkGeneratorGPUTest, GPUUnitBufferHandleValid)
{
  if (!generator || !generator->IsInitialized())
  {
    return;
  }

  VkBuffer gpuBuffer = generator->GetGPUUnitBuffer();
  EXPECT_NE(gpuBuffer, VK_NULL_HANDLE);
}

TEST_F(ChunkGeneratorGPUTest, ChunkBoundsAreRespected)
{
  if (!generator || !generator->IsInitialized())
  {
    return;
  }

  UnitChunkBuffer chunkBuffer;

  WorldChunkCoord coord{0, 0, 0};

  Game& game = Game::GetInstance();
  VkCommandBuffer cmdBuffer = game.GetMainBinding().commandBuffers[0];

  ASSERT_TRUE(ExecuteCommandBuffer());
  bool generated = generator->GenerateChunk(device, cmdBuffer, coord, chunkBuffer);
  ASSERT_TRUE(generator->ReadbackUnitData(device, cmdBuffer, chunkBuffer));
  ASSERT_TRUE(SubmitAndWait());
  ASSERT_TRUE(generator->CompleteReadback(device, chunkBuffer));

  EXPECT_TRUE(generated);

  // Check that all values are within valid range
  for (uint32_t y = 0; y < UnitChunkBuffer::H; ++y)
  {
    for (uint32_t x = 0; x < UnitChunkBuffer::W; ++x)
    {
      for (uint32_t z = 0; z < UnitChunkBuffer::D; ++z)
      {
        auto unitIdOpt = chunkBuffer.GetUnitIdXYZ(x, y, z);
        if (unitIdOpt.has_value())
        {
          uint32_t unitId = unitIdOpt.value();
          // Unit IDs should be reasonable (not garbage)
          EXPECT_LT(unitId, 1000u) << "Unit ID at position (" << x << ", " << y << ", " << z << ") is suspiciously large";
        }
      }
    }
  }
}
