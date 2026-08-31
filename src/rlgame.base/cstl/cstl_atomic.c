#include "rlgame.base/cstl/cstl_atomic.h"

#define R_CSTL_INLINE
#include "rlgame.base/cstl/cstl_platform.h"

#if defined(R_CSTL_PLATFORM_WINDOWS)

R_CSTL_API int
r_cstl_atomic_int32_increment (r_cstl_atomic_int32* pAtomic)
{
    return InterlockedIncrement (pAtomic);
}

R_CSTL_API int
r_cstl_atomic_int32_decrement (r_cstl_atomic_int32* pAtomic)
{
    return InterlockedDecrement (pAtomic);
}

R_CSTL_API int
r_cstl_atomic_int32_compare_exchange (r_cstl_atomic_int32* pAtomic, int expected, int desired)
{
    return InterlockedCompareExchange (pAtomic, desired, expected);
}

R_CSTL_API int
r_cstl_atomic_int32_exchange (r_cstl_atomic_int32* pAtomic, int value)
{
    return InterlockedExchange (pAtomic, value);
}

R_CSTL_API int
r_cstl_atomic_int32_load (const r_cstl_atomic_int32* pAtomic)
{
    return *pAtomic;
}

R_CSTL_API void
r_cstl_atomic_int32_store (r_cstl_atomic_int32* pAtomic, int value)
{
    *pAtomic = value;
}

R_CSTL_API int
r_cstl_atomic_int32_add (r_cstl_atomic_int32* pAtomic, int value)
{
    return InterlockedAdd (pAtomic, value);
}

R_CSTL_API uint32_t
r_cstl_atomic_uint32_increment (r_cstl_atomic_uint32* pAtomic)
{
    return (uint32_t)InterlockedIncrement ((LONG*)pAtomic);
}

R_CSTL_API uint32_t
r_cstl_atomic_uint32_decrement (r_cstl_atomic_uint32* pAtomic)
{
    return (uint32_t)InterlockedDecrement ((LONG*)pAtomic);
}

R_CSTL_API uint32_t
r_cstl_atomic_uint32_compare_exchange (r_cstl_atomic_uint32* pAtomic, uint32_t expected, uint32_t desired)
{
    return (uint32_t)InterlockedCompareExchange ((LONG*)pAtomic, (LONG)desired, (LONG)expected);
}

R_CSTL_API uint32_t
r_cstl_atomic_uint32_exchange (r_cstl_atomic_uint32* pAtomic, uint32_t value)
{
    return (uint32_t)InterlockedExchange ((LONG*)pAtomic, (LONG)value);
}

R_CSTL_API uint32_t
r_cstl_atomic_uint32_load (const r_cstl_atomic_uint32* pAtomic)
{
    return *pAtomic;
}

R_CSTL_API void
r_cstl_atomic_uint32_store (r_cstl_atomic_uint32* pAtomic, uint32_t value)
{
    *pAtomic = value;
}

R_CSTL_API uint32_t
r_cstl_atomic_uint32_add (r_cstl_atomic_uint32* pAtomic, uint32_t value)
{
    return (uint32_t)InterlockedAdd ((LONG*)pAtomic, (LONG)value);
}

R_CSTL_API int64_t
r_cstl_atomic_int64_increment (r_cstl_atomic_int64* pAtomic)
{
    return InterlockedIncrement64 (pAtomic);
}

R_CSTL_API int64_t
r_cstl_atomic_int64_decrement (r_cstl_atomic_int64* pAtomic)
{
    return InterlockedDecrement64 (pAtomic);
}

R_CSTL_API int64_t
r_cstl_atomic_int64_compare_exchange (r_cstl_atomic_int64* pAtomic, int64_t expected, int64_t desired)
{
    return InterlockedCompareExchange64 (pAtomic, desired, expected);
}

R_CSTL_API int64_t
r_cstl_atomic_int64_exchange (r_cstl_atomic_int64* pAtomic, int64_t value)
{
    return InterlockedExchange64 (pAtomic, value);
}

R_CSTL_API int64_t
r_cstl_atomic_int64_load (const r_cstl_atomic_int64* pAtomic)
{
    return *pAtomic;
}

R_CSTL_API void
r_cstl_atomic_int64_store (r_cstl_atomic_int64* pAtomic, int64_t value)
{
    *pAtomic = value;
}

R_CSTL_API int64_t
r_cstl_atomic_int64_add (r_cstl_atomic_int64* pAtomic, int64_t value)
{
    return InterlockedAdd64 (pAtomic, value);
}

R_CSTL_API uint64_t
r_cstl_atomic_uint64_increment (r_cstl_atomic_uint64* pAtomic)
{
    return (uint64_t)InterlockedIncrement64 ((LONGLONG*)pAtomic);
}

R_CSTL_API uint64_t
r_cstl_atomic_uint64_decrement (r_cstl_atomic_uint64* pAtomic)
{
    return (uint64_t)InterlockedDecrement64 ((LONGLONG*)pAtomic);
}

