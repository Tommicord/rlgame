#include "rlgame.base/cstl/cstl_array.h"
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

#ifndef R_CSTL_ARRAY_MIN_CAPACITY
#define R_CSTL_ARRAY_MIN_CAPACITY 16u
#endif

#ifndef R_CSTL_ARRAY_SIMD_THRESHOLD
#define R_CSTL_ARRAY_SIMD_THRESHOLD 64u
#endif

#ifndef R_CSTL_ARRAY_SORT_INSERTION_THRESHOLD
#define R_CSTL_ARRAY_SORT_INSERTION_THRESHOLD 24u
#endif

struct r_cstl_array
{
        uint8_t* pData;
        size_t   length;
        size_t   capacity;
};

R_CSTL_API_ATTR static int
r_cstl_array_buffer_is_usable (const struct r_cstl_array* pArray)
{
    if (!pArray) return 0;
#ifndef R_CSTL_HEAP_DEBUG
    return 1;
#else
    if (!pArray->pData) return 1;
    return r_cstl_heap_is_valid_pointer (pArray->pData) != 0;
#endif
}

R_CSTL_API_ATTR static int
r_cstl_array_buffer_is_live (const struct r_cstl_array* pArray)
{
    if (!pArray || !pArray->pData) return 0;
#ifndef R_CSTL_HEAP_DEBUG
    return 1;
#else
    return r_cstl_heap_is_valid_pointer (pArray->pData) != 0;
#endif
}

R_CSTL_API_ATTR static size_t
r_cstl_array_next_capacity (size_t current, size_t required)
{
    size_t next = current ? current : R_CSTL_ARRAY_MIN_CAPACITY;
    while (next < required)
    {
        if (next > (SIZE_MAX / 2)) return required;
        next <<= 1;
    }
    return next;
}

static void
r_cstl_array_copy_bytes (uint8_t* pDst, const uint8_t* pSrc, size_t sizeBytes)
{
    if (sizeBytes == 0 || pDst == pSrc) return;

#if defined(R_SIMD_AVX2)
    if (sizeBytes >= R_CSTL_ARRAY_SIMD_THRESHOLD)
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
    if (sizeBytes >= R_CSTL_ARRAY_SIMD_THRESHOLD)
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
    if (sizeBytes >= R_CSTL_ARRAY_SIMD_THRESHOLD)
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
r_cstl_array_release_buffer (struct r_cstl_array* pArray)
{
    if (!pArray || !pArray->pData) return;

#if defined(R_CSTL_HEAP_DEBUG)
    if (!r_cstl_heap_is_valid_pointer (pArray->pData)) goto cstl_fail;
    r_cstl_heap_unregister_allocation (pArray, pArray->pData);
#endif

    r_cstl_heap_free (pArray->pData);
    pArray->pData = NULL;
    pArray->capacity = 0;

#if defined(R_CSTL_HEAP_DEBUG)
cstl_fail:
#endif
    return;
}

static int
r_cstl_array_reallocate (struct r_cstl_array* pArray, size_t newCapacity)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pArray || newCapacity == 0) goto cstl_fail;
#endif
    uint8_t* pOldData = pArray->pData;
    size_t   oldLength = pArray->length;
#if defined(R_CSTL_HEAP_DEBUG)
    if (pOldData && !r_cstl_heap_is_valid_pointer (pOldData)) goto cstl_fail;
#endif

    uint8_t* pNew = (uint8_t*)r_cstl_heap_alloc (newCapacity);
    if (!pNew) return -1;

    if (pOldData && oldLength > 0)
    {
        memcpy (pNew, pOldData, oldLength);
    }

    if (pOldData)
    {
#if defined(R_CSTL_HEAP_DEBUG)
        if (r_cstl_heap_is_valid_pointer (pOldData)) r_cstl_heap_unregister_allocation (pArray, pOldData);
#endif
        r_cstl_heap_free (pOldData);
    }

#if defined(R_CSTL_HEAP_DEBUG)
    uint64_t success = r_cstl_heap_register_allocation (
        pArray,
        pNew,
        newCapacity,
        R_CSTL_HEAP_NAME (r_cstl_array_reallocate));
    if (success == 0) goto cstl_fail_register;
#endif

    pArray->pData = pNew;
    pArray->capacity = newCapacity;
    return 0;

cstl_fail:
    return -1;
cstl_fail_register:
    r_cstl_heap_free (pNew);
    return -1;
}

static int
r_cstl_array_ensure_capacity (struct r_cstl_array* pArray, size_t requiredCapacity)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pArray) goto cstl_fail;
    if (!r_cstl_array_buffer_is_usable (pArray)) goto cstl_fail;
#endif
    if (pArray->capacity >= requiredCapacity) return 0;

    size_t newCapacity = r_cstl_array_next_capacity (pArray->capacity, requiredCapacity);
    return r_cstl_array_reallocate (pArray, newCapacity);

cstl_fail:
    return -1;
}

R_CSTL_API_ATTR struct r_cstl_array*
r_cstl_array_create_shell (void)
{
    struct r_cstl_array* pArray = (struct r_cstl_array*)r_cstl_heap_alloc (sizeof (struct r_cstl_array));
    if (!pArray) return NULL;
    pArray->pData = NULL;
    pArray->length = 0;
    pArray->capacity = 0;
    return pArray;
}

R_CSTL_API_ATTR struct r_cstl_array*
r_cstl_new_array (void)
{
    return r_cstl_array_create_shell ();
}

R_CSTL_API_ATTR struct r_cstl_array*
r_cstl_new_array_with_capacity (size_t capacityBytes)
{
    struct r_cstl_array* pArray = r_cstl_array_create_shell ();
    if (!pArray) goto cstl_fail;

    if (capacityBytes == 0) return pArray;

    if (r_cstl_array_rev_bytes (pArray, capacityBytes) != 0) goto cstl_fail;

    return pArray;

cstl_fail:
    r_cstl_delete_array (pArray);
    return NULL;
}

R_CSTL_API_ATTR struct r_cstl_array*
r_cstl_new_array_with_length_zeroed (size_t lengthBytes)
{
    struct r_cstl_array* pArray = r_cstl_new_array_with_capacity (lengthBytes);
    if (!pArray) return NULL;

    if (pArray->pData) memset (pArray->pData, 0, lengthBytes);
    pArray->length = lengthBytes;
    return pArray;
}

R_CSTL_API_ATTR struct r_cstl_array*
r_cstl_new_array_with_length (size_t lengthBytes)
{
    struct r_cstl_array* pArray = r_cstl_new_array_with_capacity (lengthBytes);
    if (!pArray) return NULL;
    pArray->length = lengthBytes;
    return pArray;
}

R_CSTL_API_ATTR struct r_cstl_array*
r_cstl_new_array_with_data (const uint8_t* pData, size_t lengthBytes)
{
    struct r_cstl_array* pArray = r_cstl_array_create_shell ();
    if (!pArray) return NULL;
    if (lengthBytes == 0) return pArray;
    if (!pData) goto cstl_fail;
    if (r_cstl_array_rev_bytes (pArray, lengthBytes) != 0) goto cstl_fail;

    r_cstl_array_copy_bytes (pArray->pData, pData, lengthBytes);
    pArray->length = lengthBytes;
    return pArray;

cstl_fail:
    r_cstl_delete_array (pArray);
    return NULL;
}

R_CSTL_API_ATTR void
r_cstl_delete_array (struct r_cstl_array* pArray)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pArray) return;
#endif
    r_cstl_array_release_buffer (pArray);
    r_cstl_heap_free (pArray);
}

R_CSTL_API_ATTR int
r_cstl_array_rev_bytes (struct r_cstl_array* pArray, size_t capacityBytes)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pArray) goto cstl_fail;
    if (!r_cstl_array_buffer_is_usable (pArray)) goto cstl_fail;
#endif
    if (capacityBytes <= pArray->capacity) return 0;

    return r_cstl_array_reallocate (pArray, capacityBytes);

cstl_fail:
    return -1;
}

R_CSTL_API_ATTR int
r_cstl_array_push (struct r_cstl_array* pArray, uint8_t value)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pArray) goto cstl_fail;
    if (!r_cstl_array_buffer_is_usable (pArray)) goto cstl_fail;
