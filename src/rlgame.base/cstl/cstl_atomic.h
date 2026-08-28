#pragma once

#include "rlgame.base/cstl/cstl_platform.h"

#include <stddef.h>
#include <stdint.h>

#if defined(R_CSTL_PLATFORM_WINDOWS)
#include <windows.h>
#elif defined(R_CSTL_PLATFORM_LINUX)
#include <stdatomic.h>
#else
#error "Unsupported platform for cstl_atomic"
#endif

#if defined(R_CSTL_PLATFORM_WINDOWS)

typedef volatile LONG R_CSTL_AtomicInt32;
typedef volatile ULONG R_CSTL_AtomicUint32;
typedef volatile LONGLONG R_CSTL_AtomicInt64;
typedef volatile ULONGLONG R_CSTL_AtomicUint64;
typedef volatile PVOID R_CSTL_AtomicPtr;

#define R_CSTL_ATOMIC_INT32_INIT(value) (value)
#define R_CSTL_ATOMIC_UINT32_INIT(value) (value)
#define R_CSTL_ATOMIC_INT64_INIT(value) (value)
#define R_CSTL_ATOMIC_UINT64_INIT(value) (value)
#define R_CSTL_ATOMIC_PTR_INIT(value) (value)

R_CSTL_API int
R_CSTL_AtomicInt32Increment (R_CSTL_AtomicInt32* pAtomic);

R_CSTL_API int
R_CSTL_AtomicInt32Decrement (R_CSTL_AtomicInt32* pAtomic);

R_CSTL_API int
R_CSTL_AtomicInt32CompareExchange (R_CSTL_AtomicInt32* pAtomic, int expected, int desired);

R_CSTL_API int
R_CSTL_AtomicInt32Exchange (R_CSTL_AtomicInt32* pAtomic, int value);

R_CSTL_API int
R_CSTL_AtomicInt32Load (const R_CSTL_AtomicInt32* pAtomic);

R_CSTL_API void
R_CSTL_AtomicInt32Store (R_CSTL_AtomicInt32* pAtomic, int value);

R_CSTL_API int
R_CSTL_AtomicInt32Add (R_CSTL_AtomicInt32* pAtomic, int value);

R_CSTL_API uint32_t
R_CSTL_AtomicUint32Increment (R_CSTL_AtomicUint32* pAtomic);

R_CSTL_API uint32_t
R_CSTL_AtomicUint32Decrement (R_CSTL_AtomicUint32* pAtomic);

R_CSTL_API uint32_t
R_CSTL_AtomicUint32CompareExchange (R_CSTL_AtomicUint32* pAtomic, uint32_t expected, uint32_t desired);

R_CSTL_API uint32_t
R_CSTL_AtomicUint32Exchange (R_CSTL_AtomicUint32* pAtomic, uint32_t value);

R_CSTL_API uint32_t
R_CSTL_AtomicUint32Load (const R_CSTL_AtomicUint32* pAtomic);

R_CSTL_API void
R_CSTL_AtomicUint32Store (R_CSTL_AtomicUint32* pAtomic, uint32_t value);

R_CSTL_API uint32_t
R_CSTL_AtomicUint32Add (R_CSTL_AtomicUint32* pAtomic, uint32_t value);

R_CSTL_API int64_t
R_CSTL_AtomicInt64Increment (R_CSTL_AtomicInt64* pAtomic);

R_CSTL_API int64_t
R_CSTL_AtomicInt64Decrement (R_CSTL_AtomicInt64* pAtomic);

R_CSTL_API int64_t
R_CSTL_AtomicInt64CompareExchange (R_CSTL_AtomicInt64* pAtomic, int64_t expected, int64_t desired);

R_CSTL_API int64_t
R_CSTL_AtomicInt64Exchange (R_CSTL_AtomicInt64* pAtomic, int64_t value);

R_CSTL_API int64_t
R_CSTL_AtomicInt64Load (const R_CSTL_AtomicInt64* pAtomic);

R_CSTL_API void
R_CSTL_AtomicInt64Store (R_CSTL_AtomicInt64* pAtomic, int64_t value);

R_CSTL_API int64_t
R_CSTL_AtomicInt64Add (R_CSTL_AtomicInt64* pAtomic, int64_t value);

R_CSTL_API uint64_t
R_CSTL_AtomicUint64Increment (R_CSTL_AtomicUint64* pAtomic);

R_CSTL_API uint64_t
R_CSTL_AtomicUint64Decrement (R_CSTL_AtomicUint64* pAtomic);

R_CSTL_API uint64_t
R_CSTL_AtomicUint64CompareExchange (R_CSTL_AtomicUint64* pAtomic, uint64_t expected, uint64_t desired);

R_CSTL_API uint64_t
R_CSTL_AtomicUint64Exchange (R_CSTL_AtomicUint64* pAtomic, uint64_t value);

