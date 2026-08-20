#include <gtest/gtest.h>

#include <algorithm>
#include <cstring>
#include <vector>

extern "C"
{
#include "rlgame.base/cstl/cstl_array.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"
}

namespace
{

constexpr size_t kTestHeapSize = 256 * 1024;

class CstlArrayTest : public ::testing::Test
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

                static void
                ExpectBytes (const struct R_CSTL_Array* pArray, const std::vector<uint8_t>& expected)
                {
                        ASSERT_NE (nullptr, pArray);
                        EXPECT_EQ (expected.size (), R_CSTL_ArrayLength (pArray));
                        const uint8_t* pData = R_CSTL_ArrayData (pArray);
                        if (expected.empty ())
                        {
                                EXPECT_EQ (nullptr, pData);
                                return;
                        }
                        ASSERT_NE (nullptr, pData);
                        for (size_t i = 0; i < expected.size (); ++i)
                        {
                                uint8_t value = 0;
                                ASSERT_EQ (0, R_CSTL_ArrayAt (pArray, i, &value));
                                EXPECT_EQ (expected[i], value) << "index " << i;
                                uint8_t uncheckedValue = 0;
                                ASSERT_EQ (0, R_CSTL_ArrayUncheckedAt (pArray, i, &uncheckedValue));
                                EXPECT_EQ (expected[i], uncheckedValue) << "index " << i;
                        }
                }
};

} // namespace

TEST (CstlArrayInitTest, DeleteNullIsSafe)
{
        R_CSTL_DeleteArray (nullptr);
        SUCCEED ();
}

TEST_F (CstlArrayTest, NewEmptyArrayHasNoBufferUntilPush)
{
        struct R_CSTL_Array* pArray = R_CSTL_NewArray ();
        ASSERT_NE (nullptr, pArray);
        EXPECT_EQ (0u, R_CSTL_ArrayLength (pArray));
        EXPECT_EQ (0u, R_CSTL_ArrayGetCapacity (pArray));
        EXPECT_EQ (nullptr, R_CSTL_ArrayData (pArray));

        ASSERT_EQ (0, R_CSTL_ArrayPush (pArray, 0xAB));
        EXPECT_EQ (1u, R_CSTL_ArrayLength (pArray));
        EXPECT_GE (R_CSTL_ArrayGetCapacity (pArray), 1u);
        ASSERT_NE (nullptr, R_CSTL_ArrayData (pArray));
        uint8_t value = 0;
        ASSERT_EQ (0, R_CSTL_ArrayUncheckedAt (pArray, 0, &value));
        EXPECT_EQ (0xAB, value);

        R_CSTL_DeleteArray (pArray);
}

TEST_F (CstlArrayTest, NewArrayWithCapacity)
{
        struct R_CSTL_Array* pArray = R_CSTL_NewArrayWithCapacity (32);
        ASSERT_NE (nullptr, pArray);
        EXPECT_EQ (0u, R_CSTL_ArrayLength (pArray));
        EXPECT_GE (R_CSTL_ArrayGetCapacity (pArray), 32u);
        ASSERT_NE (nullptr, R_CSTL_ArrayData (pArray));
        R_CSTL_DeleteArray (pArray);
}

TEST_F (CstlArrayTest, NewArrayWithLengthZeroed)
{
        struct R_CSTL_Array* pArray = R_CSTL_NewArrayWithLengthZeroed (8);
        ASSERT_NE (nullptr, pArray);
        EXPECT_EQ (8u, R_CSTL_ArrayLength (pArray));
        for (size_t i = 0; i < 8; ++i)
        {
                uint8_t value = 0;
                ASSERT_EQ (0, R_CSTL_ArrayUncheckedAt (pArray, i, &value));
                EXPECT_EQ (0u, value);
        }
        R_CSTL_DeleteArray (pArray);
}

TEST_F (CstlArrayTest, NewArrayWithLengthUninitialized)
{
        struct R_CSTL_Array* pArray = R_CSTL_NewArrayWithLength (4);
        ASSERT_NE (nullptr, pArray);
        EXPECT_EQ (4u, R_CSTL_ArrayLength (pArray));
        EXPECT_GE (R_CSTL_ArrayGetCapacity (pArray), 4u);
        R_CSTL_DeleteArray (pArray);
}

TEST_F (CstlArrayTest, NewArrayWithData)
{
        const uint8_t        src[] = {1, 2, 3, 4, 5};
        struct R_CSTL_Array* pArray = R_CSTL_NewArrayWithData (src, sizeof (src));
        ASSERT_NE (nullptr, pArray);
        ExpectBytes (pArray, {1, 2, 3, 4, 5});
        R_CSTL_DeleteArray (pArray);
}

