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

struct R_CSTL_Stack
{
                uint8_t* pData;
                size_t   size;
                size_t   capacity;
};

R_CSTL_API_ATTR static int
R_CSTL_StackBufferIsUsable (const struct R_CSTL_Stack* pStack)
{
        if (!pStack) return 0;
#ifndef R_CSTL_HEAP_DEBUG
        return 1;
#else
        if (!pStack->pData) return 1;
        return R_CSTL_HeapIsValidPointer (pStack->pData) != 0;
#endif
}

R_CSTL_API_ATTR static int
R_CSTL_StackBufferIsLive (const struct R_CSTL_Stack* pStack)
{
        if (!pStack || !pStack->pData) return 0;
#ifndef R_CSTL_HEAP_DEBUG
        return 1;
#else
        return R_CSTL_HeapIsValidPointer (pStack->pData) != 0;
#endif
}

R_CSTL_API_ATTR static size_t
R_CSTL_StackNextCapacity (size_t current, size_t required)
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
R_CSTL_StackCopyBytes (uint8_t* pDst, const uint8_t* pSrc, size_t sizeBytes)
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
R_CSTL_StackReleaseBuffer (struct R_CSTL_Stack* pStack)
{
        if (!pStack || !pStack->pData) return;

#if defined(R_CSTL_HEAP_DEBUG)
        if (!R_CSTL_HeapIsValidPointer (pStack->pData)) goto cstl_fail;
        R_CSTL_HeapUnregisterAllocation (pStack, pStack->pData);
#endif

        R_CSTL_HeapFree (pStack->pData);
        pStack->pData = NULL;
        pStack->capacity = 0;

#if defined(R_CSTL_HEAP_DEBUG)
cstl_fail:
#endif
        return;
}

static int
R_CSTL_StackReallocate (struct R_CSTL_Stack* pStack, size_t newCapacity)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pStack || newCapacity == 0) goto cstl_fail;
#endif
        uint8_t* pOldData = pStack->pData;
        size_t   oldSize = pStack->size;
#if defined(R_CSTL_HEAP_DEBUG)
        if (pOldData && !R_CSTL_HeapIsValidPointer (pOldData)) goto cstl_fail;
#endif

        uint8_t* pNew = (uint8_t*)R_CSTL_HeapAlloc (newCapacity);
        if (!pNew) return -1;

        if (pOldData && oldSize > 0)
        {
                memcpy (pNew, pOldData, oldSize);
        }

        if (pOldData)
        {
#if defined(R_CSTL_HEAP_DEBUG)
                if (R_CSTL_HeapIsValidPointer (pOldData)) R_CSTL_HeapUnregisterAllocation (pStack, pOldData);
#endif
                R_CSTL_HeapFree (pOldData);
        }

#if defined(R_CSTL_HEAP_DEBUG)
        uint64_t success = R_CSTL_HeapRegisterAllocation (
            pStack,
            pNew,
            newCapacity,
            R_CSTL_HEAP_NAME (R_CSTL_StackReallocate));
        if (success == 0) goto cstl_fail_register;
#endif

        pStack->pData = pNew;
        pStack->capacity = newCapacity;
        return 0;

cstl_fail:
        return -1;
cstl_fail_register:
        R_CSTL_HeapFree (pNew);
        return -1;
}

static int
R_CSTL_StackEnsureCapacity (struct R_CSTL_Stack* pStack, size_t requiredCapacity)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pStack) goto cstl_fail;
        if (!R_CSTL_StackBufferIsUsable (pStack)) goto cstl_fail;
#endif
        if (pStack->capacity >= requiredCapacity) return 0;

        size_t newCapacity = R_CSTL_StackNextCapacity (pStack->capacity, requiredCapacity);
        return R_CSTL_StackReallocate (pStack, newCapacity);

cstl_fail:
        return -1;
}

R_CSTL_API_ATTR static struct R_CSTL_Stack*
R_CSTL_StackCreateShell (void)
{
        struct R_CSTL_Stack* pStack = (struct R_CSTL_Stack*)R_CSTL_HeapAlloc (sizeof (struct R_CSTL_Stack));
        if (!pStack) return NULL;
        pStack->pData = NULL;
        pStack->size = 0;
        pStack->capacity = 0;
        return pStack;
}

R_CSTL_API_ATTR struct R_CSTL_Stack*
R_CSTL_NewStack (void)
{
        return R_CSTL_StackCreateShell ();
}

R_CSTL_API_ATTR struct R_CSTL_Stack*
R_CSTL_NewStackWithCapacity (size_t capacityBytes)
{
        struct R_CSTL_Stack* pStack = R_CSTL_StackCreateShell ();
        if (!pStack) goto cstl_fail;

        if (capacityBytes == 0) return pStack;

        if (R_CSTL_StackReserve (pStack, capacityBytes) != 0) goto cstl_fail;

        return pStack;

cstl_fail:
        R_CSTL_DeleteStack (pStack);
        return NULL;
}

R_CSTL_API_ATTR void
R_CSTL_DeleteStack (struct R_CSTL_Stack* pStack)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pStack) return;
#endif
        R_CSTL_StackReleaseBuffer (pStack);
        R_CSTL_HeapFree (pStack);
}

