#include "rlgame.base/cstl/cstl_stack.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"

#define R_CSTL_INLINE
#include "rlgame.base/cstl/cstl_platform.h"

#include <string.h>

#if defined(R_SIMD_AVX2)
#include <immintrin.h>
#elif defined(R_SIMD_SSE)
#include <immintrin.h>
#elif defined(_RL_SIMD_ARM_NEON) || defined(R_SIMD_ARM_NEON)
#include <arm_neon.h>
#endif

#if defined(_MSC_VER) || defined(_WIN32)
#include <malloc.h>
#define R_CSTL_STACK_ALLOC(sz) _alloca (sz)
#else
#include <alloca.h>
#define R_CSTL_STACK_ALLOC(sz) alloca (sz)
#endif

#ifndef R_CSTL_STACK_MIN_CAPACITY
#define R_CSTL_STACK_MIN_CAPACITY 16u
#endif

#ifndef R_CSTL_STACK_SIMD_THRESHOLD
#define R_CSTL_STACK_SIMD_THRESHOLD 64u
#endif

struct r_cstl_stack
{
        uint8_t* pData;
        size_t   size;
        size_t   capacity;
};

R_CSTL_API_ATTR static int
r_cstl_stack_buffer_is_usable (const struct r_cstl_stack* pStack)
{
    if (!pStack) return 0;
#ifndef R_CSTL_HEAP_DEBUG
    return 1;
#else
    if (!pStack->pData) return 1;
    return r_cstl_heap_is_valid_pointer (pStack->pData) != 0;
#endif
}

R_CSTL_API_ATTR static int
r_cstl_stack_buffer_is_live (const struct r_cstl_stack* pStack)
{
    if (!pStack || !pStack->pData) return 0;
#ifndef R_CSTL_HEAP_DEBUG
    return 1;
#else
    return r_cstl_heap_is_valid_pointer (pStack->pData) != 0;
#endif
}

R_CSTL_API_ATTR static size_t
r_cstl_stack_next_capacity (size_t current, size_t required)
{
    size_t next = current ? current : R_CSTL_STACK_MIN_CAPACITY;
    while (next < required)
    {
        if (next > (SIZE_MAX / 2)) return required;
        next <<= 1;
    }
    return next;
}

static void
r_cstl_stack_copy_bytes (uint8_t* pDst, const uint8_t* pSrc, size_t sizeBytes)
{
    if (sizeBytes == 0 || pDst == pSrc) return;

#if defined(R_SIMD_AVX2)
    if (sizeBytes >= R_CSTL_STACK_SIMD_THRESHOLD)
    {
        size_t i = 0;
        for (; i + 32 <= sizeBytes; i += 32)
        {
            __m256i v = _mm256_loadu_si256 ((const __m256i*)(pSrc + i));
            _mm256_storeu_si256 ((__m256i*)(pDst + i), v);
        }
        for (; i < sizeBytes; ++i)
            pDst[i] = pSrc[i];
        return;
    }
#elif defined(R_SIMD_SSE)
    if (sizeBytes >= R_CSTL_STACK_SIMD_THRESHOLD)
    {
        size_t i = 0;
        for (; i + 16 <= sizeBytes; i += 16)
        {
            __m128i v = _mm_loadu_si128 ((const __m128i*)(pSrc + i));
            _mm_storeu_si128 ((__m128i*)(pDst + i), v);
        }
        for (; i < sizeBytes; ++i)
            pDst[i] = pSrc[i];
        return;
    }
#elif defined(_RL_SIMD_ARM_NEON) || defined(R_SIMD_ARM_NEON)
    if (sizeBytes >= R_CSTL_STACK_SIMD_THRESHOLD)
    {
        size_t i = 0;
        for (; i + 16 <= sizeBytes; i += 16)
        {
            uint8x16_t v = vld1q_u8 (pSrc + i);
            vst1q_u8 (pDst + i, v);
        }
        for (; i < sizeBytes; ++i)
            pDst[i] = pSrc[i];
        return;
    }
#endif
    memcpy (pDst, pSrc, sizeBytes);
}

static void
r_cstl_stack_release_buffer (struct r_cstl_stack* pStack)
{
    if (!pStack || !pStack->pData) return;

#if defined(R_CSTL_HEAP_DEBUG)
    if (!r_cstl_heap_is_valid_pointer (pStack->pData)) goto cstl_fail;
    r_cstl_heap_unregister_allocation (pStack, pStack->pData);
#endif

    r_cstl_heap_free (pStack->pData);
    pStack->pData = NULL;
    pStack->capacity = 0;

#if defined(R_CSTL_HEAP_DEBUG)
cstl_fail:
#endif
    return;
}

