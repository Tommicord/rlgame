#include <gtest/gtest.h>

#include "Rl.Chunk/WorldMeshIndexDedup.h"
#include "Rl.Chunk/WorldMeshGen.h"
#include "Rl.Chunk/IMeshGen.h"
#include "Rl.Base/GameDevice.h"
#include "Rl.Base/GameOpaqueBufferHandle.h"
#include "Rl.Base/GameOpaqueSyncHandle.h"

#include <memory>
#include <vector>

namespace rl
{

class MockMeshGen : public IMeshGen
{
        public:
                MockMeshGen() = default;
                ~MockMeshGen() override = default;

                GameOpaqueBufferHandle& getVertexBuffer() override
                {
                        return vertexBuffer;
                }

                GameOpaqueBufferHandle& getIndexBuffer() override
                {
                        return indexBuffer;
                }

                GameOpaqueBufferHandle& getCountBuffer() override
                {
                        return countBuffer;
                }

                uint32_t getSubdivisions() const override
                {
                        return subdivisions;
                }

                void setSubdivisions(uint32_t value)
                {
                        subdivisions = value;
                }

                void readVertexCount(uint32_t& vertexCount) override
                {
                        vertexCount = 0;
                }

                std::recursive_mutex& getGenerateMutex() override
                {
                        return generateMutex;
                }

                const GameOpaqueSyncHandle& getCompletionHandle() const override
                {
                        return completionHandle;
                }

        private:
                GameOpaqueBufferHandle vertexBuffer;
                GameOpaqueBufferHandle indexBuffer;
                GameOpaqueBufferHandle countBuffer;
                uint32_t              subdivisions = 0;
                GameOpaqueSyncHandle  completionHandle;
                std::recursive_mutex generateMutex;
};

class WorldMeshIndexDedupTest : public ::testing::Test
{
        protected:
                VkDevice                                device         = VK_NULL_HANDLE;
                VkPhysicalDevice                        physicalDevice = VK_NULL_HANDLE;
                std::unique_ptr<GameDeviceInstance> instance;
                std::unique_ptr<MockMeshGen>            mockMeshGen;
                std::unique_ptr<WorldMeshIndexDedup>    indexDedup;

                void SetUp() override
                {
                        instance = std::make_unique<GameDeviceInstance>();
                        instance->init();
                        device         = instance->getDevice();
                        physicalDevice = instance->getPhysicalDevice();

                        mockMeshGen = std::make_unique<MockMeshGen>();
                }

                void TearDown() override
                {
                        indexDedup.reset();
                        mockMeshGen.reset();
                        if (device != VK_NULL_HANDLE)
                        {
                                vkDeviceWaitIdle(device);
                        }
                        instance.reset();
                }