R_CSTL_API_ATTR int
R_CSTL_StackReserve (struct R_CSTL_Stack* pStack, size_t capacityBytes)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pStack) goto cstl_fail;
        if (!R_CSTL_StackBufferIsUsable (pStack)) goto cstl_fail;
#endif
        if (capacityBytes <= pStack->capacity) return 0;

        return R_CSTL_StackReallocate (pStack, capacityBytes);

cstl_fail:
        return -1;
}

R_CSTL_API_ATTR int
R_CSTL_StackPush (struct R_CSTL_Stack* pStack, uint8_t value)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pStack) goto cstl_fail;
        if (!R_CSTL_StackBufferIsUsable (pStack)) goto cstl_fail;
#endif
        if (R_CSTL_StackEnsureCapacity (pStack, pStack->size + 1) != 0) goto cstl_fail;
        pStack->pData[pStack->size] = value;
        ++pStack->size;
        return 0;

cstl_fail:
        return -1;
}

R_CSTL_API int
R_CSTL_StackPushData (struct R_CSTL_Stack* pStack, const uint8_t* pData, size_t size)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pStack || !pData) goto cstl_fail;
        if (!R_CSTL_StackBufferIsUsable (pStack)) goto cstl_fail;
#endif
        if (size == 0) return 0;
        if (R_CSTL_StackEnsureCapacity (pStack, pStack->size + size) != 0) goto cstl_fail;
        R_CSTL_StackCopyBytes (pStack->pData + pStack->size, pData, size);
        pStack->size += size;
        return 0;
cstl_fail:
        return -1;
}

R_CSTL_API_ATTR int
R_CSTL_StackPop (struct R_CSTL_Stack* pStack, uint8_t* pOutValue)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pStack || pStack->size == 0) return -1;
        if (!R_CSTL_StackBufferIsLive (pStack)) goto cstl_fail;
#endif
        uint8_t value = pStack->pData[pStack->size - 1];
        --pStack->size;
        if (pOutValue) *pOutValue = value;
        return 0;

cstl_fail:
        return -1;
}

R_CSTL_API_ATTR int
R_CSTL_StackPopData (struct R_CSTL_Stack* pStack, uint8_t* pOutData, size_t size)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pStack || pOutData == NULL) goto cstl_fail;
        if (size > pStack->size) goto cstl_fail;
        if (!R_CSTL_StackBufferIsLive (pStack)) goto cstl_fail;
#endif
        size_t offset = pStack->size - size;
        memcpy (pOutData, pStack->pData + offset, size);
        pStack->size -= size;
        return 0;

cstl_fail:
        return -1;
}

R_CSTL_API_ATTR int
R_CSTL_StackPeek (struct R_CSTL_Stack* pStack, uint8_t* pOutValue)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pStack || pStack->size == 0) return -1;
        if (!R_CSTL_StackBufferIsLive (pStack)) goto cstl_fail;
#endif
        if (pOutValue) *pOutValue = pStack->pData[pStack->size - 1];
        return 0;

cstl_fail:
        return -1;
}

R_CSTL_API_ATTR int
R_CSTL_StackPeekData (struct R_CSTL_Stack* pStack, uint8_t* pOutData, size_t size)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pStack || pOutData == NULL) goto cstl_fail;
        if (size > pStack->size) goto cstl_fail;
        if (!R_CSTL_StackBufferIsLive (pStack)) goto cstl_fail;
#endif
        size_t offset = pStack->size - size;
        memcpy (pOutData, pStack->pData + offset, size);
        return 0;

cstl_fail:
        return -1;
}

R_CSTL_API_ATTR int
R_CSTL_StackEmpty (const struct R_CSTL_Stack* pStack)
{
        if (!pStack) return 1;
        return pStack->size == 0 ? 1 : 0;
}

R_CSTL_API_ATTR size_t
R_CSTL_StackSize (const struct R_CSTL_Stack* pStack)
{
        if (!pStack) return 0;
        return pStack->size;
}

R_CSTL_API_ATTR size_t
R_CSTL_StackGetCapacity (const struct R_CSTL_Stack* pStack)
{
        if (!pStack) return 0;
        return pStack->capacity;
}

R_CSTL_API_ATTR size_t
R_CSTL_StackSearch (const struct R_CSTL_Stack* pStack, uint8_t value)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pStack) return 0;
        if (!R_CSTL_StackBufferIsLive (pStack)) return 0;
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
R_CSTL_StackSearchData (const struct R_CSTL_Stack* pStack, const uint8_t* pData, size_t size)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pStack || !pData || size == 0) return 0;
        if (!R_CSTL_StackBufferIsLive (pStack)) return 0;
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
R_CSTL_StackClear (struct R_CSTL_Stack* pStack, int zeroMemory)
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
R_CSTL_StackData (const struct R_CSTL_Stack* pStack)
{
#if defined(R_CSTL_HEAP_DEBUG)
        if (!pStack) goto cstl_fail;
        if (!R_CSTL_StackBufferIsLive (pStack)) goto cstl_fail;
#endif
        return pStack->pData;
cstl_fail:
        return NULL;
}