R_CSTL_API uint64_t
R_CSTL_AtomicUint64Load (const R_CSTL_AtomicUint64* pAtomic);

R_CSTL_API void
R_CSTL_AtomicUint64Store (R_CSTL_AtomicUint64* pAtomic, uint64_t value);

R_CSTL_API uint64_t
R_CSTL_AtomicUint64Add (R_CSTL_AtomicUint64* pAtomic, uint64_t value);

R_CSTL_API void*
R_CSTL_AtomicPtrCompareExchange (R_CSTL_AtomicPtr* pAtomic, void* expected, void* desired);

R_CSTL_API void*
R_CSTL_AtomicPtrExchange (R_CSTL_AtomicPtr* pAtomic, void* value);

R_CSTL_API void*
R_CSTL_AtomicPtrLoad (const R_CSTL_AtomicPtr* pAtomic);

R_CSTL_API void
R_CSTL_AtomicPtrStore (R_CSTL_AtomicPtr* pAtomic, void* value);

#elif defined(R_CSTL_PLATFORM_LINUX)

typedef volatile int32_t  _Atomic R_CSTL_AtomicInt32;
typedef volatile uint32_t _Atomic R_CSTL_AtomicUint32;
typedef volatile int64_t  _Atomic R_CSTL_AtomicInt64;
typedef volatile uint64_t _Atomic R_CSTL_AtomicUint64;
typedef volatile void*    _Atomic R_CSTL_AtomicVoidP;

#define R_CSTL_ATOMIC_INT32_INIT(value) ATOMIC_VAR_INIT(value)
#define R_CSTL_ATOMIC_UINT32_INIT(value) ATOMIC_VAR_INIT(value)
#define R_CSTL_ATOMIC_INT64_INIT(value) ATOMIC_VAR_INIT(value)
#define R_CSTL_ATOMIC_UINT64_INIT(value) ATOMIC_VAR_INIT(value)
#define R_CSTL_ATOMIC_PTR_INIT(value) ATOMIC_VAR_INIT(value)

R_CSTL_API int
R_CSTL_AtomicInt32Increment (R_CSTL_AtomicInt32* pAtomic);

R_CSTL_API int
R_CSTL_AtomicInt32Decrement (R_CSTL_AtomicInt32* pAtomic);

R_CSTL_API int
R_CSTL_AtomicInt32CompareExchange (R_CSTL_AtomicInt32* pAtomic, int expected, int desired);

R_CSTL_API int
R_CSTL_AtomicInt32Exchange (R_CSTL_AtomicInt32* pAtomic, int value);

R_CSTL_API int
R_CSTL_AtomicInt32Load (const R_CSTL_AtomicInt32* pAtomic);

R_CSTL_API void
R_CSTL_AtomicInt32Store (R_CSTL_AtomicInt32* pAtomic, int value);

R_CSTL_API int
R_CSTL_AtomicInt32Add (R_CSTL_AtomicInt32* pAtomic, int value);

R_CSTL_API uint32_t
R_CSTL_AtomicUint32Increment (R_CSTL_AtomicUint32* pAtomic);

R_CSTL_API uint32_t
R_CSTL_AtomicUint32Decrement (R_CSTL_AtomicUint32* pAtomic);

R_CSTL_API uint32_t
R_CSTL_AtomicUint32CompareExchange (R_CSTL_AtomicUint32* pAtomic, uint32_t expected, uint32_t desired);

R_CSTL_API uint32_t
R_CSTL_AtomicUint32Exchange (R_CSTL_AtomicUint32* pAtomic, uint32_t value);

R_CSTL_API uint32_t
R_CSTL_AtomicUint32Load (const R_CSTL_AtomicUint32* pAtomic);

R_CSTL_API void
R_CSTL_AtomicUint32Store (R_CSTL_AtomicUint32* pAtomic, uint32_t value);

R_CSTL_API uint32_t
R_CSTL_AtomicUint32Add (R_CSTL_AtomicUint32* pAtomic, uint32_t value);

R_CSTL_API int64_t
R_CSTL_AtomicInt64Increment (R_CSTL_AtomicInt64* pAtomic);

R_CSTL_API int64_t
R_CSTL_AtomicInt64Decrement (R_CSTL_AtomicInt64* pAtomic);

R_CSTL_API int64_t
R_CSTL_AtomicInt64CompareExchange (R_CSTL_AtomicInt64* pAtomic, int64_t expected, int64_t desired);

R_CSTL_API int64_t
R_CSTL_AtomicInt64Exchange (R_CSTL_AtomicInt64* pAtomic, int64_t value);

R_CSTL_API int64_t
R_CSTL_AtomicInt64Load (const R_CSTL_AtomicInt64* pAtomic);

R_CSTL_API void
R_CSTL_AtomicInt64Store (R_CSTL_AtomicInt64* pAtomic, int64_t value);

R_CSTL_API int64_t
R_CSTL_AtomicInt64Add (R_CSTL_AtomicInt64* pAtomic, int64_t value);

