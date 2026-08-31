#include "rlgame.base/cstl/cstl_string.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"

#define R_CSTL_INLINE
#include "rlgame.base/cstl/cstl_platform.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#if defined(R_SIMD_AVX2)
#include <immintrin.h>
#elif defined(R_SIMD_SSE)
#include <immintrin.h>
#elif defined(_RL_SIMD_ARM_NEON) || defined(R_SIMD_ARM_NEON)
#include <arm_neon.h>
#endif

#if defined(_MSC_VER)
#include <intrin.h>

R_CSTL_API_ATTR static unsigned int
r_cstl_count_trailing_zeros (unsigned int value)
{
    unsigned long index;
    if (_BitScanForward (&index, value)) return (unsigned int)index;
    return 32;
}
R_CSTL_API_ATTR static unsigned int
r_cstl_count_trailing_zeros64 (unsigned long long value)
{
    unsigned long index;
    if (_BitScanForward64 (&index, value)) return (unsigned int)index;
    return 64;
}
#define R_CSTL_CTZ(v)   r_cstl_count_trailing_zeros (v)
#define R_CSTL_CTZLL(v) r_cstl_count_trailing_zeros64 (v)
#else
#define R_CSTL_CTZ(v)   __builtin_ctz (v)
#define R_CSTL_CTZLL(v) __builtin_ctzll (v)
#endif
#define R_CSTL_STRING_SBO_SIZE 24

#define R_CSTL_STORAGE_FLAG_BIT         ((size_t)1 << (sizeof (size_t) * 8 - 1))
#define R_CSTL_STORAGE_HEAP_FLAG        R_CSTL_STORAGE_FLAG_BIT
#define R_CSTL_STORAGE_SBO_FLAG         0
#define R_CSTL_GET_CAPACITY(cap)        ((cap) & ~R_CSTL_STORAGE_FLAG_BIT)
#define R_CSTL_SET_CAPACITY(cap, value) (((cap) & R_CSTL_STORAGE_FLAG_BIT) | (value))
#define R_CSTL_SET_STORAGE_HEAP(cap)    ((cap) | R_CSTL_STORAGE_HEAP_FLAG)
#define R_CSTL_SET_STORAGE_SBO(cap)     ((cap) & ~R_CSTL_STORAGE_HEAP_FLAG)
#define R_CSTL_IS_STORAGE_HEAP(cap)     (((cap) & R_CSTL_STORAGE_FLAG_BIT) != 0)
#define R_CSTL_IS_STORAGE_SBO(cap)      (((cap) & R_CSTL_STORAGE_FLAG_BIT) == 0)

struct r_cstl_string
{
        size_t length;
        size_t capacity;
        union
        {
                char* pData;
                char  sbo[R_CSTL_STRING_SBO_SIZE];
        };
};

struct r_cstl_string_builder
{
        size_t length;
        size_t capacity;
        union
        {
                char* pData;
                char  sbo[R_CSTL_STRING_SBO_SIZE];
        };
};

static void
r_cstl_string_copy_bytes (char* R_CSTL_RESTRICT pDst, const char* R_CSTL_RESTRICT pSrc, const size_t sizeBytes)
{
    if (sizeBytes == 0 || pDst == pSrc) return;

#if defined(R_SIMD_AVX2)
    if (sizeBytes >= 64)
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
    if (sizeBytes >= 32)
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
    if (sizeBytes >= 32)
    {
        size_t i = 0;
        for (; i + 16 <= sizeBytes; i += 16)
        {
            uint8x16_t v = vld1q_u8 ((const uint8_t*)(pSrc + i));
            vst1q_u8 ((uint8_t*)(pDst + i), v);
        }
        for (; i < sizeBytes; ++i)
            pDst[i] = pSrc[i];
        return;
    }
#endif
    memcpy (pDst, pSrc, sizeBytes);
}

static void
r_cstl_string_set_bytes (char* R_CSTL_RESTRICT pDst, char value, const size_t sizeBytes)
{
    if (sizeBytes == 0) return;

#if defined(R_SIMD_AVX2)
    if (sizeBytes >= 32)
    {
        __m256i vec = _mm256_set1_epi8 (value);
        size_t  i = 0;
        for (; i + 32 <= sizeBytes; i += 32)
        {
            _mm256_storeu_si256 ((__m256i*)(pDst + i), vec);
        }
        for (; i < sizeBytes; ++i)
            pDst[i] = value;
        return;
    }
#elif defined(R_SIMD_SSE)
    if (sizeBytes >= 16)
    {
        __m128i vec = _mm_set1_epi8 (value);
        size_t  i = 0;
        for (; i + 16 <= sizeBytes; i += 16)
        {
            _mm_storeu_si128 ((__m128i*)(pDst + i), vec);
        }
        for (; i < sizeBytes; ++i)
            pDst[i] = value;
        return;
    }
#elif defined(_RL_SIMD_ARM_NEON) || defined(R_SIMD_ARM_NEON)
    if (sizeBytes >= 16)
    {
        uint8x16_t vec = vdupq_n_u8 ((uint8_t)value);
        size_t     i = 0;
        for (; i + 16 <= sizeBytes; i += 16)
        {
            vst1q_u8 ((uint8_t*)(pDst + i), vec);
        }
        for (; i < sizeBytes; ++i)
            pDst[i] = value;
        return;
    }
#endif
    memset (pDst, value, sizeBytes);
}

R_CSTL_API_ATTR static int
r_cstl_string_buffer_is_live (const struct r_cstl_string* pString)
{
    if (!pString) return 0;
#ifndef R_CSTL_HEAP_DEBUG
    return 1;
#else
    if (R_CSTL_IS_STORAGE_HEAP (pString->capacity))
    {
        if (!pString->pData) return 0;
        return r_cstl_heap_is_valid_pointer (pString->pData) != 0;
    }
    // SBO is always valid if the string itself is valid
    return 1;
#endif
}

static size_t
r_cstl_string_find_char (const char* pData, size_t length, char ch)
{
    if (length == 0) return (size_t)-1;

#if defined(R_SIMD_AVX2)
    if (length >= 32)
    {
        __m256i target = _mm256_set1_epi8 (ch);
        size_t  i = 0;
        for (; i + 32 <= length; i += 32)
        {
            __m256i data = _mm256_loadu_si256 ((const __m256i*)(pData + i));
            __m256i cmp = _mm256_cmpeq_epi8 (data, target);
            int     mask = _mm256_movemask_epi8 (cmp);
            if (mask != 0)
            {
                return i + R_CSTL_CTZ (mask);
            }
        }
        for (; i < length; ++i)
        {
            if (pData[i] == ch) return i;
        }
        return (size_t)-1;
    }
#elif defined(R_SIMD_SSE)
    if (length >= 16)
    {
        __m128i target = _mm_set1_epi8 (ch);
        size_t  i = 0;
        for (; i + 16 <= length; i += 16)
        {
            __m128i data = _mm_loadu_si128 ((const __m128i*)(pData + i));
            __m128i cmp = _mm_cmpeq_epi8 (data, target);
            int     mask = _mm_movemask_epi8 (cmp);
            if (mask != 0)
            {
                return i + R_CSTL_CTZ (mask);
            }
        }
        for (; i < length; ++i)
        {
            if (pData[i] == ch) return i;
        }
        return (size_t)-1;
    }
#elif defined(_RL_SIMD_ARM_NEON) || defined(R_SIMD_ARM_NEON)
    if (length >= 16)
    {
        uint8x16_t target = vdupq_n_u8 ((uint8_t)ch);
        size_t     i = 0;
        for (; i + 16 <= length; i += 16)
        {
            uint8x16_t data = vld1q_u8 ((const uint8_t*)(pData + i));
            uint8x16_t cmp = vceqq_u8 (data, target);
            uint64_t   mask = vget_lane_u64 (vreinterpretq_u64_u8 (cmp), 0);
            if (mask != 0)
            {
                return i + R_CSTL_CTZLL (mask);
            }
            mask = vget_lane_u64 (vreinterpretq_u64_u8 (cmp), 1);
            if (mask != 0)
            {
                return i + 8 + R_CSTL_CTZLL (mask);
            }
        }
        for (; i < length; ++i)
        {
            if (pData[i] == ch) return i;
        }
        return (size_t)-1;
    }
#endif
    for (size_t i = 0; i < length; ++i)
    {
        if (pData[i] == ch) return i;
    }
    return (size_t)-1;
}

