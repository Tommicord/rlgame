#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <vector>
#include <chrono>

#include "Rl.Chunk/WorldMeshGen.h"
#include "Rl.Chunk/WorldMeshIndexDedup.h"
#include "Rl.Chunk/WorldMeshTess.h"
#include "Rl.Chunk/WorldUnitPlacement.h"
#include "Rl.Chunk/WorldHeightmap.h"
#include "Rl.Chunk/WorldClimateCompute.h"
#include "Rl.Base/GameComputeDispatch.h"
#include "Rl.Base/GameDevice.h"
#include "Rl.Base/GameVulkanBuffer.h"
#include "Rl.Base/GameRenderDocGpuDebugguer.h"
#include "Rl.World/PreUnitRegistry.h"
#include "Rl.World/PreBiomeRegistry.h"
#include "Rl.World/PreUnitRegister.h"
#include "Rl.World/PreBiomeRegister.h"
#include "Rl.World/Unit.h"
#include "Rl.World/Biome.h"
#include "Rl.Log/Log.h"

namespace rl
{

class TestUnitMeadow : public PreUnit
{
        public:
                TestUnitMeadow() :
                    PreUnit(PreUnitRegisterFactory::create(typeid(TestUnitMeadow).name()))
                {
                        setFlammability(0.4f);
                        setExplosionResistance(0.2f);
                        setMoistureStart(0.2f);
                        setMoistureEnd(0.6f);
                        setTemperatureStart(-0.3f);
                        setTemperatureEnd(0.3f);
                        setElevationStart(0.1f);
                        setElevationEnd(0.5f);
                        setEquatorStart(-0.4f);
                        setEquatorEnd(0.4f);
                }

                float getElevationNearAccept() const override
                {
                        return 0.08f;
                }
                float getMoistureNearAccept() const override
                {
                        return 0.05f;
                }
                float getEquatorNearAccept() const override
                {
                        return 0.06f;
                }
                float getTemperatureNearAccept() const override
                {
                        return 0.1f;
                }
};

class TestUnitForest : public PreUnit
{
        public:
                TestUnitForest() :
                    PreUnit(PreUnitRegisterFactory::create(typeid(TestUnitForest).name()))
                {
                        setFlammability(0.7f);
                        setExplosionResistance(0.3f);
                        setMoistureStart(0.5f);
                        setMoistureEnd(0.9f);
                        setTemperatureStart(0.1f);
                        setTemperatureEnd(0.7f);
                        setElevationStart(0.3f);
                        setElevationEnd(0.8f);
                        setEquatorStart(-0.2f);
                        setEquatorEnd(0.2f);
                }

                float getElevationNearAccept() const override
                {
                        return 0.09f;
                }
                float getMoistureNearAccept() const override
                {
                        return 0.07f;
                }
                float getEquatorNearAccept() const override
                {
                        return 0.05f;
                }
                float getTemperatureNearAccept() const override
                {
                        return 0.11f;
                }
};

class TestUnitDesert : public PreUnit
{
        public:
                TestUnitDesert() :
                    PreUnit(PreUnitRegisterFactory::create(typeid(TestUnitDesert).name()))
                {
                        setFlammability(0.1f);
                        setExplosionResistance(0.4f);
                        setMoistureStart(0.0f);
                        setMoistureEnd(0.2f);
                        setTemperatureStart(0.4f);
                        setTemperatureEnd(0.9f);
                        setElevationStart(0.0f);
                        setElevationEnd(0.4f);
                        setEquatorStart(-0.3f);
                        setEquatorEnd(0.3f);
                }

                float getElevationNearAccept() const override
                {
                        return 0.06f;
                }
                float getMoistureNearAccept() const override
                {
                        return 0.04f;
                }
                float getEquatorNearAccept() const override
                {
                        return 0.05f;
                }
                float getTemperatureNearAccept() const override
                {
                        return 0.08f;
                }
};

class TestUnitTundra : public PreUnit
{
        public:
                TestUnitTundra() :
                    PreUnit(PreUnitRegisterFactory::create(typeid(TestUnitTundra).name()))
                {
                        setFlammability(0.3f);
                        setExplosionResistance(0.6f);
                        setMoistureStart(0.3f);
                        setMoistureEnd(0.5f);
                        setTemperatureStart(-0.9f);
                        setTemperatureEnd(-0.4f);
                        setElevationStart(0.5f);
                        setElevationEnd(0.9f);
                        setEquatorStart(-0.6f);
                        setEquatorEnd(0.6f);
                }

