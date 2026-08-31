#include "rlgame.base/cstl/cstl_bytecode.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"
#include "rlgame.base/cstl/cstl_thread.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>

#define r_cstl_x86_OPCODE_ESCAPE          0x0F
#define r_cstl_x86_OPCODE_THREEBYTE_38    0x38
#define r_cstl_x86_OPCODE_THREEBYTE_3A    0x3A
#define r_cstl_x86_REX_MIN                0x40
#define r_cstl_x86_REX_MAX                0x4F
#define r_cstl_x86_PREFIX_LOCK            0xF0
#define r_cstl_x86_PREFIX_REPNE           0xF2
#define r_cstl_x86_PREFIX_REPE            0xF3
#define r_cstl_x86_PREFIX_DATA_SIZE       0x66
#define r_cstl_x86_PREFIX_ADDR_SIZE       0x67
#define r_cstl_x86_COND_JUMP_MIN          0x80
#define r_cstl_x86_COND_JUMP_MAX          0x8F
#define r_cstl_x86_MAX_INSTRUCTION_LENGTH 15

#if defined(R_SIMD_SSE) || defined(R_SIMD_AVX2)
#include <immintrin.h>
#endif

static inline size_t
r_cstl_scan_prefixes (const uint8_t* p, size_t remaining, size_t maxScan)
{
    size_t i = 0;
#if defined(R_SIMD_AVX2)
    while (i + 32 <= remaining && i < maxScan)
    {
        __m256i chunk = _mm256_loadu_si256 ((__m256i const*)(p + i));

        // Check for prefix bytes using constants
        __m256i cmpLock = _mm256_cmpeq_epi8 (chunk, _mm256_set1_epi8 (r_cstl_x86_PREFIX_LOCK));
        __m256i cmpRepne = _mm256_cmpeq_epi8 (chunk, _mm256_set1_epi8 (r_cstl_x86_PREFIX_REPNE));
        __m256i cmpRepe = _mm256_cmpeq_epi8 (chunk, _mm256_set1_epi8 (r_cstl_x86_PREFIX_REPE));
        __m256i cmpDataSize = _mm256_cmpeq_epi8 (chunk, _mm256_set1_epi8 (r_cstl_x86_PREFIX_DATA_SIZE));
        __m256i cmpAddrSize = _mm256_cmpeq_epi8 (chunk, _mm256_set1_epi8 (r_cstl_x86_PREFIX_ADDR_SIZE));
        __m256i cmpRexMin = _mm256_cmpeq_epi8 (chunk, _mm256_set1_epi8 (r_cstl_x86_REX_MIN));
        __m256i cmpRexMax = _mm256_cmpeq_epi8 (chunk, _mm256_set1_epi8 (r_cstl_x86_REX_MAX));

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
    while (i + 16 <= remaining && i < maxScan)
    {
        __m128i chunk = _mm_loadu_si128 ((__m128i const*)(p + i));

        // Check for prefix bytes using constants
        __m128i cmpLock = _mm_cmpeq_epi8 (chunk, _mm_set1_epi8 (r_cstl_x86_PREFIX_LOCK));
        __m128i cmpRepne = _mm_cmpeq_epi8 (chunk, _mm_set1_epi8 (r_cstl_x86_PREFIX_REPNE));
        __m128i cmpRepe = _mm_cmpeq_epi8 (chunk, _mm_set1_epi8 (r_cstl_x86_PREFIX_REPE));
        __m128i cmpDataSize = _mm_cmpeq_epi8 (chunk, _mm_set1_epi8 (r_cstl_x86_PREFIX_DATA_SIZE));
        __m128i cmpAddrSize = _mm_cmpeq_epi8 (chunk, _mm_set1_epi8 (r_cstl_x86_PREFIX_ADDR_SIZE));
        __m128i cmpRexMin = _mm_cmpeq_epi8 (chunk, _mm_set1_epi8 (r_cstl_x86_REX_MIN));
        __m128i cmpRexMax = _mm_cmpeq_epi8 (chunk, _mm_set1_epi8 (r_cstl_x86_REX_MAX));

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
           && (p[i] == r_cstl_x86_PREFIX_LOCK || p[i] == r_cstl_x86_PREFIX_REPNE
               || p[i] == r_cstl_x86_PREFIX_REPE || p[i] == r_cstl_x86_PREFIX_DATA_SIZE
               || p[i] == r_cstl_x86_PREFIX_ADDR_SIZE
               || (p[i] >= r_cstl_x86_REX_MIN && p[i] <= r_cstl_x86_REX_MAX)))
        ++i;

    return i;
}

static inline void
r_cstl_copy_bytes (uint8_t* dst, const uint8_t* src, size_t size)
{
    if (size == 0) return;
    size_t i = 0;
#if defined(R_SIMD_AVX2)
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

#if defined(R_LOG)
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

struct r_cstl_bytecode
{
        const uint8_t*                   pCode;
        uint8_t*                         pOwnedCode;
        size_t                           size;
        enum r_cstl_bytecode_architecture architecture;
        struct r_cstl_mutex*             pMutex;
        bool                             mutexInitialized;
};

static int
r_cstl_bytecode_architecture_is_valid (enum r_cstl_bytecode_architecture architecture)
{
    return architecture >= R_CSTL_BYTECODE_ARCH_X86 && architecture <= R_CSTL_BYTECODE_ARCH_RISC;
}

static struct r_cstl_bytecode*
r_cstl_bytecode_create (const uint8_t* pCode, size_t sizeBytes, enum r_cstl_bytecode_architecture architecture)
{
#if defined(R_LOG)
    assert (r_cstl_bytecode_architecture_is_valid (architecture) || "Invalid architecture");
#endif

    if ((!pCode && sizeBytes != 0) || !r_cstl_bytecode_architecture_is_valid (architecture)) return NULL;

    struct r_cstl_bytecode* pBytecode = (struct r_cstl_bytecode*)r_cstl_heap_alloc (sizeof (*pBytecode));
    if (!pBytecode) return NULL;

#if defined(R_LOG)
    assert (pBytecode);
#endif

    pBytecode->pCode = pCode;
    pBytecode->pOwnedCode = NULL;
    pBytecode->size = sizeBytes;
    pBytecode->architecture = architecture;
    pBytecode->pMutex = NULL;
    pBytecode->mutexInitialized = false;

    pBytecode->pMutex = r_cstl_new_mutex ();
    if (pBytecode->pMutex) pBytecode->mutexInitialized = true;

#if defined(R_LOG)
    assert (pBytecode->pCode || sizeBytes == 0);
    assert (pBytecode->size == sizeBytes);
#endif

    return pBytecode;
}

R_CSTL_API struct r_cstl_bytecode*
r_cstl_new_bytecode_view (const void* pCode, size_t sizeBytes, enum r_cstl_bytecode_architecture architecture)
{
    return r_cstl_bytecode_create ((const uint8_t*)pCode, sizeBytes, architecture);
}

R_CSTL_API struct r_cstl_bytecode*
r_cstl_new_bytecode_with_data (
    const uint8_t*                   pCode,
    size_t                           sizeBytes,
    enum r_cstl_bytecode_architecture architecture)
{
    struct r_cstl_bytecode* pBytecode = r_cstl_bytecode_create (pCode, sizeBytes, architecture);
    if (!pBytecode || sizeBytes == 0) return pBytecode;
    pBytecode->pOwnedCode = (uint8_t*)r_cstl_heap_alloc (sizeBytes);
    if (!pBytecode->pOwnedCode)
    {
        r_cstl_heap_free (pBytecode);
        return NULL;
    }
    memcpy (pBytecode->pOwnedCode, pCode, sizeBytes);
    pBytecode->pCode = pBytecode->pOwnedCode;
    return pBytecode;
}

R_CSTL_API struct r_cstl_bytecode*
r_cstl_new_bytecode_from_function (
    r_cstl_bytecode_function          pFunction,
    size_t                           sizeBytes,
    enum r_cstl_bytecode_architecture architecture)
{
    if (!pFunction) return NULL;
    return r_cstl_new_bytecode_view ((const void*)(uintptr_t)pFunction, sizeBytes, architecture);
}

R_CSTL_API void
r_cstl_delete_bytecode (struct r_cstl_bytecode* pBytecode)
{
    if (!pBytecode) return;
    if (pBytecode->mutexInitialized && pBytecode->pMutex) r_cstl_mutex_destroy (pBytecode->pMutex);
    if (pBytecode->pOwnedCode) r_cstl_heap_free (pBytecode->pOwnedCode);
    r_cstl_heap_free (pBytecode);
}

R_CSTL_API int
r_cstl_bytecode_read (
    const struct r_cstl_bytecode* pBytecode,
    size_t                        offset,
    uint8_t*                      pOutBytes,
    size_t                        sizeBytes)
{
#if defined(R_LOG)
    assert (pBytecode);
    assert (pBytecode->pCode || pBytecode->size == 0);
    assert (offset <= pBytecode->size);
    assert (sizeBytes <= pBytecode->size - offset);
#endif

    if (!pBytecode || (!pOutBytes && sizeBytes != 0)) return R_CSTL_ERROR_INVALID_ARGUMENT;
    if (offset > pBytecode->size || sizeBytes > pBytecode->size - offset)
        return R_CSTL_ERROR_BUFFER_TOO_SMALL;

    r_cstl_mutex_lock (pBytecode->pMutex);
    if (sizeBytes) memcpy (pOutBytes, pBytecode->pCode + offset, sizeBytes);
    r_cstl_mutex_unlock (pBytecode->pMutex);

    return R_CSTL_OK;
}

static int
r_cstl_bytecode_parse_x86 (
    const struct r_cstl_bytecode*      pBytecode,
    size_t                             offset,
    struct r_cstl_bytecode_instruction* pOut)
{
    const uint8_t* p = pBytecode->pCode + offset;
    size_t         remaining = pBytecode->size - offset;

    size_t i = r_cstl_scan_prefixes (p, remaining, r_cstl_x86_MAX_INSTRUCTION_LENGTH);
    if (i >= remaining) return R_CSTL_ERROR_BUFFER_TOO_SMALL;

    size_t  opcodeStart = i;
    uint8_t opcode = p[i++];

    // Handle multi-byte opcodes
    if (opcode == r_cstl_x86_OPCODE_ESCAPE)
    {
        if (i >= remaining) return R_CSTL_ERROR_BUFFER_TOO_SMALL;
        opcode = p[i++];

        if (opcode == r_cstl_x86_OPCODE_THREEBYTE_38 || opcode == r_cstl_x86_OPCODE_THREEBYTE_3A)
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

    if (i > remaining || i == 0 || i > r_cstl_x86_MAX_INSTRUCTION_LENGTH)
        return R_CSTL_ERROR_BUFFER_TOO_SMALL;

    memset (pOut, 0, sizeof (*pOut));
    pOut->offset = offset;
    pOut->size = (uint8_t)i;
    pOut->opcodeSize = (uint8_t)(i - opcodeStart);
    pOut->opcode = opcode;
    r_cstl_copy_bytes (pOut->bytes, p, i);

    return R_CSTL_OK;
}

#if defined(R_LOG)

static int
r_cstl_bytecode_parse_x86_enhanced (
    const struct r_cstl_bytecode*      pBytecode,
    size_t                             offset,
    struct r_cstl_bytecode_instruction* pOut)
{
    const uint8_t* p = pBytecode->pCode + offset;
    size_t         remaining = pBytecode->size - offset;

    uint8_t rexPrefix = 0;
    uint8_t legacyPrefixes = 0;
    size_t  i = 0;

    while (i < remaining && i < r_cstl_x86_MAX_INSTRUCTION_LENGTH)
    {
        if (p[i] == r_cstl_x86_PREFIX_LOCK || p[i] == r_cstl_x86_PREFIX_REPNE
            || p[i] == r_cstl_x86_PREFIX_REPE || p[i] == r_cstl_x86_PREFIX_DATA_SIZE
            || p[i] == r_cstl_x86_PREFIX_ADDR_SIZE)
        {
            legacyPrefixes |= (1 << (p[i] & 0x07));
            ++i;
        }
        else if (p[i] >= r_cstl_x86_REX_MIN && p[i] <= r_cstl_x86_REX_MAX)
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
    if (opcode == r_cstl_x86_OPCODE_ESCAPE)
    {
        if (i >= remaining) return R_CSTL_ERROR_BUFFER_TOO_SMALL;
        opcode = p[i++];

        // Handle 3-byte opcodes
        if (opcode == r_cstl_x86_OPCODE_THREEBYTE_38 || opcode == r_cstl_x86_OPCODE_THREEBYTE_3A)
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

    if (i > remaining || i == 0 || i > r_cstl_x86_MAX_INSTRUCTION_LENGTH)
        return R_CSTL_ERROR_BUFFER_TOO_SMALL;

    memset (pOut, 0, sizeof (*pOut));
    pOut->offset = offset;
    pOut->size = (uint8_t)i;
    pOut->opcodeSize = (uint8_t)(i - opcodeStart);
    pOut->opcode = opcode;
    r_cstl_copy_bytes (pOut->bytes, p, i);

    pOut->rexPrefix = rexPrefix;
    pOut->legacyPrefixes = legacyPrefixes;
    pOut->hasRex = (rexPrefix != 0);
    pOut->rexW = (rexPrefix >> 3) & 1;
    pOut->rexR = (rexPrefix >> 2) & 1;
    pOut->rexX = (rexPrefix >> 1) & 1;
    pOut->rexB = rexPrefix & 1;

    // Enhanced CALL/JMP detection with more jump variants
    pOut->isCall = (opcode == 0xE8 || (opcode == 0xFF && hasModRM && ((p[opcodeStart + 1] & 0x38) == 0x10)));
    pOut->isJump
        = (opcode == 0xE9 || opcode == 0xEB || opcode == 0xEA || opcode == 0xE3 || opcode == 0xE0
           || opcode == 0xE1 || opcode == 0xE2
           || (opcode == 0xFF && hasModRM && ((p[opcodeStart + 1] & 0x38) == 0x20))
           || (originalOpcode == r_cstl_x86_OPCODE_ESCAPE
               && (opcode >= r_cstl_x86_COND_JUMP_MIN && opcode <= r_cstl_x86_COND_JUMP_MAX)));

    // Extract CALL/JMP target addresses
    if (opcode == 0xE8 || opcode == 0xE9)
    {
        // Relative CALL/JMP with 32-bit displacement
        if (i >= opcodeStart + 5)
        {
            int32_t relOffset;
            memcpy (&relOffset, p + opcodeStart + 1, sizeof (relOffset));
            pOut->targetAddress = (uint64_t)((uintptr_t)pBytecode->pCode + offset + i + relOffset);
        }
    }
    else if (opcode == 0xEB)
    {
        // Short JMP with 8-bit displacement
        if (i >= opcodeStart + 2)
        {
            int8_t relOffset = (int8_t)p[opcodeStart + 1];
            pOut->targetAddress = (uint64_t)((uintptr_t)pBytecode->pCode + offset + i + relOffset);
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
            pOut->targetAddress = (uint64_t)((uintptr_t)pBytecode->pCode + offset + i + relOffset);
        }
    }
    else if (opcode == 0xE3)
    {
        // JCXZ/JECXZ/JRCXZ with 8-bit displacement
        if (i >= opcodeStart + 2)
        {
            int8_t relOffset = (int8_t)p[opcodeStart + 1];
            pOut->targetAddress = (uint64_t)((uintptr_t)pBytecode->pCode + offset + i + relOffset);
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
    if (originalOpcode == r_cstl_x86_OPCODE_ESCAPE
        && (opcode >= r_cstl_x86_COND_JUMP_MIN && opcode <= r_cstl_x86_COND_JUMP_MAX))
    {
        if (i >= opcodeStart + 6)
        {
            int32_t relOffset;
            memcpy (&relOffset, p + opcodeStart + 2, sizeof (relOffset));
            pOut->targetAddress = (uint64_t)((uintptr_t)pBytecode->pCode + offset + i + relOffset);
        }
    }

    return R_CSTL_OK;
}

#endif

#if defined(R_LOG)

#if defined(R_CSTL_PLATFORM_WINDOWS)

static HANDLE g_symbolHandle = NULL;
static bool   g_symbolInitialized = false;

static int
r_cstl_initialize_symbols_windows (void)
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
r_cstl_cleanup_symbols_windows (void)
{
    if (g_symbolInitialized && g_symbolHandle)
    {
        SymCleanup (g_symbolHandle);
        g_symbolInitialized = false;
    }
}

static int
r_cstl_resolve_symbol_windows (uint64_t address, struct r_cstl_bytecode_symbol* pOutSymbol)
{
    if (!g_symbolInitialized)
    {
        int result = r_cstl_initialize_symbols_windows ();
        if (result != R_CSTL_OK) return result;
    }

    char         buffer[sizeof (SYMBOL_INFO) + MAX_SYM_NAME * sizeof (TCHAR)];
    PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;
    pSymbol->SizeOfStruct = sizeof (SYMBOL_INFO);
    pSymbol->MaxNameLen = MAX_SYM_NAME;

    if (!SymFromAddr (g_symbolHandle, (DWORD64)address, NULL, pSymbol)) return R_CSTL_ERROR_SYMBOL_NOT_FOUND;

    pOutSymbol->address = address;
    pOutSymbol->pName = pSymbol->Name;
    pOutSymbol->nameSize = pSymbol->NameLen;
    pOutSymbol->size = pSymbol->Size;

    return R_CSTL_OK;
}

#elif defined(R_CSTL_PLATFORM_LINUX)

static int
r_cstl_initialize_symbols_linux (void)
{
    return R_CSTL_OK;
}

static void
r_cstl_cleanup_symbols_linux (void)
{
}

static int
r_cstl_resolve_symbol_linux (uint64_t address, struct r_cstl_bytecode_symbol* pOutSymbol)
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
r_cstl_bytecode_decoder_create (
    enum r_cstl_bytecode_architecture architecture,
    struct r_cstl_bytecode_decoder*   pOutDecoder)
{
    if (!pOutDecoder) return R_CSTL_ERROR_INVALID_ARGUMENT;

    if (architecture != R_CSTL_BYTECODE_ARCH_X86 && architecture != R_CSTL_BYTECODE_ARCH_X86_64)
        return R_CSTL_ERROR_ARCHITECTURE_NOT_SUPPORTED;

    memset (pOutDecoder, 0, sizeof (*pOutDecoder));
    pOutDecoder->architecture = architecture;

#if defined(R_CSTL_PLATFORM_WINDOWS)
    int result = r_cstl_initialize_symbols_windows ();
    if (result != R_CSTL_OK) return result;
    pOutDecoder->pPlatformHandle = g_symbolHandle;
#elif defined(R_CSTL_PLATFORM_LINUX)
    int result = r_cstl_initialize_symbols_linux ();
    if (result != R_CSTL_OK) return result;
#endif
    pOutDecoder->initialized = true;
    return R_CSTL_OK;
}

R_CSTL_API void
r_cstl_delete_bytecode_decoder (struct r_cstl_bytecode_decoder* pDecoder)
{
    if (!pDecoder) return;

#if defined(R_CSTL_PLATFORM_WINDOWS)
    r_cstl_cleanup_symbols_windows ();
#elif defined(R_CSTL_PLATFORM_LINUX)
    r_cstl_cleanup_symbols_linux ();
#endif

    memset (pDecoder, 0, sizeof (*pDecoder));
}

R_CSTL_API int
r_cstl_bytecode_resolve_symbol (
    const struct r_cstl_bytecode_decoder* pDecoder,
    uint64_t                             address,
    struct r_cstl_bytecode_symbol*        pOutSymbol)
{
    if (!pDecoder || !pOutSymbol) return R_CSTL_ERROR_INVALID_ARGUMENT;
    if (!pDecoder->initialized) return R_CSTL_ERROR_INVALID_ARGUMENT;

#if defined(R_CSTL_PLATFORM_WINDOWS)
    return r_cstl_resolve_symbol_windows (address, pOutSymbol);
#elif defined(R_CSTL_PLATFORM_LINUX)
    return r_cstl_resolve_symbol_linux (address, pOutSymbol);
#else
    return R_CSTL_ERROR_EXECUTABLE_TYPE_NOT_SUPPORTED;
#endif
}

R_CSTL_API int
r_cstl_bytecode_get_function_info (
    const struct r_cstl_bytecode_decoder* pDecoder,
    uint64_t                             address,
    struct r_cstl_bytecode_function_info*  pOutInfo)
{
    if (!pDecoder || !pOutInfo) return R_CSTL_ERROR_INVALID_ARGUMENT;
    if (!pDecoder->initialized) return R_CSTL_ERROR_INVALID_ARGUMENT;

    struct r_cstl_bytecode_symbol symbol;
    int                          result = r_cstl_bytecode_resolve_symbol (pDecoder, address, &symbol);
    if (result != R_CSTL_OK) return result;

    pOutInfo->startAddress = symbol.address;
    pOutInfo->endAddress = symbol.address + symbol.size;
    pOutInfo->pName = symbol.pName;
    pOutInfo->nameSize = symbol.nameSize;

    return R_CSTL_OK;
}

R_CSTL_API int
r_cstl_bytecode_parse_enhanced (
    const struct r_cstl_bytecode*      pBytecode,
    size_t                             offset,
    struct r_cstl_bytecode_instruction* pOutInstruction)
{
    if (!pBytecode || !pOutInstruction || offset >= pBytecode->size) return R_CSTL_ERROR_INVALID_ARGUMENT;

    if (pBytecode->architecture == R_CSTL_BYTECODE_ARCH_X86
        || pBytecode->architecture == R_CSTL_BYTECODE_ARCH_X86_64)
        return r_cstl_bytecode_parse_x86_enhanced (pBytecode, offset, pOutInstruction);

    return R_CSTL_ERROR_ARCHITECTURE_NOT_SUPPORTED;
}

R_CSTL_API int
r_cstl_bytecode_get_instruction_target_symbol (
    const struct r_cstl_bytecode_decoder*     pDecoder,
    const struct r_cstl_bytecode_instruction* pInstruction,
    char*                                    pOutBuffer,
    size_t                                   bufferSize)
{
    if (!pDecoder || !pInstruction || !pOutBuffer) return R_CSTL_ERROR_INVALID_ARGUMENT;
    if (!pDecoder->initialized) return R_CSTL_ERROR_INVALID_ARGUMENT;

    if (pInstruction->targetAddress == 0)
    {
        snprintf (pOutBuffer, bufferSize, "0x%016llX", (unsigned long long)pInstruction->targetAddress);
        return R_CSTL_OK;
    }

    struct r_cstl_bytecode_symbol symbol;
    int result = r_cstl_bytecode_resolve_symbol (pDecoder, pInstruction->targetAddress, &symbol);
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
r_cstl_bytecode_function_contains_symbol (
    const struct r_cstl_bytecode_decoder* pDecoder,
    r_cstl_bytecode_function              pFunction,
    size_t                               functionSize,
    const char*                          pSymbolName,
    int*                                 pOutFound)
{
    if (!pDecoder || !pFunction || !pSymbolName || !pOutFound) return R_CSTL_ERROR_INVALID_ARGUMENT;
    if (!pDecoder->initialized) return R_CSTL_ERROR_INVALID_ARGUMENT;

    *pOutFound = 0;

    struct r_cstl_bytecode* pBytecode
        = r_cstl_new_bytecode_from_function (pFunction, functionSize, pDecoder->architecture);
    if (!pBytecode) return R_CSTL_ERROR_INVALID_ARGUMENT;

    size_t offset = 0;
    int    found = 0;

    while (offset < functionSize)
    {
        struct r_cstl_bytecode_instruction instruction;
        int result = r_cstl_bytecode_parse_enhanced (pBytecode, offset, &instruction);
        if (result != R_CSTL_OK)
        {
            r_cstl_delete_bytecode (pBytecode);
            return result;
        }

        if (instruction.isCall && instruction.targetAddress != 0)
        {
            char symbolBuffer[256];
            result = r_cstl_bytecode_get_instruction_target_symbol (
                pDecoder,
                &instruction,
                symbolBuffer,
                sizeof (symbolBuffer));
            if (result == R_CSTL_OK)
            {
                if (strstr (symbolBuffer, pSymbolName))
                {
                    found = 1;
                    break;
                }
            }
        }
        offset += instruction.size;
    }

    r_cstl_delete_bytecode (pBytecode);
    *pOutFound = found;
    return R_CSTL_OK;
}

#endif

R_CSTL_API int
r_cstl_bytecode_parse (
    const struct r_cstl_bytecode*      pBytecode,
    size_t                             offset,
    struct r_cstl_bytecode_instruction* pOutInstruction)
{
    if (!pBytecode || !pOutInstruction || offset >= pBytecode->size) return R_CSTL_ERROR_INVALID_ARGUMENT;
    if (pBytecode->architecture == R_CSTL_BYTECODE_ARCH_X86
        || pBytecode->architecture == R_CSTL_BYTECODE_ARCH_X86_64)
        return r_cstl_bytecode_parse_x86 (pBytecode, offset, pOutInstruction);
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
r_cstl_bytecode_add_token (
    struct r_cstl_bytecode_token*  pTokens,
    size_t                        capacity,
    size_t*                       pCount,
    enum r_cstl_bytecode_token_kind kind,
    size_t                        offset,
    uint8_t                       size,
    uint64_t                      value)
{
    if (*pCount >= capacity) return R_CSTL_ERROR_BUFFER_TOO_SMALL;
    pTokens[*pCount] = (struct r_cstl_bytecode_token){kind, offset, size, value};
    ++*pCount;
    return R_CSTL_OK;
}

R_CSTL_API int
r_cstl_bytecode_tokenize (
    const struct r_cstl_bytecode* pBytecode,
    size_t                        offset,
    struct r_cstl_bytecode_token*  pTokens,
    size_t                        tokenCapacity,
    size_t*                       pOutTokenCount)
{
    if (!pBytecode || !pTokens || !pOutTokenCount) return R_CSTL_ERROR_INVALID_ARGUMENT;
    *pOutTokenCount = 0;
    struct r_cstl_bytecode_instruction instruction;
    int                               result = r_cstl_bytecode_parse (pBytecode, offset, &instruction);
    if (result != R_CSTL_OK) return result;
    result = r_cstl_bytecode_add_token (
        pTokens,
        tokenCapacity,
        pOutTokenCount,
        R_CSTL_BYTECODE_TOKEN_OPCODE,
        offset,
        instruction.opcodeSize,
        instruction.opcode);
    if (result != R_CSTL_OK) return result;
    if (instruction.size > instruction.opcodeSize)
        result = r_cstl_bytecode_add_token (
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
        result = r_cstl_bytecode_add_token (
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
r_cstl_bytecode_data (const struct r_cstl_bytecode* pBytecode)
{
#if defined(R_CSTL_DEBUG)
    assert (pBytecode);
#endif
    return pBytecode->pCode;
}

R_CSTL_API size_t
r_cstl_bytecode_length (const struct r_cstl_bytecode* pBytecode)
{
#if defined(R_CSTL_DEBUG)
    assert (pBytecode);
#endif
    return pBytecode->size;
}

R_CSTL_API enum r_cstl_bytecode_architecture
r_cstl_bytecode_get_architecture (const struct r_cstl_bytecode* pBytecode)
{
#if defined(R_CSTL_DEBUG)
    assert (pBytecode);
#endif
    return pBytecode->architecture;
}