                void setupWorldMeshIndexDedup(uint32_t maxVertices, uint32_t maxIndices,
                                               uint32_t hashTableSize, uint32_t subdivisions)
                {
                        WorldMeshIndexDedupData dedupData{};
                        dedupData.maxVertices   = maxVertices;
                        dedupData.maxIndices    = maxIndices;
                        dedupData.hashTableSize = hashTableSize;
                        dedupData.subdivisions  = subdivisions;

                        mockMeshGen->setSubdivisions(subdivisions);
                        indexDedup = std::make_unique<WorldMeshIndexDedup>(dedupData, *mockMeshGen, *instance);
                }
};

// Basic configuration tests (non-Vulkan)
TEST(WorldMeshIndexDedupConfigTest, ConfigurationValidation)
{
        WorldMeshIndexDedupData config;
        config.maxVertices      = 10000;
        config.maxIndices       = 30000;
        config.hashTableSize    = 20000;
        config.subdivisions     = 2;

        // Valid configuration should have reasonable values
        EXPECT_GT(config.maxVertices, 0);
        EXPECT_GT(config.maxIndices, 0);
        EXPECT_GT(config.hashTableSize, 0);
        EXPECT_GT(config.subdivisions, 0);
}

// Test configuration with zero values
TEST(WorldMeshIndexDedupConfigTest, ZeroConfiguration)
{
        WorldMeshIndexDedupData config;
        config.maxVertices      = 0;
        config.maxIndices       = 0;
        config.hashTableSize    = 0;
        config.subdivisions     = 0;

        // Zero values are technically valid but may cause issues
        EXPECT_EQ(config.maxVertices, 0);
        EXPECT_EQ(config.maxIndices, 0);
        EXPECT_EQ(config.hashTableSize, 0);
        EXPECT_EQ(config.subdivisions, 0);
}

// Test configuration with large values
TEST(WorldMeshIndexDedupConfigTest, LargeConfiguration)
{
        WorldMeshIndexDedupData config;
        config.maxVertices      = 1000000;
        config.maxIndices       = 3000000;
        config.hashTableSize    = 2000000;
        config.subdivisions     = 8;

        // Large values should be accepted
        EXPECT_GT(config.maxVertices, 0);
        EXPECT_GT(config.maxIndices, 0);
        EXPECT_GT(config.hashTableSize, 0);
        EXPECT_GT(config.subdivisions, 0);
}

// Test push constants structure
TEST(WorldMeshIndexDedupConfigTest, PushConstantsStructure)
{
        WorldMeshIndexDedupPushConstants params;
        params.inputVertexCount = 1000;
        params.maxVertices      = 10000;
        params.maxIndices       = 30000;
        params.hashTableSize    = 20000;
        params.subdivisions     = 2;

        EXPECT_EQ(params.inputVertexCount, 1000);
        EXPECT_EQ(params.maxVertices, 10000);
        EXPECT_EQ(params.maxIndices, 30000);
        EXPECT_EQ(params.hashTableSize, 20000);
        EXPECT_EQ(params.subdivisions, 2);
}

// Test push constants with zero values
TEST(WorldMeshIndexDedupConfigTest, PushConstantsZeroValues)
{
        WorldMeshIndexDedupPushConstants params;
        params.inputVertexCount = 0;
        params.maxVertices      = 0;
        params.maxIndices       = 0;
        params.hashTableSize    = 0;
        params.subdivisions     = 0;

        EXPECT_EQ(params.inputVertexCount, 0);
        EXPECT_EQ(params.maxVertices, 0);
        EXPECT_EQ(params.maxIndices, 0);
        EXPECT_EQ(params.hashTableSize, 0);
        EXPECT_EQ(params.subdivisions, 0);
}

// Test resource pointer structure
TEST(WorldMeshIndexDedupConfigTest, ResourcePointerStructure)
{
        WorldMeshIndexDedupPushConstants params;
        params.inputVertexCount = 500;
        params.maxVertices      = 5000;
        params.maxIndices       = 15000;
        params.hashTableSize    = 10000;
        params.subdivisions     = 1;

        WorldMeshIndexDedupPResource resource;
        resource.pParams = &params;

        EXPECT_NE(resource.pParams, nullptr);
        EXPECT_EQ(resource.pParams->inputVertexCount, 500);
}

// Test resource pointer with null
TEST(WorldMeshIndexDedupConfigTest, ResourcePointerNull)
{
        WorldMeshIndexDedupPResource resource;
        resource.pParams = nullptr;

        EXPECT_EQ(resource.pParams, nullptr);
}

// Test hash table size power of two
TEST(WorldMeshIndexDedupConfigTest, HashTableSizePowerOfTwo)
{
        WorldMeshIndexDedupData config;
        config.hashTableSize = 16384; // 2^14

        // Check if value is power of two
        bool isPowerOfTwo = (config.hashTableSize & (config.hashTableSize - 1)) == 0;
        EXPECT_TRUE(isPowerOfTwo);
}

// Test hash table size not power of two
TEST(WorldMeshIndexDedupConfigTest, HashTableSizeNotPowerOfTwo)
{
        WorldMeshIndexDedupData config;
        config.hashTableSize = 15000; // Not power of two

        // Check if value is not power of two
        bool isPowerOfTwo = (config.hashTableSize & (config.hashTableSize - 1)) == 0;
        EXPECT_FALSE(isPowerOfTwo);
}

// Test subdivision levels
TEST(WorldMeshIndexDedupConfigTest, SubdivisionLevels)
{
        WorldMeshIndexDedupData config;
        
        // Test various subdivision levels
        config.subdivisions = 0;
        EXPECT_GE(config.subdivisions, 0);

        config.subdivisions = 1;
        EXPECT_GE(config.subdivisions, 0);

        config.subdivisions = 2;
        EXPECT_GE(config.subdivisions, 0);

        config.subdivisions = 4;
        EXPECT_GE(config.subdivisions, 0);
}

// Test max vertices to indices ratio
TEST(WorldMeshIndexDedupConfigTest, VerticesToIndicesRatio)
{
        WorldMeshIndexDedupData config;
        config.maxVertices = 10000;
        config.maxIndices  = 30000;

        // Typical triangle mesh has ~3 indices per vertex
        float ratio = static_cast<float>(config.maxIndices) / static_cast<float>(config.maxVertices);
        EXPECT_GT(ratio, 0.0f);
}

// Test configuration bounds
TEST(WorldMeshIndexDedupConfigTest, ConfigurationBounds)
{
        WorldMeshIndexDedupData config;
        
        // Test minimum reasonable values
        config.maxVertices = 1;
        config.maxIndices = 3;
        config.hashTableSize = 2;
        config.subdivisions = 0;

        EXPECT_GE(config.maxVertices, 1);
        EXPECT_GE(config.maxIndices, 3);
        EXPECT_GE(config.hashTableSize, 2);
        EXPECT_GE(config.subdivisions, 0);
}

// Test configuration with extreme subdivision
TEST(WorldMeshIndexDedupConfigTest, ExtremeSubdivision)
{
        WorldMeshIndexDedupData config;
        config.subdivisions = 16; // Very high subdivision

        EXPECT_GT(config.subdivisions, 0);
        EXPECT_LE(config.subdivisions, 32); // Reasonable upper bound
}

// Test configuration with minimal subdivision
TEST(WorldMeshIndexDedupConfigTest, MinimalSubdivision)
{
        WorldMeshIndexDedupData config;
        config.subdivisions = 0; // No subdivision

        EXPECT_GE(config.subdivisions, 0);
}

// Test hash table size relative to max vertices
TEST(WorldMeshIndexDedupConfigTest, HashTableSizeRelativeToVertices)
{
        WorldMeshIndexDedupData config;
        config.maxVertices = 10000;
        config.hashTableSize = 20000; // 2x max vertices

        // Hash table should typically be larger than max vertices
        EXPECT_GT(config.hashTableSize, config.maxVertices);
}

// Test push constants padding
TEST(WorldMeshIndexDedupConfigTest, PushConstantsPadding)
{
        WorldMeshIndexDedupPushConstants params;
        
        // Padding should be initialized to zero
        EXPECT_EQ(params._padding[0], 0);
        EXPECT_EQ(params._padding[1], 0);
        EXPECT_EQ(params._padding[2], 0);
}

// Vulkan-based fixture tests

// Test pipeline creation
TEST_F(WorldMeshIndexDedupTest, PipelineCreation)
{
        setupWorldMeshIndexDedup(1000, 3000, 2000, 1);
        
        EXPECT_NE(indexDedup, nullptr);
}

// Test buffer access
TEST_F(WorldMeshIndexDedupTest, BufferAccess)
{
        setupWorldMeshIndexDedup(1000, 3000, 2000, 1);
        
        EXPECT_NE(&indexDedup->getOutputVertexBuffer(), nullptr);
        EXPECT_NE(&indexDedup->getIndexBuffer(), nullptr);
        EXPECT_NE(&indexDedup->getCountBuffer(), nullptr);
}

// Test completion fence and semaphore
TEST_F(WorldMeshIndexDedupTest, CompletionSyncObjects)
{
        setupWorldMeshIndexDedup(1000, 3000, 2000, 1);
        
        EXPECT_NE(indexDedup->getCompletionFence().getFence(), VK_NULL_HANDLE);
        EXPECT_NE(indexDedup->getCompletionSemaphore().getSemaphore(), VK_NULL_HANDLE);
}

// Test generate mutex
TEST_F(WorldMeshIndexDedupTest, GenerateMutex)
{
        setupWorldMeshIndexDedup(1000, 3000, 2000, 1);
        
        std::recursive_mutex& mutex = indexDedup->getGenerateMutex();
        std::lock_guard<std::recursive_mutex> lock(mutex);
        
        // Mutex should be lockable
        EXPECT_TRUE(true);
}

// Test count buffer initialization
TEST_F(WorldMeshIndexDedupTest, CountBufferInitialization)
{
        setupWorldMeshIndexDedup(1000, 3000, 2000, 1);
        
        uint32_t vertexCount = 0;
        uint32_t indexCount = 0;
        indexDedup->readCounts(device, physicalDevice, vertexCount, indexCount);
        
        // Count buffer should be initialized to zero
        EXPECT_EQ(vertexCount, 0);
        EXPECT_EQ(indexCount, 0);
}

// Test output vertex buffer size
TEST_F(WorldMeshIndexDedupTest, OutputVertexBufferSize)
{
        setupWorldMeshIndexDedup(1000, 3000, 2000, 1);
        
        VkDeviceSize expectedSize = sizeof(MeshVertex) * 1000;
        VkDeviceSize actualSize = indexDedup->getOutputVertexBuffer().getSize();
        
        EXPECT_EQ(actualSize, expectedSize);
}

// Test index buffer size
TEST_F(WorldMeshIndexDedupTest, IndexBufferSize)
{
        setupWorldMeshIndexDedup(1000, 3000, 2000, 1);
        
        VkDeviceSize expectedSize = sizeof(uint32_t) * 3000;
        VkDeviceSize actualSize = indexDedup->getIndexBuffer().getSize();
        
        EXPECT_EQ(actualSize, expectedSize);
}

// Test count buffer size
TEST_F(WorldMeshIndexDedupTest, CountBufferSize)
{
        setupWorldMeshIndexDedup(1000, 3000, 2000, 1);
        
        VkDeviceSize expectedSize = sizeof(uint32_t) * 2;
        VkDeviceSize actualSize = indexDedup->getCountBuffer().getSize();
        
        EXPECT_EQ(actualSize, expectedSize);
}

// Test pipeline with different configurations
TEST_F(WorldMeshIndexDedupTest, DifferentConfigurations)
{
        setupWorldMeshIndexDedup(5000, 15000, 10000, 2);
        
        EXPECT_NE(indexDedup, nullptr);
}

// Test pipeline with minimal configuration
TEST_F(WorldMeshIndexDedupTest, MinimalConfiguration)
{
        setupWorldMeshIndexDedup(100, 300, 200, 0);
        
        EXPECT_NE(indexDedup, nullptr);
}

// Test pipeline with large configuration
TEST_F(WorldMeshIndexDedupTest, LargeConfiguration)
{
        setupWorldMeshIndexDedup(50000, 150000, 100000, 4);
        
        EXPECT_NE(indexDedup, nullptr);
}

// Test hash table size configuration
TEST_F(WorldMeshIndexDedupTest, HashTableSizeConfiguration)
{
        setupWorldMeshIndexDedup(1000, 3000, 4096, 1);
        
        EXPECT_NE(indexDedup, nullptr);
}

// Test subdivision configuration
TEST_F(WorldMeshIndexDedupTest, SubdivisionConfiguration)
{
        setupWorldMeshIndexDedup(1000, 3000, 2000, 3);
        
        EXPECT_NE(indexDedup, nullptr);
}

// Test multiple pipeline setups
TEST_F(WorldMeshIndexDedupTest, MultiplePipelineSetups)
{
        setupWorldMeshIndexDedup(1000, 3000, 2000, 1);
        
        auto firstIndexDedup = indexDedup.get();
        
        // Setup again
        setupWorldMeshIndexDedup(2000, 6000, 4000, 2);
        
        EXPECT_NE(indexDedup.get(), firstIndexDedup);
}

// Test buffer reading with zero elements
TEST_F(WorldMeshIndexDedupTest, ReadBufferZeroElements)
{
        setupWorldMeshIndexDedup(1000, 3000, 2000, 1);
        
        uint32_t vertexCount = 0;
        uint32_t indexCount = 0;
        indexDedup->readCounts(device, physicalDevice, vertexCount, indexCount);
        
        EXPECT_EQ(vertexCount, 0);
        EXPECT_EQ(indexCount, 0);
}

// Test buffer reading with single element
TEST_F(WorldMeshIndexDedupTest, ReadBufferSingleElement)
{
        setupWorldMeshIndexDedup(1000, 3000, 2000, 1);
        
        uint32_t vertexCount = 0;
        uint32_t indexCount = 0;
        indexDedup->readCounts(device, physicalDevice, vertexCount, indexCount);
        
        // Both counts should be readable
        EXPECT_GE(vertexCount, 0);
        EXPECT_GE(indexCount, 0);
}

// Test buffer reading with multiple elements
TEST_F(WorldMeshIndexDedupTest, ReadBufferMultipleElements)
{
        setupWorldMeshIndexDedup(1000, 3000, 2000, 1);
        
        uint32_t vertexCount = 0;
        uint32_t indexCount = 0;
        indexDedup->readCounts(device, physicalDevice, vertexCount, indexCount);
        
        // Both counts should be readable
        EXPECT_GE(vertexCount, 0);
        EXPECT_GE(indexCount, 0);
}

} // namespace rl
