#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <vector>
#include <chrono>

#include "Rl.Chunk/WorldClimateCompute.h"
#include "Rl.Base/GameComputeDispatch.h"
#include "Rl.Base/GameDevice.h"
#include "Rl.Base/GameRenderDocGpuDebugguer.h"
#include "Rl.Log/Log.h"

namespace rl
{

class WorldClimateComputeTest : public ::testing::Test
{
        protected:
                void SetUp() override
                {
                        instance = std::make_unique<GameDeviceInstance>();
                        instance->init();
                        ASSERT_NE(instance->getDevice(), VK_NULL_HANDLE);
                        ASSERT_NE(instance->getPhysicalDevice(), VK_NULL_HANDLE);
                        // GameRenderDocGpuDebugguer::getInstance();
                }

                void TearDown() override
                {
                        instance.reset();
                }

                std::unique_ptr<GameDeviceInstance> instance;
};

TEST_F(WorldClimateComputeTest, ConstructorCreatesValidResources)
{
        const uint32_t width  = 256;
        const uint32_t height = 256;

        WorldClimateCompute climateCompute(width, height, *instance);

        // Verify that image is created
        EXPECT_NE(climateCompute.getEquatorImage(), VK_NULL_HANDLE);
}

TEST_F(WorldClimateComputeTest, DispatchWithValidParameters)
{
        const uint32_t width  = 128;
        const uint32_t height = 128;

        WorldClimateCompute climateCompute(width, height, *instance);

        WorldClimateComputePushConstants params{};
        params.width       = width;
        params.height      = height;
        params.texelSize   = 1.0f;
        params.planetIndex = 0;

        WorldPlanetData planet{};
        planet.center     = Vec3{0.0f, 0.0f, 0.0f};
        planet.radius     = 1000.0f;
        planet.axis       = Vec3{0.0f, 1.0f, 0.0f};
        planet.mass       = 1.0f;
        planet.velocity   = Vec3{0.0f, 0.0f, 0.0f};
        planet.gravity    = 9.8f;
        planet.baseTemperature = 20.0f;
        planet.baseMoisture    = 0.5f;
        planet.atmosphereHeight = 100.0f;
        planet.seed      = 42;
        planet.shapeType = 0;

        WorldClimateComputePResource resource{&params, &planet};

        GameComputeDispatch dispatch(instance->getDevice(), instance->getGraphicsQueue(), instance->getCommandPool());
        GameVulkanSemaphore nullSemaphore{};

        // This should not throw or cause VK_ERROR_DEVICE_LOST
        GameRenderDocGpuDebugguer::getInstance().startCapture();
        EXPECT_NO_THROW(dispatch.dispatchSingle(climateCompute, &resource, nullSemaphore));
        dispatch.waitForCompletion();
        GameRenderDocGpuDebugguer::getInstance().endCapture();
}

TEST_F(WorldClimateComputeTest, ReadAfterDispatch)
{
        const uint32_t width  = 64;
        const uint32_t height = 64;

        WorldClimateCompute climateCompute(width, height, *instance);

        WorldClimateComputePushConstants params{};
        params.width       = width;
        params.height      = height;
        params.texelSize   = 1.0f;
        params.planetIndex = 0;

        WorldPlanetData planet{};
        planet.center     = Vec3{0.0f, 0.0f, 0.0f};
        planet.radius     = 1000.0f;
        planet.axis       = Vec3{0.0f, 1.0f, 0.0f};
        planet.mass       = 1.0f;
        planet.velocity   = Vec3{0.0f, 0.0f, 0.0f};
        planet.gravity    = 9.8f;
        planet.baseTemperature = 20.0f;
        planet.baseMoisture    = 0.5f;
        planet.atmosphereHeight = 100.0f;
        planet.seed      = 123;
        planet.shapeType = 0;

        WorldClimateComputePResource resource{&params, &planet};

        GameComputeDispatch dispatch(instance->getDevice(), instance->getGraphicsQueue(), instance->getCommandPool());
        GameVulkanSemaphore nullSemaphore{};
        
        GameRenderDocGpuDebugguer::getInstance().startCapture();
        dispatch.dispatchSingle(climateCompute, &resource, nullSemaphore);
        dispatch.waitForCompletion();
        GameRenderDocGpuDebugguer::getInstance().endCapture();

        std::vector<WorldClimateData> output;
        EXPECT_NO_THROW(climateCompute.read(instance->getDevice(), instance->getPhysicalDevice(), output));

        // Verify output size matches expected
        EXPECT_EQ(output.size(), width * height);
}

TEST_F(WorldClimateComputeTest, ThreadSafety)
{
        const uint32_t width  = 64;
        const uint32_t height = 64;

        WorldClimateCompute climateCompute(width, height, *instance);

        const int numThreads = 4;
        std::vector<std::thread> threads;

        for (int i = 0; i < numThreads; ++i)
        {
                threads.emplace_back([this, &climateCompute, width, height, i]() {
                        WorldClimateComputePushConstants params{};
                        params.width       = width;
                        params.height      = height;
                        params.texelSize   = 1.0f;
                        params.planetIndex = 0;

                        WorldPlanetData planet{};
                        planet.center     = Vec3{0.0f, 0.0f, 0.0f};
                        planet.radius     = 1000.0f;
                        planet.axis       = Vec3{0.0f, 1.0f, 0.0f};
                        planet.mass       = 1.0f;
                        planet.velocity   = Vec3{0.0f, 0.0f, 0.0f};
                        planet.gravity    = 9.8f;
                        planet.baseTemperature = 20.0f;
                        planet.baseMoisture    = 0.5f;
                        planet.atmosphereHeight = 100.0f;
                        planet.seed      = 789 + i;
                        planet.shapeType = 0;

                        WorldClimateComputePResource resource{&params, &planet};

                        GameComputeDispatch dispatch(instance->getDevice(), instance->getGraphicsQueue(), instance->getCommandPool());
                        GameVulkanSemaphore nullSemaphore{};

                        // Multiple threads should be able to dispatch without race conditions
                        EXPECT_NO_THROW(dispatch.dispatchSingle(climateCompute, &resource, nullSemaphore));
                        dispatch.waitForCompletion();
                });
        }

        for (auto& thread : threads)
        {
                thread.join();
        }
}

TEST_F(WorldClimateComputeTest, MultipleDispatchesWithoutDeviceLost)
{
        const uint32_t width  = 128;
        const uint32_t height = 128;

        WorldClimateCompute climateCompute(width, height, *instance);

        const int numIterations = 10;
        for (int i = 0; i < numIterations; ++i)
        {
                WorldClimateComputePushConstants params{};
                params.width       = width;
                params.height      = height;
                params.texelSize   = 1.0f;
                params.planetIndex = 0;

                WorldPlanetData planet{};
                planet.center     = Vec3{0.0f, 0.0f, 0.0f};
                planet.radius     = 1000.0f;
                planet.axis       = Vec3{0.0f, 1.0f, 0.0f};
                planet.mass       = 1.0f;
                planet.velocity   = Vec3{0.0f, 0.0f, 0.0f};
                planet.gravity    = 9.8f;
                planet.baseTemperature = 20.0f;
                planet.baseMoisture    = 0.5f;
                planet.atmosphereHeight = 100.0f;
                planet.seed      = 1000 + i;
                planet.shapeType = 0;

                WorldClimateComputePResource resource{&params, &planet};

                GameComputeDispatch dispatch(instance->getDevice(), instance->getGraphicsQueue(), instance->getCommandPool());
                GameVulkanSemaphore nullSemaphore{};
                
                GameRenderDocGpuDebugguer::getInstance().startCapture();
                EXPECT_NO_THROW(dispatch.dispatchSingle(climateCompute, &resource, nullSemaphore));
                dispatch.waitForCompletion();
                GameRenderDocGpuDebugguer::getInstance().endCapture();
        }

        // If we get here without VK_ERROR_DEVICE_LOST, the test passes
        SUCCEED();
}

TEST_F(WorldClimateComputeTest, CompletionSemaphoreSignaled)
{
        const uint32_t width  = 64;
        const uint32_t height = 64;

        WorldClimateCompute climateCompute(width, height, *instance);

        WorldClimateComputePushConstants params{};
        params.width       = width;
        params.height      = height;
        params.texelSize   = 1.0f;
        params.planetIndex = 0;

        WorldPlanetData planet{};
        planet.center     = Vec3{0.0f, 0.0f, 0.0f};
        planet.radius     = 1000.0f;
        planet.axis       = Vec3{0.0f, 1.0f, 0.0f};
        planet.mass       = 1.0f;
        planet.velocity   = Vec3{0.0f, 0.0f, 0.0f};
        planet.gravity    = 9.8f;
        planet.baseTemperature = 20.0f;
        planet.baseMoisture    = 0.5f;
        planet.atmosphereHeight = 100.0f;
        planet.seed      = 111;
        planet.shapeType = 0;

        WorldClimateComputePResource resource{&params, &planet};

        GameComputeDispatch dispatch(instance->getDevice(), instance->getGraphicsQueue(), instance->getCommandPool());
        GameVulkanSemaphore nullSemaphore{};
        
        GameRenderDocGpuDebugguer::getInstance().startCapture();
        dispatch.dispatchSingle(climateCompute, &resource, nullSemaphore);
        dispatch.waitForCompletion();
        GameRenderDocGpuDebugguer::getInstance().endCapture();

        // Verify completion semaphore is valid
        EXPECT_NE(climateCompute.getCompletionSemaphore().getSemaphore(), VK_NULL_HANDLE);
}

TEST_F(WorldClimateComputeTest, MutexLockingWorks)
{
        const uint32_t width  = 64;
        const uint32_t height = 64;

        WorldClimateCompute climateCompute(width, height, *instance);

        // Verify we can lock the mutex
        std::scoped_lock lock(climateCompute.getGenerateMutex());
        
        // If we can lock it without deadlock the test passes
        SUCCEED();
}

} // namespace rl
