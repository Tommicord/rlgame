#include <gtest/gtest.h>

#include <cstring>
#include <vector>

extern "C"
{
#include "rlgame.base/cstl/cstl_heap_allocator.h"
}

namespace
{

constexpr size_t kTestHeapSize = 256 * 1024;

class CstlHeapTest : public ::testing::Test
{
  protected:
    void
    SetUp () override
    {
        ASSERT_EQ (0, R_CSTL_HeapInit (kTestHeapSize));
    }

    void
    TearDown () override
    {
        R_CSTL_HeapShutdown ();
    }
};

TEST_F (CstlHeapTest, InitAndShutdown)
{
        // Already tested in SetUp/TearDown
        EXPECT_EQ (0, R_CSTL_HeapLogLeaks ());
}


TEST_F (CstlHeapTest, AllocAndFree)
{
        void* p = R_CSTL_HeapAlloc (100);
        ASSERT_NE (nullptr, p);
        R_CSTL_HeapFree (p);
}

TEST_F (CstlHeapTest, AllocZeroReturnsNull)
{
        void* p = R_CSTL_HeapAlloc (0);
        EXPECT_EQ (nullptr, p);
}

TEST_F (CstlHeapTest, AllocLargeFails)
{
        void* p = R_CSTL_HeapAlloc (kTestHeapSize * 2);
        EXPECT_EQ (nullptr, p);
}

TEST_F (CstlHeapTest, FreeNullIsNoOp)
{
        R_CSTL_HeapFree (nullptr);
}

TEST_F (CstlHeapTest, ReallocNullBehavesLikeAlloc)
{
        void* p = R_CSTL_HeapRealloc (nullptr, 100);
        ASSERT_NE (nullptr, p);
        R_CSTL_HeapFree (p);
}

TEST_F (CstlHeapTest, ReallocZeroBehavesLikeFree)
{
        void* p = R_CSTL_HeapAlloc (100);
        ASSERT_NE (nullptr, p);
        void* result = R_CSTL_HeapRealloc (p, 0);
        EXPECT_EQ (nullptr, result);
}

TEST_F (CstlHeapTest, ReallocGrow)
{
        void* p = R_CSTL_HeapAlloc (100);
        ASSERT_NE (nullptr, p);
        memset (p, 0xAA, 100);

        void* p2 = R_CSTL_HeapRealloc (p, 200);
        ASSERT_NE (nullptr, p2);
        // First 100 bytes should still be 0xAA
        for (size_t i = 0; i < 100; ++i)
        {
                EXPECT_EQ (0xAA, ((uint8_t*)p2)[i]);
        }
        R_CSTL_HeapFree (p2);
}

TEST_F (CstlHeapTest, ReallocShrink)
{
        void* p = R_CSTL_HeapAlloc (200);
        ASSERT_NE (nullptr, p);

        void* p2 = R_CSTL_HeapRealloc (p, 100);
        ASSERT_NE (nullptr, p2);
        R_CSTL_HeapFree (p2);
}

TEST_F (CstlHeapTest, ReallocSameSize)
{
        void* p = R_CSTL_HeapAlloc (100);
        ASSERT_NE (nullptr, p);

        void* p2 = R_CSTL_HeapRealloc (p, 100);
        ASSERT_NE (nullptr, p2);
        R_CSTL_HeapFree (p2);
}

TEST_F (CstlHeapTest, ReallocFailLeavesOriginal)
{
        void* p = R_CSTL_HeapAlloc (100);
        ASSERT_NE (nullptr, p);
        memset (p, 0xCC, 100);

        // Try to realloc to impossibly large size
        void* p2 = R_CSTL_HeapRealloc (p, kTestHeapSize * 2);
        EXPECT_EQ (nullptr, p2);

        // Original should still be valid
        for (size_t i = 0; i < 100; ++i)
        {
                EXPECT_EQ (0xCC, ((uint8_t*)p)[i]);
        }
        R_CSTL_HeapFree (p);
}

TEST_F (CstlHeapTest, AllocAligned)
{
        void* p = R_CSTL_HeapAllocAligned (100, 16);
        ASSERT_NE (nullptr, p);
        EXPECT_EQ (0, (uintptr_t)p % 16);
        R_CSTL_HeapFree (p);
}

TEST_F (CstlHeapTest, AllocAligned32)
{
        void* p = R_CSTL_HeapAllocAligned (100, 32);
        ASSERT_NE (nullptr, p);
        EXPECT_EQ (0, (uintptr_t)p % 32);
        R_CSTL_HeapFree (p);
}

TEST_F (CstlHeapTest, AllocAligned64)
{
        void* p = R_CSTL_HeapAllocAligned (100, 64);
        ASSERT_NE (nullptr, p);
        EXPECT_EQ (0, (uintptr_t)p % 64);
        R_CSTL_HeapFree (p);
}

TEST_F (CstlHeapTest, AllocAlignedZeroReturnsNull)
{
        void* p = R_CSTL_HeapAllocAligned (0, 16);
        EXPECT_EQ (nullptr, p);
}

TEST_F (CstlHeapTest, IsValidPointer)
{
        void* p = R_CSTL_HeapAlloc (100);
        ASSERT_NE (nullptr, p);

        EXPECT_EQ (1, R_CSTL_HeapIsValidPointer (p));
        EXPECT_EQ (0, R_CSTL_HeapIsValidPointer (nullptr));
        EXPECT_EQ (0, R_CSTL_HeapIsValidPointer ((void*)0x1000));

        R_CSTL_HeapFree (p);
        EXPECT_EQ (0, R_CSTL_HeapIsValidPointer (p));
}

TEST_F (CstlHeapTest, RegisterAllocation)
{
        void* pOwner   = (void*)0x1000;
        void* pAlloc   = R_CSTL_HeapAlloc (100);
        ASSERT_NE (nullptr, pAlloc);

        uint64_t hash = R_CSTL_HeapRegisterAllocation (pOwner, pAlloc, 100, "TestAlloc");
        EXPECT_NE (0, hash);

        R_CSTL_HeapUnregisterAllocation (pOwner, pAlloc);
        R_CSTL_HeapFree (pAlloc);
}

TEST_F (CstlHeapTest, RegisterAllocationNullOwner)
{
        void* pAlloc = R_CSTL_HeapAlloc (100);
        ASSERT_NE (nullptr, pAlloc);

        uint64_t hash = R_CSTL_HeapRegisterAllocation (nullptr, pAlloc, 100, "TestAlloc");
        EXPECT_EQ (0, hash);

        R_CSTL_HeapFree (pAlloc);
}

TEST_F (CstlHeapTest, UnregisterAllocationNoOpIfNotFound)
{
        void* pOwner = (void*)0x1000;
        R_CSTL_HeapUnregisterAllocation (pOwner, (void*)0x2000);
}

TEST_F (CstlHeapTest, CheckObjectLeaks)
{
        void* pOwner = (void*)0x1000;
        void* pAlloc = R_CSTL_HeapAlloc (100);
        ASSERT_NE (nullptr, pAlloc);

        R_CSTL_HeapRegisterAllocation (pOwner, pAlloc, 100, "TestAlloc");

        size_t leaks = R_CSTL_HeapCheckObjectLeaks (pOwner);
        EXPECT_EQ (1, leaks);

        R_CSTL_HeapUnregisterAllocation (pOwner, pAlloc);
        R_CSTL_HeapFree (pAlloc);
}

TEST_F (CstlHeapTest, LogLeaks)
{
        void* pOwner = (void*)0x1000;
        void* pAlloc = R_CSTL_HeapAlloc (100);
        ASSERT_NE (nullptr, pAlloc);

        R_CSTL_HeapRegisterAllocation (pOwner, pAlloc, 100, "TestAlloc");

        size_t leaks = R_CSTL_HeapLogLeaks ();
        EXPECT_EQ (1, leaks);

        R_CSTL_HeapUnregisterAllocation (pOwner, pAlloc);
        R_CSTL_HeapFree (pAlloc);
}

TEST_F (CstlHeapTest, MultipleAllocations)
{
        std::vector<void*> allocations;
        for (int i = 0; i < 100; ++i)
        {
                void* p = R_CSTL_HeapAlloc (100);
                ASSERT_NE (nullptr, p);
                allocations.push_back (p);
        }

        for (void* p : allocations)
        {
                R_CSTL_HeapFree (p);
        }
}

TEST_F (CstlHeapTest, SmallAllocations)
{
        for (size_t size = 1; size <= 64; ++size)
        {
                void* p = R_CSTL_HeapAlloc (size);
                ASSERT_NE (nullptr, p) << "Failed to allocate " << size << " bytes";
                R_CSTL_HeapFree (p);
        }
}

TEST_F (CstlHeapTest, PowerOfTwoAllocations)
{
        for (size_t size = 32; size <= 4096; size *= 2)
        {
                void* p = R_CSTL_HeapAlloc (size);
                ASSERT_NE (nullptr, p) << "Failed to allocate " << size << " bytes";
                R_CSTL_HeapFree (p);
        }
}

// Stress tests
TEST_F (CstlHeapTest, StressManySmallAllocations)
{
        constexpr int kNumAllocs = 1000;
        std::vector<void*> allocations;
        allocations.reserve (kNumAllocs);

        for (int i = 0; i < kNumAllocs; ++i)
        {
                void* p = R_CSTL_HeapAlloc (64);
                ASSERT_NE (nullptr, p) << "Failed allocation " << i;
                allocations.push_back (p);
        }

        for (void* p : allocations)
        {
                R_CSTL_HeapFree (p);
        }
}

TEST_F (CstlHeapTest, StressRandomSizedAllocations)
{
        constexpr int kNumAllocs = 200;
        std::vector<void*> allocations;
        allocations.reserve (kNumAllocs);

        for (int i = 0; i < kNumAllocs; ++i)
        {
                size_t size = (rand () % 256) + 16;
                void*   p    = R_CSTL_HeapAlloc (size);
                ASSERT_NE (nullptr, p) << "Failed allocation " << i << " of size " << size;
                allocations.push_back (p);
        }

        for (void* p : allocations)
        {
                R_CSTL_HeapFree (p);
        }
}

TEST_F (CstlHeapTest, StressAllocFreePattern)
{
        constexpr int kIterations = 100;
        std::vector<void*> allocations;

        for (int iter = 0; iter < kIterations; ++iter)
        {
                // Allocate 10 blocks
                for (int i = 0; i < 10; ++i)
                {
                        void* p = R_CSTL_HeapAlloc ((rand () % 256) + 32);
                        ASSERT_NE (nullptr, p);
                        allocations.push_back (p);
                }

                // Free half of them
                for (int i = 0; i < 5; ++i)
                {
                        if (!allocations.empty ())
                        {
                                size_t idx = rand () % allocations.size ();
                                R_CSTL_HeapFree (allocations[idx]);
                                allocations.erase (allocations.begin () + idx);
                        }
                }
        }

        // Free remaining
        for (void* p : allocations)
        {
                R_CSTL_HeapFree (p);
        }
}

TEST_F (CstlHeapTest, StressReallocPattern)
{
        constexpr int kIterations = 200;
        std::vector<void*> allocations;

        for (int iter = 0; iter < kIterations; ++iter)
        {
                void* p = R_CSTL_HeapAlloc (100);
                ASSERT_NE (nullptr, p);
                allocations.push_back (p);

                // Grow and shrink a few times
                for (int i = 0; i < 5; ++i)
                {
                        size_t newSize = (rand () % 500) + 50;
                        p = R_CSTL_HeapRealloc (p, newSize);
                        ASSERT_NE (nullptr, p);
                        allocations.back () = p;
                }
        }

        for (void* p : allocations)
        {
                R_CSTL_HeapFree (p);
        }
}

TEST_F (CstlHeapTest, StressFragmentation)
{
        // Pattern designed to cause fragmentation
        constexpr int kNumBlocks = 100;
        std::vector<void*> smallAllocs;
        std::vector<void*> largeAllocs;

        // Allocate many small blocks
        for (int i = 0; i < kNumBlocks; ++i)
        {
                void* p = R_CSTL_HeapAlloc (32);
                ASSERT_NE (nullptr, p);
                smallAllocs.push_back (p);
        }

        // Free every other small block to create holes
        for (size_t i = 1; i < smallAllocs.size (); i += 2)
        {
                R_CSTL_HeapFree (smallAllocs[i]);
                smallAllocs[i] = nullptr;
        }

        // Try to allocate larger blocks in the holes
        for (int i = 0; i < kNumBlocks / 4; ++i)
        {
                void* p = R_CSTL_HeapAlloc (64);
                if (p)
                {
                        largeAllocs.push_back (p);
                }
        }

        // Free all remaining allocations
        for (void* p : smallAllocs)
        {
                if (p)
                        R_CSTL_HeapFree (p);
        }
        for (void* p : largeAllocs)
        {
                R_CSTL_HeapFree (p);
        }
}

TEST_F (CstlHeapTest, StressAlignment)
{
        constexpr int kNumAllocs = 200;
        for (int i = 0; i < kNumAllocs; ++i)
        {
                size_t size      = (rand () % 256) + 16;
                size_t alignment = 16 << (rand () % 4); // 16, 32, 64, 128
                void*  p         = R_CSTL_HeapAllocAligned (size, alignment);
                ASSERT_NE (nullptr, p);
                EXPECT_EQ (0, (uintptr_t)p % alignment);
                R_CSTL_HeapFree (p);
        }
}

TEST_F (CstlHeapTest, StressLeakTracking)
{
        constexpr int kNumOwners = 3;
        constexpr int kNumAllocsPerOwner = 10;
        std::vector<void*> owners;
        std::vector<std::vector<void*>> allocations;

        for (int i = 0; i < kNumOwners; ++i)
        {
                void* owner = (void*)(uintptr_t)(0x1000 + i * 0x100);
                owners.push_back (owner);
                allocations.emplace_back ();

                for (int j = 0; j < kNumAllocsPerOwner; ++j)
                {
                        void* p = R_CSTL_HeapAlloc (32);
                        ASSERT_NE (nullptr, p);
                        uint64_t hash = R_CSTL_HeapRegisterAllocation (owner, p, 32, "StressAlloc");
                        if (hash != 0)
                        {
                                allocations.back ().push_back (p);
                        }
                        else
                        {
                                R_CSTL_HeapFree (p);
                        }
                }
        }

        for (size_t i = 0; i < owners.size (); ++i)
        {
                for (void* p : allocations[i])
                {
                        R_CSTL_HeapUnregisterAllocation (owners[i], p);
                        R_CSTL_HeapFree (p);
                }
        }
}

} // namespace