#endif
    // ensure there is room for one more byte
    if (r_cstl_array_ensure_capacity (pArray, pArray->length + 1) != 0) goto cstl_fail;
    pArray->pData[pArray->length] = value;
    ++pArray->length;
    return 0;

cstl_fail:
    return -1;
}

R_CSTL_API int
r_cstl_array_push_data (struct r_cstl_array* pArray, const uint8_t* pData, size_t size)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pArray || !pData) goto cstl_fail;
    if (!r_cstl_array_buffer_is_usable (pArray)) goto cstl_fail;
#endif
    if (size == 0) return 0;
    if (r_cstl_array_ensure_capacity (pArray, pArray->length + size) != 0) goto cstl_fail;
    r_cstl_array_copy_bytes (pArray->pData + pArray->length, pData, size);
    pArray->length += size;
    return 0;
cstl_fail:
    return -1;
}

R_CSTL_API_ATTR int
r_cstl_array_pop (struct r_cstl_array* pArray, uint8_t* pOutValue)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pArray || pArray->length == 0) return -1;
    if (!r_cstl_array_buffer_is_live (pArray)) goto cstl_fail;
#endif
    uint8_t value = pArray->pData[pArray->length - 1];
    --pArray->length;
    if (pOutValue) *pOutValue = value;
    return 0;

cstl_fail:
    return -1;
}

R_CSTL_API_ATTR int
r_cstl_array_shift (struct r_cstl_array* pArray, uint8_t* pOutValue)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pArray || pArray->length == 0) goto cstl_fail;
    if (!r_cstl_array_buffer_is_live (pArray)) goto cstl_fail;
#endif
    uint8_t value = pArray->pData[0];
    if (pOutValue) *pOutValue = value;

    if (pArray->length > 1) memmove (pArray->pData, pArray->pData + 1, pArray->length - 1);
    --pArray->length;
    return 0;

cstl_fail:
    return -1;
}

R_CSTL_API_ATTR int
r_cstl_array_unshift (struct r_cstl_array* pArray, uint8_t value)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pArray) goto cstl_fail;
    if (!r_cstl_array_buffer_is_usable (pArray)) goto cstl_fail;
#endif
    if (r_cstl_array_ensure_capacity (pArray, pArray->length + 1) != 0) goto cstl_fail;

    if (pArray->length > 0) memmove (pArray->pData + 1, pArray->pData, pArray->length);

    pArray->pData[0] = value;
    ++pArray->length;
    return 0;

cstl_fail:
    return -1;
}

R_CSTL_API_ATTR struct r_cstl_array*
r_cstl_array_slice (const struct r_cstl_array* pArray, size_t start, size_t end)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pArray || start > end || end > pArray->length) goto cstl_fail;
    if (end > start && !r_cstl_array_buffer_is_live (pArray)) goto cstl_fail;
#endif

    size_t               sliceLength = end - start;
    struct r_cstl_array* pSlice = r_cstl_new_array_with_capacity (sliceLength);
    if (!pSlice) goto cstl_fail;

    if (sliceLength > 0)
    {
        if (!pArray->pData) goto cstl_fail;
        r_cstl_array_copy_bytes (pSlice->pData, pArray->pData + start, sliceLength);
        pSlice->length = sliceLength;
    }
    return pSlice;
cstl_fail:
    return NULL;
}

R_CSTL_API_ATTR const uint8_t*
r_cstl_array_data (const struct r_cstl_array* pArray)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pArray) goto cstl_fail;
    if (!r_cstl_array_buffer_is_live (pArray)) goto cstl_fail;
#endif
    return pArray->pData;
cstl_fail:
    return NULL;
}

R_CSTL_API_ATTR int
r_cstl_array_clear (struct r_cstl_array* pArray, int zeroMemory)
{
    if (!pArray) goto cstl_fail;
    if (zeroMemory && pArray->pData && pArray->length > 0)
    {
        memset (pArray->pData, 0, pArray->length);
    }
    pArray->length = 0;
    return 0;
cstl_fail:
    return -1;
}

R_CSTL_API_ATTR int
r_cstl_array_fill (struct r_cstl_array* pArray, uint8_t value)
{
    if (!pArray || pArray->length == 0) return -1;
    if (!r_cstl_array_buffer_is_live (pArray)) return -1;
    memset (pArray->pData, value, pArray->length);
    return 0;
}

typedef int (*r_cstl_array_sort_comparator) (const void* pLeft, const void* pRight, void* pData);

#define R_CSTL_COMPARE(left, right) (((left) > (right)) - ((left) < (right)))

R_CSTL_API_ATTR int
r_cstl_array_compare_u8 (const void* pLeft, const void* pRight, void* pData)
{
    (void)pData;
    const uint8_t left = *(const uint8_t*)pLeft;
    const uint8_t right = *(const uint8_t*)pRight;
    return R_CSTL_COMPARE (left, right);
}

R_CSTL_API_ATTR int
r_cstl_array_compare_u16 (const void* pLeft, const void* pRight, void* pData)
{
    (void)pData;
    const uint16_t left = *(const uint16_t*)pLeft;
    const uint16_t right = *(const uint16_t*)pRight;
    return R_CSTL_COMPARE (left, right);
}

R_CSTL_API_ATTR int
r_cstl_array_compare_u32 (const void* pLeft, const void* pRight, void* pData)
{
    (void)pData;
    const uint32_t left = *(const uint32_t*)pLeft;
    const uint32_t right = *(const uint32_t*)pRight;
    return R_CSTL_COMPARE (left, right);
}

R_CSTL_API_ATTR int
r_cstl_array_compare_u64 (const void* pLeft, const void* pRight, void* pData)
{
    (void)pData;
    const uint64_t left = *(const uint64_t*)pLeft;
    const uint64_t right = *(const uint64_t*)pRight;
    return R_CSTL_COMPARE (left, right);
}

static void
r_cstl_array_sort_u8_counting (uint8_t* pBase, size_t nelem)
{
    size_t count[256] = {0};
    for (size_t i = 0; i < nelem; ++i)
        ++count[pBase[i]];

    size_t write = 0;
    for (size_t value = 0; value < 256; ++value)
    {
        for (size_t n = count[value]; n > 0; --n)
            pBase[write++] = (uint8_t)value;
    }
}

static size_t
r_cstl_array_floor_log2 (size_t n)
{
    size_t log = 0;
    while (n > 1u)
    {
        n >>= 1;
        ++log;
    }
    return log;
}