static int
r_cstl_stack_reallocate (struct r_cstl_stack* pStack, size_t newCapacity)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pStack || newCapacity == 0) goto cstl_fail;
#endif
    uint8_t* pOldData = pStack->pData;
    size_t   oldSize = pStack->size;
#if defined(R_CSTL_HEAP_DEBUG)
    if (pOldData && !r_cstl_heap_is_valid_pointer (pOldData)) goto cstl_fail;
#endif

    uint8_t* pNew = (uint8_t*)r_cstl_heap_alloc (newCapacity);
    if (!pNew) return -1;

    if (pOldData && oldSize > 0)
    {
        memcpy (pNew, pOldData, oldSize);
    }

    if (pOldData)
    {
#if defined(R_CSTL_HEAP_DEBUG)
        if (r_cstl_heap_is_valid_pointer (pOldData)) r_cstl_heap_unregister_allocation (pStack, pOldData);
#endif
        r_cstl_heap_free (pOldData);
    }

#if defined(R_CSTL_HEAP_DEBUG)
    uint64_t success = r_cstl_heap_register_allocation (
        pStack,
        pNew,
        newCapacity,
        R_CSTL_HEAP_NAME (r_cstl_stack_reallocate));
    if (success == 0) goto cstl_fail_register;
#endif

    pStack->pData = pNew;
    pStack->capacity = newCapacity;
    return 0;

cstl_fail:
    return -1;
cstl_fail_register:
    r_cstl_heap_free (pNew);
    return -1;
}

static int
r_cstl_stack_ensure_capacity (struct r_cstl_stack* pStack, size_t requiredCapacity)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pStack) goto cstl_fail;
    if (!r_cstl_stack_buffer_is_usable (pStack)) goto cstl_fail;
#endif
    if (pStack->capacity >= requiredCapacity) return 0;

    size_t newCapacity = r_cstl_stack_next_capacity (pStack->capacity, requiredCapacity);
    return r_cstl_stack_reallocate (pStack, newCapacity);

cstl_fail:
    return -1;
}

R_CSTL_API_ATTR static struct r_cstl_stack*
r_cstl_stack_create_shell (void)
{
    struct r_cstl_stack* pStack = (struct r_cstl_stack*)r_cstl_heap_alloc (sizeof (struct r_cstl_stack));
    if (!pStack) return NULL;
    pStack->pData = NULL;
    pStack->size = 0;
    pStack->capacity = 0;
    return pStack;
}

R_CSTL_API_ATTR struct r_cstl_stack*
r_cstl_new_stack (void)
{
    return r_cstl_stack_create_shell ();
}

R_CSTL_API_ATTR struct r_cstl_stack*
r_cstl_new_stack_with_capacity (size_t capacityBytes)
{
    struct r_cstl_stack* pStack = r_cstl_stack_create_shell ();
    if (!pStack) goto cstl_fail;

    if (capacityBytes == 0) return pStack;

    if (r_cstl_stack_reserve (pStack, capacityBytes) != 0) goto cstl_fail;

    return pStack;

cstl_fail:
    r_cstl_delete_stack (pStack);
    return NULL;
}

R_CSTL_API_ATTR void
r_cstl_delete_stack (struct r_cstl_stack* pStack)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pStack) return;
#endif
    r_cstl_stack_release_buffer (pStack);
    r_cstl_heap_free (pStack);
}

R_CSTL_API_ATTR int
r_cstl_stack_reserve (struct r_cstl_stack* pStack, size_t capacityBytes)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pStack) goto cstl_fail;
    if (!r_cstl_stack_buffer_is_usable (pStack)) goto cstl_fail;
#endif
    if (capacityBytes <= pStack->capacity) return 0;

    return r_cstl_stack_reallocate (pStack, capacityBytes);

cstl_fail:
    return -1;
}

R_CSTL_API_ATTR int
r_cstl_stack_push (struct r_cstl_stack* pStack, uint8_t value)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pStack) goto cstl_fail;
    if (!r_cstl_stack_buffer_is_usable (pStack)) goto cstl_fail;
#endif
    if (r_cstl_stack_ensure_capacity (pStack, pStack->size + 1) != 0) goto cstl_fail;
    pStack->pData[pStack->size] = value;
    ++pStack->size;
    return 0;

cstl_fail:
    return -1;
}

R_CSTL_API int
r_cstl_stack_push_data (struct r_cstl_stack* pStack, const uint8_t* pData, size_t size)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pStack || !pData) goto cstl_fail;
    if (!r_cstl_stack_buffer_is_usable (pStack)) goto cstl_fail;