static size_t
r_cstl_string_find_char_reverse (const char* pData, size_t length, char ch)
{
    if (length == 0) return (size_t)-1;

    for (size_t i = length; i > 0; --i)
    {
        if (pData[i - 1] == ch) return i - 1;
    }
    return (size_t)-1;
}

static void
r_cstl_string_reverse_bytes (char* pData, size_t length)
{
    if (length <= 1) return;

    size_t i = 0;
    size_t j = length - 1;
    while (i < j)
    {
        char temp = pData[i];
        pData[i] = pData[j];
        pData[j] = temp;
        ++i;
        --j;
    }
}

R_CSTL_API_ATTR static char
r_cstl_to_lower_char (char c)
{
    return (c >= 'A' && c <= 'Z') ? (c + 32) : c;
}

R_CSTL_API_ATTR static char
r_cstl_to_upper_char (char c)
{
    return (c >= 'a' && c <= 'z') ? (c - 32) : c;
}

#define R_CSTL_STRING_HEAP_ALLOC_INITIAL_CAPACITY 128
R_CSTL_API_ATTR static size_t
r_cstl_string_next_capacity (size_t current, const size_t required)
{
    size_t next = current ? R_CSTL_GET_CAPACITY (current) : R_CSTL_STRING_SBO_SIZE;
    while (next < required)
    {
        if (next > (SIZE_MAX / 2)) return required;
        next <<= 1;
    }
    return next;
}

static int
r_cstl_string_ensure_capacity_internal (struct r_cstl_string* pString, const size_t required)
{
    if (!pString) goto cstl_fail;
    if (!r_cstl_string_buffer_is_live (pString)) goto cstl_fail;
    if (R_CSTL_GET_CAPACITY (pString->capacity) >= required) return 0;

    size_t newCap = r_cstl_string_next_capacity (R_CSTL_GET_CAPACITY (pString->capacity), required);
    char*  pOld = R_CSTL_IS_STORAGE_HEAP (pString->capacity) ? pString->pData : NULL;

    if (pOld)
    {
        char* pNew = (char*)r_cstl_heap_realloc (pOld, newCap + 1);
        if (!pNew) return -1;
#if defined(R_CSTL_HEAP_DEBUG)
        if (pNew != pOld)
        {
            uint64_t h = r_cstl_heap_register_allocation (
                pString,
                pNew,
                newCap + 1,
                R_CSTL_HEAP_NAME (r_cstl_string_ensure_capacity_internal));
            if (h == 0) goto cstl_fail_register;
        }
#endif
        pString->pData = pNew;
        pString->capacity = R_CSTL_SET_STORAGE_HEAP (newCap);
        return 0;
    }
    else
    {
        char* pNew = (char*)r_cstl_heap_alloc (newCap + 1);
        if (!pNew) return -1;
#if defined(R_CSTL_HEAP_DEBUG)
        uint64_t h = r_cstl_heap_register_allocation (
            pString,
            pNew,
            newCap + 1,
            R_CSTL_HEAP_NAME (r_cstl_string_ensure_capacity_internal));
        if (h == 0) goto cstl_fail_register;
#endif
        r_cstl_string_copy_bytes (pNew, pString->sbo, pString->length);
        pNew[pString->length] = '\0';
        pString->pData = pNew;
        pString->capacity = R_CSTL_SET_STORAGE_HEAP (newCap);
        return 0;
    }

cstl_fail_register:
#if defined(R_CSTL_HEAP_DEBUG)
    if (pOld) r_cstl_heap_unregister_allocation (pString, pOld);
#endif
    return -1;
cstl_fail:
    return -1;
}

static void
r_cstl_string_release_buffer (struct r_cstl_string* pString)
{
    if (!pString) goto cstl_fail;
    if (!r_cstl_string_buffer_is_live (pString)) goto cstl_fail;

#if defined(R_CSTL_HEAP_DEBUG)
    if (R_CSTL_IS_STORAGE_HEAP (pString->capacity))
    {
        if (!r_cstl_heap_is_valid_pointer (pString->pData)) goto cstl_fail;
        r_cstl_heap_unregister_allocation (pString, pString->pData);
    }
#endif

    if (R_CSTL_IS_STORAGE_HEAP (pString->capacity)) r_cstl_heap_free (pString->pData);
    pString->pData = NULL;
    pString->capacity = 0;
    pString->length = 0;
    return;
cstl_fail:
    return;
}

R_CSTL_API_ATTR static struct r_cstl_string*
r_cstl_string_create_shell (void)
{
    struct r_cstl_string* pString = (struct r_cstl_string*)r_cstl_heap_alloc (sizeof (struct r_cstl_string));
    if (!pString) return NULL;
    pString->length = 0;
    pString->capacity = R_CSTL_SET_STORAGE_SBO (R_CSTL_STRING_SBO_SIZE);
    pString->sbo[0] = '\0';
    return pString;
}

R_CSTL_API_ATTR struct r_cstl_string*
r_cstl_new_string (void)
{
    return r_cstl_string_create_shell ();
}

#define R_CSTL_STRING_STRLEN(s)                                                                              \
    const char* pDataString = (const char*)pData;                                                            \
    while (*pDataString != '\0')                                                                             \
        ++pDataString;                                                                                       \
    const size_t length = pDataString - (const char*)pData

struct r_cstl_string*
r_cstl_new_string_with_data_sized (const char* pData, const size_t length)
{
    if (!pData) goto cstl_fail;
    struct r_cstl_string* pString = r_cstl_string_create_shell ();
    if (!pString) goto cstl_fail;
    if (!r_cstl_string_buffer_is_live (pString)) goto cstl_fail;

    if (length < R_CSTL_STRING_SBO_SIZE)
    {
        r_cstl_string_copy_bytes (pString->sbo, pData, length);
        pString->sbo[length] = '\0';
        pString->length = length;
        pString->capacity = R_CSTL_SET_STORAGE_SBO (R_CSTL_STRING_SBO_SIZE);
        return pString;
    }
    size_t cap = r_cstl_string_next_capacity (R_CSTL_STRING_SBO_SIZE, length);
    char*  mem = (char*)r_cstl_heap_alloc (cap + 1);
    if (!mem)
    {
        r_cstl_heap_free (pString);
        return NULL;
    }
#if defined(R_CSTL_HEAP_DEBUG)
    uint64_t h
        = r_cstl_heap_register_allocation (pString, mem, cap + 1, R_CSTL_HEAP_NAME (r_cstl_new_string_with_data));
    if (h == 0)
    {
        r_cstl_heap_free (mem);
        r_cstl_heap_free (pString);
        return NULL;
    }
#endif
    r_cstl_string_copy_bytes (mem, pData, length);
    mem[length] = '\0';
    pString->pData = mem;
    pString->length = length;
    pString->capacity = R_CSTL_SET_STORAGE_HEAP (cap);
    return pString;
cstl_fail:
    return NULL;
}

