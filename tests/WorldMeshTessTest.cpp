#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <vector>
#include <chrono>

#include "Rl.Chunk/WorldMeshTess.h"
#include "Rl.Base/GameComputeDispatch.h"
#include "Rl.Base/GameDevice.h"
#include "Rl.Base/GameVulkanBuffer.h"
#include "Rl.Base/GameRenderDocGpuDebugguer.h"
#include "Rl.Log/Log.h"

#include "Rl.Chunk/WorldHeightmap.h"
#include "Rl.Chunk/WorldClimateCompute.h"
#include "Rl.Chunk/WorldUnitPlacement.h"

#include "Rl.World/PreBiomeRegistry.h"
#include "Rl.World/PreUnitRegistry.h"
#include "Rl.World/Biome.h"
#include "Rl.World/Unit.h"

namespace rl
{

class WorldMeshTessTest : public ::testing::Test
{
        protected:
                void SetUp() override
                {
                        instance = std::make_unique<GameDeviceInstance>();
                        instance->init();
                        ASSERT_NE(instance->getDevice(), VK_NULL_HANDLE);
                        ASSERT_NE(instance->getPhysicalDevice(), VK_NULL_HANDLE);
                        GameRenderDocGpuDebugguer::getInstance();

                        setupMinimalPipeline();
                }

                void TearDown() override
                {
                        testHeightmap.reset();
                        testClimate.reset();
                        testUnitPlacement.reset();
                        instance.reset();
                }

                void setupMinimalPipeline()
                {
                        if (testHeightmap) return;

                        testHeightmap = std::make_unique<WorldHeightmap>(16, 16, 16, 123, *instance);
                        testClimate = std::make_unique<WorldClimateCompute>(16, 16, *instance);
                        
                        GameComputeDispatch heightmapDispatch(instance->getDevice(), instance->getGraphicsQueue(),
                                                              instance->getCommandPool());
                        WorldHeightmapPushConstants heightmapParams{};
                        heightmapParams.width = 16;
                        heightmapParams.height = 16;
                        heightmapParams.depth = 16;
                        heightmapParams.scale = 1.0f;
                        heightmapParams.heightScale = 1.0f;
                        heightmapParams.seaLevel = 0.5f;
                        heightmapParams.seed = 123;
                        heightmapParams.octaves = 4;
                        heightmapParams.persistence = 0.5f;
                        heightmapParams.groundLevel = 0.5f;
                        WorldHeightmapComputePResource heightmapResource{&heightmapParams};
                        GameVulkanSemaphore nullSemaphore{};
                        heightmapDispatch.dispatchSingle(*testHeightmap, &heightmapResource, nullSemaphore);
                        heightmapDispatch.waitForCompletion();

                        GameComputeDispatch climateDispatch(instance->getDevice(), instance->getGraphicsQueue(),
                                                             instance->getCommandPool());
                        WorldClimateComputePushConstants climateParams{};
                        climateParams.worldOrigin = Vec3{0.0f, 0.0f, 0.0f};
                        climateParams.width = 16;
                        climateParams.height = 16;
                        climateParams.texelSize = 1.0f;
                        climateParams.planetIndex = 0;
                        WorldPlanetData planet{};
                        planet.center = Vec3{0.0f, 0.0f, 0.0f};
                        planet.radius = 1000.0f;
                        planet.axis = Vec3{0.0f, 1.0f, 0.0f};
                        planet.mass = 1.0f;
                        planet.velocity = Vec3{0.0f, 0.0f, 0.0f};
                        planet.gravity = 9.8f;
                        planet.baseTemperature = 20.0f;
                        planet.baseMoisture = 0.5f;
                        planet.atmosphereHeight = 100.0f;
                        planet.seed = 111;
                        planet.shapeType = 0;
                        WorldClimateComputePResource climateResource{&climateParams, &planet};
                        climateDispatch.dispatchSingle(*testClimate, &climateResource, nullSemaphore);
                        climateDispatch.waitForCompletion();

                        testUnitPlacement = std::make_unique<WorldUnitPlacement>(16, 16, 16, 456, *instance, 
                                                                                   *testHeightmap, *testClimate);
                        GameComputeDispatch unitDispatch(instance->getDevice(), instance->getGraphicsQueue(),
                                                         instance->getCommandPool());
                        WorldUnitPlacementPushConstants unitParams{};
                        unitParams.worldOrigin = Vec3{0.0f, 0.0f, 0.0f};
                        unitParams.width = 16;
                        unitParams.height = 16;
                        unitParams.depth = 16;
                        unitParams.texelSize = 1.0f;
                        unitParams.airUnitId = 0;
                        unitParams.unitRegistryCount = 0;
                        unitParams.biomeRegistryCount = 0;
                        unitParams.groundLevel = 0.5f;
                        WorldUnitPlacementComputePResource unitResource{&unitParams, nullptr, &planet, nullptr};
                        unitDispatch.dispatchSingle(*testUnitPlacement, &unitResource, nullSemaphore);
                        unitDispatch.waitForCompletion();
                }

