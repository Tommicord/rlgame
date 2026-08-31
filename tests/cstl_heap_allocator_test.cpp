#include <gtest/gtest.h>

#include <cstring>
#include <vector>

extern "C"
{
#include "rlgame.base/cstl/cstl_heap_allocator.h"
}

namespace
{

constexpr size_t kTestHeapSize = 64 * 1024;

class CstlHeapAllocatorTest : public ::testing::Test
{
    protected:
        void
        SetUp () override
        {
            ASSERT_EQ (0, r_cstl_heap_init (kTestHeapSize));
        }

        void
        TearDown () override
        {
            r_cstl_heap_shutdown ();
        }
};

} // namespace

TEST (CstlHeapAllocatorInitTest, InitFailsWithZeroSize)
{
    EXPECT_EQ (-1, r_cstl_heap_init (0));
    EXPECT_EQ (0, r_cstl_heap_GetTotalSize ());
    EXPECT_EQ (0, r_cstl_heap_GetUsedSize ());
}

TEST (CstlHeapAllocatorInitTest, InitDestroyAndReinit)
{
    ASSERT_EQ (0, r_cstl_heap_init (4096));
    EXPECT_GE (r_cstl_heap_GetTotalSize (), 4096u);
    r_cstl_heap_shutdown ();

    ASSERT_EQ (0, r_cstl_heap_init (8192));
    EXPECT_GE (r_cstl_heap_GetTotalSize (), 8192u);
    r_cstl_heap_shutdown ();
}

TEST (CstlHeapAllocatorInitTest, SecondInitIsIdempotent)
{
    ASSERT_EQ (0, r_cstl_heap_init (4096));
    const size_t total = r_cstl_heap_GetTotalSize ();
    EXPECT_EQ (0, r_cstl_heap_init (1024 * 1024));
    EXPECT_EQ (total, r_cstl_heap_GetTotalSize ());
    r_cstl_heap_shutdown ();
}

TEST (CstlHeapAllocatorInitTest, TotalSizeRoundsUpToPowerOfTwo)
{
    ASSERT_EQ (0, r_cstl_heap_init (1000));
    EXPECT_EQ (1024u, r_cstl_heap_GetTotalSize ());
    r_cstl_heap_shutdown ();
}

TEST_F (CstlHeapAllocatorTest, AllocReturnsNullForZeroSize)
{
    EXPECT_EQ (nullptr, r_cstl_heap_alloc (0));
    EXPECT_EQ (0u, r_cstl_heap_GetUsedSize ());
}

TEST (CstlHeapAllocatorAllocTest, AllocFailsWhenHeapNotInitialized)
{
    EXPECT_EQ (nullptr, r_cstl_heap_alloc (16));
    r_cstl_heap_free (nullptr);
    EXPECT_EQ (0, r_cstl_heap_is_valid_pointer (nullptr));
    EXPECT_EQ (0u, r_cstl_heap_get_registered_count ());
}

TEST_F (CstlHeapAllocatorTest, BasicAllocFreeUpdatesUsedSize)
{
    void* p = r_cstl_heap_alloc (64);
    ASSERT_NE (nullptr, p);
    EXPECT_EQ (1, r_cstl_heap_is_valid_pointer (p));
    EXPECT_GT (r_cstl_heap_GetUsedSize (), 0u);

    const size_t used = r_cstl_heap_GetUsedSize ();
    r_cstl_heap_free (p);
    EXPECT_EQ (0, r_cstl_heap_is_valid_pointer (p));
    EXPECT_LT (r_cstl_heap_GetUsedSize (), used);
}

TEST_F (CstlHeapAllocatorTest, ReallocMovesWhenOrderChanges)
{
    void* p = r_cstl_heap_alloc (32);
    ASSERT_NE (nullptr, p);
    memset (p, 0xCD, 32);

    void* q = r_cstl_heap_realloc (p, 4096);
    ASSERT_NE (nullptr, q);
    EXPECT_EQ (1, r_cstl_heap_is_valid_pointer (q));

    const auto* bytes = static_cast<const uint8_t*> (q);
    for (size_t i = 0; i < 32; ++i)
        EXPECT_EQ (0xCDu, bytes[i]);

    r_cstl_heap_free (q);
}

TEST_F (CstlHeapAllocatorTest, MultipleAllocationsAreDistinct)
{
    void* a = r_cstl_heap_alloc (32);
    void* b = r_cstl_heap_alloc (32);
    void* c = r_cstl_heap_alloc (32);
    ASSERT_NE (nullptr, a);
    ASSERT_NE (nullptr, b);
    ASSERT_NE (nullptr, c);
    EXPECT_NE (a, b);
    EXPECT_NE (b, c);
    EXPECT_NE (a, c);

    r_cstl_heap_free (b);
    r_cstl_heap_free (a);
    r_cstl_heap_free (c);
    EXPECT_EQ (0u, r_cstl_heap_GetUsedSize ());
}

