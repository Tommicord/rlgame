#pragma once

#include <benchmark/benchmark.h>
#include <cstddef>

extern "C"
{
#include "rlgame.base/cstl/cstl_heap_allocator.h"
}

inline bool
BenchEnsureHeap (::benchmark::State& state, size_t heapBytes)
{
        if (r_cstl_heap_init (heapBytes) != 0)
        {
                state.SkipWithError ("r_cstl_heap_init failed");
                return false;
        }

        void* pProbe = r_cstl_heap_alloc (64);
        if (!pProbe)
        {
                r_cstl_heap_shutdown ();
                state.SkipWithError ("r_cstl_heap_alloc probe failed (heap not ready)");
                return false;
        }

        benchmark::DoNotOptimize (pProbe);
        r_cstl_heap_free (pProbe);
        return true;
}
