#include "rlgame.base/cstl/cstl_bytecode.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>

#if defined(R_SIMD_SSE) || defined(R_SIMD_AVX2)
#include <immintrin.h>
#endif

#if defined(_WIN32) || defined(_WIN64)
#define R_CSTL_MUTEX_WINDOWS
#elif defined(__linux__)
#define R_CSTL_MUTEX_PTHREAD
#endif

#ifdef R_CSTL_MUTEX_WINDOWS
#include <windows.h>
#elif defined(R_CSTL_MUTEX_PTHREAD)
#include <pthread.h>
#endif

#define R_CSTL_X86_OPCODE_ESCAPE 0x0F
#define R_CSTL_X86_OPCODE_THREEBYTE_38 0x38
#define R_CSTL_X86_OPCODE_THREEBYTE_3A 0x3A
#define R_CSTL_X86_REX_MIN 0x40
#define R_CSTL_X86_REX_MAX 0x4F
#define R_CSTL_X86_PREFIX_LOCK 0xF0
#define R_CSTL_X86_PREFIX_REPNE 0xF2
#define R_CSTL_X86_PREFIX_REPE 0xF3
#define R_CSTL_X86_PREFIX_DATA_SIZE 0x66
#define R_CSTL_X86_PREFIX_ADDR_SIZE 0x67
#define R_CSTL_X86_COND_JUMP_MIN 0x80
#define R_CSTL_X86_COND_JUMP_MAX 0x8F
#define R_CSTL_X86_MAX_INSTRUCTION_LENGTH 15

struct R_CSTL_Mutex
{
#ifdef R_CSTL_MUTEX_WINDOWS
                CRITICAL_SECTION handle;
#elif defined(R_CSTL_MUTEX_PTHREAD)
                pthread_mutex_t handle;
#endif
};

R_CSTL_API int  R_CSTL_MutexInit (struct R_CSTL_Mutex* pMutex);
R_CSTL_API void R_CSTL_MutexLock (struct R_CSTL_Mutex* pMutex);
R_CSTL_API void R_CSTL_MutexUnlock (struct R_CSTL_Mutex* pMutex);
R_CSTL_API void R_CSTL_MutexDestroy (struct R_CSTL_Mutex* pMutex);

static inline size_t
R_CSTL_ScanPrefixes (const uint8_t* p, size_t remaining, size_t maxScan)
{
        size_t i = 0;
#if defined(R_SIMD_AVX2)
        while (i + 32 <= remaining && i < maxScan)
        {
                __m256i chunk = _mm256_loadu_si256 ((__m256i const*)(p + i));

                // Check for prefix bytes using constants
                __m256i cmpLock = _mm256_cmpeq_epi8 (chunk, _mm256_set1_epi8 (R_CSTL_X86_PREFIX_LOCK));
                __m256i cmpRepne = _mm256_cmpeq_epi8 (chunk, _mm256_set1_epi8 (R_CSTL_X86_PREFIX_REPNE));
                __m256i cmpRepe = _mm256_cmpeq_epi8 (chunk, _mm256_set1_epi8 (R_CSTL_X86_PREFIX_REPE));
                __m256i cmpDataSize = _mm256_cmpeq_epi8 (chunk, _mm256_set1_epi8 (R_CSTL_X86_PREFIX_DATA_SIZE));
                __m256i cmpAddrSize = _mm256_cmpeq_epi8 (chunk, _mm256_set1_epi8 (R_CSTL_X86_PREFIX_ADDR_SIZE));
                __m256i cmpRexMin = _mm256_cmpeq_epi8 (chunk, _mm256_set1_epi8 (R_CSTL_X86_REX_MIN));
                __m256i cmpRexMax = _mm256_cmpeq_epi8 (chunk, _mm256_set1_epi8 (R_CSTL_X86_REX_MAX));

                __m256i isPrefix = _mm256_or_si256 (
                    _mm256_or_si256 (_mm256_or_si256 (cmpLock, cmpRepne), _mm256_or_si256 (cmpRepe, cmpDataSize)),
                    _mm256_or_si256 (cmpAddrSize, _mm256_or_si256 (cmpRexMin, cmpRexMax)));

                int mask = _mm256_movemask_epi8 (isPrefix);
                if (mask == 0xFFFFFFFF)
                {
                        i += 32;
                }
                else
                {
                        // Find first non-prefix byte
                        for (int j = 0; j < 32 && i + j < remaining && i + j < maxScan; ++j)
                        {
                                if (!(mask & (1 << j))) return i + j;
                        }
                        i += 32;
                }
        }
#elif defined(R_SIMD_SSE)
        while (i + R_CSTL_X86_SIMD_VECTOR_SIZE <= remaining && i < maxScan)
        {
                __m128i chunk = _mm_loadu_si128 ((__m128i const*)(p + i));

                // Check for prefix bytes using constants
                __m128i cmpLock = _mm_cmpeq_epi8 (chunk, _mm_set1_epi8 (R_CSTL_X86_PREFIX_LOCK));
                __m128i cmpRepne = _mm_cmpeq_epi8 (chunk, _mm_set1_epi8 (R_CSTL_X86_PREFIX_REPNE));
                __m128i cmpRepe = _mm_cmpeq_epi8 (chunk, _mm_set1_epi8 (R_CSTL_X86_PREFIX_REPE));
                __m128i cmpDataSize = _mm_cmpeq_epi8 (chunk, _mm_set1_epi8 (R_CSTL_X86_PREFIX_DATA_SIZE));
                __m128i cmpAddrSize = _mm_cmpeq_epi8 (chunk, _mm_set1_epi8 (R_CSTL_X86_PREFIX_ADDR_SIZE));
                __m128i cmpRexMin = _mm_cmpeq_epi8 (chunk, _mm_set1_epi8 (R_CSTL_X86_REX_MIN));
                __m128i cmpRexMax = _mm_cmpeq_epi8 (chunk, _mm_set1_epi8 (R_CSTL_X86_REX_MAX));

                __m128i isPrefix = _mm_or_si128 (
                    _mm_or_si128 (_mm_or_si128 (cmpLock, cmpRepne), _mm_or_si128 (cmpRepe, cmpDataSize)),
                    _mm_or_si128 (cmpAddrSize, _mm_or_si128 (cmpRexMin, cmpRexMax)));

                int mask = _mm_movemask_epi8 (isPrefix);
                if (mask == 0xFFFF)
                {
                        i += 16;
                }
                else
                {
                        // Find first non-prefix byte
                        for (int j = 0; j < 16 && i + j < remaining && i + j < maxScan; ++j)
                        {
                                if (!(mask & (1 << j))) return i + j;
                        }
                        i += 16;
                }
        }
#endif
        while (i < remaining && i < maxScan
               && (p[i] == R_CSTL_X86_PREFIX_LOCK || p[i] == R_CSTL_X86_PREFIX_REPNE
                   || p[i] == R_CSTL_X86_PREFIX_REPE || p[i] == R_CSTL_X86_PREFIX_DATA_SIZE
                   || p[i] == R_CSTL_X86_PREFIX_ADDR_SIZE
                   || (p[i] >= R_CSTL_X86_REX_MIN && p[i] <= R_CSTL_X86_REX_MAX)))
                ++i;

        return i;
}

