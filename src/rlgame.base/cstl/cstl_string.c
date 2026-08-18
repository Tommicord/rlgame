#include "rlgame.base/cstl/cstl_string.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"

#define R_CSTL_INLINE
#include "rlgame.base/cstl/cstl_platform.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#if defined(_R_SIMD_AVX2)
#include <immintrin.h>
#elif defined(_R_SIMD_SSE)
#include <immintrin.h>
#elif defined(_RL_SIMD_ARM_NEON) || defined(_R_SIMD_ARM_NEON)
#include <arm_neon.h>
#endif

#if defined(_MSC_VER)
#include <intrin.h>

R_CSTL_API_ATTR static unsigned int
R_CSTL_CountTrailingZeros (unsigned int value)
{
        unsigned long index;
        if (_BitScanForward (&index, value))
                return (unsigned int)index;
        return 32;
}
R_CSTL_API_ATTR static unsigned int
R_CSTL_CountTrailingZeros64 (unsigned long long value)
{
        unsigned long index;
        if (_BitScanForward64 (&index, value))
                return (unsigned int)index;
        return 64;
}
#define R_CSTL_CTZ(v)   R_CSTL_CountTrailingZeros (v)
#define R_CSTL_CTZLL(v) R_CSTL_CountTrailingZeros64 (v)
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

struct R_CSTL_String
{
                size_t length;
                size_t capacity;
                union
                {
                                char* pData;
                                char  sbo[R_CSTL_STRING_SBO_SIZE];
                };
};

struct R_CSTL_StringBuilder
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
R_CSTL_StringCopyBytes (char* R_CSTL_RESTRICT pDst, const char* R_CSTL_RESTRICT pSrc, const size_t sizeBytes)
{
        if (sizeBytes == 0 || pDst == pSrc)
                return;

#if defined(_R_SIMD_AVX2)
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
#elif defined(_R_SIMD_SSE)
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
#elif defined(_RL_SIMD_ARM_NEON) || defined(_R_SIMD_ARM_NEON)
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
R_CSTL_StringSetBytes (char* R_CSTL_RESTRICT pDst, char value, const size_t sizeBytes)
{
        if (sizeBytes == 0)
                return;

#if defined(_R_SIMD_AVX2)
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
#elif defined(_R_SIMD_SSE)
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
#elif defined(_RL_SIMD_ARM_NEON) || defined(_R_SIMD_ARM_NEON)
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
R_CSTL_StringBufferIsLive (const struct R_CSTL_String* pString)
{
        if (!pString)
                return 0;
#ifndef R_CSTL_HEAP_DEBUG
        return 1;
#else
        if (R_CSTL_IS_STORAGE_HEAP (pString->capacity))
        {
                if (!pString->pData)
                        return 0;
                return R_CSTL_HeapIsValidPointer (pString->pData) != 0;
        }
        // SBO is always valid if the string itself is valid
        return 1;
#endif
}

static size_t
R_CSTL_StringFindChar (const char* pData, size_t length, char ch)
{
        if (length == 0)
                return (size_t)-1;

#if defined(_R_SIMD_AVX2)
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
                        if (pData[i] == ch)
                                return i;
                }
                return (size_t)-1;
        }
#elif defined(_R_SIMD_SSE)
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
                        if (pData[i] == ch)
                                return i;
                }
                return (size_t)-1;
        }
#elif defined(_RL_SIMD_ARM_NEON) || defined(_R_SIMD_ARM_NEON)
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
                        if (pData[i] == ch)
                                return i;
                }
                return (size_t)-1;
        }
#endif
        for (size_t i = 0; i < length; ++i)
        {
                if (pData[i] == ch)
                        return i;
        }
        return (size_t)-1;
}

static size_t
R_CSTL_StringFindCharReverse (const char* pData, size_t length, char ch)
{
        if (length == 0)
                return (size_t)-1;

        for (size_t i = length; i > 0; --i)
        {
                if (pData[i - 1] == ch)
                        return i - 1;
        }
        return (size_t)-1;
}