#endif
    if (size == 0) return 0;
    if (r_cstl_stack_ensure_capacity (pStack, pStack->size + size) != 0) goto cstl_fail;
    r_cstl_stack_copy_bytes (pStack->pData + pStack->size, pData, size);
    pStack->size += size;
    return 0;
cstl_fail:
    return -1;
}

R_CSTL_API_ATTR int
r_cstl_stack_pop (struct r_cstl_stack* pStack, uint8_t* pOutValue)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pStack || pStack->size == 0) return -1;
    if (!r_cstl_stack_buffer_is_live (pStack)) goto cstl_fail;
#endif
    uint8_t value = pStack->pData[pStack->size - 1];
    --pStack->size;
    if (pOutValue) *pOutValue = value;
    return 0;

cstl_fail:
    return -1;
}

R_CSTL_API_ATTR int
r_cstl_stack_pop_data (struct r_cstl_stack* pStack, uint8_t* pOutData, size_t size)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pStack || pOutData == NULL) goto cstl_fail;
    if (size > pStack->size) goto cstl_fail;
    if (!r_cstl_stack_buffer_is_live (pStack)) goto cstl_fail;
#endif
    size_t offset = pStack->size - size;
    memcpy (pOutData, pStack->pData + offset, size);
    pStack->size -= size;
    return 0;

cstl_fail:
    return -1;
}

R_CSTL_API_ATTR int
r_cstl_stack_peek (struct r_cstl_stack* pStack, uint8_t* pOutValue)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pStack || pStack->size == 0) return -1;
    if (!r_cstl_stack_buffer_is_live (pStack)) goto cstl_fail;
#endif
    if (pOutValue) *pOutValue = pStack->pData[pStack->size - 1];
    return 0;

cstl_fail:
    return -1;
}

R_CSTL_API_ATTR int
r_cstl_stack_peek_data (struct r_cstl_stack* pStack, uint8_t* pOutData, size_t size)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pStack || pOutData == NULL) goto cstl_fail;
    if (size > pStack->size) goto cstl_fail;
    if (!r_cstl_stack_buffer_is_live (pStack)) goto cstl_fail;
#endif
    size_t offset = pStack->size - size;
    memcpy (pOutData, pStack->pData + offset, size);
    return 0;

cstl_fail:
    return -1;
}

R_CSTL_API_ATTR int
r_cstl_stack_empty (const struct r_cstl_stack* pStack)
{
    if (!pStack) return 1;
    return pStack->size == 0 ? 1 : 0;
}

R_CSTL_API_ATTR size_t
r_cstl_stack_size (const struct r_cstl_stack* pStack)
{
    if (!pStack) return 0;
    return pStack->size;
}

R_CSTL_API_ATTR size_t
r_cstl_stack_get_capacity (const struct r_cstl_stack* pStack)
{
    if (!pStack) return 0;
    return pStack->capacity;
}

R_CSTL_API_ATTR size_t
r_cstl_stack_search (const struct r_cstl_stack* pStack, uint8_t value)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pStack) return 0;
    if (!r_cstl_stack_buffer_is_live (pStack)) return 0;
#endif
    for (size_t i = pStack->size; i > 0; --i)
    {
        if (pStack->pData[i - 1] == value)
        {
            return pStack->size - i + 1;
        }
    }
    return 0;
}

R_CSTL_API_ATTR size_t
r_cstl_stack_search_data (const struct r_cstl_stack* pStack, const uint8_t* pData, size_t size)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pStack || !pData || size == 0) return 0;
    if (!r_cstl_stack_buffer_is_live (pStack)) return 0;
#endif
    if (size > pStack->size) return 0;

    for (size_t i = pStack->size - size + 1; i > 0; --i)
    {
        size_t offset = i - 1;
        if (memcmp (pStack->pData + offset, pData, size) == 0)
        {
            return pStack->size - offset - size + 1;
        }
    }
    return 0;
}

R_CSTL_API_ATTR int
r_cstl_stack_clear (struct r_cstl_stack* pStack, int zeroMemory)
{
    if (!pStack) goto cstl_fail;
    if (zeroMemory && pStack->pData && pStack->size > 0)
    {
        memset (pStack->pData, 0, pStack->size);
    }
    pStack->size = 0;
    return 0;
cstl_fail:
    return -1;
}

R_CSTL_API_ATTR const uint8_t*
r_cstl_stack_data (const struct r_cstl_stack* pStack)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pStack) goto cstl_fail;
    if (!r_cstl_stack_buffer_is_live (pStack)) goto cstl_fail;
#endif
    return pStack->pData;
cstl_fail:
    return NULL;
}
