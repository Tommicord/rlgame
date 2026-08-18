#include <benchmark/benchmark.h>

#include <cstdint>
#include <cstring>
#include <vector>
#include <algorithm>

extern "C" {
#include "rlgame.base/cstl/cstl_array.h"
}

#include "cstl_bench_common.h"

namespace {

constexpr size_t kBenchHeapSize = 64u * 1024u * 1024u;

class ArrayBenchFixture : public benchmark::Fixture
{
protected:
        void
        SetUp (const ::benchmark::State& state) override
        {
                if (!BenchEnsureHeap (const_cast<::benchmark::State&> (state), kBenchHeapSize))
                        return;
        }

        void
        TearDown (const ::benchmark::State& /*state*/) override
        { R_CSTL_HeapShutdown (); }
};

} // namespace

BENCHMARK_DEFINE_F (ArrayBenchFixture, PushWithoutReserve) (benchmark::State& state)
{
        if (!BenchEnsureHeap (state, kBenchHeapSize))
                return;

        const int64_t count = state.range (0);

        for (auto _ : state)
        {
                struct R_CSTL_Array* pArray = R_CSTL_NewArray ();
                if (!pArray)
                {
                        state.SkipWithError ("NewArray failed");
                        return;
                }

                for (int64_t i = 0; i < count; ++i)
                        R_CSTL_ArrayPush (pArray, static_cast<uint8_t> (i & 0xFF));

                benchmark::DoNotOptimize (pArray);
                R_CSTL_DeleteArray (pArray);
        }

        state.SetItemsProcessed (state.iterations () * count);
}
BENCHMARK_REGISTER_F (ArrayBenchFixture, PushWithoutReserve)
    ->Arg (128)
    ->Arg (1024)
    ->Arg (8192)
    ->Unit (benchmark::kMicrosecond);

BENCHMARK_DEFINE_F (ArrayBenchFixture, PushWithReserve) (benchmark::State& state)
{
        if (!BenchEnsureHeap (state, kBenchHeapSize))
                return;

        const int64_t count = state.range (0);

        for (auto _ : state)
        {
                struct R_CSTL_Array* pArray = R_CSTL_NewArrayWithCapacity (static_cast<size_t> (count));
                if (!pArray)
                {
                        state.SkipWithError ("NewArrayWithCapacity failed");
                        return;
                }

                for (int64_t i = 0; i < count; ++i)
                        R_CSTL_ArrayPush (pArray, static_cast<uint8_t> (i & 0xFF));

                benchmark::DoNotOptimize (pArray);
                R_CSTL_DeleteArray (pArray);
        }

        state.SetItemsProcessed (state.iterations () * count);
}
BENCHMARK_REGISTER_F (ArrayBenchFixture, PushWithReserve)
    ->Arg (128)
    ->Arg (1024)
    ->Arg (8192)
    ->Unit (benchmark::kMicrosecond);

BENCHMARK_DEFINE_F (ArrayBenchFixture, Pop) (benchmark::State& state)
{
        if (!BenchEnsureHeap (state, kBenchHeapSize))
                return;

        const int64_t count = state.range (0);
        struct R_CSTL_Array* pArray = R_CSTL_NewArrayWithCapacity (static_cast<size_t> (count));
        if (!pArray)
        {
                state.SkipWithError ("setup failed");
                return;
        }
        for (int64_t i = 0; i < count; ++i)
                R_CSTL_ArrayPush (pArray, static_cast<uint8_t> (i & 0xFF));

        for (auto _ : state)
        {
                state.PauseTiming ();
                for (int64_t i = 0; i < count; ++i)
                        R_CSTL_ArrayPush (pArray, static_cast<uint8_t> (i & 0xFF));
                state.ResumeTiming ();

                for (int64_t i = 0; i < count; ++i)
                        R_CSTL_ArrayPop (pArray, nullptr);

                benchmark::DoNotOptimize (pArray);
        }

        R_CSTL_DeleteArray (pArray);
        state.SetItemsProcessed (state.iterations () * count);
}
BENCHMARK_REGISTER_F (ArrayBenchFixture, Pop)->Arg (1024)->Unit (benchmark::kMicrosecond);

BENCHMARK_DEFINE_F (ArrayBenchFixture, Shift) (benchmark::State& state)
{
        if (!BenchEnsureHeap (state, kBenchHeapSize))
                return;

        const int64_t count = state.range (0);

        for (auto _ : state)
        {
                struct R_CSTL_Array* pArray = R_CSTL_NewArrayWithCapacity (static_cast<size_t> (count));
                if (!pArray)
                {
                        state.SkipWithError ("setup failed");
                        return;
                }
                for (int64_t i = 0; i < count; ++i)
                        R_CSTL_ArrayPush (pArray, static_cast<uint8_t> (i & 0xFF));

                for (int64_t i = 0; i < count; ++i)
                        R_CSTL_ArrayShift (pArray, nullptr);

                benchmark::DoNotOptimize (pArray);
                R_CSTL_DeleteArray (pArray);
        }

        state.SetItemsProcessed (state.iterations () * count);
}
BENCHMARK_REGISTER_F (ArrayBenchFixture, Shift)
      ->Arg (256)
      ->Arg (1024)
      ->Arg (4096)
      ->Unit (benchmark::kMicrosecond);