static void
R_CSTL_StringReverseBytes (char* pData, size_t length)
{
        if (length <= 1)
                return;

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
R_CSTL_ToLowerChar (char c)
{
        return (c >= 'A' && c <= 'Z') ? (c + 32) : c;
}

R_CSTL_API_ATTR static char
R_CSTL_ToUpperChar (char c)
{
        return (c >= 'a' && c <= 'z') ? (c - 32) : c;
}

#define R_CSTL_STRING_HEAP_ALLOC_INITIAL_CAPACITY 128
R_CSTL_API_ATTR static size_t
R_CSTL_StringNextCapacity (size_t current, const size_t required)
{
        size_t next = current ? R_CSTL_GET_CAPACITY (current) : R_CSTL_STRING_SBO_SIZE;
        while (next < required)
        {
                if (next > (SIZE_MAX / 2))
                        return required;
                next <<= 1;
        }
        return next;
}

static int
R_CSTL_StringEnsureCapacityInternal (struct R_CSTL_String* pString, const size_t required)
{
        if (!pString)
                goto cstl_fail;
        if (!R_CSTL_StringBufferIsLive (pString))
                goto cstl_fail;
        if (R_CSTL_GET_CAPACITY (pString->capacity) >= required)
                return 0;

        size_t newCap = R_CSTL_StringNextCapacity (R_CSTL_GET_CAPACITY (pString->capacity), required);
        char*  pOld = R_CSTL_IS_STORAGE_HEAP (pString->capacity) ? pString->pData : NULL;

        if (pOld)
        {
                char* pNew = (char*)R_CSTL_HeapRealloc (pOld, newCap + 1);
                if (!pNew)
                        return -1;
#if defined(R_CSTL_HEAP_DEBUG)
                if (pNew != pOld)
                {
                        uint64_t h = R_CSTL_HeapRegisterAllocation (
                            pString,
                            pNew,
                            newCap + 1,
                            R_CSTL_HEAP_NAME (R_CSTL_StringEnsureCapacityInternal));
                        if (h == 0)
                                goto cstl_fail_register;
                }
#endif
                pString->pData = pNew;
                pString->capacity = R_CSTL_SET_STORAGE_HEAP (newCap);
                return 0;
        }
        else
        {
                char* pNew = (char*)R_CSTL_HeapAlloc (newCap + 1);
                if (!pNew)
                        return -1;
#if defined(R_CSTL_HEAP_DEBUG)
                uint64_t h = R_CSTL_HeapRegisterAllocation (
                    pString,
                    pNew,
                    newCap + 1,
                    R_CSTL_HEAP_NAME (R_CSTL_StringEnsureCapacityInternal));
                if (h == 0)
                        goto cstl_fail_register;
#endif
                R_CSTL_StringCopyBytes (pNew, pString->sbo, pString->length);
                pNew[pString->length] = '\0';
                pString->pData = pNew;
                pString->capacity = R_CSTL_SET_STORAGE_HEAP (newCap);
                return 0;
        }

cstl_fail_register:
#if defined(R_CSTL_HEAP_DEBUG)
        if (pOld)
                R_CSTL_HeapUnregisterAllocation (pString, pOld);
#endif
        return -1;
cstl_fail:
        return -1;
}

static void
R_CSTL_StringReleaseBuffer (struct R_CSTL_String* pString)
{
        if (!pString)
                goto cstl_fail;
        if (!R_CSTL_StringBufferIsLive (pString))
                goto cstl_fail;

#if defined(R_CSTL_HEAP_DEBUG)
        if (R_CSTL_IS_STORAGE_HEAP (pString->capacity))
        {
                if (!R_CSTL_HeapIsValidPointer (pString->pData))
                        goto cstl_fail;
                R_CSTL_HeapUnregisterAllocation (pString, pString->pData);
        }
#endif

        if (R_CSTL_IS_STORAGE_HEAP (pString->capacity))
                R_CSTL_HeapFree (pString->pData);
        pString->pData = NULL;
        pString->capacity = 0;
        pString->length = 0;
        return;
cstl_fail:
        return;
}

R_CSTL_API_ATTR static struct R_CSTL_String*
R_CSTL_StringCreateShell (void)
{
        struct R_CSTL_String* pString = (struct R_CSTL_String*)R_CSTL_HeapAlloc (sizeof (R_CSTL_String));
        if (!pString)
                return NULL;
        pString->length = 0;
        pString->capacity = R_CSTL_SET_STORAGE_SBO (R_CSTL_STRING_SBO_SIZE);
        pString->sbo[0] = '\0';
        return pString;
}

R_CSTL_API_ATTR struct R_CSTL_String*
R_CSTL_NewString (void)
{
        return R_CSTL_StringCreateShell ();
}

#define R_CSTL_STRING_STRLEN(s)                                                                              \
        const char* pDataString = (const char*)pData;                                                        \
        while (*pDataString != '\0')                                                                         \
                ++pDataString;                                                                               \
        const size_t length = pDataString - (const char*)pData

struct R_CSTL_String*
R_CSTL_NewStringWithDataSized (const char* pData, const size_t length)
{
        if (!pData)
                return NULL;
        struct R_CSTL_String* pString = R_CSTL_StringCreateShell ();
        if (!pString)
                goto cstl_fail;
        if (!R_CSTL_StringBufferIsLive (pString))
                goto cstl_fail;

        if (length < R_CSTL_STRING_SBO_SIZE)
        {
                R_CSTL_StringCopyBytes (pString->sbo, pData, length);
                pString->sbo[length] = '\0';
                pString->length = length;
                pString->capacity = R_CSTL_SET_STORAGE_SBO (R_CSTL_STRING_SBO_SIZE);
                return pString;
        }

        size_t cap = R_CSTL_StringNextCapacity (R_CSTL_STRING_SBO_SIZE, length);
        char*  mem = (char*)R_CSTL_HeapAlloc (cap + 1);
        if (!mem)
        {
                R_CSTL_HeapFree (pString);
                return NULL;
        }
#if defined(R_CSTL_HEAP_DEBUG)
        uint64_t h = R_CSTL_HeapRegisterAllocation (
            pString,
            mem,
            cap + 1,
            R_CSTL_HEAP_NAME (R_CSTL_NewStringWithData));
        if (h == 0)
        {
                R_CSTL_HeapFree (mem);
                R_CSTL_HeapFree (pString);
                return NULL;
        }
#endif
        R_CSTL_StringCopyBytes (mem, pData, length);
        mem[length] = '\0';
        pString->pData = mem;
        pString->length = length;
        pString->capacity = R_CSTL_SET_STORAGE_HEAP (cap);
        return pString;
cstl_fail:
        return NULL;
}

struct R_CSTL_String*
R_CSTL_NewStringWithCapacity (const size_t cap)
{
        struct R_CSTL_String* pString
            = (struct R_CSTL_String*)R_CSTL_HeapAlloc (sizeof (R_CSTL_String) + cap);
        if (!pString)
                return NULL;
        char* mem = (char*)R_CSTL_HeapAlloc (cap + 1);
        if (!mem)
        {
                R_CSTL_HeapFree (pString);
                return NULL;
        }
#if defined(R_CSTL_HEAP_DEBUG)
        uint64_t h = R_CSTL_HeapRegisterAllocation (
            pString,
            mem,
            cap + 1,
            R_CSTL_HEAP_NAME (R_CSTL_NewStringWithData));
        if (h == 0)
        {
                R_CSTL_HeapFree (mem);
                R_CSTL_HeapFree (pString);
                return NULL;
        }
#endif
        mem[0] = '\0';
        pString->length = 0;
        pString->capacity = R_CSTL_SET_STORAGE_HEAP (cap);
        pString->pData = mem;
        return pString;
}

R_CSTL_API_ATTR struct R_CSTL_String*
R_CSTL_NewStringWithData (const char* pData)
{
        if (!pData)
                return NULL;
        R_CSTL_STRING_STRLEN (pData);
        return R_CSTL_NewStringWithDataSized (pData, length);
}

struct R_CSTL_String*
R_CSTL_NewStringWithFormat (const char* pFormat, ...)
{
        if (!pFormat)
                return R_CSTL_NewString ();
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
        char*  buf = (char*)R_CSTL_HeapAlloc (len + 1);
        if (!buf)
        {
                va_end (args);
                return NULL;
        }
        vsnprintf (buf, len + 1, pFormat, args);
        va_end (args);
        struct R_CSTL_String* pString = R_CSTL_NewStringWithDataSized (buf, len);
        R_CSTL_HeapFree (buf);
        return pString;
}

R_CSTL_API_ATTR void
R_CSTL_StringDelete (struct R_CSTL_String* pString)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pString)
                goto cstl_fail;
        if (!R_CSTL_StringBufferIsLive (pString))
                goto cstl_fail;
#endif
        R_CSTL_StringReleaseBuffer (pString);
        R_CSTL_HeapFree (pString);
        R_CSTL_HeapUnregisterAllocation (pString, pString->pData);
cstl_fail:
        return;
}

R_CSTL_API_ATTR size_t
R_CSTL_StringLength (const struct R_CSTL_String* pString)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pString)
                goto cstl_fail;
        if (!R_CSTL_StringBufferIsLive (pString))
                goto cstl_fail;
#endif
        return pString->length;
cstl_fail:
        return 0;
}

R_CSTL_API_ATTR const char*
R_CSTL_StringData (const struct R_CSTL_String* pString)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pString)
                goto cstl_fail;
        if (!R_CSTL_StringBufferIsLive (pString))
                goto cstl_fail;
#endif
        if (R_CSTL_IS_STORAGE_SBO (pString->capacity))
                return pString->sbo;
        return pString->pData;