struct r_cstl_string*
r_cstl_new_string_with_capacity (const size_t cap)
{
    struct r_cstl_string* pString
        = (struct r_cstl_string*)r_cstl_heap_alloc (sizeof (struct r_cstl_string) + cap);
    if (!pString) return NULL;
    char* mem = (char*)r_cstl_heap_alloc (cap + 1);
    if (!mem)
    {
        r_cstl_heap_free (pString);
        return NULL;
    }
#if defined(R_CSTL_HEAP_DEBUG)
    uint64_t h
        = r_cstl_heap_register_allocation (pString, mem, cap + 1, R_CSTL_HEAP_NAME (r_cstl_new_string_with_data));
    if (h == 0)
    {
        r_cstl_heap_free (mem);
        r_cstl_heap_free (pString);
        return NULL;
    }
#endif
    mem[0] = '\0';
    pString->length = 0;
    pString->capacity = R_CSTL_SET_STORAGE_HEAP (cap);
    pString->pData = mem;
    return pString;
}

R_CSTL_API_ATTR struct r_cstl_string*
r_cstl_new_string_with_data (const char* pData)
{
    if (!pData) return NULL;
    R_CSTL_STRING_STRLEN (pData);
    return r_cstl_new_string_with_data_sized (pData, length);
}

struct r_cstl_string*
r_cstl_new_string_with_format (const char* pFormat, ...)
{
    if (!pFormat) return r_cstl_new_string ();
    va_list args;
    va_start (args, pFormat);
    va_list copy;
    va_copy (copy, args);
    int needed = vsnprintf (NULL, 0, pFormat, copy);
    va_end (copy);
    if (needed < 0)
    {
        va_end (args);
        return NULL;
    }
    size_t len = (size_t)needed;
    char*  buf = (char*)r_cstl_heap_alloc (len + 1);
    if (!buf)
    {
        va_end (args);
        return NULL;
    }
    vsnprintf (buf, len + 1, pFormat, args);
    va_end (args);
    struct r_cstl_string* pString = r_cstl_new_string_with_data_sized (buf, len);
    r_cstl_heap_free (buf);
    return pString;
}

R_CSTL_API_ATTR void
r_cstl_string_delete (struct r_cstl_string* pString)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pString) goto cstl_fail;
    if (!r_cstl_string_buffer_is_live (pString)) goto cstl_fail;
#endif
    r_cstl_string_release_buffer (pString);
    r_cstl_heap_free (pString);
    r_cstl_heap_unregister_allocation (pString, pString->pData);
cstl_fail:
    return;
}

R_CSTL_API_ATTR size_t
r_cstl_string_length (const struct r_cstl_string* pString)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pString) goto cstl_fail;
    if (!r_cstl_string_buffer_is_live (pString)) goto cstl_fail;
#endif
    return pString->length;
cstl_fail:
    return 0;
}

R_CSTL_API_ATTR const char*
r_cstl_string_data (const struct r_cstl_string* pString)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pString) goto cstl_fail;
    if (!r_cstl_string_buffer_is_live (pString)) goto cstl_fail;
#endif
    if (R_CSTL_IS_STORAGE_SBO (pString->capacity)) return pString->sbo;
    return pString->pData;
cstl_fail:
    return NULL;
}

R_CSTL_API_ATTR char
r_cstl_string_char_at (const struct r_cstl_string* pString, const size_t index)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pString) goto cstl_fail;
    if (!r_cstl_string_buffer_is_live (pString)) goto cstl_fail;
    if (index >= pString->length) goto cstl_fail;
#endif
    if (R_CSTL_IS_STORAGE_SBO (pString->capacity)) return pString->sbo[index];
    return pString->pData[index];
cstl_fail:
    return 0x00;
}

R_CSTL_API_ATTR int
r_cstl_string_equals (const struct r_cstl_string* pLeft, const struct r_cstl_string* pRight)
{
    if (pLeft == pRight) return 1;
    if (!pLeft || !pRight) return 0;
    if (pLeft->length != pRight->length) return 0;
    return memcmp (r_cstl_string_data (pLeft), r_cstl_string_data (pRight), pLeft->length) == 0;
}

int
r_cstl_string_compare (const struct r_cstl_string* pLeft, const struct r_cstl_string* pRight)
{
    if (pLeft == pRight) return 0;
    if (!pLeft) return -1;
    if (!pRight) return 1;
    size_t n = pLeft->length < pRight->length ? pLeft->length : pRight->length;
    int    cmp = memcmp (r_cstl_string_data (pLeft), r_cstl_string_data (pRight), n);
    if (cmp != 0) return cmp;
    if (pLeft->length < pRight->length) return -1;
    if (pLeft->length > pRight->length) return 1;
    return 0;
}

struct r_cstl_string*
r_cstl_string_concat (const struct r_cstl_string* pLeft, const struct r_cstl_string* pRight)
{
    if (!pLeft && !pRight) return r_cstl_new_string ();
    if (!pLeft)
    {
        if (!r_cstl_string_buffer_is_live (pRight)) return NULL;
        return r_cstl_new_string_with_data_sized (r_cstl_string_data (pRight), pRight->length);
    }
    if (!pRight)
    {
        if (!r_cstl_string_buffer_is_live (pLeft)) return NULL;
        return r_cstl_new_string_with_data_sized (r_cstl_string_data (pLeft), pLeft->length);
    }
    if (!r_cstl_string_buffer_is_live (pLeft) || !r_cstl_string_buffer_is_live (pRight)) return NULL;
    size_t                total = pLeft->length + pRight->length;
    struct r_cstl_string* pString = r_cstl_string_create_shell ();
    if (!pString) return NULL;
    const char* pLeftData = r_cstl_string_data (pLeft);
    const char* pRightData = r_cstl_string_data (pRight);
    if (total <= R_CSTL_STRING_SBO_SIZE)
    {
        r_cstl_string_copy_bytes (pString->sbo, pLeftData, pLeft->length);
        r_cstl_string_copy_bytes (pString->sbo + pLeft->length, pRightData, pRight->length);
        pString->sbo[total] = '\0';
        pString->length = total;
        return pString;
    }
    size_t cap = r_cstl_string_next_capacity (R_CSTL_STRING_SBO_SIZE, total);
    char*  mem = (char*)r_cstl_heap_alloc (cap + 1);
    if (!mem)
    {
        r_cstl_heap_free (pString);
        return NULL;
    }
#if defined(R_CSTL_HEAP_DEBUG)
    uint64_t h
        = r_cstl_heap_register_allocation (pString, mem, cap + 1, R_CSTL_HEAP_NAME (r_cstl_string_concat));
    if (h == 0)
    {
        r_cstl_heap_free (mem);
        r_cstl_heap_free (pString);
        return NULL;
    }
#endif
    r_cstl_string_copy_bytes (mem, pLeftData, pLeft->length);
    r_cstl_string_copy_bytes (mem + pLeft->length, pRightData, pRight->length);
    mem[total] = '\0';
    pString->pData = mem;
    pString->length = total;
    pString->capacity = R_CSTL_SET_STORAGE_HEAP (cap);
    return pString;
}