BENCHMARK_DEFINE_F (ArrayBenchFixture, SortU8) (benchmark::State& state)
{
        if (!BenchEnsureHeap (state, kBenchHeapSize))
                return;

        const int64_t count = state.range (0);
        std::vector<uint8_t> values (static_cast<size_t> (count));
        for (int64_t i = 0; i < count; ++i)
                values[static_cast<size_t> (i)] = static_cast<uint8_t> ((i * 73) & 0xFF);

        for (auto _ : state)
        {
                struct R_CSTL_Array* pArray =
                    R_CSTL_NewArrayWithData (values.data (), values.size ());
                if (!pArray)
                {
                        state.SkipWithError ("NewArrayWithData failed");
                        return;
                }

                R_CSTL_ArraySort (pArray, sizeof (uint8_t), R_CSTL_ArrayCompareU8, nullptr);
                benchmark::DoNotOptimize (pArray);
                R_CSTL_DeleteArray (pArray);
        }

        state.SetItemsProcessed (state.iterations () * count);
}
BENCHMARK_REGISTER_F (ArrayBenchFixture, SortU8)
    ->Arg (64)
    ->Arg (1024)
    ->Arg (4096)
    ->Arg (8192)
    ->Unit (benchmark::kMicrosecond);

BENCHMARK_DEFINE_F (ArrayBenchFixture, SortU32) (benchmark::State& state)
{
        if (!BenchEnsureHeap (state, kBenchHeapSize))
                return;

        const int64_t count = state.range (0);
        std::vector<uint32_t> values (static_cast<size_t> (count));
        for (int64_t i = 0; i < count; ++i)
                values[static_cast<size_t> (i)] = static_cast<uint32_t> (i * 2654435761u);

        for (auto _ : state)
        {
                struct R_CSTL_Array* pArray = R_CSTL_NewArrayWithData (
                    reinterpret_cast<const uint8_t*> (values.data ()),
                    values.size () * sizeof (uint32_t));
                if (!pArray)
                {
                        state.SkipWithError ("NewArrayWithData failed");
                        return;
                }

                R_CSTL_ArraySort (pArray, sizeof (uint32_t), R_CSTL_ArrayCompareU32, nullptr);
                benchmark::DoNotOptimize (pArray);
                R_CSTL_DeleteArray (pArray);
        }

        state.SetItemsProcessed (state.iterations () * count);
}
BENCHMARK_REGISTER_F (ArrayBenchFixture, SortU32)
    ->Arg (256)
    ->Arg (4096)
    ->Arg (8192)
    ->Arg (16384)
    ->Arg (2097152)
    ->Unit (benchmark::kMicrosecond);

BENCHMARK_DEFINE_F (ArrayBenchFixture, Slice) (benchmark::State& state)
{
        if (!BenchEnsureHeap (state, kBenchHeapSize))
                return;

        const int64_t count = state.range (0);
        std::vector<uint8_t> values (static_cast<size_t> (count), 0xAB);
        struct R_CSTL_Array* pSource = R_CSTL_NewArrayWithData (values.data (), values.size ());
        if (!pSource)
        {
                state.SkipWithError ("setup failed");
                return;
        }

        for (auto _ : state)
        {
                struct R_CSTL_Array* pSlice = R_CSTL_ArraySlice (pSource, 0, static_cast<size_t> (count));
                benchmark::DoNotOptimize (pSlice);
                R_CSTL_DeleteArray (pSlice);
        }

        R_CSTL_DeleteArray (pSource);
        state.SetItemsProcessed (state.iterations () * count);
}
BENCHMARK_REGISTER_F (ArrayBenchFixture, Slice)->Arg (4096)->Arg (65536)->Unit (benchmark::kMicrosecond);

BENCHMARK_DEFINE_F (ArrayBenchFixture, UncheckedAt) (benchmark::State& state)
{
        if (!BenchEnsureHeap (state, kBenchHeapSize))
                return;

        const int64_t count = state.range (0);
        struct R_CSTL_Array* pArray = R_CSTL_NewArrayWithCapacity (static_cast<size_t> (count));
        if (!pArray)
        {
                state.SkipWithError ("setup failed");
                return;
        }
        for (int64_t i = 0; i < count; ++i)
                R_CSTL_ArrayPush (pArray, static_cast<uint8_t> (i & 0xFF));

        uint64_t checksum = 0;
        for (auto _ : state)
        {
                for (int64_t i = 0; i < count; ++i)
                        checksum += R_CSTL_ArrayUncheckedAt (pArray, static_cast<size_t> (i));
                benchmark::DoNotOptimize (checksum);
        }

        R_CSTL_DeleteArray (pArray);
        state.SetItemsProcessed (state.iterations () * count);
}
BENCHMARK_REGISTER_F (ArrayBenchFixture, UncheckedAt)->Arg (1024)->Arg (8192)->Unit (benchmark::kMicrosecond);