cstl_fail:
        return NULL;
}

R_CSTL_API_ATTR char
R_CSTL_StringCharAt (const struct R_CSTL_String* pString, const size_t index)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pString)
                goto cstl_fail;
        if (!R_CSTL_StringBufferIsLive (pString))
                goto cstl_fail;
        if (index >= pString->length)
                goto cstl_fail;
#endif
        if (R_CSTL_IS_STORAGE_SBO (pString->capacity))
                return pString->sbo[index];
        return pString->pData[index];
cstl_fail:
        return 0x00;
}

R_CSTL_API_ATTR int
R_CSTL_StringEquals (const struct R_CSTL_String* pLeft, const struct R_CSTL_String* pRight)
{
        if (pLeft == pRight)
                return 1;
        if (!pLeft || !pRight)
                return 0;
        if (pLeft->length != pRight->length)
                return 0;
        return memcmp (R_CSTL_StringData (pLeft), R_CSTL_StringData (pRight), pLeft->length) == 0;
}

int
R_CSTL_StringCompare (const struct R_CSTL_String* pLeft, const struct R_CSTL_String* pRight)
{
        if (pLeft == pRight)
                return 0;
        if (!pLeft)
                return -1;
        if (!pRight)
                return 1;
        size_t n = pLeft->length < pRight->length ? pLeft->length : pRight->length;
        int    cmp = memcmp (R_CSTL_StringData (pLeft), R_CSTL_StringData (pRight), n);
        if (cmp != 0)
                return cmp;
        if (pLeft->length < pRight->length)
                return -1;
        if (pLeft->length > pRight->length)
                return 1;
        return 0;
}

struct R_CSTL_String*
R_CSTL_StringConcat (const struct R_CSTL_String* pLeft, const struct R_CSTL_String* pRight)
{
        if (!pLeft && !pRight)
                return R_CSTL_NewString ();
        if (!pLeft)
        {
                if (!R_CSTL_StringBufferIsLive (pRight))
                        return NULL;
                return R_CSTL_NewStringWithDataSized (R_CSTL_StringData (pRight), pRight->length);
        }
        if (!pRight)
        {
                if (!R_CSTL_StringBufferIsLive (pLeft))
                        return NULL;
                return R_CSTL_NewStringWithDataSized (R_CSTL_StringData (pLeft), pLeft->length);
        }
        if (!R_CSTL_StringBufferIsLive (pLeft) || !R_CSTL_StringBufferIsLive (pRight))
                return NULL;
        size_t                total = pLeft->length + pRight->length;
        struct R_CSTL_String* pString = R_CSTL_StringCreateShell ();
        if (!pString)
                return NULL;
        const char* pLeftData = R_CSTL_StringData (pLeft);
        const char* pRightData = R_CSTL_StringData (pRight);
        if (total <= R_CSTL_STRING_SBO_SIZE)
        {
                R_CSTL_StringCopyBytes (pString->sbo, pLeftData, pLeft->length);
                R_CSTL_StringCopyBytes (pString->sbo + pLeft->length, pRightData, pRight->length);
                pString->sbo[total] = '\0';
                pString->length = total;
                return pString;
        }
        size_t cap = R_CSTL_StringNextCapacity (R_CSTL_STRING_SBO_SIZE, total);
        char*  mem = (char*)R_CSTL_HeapAlloc (cap + 1);
        if (!mem)
        {
                R_CSTL_HeapFree (pString);
                return NULL;
        }
#if defined(R_CSTL_HEAP_DEBUG)
        uint64_t h
            = R_CSTL_HeapRegisterAllocation (pString, mem, cap + 1, R_CSTL_HEAP_NAME (R_CSTL_StringConcat));
        if (h == 0)
        {
                R_CSTL_HeapFree (mem);
                R_CSTL_HeapFree (pString);
                return NULL;
        }
#endif
        R_CSTL_StringCopyBytes (mem, pLeftData, pLeft->length);
        R_CSTL_StringCopyBytes (mem + pLeft->length, pRightData, pRight->length);
        mem[total] = '\0';
        pString->pData = mem;
        pString->length = total;
        pString->capacity = R_CSTL_SET_STORAGE_HEAP (cap);
        return pString;
}

struct R_CSTL_String*
R_CSTL_StringSubstring (const struct R_CSTL_String* pString, size_t begin, size_t end)
{
        if (!pString)
                return NULL;
        if (!R_CSTL_StringBufferIsLive (pString))
                return NULL;
        if (begin >= pString->length || begin >= end)
                return R_CSTL_NewString ();
        if (end > pString->length)
                end = pString->length;
        size_t      len = end - begin;
        const char* pData = R_CSTL_StringData (pString);
        return R_CSTL_NewStringWithDataSized (pData + begin, len);
}

R_CSTL_API_ATTR int
R_CSTL_StringStartsWith (const struct R_CSTL_String* pString, const char* pPrefix)
{
        if (!pString || !pPrefix)
                return 0;
        size_t plen = strlen (pPrefix);
        if (plen > pString->length)
                return 0;
        return memcmp (R_CSTL_StringData (pString), pPrefix, plen) == 0;
}

R_CSTL_API_ATTR int
R_CSTL_StringEndsWith (const struct R_CSTL_String* pString, const char* pSuffix)
{
        if (!pString || !pSuffix)
                return 0;
        size_t slen = strlen (pSuffix);
        if (slen > pString->length)
                return 0;
        const char* pData = R_CSTL_StringData (pString);
        return memcmp (pData + (pString->length - slen), pSuffix, slen) == 0;
}

R_CSTL_API_ATTR size_t
R_CSTL_StringIndexOf (const struct R_CSTL_String* pString, const char* pNeedle)
{
        if (!pString || !pNeedle)
                return (size_t)-1;
        if (!R_CSTL_StringBufferIsLive (pString))
                return (size_t)-1;
        size_t nlen = strlen (pNeedle);
        if (nlen == 0)
                return 0;
        if (nlen > pString->length)
                return (size_t)-1;

        const char* pHay = R_CSTL_StringData (pString);
        size_t      haylen = pString->length;
        const char  needle0 = pNeedle[0];
        for (size_t i = 0; i + nlen <= haylen; ++i)
        {
                if (pHay[i] != needle0)
                        continue;
                if (memcmp (pHay + i, pNeedle, nlen) == 0)
                        return i;
        }
        return (size_t)-1;
}

size_t
R_CSTL_StringLastIndexOf (const struct R_CSTL_String* pString, const char* pNeedle)
{
        if (!pString || !pNeedle)
                return (size_t)-1;
        if (!R_CSTL_StringBufferIsLive (pString))
                return (size_t)-1;
        size_t nlen = strlen (pNeedle);
        if (nlen == 0)
                return pString->length;
        if (nlen > pString->length)
                return (size_t)-1;

        const char* pHay = R_CSTL_StringData (pString);
        size_t      haylen = pString->length;
        for (size_t i = haylen - nlen + 1; i > 0; --i)
        {
                if (memcmp (pHay + (i - 1), pNeedle, nlen) == 0)
                        return i - 1;
        }
        return (size_t)-1;
}