#define R_CSTL_ARRAY_SORT_TYPED(Suffix, Type)                                                                \
    R_CSTL_API_ATTR static int r_cstl_array_cmp_##Suffix##Inline (const Type* pLeft, const Type* pRight)     \
    {                                                                                                        \
        const Type left = *pLeft;                                                                            \
        const Type right = *pRight;                                                                          \
        return (left > right) - (left < right);                                                              \
    }                                                                                                        \
                                                                                                             \
    R_CSTL_API_ATTR static void r_cstl_array_swap_##Suffix (Type* pLeft, Type* pRight)                       \
    {                                                                                                        \
        const Type tmp = *pLeft;                                                                             \
        *pLeft = *pRight;                                                                                    \
        *pRight = tmp;                                                                                       \
    }                                                                                                        \
                                                                                                             \
    static void r_cstl_array_insertion_sort_##Suffix (Type* pBase, long left, long right)                    \
    {                                                                                                        \
        for (long i = left + 1; i <= right; ++i)                                                             \
        {                                                                                                    \
            const Type key = pBase[i];                                                                       \
            long       j = i - 1;                                                                            \
            while (j >= left && r_cstl_array_cmp_##Suffix##Inline (pBase + j, &key) > 0)                     \
            {                                                                                                \
                pBase[j + 1] = pBase[j];                                                                     \
                --j;                                                                                         \
            }                                                                                                \
            pBase[j + 1] = key;                                                                              \
        }                                                                                                    \
    }                                                                                                        \
                                                                                                             \
    static void r_cstl_array_insertion_sort_##Suffix##Cb (                                                   \
        Type*                              pBase,                                                            \
        long                               left,                                                             \
        long                               right,                                                            \
        const r_cstl_array_sort_comparator pCmp,                                                             \
        void*                              pData)                                                            \
    {                                                                                                        \
        r_cstl_array_sort_comparator cmp = pCmp;                                                             \
        void*                        data = pData;                                                           \
        for (long i = left + 1; i <= right; ++i)                                                             \
        {                                                                                                    \
            const Type key = pBase[i];                                                                       \
            long       j = i - 1;                                                                            \
            while (j >= left && cmp (pBase + j, &key, data) > 0)                                             \
            {                                                                                                \
                pBase[j + 1] = pBase[j];                                                                     \
                --j;                                                                                         \
            }                                                                                                \
            pBase[j + 1] = key;                                                                              \
        }                                                                                                    \
    }                                                                                                        \
                                                                                                             \
    static void r_cstl_array_sift_down_##Suffix (Type* pBase, long start, long end)                          \
    {                                                                                                        \
        long root = start;                                                                                   \
        while ((root << 1) + 1 <= end)                                                                       \
        {                                                                                                    \
            long child = (root << 1) + 1;                                                                    \
            if (child + 1 < end && r_cstl_array_cmp_##Suffix##Inline (pBase + child, pBase + child + 1) < 0) \
                ++child;                                                                                     \
            if (r_cstl_array_cmp_##Suffix##Inline (pBase + root, pBase + child) >= 0) break;                 \
            r_cstl_array_swap_##Suffix (pBase + root, pBase + child);                                        \
            root = child;                                                                                    \
        }                                                                                                    \
    }                                                                                                        \
                                                                                                             \
    static void r_cstl_array_sift_down_##Suffix##Cb (                                                        \
        Type*                              pBase,                                                            \
        long                               start,                                                            \
        long                               end,                                                              \
        const r_cstl_array_sort_comparator pCmp,                                                             \
        void*                              pData)                                                            \
    {                                                                                                        \
        r_cstl_array_sort_comparator cmp = pCmp;                                                             \
        void*                        data = pData;                                                           \
        long                         root = start;                                                           \
        while ((root << 1) + 1 <= end)                                                                       \
        {                                                                                                    \
            long child = (root << 1) + 1;                                                                    \
            if (child + 1 < end && cmp (pBase + child, pBase + child + 1, data) < 0) ++child;                \
            if (cmp (pBase + root, pBase + child, data) >= 0) break;                                         \
            r_cstl_array_swap_##Suffix (pBase + root, pBase + child);                                        \
            root = child;                                                                                    \
        }                                                                                                    \
    }                                                                                                        \
                                                                                                             \
    static void r_cstl_array_heapsort_##Suffix (Type* pBase, long left, long right)                          \
    {                                                                                                        \
        for (long start = (left + right) >> 1; start >= left; --start)                                       \
            r_cstl_array_sift_down_##Suffix (pBase, start, right);                                           \
        for (long end = right; end > left; --end)                                                            \
        {                                                                                                    \
            r_cstl_array_swap_##Suffix (pBase + left, pBase + end);                                          \
            r_cstl_array_sift_down_##Suffix (pBase, left, end - 1);                                          \
        }                                                                                                    \
    }                                                                                                        \
                                                                                                             \
    static void r_cstl_array_heapsort_##Suffix##Cb (                                                         \
        Type*                              pBase,                                                            \
        long                               left,                                                             \
        long                               right,                                                            \
        const r_cstl_array_sort_comparator pCmp,                                                             \
        void*                              pData)                                                            \
    {                                                                                                        \
        for (long start = (left + right) >> 1; start >= left; --start)                                       \
            r_cstl_array_sift_down_##Suffix##Cb (pBase, start, right, pCmp, pData);                          \
        for (long end = right; end > left; --end)                                                            \
        {                                                                                                    \
            r_cstl_array_swap_##Suffix (pBase + left, pBase + end);                                          \
            r_cstl_array_sift_down_##Suffix##Cb (pBase, left, end - 1, pCmp, pData);                         \
        }                                                                                                    \
    }                                                                                                        \
                                                                                                             \
    static long r_cstl_array_partition_##Suffix (Type* pBase, long left, long right)                         \
    {                                                                                                        \
        long mid = (left + right) >> 1;                                                                      \
        if (R_CSTL_COMPARE (pBase[mid], pBase[left]) < 0)                                                    \
            r_cstl_array_swap_##Suffix (pBase + mid, pBase + left);                                          \
        if (R_CSTL_COMPARE (pBase[right], pBase[left]) < 0)                                                  \
            r_cstl_array_swap_##Suffix (pBase + left, pBase + right);                                        \
        if (R_CSTL_COMPARE (pBase[right], pBase[mid]) < 0)                                                   \
            r_cstl_array_swap_##Suffix (pBase + mid, pBase + right);                                         \
        r_cstl_array_swap_##Suffix (pBase + mid, pBase + right - 1);                                         \
                                                                                                             \
        const Type pivot = pBase[right - 1];                                                                 \
        long       i = left;                                                                                 \
        long       j = right - 2;                                                                            \
        for (;;)                                                                                             \
        {                                                                                                    \
            while (R_CSTL_COMPARE (pBase[i], pivot) < 0)                                                     \
                ++i;                                                                                         \
            while (R_CSTL_COMPARE (pBase[j], pivot) > 0)                                                     \
                --j;                                                                                         \
            if (i >= j) break;                                                                               \
            r_cstl_array_swap_##Suffix (pBase + i, pBase + j);                                               \
            ++i;                                                                                             \
            --j;                                                                                             \
        }                                                                                                    \
        r_cstl_array_swap_##Suffix (pBase + i, pBase + right - 1);                                           \
        return i;                                                                                            \
    }                                                                                                        \
                                                                                                             \
    static long r_cstl_array_partition_##Suffix##Cb (                                                        \
        Type*                              pBase,                                                            \
        long                               left,                                                             \
        long                               right,                                                            \
        const r_cstl_array_sort_comparator pCmp,                                                             \
        void*                              pData)                                                            \
    {                                                                                                        \
        r_cstl_array_sort_comparator cmp = pCmp;                                                             \
        void*                        data = pData;                                                           \
        long                         mid = (left + right) >> 1;                                              \
        if (cmp (pBase + mid, pBase + left, data) < 0)                                                       \
            r_cstl_array_swap_##Suffix (pBase + mid, pBase + left);                                          \
        if (cmp (pBase + right, pBase + left, data) < 0)                                                     \
            r_cstl_array_swap_##Suffix (pBase + left, pBase + right);                                        \
        if (cmp (pBase + right, pBase + mid, data) < 0)                                                      \
            r_cstl_array_swap_##Suffix (pBase + mid, pBase + right);                                         \
        r_cstl_array_swap_##Suffix (pBase + mid, pBase + right - 1);                                         \
                                                                                                             \
        const void* pPivot = pBase + right - 1;                                                              \
        long        i = left;                                                                                \
        long        j = right - 2;                                                                           \
        for (;;)                                                                                             \
        {                                                                                                    \
            while (cmp (pBase + i, pPivot, data) < 0)                                                        \
                ++i;                                                                                         \
            while (cmp (pBase + j, pPivot, data) > 0)                                                        \
                --j;                                                                                         \
            if (i >= j) break;                                                                               \
            r_cstl_array_swap_##Suffix (pBase + i, pBase + j);                                               \
            ++i;                                                                                             \
            --j;                                                                                             \
        }                                                                                                    \
        r_cstl_array_swap_##Suffix (pBase + i, pBase + right - 1);                                           \
        return i;                                                                                            \
    }                                                                                                        \
                                                                                                             \
    static void r_cstl_array_introsort_##Suffix (Type* pBase, long left, long right, int depthLimit)         \
    {                                                                                                        \
        while (left < right)                                                                                 \
        {                                                                                                    \
            const long count = right - left + 1;                                                             \
            if (count <= (long)R_CSTL_ARRAY_SORT_INSERTION_THRESHOLD)                                        \
            {                                                                                                \
                r_cstl_array_insertion_sort_##Suffix (pBase, left, right);                                   \
                return;                                                                                      \
            }                                                                                                \
            if (depthLimit <= 0)                                                                             \
            {                                                                                                \
                r_cstl_array_heapsort_##Suffix (pBase, left, right);                                         \
                return;                                                                                      \
            }                                                                                                \
            const long pivot = r_cstl_array_partition_##Suffix (pBase, left, right);                         \
            --depthLimit;                                                                                    \
            if (pivot - left < right - pivot)                                                                \
            {                                                                                                \
                r_cstl_array_introsort_##Suffix (pBase, left, pivot - 1, depthLimit);                        \
                left = pivot + 1;                                                                            \
            }                                                                                                \
            else                                                                                             \
            {                                                                                                \
                r_cstl_array_introsort_##Suffix (pBase, pivot + 1, right, depthLimit);                       \
                right = pivot - 1;                                                                           \
            }                                                                                                \
        }                                                                                                    \
    }                                                                                                        \
                                                                                                             \
    static void r_cstl_array_introsort_##Suffix##Cb (                                                        \
        Type*                              pBase,                                                            \
        long                               left,                                                             \
        long                               right,                                                            \
        int                                depthLimit,                                                       \
        const r_cstl_array_sort_comparator pCmp,                                                             \
        void*                              pData)                                                            \
    {                                                                                                        \
        while (left < right)                                                                                 \
        {                                                                                                    \
            const long count = right - left + 1;                                                             \
            if (count <= (long)R_CSTL_ARRAY_SORT_INSERTION_THRESHOLD)                                        \
            {                                                                                                \
                r_cstl_array_insertion_sort_##Suffix##Cb (pBase, left, right, pCmp, pData);                  \
                return;                                                                                      \
            }                                                                                                \
            if (depthLimit <= 0)                                                                             \
            {                                                                                                \
                r_cstl_array_heapsort_##Suffix##Cb (pBase, left, right, pCmp, pData);                        \
                return;                                                                                      \
            }                                                                                                \
            const long pivot = r_cstl_array_partition_##Suffix##Cb (pBase, left, right, pCmp, pData);        \
            --depthLimit;                                                                                    \
            if (pivot - left < right - pivot)                                                                \
            {                                                                                                \
                r_cstl_array_introsort_##Suffix##Cb (pBase, left, pivot - 1, depthLimit, pCmp, pData);       \
                left = pivot + 1;                                                                            \
            }                                                                                                \
            else                                                                                             \
            {                                                                                                \
                r_cstl_array_introsort_##Suffix##Cb (pBase, pivot + 1, right, depthLimit, pCmp, pData);      \
                right = pivot - 1;                                                                           \
            }                                                                                                \
        }                                                                                                    \
    }                                                                                                        \
                                                                                                             \
    static void r_cstl_array_sort_##Suffix##_inline (Type* pBase, size_t nelem)                                \
    {                                                                                                        \
        if (nelem <= 1) return;                                                                              \
        const int depthLimit = (int)(2 * r_cstl_array_floor_log2 (nelem));                                   \
        r_cstl_array_introsort_##Suffix (pBase, 0, (long)nelem - 1, depthLimit);                             \
    }                                                                                                        \
                                                                                                             \
    static void r_cstl_array_sort_##Suffix##_callback (                                                      \
        Type*                              pBase,                                                            \
        size_t                             nelem,                                                            \
        const r_cstl_array_sort_comparator pCmp,                                                             \
        void*                              pData)                                                            \
    {                                                                                                        \
        if (nelem <= 1) return;                                                                              \
        const int depthLimit = (int)(2 * r_cstl_array_floor_log2 (nelem));                                   \
        r_cstl_array_introsort_##Suffix##Cb (pBase, 0, (long)nelem - 1, depthLimit, pCmp, pData);            \
    }

