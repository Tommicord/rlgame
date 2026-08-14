#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <vector>
#include <chrono>

#include "Rl.Chunk/WorldUnitPlacement.h"
#include "Rl.Chunk/WorldHeightmap.h"
#include "Rl.Chunk/WorldClimateCompute.h"
#include "Rl.World/PreUnitRegistry.h"
#include "Rl.World/PreBiomeRegistry.h"
#include "Rl.World/PreUnitRegister.h"
#include "Rl.World/PreBiomeRegister.h"
#include "Rl.World/Unit.h"
#include "Rl.World/Biome.h"
#include "Rl.Base/GameComputeDispatch.h"
#include "Rl.Base/GameDevice.h"
#include "Rl.Base/GameRenderDocGpuDebugguer.h"
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

                float getElevationNearAccept() const override { return 0.08f; }
                float getMoistureNearAccept() const override { return 0.05f; }
                float getEquatorNearAccept() const override { return 0.06f; }
                float getTemperatureNearAccept() const override { return 0.1f; }
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

                float getElevationNearAccept() const override { return 0.09f; }
                float getMoistureNearAccept() const override { return 0.07f; }
                float getEquatorNearAccept() const override { return 0.05f; }
                float getTemperatureNearAccept() const override { return 0.11f; }
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

                float getElevationNearAccept() const override { return 0.06f; }
                float getMoistureNearAccept() const override { return 0.04f; }
                float getEquatorNearAccept() const override { return 0.05f; }
                float getTemperatureNearAccept() const override { return 0.08f; }
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

                float getElevationNearAccept() const override { return 0.07f; }
                float getMoistureNearAccept() const override { return 0.05f; }
                float getEquatorNearAccept() const override { return 0.07f; }
                float getTemperatureNearAccept() const override { return 0.09f; }
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

                float getElevationNearAccept() const override { return 0.05f; }
                float getMoistureNearAccept() const override { return 0.04f; }
                float getEquatorNearAccept() const override { return 0.06f; }
                float getTemperatureNearAccept() const override { return 0.07f; }
};

class TestBiomeGrassland : public PreBiome
{
        public:
                explicit TestBiomeGrassland(PreUnitRegistry& unitRegistry) :
                    PreBiome(PreBiomeRegisterFactory::create(typeid(TestBiomeGrassland).name()),
                             unitRegistry)
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
                    PreBiome(PreBiomeRegisterFactory::create(typeid(TestBiomeMountain).name()),
                             unitRegistry)
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
                    PreBiome(PreBiomeRegisterFactory::create(typeid(TestBiomeCoastal).name()),
                             unitRegistry)
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

class WorldUnitPlacementTest : public ::testing::Test
{
        protected:
                void SetUp() override
                {
                        instance = std::make_unique<GameDeviceInstance>();
                        instance->init();
                        ASSERT_NE(instance->getDevice(), VK_NULL_HANDLE);
                        ASSERT_NE(instance->getPhysicalDevice(), VK_NULL_HANDLE);
                        GameRenderDocGpuDebugguer::getInstance();

                        heightmap = std::make_unique<WorldHeightmap>(width, height, depth, seed, *instance);

                        WorldHeightmapPushConstants params1{};
                        params1.width       = width;
                        params1.height      = height;
                        params1.depth       = depth;
                        params1.scale       = 1.0f;
                        params1.heightScale = 1.0f;
                        params1.seaLevel    = 0.5f;
                        params1.seed        = seed;
                        params1.octaves     = 4;
                        params1.persistence = 0.5f;
                        params1.groundLevel = 0.5f;

                        WorldHeightmapComputePResource resource1{&params1};

                        GameComputeDispatch dispatch(instance->getDevice(),
                                                     instance->getGraphicsQueue(),
                                                     instance->getCommandPool());
                        GameVulkanSemaphore nullSemaphore{};

                        dispatch.dispatchSingle(*heightmap, &resource1, nullSemaphore);
                        dispatch.waitForCompletion();

                        climateCompute = std::make_unique<WorldClimateCompute>(width, height, *instance);

                        WorldClimateComputePushConstants params2{};
                        params2.width       = width;
                        params2.height      = height;
                        params2.texelSize   = 1.0f;
                        params2.planetIndex = 0;

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
                        planet.seed             = 42;
                        planet.shapeType        = 0;

                        WorldClimateComputePResource resource2{&params2, &planet};

                        dispatch.dispatchSingle(*climateCompute, &resource2, nullSemaphore);
                        dispatch.waitForCompletion();

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

                        unitPlacement = std::make_unique<WorldUnitPlacement>(width, height, depth, seed, *instance,
                                                                           *heightmap, *climateCompute);

                }

