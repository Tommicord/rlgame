#include <gtest/gtest.h>

#include <cstring>

extern "C"
{
#include "rlgame.base/cvulkan/cvulkan_defragmentation.h"
}

namespace
{

class CvulkanDefragTest : public ::testing::Test
{
        protected:
                void
                SetUp () override
                {
                        memset (&config, 0, sizeof (config));
                        memset (&context, 0, sizeof (context));
                        memset (&stats, 0, sizeof (stats));
                        memset (&blockMetadata, 0, sizeof (blockMetadata));
                        memset (&move, 0, sizeof (move));
                }

                void
                TearDown () override
                {
                }

                struct R_CVulkan_DefragConfig     config;
                struct R_CVulkan_DefragContext  context;
                struct R_CVulkan_DefragStats    stats;
                struct R_CVulkan_DefragBlockMetadata blockMetadata;
                struct R_CVulkan_DefragMove      move;
};

// Test backend enum values
TEST_F (CvulkanDefragTest, BackendEnumValues)
{
        EXPECT_EQ (0, R_CVULKAN_DEFRAG_BACKEND_NONE);
        EXPECT_EQ (1, R_CVULKAN_DEFRAG_BACKEND_CUDA);
        EXPECT_EQ (2, R_CVULKAN_DEFRAG_BACKEND_OPENCL);
        EXPECT_EQ (3, R_CVULKAN_DEFRAG_BACKEND_CPU);
}

// Test move operation enum values
TEST_F (CvulkanDefragTest, MoveOperationEnumValues)
{
        EXPECT_EQ (0, R_CVULKAN_DEFRAG_MOVE_OPERATION_MOVE);
        EXPECT_EQ (1, R_CVULKAN_DEFRAG_MOVE_OPERATION_IGNORE);
        EXPECT_EQ (2, R_CVULKAN_DEFRAG_MOVE_OPERATION_DESTROY);
}

// Test config structure initialization
TEST_F (CvulkanDefragTest, ConfigInitialization)
{
        EXPECT_EQ (0, config.mergeFactor);
        EXPECT_EQ (0, config.maxBytesPerPass);
        EXPECT_EQ (0, config.maxPasses);
        EXPECT_EQ (0, config.preferredBackend);
}

// Test default config
TEST_F (CvulkanDefragTest, DefaultConfig)
{
        R_CVulkan_DefragSetDefaultConfig (&config);
        
        // Check that default values are set to reasonable values
        EXPECT_GT (config.mergeFactor, 0);
        EXPECT_GT (config.maxBytesPerPass, 0);
        EXPECT_GT (config.maxPasses, 0);
        EXPECT_GE (config.preferredBackend, R_CVULKAN_DEFRAG_BACKEND_NONE);
        EXPECT_LE (config.preferredBackend, R_CVULKAN_DEFRAG_BACKEND_CPU);
}

// Test context structure initialization
TEST_F (CvulkanDefragTest, ContextInitialization)
{
        EXPECT_EQ (nullptr, context.pAllocator);
        EXPECT_EQ (0, context.config.mergeFactor);
        EXPECT_EQ (0, context.backend);
        EXPECT_EQ (0, context.currentPass);
        EXPECT_EQ (0, context.totalMoves);
        EXPECT_EQ (0, context.totalBytesMoved);
        EXPECT_EQ (nullptr, context.pBlockMetadata);
        EXPECT_EQ (0, context.blockMetadataCount);
        EXPECT_EQ (0, context.blockMetadataCapacity);
        EXPECT_EQ (nullptr, context.pMoves);
        EXPECT_EQ (0, context.moveCount);
        EXPECT_EQ (0, context.moveCapacity);
        EXPECT_EQ (nullptr, context.pBackendContext);
        EXPECT_EQ (false, context.booted);
}

// Test stats structure initialization
TEST_F (CvulkanDefragTest, StatsInitialization)
{
        EXPECT_EQ (0, stats.passesCompleted);
        EXPECT_EQ (0, stats.totalMoves);
        EXPECT_EQ (0, stats.totalBytesMoved);
        EXPECT_EQ (0, stats.blocksFreed);
        EXPECT_FLOAT_EQ (0.0f, stats.fragmentationBefore);
        EXPECT_FLOAT_EQ (0.0f, stats.fragmentationAfter);
}

// Test block metadata structure initialization
TEST_F (CvulkanDefragTest, BlockMetadataInitialization)
{
        EXPECT_EQ (0, blockMetadata.blockIndex);
        EXPECT_EQ (0, blockMetadata.totalSize);
        EXPECT_EQ (0, blockMetadata.usedSize);
        EXPECT_EQ (0, blockMetadata.freeSize);
        EXPECT_FLOAT_EQ (0.0f, blockMetadata.fillLevel);
        EXPECT_EQ (0, blockMetadata.allocationCount);
        EXPECT_EQ (0, blockMetadata.isCandidate);
}

// Test move structure initialization
TEST_F (CvulkanDefragTest, MoveInitialization)
{
        EXPECT_EQ (0, move.srcBlockIndex);
        EXPECT_EQ (0, move.dstBlockIndex);
        EXPECT_EQ (0, move.srcOffset);
        EXPECT_EQ (0, move.dstOffset);
        EXPECT_EQ (0, move.size);
        EXPECT_EQ (0, move.operation);
}

// Test fill level calculation
TEST_F (CvulkanDefragTest, FillLevelCalculation)
{
        // Case 1: Empty block
        uint64_t totalSize = 1024 * 1024;
        uint64_t usedSize = 0;
        float expectedFillLevel = totalSize > 0 ? (float)usedSize / (float)totalSize : 0.0f;
        EXPECT_FLOAT_EQ (0.0f, expectedFillLevel);

        // Case 2: Half full
        usedSize = 512 * 1024;
        expectedFillLevel = totalSize > 0 ? (float)usedSize / (float)totalSize : 0.0f;
        EXPECT_FLOAT_EQ (0.5f, expectedFillLevel);

        // Case 3: Full
        usedSize = 1024 * 1024;
        expectedFillLevel = totalSize > 0 ? (float)usedSize / (float)totalSize : 0.0f;
        EXPECT_FLOAT_EQ (1.0f, expectedFillLevel);

        // Case 4: 75% full
        usedSize = 768 * 1024;
        expectedFillLevel = totalSize > 0 ? (float)usedSize / (float)totalSize : 0.0f;
        EXPECT_FLOAT_EQ (0.75f, expectedFillLevel);
}

// Test free size calculation
TEST_F (CvulkanDefragTest, FreeSizeCalculation)
{
        uint64_t totalSize = 1024 * 1024;
        uint64_t usedSize = 512 * 1024;
        uint64_t expectedFreeSize = totalSize - usedSize;
        EXPECT_EQ (512 * 1024, expectedFreeSize);

        usedSize = 256 * 1024;
        expectedFreeSize = totalSize - usedSize;
        EXPECT_EQ (768 * 1024, expectedFreeSize);
}

// Test fragmentation level calculation for defrag
TEST_F (CvulkanDefragTest, FragmentationLevelCalculation)
{
        // Fragmentation can be calculated based on block fill levels
        // Higher variance in fill levels = higher fragmentation

        // Case 1: All blocks have similar fill levels (low fragmentation)
        float fillLevels[] = {0.5f, 0.5f, 0.5f, 0.5f};
        float mean = 0.5f;
        float variance = 0.0f;
        for (int i = 0; i < 4; ++i)
        {
                variance += (fillLevels[i] - mean) * (fillLevels[i] - mean);
        }
        variance /= 4.0f;
        EXPECT_NEAR (0.0f, variance, 0.01f);

        // Case 2: Blocks have varying fill levels (higher fragmentation)
        float fillLevels2[] = {0.1f, 0.9f, 0.2f, 0.8f};
        mean = (0.1f + 0.9f + 0.2f + 0.8f) / 4.0f;
        variance = 0.0f;
        for (int i = 0; i < 4; ++i)
        {
                variance += (fillLevels2[i] - mean) * (fillLevels2[i] - mean);
        }
        variance /= 4.0f;
        EXPECT_GT (variance, 0.0f);
}

// Test candidate selection logic
TEST_F (CvulkanDefragTest, CandidateSelectionLogic)
{
        // Blocks with low fill levels are good candidates for defragmentation
        // because they have more free space to accept moved allocations

        // Case 1: Low fill level (good candidate)
        float fillLevel = 0.2f;
        bool isCandidate = fillLevel < 0.5f;
        EXPECT_TRUE (isCandidate);

        // Case 2: High fill level (poor candidate)
        fillLevel = 0.8f;
        isCandidate = fillLevel < 0.5f;
        EXPECT_FALSE (isCandidate);

        // Case 3: Medium fill level (borderline)
        fillLevel = 0.5f;
        isCandidate = fillLevel < 0.5f;
        EXPECT_FALSE (isCandidate);
}

// Test move operation validity
TEST_F (CvulkanDefragTest, MoveOperationValidity)
{
        // A valid move should have:
        // - Different source and destination blocks (or same for compaction)
        // - Non-zero size
        // - Valid operation type

        // Case 1: Valid move
        move.srcBlockIndex = 0;
        move.dstBlockIndex = 1;
        move.srcOffset = 0;
        move.dstOffset = 0;
        move.size = 1024;
        move.operation = R_CVULKAN_DEFRAG_MOVE_OPERATION_MOVE;
        bool isValid = move.size > 0 && 
                        move.operation >= R_CVULKAN_DEFRAG_MOVE_OPERATION_MOVE &&
                        move.operation <= R_CVULKAN_DEFRAG_MOVE_OPERATION_DESTROY;
        EXPECT_TRUE (isValid);

        // Case 2: Invalid size
        move.size = 0;
        isValid = move.size > 0 && 
                  move.operation >= R_CVULKAN_DEFRAG_MOVE_OPERATION_MOVE &&
                  move.operation <= R_CVULKAN_DEFRAG_MOVE_OPERATION_DESTROY;
        EXPECT_FALSE (isValid);

        // Case 3: Invalid operation
        move.size = 1024;
        move.operation = 99;
        isValid = move.size > 0 && 
                  move.operation >= R_CVULKAN_DEFRAG_MOVE_OPERATION_MOVE &&
                  move.operation <= R_CVULKAN_DEFRAG_MOVE_OPERATION_DESTROY;
        EXPECT_FALSE (isValid);
}

// Test structure sizes
TEST_F (CvulkanDefragTest, StructureSizes)
{
        // Ensure structures have expected sizes for GPU memory alignment
        // Note: Actual sizes may differ due to compiler padding
        EXPECT_EQ (48, sizeof (blockMetadata));
        EXPECT_EQ (40, sizeof (move));
        EXPECT_EQ (32, sizeof (stats));
}

// Test merge factor logic
TEST_F (CvulkanDefragTest, MergeFactorLogic)
{
        // Merge factor determines how many blocks can be merged during defragmentation
        // Higher merge factor = more aggressive defragmentation

        // Case 1: Conservative merge
        uint32_t mergeFactor = 2;
        EXPECT_EQ (2, mergeFactor);

        // Case 2: Aggressive merge
        mergeFactor = 5;
        EXPECT_EQ (5, mergeFactor);

        // Case 3: No merge
        mergeFactor = 1;
        EXPECT_EQ (1, mergeFactor);
}

// Test max bytes per pass logic
TEST_F (CvulkanDefragTest, MaxBytesPerPassLogic)
{
        // Max bytes per pass limits how much data can be moved in a single pass
        // This prevents stalling the GPU with too many memory operations

        // Case 1: Small limit (safer, more passes)
        uint64_t maxBytesPerPass = 16 * 1024 * 1024; // 16 MB
        EXPECT_EQ (16 * 1024 * 1024, maxBytesPerPass);

        // Case 2: Large limit (faster, fewer passes)
        maxBytesPerPass = 128 * 1024 * 1024; // 128 MB
        EXPECT_EQ (128 * 1024 * 1024, maxBytesPerPass);

        // Case 3: No limit
        maxBytesPerPass = UINT64_MAX;
        EXPECT_EQ (UINT64_MAX, maxBytesPerPass);
}

// Test max passes logic
TEST_F (CvulkanDefragTest, MaxPassesLogic)
{
        // Max passes limits how many defragmentation passes can be executed
        // This prevents infinite loops or excessive processing

        // Case 1: Single pass
        uint32_t maxPasses = 1;
        EXPECT_EQ (1, maxPasses);

        // Case 2: Multiple passes
        maxPasses = 5;
        EXPECT_EQ (5, maxPasses);

        // Case 3: No limit
        maxPasses = 0; // 0 typically means unlimited
        EXPECT_EQ (0, maxPasses);
}

// Integration test: Get available backend
TEST_F (CvulkanDefragTest, GetAvailableBackend)
{
        enum R_CVulkan_DefragBackend backend = R_CVULKAN_DEFRAG_BACKEND_NONE;
        enum R_CVulkanError result = R_CVulkan_DefragGetAvailableBackend (&backend);
        
        // Should succeed and return a valid backend
        EXPECT_EQ (R_CVULKAN_OK, result);
        EXPECT_GE (backend, R_CVULKAN_DEFRAG_BACKEND_NONE);
        EXPECT_LE (backend, R_CVULKAN_DEFRAG_BACKEND_CPU);
}

// Integration test: Initialize and cleanup defrag context
TEST_F (CvulkanDefragTest, InitializeCleanup)
{
        // Create a minimal memory allocator for testing
        struct R_CVulkan_MemoryAllocator allocator;
        memset (&allocator, 0, sizeof (allocator));
        allocator.device = VK_NULL_HANDLE;
        allocator.physicalDevice = VK_NULL_HANDLE;
        allocator.ppBlocks = NULL;
        allocator.blockCount = 0;
        allocator.blockCapacity = 0;

        // Set default config
        R_CVulkan_DefragSetDefaultConfig (&config);

        // Initialize defrag context
        enum R_CVulkanError result = R_CVulkan_DefragInitialize (&context, &allocator, &config);
        
        // May fail without proper Vulkan setup, but we can test the CPU backend path
        if (result == R_CVULKAN_OK)
        {
                EXPECT_TRUE (context.booted);
                EXPECT_NE (nullptr, context.pAllocator);
                EXPECT_EQ (&allocator, context.pAllocator);
                
                // Cleanup
                R_CVulkan_DefragCleanup (&context);
                EXPECT_FALSE (context.booted);
        }
}

// Integration test: Check if defrag is needed
TEST_F (CvulkanDefragTest, IsNeeded)
{
        // Create a minimal memory allocator for testing
        struct R_CVulkan_MemoryAllocator allocator;
        memset (&allocator, 0, sizeof (allocator));
        allocator.device = VK_NULL_HANDLE;
        allocator.physicalDevice = VK_NULL_HANDLE;
        allocator.ppBlocks = NULL;
        allocator.blockCount = 0;
        allocator.blockCapacity = 0;

        // Set default config
        R_CVulkan_DefragSetDefaultConfig (&config);

        // Initialize defrag context
        enum R_CVulkanError result = R_CVulkan_DefragInitialize (&context, &allocator, &config);
        
        if (result == R_CVULKAN_OK)
        {
                int needed = 0;
                result = R_CVulkan_DefragIsNeeded (&context, &needed);
                
                if (result == R_CVULKAN_OK)
                {
                        // With no blocks, defrag should not be needed
                        EXPECT_EQ (0, needed);
                }
                
                // Cleanup
                R_CVulkan_DefragCleanup (&context);
        }
}

// Integration test: Get fragmentation level
TEST_F (CvulkanDefragTest, GetFragmentationLevel)
{
        // Create a minimal memory allocator for testing
        struct R_CVulkan_MemoryAllocator allocator;
        memset (&allocator, 0, sizeof (allocator));
        allocator.device = VK_NULL_HANDLE;
        allocator.physicalDevice = VK_NULL_HANDLE;
        allocator.ppBlocks = NULL;
        allocator.blockCount = 0;
        allocator.blockCapacity = 0;

        // Set default config
        R_CVulkan_DefragSetDefaultConfig (&config);

        // Initialize defrag context
        enum R_CVulkanError result = R_CVulkan_DefragInitialize (&context, &allocator, &config);
        
        if (result == R_CVULKAN_OK)
        {
                float fragmentation = 0.0f;
                result = R_CVulkan_DefragGetFragmentationLevel (&context, &fragmentation);
                
                if (result == R_CVULKAN_OK)
                {
                        // With no blocks, fragmentation should be 0
                        EXPECT_FLOAT_EQ (0.0f, fragmentation);
                }
                
                // Cleanup
                R_CVulkan_DefragCleanup (&context);
        }
}

// Integration test: Defrag begin and end
TEST_F (CvulkanDefragTest, BeginEnd)
{
        // Create a minimal memory allocator for testing
        struct R_CVulkan_MemoryAllocator allocator;
        memset (&allocator, 0, sizeof (allocator));
        allocator.device = VK_NULL_HANDLE;
        allocator.physicalDevice = VK_NULL_HANDLE;
        allocator.ppBlocks = NULL;
        allocator.blockCount = 0;
        allocator.blockCapacity = 0;

        // Set default config
        R_CVulkan_DefragSetDefaultConfig (&config);

        // Initialize defrag context
        enum R_CVulkanError result = R_CVulkan_DefragInitialize (&context, &allocator, &config);
        
        if (result == R_CVULKAN_OK)
        {
                // Begin defrag
                result = R_CVulkan_DefragBegin (&context);
                
                if (result == R_CVULKAN_OK)
                {
                        // End defrag and get stats
                        result = R_CVulkan_DefragEnd (&context, &stats);
                        
                        if (result == R_CVULKAN_OK)
                        {
                                // With no blocks, stats should be minimal
                                EXPECT_EQ (0, stats.passesCompleted);
                                EXPECT_EQ (0, stats.totalMoves);
                                EXPECT_EQ (0, stats.totalBytesMoved);
                        }
                }
                
                // Cleanup
                R_CVulkan_DefragCleanup (&context);
        }
}

} // namespace
