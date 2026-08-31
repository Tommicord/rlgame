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

typedef volatile LONG      r_cstl_atomic_int32;
typedef volatile ULONG     r_cstl_atomic_uint32;
typedef volatile LONGLONG  r_cstl_atomic_int64;
typedef volatile ULONGLONG r_cstl_atomic_uint64;
typedef volatile PVOID     r_cstl_atomic_ptr;

#define R_CSTL_ATOMIC_INT32_INIT(value)  (value)
#define R_CSTL_ATOMIC_UINT32_INIT(value) (value)
#define R_CSTL_ATOMIC_INT64_INIT(value)  (value)
#define R_CSTL_ATOMIC_UINT64_INIT(value) (value)
#define R_CSTL_ATOMIC_PTR_INIT(value)    (value)

R_CSTL_API int r_cstl_atomic_int32_increment (r_cstl_atomic_int32* pAtomic);

R_CSTL_API int r_cstl_atomic_int32_decrement (r_cstl_atomic_int32* pAtomic);

R_CSTL_API int r_cstl_atomic_int32_compare_exchange (r_cstl_atomic_int32* pAtomic, int expected, int desired);

R_CSTL_API int r_cstl_atomic_int32_exchange (r_cstl_atomic_int32* pAtomic, int value);

R_CSTL_API int r_cstl_atomic_int32_load (const r_cstl_atomic_int32* pAtomic);

R_CSTL_API void r_cstl_atomic_int32_store (r_cstl_atomic_int32* pAtomic, int value);

R_CSTL_API int r_cstl_atomic_int32_add (r_cstl_atomic_int32* pAtomic, int value);

R_CSTL_API uint32_t r_cstl_atomic_uint32_increment (r_cstl_atomic_uint32* pAtomic);

R_CSTL_API uint32_t r_cstl_atomic_uint32_decrement (r_cstl_atomic_uint32* pAtomic);

R_CSTL_API uint32_t
r_cstl_atomic_uint32_compare_exchange (r_cstl_atomic_uint32* pAtomic, uint32_t expected, uint32_t desired);

R_CSTL_API uint32_t r_cstl_atomic_uint32_exchange (r_cstl_atomic_uint32* pAtomic, uint32_t value);

R_CSTL_API uint32_t r_cstl_atomic_uint32_load (const r_cstl_atomic_uint32* pAtomic);

R_CSTL_API void r_cstl_atomic_uint32_store (r_cstl_atomic_uint32* pAtomic, uint32_t value);

R_CSTL_API uint32_t r_cstl_atomic_uint32_add (r_cstl_atomic_uint32* pAtomic, uint32_t value);

R_CSTL_API int64_t r_cstl_atomic_int64_increment (r_cstl_atomic_int64* pAtomic);

R_CSTL_API int64_t r_cstl_atomic_int64_decrement (r_cstl_atomic_int64* pAtomic);

R_CSTL_API int64_t
r_cstl_atomic_int64_compare_exchange (r_cstl_atomic_int64* pAtomic, int64_t expected, int64_t desired);

R_CSTL_API int64_t r_cstl_atomic_int64_exchange (r_cstl_atomic_int64* pAtomic, int64_t value);

R_CSTL_API int64_t r_cstl_atomic_int64_load (const r_cstl_atomic_int64* pAtomic);

R_CSTL_API void r_cstl_atomic_int64_store (r_cstl_atomic_int64* pAtomic, int64_t value);

R_CSTL_API int64_t r_cstl_atomic_int64_add (r_cstl_atomic_int64* pAtomic, int64_t value);

R_CSTL_API uint64_t r_cstl_atomic_uint64_increment (r_cstl_atomic_uint64* pAtomic);

R_CSTL_API uint64_t r_cstl_atomic_uint64_decrement (r_cstl_atomic_uint64* pAtomic);

R_CSTL_API uint64_t
r_cstl_atomic_uint64_compare_exchange (r_cstl_atomic_uint64* pAtomic, uint64_t expected, uint64_t desired);

R_CSTL_API uint64_t r_cstl_atomic_uint64_exchange (r_cstl_atomic_uint64* pAtomic, uint64_t value);

R_CSTL_API uint64_t r_cstl_atomic_uint64_load (const r_cstl_atomic_uint64* pAtomic);

R_CSTL_API void r_cstl_atomic_uint64_store (r_cstl_atomic_uint64* pAtomic, uint64_t value);

R_CSTL_API uint64_t r_cstl_atomic_uint64_add (r_cstl_atomic_uint64* pAtomic, uint64_t value);