struct r_cstl_string*
r_cstl_string_substring (const struct r_cstl_string* pString, size_t begin, size_t end)
{
    if (!pString) return NULL;
    if (!r_cstl_string_buffer_is_live (pString)) return NULL;
    if (begin >= pString->length || begin >= end) return r_cstl_new_string ();
    if (end > pString->length) end = pString->length;
    size_t      len = end - begin;
    const char* pData = r_cstl_string_data (pString);
    return r_cstl_new_string_with_data_sized (pData + begin, len);
}

R_CSTL_API_ATTR int
r_cstl_string_starts_with (const struct r_cstl_string* pString, const char* pPrefix)
{
    if (!pString || !pPrefix) return 0;
    size_t plen = strlen (pPrefix);
    if (plen > pString->length) return 0;
    return memcmp (r_cstl_string_data (pString), pPrefix, plen) == 0;
}

R_CSTL_API_ATTR int
r_cstl_string_ends_with (const struct r_cstl_string* pString, const char* pSuffix)
{
    if (!pString || !pSuffix) return 0;
    size_t slen = strlen (pSuffix);
    if (slen > pString->length) return 0;
    const char* pData = r_cstl_string_data (pString);
    return memcmp (pData + (pString->length - slen), pSuffix, slen) == 0;
}

R_CSTL_API_ATTR size_t
r_cstl_string_index_of (const struct r_cstl_string* pString, const char* pNeedle)
{
    if (!pString || !pNeedle) return (size_t)-1;
    if (!r_cstl_string_buffer_is_live (pString)) return (size_t)-1;
    size_t nlen = strlen (pNeedle);
    if (nlen == 0) return 0;
    if (nlen > pString->length) return (size_t)-1;

    const char* pHay = r_cstl_string_data (pString);
    size_t      haylen = pString->length;
    const char  needle0 = pNeedle[0];
    for (size_t i = 0; i + nlen <= haylen; ++i)
    {
        if (pHay[i] != needle0) continue;
        if (memcmp (pHay + i, pNeedle, nlen) == 0) return i;
    }
    return (size_t)-1;
}

size_t
r_cstl_string_last_index_of (const struct r_cstl_string* pString, const char* pNeedle)
{
    if (!pString || !pNeedle) return (size_t)-1;
    if (!r_cstl_string_buffer_is_live (pString)) return (size_t)-1;
    size_t nlen = strlen (pNeedle);
    if (nlen == 0) return pString->length;
    if (nlen > pString->length) return (size_t)-1;

    const char* pHay = r_cstl_string_data (pString);
    size_t      haylen = pString->length;
    for (size_t i = haylen - nlen + 1; i > 0; --i)
    {
        if (memcmp (pHay + (i - 1), pNeedle, nlen) == 0) return i - 1;
    }
    return (size_t)-1;
}

size_t
r_cstl_string_index_of_char (const struct r_cstl_string* pString, char ch)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pString) goto cstl_fail;
    if (!r_cstl_string_buffer_is_live (pString)) goto cstl_fail;
#endif
    return r_cstl_string_find_char (r_cstl_string_data (pString), pString->length, ch);
cstl_fail:
    return (size_t)-1;
}

R_CSTL_API_ATTR size_t
r_cstl_string_last_index_of_char (const struct r_cstl_string* pString, char ch)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pString) goto cstl_fail;
    if (!r_cstl_string_buffer_is_live (pString)) goto cstl_fail;
#endif
    return r_cstl_string_find_char_reverse (r_cstl_string_data (pString), pString->length, ch);
cstl_fail:
    return (size_t)-1;
}

R_CSTL_API_ATTR int
r_cstl_string_contains (const struct r_cstl_string* pString, const char* pNeedle)
{
    return r_cstl_string_index_of (pString, pNeedle) != (size_t)-1;
}

R_CSTL_API_ATTR int
r_cstl_string_is_empty (const struct r_cstl_string* pString)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pString) goto cstl_fail;
    if (!r_cstl_string_buffer_is_live (pString)) goto cstl_fail;
#endif
    return pString->length == 0;
cstl_fail:
    return -1;
}

struct r_cstl_string*
r_cstl_string_to_lower_case (const struct r_cstl_string* pString)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pString) return r_cstl_new_string ();
    if (!r_cstl_string_buffer_is_live (pString)) goto cstl_fail;
#endif
    const char*           pData = r_cstl_string_data (pString);
    size_t                len = pString->length;
    struct r_cstl_string* pResult = r_cstl_new_string_with_data_sized (pData, len);
    if (!pResult) return NULL;
    char* pDst = (char*)r_cstl_string_data (pResult);
    for (size_t i = 0; i < len; ++i)
        pDst[i] = r_cstl_to_lower_char (pDst[i]);
    return pResult;
cstl_fail:
    return NULL;
}

struct r_cstl_string*
r_cstl_string_to_upper_case (const struct r_cstl_string* pString)
{
    if (!pString) return r_cstl_new_string ();
    const char*           pData = r_cstl_string_data (pString);
    size_t                len = pString->length;
    struct r_cstl_string* pResult = r_cstl_new_string_with_data_sized (pData, len);
    if (!pResult) return NULL;
    char* pDst = (char*)r_cstl_string_data (pResult);
    for (size_t i = 0; i < len; ++i)
        pDst[i] = r_cstl_to_upper_char (pDst[i]);
    return pResult;
}

struct r_cstl_string*
r_cstl_string_trim (const struct r_cstl_string* pString)
{
    if (!pString) return r_cstl_new_string ();
    if (!r_cstl_string_buffer_is_live (pString)) goto cstl_fail;
    const char* pData = r_cstl_string_data (pString);
    size_t      len = pString->length;
    size_t      start = 0;
    while (start < len
           && (pData[start] == ' ' || pData[start] == '\t' || pData[start] == '\n' || pData[start] == '\r'))
        ++start;
    size_t end = len;
    while (end > start
           && (pData[end - 1] == ' ' || pData[end - 1] == '\t' || pData[end - 1] == '\n'
               || pData[end - 1] == '\r'))
        --end;
    return r_cstl_string_substring (pString, start, end);
cstl_fail:
    return NULL;
}