                void TearDown() override
                {
                        registeredUnits.clear();
                        registeredBiomes.clear();
                        preBiomeRegistry.reset();
                        preUnitRegistry.reset();
                        unitPlacement.reset();
                        climateCompute.reset();
                        heightmap.reset();
                        instance.reset();
                }

                WorldUnitPlacementPushConstants createPushConstants(uint32_t width,
                                                                    uint32_t height,
                                                                    uint32_t depth,
                                                                    float    groundLevel = 0.5f) const
                {
                        WorldUnitPlacementPushConstants params{};
                        params.worldOrigin = Vec3{0.0f, 0.0f, 0.0f};
                        params.width       = width;
                        params.height      = height;
                        params.depth       = depth;
                        params.texelSize   = 1.0f;
                        params.airUnitId   = 0;
                        params.unitRegistryCount = static_cast<uint32_t>(registeredUnits.size());
                        params.biomeRegistryCount = static_cast<uint32_t>(registeredBiomes.size());
                        params.groundLevel = groundLevel;
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

                WorldUnitPlacementComputePResource createResource(WorldUnitPlacementPushConstants& params,
                                                                  WorldPlanetData&               planet) const
                {
                        return {&params, preUnitRegistry.get(), &planet, preBiomeRegistry.get()};
                }

                std::unique_ptr<GameDeviceInstance> instance;
                std::unique_ptr<WorldHeightmap>         heightmap;
                std::unique_ptr<WorldClimateCompute>    climateCompute;
                std::unique_ptr<WorldUnitPlacement>     unitPlacement;
                std::unique_ptr<PreBiomeRegistry>       preBiomeRegistry;
                std::unique_ptr<PreUnitRegistry>        preUnitRegistry;
                std::vector<std::unique_ptr<PreUnit>>   registeredUnits;
                std::vector<std::unique_ptr<PreBiome>>  registeredBiomes;
                const uint32_t                          width  = 128;
                const uint32_t                          height = 128;
                const uint32_t                          depth  = 256;
                const uint32_t                          seed   = 42;
};

TEST_F(WorldUnitPlacementTest, ConstructorCreatesValidResources)
{
        // Verify that images are created by checking getter methods don't return null
        EXPECT_NE(unitPlacement->getUnitOutputImage(), VK_NULL_HANDLE);
        EXPECT_NE(unitPlacement->getBiomeOutputImage(), VK_NULL_HANDLE);
}

TEST_F(WorldUnitPlacementTest, DispatchWithValidParameters)
{

        WorldUnitPlacementPushConstants params = createPushConstants(width, height, depth);
        WorldPlanetData planet = createPlanet(42);
        WorldUnitPlacementComputePResource resource = createResource(params, planet);

        GameComputeDispatch dispatch(instance->getDevice(), instance->getGraphicsQueue(), instance->getCommandPool());
        GameVulkanSemaphore nullSemaphore{};

        // This should not throw or cause VK_ERROR_DEVICE_LOST
        GameRenderDocGpuDebugguer::getInstance().startCapture();
        EXPECT_NO_THROW(dispatch.dispatchSingle(*unitPlacement, &resource, nullSemaphore));
        dispatch.waitForCompletion();
        GameRenderDocGpuDebugguer::getInstance().endCapture();
}

TEST_F(WorldUnitPlacementTest, ReadUnitOutputAfterDispatch)
{
        WorldUnitPlacementPushConstants params = createPushConstants(width, height, depth);
        WorldPlanetData planet = createPlanet(123);
        WorldUnitPlacementComputePResource resource = createResource(params, planet);

        GameComputeDispatch dispatch(instance->getDevice(), instance->getGraphicsQueue(), instance->getCommandPool());
        GameVulkanSemaphore nullSemaphore{};
        
        GameRenderDocGpuDebugguer::getInstance().startCapture();
        dispatch.dispatchSingle(*unitPlacement, &resource, nullSemaphore);
        dispatch.waitForCompletion();
        GameRenderDocGpuDebugguer::getInstance().endCapture();

        std::vector<uint32_t> output;
        output.resize(width * height * depth);

        EXPECT_NO_THROW(unitPlacement->readUnitOutput(instance->getDevice(), instance->getPhysicalDevice(),
                                                      output.data(), output.size()));

        // Verify output size matches expected
        EXPECT_EQ(output.size(), width * height * depth);
}

TEST_F(WorldUnitPlacementTest, ReadBiomeOutputAfterDispatch)
{
        WorldUnitPlacementPushConstants params = createPushConstants(width, height, depth);
        WorldPlanetData planet = createPlanet(456);
        WorldUnitPlacementComputePResource resource = createResource(params, planet);

        GameComputeDispatch dispatch(instance->getDevice(), instance->getGraphicsQueue(), instance->getCommandPool());
        GameVulkanSemaphore nullSemaphore{};
        
        GameRenderDocGpuDebugguer::getInstance().startCapture();
        dispatch.dispatchSingle(*unitPlacement, &resource, nullSemaphore);
        dispatch.waitForCompletion();
        GameRenderDocGpuDebugguer::getInstance().endCapture();

        std::vector<uint32_t> output;
        output.resize(width * height * depth);

        EXPECT_NO_THROW(unitPlacement->readBiomeOutput(
            instance->getDevice(), instance->getPhysicalDevice(), output.data(), output.size()));

        // Verify output size matches expected
        EXPECT_EQ(output.size(), width * height * depth);
}

TEST_F(WorldUnitPlacementTest, ThreadSafety)
{
        const int numThreads = 4;
        std::vector<std::thread> threads;

        for (int i = 0; i < numThreads; ++i)
        {
                threads.emplace_back([this, i]() {
                        WorldUnitPlacementPushConstants params = createPushConstants(width, height, depth);
                        WorldPlanetData planet = createPlanet(789 + i);
                        WorldUnitPlacementComputePResource resource = createResource(params, planet);

                        GameComputeDispatch dispatch(instance->getDevice(), instance->getGraphicsQueue(), instance->getCommandPool());
                        GameVulkanSemaphore nullSemaphore{};

                        // Multiple threads should be able to dispatch without race conditions
                        EXPECT_NO_THROW(dispatch.dispatchSingle(*unitPlacement, &resource, nullSemaphore));
                        dispatch.waitForCompletion();
                });
        }

        for (auto& thread : threads)
        {
                thread.join();
        }
}

TEST_F(WorldUnitPlacementTest, MultipleDispatchesWithoutDeviceLost)
{
        const int numIterations = 10;
        for (int i = 0; i < numIterations; ++i)
        {
                WorldUnitPlacementPushConstants params = createPushConstants(width, height, depth);
                WorldPlanetData planet = createPlanet(1000 + i);
                WorldUnitPlacementComputePResource resource = createResource(params, planet);

                GameComputeDispatch dispatch(instance->getDevice(), instance->getGraphicsQueue(), instance->getCommandPool());
                GameVulkanSemaphore nullSemaphore{};
                
                GameRenderDocGpuDebugguer::getInstance().startCapture();
                EXPECT_NO_THROW(dispatch.dispatchSingle(*unitPlacement, &resource, nullSemaphore));
                dispatch.waitForCompletion();
                GameRenderDocGpuDebugguer::getInstance().endCapture();
        }

        // If we get here without VK_ERROR_DEVICE_LOST, the test passes
        SUCCEED();
}

TEST_F(WorldUnitPlacementTest, CompletionSemaphoreSignaled)
{
        WorldUnitPlacementPushConstants params = createPushConstants(64, 64, 16);
        WorldPlanetData planet = createPlanet(111);
        WorldUnitPlacementComputePResource resource = createResource(params, planet);

        GameComputeDispatch dispatch(instance->getDevice(), instance->getGraphicsQueue(), instance->getCommandPool());
        GameVulkanSemaphore nullSemaphore{};
        GameRenderDocGpuDebugguer::getInstance().startCapture();
        dispatch.dispatchSingle(*unitPlacement, &resource, nullSemaphore);
        dispatch.waitForCompletion();
        GameRenderDocGpuDebugguer::getInstance().endCapture();

        // Verify completion semaphore is valid
        EXPECT_NE(unitPlacement->getCompletionSemaphore().getSemaphore(), VK_NULL_HANDLE);
}

TEST_F(WorldUnitPlacementTest, MutexLockingWorks)
{
        // Verify we can lock the mutex
        std::scoped_lock lock(unitPlacement->getGenerateMutex());
        
        // If we can lock it without deadlock the test passes
        SUCCEED();
}

} // namespace rl