TEST_F (CstlArrayTest, NewArrayWithDataRejectsNullForNonZeroLength)
{
        EXPECT_EQ (nullptr, R_CSTL_NewArrayWithData (nullptr, 4));
}

TEST_F (CstlArrayTest, NewArrayWithDataAllowsEmpty)
{
        struct R_CSTL_Array* pArray = R_CSTL_NewArrayWithData (nullptr, 0);
        ASSERT_NE (nullptr, pArray);
        EXPECT_EQ (0u, R_CSTL_ArrayLength (pArray));
        R_CSTL_DeleteArray (pArray);
}

TEST_F (CstlArrayTest, RevBytesGrowsCapacityNotLength)
{
        struct R_CSTL_Array* pArray = R_CSTL_NewArray ();
        ASSERT_NE (nullptr, pArray);
        ASSERT_EQ (0, R_CSTL_ArrayRevBytes (pArray, 64));
        EXPECT_EQ (0u, R_CSTL_ArrayLength (pArray));
        EXPECT_GE (R_CSTL_ArrayGetCapacity (pArray), 64u);
        R_CSTL_DeleteArray (pArray);
}

TEST_F (CstlArrayTest, PushAndPop)
{
        struct R_CSTL_Array* pArray = R_CSTL_NewArray ();
        ASSERT_NE (nullptr, pArray);

        ASSERT_EQ (0, R_CSTL_ArrayPush (pArray, 10));
        ASSERT_EQ (0, R_CSTL_ArrayPush (pArray, 20));
        ASSERT_EQ (0, R_CSTL_ArrayPush (pArray, 30));
        ExpectBytes (pArray, {10, 20, 30});

        uint8_t value = 0;
        ASSERT_EQ (0, R_CSTL_ArrayPop (pArray, &value));
        EXPECT_EQ (30u, value);
        ExpectBytes (pArray, {10, 20});

        ASSERT_EQ (0, R_CSTL_ArrayPop (pArray, nullptr));
        ExpectBytes (pArray, {10});

        R_CSTL_DeleteArray (pArray);
}

TEST_F (CstlArrayTest, PopOnEmptyFails)
{
        struct R_CSTL_Array* pArray = R_CSTL_NewArray ();
        ASSERT_NE (nullptr, pArray);
        EXPECT_EQ (-1, R_CSTL_ArrayPop (pArray, nullptr));
        R_CSTL_DeleteArray (pArray);
}

TEST_F (CstlArrayTest, ShiftAndUnshift)
{
        const uint8_t        src[] = {1, 2, 3};
        struct R_CSTL_Array* pArray = R_CSTL_NewArrayWithData (src, sizeof (src));
        ASSERT_NE (nullptr, pArray);

        uint8_t value = 0;
        ASSERT_EQ (0, R_CSTL_ArrayShift (pArray, &value));
        EXPECT_EQ (1u, value);
        ExpectBytes (pArray, {2, 3});

        ASSERT_EQ (0, R_CSTL_ArrayUnshift (pArray, 9));
        ExpectBytes (pArray, {9, 2, 3});

        R_CSTL_DeleteArray (pArray);
}

TEST_F (CstlArrayTest, ShiftOnEmptyFails)
{
        struct R_CSTL_Array* pArray = R_CSTL_NewArray ();
        ASSERT_NE (nullptr, pArray);
        EXPECT_EQ (-1, R_CSTL_ArrayShift (pArray, nullptr));
        R_CSTL_DeleteArray (pArray);
}

TEST_F (CstlArrayTest, SliceReturnsSubrange)
{
        const uint8_t        src[] = {0, 1, 2, 3, 4, 5};
        struct R_CSTL_Array* pArray = R_CSTL_NewArrayWithData (src, sizeof (src));
        ASSERT_NE (nullptr, pArray);

        struct R_CSTL_Array* pSlice = R_CSTL_ArraySlice (pArray, 2, 5);
        ASSERT_NE (nullptr, pSlice);
        ExpectBytes (pSlice, {2, 3, 4});

        R_CSTL_DeleteArray (pSlice);
        R_CSTL_DeleteArray (pArray);
}

TEST_F (CstlArrayTest, SliceEmptyRange)
{
        const uint8_t        src[] = {1, 2, 3};
        struct R_CSTL_Array* pArray = R_CSTL_NewArrayWithData (src, sizeof (src));
        ASSERT_NE (nullptr, pArray);

        struct R_CSTL_Array* pSlice = R_CSTL_ArraySlice (pArray, 2, 2);
        ASSERT_NE (nullptr, pSlice);
        ExpectBytes (pSlice, {});

        R_CSTL_DeleteArray (pSlice);
        R_CSTL_DeleteArray (pArray);
}