struct r_cstl_string*
r_cstl_string_replace (const struct r_cstl_string* pString, const char* pTarget, const char* pReplacement)
{
    if (!pString || !pTarget || !pReplacement)
    {
        if (pString && r_cstl_string_buffer_is_live (pString))
            return r_cstl_new_string_with_data_sized (r_cstl_string_data (pString), pString->length);
        return r_cstl_new_string ();
    }
    if (!r_cstl_string_buffer_is_live (pString)) return NULL;
    size_t tlen = strlen (pTarget);
    if (tlen == 0) return r_cstl_new_string_with_data_sized (r_cstl_string_data (pString), pString->length);
    size_t rlen = strlen (pReplacement);

    struct r_cstl_string_builder* pBuilder = r_cstl_new_string_builder ();
    if (!pBuilder) return NULL;

    const char* pData = r_cstl_string_data (pString);
    size_t      len = pString->length;
    size_t      pos = 0;
    size_t      found = r_cstl_string_index_of (pString, pTarget);

    while (found != (size_t)-1)
    {
        r_cstl_string_builder_append_data (pBuilder, pData + pos, found - pos);
        r_cstl_string_builder_emplace (pBuilder, pReplacement);
        pos = found + tlen;
        struct r_cstl_string* pStringTemp = r_cstl_string_substring (pString, pos, len);
        found = r_cstl_string_index_of (pStringTemp, pTarget);
        r_cstl_string_delete (pStringTemp);
    }
    r_cstl_string_builder_append_data (pBuilder, pData + pos, len - pos);

    struct r_cstl_string* pResult = r_cstl_string_builder_to_string (pBuilder);
    r_cstl_delete_string_builder (pBuilder);
    return pResult;
}

struct r_cstl_string*
r_cstl_string_replace_char (const struct r_cstl_string* pString, char oldChar, char newChar)
{
    if (!pString) return r_cstl_new_string ();
    if (!r_cstl_string_buffer_is_live (pString)) goto cstl_fail;
    const char*           pData = r_cstl_string_data (pString);
    size_t                len = pString->length;
    struct r_cstl_string* pResult = r_cstl_new_string_with_data_sized (pData, len);
    if (!pResult) return NULL;
    char* pDst = (char*)r_cstl_string_data (pResult);
    for (size_t i = 0; i < len; ++i)
    {
        if (pDst[i] == oldChar) pDst[i] = newChar;
    }
    return pResult;
cstl_fail:
    return NULL;
}

struct r_cstl_string*
r_cstl_string_repeat (struct r_cstl_string* pString, size_t count)
{
    if (!pString || count == 0) return r_cstl_new_string ();
    if (!r_cstl_string_buffer_is_live (pString)) return NULL;
    if (count == 1) return r_cstl_new_string_with_data_sized (r_cstl_string_data (pString), pString->length);

    size_t len = pString->length;
    if (len == 0) return r_cstl_new_string ();

    size_t total = len * count;
    if (total < len) return NULL;

    struct r_cstl_string* pResult = r_cstl_string_create_shell ();
    if (!pResult) return NULL;

    if (total <= R_CSTL_STRING_SBO_SIZE)
    {
        for (size_t i = 0; i < count; ++i)
            r_cstl_string_copy_bytes (pResult->sbo + (i * len), r_cstl_string_data (pString), len);
        pResult->sbo[total] = '\0';
        pResult->length = total;
        return pResult;
    }

    size_t cap = r_cstl_string_next_capacity (R_CSTL_STRING_SBO_SIZE, total);
    char*  mem = (char*)r_cstl_heap_alloc (cap + 1);
    if (!mem)
    {
        r_cstl_heap_free (pResult);
        return NULL;
    }
#if defined(R_CSTL_HEAP_DEBUG)
    uint64_t h
        = r_cstl_heap_register_allocation (pResult, mem, cap + 1, R_CSTL_HEAP_NAME (r_cstl_string_repeat));
    if (h == 0)
    {
        r_cstl_heap_free (mem);
        r_cstl_heap_free (pResult);
        return NULL;
    }
#endif
    for (size_t i = 0; i < count; ++i)
        r_cstl_string_copy_bytes (mem + (i * len), r_cstl_string_data (pString), len);
    mem[total] = '\0';
    pResult->pData = mem;
    pResult->length = total;
    pResult->capacity = R_CSTL_SET_STORAGE_HEAP (cap);
    return pResult;
}

static int
r_cstl_string_compare_ignore_case_internal (
    const char* pLeft,
    size_t      leftLen,
    const char* pRight,
    size_t      rightLen)
{
    size_t n = leftLen < rightLen ? leftLen : rightLen;
    for (size_t i = 0; i < n; ++i)
    {
        char cl = r_cstl_to_lower_char (pLeft[i]);
        char cr = r_cstl_to_lower_char (pRight[i]);
        if (cl < cr) return -1;
        if (cl > cr) return 1;
    }
    if (leftLen < rightLen) return -1;
    if (leftLen > rightLen) return 1;
    return 0;
}

int
r_cstl_string_equals_ignore_case (const struct r_cstl_string* pLeft, const struct r_cstl_string* pRight)
{
    if (pLeft == pRight) return 1;
    if (!pLeft || !pRight) return 0;
    if (pLeft->length != pRight->length) return 0;
    int res = r_cstl_string_compare_ignore_case_internal (
        r_cstl_string_data (pLeft),
        pLeft->length,
        r_cstl_string_data (pRight),
        pRight->length);
    return res == 0;
}

int
r_cstl_string_compare_ignore_case (const struct r_cstl_string* pLeft, const struct r_cstl_string* pRight)
{
    if (pLeft == pRight) return 0;
    if (!pLeft) return -1;
    if (!pRight) return 1;
    return r_cstl_string_compare_ignore_case_internal (
        r_cstl_string_data (pLeft),
        pLeft->length,
        r_cstl_string_data (pRight),
        pRight->length);
}

size_t
r_cstl_string_hash_code (const struct r_cstl_string* pString)
{
    if (!pString) return 0;
    const char* pData = r_cstl_string_data (pString);
    size_t      len = pString->length;
    size_t      hash = 5381;
    for (size_t i = 0; i < len; ++i)
        hash = ((hash << 5) + hash) + (unsigned char)pData[i];
    return hash;
}

struct r_cstl_string*
r_cstl_string_remove (const struct r_cstl_string* pString, size_t start, size_t end)
{
    if (!pString) return NULL;
    if (!r_cstl_string_buffer_is_live (pString)) return NULL;
    if (start >= pString->length || start >= end)
        return r_cstl_new_string_with_data_sized (r_cstl_string_data (pString), pString->length);
    if (end > pString->length) end = pString->length;

    struct r_cstl_string_builder* pBuilder = r_cstl_new_string_builder ();
    if (!pBuilder) return NULL;

    r_cstl_string_builder_append_data (pBuilder, r_cstl_string_data (pString), start);
    r_cstl_string_builder_append_data (pBuilder, r_cstl_string_data (pString) + end, pString->length - end);

    struct r_cstl_string* pResult = r_cstl_string_builder_to_string (pBuilder);
    r_cstl_delete_string_builder (pBuilder);
    return pResult;
}

struct r_cstl_string*
r_cstl_string_value_of_int (int value)
{
    char   buf[32];
    size_t len = (size_t)snprintf (buf, sizeof (buf), "%d", value);
    return r_cstl_new_string_with_data_sized (buf, len);
}

struct r_cstl_string*
r_cstl_string_value_of_long (long long value)
{
    char   buf[64];
    size_t len = (size_t)snprintf (buf, sizeof (buf), "%lld", value);
    return r_cstl_new_string_with_data_sized (buf, len);
}

struct r_cstl_string*
r_cstl_string_value_of_double (double value)
{
    char   buf[64];
    size_t len = (size_t)snprintf (buf, sizeof (buf), "%g", value);
    return r_cstl_new_string_with_data_sized (buf, len);
}