R_CSTL_API void* r_cstl_atomic_ptr_compare_exchange (r_cstl_atomic_ptr* pAtomic, void* expected, void* desired);

R_CSTL_API void* r_cstl_atomic_ptr_exchange (r_cstl_atomic_ptr* pAtomic, void* value);

R_CSTL_API void* r_cstl_atomic_ptr_load (const r_cstl_atomic_ptr* pAtomic);

R_CSTL_API void r_cstl_atomic_ptr_store (r_cstl_atomic_ptr* pAtomic, void* value);

#elif defined(R_CSTL_PLATFORM_LINUX)

typedef volatile int32_t _Atomic r_cstl_atomic_int32;
typedef volatile uint32_t _Atomic r_cstl_atomic_uint32;
typedef volatile int64_t _Atomic r_cstl_atomic_int64;
typedef volatile uint64_t _Atomic r_cstl_atomic_uint64;
typedef volatile void* _Atomic r_cstl_atomic_voidP;

#define R_CSTL_ATOMIC_INT32_INIT(value)  ATOMIC_VAR_INIT (value)
#define R_CSTL_ATOMIC_UINT32_INIT(value) ATOMIC_VAR_INIT (value)
#define R_CSTL_ATOMIC_INT64_INIT(value)  ATOMIC_VAR_INIT (value)
#define R_CSTL_ATOMIC_UINT64_INIT(value) ATOMIC_VAR_INIT (value)
#define R_CSTL_ATOMIC_PTR_INIT(value)    ATOMIC_VAR_INIT (value)

R_CSTL_API int r_cstl_atomic_int32_increment (r_cstl_atomic_int32* pAtomic);

R_CSTL_API int r_cstl_atomic_int32_decrement (r_cstl_atomic_int32* pAtomic);

R_CSTL_API int r_cstl_atomic_int32_compare_exchange (r_cstl_atomic_int32* pAtomic, int expected, int desired);

R_CSTL_API int r_cstl_atomic_int32_exchange (r_cstl_atomic_int32* pAtomic, int value);

R_CSTL_API int r_cstl_atomic_int32_load (const r_cstl_atomic_int32* pAtomic);

R_CSTL_API void r_cstl_atomic_int32_store (r_cstl_atomic_int32* pAtomic, int value);

R_CSTL_API int r_cstl_atomic_int32_add (r_cstl_atomic_int32* pAtomic, int value);

R_CSTL_API uint32_t r_cstl_atomic_uint32_increment (r_cstl_atomic_uint32* pAtomic);

R_CSTL_API uint32_t r_cstl_atomic_uint32_decrement (r_cstl_atomic_uint32* pAtomic);

R_CSTL_API uint32_t
r_cstl_atomic_uint32_compare_exchange (r_cstl_atomic_uint32* pAtomic, uint32_t expected, uint32_t desired);

R_CSTL_API uint32_t r_cstl_atomic_uint32_exchange (r_cstl_atomic_uint32* pAtomic, uint32_t value);

R_CSTL_API uint32_t r_cstl_atomic_uint32_load (const r_cstl_atomic_uint32* pAtomic);

R_CSTL_API void r_cstl_atomic_uint32_store (r_cstl_atomic_uint32* pAtomic, uint32_t value);

R_CSTL_API uint32_t r_cstl_atomic_uint32_add (r_cstl_atomic_uint32* pAtomic, uint32_t value);

R_CSTL_API int64_t r_cstl_atomic_int64_increment (r_cstl_atomic_int64* pAtomic);

R_CSTL_API int64_t r_cstl_atomic_int64_decrement (r_cstl_atomic_int64* pAtomic);

R_CSTL_API int64_t
r_cstl_atomic_int64_compare_exchange (r_cstl_atomic_int64* pAtomic, int64_t expected, int64_t desired);

R_CSTL_API int64_t r_cstl_atomic_int64_exchange (r_cstl_atomic_int64* pAtomic, int64_t value);

R_CSTL_API int64_t r_cstl_atomic_int64_load (const r_cstl_atomic_int64* pAtomic);

R_CSTL_API void r_cstl_atomic_int64_store (r_cstl_atomic_int64* pAtomic, int64_t value);

R_CSTL_API int64_t r_cstl_atomic_int64_add (r_cstl_atomic_int64* pAtomic, int64_t value);

R_CSTL_API uint64_t r_cstl_atomic_uint64_increment (r_cstl_atomic_uint64* pAtomic);

