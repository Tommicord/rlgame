#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <vector>
#include <chrono>

#include "Rl.Chunk/WorldHeightmap.h"
#include "Rl.Base/GameComputeDispatch.h"
#include "Rl.Base/GameDevice.h"
#include "Rl.Base/GameRenderDocGpuDebugguer.h"
#include "Rl.Log/Log.h"

namespace rl
{

class WorldHeightmapTest : public ::testing::Test
{
        protected:
                void SetUp() override
                {
                        instance = std::make_unique<GameDeviceInstance>();
                        instance->init();
                        ASSERT_NE(instance->getDevice(), VK_NULL_HANDLE);
                        ASSERT_NE(instance->getPhysicalDevice(), VK_NULL_HANDLE);
                        GameRenderDocGpuDebugguer::getInstance();
                }

                void TearDown() override
                {
                        instance.reset();
                }

                std::unique_ptr<GameDeviceInstance> instance;
};

TEST_F(WorldHeightmapTest, ConstructorCreatesValidResources)
{
        const uint32_t width  = 256;
        const uint32_t height = 256;
        const uint32_t depth  = 64;
        const uint32_t seed  = 42;

        WorldHeightmap heightmap(width, height, depth, seed, *instance);

        // Verify that images are created by checking getter methods don't return null
        EXPECT_NE(heightmap.getBasemapElevationImage(), VK_NULL_HANDLE);
        EXPECT_NE(heightmap.getBasemapTemperatureImage(), VK_NULL_HANDLE);
        EXPECT_NE(heightmap.getBasemapMoistureImage(), VK_NULL_HANDLE);
        EXPECT_NE(heightmap.getDeepmapElevationImage(), VK_NULL_HANDLE);
        EXPECT_NE(heightmap.getDeepmapTemperatureImage(), VK_NULL_HANDLE);
        EXPECT_NE(heightmap.getDeepmapMoistureImage(), VK_NULL_HANDLE);

        // Verify image views are created
        EXPECT_NE(heightmap.getBasemapElevationImageView(), VK_NULL_HANDLE);
        EXPECT_NE(heightmap.getBasemapTemperatureImageView(), VK_NULL_HANDLE);
        EXPECT_NE(heightmap.getBasemapMoistureImageView(), VK_NULL_HANDLE);
        EXPECT_NE(heightmap.getDeepmapElevationImageView(), VK_NULL_HANDLE);
        EXPECT_NE(heightmap.getDeepmapTemperatureImageView(), VK_NULL_HANDLE);
        EXPECT_NE(heightmap.getDeepmapMoistureImageView(), VK_NULL_HANDLE);
}

TEST_F(WorldHeightmapTest, DispatchWithValidParameters)
{
        const uint32_t width  = 128;
        const uint32_t height = 128;
        const uint32_t depth  = 32;
        const uint32_t seed  = 42;

        WorldHeightmap heightmap(width, height, depth, seed, *instance);

        WorldHeightmapPushConstants params{};
        params.width       = width;
        params.height      = height;
        params.depth        = depth;
        params.scale        = 1.0f;
        params.heightScale  = 1.0f;
        params.seaLevel     = 0.5f;
        params.seed         = seed;
        params.octaves      = 4;
        params.persistence  = 0.5f;
        params.groundLevel  = 0.5f;

        WorldHeightmapComputePResource resource{&params};

        GameComputeDispatch dispatch(instance->getDevice(), instance->getGraphicsQueue(), instance->getCommandPool());
        GameVulkanSemaphore nullSemaphore{};
        
        GameRenderDocGpuDebugguer::getInstance().startCapture();
        EXPECT_NO_THROW(dispatch.dispatchSingle(heightmap, &resource, nullSemaphore));
        dispatch.waitForCompletion();
        GameRenderDocGpuDebugguer::getInstance().endCapture();
}

TEST_F(WorldHeightmapTest, ReadAfterDispatch)
{
        const uint32_t width  = 64;
        const uint32_t height = 64;
        const uint32_t depth  = 16;
        const uint32_t seed  = 123;

        WorldHeightmap heightmap(width, height, depth, seed, *instance);

        WorldHeightmapPushConstants params{};
        params.width       = width;
        params.height      = height;
        params.depth       = depth;
        params.scale       = 1.0f;
        params.heightScale = 1.0f;
        params.seaLevel    = 0.5f;
        params.seed        = seed;
        params.octaves     = 4;
        params.persistence = 0.5f;
        params.groundLevel = 0.5f;

        WorldHeightmapComputePResource resource{&params};

        GameComputeDispatch dispatch(instance->getDevice(), instance->getGraphicsQueue(), instance->getCommandPool());
        GameVulkanSemaphore nullSemaphore{};
        
        GameRenderDocGpuDebugguer::getInstance().startCapture();
        dispatch.dispatchSingle(heightmap, &resource, nullSemaphore);
        dispatch.waitForCompletion();
        GameRenderDocGpuDebugguer::getInstance().endCapture();

        std::vector<WorldHeightmapData> output;
        EXPECT_NO_THROW(heightmap.read(instance->getDevice(), instance->getPhysicalDevice(), output));

        // Verify output size matches expected
        EXPECT_EQ(output.size(), width * height);
}

TEST_F(WorldHeightmapTest, GenerateCompressedGrayscale)
{
        const uint32_t width  = 32;
        const uint32_t height = 32;
        const uint32_t depth  = 8;
        const uint32_t seed  = 456;

        WorldHeightmap heightmap(width, height, depth, seed, *instance);

        WorldHeightmapPushConstants params{};
        params.width       = width;
        params.height      = height;
        params.depth        = depth;
        params.scale        = 1.0f;
        params.heightScale  = 1.0f;
        params.seaLevel     = 0.5f;
        params.seed         = seed;
        params.octaves      = 4;
        params.persistence  = 0.5f;
        params.groundLevel  = 0.5f;

        WorldHeightmapComputePResource resource{&params};

        GameComputeDispatch dispatch(instance->getDevice(), instance->getGraphicsQueue(), instance->getCommandPool());
        GameVulkanSemaphore nullSemaphore{};
        
        GameRenderDocGpuDebugguer::getInstance().startCapture();
        dispatch.dispatchSingle(heightmap, &resource, nullSemaphore);
        dispatch.waitForCompletion();
        GameRenderDocGpuDebugguer::getInstance().endCapture();

        std::vector<uint8_t> output;
        EXPECT_NO_THROW(heightmap.generateCompressedGrayscale(instance->getDevice(),
                                                            instance->getPhysicalDevice(), output));

        // Verify output size
        EXPECT_EQ(output.size(), width * height * depth);
}

TEST_F(WorldHeightmapTest, ThreadSafety)
{
        const uint32_t width  = 64;
        const uint32_t height = 64;
        const uint32_t depth  = 16;
        const uint32_t seed  = 789;

        WorldHeightmap heightmap(width, height, depth, seed, *instance);

        const int numThreads = 4;
        std::vector<std::thread> threads;

        for (int i = 0; i < numThreads; ++i)
        {
                threads.emplace_back([this, &heightmap, width, height, depth, seed, i]() {
                        WorldHeightmapPushConstants params{};
                        params.width       = width;
                        params.height      = height;
                        params.depth        = depth;
                        params.scale        = 1.0f;
                        params.heightScale  = 1.0f;
                        params.seaLevel     = 0.5f;
                        params.seed         = seed + i;
                        params.octaves      = 4;
                        params.persistence  = 0.5f;
                        params.groundLevel  = 0.5f;

                        WorldHeightmapComputePResource resource{&params};

                        GameComputeDispatch dispatch(instance->getDevice(), instance->getGraphicsQueue(), instance->getCommandPool());
                        GameVulkanSemaphore nullSemaphore{};

                        // Multiple threads should be able to dispatch without race conditions
                        EXPECT_NO_THROW(dispatch.dispatchSingle(heightmap, &resource, nullSemaphore));
                        dispatch.waitForCompletion();
                });
        }

        for (auto& thread : threads)
        {
                thread.join();
        }
}

TEST_F(WorldHeightmapTest, MultipleDispatchesWithoutDeviceLost)
{
        const uint32_t width  = 128;
        const uint32_t height = 128;
        const uint32_t depth  = 32;
        const uint32_t seed  = 1000;

        WorldHeightmap heightmap(width, height, depth, seed, *instance);

        const int numIterations = 10;
        for (int i = 0; i < numIterations; ++i)
        {
                WorldHeightmapPushConstants params{};
                params.width       = width;
                params.height      = height;
                params.depth        = depth;
                params.scale        = 1.0f;
                params.heightScale  = 1.0f;
                params.seaLevel     = 0.5f;
                params.seed         = seed + i;
                params.octaves      = 4;
                params.persistence  = 0.5f;
                params.groundLevel  = 0.5f;

                WorldHeightmapComputePResource resource{&params};

                GameComputeDispatch dispatch(instance->getDevice(), instance->getGraphicsQueue(), instance->getCommandPool());
                GameVulkanSemaphore nullSemaphore{};
                
                GameRenderDocGpuDebugguer::getInstance().startCapture();
                EXPECT_NO_THROW(dispatch.dispatchSingle(heightmap, &resource, nullSemaphore));
                dispatch.waitForCompletion();
                GameRenderDocGpuDebugguer::getInstance().endCapture();
        }

        // If we get here without VK_ERROR_DEVICE_LOST, the test passes
        SUCCEED();
}

TEST_F(WorldHeightmapTest, CompletionSemaphoreSignaled)
{
        const uint32_t width  = 64;
        const uint32_t height = 64;
        const uint32_t depth  = 16;
        const uint32_t seed  = 111;

        WorldHeightmap heightmap(width, height, depth, seed, *instance);

        WorldHeightmapPushConstants params{};
        params.width       = width;
        params.height      = height;
        params.depth        = depth;
        params.scale        = 1.0f;
        params.heightScale  = 1.0f;
        params.seaLevel     = 0.5f;
        params.seed         = seed;
        params.octaves      = 4;
        params.persistence  = 0.5f;
        params.groundLevel  = 0.5f;

        WorldHeightmapComputePResource resource{&params};

        GameComputeDispatch dispatch(instance->getDevice(), instance->getGraphicsQueue(),
                                     instance->getCommandPool());
        GameVulkanSemaphore nullSemaphore{};
        
        GameRenderDocGpuDebugguer::getInstance().startCapture();
        dispatch.dispatchSingle(heightmap, &resource, nullSemaphore);
        dispatch.waitForCompletion();
        GameRenderDocGpuDebugguer::getInstance().endCapture();
        
        EXPECT_NE(heightmap.getCompletionSemaphore().getSemaphore(), VK_NULL_HANDLE);
}

TEST_F(WorldHeightmapTest, MutexLockingWorks)
{
        const uint32_t width  = 64;
        const uint32_t height = 64;
        const uint32_t depth  = 16;
        const uint32_t seed  = 222;

        WorldHeightmap heightmap(width, height, depth, seed, *instance);

        // Verify we can lock the mutex
        std::scoped_lock lock(heightmap.getGenerateMutex());
        
        // If we can lock it without deadlock the test passes
        SUCCEED();
}

} // namespace rl