struct r_cstl_string*
r_cstl_string_join (
    const struct r_cstl_string* pStringDelimiter,
    const struct r_cstl_string* pStringElements[],
    size_t                      count)
{
    if (count == 0) return r_cstl_new_string ();
    struct r_cstl_string_builder* pBuilder = r_cstl_new_string_builder ();
    if (!pBuilder) return NULL;
    for (size_t i = 0; i < count; ++i)
    {
        if (pStringElements[i]) r_cstl_string_builder_append (pBuilder, pStringElements[i]);
        if (i < count - 1 && pStringDelimiter) r_cstl_string_builder_append (pBuilder, pStringDelimiter);
    }
    struct r_cstl_string* pResult = r_cstl_string_builder_to_string (pBuilder);
    r_cstl_delete_string_builder (pBuilder);
    return pResult;
}

int
r_cstl_string_copy (
    struct r_cstl_string* R_CSTL_RESTRICT       pDst,
    const struct r_cstl_string* R_CSTL_RESTRICT pSrc)
{
    if (!pDst || !pSrc) goto cstl_fail;
    if (!r_cstl_string_buffer_is_live (pDst) || !r_cstl_string_buffer_is_live (pSrc)) goto cstl_fail;
    if (pDst == pSrc) return 0;

    if (r_cstl_string_ensure_capacity_internal (pDst, pSrc->length) != 0) goto cstl_fail;

    const char* pSrcData = r_cstl_string_data (pSrc);
    char*       pDstData = (char*)r_cstl_string_data (pDst);

    r_cstl_string_copy_bytes (pDstData, pSrcData, pSrc->length);
    pDstData[pSrc->length] = '\0';
    pDst->length = pSrc->length;
    return 0;

cstl_fail:
    return -1;
}

struct r_cstl_string_builder*
r_cstl_new_string_builder (void)
{
    struct r_cstl_string_builder* pBuilder
        = (struct r_cstl_string_builder*)r_cstl_heap_alloc (sizeof (struct r_cstl_string_builder));
    if (!pBuilder) return NULL;
    pBuilder->length = 0;
    pBuilder->capacity = R_CSTL_SET_STORAGE_SBO (R_CSTL_STRING_SBO_SIZE);
    pBuilder->pData = pBuilder->sbo;
    pBuilder->sbo[0] = '\0';
    return pBuilder;
}

struct r_cstl_string_builder*
r_cstl_new_string_builder_with_data (const char* pString)
{
    struct r_cstl_string_builder* pBuilder = r_cstl_new_string_builder ();
    if (!pBuilder) goto cstl_fail;
    if (!pString) goto cstl_fail;
    r_cstl_string_builder_emplace (pBuilder, pString);
    return pBuilder;
cstl_fail:
    return NULL;
}

struct r_cstl_string_builder*
r_cstl_new_string_builder_with_capacity (size_t capacity)
{
    struct r_cstl_string_builder* pBuilder
        = (struct r_cstl_string_builder*)r_cstl_heap_alloc (sizeof (struct r_cstl_string_builder));
    if (!pBuilder) return NULL;
    pBuilder->length = 0;
    if (capacity <= R_CSTL_STRING_SBO_SIZE)
    {
        pBuilder->capacity = R_CSTL_SET_STORAGE_SBO (R_CSTL_STRING_SBO_SIZE);
        pBuilder->pData = pBuilder->sbo;
    }
    else
    {
        char* mem = (char*)r_cstl_heap_alloc (capacity + 1);
        if (!mem)
        {
            r_cstl_heap_free (pBuilder);
            return NULL;
        }
#if defined(R_CSTL_HEAP_DEBUG)
        uint64_t h = r_cstl_heap_register_allocation (
            pBuilder,
            mem,
            capacity + 1,
            R_CSTL_HEAP_NAME (r_cstl_new_string_builder_with_capacity));
        if (h == 0)
        {
            r_cstl_heap_free (mem);
            r_cstl_heap_free (pBuilder);
            return NULL;
        }
#endif
        pBuilder->pData = mem;
        pBuilder->capacity = R_CSTL_SET_STORAGE_HEAP (capacity);
    }
    pBuilder->sbo[0] = '\0';
    return pBuilder;
}

void
r_cstl_delete_string_builder (struct r_cstl_string_builder* pBuilder)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pBuilder) return;
    if (R_CSTL_IS_STORAGE_HEAP (pBuilder->capacity))
    {
        r_cstl_heap_unregister_allocation (pBuilder, pBuilder->pData);
    }
#endif
    if (R_CSTL_IS_STORAGE_HEAP (pBuilder->capacity)) r_cstl_heap_free (pBuilder->pData);
#if defined(R_CSTL_HEAP_DEBUG)
    r_cstl_heap_check_object_leaks (pBuilder);
#endif
    r_cstl_heap_free (pBuilder);
}

R_CSTL_API_ATTR size_t
r_cstl_string_builder_length (const struct r_cstl_string_builder* pBuilder)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pBuilder) goto cstl_fail;
    if (!r_cstl_string_buffer_is_live ((const struct r_cstl_string*)pBuilder)) goto cstl_fail;
#endif
    return pBuilder->length;
cstl_fail:
    return 0;
}

size_t
r_cstl_string_builder_capacity (const struct r_cstl_string_builder* pBuilder)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pBuilder) goto cstl_fail;
    if (!r_cstl_string_buffer_is_live ((const struct r_cstl_string*)pBuilder)) goto cstl_fail;
#endif
    return R_CSTL_GET_CAPACITY (pBuilder->capacity);
cstl_fail:
    return 0;
}

int
r_cstl_string_builder_ensure_capacity (struct r_cstl_string_builder* pBuilder, size_t requiredCapacity)
{
    if (!pBuilder) goto cstl_fail;
    if (!r_cstl_string_buffer_is_live ((const struct r_cstl_string*)pBuilder)) goto cstl_fail;
    if (R_CSTL_GET_CAPACITY (pBuilder->capacity) >= requiredCapacity) return 0;
    size_t newCap = r_cstl_string_next_capacity (pBuilder->capacity, requiredCapacity);
    char*  old = R_CSTL_IS_STORAGE_HEAP (pBuilder->capacity) ? pBuilder->pData : NULL;
    if (old)
    {
        char* pNew = (char*)r_cstl_heap_realloc (old, newCap + 1);
        if (!pNew) return -1;
#if defined(R_CSTL_HEAP_DEBUG)
        if (pNew != old)
        {
            uint64_t h = r_cstl_heap_register_allocation (
                pBuilder,
                pNew,
                newCap + 1,
                R_CSTL_HEAP_NAME (r_cstl_string_builder_ensure_capacity));
            if (h == 0) goto cstl_fail_register;
        }
#endif
        pBuilder->pData = pNew;
        pBuilder->capacity = R_CSTL_SET_STORAGE_HEAP (newCap);
        return 0;
    }
    else
    {
        char* pNew = (char*)r_cstl_heap_alloc (newCap + 1);
        if (!pNew) return -1;
#if defined(R_CSTL_HEAP_DEBUG)
        uint64_t h = r_cstl_heap_register_allocation (
            pBuilder,
            pNew,
            newCap + 1,
            R_CSTL_HEAP_NAME (r_cstl_string_builder_ensure_capacity));
        if (h == 0) goto cstl_fail_register;
#endif
        r_cstl_string_copy_bytes (pNew, pBuilder->sbo, pBuilder->length);
        pNew[pBuilder->length] = '\0';
        pBuilder->pData = pNew;
        pBuilder->capacity = R_CSTL_SET_STORAGE_HEAP (newCap);
        return 0;
    }

cstl_fail_register:
#if defined(R_CSTL_HEAP_DEBUG)
    if (old) r_cstl_heap_unregister_allocation (pBuilder, old);
#endif
    return -1;
cstl_fail:
    return INT32_MAX;
}