R_CSTL_ARRAY_SORT_TYPED (u8, uint8_t)
R_CSTL_ARRAY_SORT_TYPED (u16, uint16_t)
R_CSTL_ARRAY_SORT_TYPED (u32, uint32_t)
R_CSTL_ARRAY_SORT_TYPED (u64, uint64_t)

struct r_cstl_array_stack_frame
{
        long left;
        long right;
        int  depth;
};

#define R_CSTL_ARRAY_INTROSORT_SIMD(Suffix, PartitionFn)                                                     \
    static void r_cstl_array_introsort_u32_##Suffix (uint32_t* pBase, long left, long right, int depthLimit) \
    {                                                                                                        \
        struct r_cstl_array_stack_frame stack[64] = {0}; /* Sufficient for log2(2^64) depth */               \
        int                             stackTop = 0;                                                        \
                                                                                                             \
        stack[stackTop].left = left;                                                                         \
        stack[stackTop].right = right;                                                                       \
        stack[stackTop].depth = depthLimit;                                                                  \
        stackTop++;                                                                                          \
                                                                                                             \
        while (stackTop > 0)                                                                                 \
        {                                                                                                    \
            stackTop--;                                                                                      \
            struct r_cstl_array_stack_frame* stackFrame = &stack[stackTop];                                  \
            left = stackFrame->left;                                                                         \
            right = stackFrame->right;                                                                       \
            depthLimit = stackFrame->depth;                                                                  \
                                                                                                             \
            while (left < right)                                                                             \
            {                                                                                                \
                const long count = right - left + 1;                                                         \
                if (count <= (long)R_CSTL_ARRAY_SORT_INSERTION_THRESHOLD)                                    \
                {                                                                                            \
                    r_cstl_array_insertion_sort_u32 (pBase, left, right);                                    \
                    break;                                                                                   \
                }                                                                                            \
                if (depthLimit <= 0)                                                                         \
                {                                                                                            \
                    r_cstl_array_heapsort_u32 (pBase, left, right);                                          \
                    break;                                                                                   \
                }                                                                                            \
                if (right - left < 2)                                                                        \
                {                                                                                            \
                    if (left < right) r_cstl_array_swap_u32 (pBase + left, pBase + right);                   \
                    break;                                                                                   \
                }                                                                                            \
                const long pivot = PartitionFn (pBase, left, right);                                         \
                --depthLimit;                                                                                \
                                                                                                             \
                if (pivot - left < right - pivot)                                                            \
                {                                                                                            \
                    /* Right partition is larger, push to stack */                                           \
                    if (stackTop < 64)                                                                       \
                    {                                                                                        \
                        stackFrame->left = pivot + 1;                                                        \
                        stackFrame->right = right;                                                           \
                        stackFrame->depth = depthLimit;                                                      \
                        stackTop++;                                                                          \
                    }                                                                                        \
                    right = pivot - 1;                                                                       \
                }                                                                                            \
                else                                                                                         \
                {                                                                                            \
                    /* Left partition is larger, push to stack */                                            \
                    if (stackTop < 64)                                                                       \
                    {                                                                                        \
                        stackFrame->left = left;                                                             \
                        stackFrame->right = pivot - 1;                                                       \
                        stackFrame->depth = depthLimit;                                                      \
                        stackTop++;                                                                          \
                    }                                                                                        \
                    left = pivot + 1;                                                                        \
                }                                                                                            \
            }                                                                                                \
        }                                                                                                    \
    }

#define R_CSTL_ARRAY_SORT_SIMD(Suffix, Introsort)                                                            \
    R_CSTL_API void r_cstl_array_sort_u32_##Suffix (uint32_t* pBase, size_t nelem)                           \
    {                                                                                                        \
        if (nelem < 2) return;                                                                               \
        const int depthLimit = (int)(2 * r_cstl_array_floor_log2 (nelem));                                   \
        Introsort (pBase, 0, (long)nelem - 1, depthLimit);                                                   \
    }