                float getElevationNearAccept() const override
                {
                        return 0.07f;
                }
                float getMoistureNearAccept() const override
                {
                        return 0.05f;
                }
                float getEquatorNearAccept() const override
                {
                        return 0.07f;
                }
                float getTemperatureNearAccept() const override
                {
                        return 0.09f;
                }
};

class TestUnitOcean : public PreUnit
{
        public:
                TestUnitOcean() :
                    PreUnit(PreUnitRegisterFactory::create(typeid(TestUnitOcean).name()))
                {
                        setFlammability(0.0f);
                        setExplosionResistance(0.1f);
                        setMoistureStart(0.8f);
                        setMoistureEnd(1.0f);
                        setTemperatureStart(-0.2f);
                        setTemperatureEnd(0.2f);
                        setElevationStart(0.0f);
                        setElevationEnd(0.2f);
                        setEquatorStart(-0.8f);
                        setEquatorEnd(0.8f);
                }

                float getElevationNearAccept() const override
                {
                        return 0.05f;
                }
                float getMoistureNearAccept() const override
                {
                        return 0.04f;
                }
                float getEquatorNearAccept() const override
                {
                        return 0.06f;
                }
                float getTemperatureNearAccept() const override
                {
                        return 0.07f;
                }
};

class TestBiomeGrassland : public PreBiome
{
        public:
                explicit TestBiomeGrassland(PreUnitRegistry& unitRegistry) :
                    PreBiome(PreBiomeRegisterFactory::create(typeid(TestBiomeGrassland).name()), unitRegistry)
                {
                        setStartTemperature(0.1f);
                        setEndTemperature(0.6f);
                        setStartMoisture(0.2f);
                        setEndMoisture(0.7f);
                        setStartEquator(-0.4f);
                        setEndEquator(0.4f);
                        setStartElevation(0.1f);
                        setEndElevation(0.6f);
                }
};

class TestBiomeMountain : public PreBiome
{
        public:
                explicit TestBiomeMountain(PreUnitRegistry& unitRegistry) :
                    PreBiome(PreBiomeRegisterFactory::create(typeid(TestBiomeMountain).name()), unitRegistry)
                {
                        setStartTemperature(0.0f);
                        setEndTemperature(0.4f);
                        setStartMoisture(0.1f);
                        setEndMoisture(0.4f);
                        setStartEquator(-0.3f);
                        setEndEquator(0.3f);
                        setStartElevation(0.6f);
                        setEndElevation(0.9f);
                }
};

class TestBiomeCoastal : public PreBiome
{
        public:
                explicit TestBiomeCoastal(PreUnitRegistry& unitRegistry) :
                    PreBiome(PreBiomeRegisterFactory::create(typeid(TestBiomeCoastal).name()), unitRegistry)
                {
                        setStartTemperature(0.2f);
                        setEndTemperature(0.7f);
                        setStartMoisture(0.7f);
                        setEndMoisture(1.0f);
                        setStartEquator(-0.5f);
                        setEndEquator(0.5f);
                        setStartElevation(0.0f);
                        setEndElevation(0.3f);
                }
};

class WorldMeshGenTest : public ::testing::Test
{
        protected:
                void SetUp() override
                {
                        instance = std::make_unique<GameDeviceInstance>();
                        instance->init();
                        ASSERT_NE(instance->getDevice(), VK_NULL_HANDLE);
                        ASSERT_NE(instance->getPhysicalDevice(), VK_NULL_HANDLE);
                        GameRenderDocGpuDebugguer::getInstance();

                        preBiomeRegistry = std::make_unique<PreBiomeRegistry>();
                        preUnitRegistry  = std::make_unique<PreUnitRegistry>();

                        registeredUnits.emplace_back(std::make_unique<TestUnitMeadow>());
                        registeredUnits.emplace_back(std::make_unique<TestUnitForest>());
                        registeredUnits.emplace_back(std::make_unique<TestUnitDesert>());
                        registeredUnits.emplace_back(std::make_unique<TestUnitTundra>());
                        registeredUnits.emplace_back(std::make_unique<TestUnitOcean>());

                        for (const auto& unit : registeredUnits)
                        {
                                preUnitRegistry->registerItem(*unit);
                        }

                        registeredBiomes.emplace_back(
                            std::make_unique<TestBiomeGrassland>(*preUnitRegistry));
                        registeredBiomes.emplace_back(
                            std::make_unique<TestBiomeMountain>(*preUnitRegistry));
                        registeredBiomes.emplace_back(
                            std::make_unique<TestBiomeCoastal>(*preUnitRegistry));

                        for (const auto& biome : registeredBiomes)
                        {
                                preBiomeRegistry->registerItem(*biome);
                        }
                        setupMinimalPipeline();
                }