TEST_F (CstlArrayTest, SliceRejectsInvalidRange)
{
        const uint8_t        src[] = {1, 2, 3};
        struct R_CSTL_Array* pArray = R_CSTL_NewArrayWithData (src, sizeof (src));
        ASSERT_NE (nullptr, pArray);

        EXPECT_EQ (nullptr, R_CSTL_ArraySlice (pArray, 4, 5));
        EXPECT_EQ (nullptr, R_CSTL_ArraySlice (pArray, 2, 1));
        EXPECT_EQ (nullptr, R_CSTL_ArraySlice (nullptr, 0, 1));

        R_CSTL_DeleteArray (pArray);
}

TEST_F (CstlArrayTest, AtBoundsCheck)
{
        const uint8_t        src[] = {7, 8};
        struct R_CSTL_Array* pArray = R_CSTL_NewArrayWithData (src, sizeof (src));
        ASSERT_NE (nullptr, pArray);

        uint8_t value = 0;
        EXPECT_EQ (0, R_CSTL_ArrayAt (pArray, 0, &value));
        EXPECT_EQ (7u, value);
        EXPECT_EQ (0, R_CSTL_ArrayAt (pArray, 1, &value));
        EXPECT_EQ (8u, value);
        EXPECT_EQ (-1, R_CSTL_ArrayAt (pArray, 2, &value));
        EXPECT_EQ (-1, R_CSTL_ArrayAt (pArray, 0, nullptr));
        EXPECT_EQ (-1, R_CSTL_ArrayAt (nullptr, 0, &value));

        R_CSTL_DeleteArray (pArray);
}

TEST_F (CstlArrayTest, GettersReturnZeroOrNullForNullArray)
{
        EXPECT_EQ (nullptr, R_CSTL_ArrayData (nullptr));
        EXPECT_EQ (0u, R_CSTL_ArrayLength (nullptr));
        EXPECT_EQ (0u, R_CSTL_ArrayGetCapacity (nullptr));
}

TEST_F (CstlArrayTest, RevBytesNoOpWhenAlreadyLargeEnough)
{
        struct R_CSTL_Array* pArray = R_CSTL_NewArrayWithCapacity (32);
        ASSERT_NE (nullptr, pArray);
        const size_t before = R_CSTL_ArrayGetCapacity (pArray);
        ASSERT_EQ (0, R_CSTL_ArrayRevBytes (pArray, 16));
        EXPECT_EQ (before, R_CSTL_ArrayGetCapacity (pArray));
        R_CSTL_DeleteArray (pArray);
}

namespace
{

struct SortableRecord
{
                uint32_t key;
                uint8_t  payload[12];
};

int
CompareRecordKey (const void* pLeft, const void* pRight, void* /*pData*/)
{
        const auto* pLeftRecord = static_cast<const SortableRecord*> (pLeft);
        const auto* pRightRecord = static_cast<const SortableRecord*> (pRight);
        if (pLeftRecord->key < pRightRecord->key) return -1;
        if (pLeftRecord->key > pRightRecord->key) return 1;
        return 0;
}

int
CompareU8DescendingWithCtx (const void* pLeft, const void* pRight, void* pData)
{
        const int* pDescending = static_cast<const int*> (pData);
        const int  sign = (*pDescending != 0) ? -1 : 1;
        return sign * R_CSTL_ArrayCompareU8 (pLeft, pRight, nullptr);
}

void
FillArrayWithU8 (struct R_CSTL_Array* pArray, const std::vector<uint8_t>& values)
{
        ASSERT_NE (nullptr, pArray);
        ASSERT_EQ (0, R_CSTL_ArrayClear (pArray, 0));
        for (uint8_t value : values)
                ASSERT_EQ (0, R_CSTL_ArrayPush (pArray, value));
}

} // namespace

TEST_F (CstlArrayTest, SortEmptyAndSingleElementSucceed)
{
        struct R_CSTL_Array* pArray = R_CSTL_NewArray ();
        ASSERT_NE (nullptr, pArray);
        EXPECT_EQ (0, R_CSTL_ArraySort (pArray, sizeof (uint8_t), R_CSTL_ArrayCompareU8, nullptr));

        ASSERT_EQ (0, R_CSTL_ArrayPush (pArray, 42));
        EXPECT_EQ (0, R_CSTL_ArraySort (pArray, sizeof (uint8_t), R_CSTL_ArrayCompareU8, nullptr));
        ExpectBytes (pArray, {42});

        R_CSTL_DeleteArray (pArray);
}

TEST_F (CstlArrayTest, SortU8Ascending)
{
        struct R_CSTL_Array* pArray = R_CSTL_NewArray ();
        ASSERT_NE (nullptr, pArray);
        FillArrayWithU8 (pArray, {5, 1, 4, 2, 8, 0, 2, 9, 1, 5});

        ASSERT_EQ (0, R_CSTL_ArraySort (pArray, sizeof (uint8_t), R_CSTL_ArrayCompareU8, nullptr));
        ExpectBytes (pArray, {0, 1, 1, 2, 2, 4, 5, 5, 8, 9});

        R_CSTL_DeleteArray (pArray);
}