static inline void
R_CSTL_CopyBytes (uint8_t* dst, const uint8_t* src, size_t size)
{
        if (size == 0) return;

#if defined(R_SIMD_AVX2)
        size_t i = 0;
        while (i + 32 <= size)
        {
                __m256i chunk = _mm256_loadu_si256 ((__m256i const*)(src + i));
                _mm256_storeu_si256 ((__m256i*)(dst + i), chunk);
                i += 32;
        }
        // Handle remaining bytes with SSE
        while (i + 32 <= size)
        {
                __m128i chunk = _mm_loadu_si128 ((__m128i const*)(src + i));
                _mm_storeu_si128 ((__m128i*)(dst + i), chunk);
                i += 32;
        }
#elif defined(R_SIMD_SSE)
        while (i + 16 <= size)
        {
                __m128i chunk = _mm_loadu_si128 ((__m128i const*)(src + i));
                _mm_storeu_si128 ((__m128i*)(dst + i), chunk);
                i += 16;
        }
#endif
        memcpy (dst, src, size);
}

R_CSTL_API int
R_CSTL_MutexInit (struct R_CSTL_Mutex* pMutex)
{
        if (!pMutex) return R_CSTL_ERROR_INVALID_ARGUMENT;
#if defined(_WIN32) || defined(_WIN64)
        InitializeCriticalSection (&pMutex->handle);
        return R_CSTL_OK;
#elif defined(__linux__)
        if (pthread_mutex_init (&pMutex->handle, NULL) != 0) return R_CSTL_ERROR_UNKNOWN;
        return R_CSTL_OK;
#else
        return R_CSTL_ERROR_EXECUTABLE_TYPE_NOT_SUPPORTED;
#endif
}

R_CSTL_API void
R_CSTL_MutexLock (struct R_CSTL_Mutex* pMutex)
{
        if (!pMutex) return;
#if defined(_WIN32) || defined(_WIN64)
        EnterCriticalSection (&pMutex->handle);
#elif defined(__linux__)
        pthread_mutex_lock (&pMutex->handle);
#endif
}

R_CSTL_API void
R_CSTL_MutexUnlock (struct R_CSTL_Mutex* pMutex)
{
        if (!pMutex) return;
#if defined(_WIN32) || defined(_WIN64)
        LeaveCriticalSection (&pMutex->handle);
#elif defined(__linux__)
        pthread_mutex_unlock (&pMutex->handle);
#endif
}

R_CSTL_API void
R_CSTL_MutexDestroy (struct R_CSTL_Mutex* pMutex)
{
        if (!pMutex) return;
#if defined(_WIN32) || defined(_WIN64)
        DeleteCriticalSection (&pMutex->handle);
#elif defined(__linux__)
        pthread_mutex_destroy (&pMutex->handle);
#endif
}

#if defined(R_CSTL_LOG_DEVMODE)
#if defined(R_CSTL_PLATFORM_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dbghelp.h>
#include <psapi.h>
#pragma comment(lib, "dbghelp.lib")
#pragma comment(lib, "psapi.lib")
#elif defined(R_CSTL_PLATFORM_LINUX)
#include <dlfcn.h>
#include <link.h>
#include <execinfo.h>
#endif
#endif

struct R_CSTL_Bytecode
{
                const uint8_t*                      pCode;
                uint8_t*                            pOwnedCode;
                size_t                              size;
                enum R_CSTL_BytecodeArchitecture architecture;
                struct R_CSTL_Mutex                 mutex;
                bool                                mutexInitialized;
};

static int
R_CSTL_BytecodeArchitectureIsValid (enum R_CSTL_BytecodeArchitecture architecture)
{
        return architecture >= R_CSTL_BYTECODE_ARCH_X86 && architecture <= R_CSTL_BYTECODE_ARCH_RISC;
}

