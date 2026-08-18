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
    { ASSERT_EQ (0, R_CSTL_HeapInit (kTestHeapSize)); }

    void
    TearDown () override
    { R_CSTL_HeapShutdown (); }
};

} // namespace

TEST (CstlHeapAllocatorInitTest, InitFailsWithZeroSize)
{
  EXPECT_EQ (-1, R_CSTL_HeapInit (0));
  EXPECT_EQ (0, R_CSTL_Heap_GetTotalSize ());
  EXPECT_EQ (0, R_CSTL_Heap_GetUsedSize ());
}

TEST (CstlHeapAllocatorInitTest, InitDestroyAndReinit)
{
  ASSERT_EQ (0, R_CSTL_HeapInit (4096));
  EXPECT_GE (R_CSTL_Heap_GetTotalSize (), 4096u);
  R_CSTL_HeapShutdown ();

  ASSERT_EQ (0, R_CSTL_HeapInit (8192));
  EXPECT_GE (R_CSTL_Heap_GetTotalSize (), 8192u);
  R_CSTL_HeapShutdown ();
}

TEST (CstlHeapAllocatorInitTest, SecondInitIsIdempotent)
{
  ASSERT_EQ (0, R_CSTL_HeapInit (4096));
  const size_t total = R_CSTL_Heap_GetTotalSize ();
  EXPECT_EQ (0, R_CSTL_HeapInit (1024 * 1024));
  EXPECT_EQ (total, R_CSTL_Heap_GetTotalSize ());
  R_CSTL_HeapShutdown ();
}

TEST (CstlHeapAllocatorInitTest, TotalSizeRoundsUpToPowerOfTwo)
{
  ASSERT_EQ (0, R_CSTL_HeapInit (1000));
  EXPECT_EQ (1024u, R_CSTL_Heap_GetTotalSize ());
  R_CSTL_HeapShutdown ();
}

TEST_F (CstlHeapAllocatorTest, AllocReturnsNullForZeroSize)
{
  EXPECT_EQ (nullptr, R_CSTL_HeapAlloc (0));
  EXPECT_EQ (0u, R_CSTL_Heap_GetUsedSize ());
}

TEST (CstlHeapAllocatorAllocTest, AllocFailsWhenHeapNotInitialized)
{
  EXPECT_EQ (nullptr, R_CSTL_HeapAlloc (16));
  R_CSTL_HeapFree (nullptr);
  EXPECT_EQ (0, R_CSTL_HeapIsValidPointer (nullptr));
  EXPECT_EQ (0u, R_CSTL_HeapGetRegisteredCount ());
}

TEST_F (CstlHeapAllocatorTest, BasicAllocFreeUpdatesUsedSize)
{
  void* p = R_CSTL_HeapAlloc (64);
  ASSERT_NE (nullptr, p);
  EXPECT_EQ (1, R_CSTL_HeapIsValidPointer (p));
  EXPECT_GT (R_CSTL_Heap_GetUsedSize (), 0u);

  const size_t used = R_CSTL_Heap_GetUsedSize ();
  R_CSTL_HeapFree (p);
  EXPECT_EQ (0, R_CSTL_HeapIsValidPointer (p));
  EXPECT_LT (R_CSTL_Heap_GetUsedSize (), used);
}

TEST_F (CstlHeapAllocatorTest, ReallocMovesWhenOrderChanges)
{
  void* p = R_CSTL_HeapAlloc (32);
  ASSERT_NE (nullptr, p);
  memset (p, 0xCD, 32);

  void* q = R_CSTL_HeapRealloc (p, 4096);
  ASSERT_NE (nullptr, q);
  EXPECT_EQ (1, R_CSTL_HeapIsValidPointer (q));

  const auto* bytes = static_cast<const uint8_t*> (q);
  for (size_t i = 0; i < 32; ++i)
    EXPECT_EQ (0xCDu, bytes[i]);

  R_CSTL_HeapFree (q);
}

TEST_F (CstlHeapAllocatorTest, MultipleAllocationsAreDistinct)
{
  void* a = R_CSTL_HeapAlloc (32);
  void* b = R_CSTL_HeapAlloc (32);
  void* c = R_CSTL_HeapAlloc (32);
  ASSERT_NE (nullptr, a);
  ASSERT_NE (nullptr, b);
  ASSERT_NE (nullptr, c);
  EXPECT_NE (a, b);
  EXPECT_NE (b, c);
  EXPECT_NE (a, c);

  R_CSTL_HeapFree (b);
  R_CSTL_HeapFree (a);
  R_CSTL_HeapFree (c);
  EXPECT_EQ (0u, R_CSTL_Heap_GetUsedSize ());
}