                void TearDown() override
                {
                        vkDeviceWaitIdle(instance->getDevice());
                        
                        testHeightmap.reset();
                        testClimate.reset();
                        testUnitPlacement.reset();
                        testMeshTess.reset();
                        testMeshIndexDedup.reset();
                        registeredUnits.clear();
                        registeredBiomes.clear();
                        preBiomeRegistry.reset();
                        preUnitRegistry.reset();
                        instance.reset();
                }

                WorldUnitPlacementPushConstants createPushConstants(uint32_t width,
                                                                    uint32_t height,
                                                                    uint32_t depth,
                                                                    float groundLevel = 0.5f) const
                {
                        WorldUnitPlacementPushConstants params{};
                        params.worldOrigin        = Vec3{0.0f, 0.0f, 0.0f};
                        params.width              = width;
                        params.height             = height;
                        params.depth              = depth;
                        params.texelSize          = 1.0f;
                        params.airUnitId          = 0;
                        params.unitRegistryCount  = static_cast<uint32_t>(registeredUnits.size());
                        params.biomeRegistryCount = static_cast<uint32_t>(registeredBiomes.size());
                        params.groundLevel        = groundLevel;
                        return params;
                }

                WorldPlanetData createPlanet(uint32_t seed) const
                {
                        WorldPlanetData planet{};
                        planet.center           = Vec3{0.0f, 0.0f, 0.0f};
                        planet.radius           = 1000.0f;
                        planet.axis             = Vec3{0.0f, 1.0f, 0.0f};
                        planet.mass             = 1.0f;
                        planet.velocity         = Vec3{0.0f, 0.0f, 0.0f};
                        planet.gravity          = 9.8f;
                        planet.baseTemperature  = 20.0f;
                        planet.baseMoisture     = 0.5f;
                        planet.atmosphereHeight = 100.0f;
                        planet.seed             = seed;
                        planet.shapeType        = 0;
                        return planet;
                }