size_t
R_CSTL_StringIndexOfChar (const struct R_CSTL_String* pString, char ch)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pString)
                goto cstl_fail;
        if (!R_CSTL_StringBufferIsLive (pString))
                goto cstl_fail;
#endif
        return R_CSTL_StringFindChar (R_CSTL_StringData (pString), pString->length, ch);
cstl_fail:
        return (size_t)-1;
}

R_CSTL_API_ATTR size_t
R_CSTL_StringLastIndexOfChar (const struct R_CSTL_String* pString, char ch)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pString)
                goto cstl_fail;
        if (!R_CSTL_StringBufferIsLive (pString))
                goto cstl_fail;
#endif
        return R_CSTL_StringFindCharReverse (R_CSTL_StringData (pString), pString->length, ch);
cstl_fail:
        return (size_t)-1;
}

R_CSTL_API_ATTR int
R_CSTL_StringContains (const struct R_CSTL_String* pString, const char* pNeedle)
{
        return R_CSTL_StringIndexOf (pString, pNeedle) != (size_t)-1;
}

R_CSTL_API_ATTR int
R_CSTL_StringIsEmpty (const struct R_CSTL_String* pString)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pString)
                goto cstl_fail;
        if (!R_CSTL_StringBufferIsLive (pString))
                goto cstl_fail;
#endif
        return pString->length == 0;
cstl_fail:
        return -1;
}

struct R_CSTL_String*
R_CSTL_StringToLowerCase (const struct R_CSTL_String* pString)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pString)
                return R_CSTL_NewString ();
        if (!R_CSTL_StringBufferIsLive (pString))
                goto cstl_fail;
#endif
        const char*           pData = R_CSTL_StringData (pString);
        size_t                len = pString->length;
        struct R_CSTL_String* pResult = R_CSTL_NewStringWithDataSized (pData, len);
        if (!pResult)
                return NULL;
        char* pDst = (char*)R_CSTL_StringData (pResult);
        for (size_t i = 0; i < len; ++i)
                pDst[i] = R_CSTL_ToLowerChar (pDst[i]);
        return pResult;
cstl_fail:
        return NULL;
}

struct R_CSTL_String*
R_CSTL_StringToUpperCase (const struct R_CSTL_String* pString)
{
        if (!pString)
                return R_CSTL_NewString ();
        const char*           pData = R_CSTL_StringData (pString);
        size_t                len = pString->length;
        struct R_CSTL_String* pResult = R_CSTL_NewStringWithDataSized (pData, len);
        if (!pResult)
                return NULL;
        char* pDst = (char*)R_CSTL_StringData (pResult);
        for (size_t i = 0; i < len; ++i)
                pDst[i] = R_CSTL_ToUpperChar (pDst[i]);
        return pResult;
}

struct R_CSTL_String*
R_CSTL_StringTrim (const struct R_CSTL_String* pString)
{
        if (!pString)
                return R_CSTL_NewString ();
        if (!R_CSTL_StringBufferIsLive (pString))
                goto cstl_fail;
        const char* pData = R_CSTL_StringData (pString);
        size_t      len = pString->length;
        size_t      start = 0;
        while (
            start < len
            && (pData[start] == ' ' || pData[start] == '\t' || pData[start] == '\n' || pData[start] == '\r'))
                ++start;
        size_t end = len;
        while (end > start
               && (pData[end - 1] == ' ' || pData[end - 1] == '\t' || pData[end - 1] == '\n'
                   || pData[end - 1] == '\r'))
                --end;
        return R_CSTL_StringSubstring (pString, start, end);
cstl_fail:
        return NULL;
}

struct R_CSTL_String*
R_CSTL_StringReplace (const struct R_CSTL_String* pString, const char* pTarget, const char* pReplacement)
{
        if (!pString || !pTarget || !pReplacement)
        {
                if (pString && R_CSTL_StringBufferIsLive (pString))
                        return R_CSTL_NewStringWithDataSized (R_CSTL_StringData (pString), pString->length);
                return R_CSTL_NewString ();
        }
        if (!R_CSTL_StringBufferIsLive (pString))
                return NULL;
        size_t tlen = strlen (pTarget);
        if (tlen == 0)
                return R_CSTL_NewStringWithDataSized (R_CSTL_StringData (pString), pString->length);
        size_t rlen = strlen (pReplacement);

        R_CSTL_StringBuilder* pBuilder = R_CSTL_NewStringBuilder ();
        if (!pBuilder)
                return NULL;

        const char* pData = R_CSTL_StringData (pString);
        size_t      len = pString->length;
        size_t      pos = 0;
        size_t      found = R_CSTL_StringIndexOf (pString, pTarget);

        while (found != (size_t)-1)
        {
                R_CSTL_StringBuilderAppendData (pBuilder, pData + pos, found - pos);
                R_CSTL_StringBuilderEmplace (pBuilder, pReplacement);
                pos = found + tlen;
                struct R_CSTL_String* pStringTemp = R_CSTL_StringSubstring (pString, pos, len);
                found = R_CSTL_StringIndexOf (pStringTemp, pTarget);
                R_CSTL_StringDelete (pStringTemp);
        }
        R_CSTL_StringBuilderAppendData (pBuilder, pData + pos, len - pos);

        struct R_CSTL_String* pResult = R_CSTL_StringBuilderToString (pBuilder);
        R_CSTL_DeleteStringBuilder (pBuilder);
        return pResult;
}

struct R_CSTL_String*
R_CSTL_StringReplaceChar (const struct R_CSTL_String* pString, char oldChar, char newChar)
{
        if (!pString)
                return R_CSTL_NewString ();
        if (!R_CSTL_StringBufferIsLive (pString))
                goto cstl_fail;
        const char*           pData = R_CSTL_StringData (pString);
        size_t                len = pString->length;
        struct R_CSTL_String* pResult = R_CSTL_NewStringWithDataSized (pData, len);
        if (!pResult)
                return NULL;
        char* pDst = (char*)R_CSTL_StringData (pResult);
        for (size_t i = 0; i < len; ++i)
        {
                if (pDst[i] == oldChar)
                        pDst[i] = newChar;
        }
        return pResult;
cstl_fail:
        return NULL;
}

