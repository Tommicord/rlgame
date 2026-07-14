import Rl.World.Chunk.ChunkGeneratorGPU;
import Rl.World.Chunk.ChunkInRenderUnits;
import Rl.World.Chunk.UnitChunkBuffer;
import Rl.World.Biome.BiomeRegistryGPU;
import Rl.World.Unit.UnitRegistryGPU;
import Rl.World.Unit.UnitMantle;
import Rl.World.Unit.UnitGrass;
import Rl.World.Unit.UnitRegistry;
import Rl.Base.Binding;
import Rl.Base.Game;
import Rl.World.Unit;
import Rl.World.Biome;
import Rl.RayLog.Macro;
import Rl.RayLog.Logger;

// Disable validation layers for GPU tests to avoid fatal cleanup errors
extern bool disableValidationLayers;
bool disableValidationLayers = true;

#include <gtest/gtest.h>
#include <vulkan/vulkan.hpp>
#include <memory>
#include <vector>

using namespace Rl::World::Chunk;
using namespace Rl::Main;
using namespace Rl::RayLog;
using namespace Rl::World;
using namespace Rl::World::Biome;

struct TestBiome : IBiome
{
  BiomeNoiseLayer            temperatureLayer = {
    1.1, 0.5f, 0.2f, 0.9f, 3, 1.0, 1.5f, 1, 1.4
  };
  BiomeNoiseLayer            moistureLayer = {
    1.3, 0.5f, 0.2f, 0.9f, 3, 1.0, 1.2f, 1, 1.2
  };
  BiomeNoiseLayer            elevationLayer{
    1.3, 0.5f, 0.2f, 0.9f, 3, 1.0, 1.5f, 1, 1.3
  };
  std::vector<BiomeUnitRule> unitRules = {
    {
      UnitMantle::GetStaticClassId(),
      0.4,
      0.6,
      600.0,
      900.0,
      0.01,
      12.0,
      0.0,
      0.8,
      0.7,
      0.9
    },
    {
      UnitGrass::GetStaticClassId(),
      0.7,
      0.8,
      2.0f,
      40.0f,
      0.01,
      12.0,
      0.7,
      0.8,
      0.9,
      0.2
    }
  };
  float minTemperature = 0.1f;
  float maxTemperature = 1.0f;
  float minMoisture    = 0.0f;
  float maxMoisture    = 0.45f;
  float minElevation   = 0.2f;
  float maxElevation   = 0.9f;
  BiomeType biomeType  = { 1 };
  const char *biomeName = "TestLands";

  TestBiome() : IBiome(*this)
  {
  }
};

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

    auto* unitRegistry = IUnit::GetGPUIDRegistry();
    unitRegistry->Initialize(device, physicalDevice);
    
    // Update GPU buffer using temporary command buffer
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    poolInfo.queueFamilyIndex = 0; // Use first queue family (graphics)

    VkCommandPool tempCommandPool = VK_NULL_HANDLE;
    vkCreateCommandPool(device, &poolInfo, nullptr, &tempCommandPool);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = tempCommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer tempCmdBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &tempCmdBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(tempCmdBuffer, &beginInfo);

    unitRegistry->UpdateGPUBuffer(device, tempCmdBuffer);

    vkEndCommandBuffer(tempCmdBuffer);

    // Submit and wait for registry updates
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &tempCmdBuffer;
    vkQueueSubmit(game.GetMainBinding().graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(game.GetMainBinding().graphicsQueue);

    vkFreeCommandBuffers(device, tempCommandPool, 1, &tempCmdBuffer);
    vkDestroyCommandPool(device, tempCommandPool, nullptr);
    
    generator->SetUnitRegistry(unitRegistry);
    generator->SetBiomeRegistry(IBiome::GetGPUIDRegistry());
    generator->SetSkipPolFence(true);
  }

  void TearDown() override
  {
    if (generator)
    {
      generator->Shutdown(device);
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
  ASSERT_TRUE(generator->ReadbackUnitData(device, cmdBuffer, game.GetMainBinding().graphicsQueue, chunkBuffer));

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
  ASSERT_TRUE(generator->ReadbackUnitData(device, cmdBuffer, game.GetMainBinding().graphicsQueue, chunk1));

  ASSERT_TRUE(ExecuteCommandBuffer());
  bool generated2 = generator->GenerateChunk(device, cmdBuffer, coord2, chunk2);
  ASSERT_TRUE(generator->ReadbackUnitData(device, cmdBuffer, game.GetMainBinding().graphicsQueue, chunk2));

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
  ASSERT_TRUE(generator->ReadbackUnitData(device, cmdBuffer, game.GetMainBinding().graphicsQueue, chunk1));

  ASSERT_TRUE(ExecuteCommandBuffer());
  bool generated2 = generator->GenerateChunk(device, cmdBuffer, coord2, chunk2);
  ASSERT_TRUE(generator->ReadbackUnitData(device, cmdBuffer, game.GetMainBinding().graphicsQueue, chunk2));

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

  // Generate a chunk first to create the intermediate buffers
  UnitChunkBuffer chunkBuffer;
  WorldChunkCoord coord{0, 0, 0};
  
  ASSERT_TRUE(ExecuteCommandBuffer());
  
  Game& game = Game::GetInstance();
  VkCommandBuffer cmdBuffer = game.GetMainBinding().commandBuffers[0];
  
  ASSERT_TRUE(generator->GenerateChunk(device, cmdBuffer, coord, chunkBuffer));
  ASSERT_TRUE(SubmitAndWait());

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

  bool generated = generator->GenerateChunk(device, cmdBuffer, coord, chunkBuffer);
  EXPECT_TRUE(generated);
  
  // ReadbackUnitData creates its own temporary command buffer
  ASSERT_TRUE(generator->ReadbackUnitData(device, cmdBuffer, game.GetMainBinding().graphicsQueue, chunkBuffer));

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
          auto unitValue = Registry::GetObjectById(unitId).value();
          EXPECT_LT(unitId, 65535u) << "Unit ID at position (" << x << ", " << y << ", " << z << ") is suspiciously large";
          LogDebug(
            "ChunkGeneratorGPUTest",
            "Unit %s with id %d at (%d, %d, %d)",
            unitValue->GetDerivedClassName(),
            unitId,
            x, y, z
            );
        }
      }
    }
  }
}