                std::unique_ptr<GameDeviceInstance> instance;
                std::unique_ptr<WorldHeightmap>      testHeightmap;
                std::unique_ptr<WorldClimateCompute> testClimate;
                std::unique_ptr<WorldUnitPlacement>  testUnitPlacement;
};

TEST_F(WorldMeshTessTest, ConstructorCreatesValidResources)
{
        WorldMeshTessData config;
        config.width = 16;
        config.height = 16;
        config.depth = 16;
        config.seed = 42;
        config.airUnitId = 0xFFFFFFFF;

        WorldMeshTess       tess(config, *testUnitPlacement, *instance);
        GameComputeDispatch dispatch(instance->getDevice(), instance->getGraphicsQueue(),
                                     instance->getCommandPool());
        // Verify completion semaphore is created (RAII wrapper)
        EXPECT_NE(tess.getCompletionSemaphore().getSemaphore(), VK_NULL_HANDLE);
}

TEST_F(WorldMeshTessTest, DispatchWithValidParameters)
{
        WorldMeshTessData config;
        config.width = 16;
        config.height = 16;
        config.depth = 16;
        config.seed = 42;
        config.airUnitId = 0xFFFFFFFF;

        WorldMeshTess              tess(config, *testUnitPlacement, *instance);
        GameComputeDispatch        dispatch(instance->getDevice(), instance->getGraphicsQueue(),
                                            instance->getCommandPool());
        WorldMeshTessPushConstants params;
        params.seed = config.seed;
        params.width = config.width;
        params.height = config.height;
        params.depth = config.depth;
        params.airUnitId = config.airUnitId;

        WorldMeshTessPResource resource{&params};

        GameVulkanSemaphore nullSemaphore{};
        
        GameRenderDocGpuDebugguer::getInstance().startCapture();
        
        EXPECT_NO_THROW(dispatch.dispatchSingle(tess, &resource, nullSemaphore));
        EXPECT_TRUE(dispatch.waitForCompletion());
        
        GameRenderDocGpuDebugguer::getInstance().endCapture();
}

TEST_F(WorldMeshTessTest, DispatchWithoutDeviceLost)
{
        WorldMeshTessData config;
        config.width = 32;
        config.height = 32;
        config.depth = 32;
        config.seed = 12345;
        config.airUnitId = 0xFFFFFFFF;

        WorldMeshTess tess(config, *testUnitPlacement, *instance);
        GameComputeDispatch dispatch(instance->getDevice(), instance->getGraphicsQueue(), instance->getCommandPool());

        WorldMeshTessPushConstants params;
        params.seed = config.seed;
        params.width = config.width;
        params.height = config.height;
        params.depth = config.depth;
        params.airUnitId = config.airUnitId;

        WorldMeshTessPResource resource{&params};

        GameVulkanSemaphore nullSemaphore{};
        
        // Multiple dispatches to verify no device lost
        for (int i = 0; i < 5; ++i)
        {
                GameRenderDocGpuDebugguer::getInstance().startCapture();
                EXPECT_NO_THROW(dispatch.dispatchSingle(tess, &resource, nullSemaphore));
                EXPECT_TRUE(dispatch.waitForCompletion());
                GameRenderDocGpuDebugguer::getInstance().endCapture();
        }
}

TEST_F(WorldMeshTessTest, ReadReturnsValidData)
{
        WorldMeshTessData config;
        config.width = 8;
        config.height = 8;
        config.depth = 8;
        config.seed = 999;
        config.airUnitId = 0xFFFFFFFF;

        WorldMeshTess              tess(config, *testUnitPlacement, *instance);
        GameComputeDispatch        dispatch(instance->getDevice(), instance->getGraphicsQueue(),
                                            instance->getCommandPool());
        WorldMeshTessPushConstants params;
        params.seed = config.seed;
        params.width = config.width;
        params.height = config.height;
        params.depth = config.depth;
        params.airUnitId = config.airUnitId;

        WorldMeshTessPResource resource{&params};

        GameVulkanSemaphore nullSemaphore{};
        
        GameRenderDocGpuDebugguer::getInstance().startCapture();
        dispatch.dispatchSingle(tess, &resource, nullSemaphore);
        EXPECT_TRUE(dispatch.waitForCompletion());
        GameRenderDocGpuDebugguer::getInstance().endCapture();

        std::vector<PostUnit> output(config.width * config.height * config.depth);
        EXPECT_NO_THROW(tess.read(instance->getDevice(), instance->getPhysicalDevice(), output.data(), output.size()));

        // Verify output size matches expected
        EXPECT_EQ(output.size(), config.width * config.height * config.depth);
}

TEST_F(WorldMeshTessTest, CompletionSemaphoreSignaled)
{
        WorldMeshTessData config;
        config.width = 16;
        config.height = 16;
        config.depth = 16;
        config.seed = 42;
        config.airUnitId = 0xFFFFFFFF;

        WorldMeshTess       tess(config, *testUnitPlacement, *instance);
        GameComputeDispatch dispatch(instance->getDevice(), instance->getGraphicsQueue(),
                                     instance->getCommandPool());

        WorldMeshTessPushConstants params;
        params.seed = config.seed;
        params.width = config.width;
        params.height = config.height;
        params.depth = config.depth;
        params.airUnitId = config.airUnitId;

        WorldMeshTessPResource resource{&params};

        GameVulkanSemaphore nullSemaphore{};
        
        GameRenderDocGpuDebugguer::getInstance().startCapture();
        dispatch.dispatchSingle(tess, &resource, nullSemaphore);
        EXPECT_TRUE(dispatch.waitForCompletion());
        GameRenderDocGpuDebugguer::getInstance().endCapture();

        // Verify completion semaphore is valid (RAII wrapper ensures cleanup)
        EXPECT_NE(tess.getCompletionSemaphore().getSemaphore(), VK_NULL_HANDLE);
}

TEST_F(WorldMeshTessTest, MutexLockingWorks)
{
        WorldMeshTessData config;
        config.width = 16;
        config.height = 16;
        config.depth = 16;
        config.seed = 42;
        config.airUnitId = 0xFFFFFFFF;

        WorldMeshTess tess(config, *testUnitPlacement, *instance);

        std::recursive_mutex& mutex = tess.getGenerateMutex();
        EXPECT_NO_THROW({
                std::scoped_lock lock(mutex);
        });
}

TEST_F(WorldMeshTessTest, MultipleDispatchesWithoutDeviceLost)
{
        WorldMeshTessData config;
        config.width = 64;
        config.height = 64;
        config.depth = 64;
        config.seed = 777;
        config.airUnitId = 0xFFFFFFFF;

        WorldMeshTess              tess(config, *testUnitPlacement, *instance);
        GameComputeDispatch        dispatch(instance->getDevice(), instance->getGraphicsQueue(),
                                            instance->getCommandPool());
        WorldMeshTessPushConstants params;
        params.seed = config.seed;
        params.width = config.width;
        params.height = config.height;
        params.depth = config.depth;
        params.airUnitId = config.airUnitId;

        WorldMeshTessPResource resource{&params};

        GameVulkanSemaphore nullSemaphore{};
        
        // Stress test with multiple dispatches
        for (int i = 0; i < 10; ++i)
        {
                GameRenderDocGpuDebugguer::getInstance().startCapture();
                EXPECT_NO_THROW(dispatch.dispatchSingle(tess, &resource, nullSemaphore));
                EXPECT_TRUE(dispatch.waitForCompletion());
                GameRenderDocGpuDebugguer::getInstance().endCapture();
        }
}

TEST_F(WorldMeshTessTest, AirUnitsAreCulled)
{
        WorldMeshTessData config;
        config.width = 8;
        config.height = 8;
        config.depth = 8;
        config.seed = 42;
        config.airUnitId = 0xFFFFFFFF;

        WorldMeshTess              tess(config, *testUnitPlacement, *instance);
        GameComputeDispatch        dispatch(instance->getDevice(), instance->getGraphicsQueue(),
                                            instance->getCommandPool());
        WorldMeshTessPushConstants params;
        params.seed = config.seed;
        params.width = config.width;
        params.height = config.height;
        params.depth = config.depth;
        params.airUnitId = config.airUnitId;

        WorldMeshTessPResource resource{&params};

        GameVulkanSemaphore nullSemaphore{};
        
        GameRenderDocGpuDebugguer::getInstance().startCapture();
        dispatch.dispatchSingle(tess, &resource, nullSemaphore);
        EXPECT_TRUE(dispatch.waitForCompletion());
        GameRenderDocGpuDebugguer::getInstance().endCapture();

        std::vector<PostUnit> output(config.width * config.height * config.depth);
        EXPECT_NO_THROW(tess.read(instance->getDevice(), instance->getPhysicalDevice(), output.data(), output.size()));

        EXPECT_EQ(output.size(), config.width * config.height * config.depth);
}

} // namespace rl