static struct R_CSTL_Bytecode*
R_CSTL_BytecodeCreate (
    const uint8_t*                      pCode,
    size_t                              sizeBytes,
    enum R_CSTL_BytecodeArchitecture architecture)
{
#if defined(R_CSTL_LOG_DEVMODE)
        assert (R_CSTL_BytecodeArchitectureIsValid (architecture) || "Invalid architecture");
        assert ((pCode != NULL) == (sizeBytes != 0) || "Invalid code pointer/size combination");
#endif

        if ((!pCode && sizeBytes != 0) || !R_CSTL_BytecodeArchitectureIsValid (architecture)) return NULL;

        struct R_CSTL_Bytecode* pBytecode
            = (struct R_CSTL_Bytecode*)R_CSTL_HeapAlloc (sizeof (*pBytecode));
        if (!pBytecode) return NULL;

#if defined(R_CSTL_LOG_DEVMODE)
        assert (pBytecode != NULL);
#endif

        pBytecode->pCode = pCode;
        pBytecode->pOwnedCode = NULL;
        pBytecode->size = sizeBytes;
        pBytecode->architecture = architecture;
        pBytecode->mutexInitialized = false;

        if (R_CSTL_MutexInit (&pBytecode->mutex) == R_CSTL_OK) pBytecode->mutexInitialized = true;

#if defined(R_CSTL_LOG_DEVMODE)
        assert (pBytecode->pCode != NULL || sizeBytes == 0);
        assert (pBytecode->size == sizeBytes);
#endif

        return pBytecode;
}

R_CSTL_API struct R_CSTL_Bytecode*
R_CSTL_NewBytecodeView (
    const void*                         pCode,
    size_t                              sizeBytes,
    enum R_CSTL_BytecodeArchitecture architecture)
{
        return R_CSTL_BytecodeCreate ((const uint8_t*)pCode, sizeBytes, architecture);
}

R_CSTL_API struct R_CSTL_Bytecode*
R_CSTL_NewBytecodeWithData (
    const uint8_t*                      pCode,
    size_t                              sizeBytes,
    enum R_CSTL_BytecodeArchitecture architecture)
{
        struct R_CSTL_Bytecode* pBytecode = R_CSTL_BytecodeCreate (pCode, sizeBytes, architecture);
        if (!pBytecode || sizeBytes == 0) return pBytecode;
        pBytecode->pOwnedCode = (uint8_t*)R_CSTL_HeapAlloc (sizeBytes);
        if (!pBytecode->pOwnedCode)
        {
                R_CSTL_HeapFree (pBytecode);
                return NULL;
        }
        memcpy (pBytecode->pOwnedCode, pCode, sizeBytes);
        pBytecode->pCode = pBytecode->pOwnedCode;
        return pBytecode;
}

R_CSTL_API struct R_CSTL_Bytecode*
R_CSTL_NewBytecodeFromFunction (
    R_CSTL_BytecodeFunction          pFunction,
    size_t                              sizeBytes,
    enum R_CSTL_BytecodeArchitecture architecture)
{
        if (!pFunction) return NULL;
        return R_CSTL_NewBytecodeView ((const void*)(uintptr_t)pFunction, sizeBytes, architecture);
}

R_CSTL_API void
R_CSTL_DeleteBytecode (struct R_CSTL_Bytecode* pBytecode)
{
        if (!pBytecode) return;
        if (pBytecode->mutexInitialized) R_CSTL_MutexDestroy (&pBytecode->mutex);
        if (pBytecode->pOwnedCode) R_CSTL_HeapFree (pBytecode->pOwnedCode);
        R_CSTL_HeapFree (pBytecode);
}

R_CSTL_API int
R_CSTL_BytecodeRead (
    const struct R_CSTL_Bytecode* pBytecode,
    size_t                           offset,
    uint8_t*                         pOutBytes,
    size_t                           sizeBytes)
{
#if defined(R_CSTL_LOG_DEVMODE)
        assert (pBytecode != NULL || "Machine code pointer is null");
        assert (pBytecode->pCode != NULL || pBytecode->size == 0);
        assert (offset <= pBytecode->size || "Offset exceeds code size");
        assert (sizeBytes <= pBytecode->size - offset || "Read size exceeds available bytes");
#endif

        if (!pBytecode || (!pOutBytes && sizeBytes != 0)) return R_CSTL_ERROR_INVALID_ARGUMENT;
        if (offset > pBytecode->size || sizeBytes > pBytecode->size - offset)
                return R_CSTL_ERROR_BUFFER_TOO_SMALL;

        R_CSTL_MutexLock ((struct R_CSTL_Mutex*)&pBytecode->mutex);
        if (sizeBytes) memcpy (pOutBytes, pBytecode->pCode + offset, sizeBytes);
        R_CSTL_MutexUnlock ((struct R_CSTL_Mutex*)&pBytecode->mutex);

        return R_CSTL_OK;
}

static int
R_CSTL_BytecodeParseX86 (
    const struct R_CSTL_Bytecode*      pBytecode,
    size_t                                offset,
    struct R_CSTL_BytecodeInstruction* pOut)
{
        const uint8_t* p = pBytecode->pCode + offset;
        size_t         remaining = pBytecode->size - offset;

        size_t i = R_CSTL_ScanPrefixes (p, remaining, R_CSTL_X86_MAX_INSTRUCTION_LENGTH);
        if (i >= remaining) return R_CSTL_ERROR_BUFFER_TOO_SMALL;

        size_t  opcodeStart = i;
        uint8_t opcode = p[i++];

