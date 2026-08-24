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
        if (R_CSTL_HeapInit (heapBytes) != 0)
        {
                state.SkipWithError ("R_CSTL_HeapInit failed");
                return false;
        }

        void* pProbe = R_CSTL_HeapAlloc (64);
        if (!pProbe)
        {
                R_CSTL_HeapShutdown ();
                state.SkipWithError ("R_CSTL_HeapAlloc probe failed (heap not ready)");
                return false;
        }

        benchmark::DoNotOptimize (pProbe);
        R_CSTL_HeapFree (pProbe);
        return true;
}