TEST_F (CstlArrayTest, SortAlreadySortedAndReverseSorted)
{
        const uint8_t sortedSrc[] = {1, 2, 3, 4, 5};
        const uint8_t reverseSrc[] = {9, 8, 7, 6, 5, 4, 3, 2, 1, 0};

        struct R_CSTL_Array* pSorted = R_CSTL_NewArrayWithData (sortedSrc, sizeof (sortedSrc));
        ASSERT_NE (nullptr, pSorted);
        ASSERT_EQ (0, R_CSTL_ArraySort (pSorted, sizeof (uint8_t), R_CSTL_ArrayCompareU8, nullptr));
        ExpectBytes (pSorted, {1, 2, 3, 4, 5});

        struct R_CSTL_Array* pReverse = R_CSTL_NewArrayWithData (reverseSrc, sizeof (reverseSrc));
        ASSERT_NE (nullptr, pReverse);
        ASSERT_EQ (0, R_CSTL_ArraySort (pReverse, sizeof (uint8_t), R_CSTL_ArrayCompareU8, nullptr));
        ExpectBytes (pReverse, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});

        R_CSTL_DeleteArray (pSorted);
        R_CSTL_DeleteArray (pReverse);
}

TEST_F (CstlArrayTest, SortSmallArrayUsesInsertionPath)
{
        struct R_CSTL_Array* pArray = R_CSTL_NewArray ();
        ASSERT_NE (nullptr, pArray);
        FillArrayWithU8 (pArray, {7, 3, 9, 1, 5, 2});

        ASSERT_EQ (0, R_CSTL_ArraySort (pArray, sizeof (uint8_t), R_CSTL_ArrayCompareU8, nullptr));
        ExpectBytes (pArray, {1, 2, 3, 5, 7, 9});

        R_CSTL_DeleteArray (pArray);
}

TEST_F (CstlArrayTest, SortU32Elements)
{
        const uint32_t       src[] = {100u, 3u, 42u, 7u, 7u, 1u, 256u};
        struct R_CSTL_Array* pArray
            = R_CSTL_NewArrayWithData (reinterpret_cast<const uint8_t*> (src), sizeof (src));
        ASSERT_NE (nullptr, pArray);

        ASSERT_EQ (0, R_CSTL_ArraySort (pArray, sizeof (uint32_t), R_CSTL_ArrayCompareU32, nullptr));

        const uint8_t* pData = R_CSTL_ArrayData (pArray);
        ASSERT_NE (nullptr, pData);
        const auto* pValues = reinterpret_cast<const uint32_t*> (pData);
        EXPECT_EQ (7u, R_CSTL_ArrayLength (pArray) / sizeof (uint32_t));
        EXPECT_EQ (1u, pValues[0]);
        EXPECT_EQ (3u, pValues[1]);
        EXPECT_EQ (7u, pValues[2]);
        EXPECT_EQ (7u, pValues[3]);
        EXPECT_EQ (42u, pValues[4]);
        EXPECT_EQ (100u, pValues[5]);
        EXPECT_EQ (256u, pValues[6]);

        R_CSTL_DeleteArray (pArray);
}

TEST_F (CstlArrayTest, SortSimdSizedElementsPreservesPayload)
{
        SortableRecord records[] = {
            {30u, {0xAA, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B}},
            {10u, {0xBB, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B}},
            {20u, {0xCC, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B}},
        };

        struct R_CSTL_Array* pArray
            = R_CSTL_NewArrayWithData (reinterpret_cast<const uint8_t*> (records), sizeof (records));
        ASSERT_NE (nullptr, pArray);

        ASSERT_EQ (0, R_CSTL_ArraySort (pArray, sizeof (SortableRecord), CompareRecordKey, nullptr));

        const auto* pSorted = reinterpret_cast<const SortableRecord*> (R_CSTL_ArrayData (pArray));
        ASSERT_NE (nullptr, pSorted);
        EXPECT_EQ (3u, R_CSTL_ArrayLength (pArray) / sizeof (SortableRecord));
        EXPECT_EQ (10u, pSorted[0].key);
        EXPECT_EQ (0xBB, pSorted[0].payload[0]);
        EXPECT_EQ (20u, pSorted[1].key);
        EXPECT_EQ (0xCC, pSorted[1].payload[0]);
        EXPECT_EQ (30u, pSorted[2].key);
        EXPECT_EQ (0xAA, pSorted[2].payload[0]);

        R_CSTL_DeleteArray (pArray);
}