struct R_CSTL_String*
R_CSTL_StringRepeat (struct R_CSTL_String* pString, size_t count)
{
        if (!pString || count == 0)
                return R_CSTL_NewString ();
        if (!R_CSTL_StringBufferIsLive (pString))
                return NULL;
        if (count == 1)
                return R_CSTL_NewStringWithDataSized (R_CSTL_StringData (pString), pString->length);

        size_t len = pString->length;
        if (len == 0)
                return R_CSTL_NewString ();

        size_t total = len * count;
        if (total < len)
                return NULL;

        struct R_CSTL_String* pResult = R_CSTL_StringCreateShell ();
        if (!pResult)
                return NULL;

        if (total <= R_CSTL_STRING_SBO_SIZE)
        {
                for (size_t i = 0; i < count; ++i)
                        R_CSTL_StringCopyBytes (pResult->sbo + (i * len), R_CSTL_StringData (pString), len);
                pResult->sbo[total] = '\0';
                pResult->length = total;
                return pResult;
        }

        size_t cap = R_CSTL_StringNextCapacity (R_CSTL_STRING_SBO_SIZE, total);
        char*  mem = (char*)R_CSTL_HeapAlloc (cap + 1);
        if (!mem)
        {
                R_CSTL_HeapFree (pResult);
                return NULL;
        }
#if defined(R_CSTL_HEAP_DEBUG)
        uint64_t h
            = R_CSTL_HeapRegisterAllocation (pResult, mem, cap + 1, R_CSTL_HEAP_NAME (R_CSTL_StringRepeat));
        if (h == 0)
        {
                R_CSTL_HeapFree (mem);
                R_CSTL_HeapFree (pResult);
                return NULL;
        }
#endif
        for (size_t i = 0; i < count; ++i)
                R_CSTL_StringCopyBytes (mem + (i * len), R_CSTL_StringData (pString), len);
        mem[total] = '\0';
        pResult->pData = mem;
        pResult->length = total;
        pResult->capacity = R_CSTL_SET_STORAGE_HEAP (cap);
        return pResult;
}

static int
R_CSTL_StringCompareIgnoreCaseInternal (
    const char* pLeft,
    size_t      leftLen,
    const char* pRight,
    size_t      rightLen)
{
        size_t n = leftLen < rightLen ? leftLen : rightLen;
        for (size_t i = 0; i < n; ++i)
        {
                char cl = R_CSTL_ToLowerChar (pLeft[i]);
                char cr = R_CSTL_ToLowerChar (pRight[i]);
                if (cl < cr)
                        return -1;
                if (cl > cr)
                        return 1;
        }
        if (leftLen < rightLen)
                return -1;
        if (leftLen > rightLen)
                return 1;
        return 0;
}

int
R_CSTL_StringEqualsIgnoreCase (const struct R_CSTL_String* pLeft, const struct R_CSTL_String* pRight)
{
        if (pLeft == pRight)
                return 1;
        if (!pLeft || !pRight)
                return 0;
        if (pLeft->length != pRight->length)
                return 0;
        int res = R_CSTL_StringCompareIgnoreCaseInternal (
            R_CSTL_StringData (pLeft),
            pLeft->length,
            R_CSTL_StringData (pRight),
            pRight->length);
        return res == 0;
}

int
R_CSTL_StringCompareIgnoreCase (const struct R_CSTL_String* pLeft, const struct R_CSTL_String* pRight)
{
        if (pLeft == pRight)
                return 0;
        if (!pLeft)
                return -1;
        if (!pRight)
                return 1;
        return R_CSTL_StringCompareIgnoreCaseInternal (
            R_CSTL_StringData (pLeft),
            pLeft->length,
            R_CSTL_StringData (pRight),
            pRight->length);
}

size_t
R_CSTL_StringHashCode (const struct R_CSTL_String* pString)
{
        if (!pString)
                return 0;
        const char* pData = R_CSTL_StringData (pString);
        size_t      len = pString->length;
        size_t      hash = 5381;
        for (size_t i = 0; i < len; ++i)
                hash = ((hash << 5) + hash) + (unsigned char)pData[i];
        return hash;
}

struct R_CSTL_String*
R_CSTL_StringRemove (const struct R_CSTL_String* pString, size_t start, size_t end)
{
        if (!pString)
                return NULL;
        if (!R_CSTL_StringBufferIsLive (pString))
                return NULL;
        if (start >= pString->length || start >= end)
                return R_CSTL_NewStringWithDataSized (R_CSTL_StringData (pString), pString->length);
        if (end > pString->length)
                end = pString->length;

        R_CSTL_StringBuilder* pBuilder = R_CSTL_NewStringBuilder ();
        if (!pBuilder)
                return NULL;

        R_CSTL_StringBuilderAppendData (pBuilder, R_CSTL_StringData (pString), start);
        R_CSTL_StringBuilderAppendData (pBuilder, R_CSTL_StringData (pString) + end, pString->length - end);

        struct R_CSTL_String* pResult = R_CSTL_StringBuilderToString (pBuilder);
        R_CSTL_DeleteStringBuilder (pBuilder);
        return pResult;
}

struct R_CSTL_String*
R_CSTL_StringValueOfInt (int value)
{
        char   buf[32];
        size_t len = (size_t)snprintf (buf, sizeof (buf), "%d", value);
        return R_CSTL_NewStringWithDataSized (buf, len);
}

struct R_CSTL_String*
R_CSTL_StringValueOfLong (long long value)
{
        char   buf[64];
        size_t len = (size_t)snprintf (buf, sizeof (buf), "%lld", value);
        return R_CSTL_NewStringWithDataSized (buf, len);
}

struct R_CSTL_String*
R_CSTL_StringValueOfDouble (double value)
{
        char   buf[64];
        size_t len = (size_t)snprintf (buf, sizeof (buf), "%g", value);
        return R_CSTL_NewStringWithDataSized (buf, len);
}

struct R_CSTL_String*
R_CSTL_StringJoin (
    const struct R_CSTL_String* pStringDelimiter,
    const struct R_CSTL_String* pStringElements[],
    size_t                      count)
{
        if (count == 0)
                return R_CSTL_NewString ();
        R_CSTL_StringBuilder* pBuilder = R_CSTL_NewStringBuilder ();
        if (!pBuilder)
                return NULL;
        for (size_t i = 0; i < count; ++i)
        {
                if (pStringElements[i])
                        R_CSTL_StringBuilderAppend (pBuilder, pStringElements[i]);
                if (i < count - 1 && pStringDelimiter)
                        R_CSTL_StringBuilderAppend (pBuilder, pStringDelimiter);
        }
        struct R_CSTL_String* pResult = R_CSTL_StringBuilderToString (pBuilder);
        R_CSTL_DeleteStringBuilder (pBuilder);
        return pResult;
}

