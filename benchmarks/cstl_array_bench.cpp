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
        { r_cstl_heap_shutdown (); }
};

} // namespace

BENCHMARK_DEFINE_F (ArrayBenchFixture, PushWithoutReserve) (benchmark::State& state)
{
        if (!BenchEnsureHeap (state, kBenchHeapSize))
                return;

        const int64_t count = state.range (0);

        for (auto _ : state)
        {
                struct r_cstl_array* pArray = r_cstl_new_array ();
                if (!pArray)
                {
                        state.SkipWithError ("NewArray failed");
                        return;
                }

                for (int64_t i = 0; i < count; ++i)
                        r_cstl_array_push (pArray, static_cast<uint8_t> (i & 0xFF));

                benchmark::DoNotOptimize (pArray);
                r_cstl_delete_array (pArray);
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
                struct r_cstl_array* pArray = r_cstl_new_array_with_capacity (static_cast<size_t> (count));
                if (!pArray)
                {
                        state.SkipWithError ("NewArrayWithCapacity failed");
                        return;
                }

                for (int64_t i = 0; i < count; ++i)
                        r_cstl_array_push (pArray, static_cast<uint8_t> (i & 0xFF));

                benchmark::DoNotOptimize (pArray);
                r_cstl_delete_array (pArray);
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
        struct r_cstl_array* pArray = r_cstl_new_array_with_capacity (static_cast<size_t> (count));
        if (!pArray)
        {
                state.SkipWithError ("setup failed");
                return;
        }
        for (int64_t i = 0; i < count; ++i)
                r_cstl_array_push (pArray, static_cast<uint8_t> (i & 0xFF));

        for (auto _ : state)
        {
                state.PauseTiming ();
                for (int64_t i = 0; i < count; ++i)
                        r_cstl_array_push (pArray, static_cast<uint8_t> (i & 0xFF));
                state.ResumeTiming ();

                for (int64_t i = 0; i < count; ++i)
                        r_cstl_array_pop (pArray, nullptr);

                benchmark::DoNotOptimize (pArray);
        }

        r_cstl_delete_array (pArray);
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
                struct r_cstl_array* pArray = r_cstl_new_array_with_capacity (static_cast<size_t> (count));
                if (!pArray)
                {
                        state.SkipWithError ("setup failed");
                        return;
                }
                for (int64_t i = 0; i < count; ++i)
                        r_cstl_array_push (pArray, static_cast<uint8_t> (i & 0xFF));

                for (int64_t i = 0; i < count; ++i)
                        r_cstl_array_shift (pArray, nullptr);

                benchmark::DoNotOptimize (pArray);
                r_cstl_delete_array (pArray);
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
                struct r_cstl_array* pArray =
                    r_cstl_new_array_with_data (values.data (), values.size ());
                if (!pArray)
                {
                        state.SkipWithError ("NewArrayWithData failed");
                        return;
                }

                r_cstl_array_sort (pArray, sizeof (uint8_t), r_cstl_array_compare_u8, nullptr);
                benchmark::DoNotOptimize (pArray);
                r_cstl_delete_array (pArray);
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
                struct r_cstl_array* pArray = r_cstl_new_array_with_data (
                    reinterpret_cast<const uint8_t*> (values.data ()),
                    values.size () * sizeof (uint32_t));
                if (!pArray)
                {
                        state.SkipWithError ("NewArrayWithData failed");
                        return;
                }

                r_cstl_array_sort (pArray, sizeof (uint32_t), r_cstl_array_compare_u32, nullptr);
                benchmark::DoNotOptimize (pArray);
                r_cstl_delete_array (pArray);
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
        struct r_cstl_array* pSource = r_cstl_new_array_with_data (values.data (), values.size ());
        if (!pSource)
        {
                state.SkipWithError ("setup failed");
                return;
        }

        for (auto _ : state)
        {
                struct r_cstl_array* pSlice = r_cstl_array_slice (pSource, 0, static_cast<size_t> (count));
                benchmark::DoNotOptimize (pSlice);
                r_cstl_delete_array (pSlice);
        }

        r_cstl_delete_array (pSource);
        state.SetItemsProcessed (state.iterations () * count);
}
BENCHMARK_REGISTER_F (ArrayBenchFixture, Slice)->Arg (4096)->Arg (65536)->Unit (benchmark::kMicrosecond);

BENCHMARK_DEFINE_F (ArrayBenchFixture, UncheckedAt) (benchmark::State& state)
{
        if (!BenchEnsureHeap (state, kBenchHeapSize))
                return;

        const int64_t count = state.range (0);
        struct r_cstl_array* pArray = r_cstl_new_array_with_capacity (static_cast<size_t> (count));
        if (!pArray)
        {
                state.SkipWithError ("setup failed");
                return;
        }
        for (int64_t i = 0; i < count; ++i)
                r_cstl_array_push (pArray, static_cast<uint8_t> (i & 0xFF));

        uint64_t checksum = 0;
        for (auto _ : state)
        {
                for (int64_t i = 0; i < count; ++i)
                {
                        uint8_t value = 0;
                        r_cstl_array_unchecked_at (pArray, static_cast<size_t> (i), &value);
                        checksum += value;
                }
                benchmark::DoNotOptimize (checksum);
        }

        r_cstl_delete_array (pArray);
        state.SetItemsProcessed (state.iterations () * count);
}
BENCHMARK_REGISTER_F (ArrayBenchFixture, UncheckedAt)->Arg (1024)->Arg (8192)->Unit (benchmark::kMicrosecond);

BENCHMARK_DEFINE_F (ArrayBenchFixture, SortU32Aligned) (benchmark::State& state)
{
        if (!BenchEnsureHeap (state, kBenchHeapSize))
                return;

        const int64_t count = state.range (0);
        const size_t totalBytes = static_cast<size_t> (count) * sizeof (uint32_t);
        
        uint32_t* pAligned = static_cast<uint32_t*> (r_cstl_heap_alloc_aligned (totalBytes, 32));
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
                struct r_cstl_array* pArray = r_cstl_new_array_with_data (
                    reinterpret_cast<const uint8_t*> (pAligned), totalBytes);
                if (!pArray)
                {
                        state.SkipWithError ("NewArrayWithData failed");
                        r_cstl_heap_free (pAligned);
                        return;
                }
                
                r_cstl_array_sort (pArray, sizeof (uint32_t), r_cstl_array_compare_u32, nullptr);
                benchmark::DoNotOptimize (pArray);
                r_cstl_delete_array (pArray);
        }
        
        r_cstl_heap_free (pAligned);
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
                struct r_cstl_array* pArray = r_cstl_new_array_with_data (
                    reinterpret_cast<const uint8_t*> (values.data ()),
                    values.size () * sizeof (uint32_t));
                if (!pArray)
                {
                        state.SkipWithError ("NewArrayWithData failed");
                        return;
                }

                r_cstl_array_sort (pArray, sizeof (uint32_t), r_cstl_array_compare_u32, nullptr);
                benchmark::DoNotOptimize (pArray);
                r_cstl_delete_array (pArray);
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