TEST_F (CstlArrayTest, SortWithComparatorContext)
{
        const uint8_t        src[] = {1, 9, 2, 8, 3};
        struct R_CSTL_Array* pArray = R_CSTL_NewArrayWithData (src, sizeof (src));
        ASSERT_NE (nullptr, pArray);

        const int descending = 1;
        ASSERT_EQ (
            0,
            R_CSTL_ArraySort (pArray, sizeof (uint8_t), CompareU8DescendingWithCtx, (void*)&descending));
        ExpectBytes (pArray, {9, 8, 3, 2, 1});

        R_CSTL_DeleteArray (pArray);
}

TEST_F (CstlArrayTest, SortRejectsInvalidArguments)
{
        const uint8_t        src[] = {1, 2, 3};
        struct R_CSTL_Array* pArray = R_CSTL_NewArrayWithData (src, sizeof (src));
        ASSERT_NE (nullptr, pArray);

        EXPECT_EQ (-1, R_CSTL_ArraySort (nullptr, sizeof (uint8_t), R_CSTL_ArrayCompareU8, nullptr));
        EXPECT_EQ (-1, R_CSTL_ArraySort (pArray, 0, R_CSTL_ArrayCompareU8, nullptr));
        EXPECT_EQ (-1, R_CSTL_ArraySort (pArray, 2, R_CSTL_ArrayCompareU8, nullptr));

        R_CSTL_DeleteArray (pArray);
}

// Stress tests
TEST_F (CstlArrayTest, StressManyPushOperations)
{
        constexpr int        kNumPushes = 1000;
        struct R_CSTL_Array* pArray = R_CSTL_NewArray ();
        ASSERT_NE (nullptr, pArray);

        for (int i = 0; i < kNumPushes; ++i)
        {
                ASSERT_EQ (0, R_CSTL_ArrayPush (pArray, static_cast<uint8_t> (i % 256)));
        }

        EXPECT_EQ (kNumPushes, (int)R_CSTL_ArrayLength (pArray));

        R_CSTL_DeleteArray (pArray);
}

TEST_F (CstlArrayTest, StressManyPopOperations)
{
        constexpr int        kNumPushes = 200;
        struct R_CSTL_Array* pArray = R_CSTL_NewArrayWithCapacity (kNumPushes);
        ASSERT_NE (nullptr, pArray);

        for (int i = 0; i < kNumPushes; ++i)
        {
                ASSERT_EQ (0, R_CSTL_ArrayPush (pArray, static_cast<uint8_t> (i % 256)));
        }

        for (int i = 0; i < kNumPushes; ++i)
        {
                uint8_t value = 0;
                ASSERT_EQ (0, R_CSTL_ArrayPop (pArray, &value));
                EXPECT_EQ (static_cast<uint8_t> ((kNumPushes - 1 - i) % 256), value);
        }

        EXPECT_EQ (0u, R_CSTL_ArrayLength (pArray));

        R_CSTL_DeleteArray (pArray);
}

TEST_F (CstlArrayTest, StressPushPopPattern)
{
        constexpr int        kIterations = 100;
        struct R_CSTL_Array* pArray = R_CSTL_NewArray ();
        ASSERT_NE (nullptr, pArray);

        for (int iter = 0; iter < kIterations; ++iter)
        {
                // Push 10 values
                for (int i = 0; i < 10; ++i)
                {
                        ASSERT_EQ (0, R_CSTL_ArrayPush (pArray, static_cast<uint8_t> (i)));
                }

                // Pop 5 values
                for (int i = 0; i < 5; ++i)
                {
                        uint8_t value = 0;
                        ASSERT_EQ (0, R_CSTL_ArrayPop (pArray, &value));
                }
        }

        R_CSTL_DeleteArray (pArray);
}

TEST_F (CstlArrayTest, StressShiftUnshiftPattern)
{
        constexpr int        kIterations = 50;
        struct R_CSTL_Array* pArray = R_CSTL_NewArray ();
        ASSERT_NE (nullptr, pArray);

        for (int iter = 0; iter < kIterations; ++iter)
        {
                // Unshift 5 values
                for (int i = 0; i < 5; ++i)
                {
                        ASSERT_EQ (0, R_CSTL_ArrayUnshift (pArray, static_cast<uint8_t> (i)));
                }

                // Shift 3 values
                for (int i = 0; i < 3; ++i)
                {
                        uint8_t value = 0;
                        ASSERT_EQ (0, R_CSTL_ArrayShift (pArray, &value));
                }
        }

        R_CSTL_DeleteArray (pArray);
}

