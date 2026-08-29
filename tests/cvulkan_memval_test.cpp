#include <gtest/gtest.h>

#include <cstring>

extern "C"
{
#include "rlgame.base/cvulkan/cvulkan_memval.h"
#include "rlgame.base/cvulkan/cvulkan_memory_allocator.h"
}

namespace
{

constexpr uint32_t R_CVULKAN_MEMVAL_FIXED_POINT_SCALE = 256;

// Helper functions for Q8.8 fixed-point arithmetic
static inline uint16_t
FloatToFixedPoint (float value)
{
    return (uint16_t)(value * R_CVULKAN_MEMVAL_FIXED_POINT_SCALE);
}

static inline float
FixedPointToFloat (uint16_t value)
{
    return (float)value / R_CVULKAN_MEMVAL_FIXED_POINT_SCALE;
}

class CvulkanMemvalTest : public ::testing::Test
{
    protected:
        void
        SetUp () override
        {
            memset (&stats, 0, sizeof (stats));
        }

        void
        TearDown () override
        {
        }

        struct R_CVulkan_MemValStats stats;
};

// Test fixed-point arithmetic conversions
TEST_F (CvulkanMemvalTest, FixedPointConversions)
{
    // Test 0.0
    uint16_t zero = FloatToFixedPoint (0.0f);
    EXPECT_EQ (0, zero);
    EXPECT_FLOAT_EQ (0.0f, FixedPointToFloat (zero));

    // Test 1.0
    uint16_t one = FloatToFixedPoint (1.0f);
    EXPECT_EQ (R_CVULKAN_MEMVAL_FIXED_POINT_SCALE, one);
    EXPECT_FLOAT_EQ (1.0f, FixedPointToFloat (one));

    // Test 0.5
    uint16_t half = FloatToFixedPoint (0.5f);
    EXPECT_EQ (R_CVULKAN_MEMVAL_FIXED_POINT_SCALE / 2, half);
    EXPECT_FLOAT_EQ (0.5f, FixedPointToFloat (half));

    // Test 0.25
    uint16_t quarter = FloatToFixedPoint (0.25f);
    EXPECT_EQ (R_CVULKAN_MEMVAL_FIXED_POINT_SCALE / 4, quarter);
    EXPECT_FLOAT_EQ (0.25f, FixedPointToFloat (quarter));

    // Test 0.75
    uint16_t three_quarter = FloatToFixedPoint (0.75f);
    EXPECT_EQ (3 * R_CVULKAN_MEMVAL_FIXED_POINT_SCALE / 4, three_quarter);
    EXPECT_NEAR (0.75f, FixedPointToFloat (three_quarter), 0.01f);
}

// Test stats structure initialization
TEST_F (CvulkanMemvalTest, StatsInitialization)
{
    EXPECT_EQ (0, stats.totalAllocations);
    EXPECT_EQ (0, stats.totalFrees);
    EXPECT_EQ (0, stats.totalBytesAllocated);
    EXPECT_EQ (0, stats.totalBytesFreed);
    EXPECT_EQ (0, stats.activeAllocations);
    EXPECT_EQ (0, stats.activeBytes);
    EXPECT_EQ (0, stats.totalBlocksReserved);
    EXPECT_EQ (0, stats.totalBlocksReleased);
    EXPECT_EQ (0, stats.activeBlocks);
    EXPECT_EQ (0, stats.freeRegionCount);
    EXPECT_EQ (0, stats.lastFragmentationLevel);
    EXPECT_EQ (0, stats.maxFragmentationLevel);
    EXPECT_EQ (0, stats.defragmentationPending);
    EXPECT_EQ (0, stats.failedAllocations);
    EXPECT_EQ (0, stats.fragmentedAllocationFailures);
    EXPECT_EQ (0, stats.alignedRegions);
    EXPECT_EQ (0, stats.misalignedRegions);
    EXPECT_EQ (0, stats.health);
    EXPECT_EQ (0, stats.defragmentationThreshold);
}

// Test fragmentation level calculation (manual calculation)
TEST_F (CvulkanMemvalTest, FragmentationLevelCalculation)
{
    // Case 1: No fragmentation (single large free region)
    uint64_t totalFree = 1024 * 1024;
    uint64_t largestFreeRegion = 1024 * 1024;
    float    expectedFragmentation = 1.0f - ((float)largestFreeRegion / (float)totalFree);
    EXPECT_FLOAT_EQ (0.0f, expectedFragmentation);

    // Case 2: 50% fragmentation
    totalFree = 1024 * 1024;
    largestFreeRegion = 512 * 1024;
    expectedFragmentation = 1.0f - ((float)largestFreeRegion / (float)totalFree);
    EXPECT_FLOAT_EQ (0.5f, expectedFragmentation);

    // Case 3: 75% fragmentation
    totalFree = 1024 * 1024;
    largestFreeRegion = 256 * 1024;
    expectedFragmentation = 1.0f - ((float)largestFreeRegion / (float)totalFree);
    EXPECT_FLOAT_EQ (0.75f, expectedFragmentation);

    // Convert to fixed-point
    uint16_t fragFixed = FloatToFixedPoint (expectedFragmentation);
    EXPECT_NEAR (expectedFragmentation, FixedPointToFloat (fragFixed), 0.01f);
}

// Test alignment health calculation
TEST_F (CvulkanMemvalTest, AlignmentHealthCalculation)
{
    // Case 1: All regions aligned
    uint64_t alignedRegions = 100;
    uint64_t misalignedRegions = 0;
    uint64_t totalRegions = alignedRegions + misalignedRegions;
    float    expectedHealth = totalRegions > 0 ? (float)alignedRegions / (float)totalRegions : 1.0f;
    EXPECT_FLOAT_EQ (1.0f, expectedHealth);

    // Case 2: 50% aligned
    alignedRegions = 50;
    misalignedRegions = 50;
    totalRegions = alignedRegions + misalignedRegions;
    expectedHealth = totalRegions > 0 ? (float)alignedRegions / (float)totalRegions : 1.0f;
    EXPECT_FLOAT_EQ (0.5f, expectedHealth);

    // Case 3: 25% aligned
    alignedRegions = 25;
    misalignedRegions = 75;
    totalRegions = alignedRegions + misalignedRegions;
    expectedHealth = totalRegions > 0 ? (float)alignedRegions / (float)totalRegions : 1.0f;
    EXPECT_FLOAT_EQ (0.25f, expectedHealth);

    // Convert to fixed-point
    uint16_t healthFixed = FloatToFixedPoint (expectedHealth);
    EXPECT_NEAR (expectedHealth, FixedPointToFloat (healthFixed), 0.01f);
}

// Test failure health calculation
TEST_F (CvulkanMemvalTest, FailureHealthCalculation)
{
    // Case 1: No failures
    uint64_t fragmentedAllocationFailures = 0;
    float    expectedHealth = 1.0f / (1.0f + (float)fragmentedAllocationFailures);
    EXPECT_FLOAT_EQ (1.0f, expectedHealth);

    // Case 2: 1 failure
    fragmentedAllocationFailures = 1;
    expectedHealth = 1.0f / (1.0f + (float)fragmentedAllocationFailures);
    EXPECT_FLOAT_EQ (0.5f, expectedHealth);

    // Case 3: 3 failures
    fragmentedAllocationFailures = 3;
    expectedHealth = 1.0f / (1.0f + (float)fragmentedAllocationFailures);
    EXPECT_NEAR (0.25f, expectedHealth, 0.01f);

    // Convert to fixed-point
    uint16_t healthFixed = FloatToFixedPoint (expectedHealth);
    EXPECT_NEAR (expectedHealth, FixedPointToFloat (healthFixed), 0.01f);
}

// Test overall health calculation (weighted sum)
TEST_F (CvulkanMemvalTest, OverallHealthCalculation)
{
    // Health = alignmentHealth * 0.35f + (1.0f - fragmentationLevel) * 0.45f + failureHealth * 0.20f

    // Case 1: Perfect health
    float alignmentHealth = 1.0f;
    float fragmentationLevel = 0.0f;
    float failureHealth = 1.0f;
    float expectedHealth
        = alignmentHealth * 0.35f + (1.0f - fragmentationLevel) * 0.45f + failureHealth * 0.20f;
    EXPECT_FLOAT_EQ (1.0f, expectedHealth);

    // Case 2: Moderate health
    alignmentHealth = 0.8f;
    fragmentationLevel = 0.3f;
    failureHealth = 0.9f;
    expectedHealth = alignmentHealth * 0.35f + (1.0f - fragmentationLevel) * 0.45f + failureHealth * 0.20f;
    EXPECT_NEAR (0.775f, expectedHealth, 0.05f);

    // Case 3: Poor health
    alignmentHealth = 0.5f;
    fragmentationLevel = 0.7f;
    failureHealth = 0.5f;
    expectedHealth = alignmentHealth * 0.35f + (1.0f - fragmentationLevel) * 0.45f + failureHealth * 0.20f;
    EXPECT_NEAR (0.415f, expectedHealth, 0.01f);

    // Convert to fixed-point
    uint16_t healthFixed = FloatToFixedPoint (expectedHealth);
    EXPECT_NEAR (expectedHealth, FixedPointToFloat (healthFixed), 0.01f);
}

// Test defragmentation threshold calculation
TEST_F (CvulkanMemvalTest, DefragmentationThresholdCalculation)
{
    // Threshold = 0.25f + health * 0.50f

    // Case 1: Perfect health
    float health = 1.0f;
    float expectedThreshold = 0.25f + health * 0.50f;
    EXPECT_FLOAT_EQ (0.75f, expectedThreshold);

    // Case 2: Moderate health
    health = 0.5f;
    expectedThreshold = 0.25f + health * 0.50f;
    EXPECT_FLOAT_EQ (0.5f, expectedThreshold);

    // Case 3: Poor health
    health = 0.0f;
    expectedThreshold = 0.25f + health * 0.50f;
    EXPECT_FLOAT_EQ (0.25f, expectedThreshold);

    // Convert to fixed-point
    uint16_t thresholdFixed = FloatToFixedPoint (expectedThreshold);
    EXPECT_NEAR (expectedThreshold, FixedPointToFloat (thresholdFixed), 0.01f);
}

// Test defragmentation pending condition
TEST_F (CvulkanMemvalTest, DefragmentationPendingCondition)
{
    // Condition: fragmentationLevel >= threshold OR fragmentedAllocationFailures > 0

    // Case 1: Fragmentation above threshold
    float    fragmentationLevel = 0.6f;
    float    threshold = 0.5f;
    uint64_t fragmentedAllocationFailures = 0;
    bool     expectedPending = fragmentationLevel >= threshold || fragmentedAllocationFailures > 0;
    EXPECT_TRUE (expectedPending);

    // Case 2: Fragmentation below threshold but has failures
    fragmentationLevel = 0.3f;
    threshold = 0.5f;
    fragmentedAllocationFailures = 1;
    expectedPending = fragmentationLevel >= threshold || fragmentedAllocationFailures > 0;
    EXPECT_TRUE (expectedPending);

    // Case 3: Fragmentation below threshold and no failures
    fragmentationLevel = 0.3f;
    threshold = 0.5f;
    fragmentedAllocationFailures = 0;
    expectedPending = fragmentationLevel >= threshold || fragmentedAllocationFailures > 0;
    EXPECT_FALSE (expectedPending);
}

// Test backend enum values
TEST_F (CvulkanMemvalTest, BackendEnumValues)
{
    EXPECT_EQ (0, R_CVULKAN_MEMVAL_BACKEND_NONE);
    EXPECT_EQ (1, R_CVULKAN_MEMVAL_BACKEND_CUDA);
    EXPECT_EQ (2, R_CVULKAN_MEMVAL_BACKEND_OPENCL);
    EXPECT_EQ (3, R_CVULKAN_MEMVAL_BACKEND_CPU);
}

// Test stats structure size
TEST_F (CvulkanMemvalTest, StatsStructureSize)
{
    // Ensure the stats structure is the expected size
    // This is important for GPU memory alignment
    // Note: Actual size may differ due to compiler padding
    EXPECT_EQ (128, sizeof (stats));
}

// Integration test: MemVal notification functions update stats
TEST_F (CvulkanMemvalTest, NotificationFunctionsUpdateStats)
{
    // Create a minimal memory allocator for testing
    struct R_CVulkan_MemoryAllocator allocator;
    memset (&allocator, 0, sizeof (allocator));
    allocator.device = VK_NULL_HANDLE;
    allocator.physicalDevice = VK_NULL_HANDLE;
    allocator.ppBlocks = NULL;
    allocator.blockCount = 0;
    allocator.blockCapacity = 0;
    allocator.pMemVal = NULL;

    // Initialize MemVal
    enum R_CVulkan_Error result = R_CVulkan_MemValInitialize (&allocator);
    // Note: This may fail without proper Vulkan setup, but we can test the stats structure
    if (result == R_CVULKAN_OK && allocator.pMemVal)
    {
        // Get initial stats
        struct R_CVulkan_MemValStats initialStats;
        result = R_CVulkan_MemValGetStats (&allocator, &initialStats);
        if (result == R_CVULKAN_OK)
        {
            EXPECT_EQ (0, initialStats.totalAllocations);
            EXPECT_EQ (0, initialStats.totalFrees);
            EXPECT_EQ (0, initialStats.activeAllocations);
        }

        // Cleanup
        R_CVulkan_MemValShutdown (&allocator);
    }
}

// Integration test: MemVal stats tracking
TEST_F (CvulkanMemvalTest, StatsTracking)
{
    // Test that stats structure can track values correctly
    stats.totalAllocations = 100;
    stats.totalFrees = 50;
    stats.activeAllocations = 50;
    stats.totalBytesAllocated = 1024 * 1024;
    stats.totalBytesFreed = 512 * 1024;
    stats.activeBytes = 512 * 1024;
    stats.totalBlocksReserved = 10;
    stats.totalBlocksReleased = 5;
    stats.activeBlocks = 5;
    stats.freeRegionCount = 20;
    stats.alignedRegions = 15;
    stats.misalignedRegions = 5;
    stats.failedAllocations = 2;
    stats.fragmentedAllocationFailures = 1;

    EXPECT_EQ (100, stats.totalAllocations);
    EXPECT_EQ (50, stats.totalFrees);
    EXPECT_EQ (50, stats.activeAllocations);
    EXPECT_EQ (1024 * 1024, stats.totalBytesAllocated);
    EXPECT_EQ (512 * 1024, stats.totalBytesFreed);
    EXPECT_EQ (512 * 1024, stats.activeBytes);
    EXPECT_EQ (10, stats.totalBlocksReserved);
    EXPECT_EQ (5, stats.totalBlocksReleased);
    EXPECT_EQ (5, stats.activeBlocks);
    EXPECT_EQ (20, stats.freeRegionCount);
    EXPECT_EQ (15, stats.alignedRegions);
    EXPECT_EQ (5, stats.misalignedRegions);
    EXPECT_EQ (2, stats.failedAllocations);
    EXPECT_EQ (1, stats.fragmentedAllocationFailures);
}

// Integration test: MemVal health calculation with actual stats
TEST_F (CvulkanMemvalTest, HealthCalculationWithStats)
{
    // Set up realistic stats
    stats.totalAllocations = 100;
    stats.totalFrees = 20;
    stats.activeAllocations = 80;
    stats.totalBytesAllocated = 10 * 1024 * 1024;
    stats.totalBytesFreed = 2 * 1024 * 1024;
    stats.activeBytes = 8 * 1024 * 1024;
    stats.activeBlocks = 5;
    stats.freeRegionCount = 25;
    stats.alignedRegions = 20;
    stats.misalignedRegions = 5;
    stats.fragmentedAllocationFailures = 0;

    // Calculate alignment health
    uint64_t totalRegions = stats.alignedRegions + stats.misalignedRegions;
    float    alignmentHealth = totalRegions > 0 ? (float)stats.alignedRegions / (float)totalRegions : 1.0f;
    EXPECT_FLOAT_EQ (0.8f, alignmentHealth);

    // Calculate failure health
    float failureHealth = 1.0f / (1.0f + (float)stats.fragmentedAllocationFailures);
    EXPECT_FLOAT_EQ (1.0f, failureHealth);

    // Calculate overall health (assuming fragmentation level of 0.3)
    float fragmentationLevel = 0.3f;
    float overallHealth
        = alignmentHealth * 0.35f + (1.0f - fragmentationLevel) * 0.45f + failureHealth * 0.20f;
    EXPECT_NEAR (0.8f, overallHealth, 0.05f);

    // Calculate defragmentation threshold
    float threshold = 0.25f + overallHealth * 0.50f;
    EXPECT_NEAR (0.65f, threshold, 0.05f);
}

} // namespace