int
r_cstl_string_builder_append (struct r_cstl_string_builder* pBuilder, const struct r_cstl_string* pString)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pBuilder) goto cstl_fail;
    if (!r_cstl_string_buffer_is_live ((const struct r_cstl_string*)pBuilder)) goto cstl_fail;
#endif
    // The pString can be NULL, in which case we treat it as an empty string and do nothing
    if (!pString) return 0;
    return r_cstl_string_builder_append_data (pBuilder, r_cstl_string_data (pString), pString->length);
cstl_fail:
    return -1;
}

#define r_cstl_emplace_string(Builder, String)                                                                \
    if (!Builder) goto cstl_fail;                                                                            \
    if (!r_cstl_string_buffer_is_live ((const struct r_cstl_string*)Builder)) goto cstl_fail;                   \
    if (!(String)) goto cstl_fail;                                                                           \
    return r_cstl_string_builder_append (Builder, String);                                                     \
    cstl_fail:                                                                                               \
    return -1

int
r_cstl_string_builder_emplace_sized (
    struct r_cstl_string_builder* pBuilder,
    const char*                  pCString,
    const size_t                 size)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pBuilder) goto cstl_fail;
    if (!r_cstl_string_buffer_is_live ((const struct r_cstl_string*)pBuilder)) goto cstl_fail;
    if (!pCString) goto cstl_fail;
#endif
    struct r_cstl_string* pString = r_cstl_new_string_with_data_sized (pCString, size);
    if (!pString) goto cstl_fail;
    int result = r_cstl_string_builder_append (pBuilder, pString);
    r_cstl_string_delete (pString);
    return result;
cstl_fail:
    return -1;
}

int
r_cstl_string_builder_emplace (struct r_cstl_string_builder* pBuilder, const char* pCString)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pBuilder) goto cstl_fail;
    if (!r_cstl_string_buffer_is_live ((const struct r_cstl_string*)pBuilder)) goto cstl_fail;
    if (!pCString) goto cstl_fail;
#endif
    struct r_cstl_string* pString = r_cstl_new_string_with_data (pCString);
    if (!pString) goto cstl_fail;
    int result = r_cstl_string_builder_append (pBuilder, pString);
    r_cstl_string_delete (pString);
    return result;
cstl_fail:
    return -1;
}

int
r_cstl_string_builder_append_data (struct r_cstl_string_builder* pBuilder, const char* pData, size_t length)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pBuilder || !pData || length == 0) return 0;
#endif
    size_t required = pBuilder->length + length;
    if (required > R_CSTL_GET_CAPACITY (pBuilder->capacity))
    {
        if (r_cstl_string_builder_ensure_capacity (pBuilder, required) != 0) return -1;
    }
    r_cstl_string_copy_bytes (pBuilder->pData + pBuilder->length, pData, length);
    pBuilder->length = required;
    pBuilder->pData[pBuilder->length] = '\0';
    return 0;
}

int
r_cstl_string_builder_append_char (struct r_cstl_string_builder* pBuilder, char value)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pBuilder) goto cstl_fail;
#endif
    size_t required = pBuilder->length + 1;
    if (required > R_CSTL_GET_CAPACITY (pBuilder->capacity))
    {
        if (r_cstl_string_builder_ensure_capacity (pBuilder, required) != 0) goto cstl_fail;
    }
    pBuilder->pData[pBuilder->length++] = value;
    pBuilder->pData[pBuilder->length] = '\0';
    return 0;
cstl_fail:
    return -1;
}

int
r_cstl_string_builder_append_int (struct r_cstl_string_builder* pBuilder, int value)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pBuilder) goto cstl_fail;
#endif
    char   buf[32];
    size_t len = (size_t)snprintf (buf, sizeof (buf), "%d", value);
    return r_cstl_string_builder_append_data (pBuilder, buf, len);
cstl_fail:
    return -1;
}

int
r_cstl_string_builder_append_long (struct r_cstl_string_builder* pBuilder, long long value)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pBuilder) goto cstl_fail;
#endif
    char   buf[64];
    size_t len = (size_t)snprintf (buf, sizeof (buf), "%lld", value);
    return r_cstl_string_builder_append_data (pBuilder, buf, len);
cstl_fail:
    return -1;
}

int
r_cstl_string_builder_append_double (struct r_cstl_string_builder* pBuilder, double value)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pBuilder) goto cstl_fail;
#endif
    char   buf[64];
    size_t len = (size_t)snprintf (buf, sizeof (buf), "%g", value);
    return r_cstl_string_builder_append_data (pBuilder, buf, len);
cstl_fail:
    return -1;
}
#define R_CSTL_STRING_BOOL_STR(b) (b ? "true" : "false")

int
r_cstl_string_builder_append_bool (struct r_cstl_string_builder* pBuilder, bool value)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pBuilder) goto cstl_fail;
    if (!r_cstl_string_buffer_is_live ((const struct r_cstl_string*)pBuilder)) goto cstl_fail;
#endif
    const char* boolStr = R_CSTL_STRING_BOOL_STR (value);
    size_t      len = strlen (boolStr);
    return r_cstl_string_builder_append (pBuilder, r_cstl_new_string_with_data_sized (boolStr, len));
cstl_fail:
    return -1;
}

int
r_cstl_string_builder_append_repeat (struct r_cstl_string_builder* pBuilder, const char* pData, size_t count)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pBuilder || !pData || count == 0) return 0;
#endif
    R_CSTL_STRING_STRLEN (pStr);

    if (length == 0) return 0;
    size_t total = length * count;
    if (total < length) return -1;

    if (r_cstl_string_builder_ensure_capacity (pBuilder, pBuilder->length + total) != 0) return -1;

    for (size_t i = 0; i < count; ++i)
        r_cstl_string_copy_bytes (pBuilder->pData + pBuilder->length + (i * length), pData, length);

    pBuilder->length += total;
    pBuilder->pData[pBuilder->length] = '\0';
    return 0;
}

R_CSTL_API int
r_cstl_string_builder_appendf (struct r_cstl_string_builder* pBuilder, const char* pFormat, ...)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pBuilder) goto cstl_fail;
    if (!r_cstl_string_buffer_is_live ((const struct r_cstl_string*)pBuilder)) goto cstl_fail;
#endif
    if (!pFormat) return 0;

    va_list args;
    va_start (args, pFormat);
    va_list copy;
    va_copy (copy, args);
    int needed = vsnprintf (NULL, 0, pFormat, copy);
    va_end (copy);
    if (needed < 0)
    {
        va_end (args);
        return -1;
    }
    size_t len = (size_t)needed;
    char*  buf = (char*)r_cstl_heap_alloc (len + 1);
    if (!buf)
    {
        va_end (args);
        return -1;
    }
    vsnprintf (buf, len + 1, pFormat, args);
    va_end (args);
    int result = r_cstl_string_builder_append_data (pBuilder, buf, len);
    r_cstl_heap_free (buf);
    return result;