TEST_F (CstlArrayTest, StressSliceOperations)
{
        constexpr int        kNumElements = 100;
        struct R_CSTL_Array* pArray = R_CSTL_NewArrayWithCapacity (kNumElements);
        ASSERT_NE (nullptr, pArray);

        for (int i = 0; i < kNumElements; ++i)
        {
                ASSERT_EQ (0, R_CSTL_ArrayPush (pArray, static_cast<uint8_t> (i)));
        }

        // Create many slices
        for (int i = 0; i < 50; ++i)
        {
                size_t               start = rand () % kNumElements;
                size_t               end = start + (rand () % (kNumElements - start));
                struct R_CSTL_Array* pSlice = R_CSTL_ArraySlice (pArray, start, end);
                if (pSlice)
                {
                        EXPECT_EQ (end - start, R_CSTL_ArrayLength (pSlice));
                        R_CSTL_DeleteArray (pSlice);
                }
        }

        R_CSTL_DeleteArray (pArray);
}

TEST_F (CstlArrayTest, StressResizeOperations)
{
        constexpr int        kIterations = 100;
        struct R_CSTL_Array* pArray = R_CSTL_NewArray ();
        ASSERT_NE (nullptr, pArray);

        for (int iter = 0; iter < kIterations; ++iter)
        {
                size_t newCapacity = (rand () % 1024) + 32;
                ASSERT_EQ (0, R_CSTL_ArrayRevBytes (pArray, newCapacity));
        }

        R_CSTL_DeleteArray (pArray);
}

TEST_F (CstlArrayTest, StressFillAndClear)
{
        constexpr int        kIterations = 200;
        struct R_CSTL_Array* pArray = R_CSTL_NewArrayWithCapacity (100);
        ASSERT_NE (nullptr, pArray);

        for (int iter = 0; iter < kIterations; ++iter)
        {
                // Fill with data
                for (int i = 0; i < 50; ++i)
                {
                        ASSERT_EQ (0, R_CSTL_ArrayPush (pArray, static_cast<uint8_t> (iter % 256)));
                }

                // Fill with pattern
                ASSERT_EQ (0, R_CSTL_ArrayFill (pArray, 0xAA));

                // Clear
                ASSERT_EQ (0, R_CSTL_ArrayClear (pArray, 0));
        }

        R_CSTL_DeleteArray (pArray);
}

TEST_F (CstlArrayTest, StressSortManyElements)
{
        constexpr int        kNumElements = 500;
        struct R_CSTL_Array* pArray = R_CSTL_NewArrayWithCapacity (kNumElements);
        ASSERT_NE (nullptr, pArray);

        // Add random elements
        for (int i = 0; i < kNumElements; ++i)
        {
                ASSERT_EQ (0, R_CSTL_ArrayPush (pArray, static_cast<uint8_t> (rand ())));
        }

        // Sort
        ASSERT_EQ (0, R_CSTL_ArraySort (pArray, sizeof (uint8_t), R_CSTL_ArrayCompareU8, nullptr));

        // Verify sorted
        const uint8_t* pData = R_CSTL_ArrayData (pArray);
        for (size_t i = 1; i < kNumElements; ++i)
        {
                EXPECT_LE (pData[i - 1], pData[i]);
        }

        R_CSTL_DeleteArray (pArray);
}

TEST_F (CstlArrayTest, StressMixedOperations)
{
        constexpr int        kIterations = 100;
        struct R_CSTL_Array* pArray = R_CSTL_NewArray ();
        ASSERT_NE (nullptr, pArray);

        for (int iter = 0; iter < kIterations; ++iter)
        {
                int operation = rand () % 5;

                switch (operation)
                {
                case 0: // Push
                        ASSERT_EQ (0, R_CSTL_ArrayPush (pArray, static_cast<uint8_t> (rand ())));
                        break;
                case 1: // Pop
                        if (R_CSTL_ArrayLength (pArray) > 0)
                        {
                                uint8_t value = 0;
                                R_CSTL_ArrayPop (pArray, &value);
                        }
                        break;
                case 2: // Unshift
                        ASSERT_EQ (0, R_CSTL_ArrayUnshift (pArray, static_cast<uint8_t> (rand ())));
                        break;
                case 3: // Shift
                        if (R_CSTL_ArrayLength (pArray) > 0)
                        {
                                uint8_t value = 0;
                                R_CSTL_ArrayShift (pArray, &value);
                        }
                        break;
                case 4: // Clear
                        R_CSTL_ArrayClear (pArray, 0);
                        break;
                }
        }

        R_CSTL_DeleteArray (pArray);
}

TEST_F (CstlArrayTest, StressLargeArray)
{
        constexpr size_t     kLargeSize = 10000;
        struct R_CSTL_Array* pArray = R_CSTL_NewArrayWithCapacity (kLargeSize);
        ASSERT_NE (nullptr, pArray);

        for (size_t i = 0; i < kLargeSize; ++i)
        {
                ASSERT_EQ (0, R_CSTL_ArrayPush (pArray, static_cast<uint8_t> (i % 256)));
        }

        EXPECT_EQ (kLargeSize, R_CSTL_ArrayLength (pArray));

        // Verify data integrity
        for (size_t i = 0; i < kLargeSize; ++i)
        {
                uint8_t value = 0;
                ASSERT_EQ (0, R_CSTL_ArrayAt (pArray, i, &value));
                EXPECT_EQ (static_cast<uint8_t> (i % 256), value);
        }

        R_CSTL_DeleteArray (pArray);
}