        // Handle multi-byte opcodes
        if (opcode == R_CSTL_X86_OPCODE_ESCAPE)
        {
                if (i >= remaining) return R_CSTL_ERROR_BUFFER_TOO_SMALL;
                opcode = p[i++];

                if (opcode == R_CSTL_X86_OPCODE_THREEBYTE_38 || opcode == R_CSTL_X86_OPCODE_THREEBYTE_3A)
                {
                        if (i >= remaining) return R_CSTL_ERROR_BUFFER_TOO_SMALL;
                        ++i;
                }
        }

        // Enhanced ModRM detection for better opcode handling
        int hasModRM = 1;
        if ((opcode >= 0x50 && opcode <= 0x5F) || opcode == 0x6A || opcode == 0x6B || opcode == 0x90
            || opcode == 0xC3 || opcode == 0xC9 || opcode == 0xE8 || opcode == 0xE9 || opcode == 0xEB
            || opcode == 0xEA || opcode == 0xE3 || opcode == 0xE0 || opcode == 0xE1 || opcode == 0xE2
            || (opcode < 0x40))
                hasModRM = 0;

        if (opcode == 0x8B || opcode == 0x89 || opcode == 0x8D || opcode == 0xC7 || opcode == 0xFF
            || opcode == 0x01 || opcode == 0x03 || opcode == 0x0F)
                hasModRM = 1;

        // Parse ModRM if present
        if (hasModRM)
        {
                if (i >= remaining) return R_CSTL_ERROR_BUFFER_TOO_SMALL;
                uint8_t modrm = p[i++];
                uint8_t mod = (uint8_t)(modrm >> 6);
                uint8_t rm = (uint8_t)(modrm & 7);

                // Handle SIB byte
                if (mod != 3 && rm == 4)
                {
                        if (i >= remaining) return R_CSTL_ERROR_BUFFER_TOO_SMALL;
                        uint8_t sib = p[i++];
                        if (mod == 0 && (sib & 7) == 5) i += 4;
                }

                // Handle displacement
                if (mod == 0 && rm == 5) i += 4;
                else if (mod == 1) i += 1;
                else if (mod == 2) i += 4;
        }

        // Handle immediate operands
        if (opcode == 0x68 || opcode == 0x69 || opcode == 0x81 || opcode == 0xC7) i += 4;
        else if (
            opcode == 0x6A || opcode == 0x6B || opcode == 0x83 || opcode == 0xCD || opcode == 0xEB
            || opcode == 0xE3 || opcode == 0xE0 || opcode == 0xE1 || opcode == 0xE2)
                i += 1;
        else if (opcode == 0xE8 || opcode == 0xE9 || opcode == 0xEA) i += 4;

        if (i > remaining || i == 0 || i > R_CSTL_X86_MAX_INSTRUCTION_LENGTH)
                return R_CSTL_ERROR_BUFFER_TOO_SMALL;

        memset (pOut, 0, sizeof (*pOut));
        pOut->offset = offset;
        pOut->size = (uint8_t)i;
        pOut->opcodeSize = (uint8_t)(i - opcodeStart);
        pOut->opcode = opcode;
        R_CSTL_CopyBytes (pOut->bytes, p, i);

        return R_CSTL_OK;
}

#if defined(R_CSTL_LOG_DEVMODE)

static int
R_CSTL_BytecodeParseX86Enhanced (
    const struct R_CSTL_Bytecode*      pBytecode,
    size_t                                offset,
    struct R_CSTL_BytecodeInstruction* pOut)
{
        const uint8_t* p = pBytecode->pCode + offset;
        size_t         remaining = pBytecode->size - offset;

        uint8_t rexPrefix = 0;
        uint8_t legacyPrefixes = 0;
        size_t  i = 0;

        while (i < remaining && i < R_CSTL_X86_MAX_INSTRUCTION_LENGTH)
        {
                if (p[i] == R_CSTL_X86_PREFIX_LOCK || p[i] == R_CSTL_X86_PREFIX_REPNE
                    || p[i] == R_CSTL_X86_PREFIX_REPE || p[i] == R_CSTL_X86_PREFIX_DATA_SIZE
                    || p[i] == R_CSTL_X86_PREFIX_ADDR_SIZE)
                {
                        legacyPrefixes |= (1 << (p[i] & 0x07));
                        ++i;
                }
                else if (p[i] >= R_CSTL_X86_REX_MIN && p[i] <= R_CSTL_X86_REX_MAX)
                {
                        rexPrefix = p[i];
                        ++i;
                }
                else break;
        }

        if (i >= remaining) return R_CSTL_ERROR_BUFFER_TOO_SMALL;

        size_t  opcodeStart = i;
        uint8_t opcode = p[i++];
        uint8_t originalOpcode = opcode; // Store original for conditional jump detection

        // Handle multi-byte opcodes
        if (opcode == R_CSTL_X86_OPCODE_ESCAPE)
        {
                if (i >= remaining) return R_CSTL_ERROR_BUFFER_TOO_SMALL;
                opcode = p[i++];

                // Handle 3-byte opcodes
                if (opcode == R_CSTL_X86_OPCODE_THREEBYTE_38 || opcode == R_CSTL_X86_OPCODE_THREEBYTE_3A)
                {
                        if (i >= remaining) return R_CSTL_ERROR_BUFFER_TOO_SMALL;
                        ++i;
                }
        }
        int hasModRM = 1;
        if ((opcode >= 0x50 && opcode <= 0x5F) || opcode == 0x6A || opcode == 0x6B || opcode == 0x90
            || opcode == 0xC3 || opcode == 0xC9 || opcode == 0xE8 || opcode == 0xE9 || opcode == 0xEB
            || opcode == 0xEA || opcode == 0xE3 || opcode == 0xE0 || opcode == 0xE1 || opcode == 0xE2
            || (opcode < 0x40))
                hasModRM = 0;