BENCHMARK_DEFINE_F (ArrayBenchFixture, SortU32Aligned) (benchmark::State& state)
{
        if (!BenchEnsureHeap (state, kBenchHeapSize))
                return;

        const int64_t count = state.range (0);
        const size_t totalBytes = static_cast<size_t> (count) * sizeof (uint32_t);
        
        uint32_t* pAligned = static_cast<uint32_t*> (R_CSTL_HeapAllocAligned (totalBytes, 32));
        if (!pAligned)
        {
                state.SkipWithError ("aligned allocation failed");
                return;
        }
        
        std::vector<uint32_t> values (static_cast<size_t> (count));
        for (int64_t i = 0; i < count; ++i)
                values[static_cast<size_t> (i)] = static_cast<uint32_t> (i * 2654435761u);
        
        for (auto _ : state)
        {
                memcpy (pAligned, values.data (), totalBytes);
                struct R_CSTL_Array* pArray = R_CSTL_NewArrayWithData (
                    reinterpret_cast<const uint8_t*> (pAligned), totalBytes);
                if (!pArray)
                {
                        state.SkipWithError ("NewArrayWithData failed");
                        R_CSTL_HeapFree (pAligned);
                        return;
                }
                
                R_CSTL_ArraySort (pArray, sizeof (uint32_t), R_CSTL_ArrayCompareU32, nullptr);
                benchmark::DoNotOptimize (pArray);
                R_CSTL_DeleteArray (pArray);
        }
        
        R_CSTL_HeapFree (pAligned);
        state.SetItemsProcessed (state.iterations () * count);
}
BENCHMARK_REGISTER_F (ArrayBenchFixture, SortU32Aligned)
    ->Arg (256)
    ->Arg (4096)
    ->Arg (8192)
    ->Arg (16384)
    ->Arg (2097152)
    ->Unit (benchmark::kMicrosecond);

BENCHMARK_DEFINE_F (ArrayBenchFixture, SortU32Unaligned) (benchmark::State& state)
{
        if (!BenchEnsureHeap (state, kBenchHeapSize))
                return;

        const int64_t count = state.range (0);
        std::vector<uint32_t> values (static_cast<size_t> (count));
        for (int64_t i = 0; i < count; ++i)
                values[static_cast<size_t> (i)] = static_cast<uint32_t> (i * 2654435761u);

        for (auto _ : state)
        {
                struct R_CSTL_Array* pArray = R_CSTL_NewArrayWithData (
                    reinterpret_cast<const uint8_t*> (values.data ()),
                    values.size () * sizeof (uint32_t));
                if (!pArray)
                {
                        state.SkipWithError ("NewArrayWithData failed");
                        return;
                }

                R_CSTL_ArraySort (pArray, sizeof (uint32_t), R_CSTL_ArrayCompareU32, nullptr);
                benchmark::DoNotOptimize (pArray);
                R_CSTL_DeleteArray (pArray);
        }

        state.SetItemsProcessed (state.iterations () * count);
}
BENCHMARK_REGISTER_F (ArrayBenchFixture, SortU32Unaligned)
    ->Arg (256)
    ->Arg (4096)
    ->Arg (8192)
    ->Arg (16384)
    ->Arg (2097152)
    ->Unit (benchmark::kMicrosecond);

BENCHMARK_DEFINE_F (ArrayBenchFixture, StdSortU32) (benchmark::State& state)
{
        const int64_t count = state.range (0);
        std::vector<uint32_t> values (static_cast<size_t> (count));
        for (int64_t i = 0; i < count; ++i)
                values[static_cast<size_t> (i)] = static_cast<uint32_t> (i * 2654435761u);

        for (auto _ : state)
        {
                std::vector<uint32_t> copy = values;
                std::sort (copy.begin (), copy.end ());
                benchmark::DoNotOptimize (copy.data ());
        }

        state.SetItemsProcessed (state.iterations () * count);
}
BENCHMARK_REGISTER_F (ArrayBenchFixture, StdSortU32)
    ->Arg (256)
    ->Arg (4096)
    ->Arg (8192)
    ->Arg (16384)
    ->Arg (2097152)
    ->Unit (benchmark::kMicrosecond);

BENCHMARK_DEFINE_F (ArrayBenchFixture, StdSortU8) (benchmark::State& state)
{
        const int64_t count = state.range (0);
        std::vector<uint8_t> values (static_cast<size_t> (count));
        for (int64_t i = 0; i < count; ++i)
                values[static_cast<size_t> (i)] = static_cast<uint8_t> ((i * 73) & 0xFF);

        for (auto _ : state)
        {
                std::vector<uint8_t> copy = values;
                std::sort (copy.begin (), copy.end ());
                benchmark::DoNotOptimize (copy.data ());
        }

        state.SetItemsProcessed (state.iterations () * count);
}
BENCHMARK_REGISTER_F (ArrayBenchFixture, StdSortU8)
    ->Arg (64)
    ->Arg (1024)
    ->Arg (4096)
    ->Arg (8192)
    ->Unit (benchmark::kMicrosecond);