TEST_F (CstlHeapAllocatorTest, FreeNullIsSafe)
{
  R_CSTL_HeapFree (nullptr);
  SUCCEED ();
}

TEST_F (CstlHeapAllocatorTest, DoubleFreeIsRejected)
{
  void* p = R_CSTL_HeapAlloc (32);
  ASSERT_NE (nullptr, p);
  R_CSTL_HeapFree (p);
  EXPECT_EQ (0, R_CSTL_HeapIsValidPointer (p));
  R_CSTL_HeapFree (p);
  SUCCEED ();
}

TEST_F (CstlHeapAllocatorTest, InvalidPointerIsNotValid)
{
  int stack = 0;
  EXPECT_EQ (0, R_CSTL_HeapIsValidPointer (&stack));
  EXPECT_EQ (0, R_CSTL_HeapIsValidPointer (nullptr));
}

TEST_F (CstlHeapAllocatorTest, RegisterAndUnregisterAllocation)
{
  char  owner = 0;
  void* p     = R_CSTL_HeapAlloc (48);
  ASSERT_NE (nullptr, p);

  const uint64_t hash = R_CSTL_HeapRegisterAllocation (&owner, p, 48, "node");
  EXPECT_NE (0u, hash);
  EXPECT_EQ (1u, R_CSTL_HeapGetRegisteredCount ());

  R_CSTL_HeapUnregisterAllocation (&owner, p);
  EXPECT_EQ (0u, R_CSTL_HeapGetRegisteredCount ());
  R_CSTL_HeapFree (p);
}

TEST_F (CstlHeapAllocatorTest, RegisterAllocationRejectsNullOwnerOrAllocation)
{
  void* p = R_CSTL_HeapAlloc (16);
  ASSERT_NE (nullptr, p);
  EXPECT_EQ (0u, R_CSTL_HeapRegisterAllocation (nullptr, p, 16, "x"));
  EXPECT_EQ (0u, R_CSTL_HeapRegisterAllocation (&p, nullptr, 16, "x"));
  EXPECT_EQ (0u, R_CSTL_HeapRegisterAllocation (&p, p, 0, "x"));
  R_CSTL_HeapFree (p);
}

TEST_F (CstlHeapAllocatorTest, RegisterAllocationRejectsNonHeapPointer)
{
  char owner = 0;
  int  stack = 0;
  EXPECT_EQ (0u, R_CSTL_HeapRegisterAllocation (&owner, &stack, sizeof (stack), "stack"));
  EXPECT_EQ (0u, R_CSTL_HeapGetRegisteredCount ());
}

TEST_F (CstlHeapAllocatorTest, RegisterAllocationRejectsSizeMismatch)
{
  char  owner = 0;
  void* p     = R_CSTL_HeapAlloc (32);
  ASSERT_NE (nullptr, p);
  EXPECT_EQ (0u, R_CSTL_HeapRegisterAllocation (&owner, p, 64, "mismatch"));
  EXPECT_EQ (0u, R_CSTL_HeapGetRegisteredCount ());
  R_CSTL_HeapFree (p);
}

TEST_F (CstlHeapAllocatorTest, RegisterAllocationAllowsNullName)
{
  char  owner = 0;
  void* p     = R_CSTL_HeapAlloc (16);
  ASSERT_NE (nullptr, p);
  EXPECT_NE (0u, R_CSTL_HeapRegisterAllocation (&owner, p, 16, nullptr));
  EXPECT_EQ (1u, R_CSTL_HeapGetRegisteredCount ());
  R_CSTL_HeapUnregisterAllocation (&owner, p);
  R_CSTL_HeapFree (p);
}

TEST_F (CstlHeapAllocatorTest, UnregisterAllocationIsNoOpWhenMissing)
{
  char  owner = 0;
  void* p     = R_CSTL_HeapAlloc (16);
  ASSERT_NE (nullptr, p);
  R_CSTL_HeapUnregisterAllocation (&owner, p);
  EXPECT_EQ (0u, R_CSTL_HeapGetRegisteredCount ());
  R_CSTL_HeapFree (p);
}

TEST_F (CstlHeapAllocatorTest, UnregisterAllocationIgnoresNullArguments)
{
  char  owner = 0;
  void* p     = R_CSTL_HeapAlloc (16);
  ASSERT_NE (nullptr, p);
  ASSERT_NE (0u, R_CSTL_HeapRegisterAllocation (&owner, p, 16, "node"));
  R_CSTL_HeapUnregisterAllocation (nullptr, p);
  R_CSTL_HeapUnregisterAllocation (&owner, nullptr);
  EXPECT_EQ (1u, R_CSTL_HeapGetRegisteredCount ());
  R_CSTL_HeapUnregisterAllocation (&owner, p);
  R_CSTL_HeapFree (p);
}