        if (opcode == 0x8B || opcode == 0x89 || opcode == 0x8D || opcode == 0xC7 || opcode == 0xFF
            || opcode == 0x01 || opcode == 0x03 || opcode == 0x0F)
                hasModRM = 1;
        if (hasModRM)
        {
                if (i >= remaining) return R_CSTL_ERROR_BUFFER_TOO_SMALL;
                uint8_t modrm = p[i++];
                uint8_t mod = (uint8_t)(modrm >> 6);
                uint8_t rm = (uint8_t)(modrm & 7);

                // Handle SIB byte
                if (mod != 3 && rm == 4)
                {
                        if (i >= remaining) return R_CSTL_ERROR_BUFFER_TOO_SMALL;
                        uint8_t sib = p[i++];
                        if (mod == 0 && (sib & 7) == 5) i += 4;
                }

                // Handle displacement
                if (mod == 0 && rm == 5) i += 4;
                else if (mod == 1) i += 1;
                else if (mod == 2) i += 4;
        }

        // Handle immediate operands
        if (opcode == 0x68 || opcode == 0x69 || opcode == 0x81 || opcode == 0xC7) i += 4;
        else if (
            opcode == 0x6A || opcode == 0x6B || opcode == 0x83 || opcode == 0xCD || opcode == 0xEB
            || opcode == 0xE3 || opcode == 0xE0 || opcode == 0xE1 || opcode == 0xE2)
                i += 1;
        else if (opcode == 0xE8 || opcode == 0xE9 || opcode == 0xEA) i += 4;

        if (i > remaining || i == 0 || i > R_CSTL_X86_MAX_INSTRUCTION_LENGTH)
                return R_CSTL_ERROR_BUFFER_TOO_SMALL;

        memset (pOut, 0, sizeof (*pOut));
        pOut->offset = offset;
        pOut->size = (uint8_t)i;
        pOut->opcodeSize = (uint8_t)(i - opcodeStart);
        pOut->opcode = opcode;
        R_CSTL_CopyBytes (pOut->bytes, p, i);

        pOut->rexPrefix = rexPrefix;
        pOut->legacyPrefixes = legacyPrefixes;
        pOut->hasRex = (rexPrefix != 0);
        pOut->rexW = (rexPrefix >> 3) & 1;
        pOut->rexR = (rexPrefix >> 2) & 1;
        pOut->rexX = (rexPrefix >> 1) & 1;
        pOut->rexB = rexPrefix & 1;

        // Enhanced CALL/JMP detection with more jump variants
        pOut->isCall
            = (opcode == 0xE8 || (opcode == 0xFF && hasModRM && ((p[opcodeStart + 1] & 0x38) == 0x10)));
        pOut->isJump
            = (opcode == 0xE9 || opcode == 0xEB || opcode == 0xEA || opcode == 0xE3 || opcode == 0xE0
               || opcode == 0xE1 || opcode == 0xE2
               || (opcode == 0xFF && hasModRM && ((p[opcodeStart + 1] & 0x38) == 0x20))
               || (originalOpcode == R_CSTL_X86_OPCODE_ESCAPE
                   && (opcode >= R_CSTL_X86_COND_JUMP_MIN && opcode <= R_CSTL_X86_COND_JUMP_MAX)));

        // Extract CALL/JMP target addresses
        if (opcode == 0xE8 || opcode == 0xE9)
        {
                // Relative CALL/JMP with 32-bit displacement
                if (i >= opcodeStart + 5)
                {
                        int32_t relOffset;
                        memcpy (&relOffset, p + opcodeStart + 1, sizeof (relOffset));
                        pOut->targetAddress
                            = (uint64_t)((uintptr_t)pBytecode->pCode + offset + i + relOffset);
                }
        }
        else if (opcode == 0xEB)
        {
                // Short JMP with 8-bit displacement
                if (i >= opcodeStart + 2)
                {
                        int8_t relOffset = (int8_t)p[opcodeStart + 1];
                        pOut->targetAddress
                            = (uint64_t)((uintptr_t)pBytecode->pCode + offset + i + relOffset);
                }
        }
        else if (opcode == 0xEA)
        {
                // Far JMP with segment:offset (not commonly used in 64-bit)
                if (i >= opcodeStart + 7)
                {
                        uint16_t segment;
                        uint32_t offsetAddr;
                        memcpy (&segment, p + opcodeStart + 5, sizeof (segment));
                        memcpy (&offsetAddr, p + opcodeStart + 1, sizeof (offsetAddr));
                        pOut->targetAddress = ((uint64_t)segment << 16) | offsetAddr;
                }
        }
        else if (opcode == 0xE0 || opcode == 0xE1 || opcode == 0xE2)
        {
                // LOOP/LOOPE/LOOPNE with 8-bit displacement
                if (i >= opcodeStart + 2)
                {
                        int8_t relOffset = (int8_t)p[opcodeStart + 1];
                        pOut->targetAddress
                            = (uint64_t)((uintptr_t)pBytecode->pCode + offset + i + relOffset);
                }
        }
        else if (opcode == 0xE3)
        {
                // JCXZ/JECXZ/JRCXZ with 8-bit displacement
                if (i >= opcodeStart + 2)
                {
                        int8_t relOffset = (int8_t)p[opcodeStart + 1];
                        pOut->targetAddress
                            = (uint64_t)((uintptr_t)pBytecode->pCode + offset + i + relOffset);
                }
        }
        else if (opcode == 0xFF && hasModRM)
        {
                // CALL/JMP through ModRM
                uint8_t modrm = p[opcodeStart + 1];
                uint8_t reg = (modrm >> 3) & 7;
                if (reg == 2 || reg == 3 || reg == 4 || reg == 5)
                {
                        // CALL/JMP with absolute or indirect addressing
                        if (i >= opcodeStart + 5 && (modrm & 0xC7) == 0x05)
                        {
                                uint32_t absAddr;
                                memcpy (&absAddr, p + opcodeStart + 2, sizeof (absAddr));
                                pOut->targetAddress = absAddr;
                        }
                }
        }
        // Handle conditional jumps (Jcc) with 32-bit displacement
        // These are 0x0F 0x80-0x8F followed by rel32
        if (originalOpcode == R_CSTL_X86_OPCODE_ESCAPE
            && (opcode >= R_CSTL_X86_COND_JUMP_MIN && opcode <= R_CSTL_X86_COND_JUMP_MAX))
        {
                if (i >= opcodeStart + 6)
                {
                        int32_t relOffset;
                        memcpy (&relOffset, p + opcodeStart + 2, sizeof (relOffset));
                        pOut->targetAddress
                            = (uint64_t)((uintptr_t)pBytecode->pCode + offset + i + relOffset);
                }
        }