cstl_fail:
    return -1;
}

void
r_cstl_string_builder_clear (struct r_cstl_string_builder* pBuilder)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pBuilder) return;
#endif
    pBuilder->length = 0;
    if (pBuilder->pData) pBuilder->pData[0] = '\0';
}

int
r_cstl_string_builder_insert (
    struct r_cstl_string_builder* pBuilder,
    size_t                       offset,
    const struct r_cstl_string*  pString)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pBuilder || !pString) return -1;
#endif
    if (offset > pBuilder->length) offset = pBuilder->length;
    return r_cstl_string_builder_emplace_insert (pBuilder, offset, r_cstl_string_data (pString));
}

int
r_cstl_string_builder_emplace_insert (struct r_cstl_string_builder* pBuilder, size_t offset, const char* pCString)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pBuilder || !pCString) return -1;
#endif
    if (offset > pBuilder->length) offset = pBuilder->length;
    size_t len = strlen (pCString);
    if (len == 0) return 0;

    size_t required = pBuilder->length + len;
    if (required > R_CSTL_GET_CAPACITY (pBuilder->capacity))
    {
        if (r_cstl_string_builder_ensure_capacity (pBuilder, required) != 0) return -1;
    }

    r_cstl_string_copy_bytes (
        pBuilder->pData + offset + len,
        pBuilder->pData + offset,
        pBuilder->length - offset);
    r_cstl_string_copy_bytes (pBuilder->pData + offset, pCString, len);
    pBuilder->length += len;
    pBuilder->pData[pBuilder->length] = '\0';
    return 0;
}

int
r_cstl_string_builder_insert_char (struct r_cstl_string_builder* pBuilder, size_t offset, char value)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pBuilder) return -1;
#endif
    if (offset > pBuilder->length) offset = pBuilder->length;

    size_t required = pBuilder->length + 1;
    if (required > R_CSTL_GET_CAPACITY (pBuilder->capacity))
    {
        if (r_cstl_string_builder_ensure_capacity (pBuilder, required) != 0) return -1;
    }

    r_cstl_string_copy_bytes (
        pBuilder->pData + offset + 1,
        pBuilder->pData + offset,
        pBuilder->length - offset);
    pBuilder->pData[offset] = value;
    pBuilder->length++;
    pBuilder->pData[pBuilder->length] = '\0';
    return 0;
}

int
r_cstl_string_builder_delete (struct r_cstl_string_builder* pBuilder, size_t start, size_t end)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pBuilder) return -1;
#endif
    if (start >= pBuilder->length || start >= end) return 0;
    if (end > pBuilder->length) end = pBuilder->length;

    size_t len = end - start;
    r_cstl_string_copy_bytes (pBuilder->pData + start, pBuilder->pData + end, pBuilder->length - end);
    pBuilder->length -= len;
    pBuilder->pData[pBuilder->length] = '\0';
    return 0;
}

int
r_cstl_string_builder_delete_char_at (struct r_cstl_string_builder* pBuilder, size_t index)
{

#if defined(R_CSTL_HEAP_DEBUG)
    if (!pBuilder || index >= pBuilder->length) goto cstl_fail;
    if (!r_cstl_string_buffer_is_live ((const struct r_cstl_string*)pBuilder)) goto cstl_fail;
#endif
    return r_cstl_string_builder_delete (pBuilder, index, index + 1);
cstl_fail:
    return -1;
}

int
r_cstl_string_builder_replace (
    struct r_cstl_string_builder* pBuilder,
    size_t                       start,
    size_t                       end,
    const struct r_cstl_string*  pString)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pBuilder || !pString) return -1;
#endif
    return r_cstl_string_builder_emplace_replace (pBuilder, start, end, r_cstl_string_data (pString));
}

int
r_cstl_string_builder_emplace_replace (
    struct r_cstl_string_builder* pBuilder,
    size_t                       start,
    size_t                       end,
    const char*                  pString)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pBuilder || !pString) return -1;
    if (start > pBuilder->length) return -1;
#endif
    if (end > pBuilder->length) end = pBuilder->length;
    if (start >= end) return r_cstl_string_builder_emplace_insert (pBuilder, start, pString);
    size_t oldLen = end - start;
    size_t newLen = strlen (pString);

    if (newLen > oldLen)
    {
        size_t delta = newLen - oldLen;
        size_t required = pBuilder->length + delta;
        if (required > R_CSTL_GET_CAPACITY (pBuilder->capacity))
        {
            if (r_cstl_string_builder_ensure_capacity (pBuilder, required) != 0) return -1;
        }
        r_cstl_string_copy_bytes (pBuilder->pData + end + delta, pBuilder->pData + end, pBuilder->length - end);
    }
    else if (newLen < oldLen)
    {
        size_t delta = oldLen - newLen;
        r_cstl_string_copy_bytes (pBuilder->pData + end - delta, pBuilder->pData + end, pBuilder->length - end);
    }

    r_cstl_string_copy_bytes (pBuilder->pData + start, pString, newLen);
    pBuilder->length = pBuilder->length - oldLen + newLen;
    pBuilder->pData[pBuilder->length] = '\0';
    return 0;
}

int
r_cstl_string_builder_set_char_at (struct r_cstl_string_builder* pBuilder, size_t index, char value)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pBuilder || index >= pBuilder->length) goto cstl_fail;
    if (!r_cstl_string_buffer_is_live ((const struct r_cstl_string*)pBuilder)) goto cstl_fail;
#endif
    pBuilder->pData[index] = value;
    return 0;
cstl_fail:
    return -1;
}

int
r_cstl_string_builder_set_length (struct r_cstl_string_builder* pBuilder, size_t newLength)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pBuilder) return -1;
#endif
    if (newLength > R_CSTL_GET_CAPACITY (pBuilder->capacity))
    {
        if (r_cstl_string_builder_ensure_capacity (pBuilder, newLength) != 0) return -1;
    }
    if (newLength < pBuilder->length)
    {
        pBuilder->length = newLength;
        pBuilder->pData[pBuilder->length] = '\0';
    }
    else if (newLength > pBuilder->length)
    {
        r_cstl_string_set_bytes (pBuilder->pData + pBuilder->length, '\0', newLength - pBuilder->length);
        pBuilder->length = newLength;
    }
    return 0;
}

int
r_cstl_string_builder_reverse (struct r_cstl_string_builder* pBuilder)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pBuilder) return -1;
#endif
    r_cstl_string_reverse_bytes (pBuilder->pData, pBuilder->length);
    return 0;
}

struct r_cstl_string*
r_cstl_string_builder_to_string (const struct r_cstl_string_builder* pBuilder)
{
#if defined(R_CSTL_HEAP_DEBUG)
    if (!pBuilder) goto cstl_fail;
    if (pBuilder->length == 0) goto cstl_fail;
#endif
    return r_cstl_new_string_with_data_sized (pBuilder->pData, pBuilder->length);
cstl_fail:
    return r_cstl_new_string ();
}