int
R_CSTL_StringCopy (
    struct R_CSTL_String* R_CSTL_RESTRICT       pDst,
    const struct R_CSTL_String* R_CSTL_RESTRICT pSrc)
{
        if (!pDst || !pSrc)
                goto cstl_fail;
        if (!R_CSTL_StringBufferIsLive (pDst) || !R_CSTL_StringBufferIsLive (pSrc))
                goto cstl_fail;
        if (pDst == pSrc)
                return 0;

        if (R_CSTL_StringEnsureCapacityInternal (pDst, pSrc->length) != 0)
                goto cstl_fail;

        const char* pSrcData = R_CSTL_StringData (pSrc);
        char*       pDstData = (char*)R_CSTL_StringData (pDst);

        R_CSTL_StringCopyBytes (pDstData, pSrcData, pSrc->length);
        pDstData[pSrc->length] = '\0';
        pDst->length = pSrc->length;
        return 0;

cstl_fail:
        return -1;
}

R_CSTL_StringBuilder*
R_CSTL_NewStringBuilder (void)
{
        R_CSTL_StringBuilder* pBuilder
            = (R_CSTL_StringBuilder*)R_CSTL_HeapAlloc (sizeof (R_CSTL_StringBuilder));
        if (!pBuilder)
                return NULL;
        pBuilder->length = 0;
        pBuilder->capacity = R_CSTL_SET_STORAGE_SBO (R_CSTL_STRING_SBO_SIZE);
        pBuilder->pData = pBuilder->sbo;
        pBuilder->sbo[0] = '\0';
        return pBuilder;
}

R_CSTL_StringBuilder*
R_CSTL_NewStringBuilderWithData (const char* pString)
{
        R_CSTL_StringBuilder* pBuilder = R_CSTL_NewStringBuilder ();
        if (!pBuilder)
                goto cstl_fail;
        if (!pString)
                goto cstl_fail;
        R_CSTL_StringBuilderEmplace (pBuilder, pString);
        return pBuilder;
cstl_fail:
        return NULL;
}

R_CSTL_StringBuilder*
R_CSTL_NewStringBuilderWithCapacity (size_t capacity)
{
        R_CSTL_StringBuilder* pBuilder
            = (R_CSTL_StringBuilder*)R_CSTL_HeapAlloc (sizeof (R_CSTL_StringBuilder));
        if (!pBuilder)
                return NULL;
        pBuilder->length = 0;
        if (capacity <= R_CSTL_STRING_SBO_SIZE)
        {
                pBuilder->capacity = R_CSTL_SET_STORAGE_SBO (R_CSTL_STRING_SBO_SIZE);
                pBuilder->pData = pBuilder->sbo;
        }
        else
        {
                char* mem = (char*)R_CSTL_HeapAlloc (capacity + 1);
                if (!mem)
                {
                        R_CSTL_HeapFree (pBuilder);
                        return NULL;
                }
#if defined(R_CSTL_HEAP_DEBUG)
                uint64_t h = R_CSTL_HeapRegisterAllocation (
                    pBuilder,
                    mem,
                    capacity + 1,
                    R_CSTL_HEAP_NAME (R_CSTL_NewStringBuilderWithCapacity));
                if (h == 0)
                {
                        R_CSTL_HeapFree (mem);
                        R_CSTL_HeapFree (pBuilder);
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
R_CSTL_DeleteStringBuilder (R_CSTL_StringBuilder* pBuilder)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pBuilder)
                return;
        if (R_CSTL_IS_STORAGE_HEAP (pBuilder->capacity))
        {
                R_CSTL_HeapUnregisterAllocation (pBuilder, pBuilder->pData);
        }
#endif
        if (R_CSTL_IS_STORAGE_HEAP (pBuilder->capacity))
                R_CSTL_HeapFree (pBuilder->pData);
#if defined(R_CSTL_HEAP_DEBUG)
        R_CSTL_HeapCheckObjectLeaks (pBuilder);
#endif
        R_CSTL_HeapFree (pBuilder);
}

R_CSTL_API_ATTR size_t
R_CSTL_StringBuilderLength (const R_CSTL_StringBuilder* pBuilder)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pBuilder)
                goto cstl_fail;
        if (!R_CSTL_StringBufferIsLive ((const struct R_CSTL_String*)pBuilder))
                goto cstl_fail;
#endif
        return pBuilder->length;
cstl_fail:
        return 0;
}

size_t
R_CSTL_StringBuilderCapacity (const R_CSTL_StringBuilder* pBuilder)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pBuilder)
                goto cstl_fail;
        if (!R_CSTL_StringBufferIsLive ((const struct R_CSTL_String*)pBuilder))
                goto cstl_fail;
#endif
        return R_CSTL_GET_CAPACITY (pBuilder->capacity);
cstl_fail:
        return 0;
}

int
R_CSTL_StringBuilderEnsureCapacity (R_CSTL_StringBuilder* pBuilder, size_t requiredCapacity)
{
        if (!pBuilder)
                goto cstl_fail;
        if (!R_CSTL_StringBufferIsLive ((const struct R_CSTL_String*)pBuilder))
                goto cstl_fail;
        if (R_CSTL_GET_CAPACITY (pBuilder->capacity) >= requiredCapacity)
                return 0;
        size_t newCap = R_CSTL_StringNextCapacity (pBuilder->capacity, requiredCapacity);
        char*  old = R_CSTL_IS_STORAGE_HEAP (pBuilder->capacity) ? pBuilder->pData : NULL;
        if (old)
        {
                char* pNew = (char*)R_CSTL_HeapRealloc (old, newCap + 1);
                if (!pNew)
                        return -1;
#if defined(R_CSTL_HEAP_DEBUG)
                if (pNew != old)
                {
                        uint64_t h = R_CSTL_HeapRegisterAllocation (
                            pBuilder,
                            pNew,
                            newCap + 1,
                            R_CSTL_HEAP_NAME (R_CSTL_StringBuilderEnsureCapacity));
                        if (h == 0)
                                goto cstl_fail_register;
                }
#endif
                pBuilder->pData = pNew;
                pBuilder->capacity = R_CSTL_SET_STORAGE_HEAP (newCap);
                return 0;
        }
        else
        {
                char* pNew = (char*)R_CSTL_HeapAlloc (newCap + 1);
                if (!pNew)
                        return -1;
#if defined(R_CSTL_HEAP_DEBUG)
                uint64_t h = R_CSTL_HeapRegisterAllocation (
                    pBuilder,
                    pNew,
                    newCap + 1,
                    R_CSTL_HEAP_NAME (R_CSTL_StringBuilderEnsureCapacity));
                if (h == 0)
                        goto cstl_fail_register;
#endif
                R_CSTL_StringCopyBytes (pNew, pBuilder->sbo, pBuilder->length);
                pNew[pBuilder->length] = '\0';
                pBuilder->pData = pNew;
                pBuilder->capacity = R_CSTL_SET_STORAGE_HEAP (newCap);
                return 0;
        }

cstl_fail_register:
#if defined(R_CSTL_HEAP_DEBUG)
        if (old)
                R_CSTL_HeapUnregisterAllocation (pBuilder, old);
#endif
        return -1;
cstl_fail:
        return (size_t)-1;
}