        return R_CSTL_OK;
}

#endif

#if defined(R_CSTL_LOG_DEVMODE)

#if defined(R_CSTL_PLATFORM_WINDOWS)

static HANDLE g_symbolHandle = NULL;
static bool   g_symbolInitialized = false;

static int
R_CSTL_InitializeSymbolsWindows (void)
{
        if (g_symbolInitialized) return R_CSTL_OK;

        g_symbolHandle = GetCurrentProcess ();
        if (!SymInitialize (g_symbolHandle, NULL, TRUE))
        {
                DWORD error = GetLastError ();
                (void)error;
                return R_CSTL_ERROR_UNKNOWN;
        }

        DWORD options = SymGetOptions ();
        options |= SYMOPT_LOAD_LINES | SYMOPT_FAIL_CRITICAL_ERRORS;
        SymSetOptions (options);

        g_symbolInitialized = true;
        return R_CSTL_OK;
}

static void
R_CSTL_CleanupSymbolsWindows (void)
{
        if (g_symbolInitialized && g_symbolHandle)
        {
                SymCleanup (g_symbolHandle);
                g_symbolInitialized = false;
        }
}

static int
R_CSTL_ResolveSymbolWindows (uint64_t address, struct R_CSTL_BytecodeSymbol* pOutSymbol)
{
        if (!g_symbolInitialized)
        {
                int result = R_CSTL_InitializeSymbolsWindows ();
                if (result != R_CSTL_OK) return result;
        }

        char         buffer[sizeof (SYMBOL_INFO) + MAX_SYM_NAME * sizeof (TCHAR)];
        PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;
        pSymbol->SizeOfStruct = sizeof (SYMBOL_INFO);
        pSymbol->MaxNameLen = MAX_SYM_NAME;

        if (!SymFromAddr (g_symbolHandle, (DWORD64)address, NULL, pSymbol))
                return R_CSTL_ERROR_SYMBOL_NOT_FOUND;

        pOutSymbol->address = address;
        pOutSymbol->pName = pSymbol->Name;
        pOutSymbol->nameSize = pSymbol->NameLen;
        pOutSymbol->size = pSymbol->Size;

        return R_CSTL_OK;
}

#elif defined(R_CSTL_PLATFORM_LINUX)

static int
R_CSTL_InitializeSymbolsLinux (void)
{
        return R_CSTL_OK;
}

static void
R_CSTL_CleanupSymbolsLinux (void)
{
}

static int
R_CSTL_ResolveSymbolLinux (uint64_t address, struct R_CSTL_BytecodeSymbol* pOutSymbol)
{
        void* buffer[16];
        int   nptrs = backtrace (buffer, 16);
        if (nptrs <= 0) return R_CSTL_ERROR_SYMBOL_NOT_FOUND;

        char** strings = backtrace_symbols (buffer, nptrs);
        if (!strings) return R_CSTL_ERROR_SYMBOL_NOT_FOUND;

        for (int i = 0; i < nptrs; ++i)
        {
                if ((uintptr_t)buffer[i] == (uintptr_t)address)
                {
                        pOutSymbol->address = address;
                        pOutSymbol->pName = strings[i];
                        pOutSymbol->nameSize = strlen (strings[i]);
                        pOutSymbol->size = 0;
                        free (strings);
                        return R_CSTL_OK;
                }
        }

        free (strings);
        return R_CSTL_ERROR_SYMBOL_NOT_FOUND;
}

#endif

R_CSTL_API int
R_CSTL_BytecodeDecoderCreate (
    enum R_CSTL_BytecodeArchitecture architecture,
    struct R_CSTL_BytecodeDecoder*   pOutDecoder)
{
        if (!pOutDecoder) return R_CSTL_ERROR_INVALID_ARGUMENT;

        if (architecture != R_CSTL_BYTECODE_ARCH_X86 && architecture != R_CSTL_BYTECODE_ARCH_X86_64)
                return R_CSTL_ERROR_ARCHITECTURE_NOT_SUPPORTED;

        memset (pOutDecoder, 0, sizeof (*pOutDecoder));
        pOutDecoder->architecture = architecture;

#if defined(R_CSTL_PLATFORM_WINDOWS)
        int result = R_CSTL_InitializeSymbolsWindows ();
        if (result != R_CSTL_OK) return result;
        pOutDecoder->pPlatformHandle = g_symbolHandle;
#elif defined(R_CSTL_PLATFORM_LINUX)
        int result = R_CSTL_InitializeSymbolsLinux ();
        if (result != R_CSTL_OK) return result;
#endif
        pOutDecoder->initialized = true;
        return R_CSTL_OK;
}