TEST_F (CstlHeapAllocatorTest, FreeNullIsSafe)
{
    r_cstl_heap_free (nullptr);
    SUCCEED ();
}

TEST_F (CstlHeapAllocatorTest, DoubleFreeIsRejected)
{
    void* p = r_cstl_heap_alloc (32);
    ASSERT_NE (nullptr, p);
    r_cstl_heap_free (p);
    EXPECT_EQ (0, r_cstl_heap_is_valid_pointer (p));
    r_cstl_heap_free (p);
    SUCCEED ();
}

TEST_F (CstlHeapAllocatorTest, InvalidPointerIsNotValid)
{
    int stack = 0;
    EXPECT_EQ (0, r_cstl_heap_is_valid_pointer (&stack));
    EXPECT_EQ (0, r_cstl_heap_is_valid_pointer (nullptr));
}

TEST_F (CstlHeapAllocatorTest, RegisterAndUnregisterAllocation)
{
    char  owner = 0;
    void* p = r_cstl_heap_alloc (48);
    ASSERT_NE (nullptr, p);

    const uint64_t hash = r_cstl_heap_register_allocation (&owner, p, 48, "node");
    EXPECT_NE (0u, hash);
    EXPECT_EQ (1u, r_cstl_heap_get_registered_count ());

    r_cstl_heap_unregister_allocation (&owner, p);
    EXPECT_EQ (0u, r_cstl_heap_get_registered_count ());
    r_cstl_heap_free (p);
}

TEST_F (CstlHeapAllocatorTest, RegisterAllocationRejectsNullOwnerOrAllocation)
{
    void* p = r_cstl_heap_alloc (16);
    ASSERT_NE (nullptr, p);
    EXPECT_EQ (0u, r_cstl_heap_register_allocation (nullptr, p, 16, "x"));
    EXPECT_EQ (0u, r_cstl_heap_register_allocation (&p, nullptr, 16, "x"));
    EXPECT_EQ (0u, r_cstl_heap_register_allocation (&p, p, 0, "x"));
    r_cstl_heap_free (p);
}

TEST_F (CstlHeapAllocatorTest, RegisterAllocationRejectsNonHeapPointer)
{
    char owner = 0;
    int  stack = 0;
    EXPECT_EQ (0u, r_cstl_heap_register_allocation (&owner, &stack, sizeof (stack), "stack"));
    EXPECT_EQ (0u, r_cstl_heap_get_registered_count ());
}

TEST_F (CstlHeapAllocatorTest, RegisterAllocationRejectsSizeMismatch)
{
    char  owner = 0;
    void* p = r_cstl_heap_alloc (32);
    ASSERT_NE (nullptr, p);
    EXPECT_EQ (0u, r_cstl_heap_register_allocation (&owner, p, 64, "mismatch"));
    EXPECT_EQ (0u, r_cstl_heap_get_registered_count ());
    r_cstl_heap_free (p);
}

TEST_F (CstlHeapAllocatorTest, RegisterAllocationAllowsNullName)
{
    char  owner = 0;
    void* p = r_cstl_heap_alloc (16);
    ASSERT_NE (nullptr, p);
    EXPECT_NE (0u, r_cstl_heap_register_allocation (&owner, p, 16, nullptr));
    EXPECT_EQ (1u, r_cstl_heap_get_registered_count ());
    r_cstl_heap_unregister_allocation (&owner, p);
    r_cstl_heap_free (p);
}

TEST_F (CstlHeapAllocatorTest, UnregisterAllocationIsNoOpWhenMissing)
{
    char  owner = 0;
    void* p = r_cstl_heap_alloc (16);
    ASSERT_NE (nullptr, p);
    r_cstl_heap_unregister_allocation (&owner, p);
    EXPECT_EQ (0u, r_cstl_heap_get_registered_count ());
    r_cstl_heap_free (p);
}

TEST_F (CstlHeapAllocatorTest, UnregisterAllocationIgnoresNullArguments)
{
    char  owner = 0;
    void* p = r_cstl_heap_alloc (16);
    ASSERT_NE (nullptr, p);
    ASSERT_NE (0u, r_cstl_heap_register_allocation (&owner, p, 16, "node"));
    r_cstl_heap_unregister_allocation (nullptr, p);
    r_cstl_heap_unregister_allocation (&owner, nullptr);
    EXPECT_EQ (1u, r_cstl_heap_get_registered_count ());
    r_cstl_heap_unregister_allocation (&owner, p);
    r_cstl_heap_free (p);
}