int
R_CSTL_StringBuilderAppend (R_CSTL_StringBuilder* pBuilder, const struct R_CSTL_String* pString)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pBuilder)
                goto cstl_fail;
        if (!R_CSTL_StringBufferIsLive ((const struct R_CSTL_String*)pBuilder))
                goto cstl_fail;
#endif
        // The pString can be NULL, in which case we treat it as an empty string and do nothing
        if (!pString)
                return 0;
        return R_CSTL_StringBuilderAppendData (pBuilder, R_CSTL_StringData (pString), pString->length);
cstl_fail:
        return -1;
}

#define R_CSTL_EmplaceString(Builder, String)                                                                \
        if (!Builder)                                                                                        \
                goto cstl_fail;                                                                              \
        if (!R_CSTL_StringBufferIsLive ((const struct R_CSTL_String*)Builder))                               \
                goto cstl_fail;                                                                              \
        if (!(String))                                                                                       \
                goto cstl_fail;                                                                              \
        return R_CSTL_StringBuilderAppend (Builder, String);                                                 \
        cstl_fail:                                                                                           \
        return -1

int
R_CSTL_StringBuilderEmplaceSized (R_CSTL_StringBuilder* pBuilder, const char* pCString, const size_t size)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pBuilder)
                goto cstl_fail;
        if (!R_CSTL_StringBufferIsLive ((const struct R_CSTL_String*)pBuilder))
                goto cstl_fail;
        if (!pCString)
                goto cstl_fail;
#endif
        struct R_CSTL_String* pString = R_CSTL_NewStringWithDataSized (pCString, size);
        if (!pString)
                goto cstl_fail;
        int result = R_CSTL_StringBuilderAppend (pBuilder, pString);
        R_CSTL_StringDelete (pString);
        return result;
cstl_fail:
        return -1;
}

int
R_CSTL_StringBuilderEmplace (R_CSTL_StringBuilder* pBuilder, const char* pCString)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pBuilder)
                goto cstl_fail;
        if (!R_CSTL_StringBufferIsLive ((const struct R_CSTL_String*)pBuilder))
                goto cstl_fail;
        if (!pCString)
                goto cstl_fail;
#endif
        struct R_CSTL_String* pString = R_CSTL_NewStringWithData (pCString);
        if (!pString)
                goto cstl_fail;
        int result = R_CSTL_StringBuilderAppend (pBuilder, pString);
        R_CSTL_StringDelete (pString);
        return result;
cstl_fail:
        return -1;
}

int
R_CSTL_StringBuilderAppendData (R_CSTL_StringBuilder* pBuilder, const char* pData, size_t length)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pBuilder || !pData || length == 0)
                return 0;
#endif
        size_t required = pBuilder->length + length;
        if (required > R_CSTL_GET_CAPACITY (pBuilder->capacity))
        {
                if (R_CSTL_StringBuilderEnsureCapacity (pBuilder, required) != 0)
                        return -1;
        }
        R_CSTL_StringCopyBytes (pBuilder->pData + pBuilder->length, pData, length);
        pBuilder->length = required;
        pBuilder->pData[pBuilder->length] = '\0';
        return 0;
}

int
R_CSTL_StringBuilderAppendChar (R_CSTL_StringBuilder* pBuilder, char value)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pBuilder)
                goto cstl_fail;
#endif
        size_t required = pBuilder->length + 1;
        if (required > R_CSTL_GET_CAPACITY (pBuilder->capacity))
        {
                if (R_CSTL_StringBuilderEnsureCapacity (pBuilder, required) != 0)
                        goto cstl_fail;
        }
        pBuilder->pData[pBuilder->length++] = value;
        pBuilder->pData[pBuilder->length] = '\0';
        return 0;
cstl_fail:
        return -1;
}

int
R_CSTL_StringBuilderAppendInt (R_CSTL_StringBuilder* pBuilder, int value)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pBuilder)
                goto cstl_fail;
#endif
        char   buf[32];
        size_t len = (size_t)snprintf (buf, sizeof (buf), "%d", value);
        return R_CSTL_StringBuilderAppendData (pBuilder, buf, len);
cstl_fail:
        return -1;
}

int
R_CSTL_StringBuilderAppendLong (R_CSTL_StringBuilder* pBuilder, long long value)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pBuilder)
                goto cstl_fail;
#endif
        char   buf[64];
        size_t len = (size_t)snprintf (buf, sizeof (buf), "%lld", value);
        return R_CSTL_StringBuilderAppendData (pBuilder, buf, len);
cstl_fail:
        return -1;
}

int
R_CSTL_StringBuilderAppendDouble (R_CSTL_StringBuilder* pBuilder, double value)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pBuilder)
                goto cstl_fail;
#endif
        char   buf[64];
        size_t len = (size_t)snprintf (buf, sizeof (buf), "%g", value);
        return R_CSTL_StringBuilderAppendData (pBuilder, buf, len);
cstl_fail:
        return -1;
}
#define R_CSTL_STRING_BOOL_STR(b) (b ? "true" : "false")

int
R_CSTL_StringBuilderAppendBool (R_CSTL_StringBuilder* pBuilder, bool value)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pBuilder)
                goto cstl_fail;
        if (!R_CSTL_StringBufferIsLive ((const struct R_CSTL_String*)pBuilder))
                goto cstl_fail;
#endif
        const char* boolStr = R_CSTL_STRING_BOOL_STR (value);
        size_t      len = strlen (boolStr);
        return R_CSTL_StringBuilderAppend (pBuilder, R_CSTL_NewStringWithDataSized (boolStr, len));
cstl_fail:
        return -1;
}

int
R_CSTL_StringBuilderAppendRepeat (R_CSTL_StringBuilder* pBuilder, const char* pData, size_t count)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pBuilder || !pData || count == 0)
                return 0;
#endif
        R_CSTL_STRING_STRLEN (pStr);

        if (length == 0)
                return 0;
        size_t total = length * count;
        if (total < length)
                return -1;

        if (R_CSTL_StringBuilderEnsureCapacity (pBuilder, pBuilder->length + total) != 0)
                return -1;

        for (size_t i = 0; i < count; ++i)
                R_CSTL_StringCopyBytes (pBuilder->pData + pBuilder->length + (i * length), pData, length);

        pBuilder->length += total;
        pBuilder->pData[pBuilder->length] = '\0';
        return 0;
}

void
R_CSTL_StringBuilderClear (R_CSTL_StringBuilder* pBuilder)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pBuilder)
                return;
#endif
        pBuilder->length = 0;
        if (pBuilder->pData)
                pBuilder->pData[0] = '\0';
}

int
R_CSTL_StringBuilderInsert (
    R_CSTL_StringBuilder*       pBuilder,
    size_t                      offset,
    const struct R_CSTL_String* pString)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pBuilder || !pString)
                return -1;