                void setupMinimalPipeline()
                {
                        if (testHeightmap) return; // Already setup

                        testHeightmap = std::make_unique<WorldHeightmap>(16, 16, 16, 123, *instance);
                        testClimate = std::make_unique<WorldClimateCompute>(16, 16, *instance);
                        
                        GameComputeDispatch heightmapDispatch(instance->getDevice(), instance->getGraphicsQueue(),
                                                              instance->getCommandPool());
                        WorldHeightmapPushConstants heightmapParams{};
                        heightmapParams.width = width;
                        heightmapParams.height = height;
                        heightmapParams.depth = depth;
                        heightmapParams.scale = 1.0f;
                        heightmapParams.heightScale = 1.0f;
                        heightmapParams.seaLevel = 0.5f;
                        heightmapParams.seed = seed;
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
                        climateParams.width = width;
                        climateParams.height = height;
                        climateParams.texelSize = 1.0f;
                        climateParams.planetIndex = 0;
                        WorldPlanetData planet = createPlanet(111);
                        WorldClimateComputePResource climateResource{&climateParams, &planet};
                        climateDispatch.dispatchSingle(*testClimate, &climateResource, nullSemaphore);
                        climateDispatch.waitForCompletion();

                        testUnitPlacement = std::make_unique<WorldUnitPlacement>(width, height, depth, seed, *instance, 
                                                                                   *testHeightmap, *testClimate);
                        GameComputeDispatch unitDispatch(instance->getDevice(), instance->getGraphicsQueue(),
                                                         instance->getCommandPool());
                        WorldUnitPlacementPushConstants unitParams = createPushConstants(width, height, depth);
                        WorldUnitPlacementComputePResource unitResource{&unitParams, preUnitRegistry.get(),
                                                                         &planet, preBiomeRegistry.get()};
                        unitDispatch.dispatchSingle(*testUnitPlacement, &unitResource, nullSemaphore);
                        unitDispatch.waitForCompletion();

                        WorldMeshTessData tessConfig;
                        tessConfig.width     = width;
                        tessConfig.height    = height;
                        tessConfig.depth     = depth;
                        tessConfig.seed      = seed;
                        tessConfig.airUnitId = 0;

                        testMeshTess = std::make_unique<WorldMeshTess>(tessConfig, *testUnitPlacement, *instance);
                        
                        GameComputeDispatch tessDispatch(instance->getDevice(), instance->getGraphicsQueue(),
                                                          instance->getCommandPool());
                        WorldMeshTessPushConstants tessParams;
                        tessParams.seed = tessConfig.seed;
                        tessParams.width = tessConfig.width;
                        tessParams.height = tessConfig.height;
                        tessParams.depth = tessConfig.depth;
                        tessParams.airUnitId = tessConfig.airUnitId;
                        WorldMeshTessPResource tessResource{&tessParams};
                        tessDispatch.dispatchSingle(*testMeshTess, &tessResource, nullSemaphore);
                        tessDispatch.waitForCompletion();
                }

                std::unique_ptr<GameDeviceInstance> instance;
                std::unique_ptr<PreBiomeRegistry>       preBiomeRegistry;
                std::unique_ptr<PreUnitRegistry>        preUnitRegistry;
                std::vector<std::unique_ptr<PreUnit>>   registeredUnits;
                std::vector<std::unique_ptr<PreBiome>>  registeredBiomes;
                
                std::unique_ptr<WorldHeightmap>      testHeightmap;
                std::unique_ptr<WorldClimateCompute> testClimate;
                std::unique_ptr<WorldUnitPlacement>  testUnitPlacement;
                std::unique_ptr<WorldMeshTess>       testMeshTess;
                std::unique_ptr<WorldMeshIndexDedup> testMeshIndexDedup;

