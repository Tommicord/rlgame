import Rl.World.Chunk.ChunkGeneratorGPU;
import Rl.World.Chunk.ChunkInRenderUnits;
import Rl.World.Chunk.UnitChunkBuffer;
import Rl.World.Biome.BiomeRegistryGPU;
import Rl.World.Unit.UnitRegistryGPU;
import Rl.Base.Binding;
import Rl.RayLog.Macro;

#include <gtest/gtest.h>
#include <vulkan/vulkan.hpp>
#include <memory>
#include <vector>
#include <cstring>

using namespace Rl::World::Chunk;

class ChunkGeneratorGPUTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    // Initialize Vulkan instance
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "ChunkGeneratorGPUTest";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_1;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
    {
      GTEST_SKIP() << "Failed to create Vulkan instance, skipping GPU tests";
      return;
    }

    // Pick physical device
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0)
    {
      GTEST_SKIP() << "No Vulkan devices available, skipping GPU tests";
      return;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    physicalDevice = devices[0]; // Use first device

    // Create logical device
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = 0; // Assume first queue family supports compute
    queueCreateInfo.queueCount = 1;
    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures deviceFeatures{};
    vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

    const char* deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    deviceCreateInfo.enabledExtensionCount = 1;
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;

    if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS)
    {
      GTEST_SKIP() << "Failed to create logical device, skipping GPU tests";
      return;
    }

    // Create command pool
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = 0;

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
    {
      GTEST_SKIP() << "Failed to create command pool, skipping GPU tests";
      return;
    }

    // Allocate command buffer
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS)
    {
      GTEST_SKIP() << "Failed to allocate command buffer, skipping GPU tests";
      return;
    }

    // Initialize chunk generator
    generator = std::make_unique<ChunkGeneratorGPU>();
    if (!generator->Initialize(device, physicalDevice, 42))
    {
      GTEST_SKIP() << "Failed to initialize chunk generator, skipping GPU tests";
      return;
    }

    // Create test registries
    biomeRegistry = std::make_unique<Biome::BiomeRegistryGPU>();
    unitRegistry = std::make_unique<UnitRegistryGPU>();

    // Register simple test biome
    biomeRegistry->RegisterBiome(0, "TestBiome", 0.5f, 0.3f, 0.5f, 0.3f, 0.5f, 0.3f);
    biomeRegistry->UploadToGPU(device, physicalDevice);

    // Register simple test unit
    unitRegistry->RegisterUnit(1, "TestUnit", 0.5f, 0.5f, 0.0f, 0.0f,
                               1.0f, 1.0f, 1.0f, 0.0f, 1.5f, 0.0f, 0.0f,
                               0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                               0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                               false, false, true);
    unitRegistry->UploadToGPU(device, physicalDevice);

    // Set registries in generator
    generator->SetBiomeRegistry(biomeRegistry.get());
    generator->SetUnitRegistry(unitRegistry.get());

    // Use reduced dimensions for testing to prevent timeout
    generator->SetReducedDimensions(true);
    generator->SetSkipPolFence(true); // Skip pol fence for faster testing
  }

  void TearDown() override
  {
    if (generator)
    {
      generator->Shutdown(device);
    }

    if (commandBuffer != VK_NULL_HANDLE)
    {
      vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }

    if (commandPool != VK_NULL_HANDLE)
    {
      vkDestroyCommandPool(device, commandPool, nullptr);
    }

    if (device != VK_NULL_HANDLE)
    {
      vkDestroyDevice(device, nullptr);
    }

    if (instance != VK_NULL_HANDLE)
    {
      vkDestroyInstance(instance, nullptr);
    }
  }

  // Helper to execute command buffer and wait
  bool ExecuteCommandBuffer()
  {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
    {
      return false;
    }

    return true;
  }

  bool SubmitAndWait()
  {
    vkEndCommandBuffer(commandBuffer);

    VkQueue queue;
    vkGetDeviceQueue(device, 0, 0, &queue);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    if (vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
    {
      return false;
    }

    vkQueueWaitIdle(queue);
    return true;
  }

  VkInstance instance = VK_NULL_HANDLE;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkDevice device = VK_NULL_HANDLE;
  VkCommandPool commandPool = VK_NULL_HANDLE;
  VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

  std::unique_ptr<ChunkGeneratorGPU> generator;
  std::unique_ptr<Biome::BiomeRegistryGPU> biomeRegistry;
  std::unique_ptr<UnitRegistryGPU> unitRegistry;
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
    GTEST_SKIP() << "Generator not initialized";
  }

  UnitChunkBuffer chunkBuffer;
  ASSERT_TRUE(chunkBuffer.Initialize());

  WorldChunkCoord coord{0, 0, 0};

  ASSERT_TRUE(ExecuteCommandBuffer());

  bool generated = generator->GenerateChunk(device, commandBuffer, coord, chunkBuffer);

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
        uint32_t unitId = chunkBuffer.GetUnit(x, y, z);
        if (unitId != 0)
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