#endif
        if (offset > pBuilder->length)
                offset = pBuilder->length;
        return R_CSTL_StringBuilderEmplaceInsert (pBuilder, offset, R_CSTL_StringData (pString));
}

int
R_CSTL_StringBuilderEmplaceInsert (R_CSTL_StringBuilder* pBuilder, size_t offset, const char* pCString)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pBuilder || !pCString)
                return -1;
#endif
        if (offset > pBuilder->length)
                offset = pBuilder->length;
        size_t len = strlen (pCString);
        if (len == 0)
                return 0;

        size_t required = pBuilder->length + len;
        if (required > R_CSTL_GET_CAPACITY (pBuilder->capacity))
        {
                if (R_CSTL_StringBuilderEnsureCapacity (pBuilder, required) != 0)
                        return -1;
        }

        R_CSTL_StringCopyBytes (
            pBuilder->pData + offset + len,
            pBuilder->pData + offset,
            pBuilder->length - offset);
        R_CSTL_StringCopyBytes (pBuilder->pData + offset, pCString, len);
        pBuilder->length += len;
        pBuilder->pData[pBuilder->length] = '\0';
        return 0;
}

int
R_CSTL_StringBuilderInsertChar (R_CSTL_StringBuilder* pBuilder, size_t offset, char value)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pBuilder)
                return -1;
#endif
        if (offset > pBuilder->length)
                offset = pBuilder->length;

        size_t required = pBuilder->length + 1;
        if (required > R_CSTL_GET_CAPACITY (pBuilder->capacity))
        {
                if (R_CSTL_StringBuilderEnsureCapacity (pBuilder, required) != 0)
                        return -1;
        }

        R_CSTL_StringCopyBytes (
            pBuilder->pData + offset + 1,
            pBuilder->pData + offset,
            pBuilder->length - offset);
        pBuilder->pData[offset] = value;
        pBuilder->length++;
        pBuilder->pData[pBuilder->length] = '\0';
        return 0;
}

int
R_CSTL_StringBuilderDelete (R_CSTL_StringBuilder* pBuilder, size_t start, size_t end)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pBuilder)
                return -1;
#endif
        if (start >= pBuilder->length || start >= end)
                return 0;
        if (end > pBuilder->length)
                end = pBuilder->length;

        size_t len = end - start;
        R_CSTL_StringCopyBytes (pBuilder->pData + start, pBuilder->pData + end, pBuilder->length - end);
        pBuilder->length -= len;
        pBuilder->pData[pBuilder->length] = '\0';
        return 0;
}

int
R_CSTL_StringBuilderDeleteCharAt (R_CSTL_StringBuilder* pBuilder, size_t index)
{

#if defined(R_CSTL_HEAP_DEBUG)
        if (!pBuilder || index >= pBuilder->length)
                goto cstl_fail;
        if (!R_CSTL_StringBufferIsLive ((const struct R_CSTL_String*)pBuilder))
                goto cstl_fail;
#endif
        return R_CSTL_StringBuilderDelete (pBuilder, index, index + 1);
cstl_fail:
        return -1;
}

int
R_CSTL_StringBuilderReplace (
    R_CSTL_StringBuilder*       pBuilder,
    size_t                      start,
    size_t                      end,
    const struct R_CSTL_String* pString)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pBuilder || !pString)
                return -1;
#endif
        return R_CSTL_StringBuilderEmplaceReplace (pBuilder, start, end, R_CSTL_StringData (pString));
}

int
R_CSTL_StringBuilderEmplaceReplace (
    R_CSTL_StringBuilder* pBuilder,
    size_t                start,
    size_t                end,
    const char*           pString)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pBuilder || !pString)
                return -1;
        if (start > pBuilder->length)
                return -1;
#endif
        if (end > pBuilder->length)
                end = pBuilder->length;
        if (start >= end)
                return R_CSTL_StringBuilderEmplaceInsert (pBuilder, start, pString);
        size_t oldLen = end - start;
        size_t newLen = strlen (pString);

        if (newLen > oldLen)
        {
                size_t delta = newLen - oldLen;
                size_t required = pBuilder->length + delta;
                if (required > R_CSTL_GET_CAPACITY (pBuilder->capacity))
                {
                        if (R_CSTL_StringBuilderEnsureCapacity (pBuilder, required) != 0)
                                return -1;
                }
                R_CSTL_StringCopyBytes (
                    pBuilder->pData + end + delta,
                    pBuilder->pData + end,
                    pBuilder->length - end);
        }
        else if (newLen < oldLen)
        {
                size_t delta = oldLen - newLen;
                R_CSTL_StringCopyBytes (
                    pBuilder->pData + end - delta,
                    pBuilder->pData + end,
                    pBuilder->length - end);
        }

        R_CSTL_StringCopyBytes (pBuilder->pData + start, pString, newLen);
        pBuilder->length = pBuilder->length - oldLen + newLen;
        pBuilder->pData[pBuilder->length] = '\0';
        return 0;
}

int
R_CSTL_StringBuilderSetCharAt (R_CSTL_StringBuilder* pBuilder, size_t index, char value)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pBuilder || index >= pBuilder->length)
                goto cstl_fail;
        if (!R_CSTL_StringBufferIsLive ((const struct R_CSTL_String*)pBuilder))
                goto cstl_fail;
#endif
        pBuilder->pData[index] = value;
        return 0;
cstl_fail:
        return -1;
}

int
R_CSTL_StringBuilderSetLength (R_CSTL_StringBuilder* pBuilder, size_t newLength)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pBuilder)
                return -1;
#endif
        if (newLength > R_CSTL_GET_CAPACITY (pBuilder->capacity))
        {
                if (R_CSTL_StringBuilderEnsureCapacity (pBuilder, newLength) != 0)
                        return -1;
        }
        if (newLength < pBuilder->length)
        {
                pBuilder->length = newLength;
                pBuilder->pData[pBuilder->length] = '\0';
        }
        else if (newLength > pBuilder->length)
        {
                R_CSTL_StringSetBytes (
                    pBuilder->pData + pBuilder->length,
                    '\0',
                    newLength - pBuilder->length);
                pBuilder->length = newLength;
        }
        return 0;
}

int
R_CSTL_StringBuilderReverse (R_CSTL_StringBuilder* pBuilder)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pBuilder)
                return -1;
#endif
        R_CSTL_StringReverseBytes (pBuilder->pData, pBuilder->length);
        return 0;
}

struct R_CSTL_String*
R_CSTL_StringBuilderToString (const R_CSTL_StringBuilder* pBuilder)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pBuilder)
                goto cstl_fail;
        if (pBuilder->length == 0)
                goto cstl_fail;
#endif
        return R_CSTL_NewStringWithDataSized (pBuilder->pData, pBuilder->length);
cstl_fail:
        return R_CSTL_NewString ();
}