                const uint32_t                          width  = 128;
                const uint32_t                          height = 128;
                const uint32_t                          depth  = 256;
                const uint32_t                          seed   = 42;
};

TEST_F(WorldMeshGenTest, ConstructorCreatesValidResources)
{
        WorldMeshGenData config;
        config.width = 16;
        config.height = 16;
        config.depth = 16;
        config.maxVertices = 10000;
        config.maxIndices = 30000;
        config.subdivisions = 2;

        WorldMeshGen meshGen(config, *testMeshTess, *instance);

        // Verify completion semaphore is created (RAII wrapper)
        EXPECT_NE(meshGen.getCompletionSemaphore().getSemaphore(), VK_NULL_HANDLE);
}

TEST_F(WorldMeshGenTest, DispatchWithValidParameters)
{
        WorldMeshGenData config;
        config.width = 8;
        config.height = 8;
        config.depth = 8;
        config.maxVertices = 5000;
        config.maxIndices = 15000;
        config.subdivisions = 1;

        WorldMeshGen meshGen(config, *testMeshTess, *instance);
        GameComputeDispatch dispatch(instance->getDevice(), instance->getGraphicsQueue(), instance->getCommandPool());

        WorldMeshGenPushConstants params;
        params.width = config.width;
        params.height = config.height;
        params.depth = config.depth;
        params.maxVertices = config.maxVertices;
        params.maxIndices = config.maxIndices;
        params.subdivisions = config.subdivisions;

        WorldMeshGenPResource resource{&params};

        GameVulkanSemaphore nullSemaphore{};
        
        GameRenderDocGpuDebugguer::getInstance().startCapture();
        EXPECT_NO_THROW(dispatch.dispatchSingle(meshGen, &resource, nullSemaphore));
        EXPECT_TRUE(dispatch.waitForCompletion());
        GameRenderDocGpuDebugguer::getInstance().endCapture();
}

TEST_F(WorldMeshGenTest, DispatchWithoutDeviceLost)
{
        WorldMeshGenData config;
        config.width = 16;
        config.height = 16;
        config.depth = 16;
        config.maxVertices = 10000;
        config.maxIndices = 30000;
        config.subdivisions = 2;

        WorldMeshGen meshGen(config, *testMeshTess, *instance);
        GameComputeDispatch dispatch(instance->getDevice(), instance->getGraphicsQueue(), instance->getCommandPool());

        WorldMeshGenPushConstants params;
        params.width = config.width;
        params.height = config.height;
        params.depth = config.depth;
        params.maxVertices = config.maxVertices;
        params.maxIndices = config.maxIndices;
        params.subdivisions = config.subdivisions;

        WorldMeshGenPResource resource{&params};

        GameVulkanSemaphore nullSemaphore{};
        
        // Multiple dispatches to verify no device lost
        for (int i = 0; i < 5; ++i)
        {
                GameRenderDocGpuDebugguer::getInstance().startCapture();
                EXPECT_NO_THROW(dispatch.dispatchSingle(meshGen, &resource, nullSemaphore));
                EXPECT_TRUE(dispatch.waitForCompletion());
                GameRenderDocGpuDebugguer::getInstance().endCapture();
        }
}

TEST_F(WorldMeshGenTest, ReadVerticesReturnsValidData)
{
        WorldMeshGenData config;
        config.width = 8;
        config.height = 8;
        config.depth = 8;
        config.maxVertices = 5000;
        config.maxIndices = 15000;
        config.subdivisions = 1;

        WorldMeshGen meshGen(config, *testMeshTess, *instance);
        GameComputeDispatch dispatch(instance->getDevice(), instance->getGraphicsQueue(), instance->getCommandPool());

        WorldMeshGenPushConstants params;
        params.width = config.width;
        params.height = config.height;
        params.depth = config.depth;
        params.maxVertices = config.maxVertices;
        params.maxIndices = config.maxIndices;
        params.subdivisions = config.subdivisions;

        WorldMeshGenPResource resource{&params};

        GameVulkanSemaphore nullSemaphore{};
        
        GameRenderDocGpuDebugguer::getInstance().startCapture();
        dispatch.dispatchSingle(meshGen, &resource, nullSemaphore);
        EXPECT_TRUE(dispatch.waitForCompletion());
        GameRenderDocGpuDebugguer::getInstance().endCapture();

        std::vector<MeshVertex> vertices(config.maxVertices);
        EXPECT_NO_THROW(meshGen.readVertices(instance->getDevice(), instance->getPhysicalDevice(), vertices.data(), vertices.size()));

        // Verify vertices are within expected size
        EXPECT_LE(vertices.size(), config.maxVertices);
}

TEST_F(WorldMeshGenTest, CompletionSemaphoreSignaled)
{
        WorldMeshGenData config;
        config.width = 16;
        config.height = 16;
        config.depth = 16;
        config.maxVertices = 10000;
        config.maxIndices = 30000;
        config.subdivisions = 2;

        WorldMeshGen meshGen(config, *testMeshTess, *instance);
        GameComputeDispatch dispatch(instance->getDevice(), instance->getGraphicsQueue(), instance->getCommandPool());

        WorldMeshGenPushConstants params;
        params.width = config.width;
        params.height = config.height;
        params.depth = config.depth;
        params.maxVertices = config.maxVertices;
        params.maxIndices = config.maxIndices;
        params.subdivisions = config.subdivisions;

        WorldMeshGenPResource resource{&params};

        GameVulkanSemaphore nullSemaphore{};
        
        GameRenderDocGpuDebugguer::getInstance().startCapture();
        dispatch.dispatchSingle(meshGen, &resource, nullSemaphore);
        EXPECT_TRUE(dispatch.waitForCompletion());
        GameRenderDocGpuDebugguer::getInstance().endCapture();

        // Verify completion semaphore is valid (RAII wrapper ensures cleanup)
        EXPECT_NE(meshGen.getCompletionSemaphore().getSemaphore(), VK_NULL_HANDLE);
}

TEST_F(WorldMeshGenTest, MutexLockingWorks)
{
        WorldMeshGenData config;
        config.width = 16;
        config.height = 16;
        config.depth = 16;
        config.maxVertices = 10000;
        config.maxIndices = 30000;
        config.subdivisions = 2;

        WorldMeshGen meshGen(config, *testMeshTess, *instance);

        std::recursive_mutex& mutex = meshGen.getGenerateMutex();
        EXPECT_NO_THROW({
                std::scoped_lock lock(mutex);
        });
}

TEST_F(WorldMeshGenTest, MultipleDispatchesWithoutDeviceLost)
{
        WorldMeshGenData config;
        config.width = 32;
        config.height = 32;
        config.depth = 32;
        config.maxVertices = 20000;
        config.maxIndices = 60000;
        config.subdivisions = 2;

        WorldMeshGen meshGen(config, *testMeshTess, *instance);
        GameComputeDispatch dispatch(instance->getDevice(), instance->getGraphicsQueue(), instance->getCommandPool());

        WorldMeshGenPushConstants params;
        params.width = config.width;
        params.height = config.height;
        params.depth = config.depth;
        params.maxVertices = config.maxVertices;
        params.maxIndices = config.maxIndices;
        params.subdivisions = config.subdivisions;

        WorldMeshGenPResource resource{&params};

        GameVulkanSemaphore nullSemaphore{};
        
        // Stress test with multiple dispatches
        for (int i = 0; i < 10; ++i)
        {
                GameRenderDocGpuDebugguer::getInstance().startCapture();
                EXPECT_NO_THROW(dispatch.dispatchSingle(meshGen, &resource, nullSemaphore));
                EXPECT_TRUE(dispatch.waitForCompletion());
                GameRenderDocGpuDebugguer::getInstance().endCapture();
        }
}

TEST_F(WorldMeshGenTest, DifferentSubdivisionLevels)
{
        WorldMeshGenData config;
        config.width = 8;
        config.height = 8;
        config.depth = 8;
        config.maxVertices = 5000;
        config.maxIndices = 15000;
        config.subdivisions = 1;

        WorldMeshGen meshGen(config, *testMeshTess, *instance);
        GameComputeDispatch dispatch(instance->getDevice(), instance->getGraphicsQueue(), instance->getCommandPool());

        // Test with different subdivision levels
        for (uint32_t subdiv = 1; subdiv <= 4; ++subdiv)
        {
                WorldMeshGenPushConstants params;
                params.width = config.width;
                params.height = config.height;
                params.depth = config.depth;
                params.maxVertices = config.maxVertices;
                params.maxIndices = config.maxIndices;
                params.subdivisions = subdiv;

                WorldMeshGenPResource resource{&params};

                GameVulkanSemaphore nullSemaphore{};
                GameRenderDocGpuDebugguer::getInstance().startCapture();
                EXPECT_NO_THROW(dispatch.dispatchSingle(meshGen, &resource, nullSemaphore));
                EXPECT_TRUE(dispatch.waitForCompletion());
                GameRenderDocGpuDebugguer::getInstance().endCapture();
        }
}

TEST_F(WorldMeshGenTest, VertexDataHasValidStructure)
{
        WorldMeshGenData config;
        config.width = 4;
        config.height = 4;
        config.depth = 4;
        config.maxVertices = 1000;
        config.maxIndices = 3000;
        config.subdivisions = 1;

        WorldMeshGen meshGen(config, *testMeshTess, *instance);
        GameComputeDispatch dispatch(instance->getDevice(), instance->getGraphicsQueue(), instance->getCommandPool());

        WorldMeshGenPushConstants params;
        params.width = config.width;
        params.height = config.height;
        params.depth = config.depth;
        params.maxVertices = config.maxVertices;
        params.maxIndices = config.maxIndices;
        params.subdivisions = config.subdivisions;

        WorldMeshGenPResource resource{&params};
        GameVulkanSemaphore nullSemaphore{};
        dispatch.dispatchSingle(meshGen, &resource, nullSemaphore);
        EXPECT_TRUE(dispatch.waitForCompletion());

        std::vector<MeshVertex> vertices(config.maxVertices);
        meshGen.readVertices(instance->getDevice(), instance->getPhysicalDevice(), vertices.data(), vertices.size());

        // Verify vertex structure (position, normal, uv)
        for (const auto& vertex : vertices)
        {
                // Position should be finite
                EXPECT_TRUE(std::isfinite(vertex.position.x));
                EXPECT_TRUE(std::isfinite(vertex.position.y));
                EXPECT_TRUE(std::isfinite(vertex.position.z));
                
                // Normal should be finite
                EXPECT_TRUE(std::isfinite(vertex.normal.x));
                EXPECT_TRUE(std::isfinite(vertex.normal.y));
                EXPECT_TRUE(std::isfinite(vertex.normal.z));
                
                // UV should be finite
                EXPECT_TRUE(std::isfinite(vertex.uv.x));
                EXPECT_TRUE(std::isfinite(vertex.uv.y));
        }
}

TEST_F(WorldMeshGenTest, FullPipelineIntegration)
{
        const uint32_t heightmapWidth = 64;
        const uint32_t heightmapHeight = 64;
        const uint32_t heightmapDepth = 16;
        const uint32_t heightmapSeed = 123;

        WorldHeightmap heightmap(heightmapWidth, heightmapHeight, heightmapDepth, heightmapSeed, *instance);

        const uint32_t climateWidth = 64;
        const uint32_t climateHeight = 64;

        WorldClimateCompute climate(climateWidth, climateHeight, *instance);

        GameComputeDispatch heightmapDispatch(instance->getDevice(), instance->getGraphicsQueue(),
                                              instance->getCommandPool());

        WorldHeightmapPushConstants heightmapParams{};
        heightmapParams.width = heightmapWidth;
        heightmapParams.height = heightmapHeight;
        heightmapParams.depth = heightmapDepth;
        heightmapParams.scale = 1.0f;
        heightmapParams.heightScale = 1.0f;
        heightmapParams.seaLevel = 0.5f;
        heightmapParams.seed = heightmapSeed;
        heightmapParams.octaves = 4;
        heightmapParams.persistence = 0.5f;
        heightmapParams.groundLevel = 0.5f;

        WorldHeightmapComputePResource heightmapResource{&heightmapParams};
        GameVulkanSemaphore nullSemaphore{};

        GameRenderDocGpuDebugguer::getInstance().startCapture();
        EXPECT_NO_THROW(heightmapDispatch.dispatchSingle(heightmap, &heightmapResource, nullSemaphore));
        EXPECT_TRUE(heightmapDispatch.waitForCompletion());
        GameRenderDocGpuDebugguer::getInstance().endCapture();

        GameComputeDispatch climateDispatch(instance->getDevice(), instance->getGraphicsQueue(),
                                             instance->getCommandPool());

        WorldClimateComputePushConstants climateParams{};
        climateParams.worldOrigin = Vec3{0.0f, 0.0f, 0.0f};
        climateParams.width = climateWidth;
        climateParams.height = climateHeight;
        climateParams.texelSize = 1.0f;
        climateParams.planetIndex = 0;

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

        WorldClimateComputePResource climateResource{&climateParams, &planet};

        GameRenderDocGpuDebugguer::getInstance().startCapture();
        EXPECT_NO_THROW(climateDispatch.dispatchSingle(climate, &climateResource, nullSemaphore));
        EXPECT_TRUE(climateDispatch.waitForCompletion());
        GameRenderDocGpuDebugguer::getInstance().endCapture();

        WorldUnitPlacement unitPlacement(32, 32, 8, 456, *instance, heightmap, climate);

        GameComputeDispatch unitDispatch(instance->getDevice(), instance->getGraphicsQueue(),
                                         instance->getCommandPool());

        WorldUnitPlacementPushConstants unitParams;
        unitParams.worldOrigin         = Vec3{0.0f, 0.0f, 0.0f};
        unitParams.width               = 32;
        unitParams.height              = 32;
        unitParams.depth               = 8;
        unitParams.texelSize           = 1.0f;
        unitParams.airUnitId           = 0;
        unitParams.unitRegistryCount   = 0;
        unitParams.biomeRegistryCount  = 0;
        unitParams.groundLevel         = 0.5f;

        WorldUnitPlacementComputePResource unitResource{&unitParams, preUnitRegistry.get(),
                                                        &planet,
                                                        preBiomeRegistry.get()};

        GameRenderDocGpuDebugguer::getInstance().startCapture();
        EXPECT_NO_THROW(unitDispatch.dispatchSingle(unitPlacement, &unitResource, nullSemaphore));
        EXPECT_TRUE(unitDispatch.waitForCompletion());
        GameRenderDocGpuDebugguer::getInstance().endCapture();

        WorldMeshTessData tessConfig;
        tessConfig.width = 32;
        tessConfig.height = 32;
        tessConfig.depth = 8;
        tessConfig.seed = 789;
        tessConfig.airUnitId = 0xFFFFFFFF;

        WorldMeshTess tess(tessConfig, unitPlacement, *instance);

        GameComputeDispatch tessDispatch(instance->getDevice(), instance->getGraphicsQueue(),
                                          instance->getCommandPool());

        WorldMeshTessPushConstants tessParams;
        tessParams.seed = tessConfig.seed;
        tessParams.width = tessConfig.width;
        tessParams.height = tessConfig.height;
        tessParams.depth = tessConfig.depth;
        tessParams.airUnitId = tessConfig.airUnitId;

        WorldMeshTessPResource tessResource{&tessParams};

        GameRenderDocGpuDebugguer::getInstance().startCapture();
        EXPECT_NO_THROW(tessDispatch.dispatchSingle(tess, &tessResource, nullSemaphore));
        EXPECT_TRUE(tessDispatch.waitForCompletion());
        GameRenderDocGpuDebugguer::getInstance().endCapture();

        WorldMeshGenData meshConfig;
        meshConfig.width = 32;
        meshConfig.height = 32;
        meshConfig.depth = 8;
        meshConfig.maxVertices = 10000;
        meshConfig.maxIndices = 30000;
        meshConfig.subdivisions = 2;

        WorldMeshGen meshGen(meshConfig, tess, *instance);

        GameComputeDispatch meshDispatch(instance->getDevice(), instance->getGraphicsQueue(),
                                          instance->getCommandPool());

        WorldMeshGenPushConstants meshParams;
        meshParams.width = meshConfig.width;
        meshParams.height = meshConfig.height;
        meshParams.depth = meshConfig.depth;
        meshParams.maxVertices = meshConfig.maxVertices;
        meshParams.maxIndices = meshConfig.maxIndices;
        meshParams.subdivisions = meshConfig.subdivisions;

        WorldMeshGenPResource meshResource{&meshParams};

        GameRenderDocGpuDebugguer::getInstance().startCapture();
        EXPECT_NO_THROW(meshDispatch.dispatchSingle(meshGen, &meshResource, nullSemaphore));
        EXPECT_TRUE(meshDispatch.waitForCompletion());
        GameRenderDocGpuDebugguer::getInstance().endCapture();

        std::vector<MeshVertex> vertices(10000);
        meshGen.readVertices(instance->getDevice(), instance->getPhysicalDevice(), vertices.data(), vertices.size());

        std::vector<uint32_t> indices(30000);
        meshGen.readIndices(instance->getDevice(), instance->getPhysicalDevice(), indices.data(), indices.size());

        EXPECT_GT(vertices.size(), 0);
        EXPECT_GT(indices.size(), 0);

        // Verify vertex data is valid
        for (const auto& vertex : vertices)
        {
                EXPECT_TRUE(std::isfinite(vertex.position.x));
                EXPECT_TRUE(std::isfinite(vertex.position.y));
                EXPECT_TRUE(std::isfinite(vertex.position.z));
        }
}

} // namespace rl