TEST_F (CstlArrayTest, PushDataAppendsMultipleBytes)
{
        struct R_CSTL_Array* pArray = R_CSTL_NewArray ();
        ASSERT_NE (nullptr, pArray);

        const uint8_t data[] = {1, 2, 3, 4, 5};
        ASSERT_EQ (0, R_CSTL_ArrayPushData (pArray, data, sizeof (data)));
        
        EXPECT_EQ (5u, R_CSTL_ArrayLength (pArray));
        ExpectBytes (pArray, {1, 2, 3, 4, 5});

        R_CSTL_DeleteArray (pArray);
}

TEST_F (CstlArrayTest, PushDataWithNullDataFails)
{
        struct R_CSTL_Array* pArray = R_CSTL_NewArray ();
        ASSERT_NE (nullptr, pArray);

        EXPECT_EQ (-1, R_CSTL_ArrayPushData (pArray, nullptr, 10));

        R_CSTL_DeleteArray (pArray);
}

TEST_F (CstlArrayTest, PushDataWithZeroSizeSucceeds)
{
        struct R_CSTL_Array* pArray = R_CSTL_NewArray ();
        ASSERT_NE (nullptr, pArray);

        const uint8_t data[] = {1, 2, 3};
        ASSERT_EQ (0, R_CSTL_ArrayPushData (pArray, data, 0));
        EXPECT_EQ (0u, R_CSTL_ArrayLength (pArray));

        R_CSTL_DeleteArray (pArray);
}

TEST_F (CstlArrayTest, ClearWithZeroMemory)
{
        const uint8_t        src[] = {1, 2, 3, 4, 5};
        struct R_CSTL_Array* pArray = R_CSTL_NewArrayWithData (src, sizeof (src));
        ASSERT_NE (nullptr, pArray);

        ASSERT_EQ (0, R_CSTL_ArrayClear (pArray, 1));
        EXPECT_EQ (0u, R_CSTL_ArrayLength (pArray));
        
        // Verify memory is zeroed
        const uint8_t* pData = R_CSTL_ArrayData (pArray);
        ASSERT_NE (nullptr, pData);
        for (size_t i = 0; i < sizeof (src); ++i)
        {
                EXPECT_EQ (0u, pData[i]);
        }

        R_CSTL_DeleteArray (pArray);
}

TEST_F (CstlArrayTest, ClearWithoutZeroMemory)
{
        const uint8_t        src[] = {1, 2, 3, 4, 5};
        struct R_CSTL_Array* pArray = R_CSTL_NewArrayWithData (src, sizeof (src));
        ASSERT_NE (nullptr, pArray);

        ASSERT_EQ (0, R_CSTL_ArrayClear (pArray, 0));
        EXPECT_EQ (0u, R_CSTL_ArrayLength (pArray));

        R_CSTL_DeleteArray (pArray);
}

TEST_F (CstlArrayTest, FillSetsAllBytes)
{
        const uint8_t        src[] = {1, 2, 3, 4, 5};
        struct R_CSTL_Array* pArray = R_CSTL_NewArrayWithData (src, sizeof (src));
        ASSERT_NE (nullptr, pArray);

        ASSERT_EQ (0, R_CSTL_ArrayFill (pArray, 0xFF));
        
        const uint8_t* pData = R_CSTL_ArrayData (pArray);
        ASSERT_NE (nullptr, pData);
        for (size_t i = 0; i < sizeof (src); ++i)
        {
                EXPECT_EQ (0xFF, pData[i]);
        }

        R_CSTL_DeleteArray (pArray);
}

TEST_F (CstlArrayTest, SortU16Elements)
{
        const uint16_t       src[] = {100u, 3u, 42u, 7u, 1u, 256u};
        struct R_CSTL_Array* pArray
            = R_CSTL_NewArrayWithData (reinterpret_cast<const uint8_t*> (src), sizeof (src));
        ASSERT_NE (nullptr, pArray);

        ASSERT_EQ (0, R_CSTL_ArraySort (pArray, sizeof (uint16_t), R_CSTL_ArrayCompareU16, nullptr));

        const uint8_t* pData = R_CSTL_ArrayData (pArray);
        ASSERT_NE (nullptr, pData);
        const auto* pValues = reinterpret_cast<const uint16_t*> (pData);
        EXPECT_EQ (6u, R_CSTL_ArrayLength (pArray) / sizeof (uint16_t));
        EXPECT_EQ (1u, pValues[0]);
        EXPECT_EQ (3u, pValues[1]);
        EXPECT_EQ (7u, pValues[2]);
        EXPECT_EQ (42u, pValues[3]);
        EXPECT_EQ (100u, pValues[4]);
        EXPECT_EQ (256u, pValues[5]);

        R_CSTL_DeleteArray (pArray);
}