#if defined(R_SIMD_AVX2)
static long
r_cstl_array_partition_u32_avx2 (uint32_t* pBase, long left, long right)
{
    if (right - left < 2) return left;

    long mid = (left + right) >> 1;
    if (mid < left) mid = left;
    if (mid > right) mid = right;

    if (R_CSTL_COMPARE (pBase[mid], pBase[left]) < 0) r_cstl_array_swap_u32 (pBase + mid, pBase + left);
    if (R_CSTL_COMPARE (pBase[right], pBase[left]) < 0) r_cstl_array_swap_u32 (pBase + left, pBase + right);
    if (R_CSTL_COMPARE (pBase[right], pBase[mid]) < 0) r_cstl_array_swap_u32 (pBase + mid, pBase + right);
    r_cstl_array_swap_u32 (pBase + mid, pBase + right - 1);
    const uint32_t pivot = pBase[right - 1];
    long           i = left, j = right - 2;

    __m256i pivotVec = _mm256_set1_epi32 ((int)pivot);
    for (;;)
    {
        while (R_CSTL_LIKELY (i <= j))
        {
            // Check if we have at least 8 elements remaining for SIMD load
            if (R_CSTL_UNLIKELY (i + 8 > j + 1))
            {
                // Fall back to scalar for remaining elements
                while (i <= j && R_CSTL_COMPARE (pBase[i], pivot) < 0)
                    ++i;
                break;
            }
            __m256i dataVec = _mm256_loadu_si256 ((__m256i*)(pBase + i));
            __m256i cmpVec = _mm256_cmpgt_epi32 (dataVec, pivotVec);
            int     mask = _mm256_movemask_epi8 (cmpVec);
            if (R_CSTL_LIKELY (mask == 0)) i += 8;
            else
            {
#if defined(R_COMPILER_MSVC)
                unsigned long tz;
                _BitScanForward (&tz, (unsigned long)mask);
                tz /= 4;
#else
                int tz = __builtin_ctz (mask) / 4;
#endif
                i += tz;
                break;
            }
        }
        while (R_CSTL_LIKELY (i <= j))
        {
            // Check if we have at least 8 elements remaining for SIMD load
            if (R_CSTL_UNLIKELY (j - 7 < i))
            {
                // Fall back to scalar for remaining elements
                while (i <= j && R_CSTL_COMPARE (pBase[j], pivot) > 0)
                    --j;
                break;
            }
            __m256i dataVec = _mm256_loadu_si256 ((__m256i*)(pBase + j - 7));
            __m256i cmpVec = _mm256_cmpgt_epi32 (pivotVec, dataVec);
            int     mask = _mm256_movemask_epi8 (cmpVec);
            if (R_CSTL_LIKELY (mask == 0)) j -= 8;
            else
            {
#if defined(R_COMPILER_MSVC)
                unsigned long tz;
                _BitScanForward (&tz, (unsigned long)mask);
                tz /= 4;
#else
                int tz = __builtin_ctz (mask) / 4;
#endif
                j -= tz;
                break;
            }
        }
        if (R_CSTL_UNLIKELY (i >= j)) break;

        r_cstl_array_swap_u32 (pBase + i, pBase + j);
        ++i;
        --j;
    }
    r_cstl_array_swap_u32 (pBase + i, pBase + right - 1);
    return i;
}

R_CSTL_ARRAY_INTROSORT_SIMD (avx2, r_cstl_array_partition_u32_avx2)
R_CSTL_ARRAY_SORT_SIMD (avx2, r_cstl_array_introsort_u32_avx2)
#endif

#if defined(R_SIMD_SSE)
static long
r_cstl_array_partition_u32_sse (uint32_t* pBase, long left, long right)
{
    if (right - left < 2) return left;
    long mid = (left + right) >> 1;
    if (mid < left) mid = left;
    if (mid > right) mid = right;

    if (R_CSTL_COMPARE (pBase[mid], pBase[left]) < 0) r_cstl_array_swap_u32 (pBase + mid, pBase + left);
    if (R_CSTL_COMPARE (pBase[right], pBase[left]) < 0) r_cstl_array_swap_u32 (pBase + left, pBase + right);
    if (R_CSTL_COMPARE (pBase[right], pBase[mid]) < 0) r_cstl_array_swap_u32 (pBase + mid, pBase + right);
    r_cstl_array_swap_u32 (pBase + mid, pBase + right - 1);
    const uint32_t pivot = pBase[right - 1];
    long           i = left;
    long           j = right - 2;

    __m128i pivotVec = _mm_set1_epi32 ((int)pivot);
    for (;;)
    {
        while (R_CSTL_LIKELY (i <= j))
        {
            // Check if we have at least 4 elements remaining for SIMD load
            if (R_CSTL_UNLIKELY (i + 4 > j + 1))
            {
                // Fall back to scalar for remaining elements
                while (i <= j && R_CSTL_COMPARE (pBase[i], pivot) < 0)
                    ++i;
                break;
            }
            __m128i dataVec = _mm_loadu_si128 ((__m128i*)(pBase + i));
            __m128i cmpVec = _mm_cmpgt_epi32 (dataVec, pivotVec);
            int     mask = _mm_movemask_epi8 (cmpVec);
            if (R_CSTL_LIKELY (mask == 0)) i += 4;
            else
            {
#if defined(R_COMPILER_MSVC)
                unsigned long tz;
                _BitScanForward (&tz, (unsigned long)mask);
                tz /= 4;
#else
                int tz = __builtin_ctz (mask) / 4;
#endif
                i += tz;
                break;
            }
        }
        while (R_CSTL_LIKELY (i <= j))
        {
            // Check if we have at least 4 elements remaining for SIMD load
            if (R_CSTL_UNLIKELY (j - 3 < i))
            {
                // Fall back to scalar for remaining elements
                while (i <= j && R_CSTL_COMPARE (pBase[j], pivot) > 0)
                    --j;
                break;
            }
            __m128i dataVec = _mm_loadu_si128 ((__m128i*)(pBase + j - 3));
            __m128i cmpVec = _mm_cmpgt_epi32 (pivotVec, dataVec);
            int     mask = _mm_movemask_epi8 (cmpVec);
            if (R_CSTL_LIKELY (mask == 0)) j -= 4;
            else
            {
#if defined(R_COMPILER_MSVC)
                unsigned long tz;
                _BitScanForward (&tz, (unsigned long)mask);
                tz /= 4;
#else
                int tz = __builtin_ctz (mask) / 4;
#endif
                j -= tz;
                break;
            }
        }
        if (R_CSTL_UNLIKELY (i >= j)) break;

        r_cstl_array_swap_u32 (pBase + i, pBase + j);
        ++i;
        --j;
    }
    r_cstl_array_swap_u32 (pBase + i, pBase + right - 1);
    return i;
}

R_CSTL_ARRAY_INTROSORT_SIMD (sse, r_cstl_array_partition_u32_sse)
R_CSTL_ARRAY_SORT_SIMD (sse, r_cstl_array_introsort_u32_sse)
#endif

#if defined(_RL_SIMD_ARM_NEON) || defined(R_SIMD_ARM_NEON)
static inline int
R_CSTL_NEON_MASK (uint32x4_t cmpVec)
{
    uint64x2_t mask64 = vreinterpretq_u64_u32 (cmpVec);
    uint64_t   mask = vgetq_lane_u64 (mask64, 0) | vgetq_lane_u64 (mask64, 1);
    return (int)mask;
}