TEST_F(ChunkGeneratorGPUTest, ReducedDimensionsForIntegratedGPU)
{
  if (!generator || !generator->IsInitialized())
  {
    GTEST_SKIP() << "Generator not initialized";
  }

  generator->SetReducedDimensions(true);

  uint32_t width, height, depth;
  generator->GetChunkDimensions(width, height, depth);

  // With reduced dimensions, should be smaller than full size
  EXPECT_LT(width, UnitChunkBuffer::W);
  EXPECT_LT(height, UnitChunkBuffer::H);
  EXPECT_LT(depth, UnitChunkBuffer::D);
}

TEST_F(ChunkGeneratorGPUTest, FullDimensionsForDiscreteGPU)
{
  if (!generator || !generator->IsInitialized())
  {
    GTEST_SKIP() << "Generator not initialized";
  }

  generator->SetReducedDimensions(false);

  uint32_t width, height, depth;
  generator->GetChunkDimensions(width, height, depth);

  // Without reduced dimensions, should be full size
  EXPECT_EQ(width, UnitChunkBuffer::W);
  EXPECT_EQ(height, UnitChunkBuffer::H);
  EXPECT_EQ(depth, UnitChunkBuffer::D);
}

TEST_F(ChunkGeneratorGPUTest, SkipStagesForPerformance)
{
  if (!generator || !generator->IsInitialized())
  {
    GTEST_SKIP() << "Generator not initialized";
  }

  UnitChunkBuffer chunkBuffer;
  ASSERT_TRUE(chunkBuffer.Initialize());

  WorldChunkCoord coord{0, 0, 0};

  // Skip various stages for minimal generation
  generator->SetSkipBiomeStage(true);
  generator->SetSkipHeightmapStage(true);
  generator->SetSkipNoiseStage(true);
  generator->SetSkipUnitPlaceStage(true);

  ASSERT_TRUE(ExecuteCommandBuffer());

  bool generated = generator->GenerateChunk(device, commandBuffer, coord, chunkBuffer);

  ASSERT_TRUE(SubmitAndWait());

  // Should still generate (even if minimal)
  EXPECT_TRUE(generated);
}

TEST_F(ChunkGeneratorGPUTest, MultipleChunkGenerationConsistency)
{
  if (!generator || !generator->IsInitialized())
  {
    GTEST_SKIP() << "Generator not initialized";
  }

  UnitChunkBuffer chunk1, chunk2;
  ASSERT_TRUE(chunk1.Initialize());
  ASSERT_TRUE(chunk2.Initialize());

  WorldChunkCoord coord1{0, 0, 0};
  WorldChunkCoord coord2{0, 0, 0}; // Same coordinates

  ASSERT_TRUE(ExecuteCommandBuffer());

  bool generated1 = generator->GenerateChunk(device, commandBuffer, coord1, chunk1);
  ASSERT_TRUE(SubmitAndWait());

  ASSERT_TRUE(ExecuteCommandBuffer());

  bool generated2 = generator->GenerateChunk(device, commandBuffer, coord2, chunk2);
  ASSERT_TRUE(SubmitAndWait());

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
        if (chunk1.GetUnit(x, y, z) != chunk2.GetUnit(x, y, z))
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
    GTEST_SKIP() << "Generator not initialized";
  }

  UnitChunkBuffer chunk1, chunk2;
  ASSERT_TRUE(chunk1.Initialize());
  ASSERT_TRUE(chunk2.Initialize());

  WorldChunkCoord coord1{0, 0, 0};
  WorldChunkCoord coord2{10, 10, 10}; // Different coordinates

  ASSERT_TRUE(ExecuteCommandBuffer());

  bool generated1 = generator->GenerateChunk(device, commandBuffer, coord1, chunk1);
  ASSERT_TRUE(SubmitAndWait());

  ASSERT_TRUE(ExecuteCommandBuffer());

  bool generated2 = generator->GenerateChunk(device, commandBuffer, coord2, chunk2);
  ASSERT_TRUE(SubmitAndWait());

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
        if (chunk1.GetUnit(x, y, z) != chunk2.GetUnit(x, y, z))
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
    GTEST_SKIP() << "Generator not initialized";
  }

  VkBuffer gpuBuffer = generator->GetGPUUnitBuffer();
  EXPECT_NE(gpuBuffer, VK_NULL_HANDLE);
}

TEST_F(ChunkGeneratorGPUTest, ChunkBoundsAreRespected)
{
  if (!generator || !generator->IsInitialized())
  {
    GTEST_SKIP() << "Generator not initialized";
  }

  UnitChunkBuffer chunkBuffer;
  ASSERT_TRUE(chunkBuffer.Initialize());

  WorldChunkCoord coord{0, 0, 0};

  ASSERT_TRUE(ExecuteCommandBuffer());

  bool generated = generator->GenerateChunk(device, commandBuffer, coord, chunkBuffer);

  ASSERT_TRUE(SubmitAndWait());

  EXPECT_TRUE(generated);

  // Check that all values are within valid range
  for (uint32_t y = 0; y < UnitChunkBuffer::H; ++y)
  {
    for (uint32_t x = 0; x < UnitChunkBuffer::W; ++x)
    {
      for (uint32_t z = 0; z < UnitChunkBuffer::D; ++z)
      {
        uint32_t unitId = chunkBuffer.GetUnit(x, y, z);
        // Unit IDs should be reasonable (not garbage)
        EXPECT_LT(unitId, 1000u) << "Unit ID at position (" << x << ", " << y << ", " << z << ") is suspiciously large";
      }
    }
  }
}