R_CSTL_API uint64_t
R_CSTL_AtomicUint64Increment (R_CSTL_AtomicUint64* pAtomic);

R_CSTL_API uint64_t
R_CSTL_AtomicUint64Decrement (R_CSTL_AtomicUint64* pAtomic);

R_CSTL_API uint64_t
R_CSTL_AtomicUint64CompareExchange (R_CSTL_AtomicUint64* pAtomic, uint64_t expected, uint64_t desired);

R_CSTL_API uint64_t
R_CSTL_AtomicUint64Exchange (R_CSTL_AtomicUint64* pAtomic, uint64_t value);

R_CSTL_API uint64_t
R_CSTL_AtomicUint64Load (const R_CSTL_AtomicUint64* pAtomic);

R_CSTL_API void
R_CSTL_AtomicUint64Store (R_CSTL_AtomicUint64* pAtomic, uint64_t value);

R_CSTL_API uint64_t
R_CSTL_AtomicUint64Add (R_CSTL_AtomicUint64* pAtomic, uint64_t value);

R_CSTL_API void*
R_CSTL_AtomicPtrCompareExchange (R_CSTL_AtomicVoidP* pAtomic, void* expected, void* desired);

R_CSTL_API void*
R_CSTL_AtomicPtrExchange (R_CSTL_AtomicVoidP* pAtomic, void* value);

R_CSTL_API void*
R_CSTL_AtomicPtrLoad (const R_CSTL_AtomicVoidP* pAtomic);

R_CSTL_API void
R_CSTL_AtomicPtrStore (R_CSTL_AtomicVoidP* pAtomic, void* value);

#endif

#define R_CSTL_AtomicInt32Inc(p) R_CSTL_AtomicInt32Increment(p)
#define R_CSTL_AtomicInt32Dec(p) R_CSTL_AtomicInt32Decrement(p)
#define R_CSTL_AtomicUint32Inc(p) R_CSTL_AtomicUint32Increment(p)
#define R_CSTL_AtomicUint32Dec(p) R_CSTL_AtomicUint32Decrement(p)
#define R_CSTL_AtomicInt64Inc(p) R_CSTL_AtomicInt64Increment(p)
#define R_CSTL_AtomicInt64Dec(p) R_CSTL_AtomicInt64Decrement(p)
#define R_CSTL_AtomicUint64Inc(p) R_CSTL_AtomicUint64Increment(p)
#define R_CSTL_AtomicUint64Dec(p) R_CSTL_AtomicUint64Decrement(p)

#define R_CSTL_AtomicInt32CmpExch(p, e, d) R_CSTL_AtomicInt32CompareExchange(p, e, d)
#define R_CSTL_AtomicUint32CmpExch(p, e, d) R_CSTL_AtomicUint32CompareExchange(p, e, d)
#define R_CSTL_AtomicInt64CmpExch(p, e, d) R_CSTL_AtomicInt64CompareExchange(p, e, d)
#define R_CSTL_AtomicUint64CmpExch(p, e, d) R_CSTL_AtomicUint64CompareExchange(p, e, d)
#define R_CSTL_AtomicPtrCmpExch(p, e, d) R_CSTL_AtomicPtrCompareExchange(p, e, d)

#define R_CSTL_AtomicInt32Exch(p, v) R_CSTL_AtomicInt32Exchange(p, v)
#define R_CSTL_AtomicUint32Exch(p, v) R_CSTL_AtomicUint32Exchange(p, v)
#define R_CSTL_AtomicInt64Exch(p, v) R_CSTL_AtomicInt64Exchange(p, v)
#define R_CSTL_AtomicUint64Exch(p, v) R_CSTL_AtomicUint64Exchange(p, v)
#define R_CSTL_AtomicPtrExch(p, v) R_CSTL_AtomicPtrExchange(p, v)

#define R_CSTL_AtomicInt32Ld(p) R_CSTL_AtomicInt32Load(p)
#define R_CSTL_AtomicUint32Ld(p) R_CSTL_AtomicUint32Load(p)
#define R_CSTL_AtomicInt64Ld(p) R_CSTL_AtomicInt64Load(p)
#define R_CSTL_AtomicUint64Ld(p) R_CSTL_AtomicUint64Load(p)
#define R_CSTL_AtomicPtrLd(p) R_CSTL_AtomicPtrLoad(p)

#define R_CSTL_AtomicInt32St(p, v) R_CSTL_AtomicInt32Store(p, v)
#define R_CSTL_AtomicUint32St(p, v) R_CSTL_AtomicUint32Store(p, v)
#define R_CSTL_AtomicInt64St(p, v) R_CSTL_AtomicInt64Store(p, v)
#define R_CSTL_AtomicUint64St(p, v) R_CSTL_AtomicUint64Store(p, v)
#define R_CSTL_AtomicPtrSt(p, v) R_CSTL_AtomicPtrStore(p, v)