R_CSTL_API uint64_t r_cstl_atomic_uint64_decrement (r_cstl_atomic_uint64* pAtomic);

R_CSTL_API uint64_t
r_cstl_atomic_uint64_compare_exchange (r_cstl_atomic_uint64* pAtomic, uint64_t expected, uint64_t desired);

R_CSTL_API uint64_t r_cstl_atomic_uint64_exchange (r_cstl_atomic_uint64* pAtomic, uint64_t value);

R_CSTL_API uint64_t r_cstl_atomic_uint64_load (const r_cstl_atomic_uint64* pAtomic);

R_CSTL_API void r_cstl_atomic_uint64_store (r_cstl_atomic_uint64* pAtomic, uint64_t value);

R_CSTL_API uint64_t r_cstl_atomic_uint64_add (r_cstl_atomic_uint64* pAtomic, uint64_t value);

R_CSTL_API void* r_cstl_atomic_ptr_compare_exchange (r_cstl_atomic_voidP* pAtomic, void* expected, void* desired);

R_CSTL_API void* r_cstl_atomic_ptr_exchange (r_cstl_atomic_voidP* pAtomic, void* value);

R_CSTL_API void* r_cstl_atomic_ptr_load (const r_cstl_atomic_voidP* pAtomic);

R_CSTL_API void r_cstl_atomic_ptr_store (r_cstl_atomic_voidP* pAtomic, void* value);

#endif

#define r_cstl_atomic_int32_inc(p)  r_cstl_atomic_int32_increment (p)
#define r_cstl_atomic_int32_dec(p)  r_cstl_atomic_int32_decrement (p)
#define r_cstl_atomic_uint32_inc(p) r_cstl_atomic_uint32_increment (p)
#define r_cstl_atomic_uint32_dec(p) r_cstl_atomic_uint32_decrement (p)
#define r_cstl_atomic_int64_inc(p)  r_cstl_atomic_int64_increment (p)
#define r_cstl_atomic_int64_dec(p)  r_cstl_atomic_int64_decrement (p)
#define r_cstl_atomic_uint64_inc(p) r_cstl_atomic_uint64_increment (p)
#define r_cstl_atomic_uint64_dec(p) r_cstl_atomic_uint64_decrement (p)

#define r_cstl_atomic_int32_cmp_exch(p, e, d)  r_cstl_atomic_int32_compare_exchange (p, e, d)
#define r_cstl_atomic_uint32_cmp_exch(p, e, d) r_cstl_atomic_uint32_compare_exchange (p, e, d)
#define r_cstl_atomic_int64_cmp_exch(p, e, d)  r_cstl_atomic_int64_compare_exchange (p, e, d)
#define r_cstl_atomic_uint64_cmp_exch(p, e, d) r_cstl_atomic_uint64_compare_exchange (p, e, d)
#define r_cstl_atomic_ptr_cmp_exch(p, e, d)    r_cstl_atomic_ptr_compare_exchange (p, e, d)

#define r_cstl_atomic_int32_exch(p, v)  r_cstl_atomic_int32_exchange (p, v)
#define r_cstl_atomic_uint32_exch(p, v) r_cstl_atomic_uint32_exchange (p, v)
#define r_cstl_atomic_int64_exch(p, v)  r_cstl_atomic_int64_exchange (p, v)
#define r_cstl_atomic_uint64_exch(p, v) r_cstl_atomic_uint64_exchange (p, v)
#define r_cstl_atomic_ptr_exch(p, v)    r_cstl_atomic_ptr_exchange (p, v)

#define r_cstl_atomic_int32_ld(p)  r_cstl_atomic_int32_load (p)
#define r_cstl_atomic_uint32_ld(p) r_cstl_atomic_uint32_load (p)
#define r_cstl_atomic_int64_ld(p)  r_cstl_atomic_int64_load (p)
#define r_cstl_atomic_uint64_ld(p) r_cstl_atomic_uint64_load (p)
#define r_cstl_atomic_ptr_ld(p)    r_cstl_atomic_ptr_load (p)

#define r_cstl_atomic_int32_st(p, v)  r_cstl_atomic_int32_store (p, v)
#define r_cstl_atomic_uint32_st(p, v) r_cstl_atomic_uint32_store (p, v)
#define r_cstl_atomic_int64_st(p, v)  r_cstl_atomic_int64_store (p, v)
#define r_cstl_atomic_uint64_st(p, v) r_cstl_atomic_uint64_store (p, v)
#define r_cstl_atomic_ptr_st(p, v)    r_cstl_atomic_ptr_store (p, v)