R_CSTL_API void
R_CSTL_BytecodeDecoderDestroy (struct R_CSTL_BytecodeDecoder* pDecoder)
{
        if (!pDecoder) return;

#if defined(R_CSTL_PLATFORM_WINDOWS)
        R_CSTL_CleanupSymbolsWindows ();
#elif defined(R_CSTL_PLATFORM_LINUX)
        R_CSTL_CleanupSymbolsLinux ();
#endif

        memset (pDecoder, 0, sizeof (*pDecoder));
}

R_CSTL_API int
R_CSTL_BytecodeResolveSymbol (
    const struct R_CSTL_BytecodeDecoder* pDecoder,
    uint64_t                                address,
    struct R_CSTL_BytecodeSymbol*        pOutSymbol)
{
        if (!pDecoder || !pOutSymbol) return R_CSTL_ERROR_INVALID_ARGUMENT;
        if (!pDecoder->initialized) return R_CSTL_ERROR_INVALID_ARGUMENT;

#if defined(R_CSTL_PLATFORM_WINDOWS)
        return R_CSTL_ResolveSymbolWindows (address, pOutSymbol);
#elif defined(R_CSTL_PLATFORM_LINUX)
        return R_CSTL_ResolveSymbolLinux (address, pOutSymbol);
#else
        return R_CSTL_ERROR_EXECUTABLE_TYPE_NOT_SUPPORTED;
#endif
}

R_CSTL_API int
R_CSTL_BytecodeGetFunctionInfo (
    const struct R_CSTL_BytecodeDecoder* pDecoder,
    uint64_t                                address,
    struct R_CSTL_BytecodeFunctionInfo*  pOutInfo)
{
        if (!pDecoder || !pOutInfo) return R_CSTL_ERROR_INVALID_ARGUMENT;
        if (!pDecoder->initialized) return R_CSTL_ERROR_INVALID_ARGUMENT;

        struct R_CSTL_BytecodeSymbol symbol;
        int                             result = R_CSTL_BytecodeResolveSymbol (pDecoder, address, &symbol);
        if (result != R_CSTL_OK) return result;

        pOutInfo->startAddress = symbol.address;
        pOutInfo->endAddress = symbol.address + symbol.size;
        pOutInfo->pName = symbol.pName;
        pOutInfo->nameSize = symbol.nameSize;

        return R_CSTL_OK;
}

R_CSTL_API int
R_CSTL_BytecodeParseEnhanced (
    const struct R_CSTL_Bytecode*      pBytecode,
    size_t                                offset,
    struct R_CSTL_BytecodeInstruction* pOutInstruction)
{
        if (!pBytecode || !pOutInstruction || offset >= pBytecode->size)
                return R_CSTL_ERROR_INVALID_ARGUMENT;

        if (pBytecode->architecture == R_CSTL_BYTECODE_ARCH_X86
            || pBytecode->architecture == R_CSTL_BYTECODE_ARCH_X86_64)
                return R_CSTL_BytecodeParseX86Enhanced (pBytecode, offset, pOutInstruction);

        return R_CSTL_ERROR_ARCHITECTURE_NOT_SUPPORTED;
}

R_CSTL_API int
R_CSTL_BytecodeGetInstructionTargetSymbol (
    const struct R_CSTL_BytecodeDecoder*     pDecoder,
    const struct R_CSTL_BytecodeInstruction* pInstruction,
    char*                                       pOutBuffer,
    size_t                                      bufferSize)
{
        if (!pDecoder || !pInstruction || !pOutBuffer) return R_CSTL_ERROR_INVALID_ARGUMENT;
        if (!pDecoder->initialized) return R_CSTL_ERROR_INVALID_ARGUMENT;

        if (pInstruction->targetAddress == 0)
        {
                snprintf (
                    pOutBuffer,
                    bufferSize,
                    "0x%016llX",
                    (unsigned long long)pInstruction->targetAddress);
                return R_CSTL_OK;
        }

        struct R_CSTL_BytecodeSymbol symbol;
        int result = R_CSTL_BytecodeResolveSymbol (pDecoder, pInstruction->targetAddress, &symbol);
        if (result == R_CSTL_OK && symbol.pName)
        {
                snprintf (pOutBuffer, bufferSize, "%.*s", (int)symbol.nameSize, symbol.pName);
                return R_CSTL_OK;
        }
        // Fallback to raw address
        snprintf (pOutBuffer, bufferSize, "0x%016llX", (unsigned long long)pInstruction->targetAddress);
        return R_CSTL_OK;
}

