#include <benchmark/benchmark.h>

#include <cstdint>
#include <vector>

#include "cstl_bench_common.h"

namespace {

constexpr size_t kBenchHeapSize = 8u * 1024u * 1024u;

class HeapBenchFixture : public benchmark::Fixture
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

BENCHMARK_DEFINE_F (HeapBenchFixture, AllocFree64) (benchmark::State& state)
{
        if (!BenchEnsureHeap (state, kBenchHeapSize))
                return;

        for (auto _ : state)
        {
                void* p = r_cstl_heap_alloc (64);
                benchmark::DoNotOptimize (p);
                if (!p)
                {
                        state.SkipWithError ("HeapAlloc returned null");
                        return;
                }
                r_cstl_heap_free (p);
        }
}
BENCHMARK_REGISTER_F (HeapBenchFixture, AllocFree64)->Unit (benchmark::kNanosecond);

BENCHMARK_DEFINE_F (HeapBenchFixture, AllocFree256) (benchmark::State& state)
{
        if (!BenchEnsureHeap (state, kBenchHeapSize))
                return;

        for (auto _ : state)
        {
                void* p = r_cstl_heap_alloc (256);
                benchmark::DoNotOptimize (p);
                if (!p)
                {
                        state.SkipWithError ("HeapAlloc returned null");
                        return;
                }
                r_cstl_heap_free (p);
        }
}
BENCHMARK_REGISTER_F (HeapBenchFixture, AllocFree256)->Unit (benchmark::kNanosecond);

BENCHMARK_DEFINE_F (HeapBenchFixture, AllocFree4096) (benchmark::State& state)
{
        if (!BenchEnsureHeap (state, kBenchHeapSize))
                return;

        for (auto _ : state)
        {
                void* p = r_cstl_heap_alloc (4096);
                benchmark::DoNotOptimize (p);
                if (!p)
                {
                        state.SkipWithError ("HeapAlloc returned null");
                        return;
                }
                r_cstl_heap_free (p);
        }
}
BENCHMARK_REGISTER_F (HeapBenchFixture, AllocFree4096)->Unit (benchmark::kNanosecond);

BENCHMARK_DEFINE_F (HeapBenchFixture, ReallocGrowInPlace) (benchmark::State& state)
{
        if (!BenchEnsureHeap (state, kBenchHeapSize))
                return;

        void* p = r_cstl_heap_alloc (64);
        if (!p)
        {
                state.SkipWithError ("initial alloc failed");
                return;
        }

        for (auto _ : state)
        {
                void* q = r_cstl_heap_realloc (p, 96);
                benchmark::DoNotOptimize (q);
                if (!q)
                {
                        state.SkipWithError ("realloc failed");
                        r_cstl_heap_free (p);
                        return;
                }
                p = q;
                benchmark::ClobberMemory ();
        }

        r_cstl_heap_free (p);
}
BENCHMARK_REGISTER_F (HeapBenchFixture, ReallocGrowInPlace)->Unit (benchmark::kNanosecond);

BENCHMARK_DEFINE_F (HeapBenchFixture, ReallocGrowCopy) (benchmark::State& state)
{
        if (!BenchEnsureHeap (state, kBenchHeapSize))
                return;

        void* p = r_cstl_heap_alloc (64);
        if (!p)
        {
                state.SkipWithError ("initial alloc failed");
                return;
        }

        for (auto _ : state)
        {
                void* q = r_cstl_heap_realloc (p, 4096);
                benchmark::DoNotOptimize (q);
                if (!q)
                {
                        state.SkipWithError ("realloc failed");
                        r_cstl_heap_free (p);
                        return;
                }
                p = q;
                benchmark::ClobberMemory ();
        }

        r_cstl_heap_free (p);
}
BENCHMARK_REGISTER_F (HeapBenchFixture, ReallocGrowCopy)->Unit (benchmark::kNanosecond);

BENCHMARK_DEFINE_F (HeapBenchFixture, AllocManyFreeMany) (benchmark::State& state)
{
        if (!BenchEnsureHeap (state, kBenchHeapSize))
                return;

        const int64_t count = state.range (0);
        std::vector<void*> ptrs (static_cast<size_t> (count));

        for (auto _ : state)
        {
                for (int64_t i = 0; i < count; ++i)
                {
                        ptrs[static_cast<size_t> (i)] =
                            r_cstl_heap_alloc (static_cast<size_t> (32 + (i % 512)));
                        benchmark::DoNotOptimize (ptrs[static_cast<size_t> (i)]);
                }
                for (int64_t i = 0; i < count; ++i)
                        r_cstl_heap_free (ptrs[static_cast<size_t> (i)]);
        }

        state.SetItemsProcessed (state.iterations () * count * 2);
}
BENCHMARK_REGISTER_F (HeapBenchFixture, AllocManyFreeMany)
    ->Arg (64)
    ->Arg (256)
    ->Arg (1024)
    ->Unit (benchmark::kMicrosecond);

BENCHMARK_DEFINE_F (HeapBenchFixture, FragmentationChurn) (benchmark::State& state)
{
        if (!BenchEnsureHeap (state, kBenchHeapSize))
                return;

        const int64_t slots = state.range (0);
        std::vector<void*> live (static_cast<size_t> (slots));

        for (auto _ : state)
        {
                for (int64_t i = 0; i < slots; ++i)
                {
                        if (live[static_cast<size_t> (i)])
                        {
                                r_cstl_heap_free (live[static_cast<size_t> (i)]);
                                live[static_cast<size_t> (i)] = nullptr;
                        }
                        else
                        {
                                live[static_cast<size_t> (i)] =
                                    r_cstl_heap_alloc (static_cast<size_t> (64 + (i % 7) * 64));
                                benchmark::DoNotOptimize (live[static_cast<size_t> (i)]);
                        }
                }
                for (void* p : live)
                {
                        if (p)
                                r_cstl_heap_free (p);
                }
                live.assign (static_cast<size_t> (slots), nullptr);
        }

        state.SetItemsProcessed (state.iterations () * slots);
}
BENCHMARK_REGISTER_F (HeapBenchFixture, FragmentationChurn)
    ->Arg (128)
    ->Arg (512)
    ->Unit (benchmark::kMicrosecond);

BENCHMARK_DEFINE_F (HeapBenchFixture, IsValidPointer) (benchmark::State& state)
{
        if (!BenchEnsureHeap (state, kBenchHeapSize))
                return;

        void* p = r_cstl_heap_alloc (128);
        if (!p)
        {
                state.SkipWithError ("alloc failed");
                return;
        }

        for (auto _ : state)
        {
                const int live = r_cstl_heap_is_valid_pointer (p);
                benchmark::DoNotOptimize (live);
        }

        r_cstl_heap_free (p);
}
BENCHMARK_REGISTER_F (HeapBenchFixture, IsValidPointer)->Unit (benchmark::kNanosecond);