TEST_F (CstlHeapAllocatorTest, CheckObjectLeaksReportsRegisteredAllocations)
{
    char  owner = 0;
    void* p = r_cstl_heap_alloc (32);
    ASSERT_NE (nullptr, p);
    ASSERT_NE (0u, r_cstl_heap_register_allocation (&owner, p, 32, "leak"));

    EXPECT_EQ (1u, r_cstl_heap_check_object_leaks (&owner));
    EXPECT_EQ (0u, r_cstl_heap_check_object_leaks (nullptr));

    r_cstl_heap_unregister_allocation (&owner, p);
    r_cstl_heap_free (p);
}

TEST_F (CstlHeapAllocatorTest, LogLeaksReturnsRegisteredCount)
{
    char  ownerA = 0;
    char  ownerB = 0;
    void* pA = r_cstl_heap_alloc (16);
    void* pB = r_cstl_heap_alloc (16);
    ASSERT_NE (nullptr, pA);
    ASSERT_NE (nullptr, pB);
    ASSERT_NE (0u, r_cstl_heap_register_allocation (&ownerA, pA, 16, "a"));
    ASSERT_NE (0u, r_cstl_heap_register_allocation (&ownerB, pB, 16, "b"));

    EXPECT_EQ (2u, r_cstl_heap_log_leaks ());

    r_cstl_heap_unregister_allocation (&ownerA, pA);
    r_cstl_heap_unregister_allocation (&ownerB, pB);
    r_cstl_heap_free (pA);
    r_cstl_heap_free (pB);
    EXPECT_EQ (0u, r_cstl_heap_log_leaks ());
}

TEST_F (CstlHeapAllocatorTest, FreeRemovesRegisteredAllocationRecord)
{
    char  owner = 0;
    void* p = r_cstl_heap_alloc (24);
    ASSERT_NE (nullptr, p);
    ASSERT_NE (0u, r_cstl_heap_register_allocation (&owner, p, 24, "auto"));
    EXPECT_EQ (1u, r_cstl_heap_get_registered_count ());

    r_cstl_heap_free (p);
    EXPECT_EQ (0u, r_cstl_heap_get_registered_count ());
}

TEST_F (CstlHeapAllocatorTest, ExhaustHeapReturnsNull)
{
    std::vector<void*> blocks;
    blocks.reserve (256);
    while (void* p = r_cstl_heap_alloc (512))
    {
        blocks.push_back (p);
    }
    EXPECT_EQ (nullptr, r_cstl_heap_alloc (512));
    for (void* p : blocks)
        r_cstl_heap_free (p);
    EXPECT_EQ (0u, r_cstl_heap_GetUsedSize ());
}

TEST_F (CstlHeapAllocatorTest, AllocatedMemoryIsWritable)
{
    void* p = r_cstl_heap_alloc (128);
    ASSERT_NE (nullptr, p);
    std::memset (p, 0xAB, 128);
    EXPECT_EQ (0xAB, static_cast<unsigned char*> (p)[0]);
    EXPECT_EQ (0xAB, static_cast<unsigned char*> (p)[127]);
    r_cstl_heap_free (p);
}

#if R_CSTL_HEAP_DEBUG_ENABLED
TEST_F (CstlHeapAllocatorTest, DebugVerifyReportsHealthyHeap)
{
    void* p = r_cstl_heap_alloc (32);
    ASSERT_NE (nullptr, p);
    EXPECT_EQ (0, r_cstl_heap_debug_verify ());
    r_cstl_heap_free (p);
    EXPECT_EQ (0, r_cstl_heap_debug_verify ());
}

TEST_F (CstlHeapAllocatorTest, DebugAllocFillsPoisonPattern)
{
    void* p = r_cstl_heap_alloc (16);
    ASSERT_NE (nullptr, p);
    const auto* bytes = static_cast<const unsigned char*> (p);
    for (size_t i = 0; i < 16; ++i)
        EXPECT_EQ (0xCDu, bytes[i]) << "byte " << i;
    r_cstl_heap_free (p);
}

TEST_F (CstlHeapAllocatorTest, DebugFreeFillsPoisonPattern)
{
    void* p = r_cstl_heap_alloc (16);
    ASSERT_NE (nullptr, p);
    r_cstl_heap_free (p);
    const auto* bytes = static_cast<const unsigned char*> (p);
    for (size_t i = 0; i < 16; ++i)
        EXPECT_EQ (0xDDu, bytes[i]) << "byte " << i;
}
#endif