static long
r_cstl_array_partition_u32_neon (uint32_t* pBase, long left, long right)
{
    if (right - left < 2) return left;
    long mid = (left + right) >> 1;
    if (mid < left) mid = left;
    if (mid > right) mid = right;

    if (R_CSTL_COMPARE (pBase[mid], pBase[left]) < 0) r_cstl_array_swap_u32 (pBase + mid, pBase + left);
    if (R_CSTL_COMPARE (pBase[right], pBase[left]) < 0) r_cstl_array_swap_u32 (pBase + left, pBase + right);
    r_cstl_array_swap_u32 (pBase + left, pBase + right);
    if (R_CSTL_COMPARE (pBase[right], pBase[mid]) < 0) r_cstl_array_swap_u32 (pBase + mid, pBase + right);
    r_cstl_array_swap_u32 (pBase + mid, pBase + right - 1);
    const uint32_t pivot = pBase[right - 1];
    long           i = left;
    long           j = right - 2;

    uint32x4_t pivotVec = vdupq_n_u32 (pivot);
    for (;;)
    {
        while (i <= j)
        {
            // Check if we have at least 4 elements remaining for SIMD load
            if (R_CSTL_UNLIKELY (i + 4 > j + 1))
            {
                // Fall back to scalar for remaining elements
                while (i <= j && R_CSTL_COMPARE (pBase[i], pivot) < 0)
                    ++i;
                break;
            }
            uint32x4_t dataVec = vld1q_u32 (pBase + i);
            uint32x4_t cmpVec = vcgtq_u32 (dataVec, pivotVec);
            uint64x2_t mask64 = vreinterpretq_u64_u32 (cmpVec);
            uint64_t   mask = vgetq_lane_u64 (mask64, 0) | vgetq_lane_u64 (mask64, 1);
            if (mask == 0) i += 4;
            else
            {
                int tz = __builtin_ctzll (mask) / 32;
                i += tz;
                break;
            }
        }
        while (i <= j)
        {
            // Check if we have at least 4 elements remaining for SIMD load
            if (R_CSTL_UNLIKELY (j - 3 < i))
            {
                // Fall back to scalar for remaining elements
                while (i <= j && R_CSTL_COMPARE (pBase[j], pivot) > 0)
                    --j;
                break;
            }
            uint32x4_t dataVec = vld1q_u32 (pBase + j - 3);
            uint32x4_t cmpVec = vcgtq_u32 (pivotVec, dataVec);
            uint64x2_t mask64 = vreinterpretq_u64_u32 (cmpVec);
            uint64_t   mask = vgetq_lane_u64 (mask64, 0) | vgetq_lane_u64 (mask64, 1);
            if (mask == 0) j -= 4;
            else
            {
                int tz = __builtin_ctzll (mask) / 32;
                j -= tz;
                break;
            }
        }
        if (i >= j) break;

        r_cstl_array_swap_u32 (pBase + i, pBase + j);
        ++i;
        --j;
    }
    r_cstl_array_swap_u32 (pBase + i, pBase + right - 1);
    return i;
}

R_CSTL_ARRAY_INTROSORT_SIMD (neon, r_cstl_array_partition_u32_neon)
R_CSTL_ARRAY_SORT_SIMD (neon, r_cstl_array_introsort_u32_neon)
#endif

struct r_cstl_array_sort_ctx
{
        uint8_t*                     pBase;
        size_t                       elemSize;
        r_cstl_array_sort_comparator pCmp;
        void*                        pData;
        uint8_t*                     pTmp;
};

R_CSTL_API_ATTR uint8_t*
r_cstl_array_element_bytes (const struct r_cstl_array_sort_ctx* pCtx, long index)
{
    return pCtx->pBase + (size_t)index * pCtx->elemSize;
}

R_CSTL_API_ATTR void
r_cstl_array_copy_element (uint8_t* pDst, const uint8_t* pSrc, size_t elemSize)
{
    if (elemSize == 1u)
    {
        *pDst = *pSrc;
        return;
    }
    if (elemSize == 2u)
    {
        *(uint16_t*)pDst = *(const uint16_t*)pSrc;
        return;
    }
    if (elemSize == 4u)
    {
        *(uint32_t*)pDst = *(const uint32_t*)pSrc;
        return;
    }
    if (elemSize == 8u)
    {
        *(uint64_t*)pDst = *(const uint64_t*)pSrc;
        return;
    }

#if defined(R_SIMD_AVX2)
    size_t off = 0;
    for (; off + 32 <= elemSize; off += 32)
    {
        __m256i v = _mm256_loadu_si256 ((const __m256i*)(pSrc + off));
        _mm256_storeu_si256 ((__m256i*)(pDst + off), v);
    }
    for (; off + 16 <= elemSize; off += 16)
    {
        __m128i v = _mm_loadu_si128 ((const __m128i*)(pSrc + off));
        _mm_storeu_si128 ((__m128i*)(pDst + off), v);
    }
    for (; off < elemSize; ++off)
        pDst[off] = pSrc[off];
#elif defined(R_SIMD_SSE)
    size_t off = 0;
    for (; off + 16 <= elemSize; off += 16)
    {
        __m128i v = _mm_loadu_si128 ((const __m128i*)(pSrc + off));
        _mm_storeu_si128 ((__m128i*)(pDst + off), v);
    }
    for (; off < elemSize; ++off)
        pDst[off] = pSrc[off];
#elif defined(_RL_SIMD_ARM_NEON) || defined(R_SIMD_ARM_NEON)
    size_t off = 0;
    for (; off + 16 <= elemSize; off += 16)
    {
        uint8x16_t v = vld1q_u8 (pSrc + off);
        vst1q_u8 (pDst + off, v);
    }
    for (; off < elemSize; ++off)
        pDst[off] = pSrc[off];
#else
    memcpy (pDst, pSrc, elemSize);
#endif
}

R_CSTL_API_ATTR void
r_cstl_array_swap_elements (uint8_t* a, uint8_t* b, size_t elemSize, uint8_t* tmpBuf)
{
    if (elemSize == 1u)
    {
        uint8_t tmp = *a;
        *a = *b;
        *b = tmp;
        return;
    }
    if (elemSize == 2u)
    {
        uint16_t tmp = *(uint16_t*)a;
        *(uint16_t*)a = *(uint16_t*)b;
        *(uint16_t*)b = tmp;
        return;
    }
    if (elemSize == 4u)
    {
        uint32_t tmp = *(uint32_t*)a;
        *(uint32_t*)a = *(uint32_t*)b;
        *(uint32_t*)b = tmp;
        return;
    }
    if (elemSize == 8u)
    {
        uint64_t tmp = *(uint64_t*)a;
        *(uint64_t*)a = *(uint64_t*)b;
        *(uint64_t*)b = tmp;
        return;
    }

#if defined(R_SIMD_AVX2) || defined(R_SIMD_SSE) || defined(_RL_SIMD_ARM_NEON) || defined(R_SIMD_ARM_NEON)
    if (elemSize < 16u)
    {
        memcpy (tmpBuf, a, elemSize);
        memcpy (a, b, elemSize);
        memcpy (b, tmpBuf, elemSize);
        return;
    }
#endif

#if defined(R_SIMD_AVX2)
    size_t off = 0;
    for (; off + 32 <= elemSize; off += 32)
    {
        __m256i va = _mm256_loadu_si256 ((const __m256i*)(a + off));
        __m256i vb = _mm256_loadu_si256 ((const __m256i*)(b + off));
        _mm256_storeu_si256 ((__m256i*)(a + off), vb);
        _mm256_storeu_si256 ((__m256i*)(b + off), va);
    }
    for (; off + 16 <= elemSize; off += 16)
    {
        __m128i va = _mm_loadu_si128 ((const __m128i*)(a + off));
        __m128i vb = _mm_loadu_si128 ((const __m128i*)(b + off));
        _mm_storeu_si128 ((__m128i*)(a + off), vb);
        _mm_storeu_si128 ((__m128i*)(b + off), va);
    }
    if (off < elemSize)
    {
        size_t tail = elemSize - off;
        memcpy (tmpBuf, a + off, tail);
        memcpy (a + off, b + off, tail);
        memcpy (b + off, tmpBuf, tail);
    }
#elif defined(R_SIMD_SSE)
    size_t off = 0;
    for (; off + 16 <= elemSize; off += 16)
    {
        __m128i va = _mm_loadu_si128 ((const __m128i*)(a + off));
        __m128i vb = _mm_loadu_si128 ((const __m128i*)(b + off));
        _mm_storeu_si128 ((__m128i*)(a + off), vb);
        _mm_storeu_si128 ((__m128i*)(b + off), va);
    }
    if (off < elemSize)
    {
        size_t tail = elemSize - off;
        memcpy (tmpBuf, a + off, tail);
        memcpy (a + off, b + off, tail);
        memcpy (b + off, tmpBuf, tail);
    }
#elif defined(_RL_SIMD_ARM_NEON) || defined(R_SIMD_ARM_NEON)
    size_t off = 0;
    for (; off + 16 <= elemSize; off += 16)
    {
        uint8x16_t va = vld1q_u8 (a + off);
        uint8x16_t vb = vld1q_u8 (b + off);
        vst1q_u8 (a + off, vb);
        vst1q_u8 (b + off, va);
    }
    if (off < elemSize)
    {
        size_t tail = elemSize - off;
        memcpy (tmpBuf, a + off, tail);
        memcpy (a + off, b + off, tail);
        memcpy (b + off, tmpBuf, tail);
    }
#else
    memcpy (tmpBuf, a, elemSize);
    memcpy (a, b, elemSize);
    memcpy (b, tmpBuf, elemSize);
#endif
}