R_CSTL_API uint64_t
r_cstl_atomic_uint64_compare_exchange (r_cstl_atomic_uint64* pAtomic, uint64_t expected, uint64_t desired)
{
    return (uint64_t)InterlockedCompareExchange64 ((LONGLONG*)pAtomic, (LONGLONG)desired, (LONGLONG)expected);
}

R_CSTL_API uint64_t
r_cstl_atomic_uint64_exchange (r_cstl_atomic_uint64* pAtomic, uint64_t value)
{
    return (uint64_t)InterlockedExchange64 ((LONGLONG*)pAtomic, (LONGLONG)value);
}

R_CSTL_API uint64_t
r_cstl_atomic_uint64_load (const r_cstl_atomic_uint64* pAtomic)
{
    return *pAtomic;
}

R_CSTL_API void
r_cstl_atomic_uint64_store (r_cstl_atomic_uint64* pAtomic, uint64_t value)
{
    *pAtomic = value;
}

R_CSTL_API uint64_t
r_cstl_atomic_uint64_add (r_cstl_atomic_uint64* pAtomic, uint64_t value)
{
    return (uint64_t)InterlockedAdd64 ((LONGLONG*)pAtomic, (LONGLONG)value);
}

R_CSTL_API void*
r_cstl_atomic_ptr_compare_exchange (r_cstl_atomic_voidP* pAtomic, void* expected, void* desired)
{
    return InterlockedCompareExchangePointer (pAtomic, desired, expected);
}

R_CSTL_API void*
r_cstl_atomic_ptr_exchange (r_cstl_atomic_voidP* pAtomic, void* value)
{
    return InterlockedExchangePointer (pAtomic, value);
}

R_CSTL_API void*
r_cstl_atomic_ptr_load (const r_cstl_atomic_voidP* pAtomic)
{
    return *pAtomic;
}

R_CSTL_API void
r_cstl_atomic_ptr_store (r_cstl_atomic_voidP* pAtomic, void* value)
{
    *pAtomic = value;
}

#elif defined(R_CSTL_PLATFORM_LINUX)

R_CSTL_API int
r_cstl_atomic_int32_increment (r_cstl_atomic_int32* pAtomic)
{
    return atomic_fetch_add_explicit (pAtomic, 1, memory_order_relaxed) + 1;
}

R_CSTL_API int
r_cstl_atomic_int32_decrement (r_cstl_atomic_int32* pAtomic)
{
    return atomic_fetch_sub_explicit (pAtomic, 1, memory_order_relaxed) - 1;
}

R_CSTL_API int
r_cstl_atomic_int32_compare_exchange (r_cstl_atomic_int32* pAtomic, int expected, int desired)
{
    atomic_compare_exchange_strong_explicit (
        pAtomic,
        &expected,
        desired,
        memory_order_seq_cst,
        memory_order_seq_cst);
    return expected;
}

R_CSTL_API int
r_cstl_atomic_int32_exchange (r_cstl_atomic_int32* pAtomic, int value)
{
    return atomic_exchange_explicit (pAtomic, value, memory_order_seq_cst);
}

R_CSTL_API int
r_cstl_atomic_int32_load (const r_cstl_atomic_int32* pAtomic)
{
    return atomic_load_explicit (pAtomic, memory_order_acquire);
}

R_CSTL_API void
r_cstl_atomic_int32_store (r_cstl_atomic_int32* pAtomic, int value)
{
    atomic_store_explicit (pAtomic, value, memory_order_release);
}

R_CSTL_API int
r_cstl_atomic_int32_add (r_cstl_atomic_int32* pAtomic, int value)
{
    return atomic_fetch_add_explicit (pAtomic, value, memory_order_relaxed) + value;
}

R_CSTL_API uint32_t
r_cstl_atomic_uint32_increment (r_cstl_atomic_uint32* pAtomic)
{
    return atomic_fetch_add_explicit (pAtomic, 1u, memory_order_relaxed) + 1u;
}

R_CSTL_API uint32_t
r_cstl_atomic_uint32_decrement (r_cstl_atomic_uint32* pAtomic)
{
    return atomic_fetch_sub_explicit (pAtomic, 1u, memory_order_relaxed) - 1u;
}

R_CSTL_API uint32_t
r_cstl_atomic_uint32_compare_exchange (r_cstl_atomic_uint32* pAtomic, uint32_t expected, uint32_t desired)
{
    atomic_compare_exchange_strong_explicit (
        pAtomic,
        &expected,
        desired,
        memory_order_seq_cst,
        memory_order_seq_cst);
    return expected;
}

R_CSTL_API uint32_t
r_cstl_atomic_uint32_exchange (r_cstl_atomic_uint32* pAtomic, uint32_t value)
{
    return atomic_exchange_explicit (pAtomic, value, memory_order_seq_cst);
}

R_CSTL_API uint32_t
r_cstl_atomic_uint32_load (const r_cstl_atomic_uint32* pAtomic)
{
    return atomic_load_explicit (pAtomic, memory_order_acquire);
}