TEST_F (CstlArrayTest, SortU64Elements)
{
        const uint64_t       src[] = {1000ULL, 3ULL, 42ULL, 7ULL, 1ULL, 256ULL};
        struct R_CSTL_Array* pArray
            = R_CSTL_NewArrayWithData (reinterpret_cast<const uint8_t*> (src), sizeof (src));
        ASSERT_NE (nullptr, pArray);

        ASSERT_EQ (0, R_CSTL_ArraySort (pArray, sizeof (uint64_t), R_CSTL_ArrayCompareU64, nullptr));

        const uint8_t* pData = R_CSTL_ArrayData (pArray);
        ASSERT_NE (nullptr, pData);
        const auto* pValues = reinterpret_cast<const uint64_t*> (pData);
        EXPECT_EQ (6u, R_CSTL_ArrayLength (pArray) / sizeof (uint64_t));
        EXPECT_EQ (1ULL, pValues[0]);
        EXPECT_EQ (3ULL, pValues[1]);
        EXPECT_EQ (7ULL, pValues[2]);
        EXPECT_EQ (42ULL, pValues[3]);
        EXPECT_EQ (256ULL, pValues[4]);
        EXPECT_EQ (1000ULL, pValues[5]);

        R_CSTL_DeleteArray (pArray);
}

TEST_F (CstlArrayTest, TypedAtMacro)
{
        const uint32_t       src[] = {100u, 200u, 300u};
        struct R_CSTL_Array* pArray
            = R_CSTL_NewArrayWithData (reinterpret_cast<const uint8_t*> (src), sizeof (src));
        ASSERT_NE (nullptr, pArray);

        uint32_t value = 0;
        R_CSTL_ArrayTypedAt (pArray, uint32_t, 1, &value);
        EXPECT_EQ (200u, value);

        R_CSTL_DeleteArray (pArray);
}

TEST_F (CstlArrayTest, TypedAtUncheckedMacro)
{
        const uint32_t       src[] = {100u, 200u, 300u};
        struct R_CSTL_Array* pArray
            = R_CSTL_NewArrayWithData (reinterpret_cast<const uint8_t*> (src), sizeof (src));
        ASSERT_NE (nullptr, pArray);

        uint32_t value = 0;
        R_CSTL_ArrayTypedAtUnchecked (pArray, uint32_t, 2, &value);
        EXPECT_EQ (300u, value);

        R_CSTL_DeleteArray (pArray);
}

TEST_F (CstlArrayTest, UncheckedAtBoundsCheck)
{
        const uint8_t        src[] = {7, 8};
        struct R_CSTL_Array* pArray = R_CSTL_NewArrayWithData (src, sizeof (src));
        ASSERT_NE (nullptr, pArray);

        uint8_t value = 0;
        EXPECT_EQ (0, R_CSTL_ArrayUncheckedAt (pArray, 0, &value));
        EXPECT_EQ (7u, value);
        EXPECT_EQ (0, R_CSTL_ArrayUncheckedAt (pArray, 1, &value));
        EXPECT_EQ (8u, value);

        R_CSTL_DeleteArray (pArray);
}

TEST_F (CstlArrayTest, RevBytesRejectsNullArray)
{
        EXPECT_EQ (-1, R_CSTL_ArrayRevBytes (nullptr, 32));
}

TEST_F (CstlArrayTest, PushRejectsNullArray)
{
        EXPECT_EQ (-1, R_CSTL_ArrayPush (nullptr, 42));
}

TEST_F (CstlArrayTest, PopRejectsNullArray)
{
        uint8_t value = 0;
        EXPECT_EQ (-1, R_CSTL_ArrayPop (nullptr, &value));
}

TEST_F (CstlArrayTest, ShiftRejectsNullArray)
{
        uint8_t value = 0;
        EXPECT_EQ (-1, R_CSTL_ArrayShift (nullptr, &value));
}

TEST_F (CstlArrayTest, UnshiftRejectsNullArray)
{
        EXPECT_EQ (-1, R_CSTL_ArrayUnshift (nullptr, 42));
}

TEST_F (CstlArrayTest, ClearRejectsNullArray)
{
        EXPECT_EQ (-1, R_CSTL_ArrayClear (nullptr, 1));
}

TEST_F (CstlArrayTest, FillRejectsNullArray)
{
        EXPECT_EQ (-1, R_CSTL_ArrayFill (nullptr, 0xFF));
}