R_CSTL_API_ATTR void
r_cstl_array_insertion_sort (const struct r_cstl_array_sort_ctx* pCtx, long left, long right)
{
    const size_t                 elemSize = pCtx->elemSize;
    uint8_t*                     pKey = pCtx->pTmp;
    r_cstl_array_sort_comparator cmp = pCtx->pCmp;
    void*                        data = pCtx->pData;

    for (long i = left + 1; i <= right; ++i)
    {
        r_cstl_array_copy_element (pKey, r_cstl_array_element_bytes (pCtx, i), elemSize);
        long j = i - 1;
        while (R_CSTL_LIKELY (j >= left) && cmp (r_cstl_array_element_bytes (pCtx, j), pKey, data) > 0)
        {
            r_cstl_array_copy_element (
                r_cstl_array_element_bytes (pCtx, j + 1),
                r_cstl_array_element_bytes (pCtx, j),
                elemSize);
            --j;
        }
        if (R_CSTL_LIKELY (j + 1 != i))
            r_cstl_array_copy_element (r_cstl_array_element_bytes (pCtx, j + 1), pKey, elemSize);
    }
}

R_CSTL_API_ATTR void
r_cstl_array_sift_down (const struct r_cstl_array_sort_ctx* pCtx, long start, long end)
{
    r_cstl_array_sort_comparator cmp = pCtx->pCmp;
    void*                        data = pCtx->pData;
    long                         root = start;
    while (R_CSTL_LIKELY ((root << 1) + 1 <= end))
    {
        long child = (root << 1) + 1;
        if (R_CSTL_LIKELY (child + 1 < end)
            && cmp (
                   r_cstl_array_element_bytes (pCtx, child),
                   r_cstl_array_element_bytes (pCtx, child + 1),
                   data)
                   < 0)
            ++child;

        if (R_CSTL_LIKELY (
                cmp (r_cstl_array_element_bytes (pCtx, root), r_cstl_array_element_bytes (pCtx, child), data)
                >= 0))
            break;

        r_cstl_array_swap_elements (
            r_cstl_array_element_bytes (pCtx, root),
            r_cstl_array_element_bytes (pCtx, child),
            pCtx->elemSize,
            pCtx->pTmp);
        root = child;
    }
}

R_CSTL_API_ATTR void
r_cstl_array_heapsort (const struct r_cstl_array_sort_ctx* pCtx, long left, long right)
{
    for (long start = (left + right) >> 1; start >= left; --start)
        r_cstl_array_sift_down (pCtx, start, right);

    for (long end = right; end > left; --end)
    {
        r_cstl_array_swap_elements (
            r_cstl_array_element_bytes (pCtx, left),
            r_cstl_array_element_bytes (pCtx, end),
            pCtx->elemSize,
            pCtx->pTmp);
        r_cstl_array_sift_down (pCtx, left, end - 1);
    }
}

R_CSTL_API_ATTR long
r_cstl_array_partition (const struct r_cstl_array_sort_ctx* pCtx, long left, long right)
{
    r_cstl_array_sort_comparator cmp = pCtx->pCmp;
    void*                        data = pCtx->pData;
    long                         mid = (left + right) >> 1;

    if (cmp (r_cstl_array_element_bytes (pCtx, mid), r_cstl_array_element_bytes (pCtx, left), data) < 0)
    {
        r_cstl_array_swap_elements (
            r_cstl_array_element_bytes (pCtx, left),
            r_cstl_array_element_bytes (pCtx, mid),
            pCtx->elemSize,
            pCtx->pTmp);
    }
    if (cmp (r_cstl_array_element_bytes (pCtx, right), r_cstl_array_element_bytes (pCtx, left), data) < 0)
    {
        r_cstl_array_swap_elements (
            r_cstl_array_element_bytes (pCtx, left),
            r_cstl_array_element_bytes (pCtx, right),
            pCtx->elemSize,
            pCtx->pTmp);
    }
    if (cmp (r_cstl_array_element_bytes (pCtx, right), r_cstl_array_element_bytes (pCtx, mid), data) < 0)
    {
        r_cstl_array_swap_elements (
            r_cstl_array_element_bytes (pCtx, mid),
            r_cstl_array_element_bytes (pCtx, right),
            pCtx->elemSize,
            pCtx->pTmp);
    }

    r_cstl_array_swap_elements (
        r_cstl_array_element_bytes (pCtx, mid),
        r_cstl_array_element_bytes (pCtx, right - 1),
        pCtx->elemSize,
        pCtx->pTmp);

    uint8_t* pPivot = r_cstl_array_element_bytes (pCtx, right - 1);
    long     i = left;
    long     j = right - 2;

    if (R_CSTL_LIKELY (pCtx->elemSize == 4u))
    {
#if defined(R_SIMD_AVX2)
        uint32_t pivotVal = *(const uint32_t*)pPivot;
        __m256i  pivotVec = _mm256_set1_epi32 ((int)pivotVal);

        for (;;)
        {
            while (R_CSTL_LIKELY (i <= j))
            {
                // Check if we have at least 8 elements remaining for SIMD load
                if (R_CSTL_UNLIKELY (j - i < 7))
                {
                    // Fall back to scalar for remaining elements
                    while (i <= j && cmp (r_cstl_array_element_bytes (pCtx, i), pPivot, data) < 0)
                        ++i;
                    break;
                }
                __m256i dataVec = _mm256_loadu_si256 ((__m256i*)(pCtx->pBase + i * 4));
                __m256i cmpVec = _mm256_cmpgt_epi32 (dataVec, pivotVec);
                int     mask = _mm256_movemask_epi8 (cmpVec);
                if (R_CSTL_LIKELY (mask == 0)) i += 8;
                else
                {
#if defined(R_COMPILER_MSVC)
                    unsigned long tz;
                    _BitScanForward (&tz, (unsigned long)mask);
                    tz /= 4;
#else
                    int tz = __builtin_ctz (mask) / 4;
#endif
                    i += tz;
                    break;
                }
            }
            while (R_CSTL_LIKELY (i <= j))
            {
                // Check if we have at least 8 elements remaining for SIMD load
                if (R_CSTL_UNLIKELY (j - i < 7))
                {
                    // Fall back to scalar for remaining elements
                    while (i <= j && cmp (r_cstl_array_element_bytes (pCtx, j), pPivot, data) > 0)
                        --j;
                    break;
                }
                __m256i dataVec = _mm256_loadu_si256 ((__m256i*)(pCtx->pBase + (j - 7) * 4));
                __m256i cmpVec = _mm256_cmpgt_epi32 (pivotVec, dataVec);
                int     mask = _mm256_movemask_epi8 (cmpVec);
                if (R_CSTL_LIKELY (mask == 0)) j -= 8;
                else
                {
#if defined(R_COMPILER_MSVC)
                    unsigned long tz;
                    _BitScanForward (&tz, (unsigned long)mask);
                    tz /= 4;
#else
                    int tz = __builtin_ctz (mask) / 4;
#endif
                    j -= tz;
                    break;
                }
            }
            if (R_CSTL_UNLIKELY (i >= j)) break;

            r_cstl_array_swap_elements (
                r_cstl_array_element_bytes (pCtx, i),
                r_cstl_array_element_bytes (pCtx, j),
                pCtx->elemSize,
                pCtx->pTmp);
            ++i;
            --j;
        }

        r_cstl_array_swap_elements (
            r_cstl_array_element_bytes (pCtx, i),
            r_cstl_array_element_bytes (pCtx, right - 1),
            pCtx->elemSize,
            pCtx->pTmp);
        return i;
#else
        for (;;)
        {
            while (cmp (r_cstl_array_element_bytes (pCtx, i), pPivot, data) < 0)
                ++i;
            while (cmp (r_cstl_array_element_bytes (pCtx, j), pPivot, data) > 0)
                --j;
            if (i >= j) break;

            r_cstl_array_swap_elements (
                r_cstl_array_element_bytes (pCtx, i),
                r_cstl_array_element_bytes (pCtx, j),
                pCtx->elemSize,
                pCtx->pTmp);
            ++i;
            --j;
        }

        r_cstl_array_swap_elements (
            r_cstl_array_element_bytes (pCtx, i),
            r_cstl_array_element_bytes (pCtx, right - 1),
            pCtx->elemSize,
            pCtx->pTmp);
        return i;
#endif
    }
    else
    {
        for (;;)
        {
            while (cmp (r_cstl_array_element_bytes (pCtx, i), pPivot, data) < 0)
                ++i;
            while (cmp (r_cstl_array_element_bytes (pCtx, j), pPivot, data) > 0)
                --j;
            if (i >= j) break;

            r_cstl_array_swap_elements (
                r_cstl_array_element_bytes (pCtx, i),
                r_cstl_array_element_bytes (pCtx, j),
                pCtx->elemSize,
                pCtx->pTmp);
            ++i;
            --j;
        }

        r_cstl_array_swap_elements (
            r_cstl_array_element_bytes (pCtx, i),
            r_cstl_array_element_bytes (pCtx, right - 1),
            pCtx->elemSize,
            pCtx->pTmp);
        return i;
    }
}