R_CSTL_API int
R_CSTL_BytecodeFunctionContainsSymbol (
    const struct R_CSTL_BytecodeDecoder* pDecoder,
    R_CSTL_BytecodeFunction              pFunction,
    size_t                                  functionSize,
    const char*                             pSymbolName,
    int*                                    pOutFound)
{
        if (!pDecoder || !pFunction || !pSymbolName || !pOutFound) return R_CSTL_ERROR_INVALID_ARGUMENT;
        if (!pDecoder->initialized) return R_CSTL_ERROR_INVALID_ARGUMENT;

        *pOutFound = 0;

        struct R_CSTL_Bytecode* pBytecode
            = R_CSTL_NewBytecodeFromFunction (pFunction, functionSize, pDecoder->architecture);
        if (!pBytecode) return R_CSTL_ERROR_INVALID_ARGUMENT;

        size_t offset = 0;
        int    found = 0;

        while (offset < functionSize)
        {
                struct R_CSTL_BytecodeInstruction instruction;
                int result = R_CSTL_BytecodeParseEnhanced (pBytecode, offset, &instruction);
                if (result != R_CSTL_OK)
                {
                        R_CSTL_DeleteBytecode (pBytecode);
                        return result;
                }

                if (instruction.isCall && instruction.targetAddress != 0)
                {
                        char symbolBuffer[256];
                        result = R_CSTL_BytecodeGetInstructionTargetSymbol (
                            pDecoder,
                            &instruction,
                            symbolBuffer,
                            sizeof (symbolBuffer));
                        if (result == R_CSTL_OK)
                        {
                                if (strstr (symbolBuffer, pSymbolName) != NULL)
                                {
                                        found = 1;
                                        break;
                                }
                        }
                }
                offset += instruction.size;
        }

        R_CSTL_DeleteBytecode (pBytecode);
        *pOutFound = found;
        return R_CSTL_OK;
}

#endif

R_CSTL_API int
R_CSTL_BytecodeParse (
    const struct R_CSTL_Bytecode*      pBytecode,
    size_t                                offset,
    struct R_CSTL_BytecodeInstruction* pOutInstruction)
{
        if (!pBytecode || !pOutInstruction || offset >= pBytecode->size)
                return R_CSTL_ERROR_INVALID_ARGUMENT;
        if (pBytecode->architecture == R_CSTL_BYTECODE_ARCH_X86
            || pBytecode->architecture == R_CSTL_BYTECODE_ARCH_X86_64)
                return R_CSTL_BytecodeParseX86 (pBytecode, offset, pOutInstruction);
        size_t width = 4;
        if (pBytecode->size - offset < width) return R_CSTL_ERROR_BUFFER_TOO_SMALL;
        memset (pOutInstruction, 0, sizeof (*pOutInstruction));
        pOutInstruction->offset = offset;
        pOutInstruction->size = (uint8_t)width;
        pOutInstruction->opcodeSize = (uint8_t)width;
        memcpy (pOutInstruction->bytes, pBytecode->pCode + offset, width);
        for (size_t i = 0; i < width; ++i)
                pOutInstruction->opcode |= (uint64_t)pOutInstruction->bytes[i] << (i * 8);
        return R_CSTL_OK;
}

static int
R_CSTL_BytecodeAddToken (
    struct R_CSTL_BytecodeToken*  pTokens,
    size_t                           capacity,
    size_t*                          pCount,
    enum R_CSTL_BytecodeTokenKind kind,
    size_t                           offset,
    uint8_t                          size,
    uint64_t                         value)
{
        if (*pCount >= capacity) return R_CSTL_ERROR_BUFFER_TOO_SMALL;
        pTokens[*pCount] = (struct R_CSTL_BytecodeToken){kind, offset, size, value};
        ++*pCount;
        return R_CSTL_OK;
}

R_CSTL_API int
R_CSTL_BytecodeTokenize (
    const struct R_CSTL_Bytecode* pBytecode,
    size_t                           offset,
    struct R_CSTL_BytecodeToken*  pTokens,
    size_t                           tokenCapacity,
    size_t*                          pOutTokenCount)
{
        if (!pBytecode || !pTokens || !pOutTokenCount) return R_CSTL_ERROR_INVALID_ARGUMENT;
        *pOutTokenCount = 0;
        struct R_CSTL_BytecodeInstruction instruction;
        int result = R_CSTL_BytecodeParse (pBytecode, offset, &instruction);
        if (result != R_CSTL_OK) return result;
        result = R_CSTL_BytecodeAddToken (
            pTokens,
            tokenCapacity,
            pOutTokenCount,
            R_CSTL_BYTECODE_TOKEN_OPCODE,
            offset,
            instruction.opcodeSize,
            instruction.opcode);
        if (result != R_CSTL_OK) return result;
        if (instruction.size > instruction.opcodeSize)
                result = R_CSTL_BytecodeAddToken (
                    pTokens,
                    tokenCapacity,
                    pOutTokenCount,
                    R_CSTL_BYTECODE_TOKEN_OPERAND,
                    offset + instruction.opcodeSize,
                    (uint8_t)(instruction.size - instruction.opcodeSize),
                    0);
        if (result != R_CSTL_OK) return result;
        if (pBytecode->architecture == R_CSTL_BYTECODE_ARCH_X86_64 && instruction.size >= 6
            && instruction.bytes[instruction.opcodeSize] == 0x05)
                result = R_CSTL_BytecodeAddToken (
                    pTokens,
                    tokenCapacity,
                    pOutTokenCount,
                    R_CSTL_BYTECODE_TOKEN_RIP_ADDRESSING,
                    offset + instruction.opcodeSize,
                    5,
                    0);
        return result;
}

R_CSTL_API const uint8_t*
R_CSTL_BytecodeData (const struct R_CSTL_Bytecode* pBytecode)
{
        return pBytecode ? pBytecode->pCode : NULL;
}

R_CSTL_API size_t
R_CSTL_BytecodeLength (const struct R_CSTL_Bytecode* pBytecode)
{
        return pBytecode ? pBytecode->size : 0;
}

R_CSTL_API enum R_CSTL_BytecodeArchitecture
R_CSTL_BytecodeGetArchitecture (const struct R_CSTL_Bytecode* pBytecode)
{
        return pBytecode ? pBytecode->architecture : R_CSTL_BYTECODE_ARCH_X86;
}