R_CSTL_API void
r_cstl_atomic_uint32_store (r_cstl_atomic_uint32* pAtomic, uint32_t value)
{
    atomic_store_explicit (pAtomic, value, memory_order_release);
}

R_CSTL_API uint32_t
r_cstl_atomic_uint32_add (r_cstl_atomic_uint32* pAtomic, uint32_t value)
{
    return atomic_fetch_add_explicit (pAtomic, value, memory_order_relaxed) + value;
}

R_CSTL_API int64_t
r_cstl_atomic_int64_increment (r_cstl_atomic_int64* pAtomic)
{
    return atomic_fetch_add_explicit (pAtomic, 1L, memory_order_relaxed) + 1L;
}

R_CSTL_API int64_t
r_cstl_atomic_int64_decrement (r_cstl_atomic_int64* pAtomic)
{
    return atomic_fetch_sub_explicit (pAtomic, 1L, memory_order_relaxed) - 1L;
}

R_CSTL_API int64_t
r_cstl_atomic_int64_compare_exchange (r_cstl_atomic_int64* pAtomic, int64_t expected, int64_t desired)
{
    atomic_compare_exchange_strong_explicit (
        pAtomic,
        &expected,
        desired,
        memory_order_seq_cst,
        memory_order_seq_cst);
    return expected;
}

R_CSTL_API int64_t
r_cstl_atomic_int64_exchange (r_cstl_atomic_int64* pAtomic, int64_t value)
{
    return atomic_exchange_explicit (pAtomic, value, memory_order_seq_cst);
}

R_CSTL_API int64_t
r_cstl_atomic_int64_load (const r_cstl_atomic_int64* pAtomic)
{
    return atomic_load_explicit (pAtomic, memory_order_acquire);
}

R_CSTL_API void
r_cstl_atomic_int64_store (r_cstl_atomic_int64* pAtomic, int64_t value)
{
    atomic_store_explicit (pAtomic, value, memory_order_release);
}

R_CSTL_API int64_t
r_cstl_atomic_int64_add (r_cstl_atomic_int64* pAtomic, int64_t value)
{
    return atomic_fetch_add_explicit (pAtomic, value, memory_order_relaxed) + value;
}

R_CSTL_API uint64_t
r_cstl_atomic_uint64_increment (r_cstl_atomic_uint64* pAtomic)
{
    return atomic_fetch_add_explicit (pAtomic, 1UL, memory_order_relaxed) + 1UL;
}

R_CSTL_API uint64_t
r_cstl_atomic_uint64_decrement (r_cstl_atomic_uint64* pAtomic)
{
    return atomic_fetch_sub_explicit (pAtomic, 1UL, memory_order_relaxed) - 1UL;
}

R_CSTL_API uint64_t
r_cstl_atomic_uint64_compare_exchange (r_cstl_atomic_uint64* pAtomic, uint64_t expected, uint64_t desired)
{
    atomic_compare_exchange_strong_explicit (
        pAtomic,
        &expected,
        desired,
        memory_order_seq_cst,
        memory_order_seq_cst);
    return expected;
}

R_CSTL_API uint64_t
r_cstl_atomic_uint64_exchange (r_cstl_atomic_uint64* pAtomic, uint64_t value)
{
    return atomic_exchange_explicit (pAtomic, value, memory_order_seq_cst);
}

R_CSTL_API uint64_t
r_cstl_atomic_uint64_load (const r_cstl_atomic_uint64* pAtomic)
{
    return atomic_load_explicit (pAtomic, memory_order_acquire);
}

R_CSTL_API void
r_cstl_atomic_uint64_store (r_cstl_atomic_uint64* pAtomic, uint64_t value)
{
    atomic_store_explicit (pAtomic, value, memory_order_release);
}

R_CSTL_API uint64_t
r_cstl_atomic_uint64_add (r_cstl_atomic_uint64* pAtomic, uint64_t value)
{
    return atomic_fetch_add_explicit (pAtomic, value, memory_order_relaxed) + value;
}

R_CSTL_API void*
r_cstl_atomic_ptr_compare_exchange (r_cstl_atomic_voidP* pAtomic, void* expected, void* desired)
{
    atomic_compare_exchange_strong_explicit (
        pAtomic,
        &expected,
        desired,
        memory_order_seq_cst,
        memory_order_seq_cst);
    return expected;
}

R_CSTL_API void*
r_cstl_atomic_ptr_exchange (r_cstl_atomic_voidP* pAtomic, void* value)
{
    return atomic_exchange_explicit (pAtomic, value, memory_order_seq_cst);
}

R_CSTL_API void*
r_cstl_atomic_ptr_load (const r_cstl_atomic_voidP* pAtomic)
{
    return atomic_load_explicit (pAtomic, memory_order_acquire);
}

R_CSTL_API void
r_cstl_atomic_ptr_store (r_cstl_atomic_voidP* pAtomic, void* value)
{
    atomic_store_explicit (pAtomic, value, memory_order_release);
}

#endif