R_CSTL_API void
r_cstl_array_introsort (const struct r_cstl_array_sort_ctx* pCtx, long left, long right, int depthLimit)
{
    struct r_cstl_array_stack_frame  stack[64] = {0}; // Sufficient for log2(2^64) depth
    long                             stackTop = 0;
    struct r_cstl_array_stack_frame* stackFrame = &stack[0];
    stackFrame->left = left;
    stackFrame->right = right;
    stackFrame->depth = depthLimit;
    stackTop++;

    while (R_CSTL_LIKELY (stackTop > 0))
    {
        stackTop--;
        stackFrame = &stack[stackTop];
        left = stackFrame->left;
        right = stackFrame->right;
        depthLimit = stackFrame->depth;

        while (R_CSTL_LIKELY (left < right))
        {
            const long count = right - left + 1;
            if (R_CSTL_LIKELY (count <= (long)R_CSTL_ARRAY_SORT_INSERTION_THRESHOLD))
            {
                r_cstl_array_insertion_sort (pCtx, left, right);
                break;
            }

            if (R_CSTL_UNLIKELY (depthLimit <= 0))
            {
                r_cstl_array_heapsort (pCtx, left, right);
                break;
            }
            long pivot;
            if (R_CSTL_LIKELY (left > 0 && right > left))
            {
                pivot = r_cstl_array_partition (pCtx, left, right);
            }
            else
            {
                pivot = left;
            }
            --depthLimit;

            // Push larger partition onto stack, process smaller one iteratively
            if (R_CSTL_LIKELY (pivot - left < right - pivot))
            {
                // Right partition is larger, push to stack
                if (R_CSTL_LIKELY (stackTop < 64))
                {
                    stackFrame->left = pivot + 1;
                    stackFrame->right = right;
                    stackFrame->depth = depthLimit;
                    stackTop++;
                }
                right = pivot - 1;
            }
            else
            {
                // Left partition is larger, push to stack
                if (R_CSTL_LIKELY (stackTop < 64))
                {
                    stackFrame->left = left;
                    stackFrame->right = pivot - 1;
                    stackFrame->depth = depthLimit;
                    stackTop++;
                }
                left = pivot + 1;
            }
        }
    }
}

R_CSTL_API int
r_cstl_array_sort (
    struct r_cstl_array* pArray,
    uint8_t              elemSize,
    int (*const pComparator) (const void* pLeft, const void* pRight, void* pData),
    void* pData)
{
    if (R_CSTL_UNLIKELY (!pArray || elemSize == 0)) goto cstl_fail;
    if (R_CSTL_UNLIKELY ((pArray->length & (elemSize - 1)) != 0)) goto cstl_fail;

    const size_t nelem = pArray->length / elemSize;
    if (R_CSTL_UNLIKELY (nelem == 0)) return 0;
    if (R_CSTL_UNLIKELY (!r_cstl_array_buffer_is_live (pArray))) goto cstl_fail;
    if (R_CSTL_UNLIKELY (nelem == 1)) return 0;
    uint8_t* pBase = pArray->pData;

    // Most common case: 4-byte elements (uint32_t) with built-in comparator
    if (R_CSTL_LIKELY (elemSize == 4))
    {
        if (R_CSTL_LIKELY (pComparator == r_cstl_array_compare_u32 || pComparator == NULL))
        {
#if defined(R_SIMD_AVX2)
            r_cstl_array_sort_u32_avx2 ((uint32_t*)pBase, nelem);
#elif defined(R_SIMD_SSE)
            r_cstl_array_sort_u32_sse ((uint32_t*)pBase, nelem);
#elif defined(_RL_SIMD_ARM_NEON) || defined(R_SIMD_ARM_NEON)
            r_cstl_array_sort_u32_neon ((uint32_t*)pBase, nelem);
#endif
        }
        else
        {
            r_cstl_array_sort_u32_callback ((uint32_t*)pBase, nelem, pComparator, pData);
        }
        return 0;
    }

    if (elemSize == 1)
    {
        if (pComparator == r_cstl_array_compare_u8)
        {
            r_cstl_array_sort_u8_counting (pBase, nelem);
            return 0;
        }
        if (pComparator == NULL) r_cstl_array_sort_u8_inline (pBase, nelem);
        else r_cstl_array_sort_u8_callback (pBase, nelem, pComparator, pData);
        return 0;
    }

    if (elemSize == 2)
    {
        if (pComparator == r_cstl_array_compare_u16 || pComparator == NULL)
            r_cstl_array_sort_u16_inline ((uint16_t*)pBase, nelem);
        else r_cstl_array_sort_u16_callback ((uint16_t*)pBase, nelem, pComparator, pData);
        return 0;
    }

    if (elemSize == 8)
    {
        if (pComparator == r_cstl_array_compare_u64 || pComparator == NULL)
            r_cstl_array_sort_u64_inline ((uint64_t*)pBase, nelem);
        else r_cstl_array_sort_u64_callback ((uint64_t*)pBase, nelem, pComparator, pData);
        return 0;
    }

    uint8_t*                     pTmp = (uint8_t*)R_CSTL_STACK_ALLOC (elemSize);
    struct r_cstl_array_sort_ctx ctx = {
        .pBase = pBase,
        .elemSize = elemSize,
        .pCmp = pComparator,
        .pData = pData,
        .pTmp = pTmp,
    };
    const int depthLimit = (int)(2 * r_cstl_array_floor_log2 (nelem));
    r_cstl_array_introsort (&ctx, 0, (long)nelem - 1, depthLimit);
    return 0;
cstl_fail:
    return -1;
}

R_CSTL_API size_t
r_cstl_array_length (const struct r_cstl_array* pArray)
{
    if (!pArray) return 0;
    return pArray->length;
}

R_CSTL_API size_t
r_cstl_array_get_capacity (const struct r_cstl_array* pArray)
{
    if (!pArray) return 0;
    return pArray->capacity;
}

R_CSTL_API int
r_cstl_array_at (const struct r_cstl_array* pArray, size_t index, uint8_t* pOutValue)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pArray || !pOutValue) goto cstl_fail;
    if (!r_cstl_array_buffer_is_live (pArray)) goto cstl_fail;
#endif
    if (index >= pArray->length) goto cstl_fail;
    *pOutValue = pArray->pData[index];
    return 0;
cstl_fail:
    return -1;
}

R_CSTL_API int
r_cstl_array_unchecked_at (const struct r_cstl_array* pArray, size_t index, uint8_t* pOutValue)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pArray || !pOutValue) goto cstl_fail;
    if (!r_cstl_array_buffer_is_live (pArray)) goto cstl_fail;
#endif
    *pOutValue = pArray->pData[index];
    return 0;
cstl_fail:
    return -1;
}