TEST_F (CstlHeapAllocatorTest, CheckObjectLeaksReportsRegisteredAllocations)
{
  char  owner = 0;
  void* p     = R_CSTL_HeapAlloc (32);
  ASSERT_NE (nullptr, p);
  ASSERT_NE (0u, R_CSTL_HeapRegisterAllocation (&owner, p, 32, "leak"));

  EXPECT_EQ (1u, R_CSTL_HeapCheckObjectLeaks (&owner));
  EXPECT_EQ (0u, R_CSTL_HeapCheckObjectLeaks (nullptr));

  R_CSTL_HeapUnregisterAllocation (&owner, p);
  R_CSTL_HeapFree (p);
}

TEST_F (CstlHeapAllocatorTest, LogLeaksReturnsRegisteredCount)
{
  char  ownerA = 0;
  char  ownerB = 0;
  void* pA     = R_CSTL_HeapAlloc (16);
  void* pB     = R_CSTL_HeapAlloc (16);
  ASSERT_NE (nullptr, pA);
  ASSERT_NE (nullptr, pB);
  ASSERT_NE (0u, R_CSTL_HeapRegisterAllocation (&ownerA, pA, 16, "a"));
  ASSERT_NE (0u, R_CSTL_HeapRegisterAllocation (&ownerB, pB, 16, "b"));

  EXPECT_EQ (2u, R_CSTL_HeapLogLeaks ());

  R_CSTL_HeapUnregisterAllocation (&ownerA, pA);
  R_CSTL_HeapUnregisterAllocation (&ownerB, pB);
  R_CSTL_HeapFree (pA);
  R_CSTL_HeapFree (pB);
  EXPECT_EQ (0u, R_CSTL_HeapLogLeaks ());
}

TEST_F (CstlHeapAllocatorTest, FreeRemovesRegisteredAllocationRecord)
{
  char  owner = 0;
  void* p     = R_CSTL_HeapAlloc (24);
  ASSERT_NE (nullptr, p);
  ASSERT_NE (0u, R_CSTL_HeapRegisterAllocation (&owner, p, 24, "auto"));
  EXPECT_EQ (1u, R_CSTL_HeapGetRegisteredCount ());

  R_CSTL_HeapFree (p);
  EXPECT_EQ (0u, R_CSTL_HeapGetRegisteredCount ());
}

TEST_F (CstlHeapAllocatorTest, ExhaustHeapReturnsNull)
{
  std::vector<void*> blocks;
  blocks.reserve (256);
  while (void* p = R_CSTL_HeapAlloc (512))
  {
    blocks.push_back (p);
  }
  EXPECT_EQ (nullptr, R_CSTL_HeapAlloc (512));
  for (void* p : blocks)
    R_CSTL_HeapFree (p);
  EXPECT_EQ (0u, R_CSTL_Heap_GetUsedSize ());
}

TEST_F (CstlHeapAllocatorTest, AllocatedMemoryIsWritable)
{
  void* p = R_CSTL_HeapAlloc (128);
  ASSERT_NE (nullptr, p);
  std::memset (p, 0xAB, 128);
  EXPECT_EQ (0xAB, static_cast<unsigned char*> (p)[0]);
  EXPECT_EQ (0xAB, static_cast<unsigned char*> (p)[127]);
  R_CSTL_HeapFree (p);
}

#if R_CSTL_HEAP_DEBUG_ENABLED
TEST_F (CstlHeapAllocatorTest, DebugVerifyReportsHealthyHeap)
{
  void* p = R_CSTL_HeapAlloc (32);
  ASSERT_NE (nullptr, p);
  EXPECT_EQ (0, R_CSTL_HeapDebugVerify ());
  R_CSTL_HeapFree (p);
  EXPECT_EQ (0, R_CSTL_HeapDebugVerify ());
}

TEST_F (CstlHeapAllocatorTest, DebugAllocFillsPoisonPattern)
{
  void* p = R_CSTL_HeapAlloc (16);
  ASSERT_NE (nullptr, p);
  const auto* bytes = static_cast<const unsigned char*> (p);
  for (size_t i = 0; i < 16; ++i)
    EXPECT_EQ (0xCDu, bytes[i]) << "byte " << i;
  R_CSTL_HeapFree (p);
}

TEST_F (CstlHeapAllocatorTest, DebugFreeFillsPoisonPattern)
{
  void* p = R_CSTL_HeapAlloc (16);
  ASSERT_NE (nullptr, p);
  R_CSTL_HeapFree (p);
  const auto* bytes = static_cast<const unsigned char*> (p);
  for (size_t i = 0; i < 16; ++i)
    EXPECT_EQ (0xDDu, bytes[i]) << "byte " << i;
}
#endif
