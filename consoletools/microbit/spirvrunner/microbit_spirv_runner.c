#include "microbit/spirvrunner/microbit_spirv_runner.h"
#include "microbit/microbit_platform.h"

#include "rlgame.base/cstl/cstl_heap_allocator.h"
#include "rlgame.base/cstl/cstl_log.h"

#include <math.h>
#include <limits.h>
#include <string.h>

#define MICROBIT_SPIRV_RUNNER_OPCODE_MASK      0xFFFFu
#define MICROBIT_SPIRV_RUNNER_WORD_COUNT_SHIFT 16u
#define MICROBIT_SPIRV_RUNNER_OP_NOP           0u
#define MICROBIT_SPIRV_RUNNER_OP_FUNCTION      54u
#define MICROBIT_SPIRV_RUNNER_OP_FUNCTION_CALL 57u
#define MICROBIT_SPIRV_RUNNER_OP_FUNCTION_END  56u
#define MICROBIT_SPIRV_RUNNER_OP_LABEL         248u
#define MICROBIT_SPIRV_RUNNER_OP_RETURN        253u
#define MICROBIT_SPIRV_RUNNER_OP_RETURN_VALUE  254u
#define MICROBIT_SPIRV_RUNNER_OP_TYPE_VOID     19u
#define MICROBIT_SPIRV_RUNNER_OP_TYPE_BOOL     20u
#define MICROBIT_SPIRV_RUNNER_OP_TYPE_INT      21u
#define MICROBIT_SPIRV_RUNNER_OP_TYPE_FLOAT    22u
#define MICROBIT_SPIRV_RUNNER_OP_TYPE_VECTOR   23u
#define MICROBIT_SPIRV_RUNNER_OP_TYPE_MATRIX   24u
#define MICROBIT_SPIRV_RUNNER_OP_TYPE_ARRAY    28u
#define MICROBIT_SPIRV_RUNNER_OP_TYPE_RUNTIME_ARRAY 29u
#define MICROBIT_SPIRV_RUNNER_OP_TYPE_STRUCT   30u
#define MICROBIT_SPIRV_RUNNER_OP_TYPE_POINTER   32u
#define MICROBIT_SPIRV_RUNNER_OP_CONSTANT      43u
#define MICROBIT_SPIRV_RUNNER_OP_VARIABLE      59u
#define MICROBIT_SPIRV_RUNNER_OP_LOAD          61u
#define MICROBIT_SPIRV_RUNNER_OP_STORE         62u
#define MICROBIT_SPIRV_RUNNER_OP_ACCESS_CHAIN  65u
#define MICROBIT_SPIRV_RUNNER_OP_COMPOSITE_CONSTRUCT 80u
#define MICROBIT_SPIRV_RUNNER_OP_COMPOSITE_EXTRACT 81u
#define MICROBIT_SPIRV_RUNNER_OP_COMPOSITE_INSERT 82u
#define MICROBIT_SPIRV_RUNNER_OP_IADD          128u
#define MICROBIT_SPIRV_RUNNER_OP_FADD          129u
#define MICROBIT_SPIRV_RUNNER_OP_ISUB          130u
#define MICROBIT_SPIRV_RUNNER_OP_FSUB          131u
#define MICROBIT_SPIRV_RUNNER_OP_IMUL          132u
#define MICROBIT_SPIRV_RUNNER_OP_FMUL          133u
#define MICROBIT_SPIRV_RUNNER_OP_UDIV          134u
#define MICROBIT_SPIRV_RUNNER_OP_SDIV          135u
#define MICROBIT_SPIRV_RUNNER_OP_FDIV          136u
#define MICROBIT_SPIRV_RUNNER_OP_UMOD          137u
#define MICROBIT_SPIRV_RUNNER_OP_SREM          138u
#define MICROBIT_SPIRV_RUNNER_OP_SMOD          139u
#define MICROBIT_SPIRV_RUNNER_OP_FREM          140u
#define MICROBIT_SPIRV_RUNNER_OP_FMOD          141u
#define MICROBIT_SPIRV_RUNNER_OP_CONVERT_F_TO_U 109u
#define MICROBIT_SPIRV_RUNNER_OP_CONVERT_F_TO_S 110u
#define MICROBIT_SPIRV_RUNNER_OP_CONVERT_S_TO_F 111u
#define MICROBIT_SPIRV_RUNNER_OP_CONVERT_U_TO_F 112u
#define MICROBIT_SPIRV_RUNNER_OP_U_CONVERT 113u
#define MICROBIT_SPIRV_RUNNER_OP_S_CONVERT 114u
#define MICROBIT_SPIRV_RUNNER_OP_F_CONVERT 115u
#define MICROBIT_SPIRV_RUNNER_OP_BITCAST 119u
#define MICROBIT_SPIRV_RUNNER_OP_I_EQUAL 170u
#define MICROBIT_SPIRV_RUNNER_OP_I_NOT_EQUAL 171u
#define MICROBIT_SPIRV_RUNNER_OP_U_LESS_THAN 172u
#define MICROBIT_SPIRV_RUNNER_OP_S_LESS_THAN 173u
#define MICROBIT_SPIRV_RUNNER_OP_U_GREATER_THAN 174u
#define MICROBIT_SPIRV_RUNNER_OP_S_GREATER_THAN 175u
#define MICROBIT_SPIRV_RUNNER_OP_U_LESS_THAN_EQUAL 176u
#define MICROBIT_SPIRV_RUNNER_OP_S_LESS_THAN_EQUAL 177u
#define MICROBIT_SPIRV_RUNNER_OP_U_GREATER_THAN_EQUAL 178u
#define MICROBIT_SPIRV_RUNNER_OP_S_GREATER_THAN_EQUAL 179u
#define MICROBIT_SPIRV_RUNNER_OP_FORD_EQUAL 180u
#define MICROBIT_SPIRV_RUNNER_OP_FORD_NOT_EQUAL 181u
#define MICROBIT_SPIRV_RUNNER_OP_FORD_LESS_THAN 182u
#define MICROBIT_SPIRV_RUNNER_OP_FORD_GREATER_THAN 183u
#define MICROBIT_SPIRV_RUNNER_OP_FORD_LESS_THAN_EQUAL 184u
#define MICROBIT_SPIRV_RUNNER_OP_FORD_GREATER_THAN_EQUAL 185u
#define MICROBIT_SPIRV_RUNNER_OP_FUNORD_EQUAL 186u
#define MICROBIT_SPIRV_RUNNER_OP_FUNORD_NOT_EQUAL 187u
#define MICROBIT_SPIRV_RUNNER_OP_FUNORD_LESS_THAN 188u
#define MICROBIT_SPIRV_RUNNER_OP_FUNORD_GREATER_THAN 189u
#define MICROBIT_SPIRV_RUNNER_OP_FUNORD_LESS_THAN_EQUAL 190u
#define MICROBIT_SPIRV_RUNNER_OP_FUNORD_GREATER_THAN_EQUAL 191u
#define MICROBIT_SPIRV_RUNNER_OP_DECORATE 71u
#define MICROBIT_SPIRV_RUNNER_OP_MEMBER_DECORATE 72u
#define MICROBIT_SPIRV_RUNNER_DECORATION_ARRAY_STRIDE 6u
#define MICROBIT_SPIRV_RUNNER_DECORATION_MATRIX_STRIDE 7u
#define MICROBIT_SPIRV_RUNNER_DECORATION_OFFSET 35u
#define MICROBIT_SPIRV_RUNNER_OP_PHI           245u
#define MICROBIT_SPIRV_RUNNER_OP_LOOP_MERGE    246u
#define MICROBIT_SPIRV_RUNNER_OP_SELECTION_MERGE 247u
#define MICROBIT_SPIRV_RUNNER_OP_BRANCH        249u
#define MICROBIT_SPIRV_RUNNER_OP_BRANCH_CONDITIONAL 250u
#define MICROBIT_SPIRV_RUNNER_VALUE_UNDEFINED 0u
#define MICROBIT_SPIRV_RUNNER_VALUE_BOOL      1u
#define MICROBIT_SPIRV_RUNNER_VALUE_SINT      2u
#define MICROBIT_SPIRV_RUNNER_VALUE_UINT      3u
#define MICROBIT_SPIRV_RUNNER_VALUE_FLOAT     4u
#define MICROBIT_SPIRV_RUNNER_VALUE_VECTOR    5u

static struct R_Microbit_SpirvParserResult*
R_Microbit_SpirvRunnerResult (struct R_Microbit_SpirvParserState* pState, uint32_t id)
{
    R_MICROBIT_ASSERT (pState != NULL);
    R_MICROBIT_ASSERT (pState->pOwner != NULL);
    R_MICROBIT_ASSERT (id < pState->pOwner->bound);
    if (!pState || !pState->pOwner || id >= pState->pOwner->bound)
        return NULL;
    return &pState->pResults[id];
}

static int
R_Microbit_SpirvRunnerHasOperands (uint32_t wordCount, uint32_t required)
{
    R_MICROBIT_ASSERT (wordCount >= required);
    return wordCount >= required;
}

static uint32_t
R_Microbit_SpirvRunnerAlignUp (uint32_t value, uint32_t alignment)
{
    if (alignment == 0u) return value;
    uint32_t remainder = value % alignment;
    return remainder == 0u ? value : value + alignment - remainder;
}

static void
R_Microbit_SpirvRunnerSetType (uint32_t wordCount, struct R_Microbit_SpirvParserState* pState)
{
    if (!R_Microbit_SpirvRunnerHasOperands (wordCount, 1u)) return;
    uint32_t id = pState->pCodeCurrent[0];
    struct R_Microbit_SpirvParserResult* pResult = R_Microbit_SpirvRunnerResult (pState, id);
    if (!pResult) return;
    pResult->valueType = MICROBIT_SPIRV_VALUE_TYPE_VOID;
    pResult->cpuByteSize = 0u;
    pResult->cpuAlignment = 1u;
    if (pState->pCodeCurrent[-1] == MICROBIT_SPIRV_RUNNER_OP_TYPE_BOOL) pResult->valueType = MICROBIT_SPIRV_VALUE_TYPE_BOOL;
    if (pState->pCodeCurrent[-1] == MICROBIT_SPIRV_RUNNER_OP_TYPE_INT)
    {
        pResult->valueType = MICROBIT_SPIRV_VALUE_TYPE_INT;
        pResult->valueBitcount = wordCount >= 2u ? pState->pCodeCurrent[1] : 32u;
        pResult->cpuByteSize = pResult->valueBitcount / 8u;
        pResult->cpuAlignment = pResult->cpuByteSize > 4u ? 8u : 4u;
    }
    if (pState->pCodeCurrent[-1] == MICROBIT_SPIRV_RUNNER_OP_TYPE_FLOAT)
    {
        pResult->valueType = MICROBIT_SPIRV_VALUE_TYPE_FLOAT;
        pResult->valueBitcount = wordCount >= 2u ? pState->pCodeCurrent[1] : 32u;
        pResult->cpuByteSize = pResult->valueBitcount / 8u;
        pResult->cpuAlignment = pResult->cpuByteSize > 4u ? 8u : 4u;
    }
    if (pState->pCodeCurrent[-1] == MICROBIT_SPIRV_RUNNER_OP_TYPE_VECTOR)
    {
        pResult->valueType = MICROBIT_SPIRV_VALUE_TYPE_VECTOR;
        if (wordCount >= 3u)
        {
            pResult->cpuElementType = pState->pCodeCurrent[1];
            pResult->cpuElementCount = pState->pCodeCurrent[2];
            pResult->cpuComponentCount = (uint8_t)pState->pCodeCurrent[2];
            struct R_Microbit_SpirvParserResult* pElement = R_Microbit_SpirvRunnerResult (pState, pResult->cpuElementType);
            uint32_t elementSize = pElement && pElement->cpuByteSize ? pElement->cpuByteSize : 4u;
            pResult->cpuAlignment = pResult->cpuElementCount == 2u ? elementSize * 2u : elementSize * 4u;
            pResult->cpuByteSize = elementSize * pResult->cpuElementCount;
        }
    }
    if (pState->pCodeCurrent[-1] == MICROBIT_SPIRV_RUNNER_OP_TYPE_INT && wordCount >= 3u)
        pResult->valueSign = (int8_t)pState->pCodeCurrent[2];
    if (pState->pCodeCurrent[-1] == MICROBIT_SPIRV_RUNNER_OP_TYPE_MATRIX && wordCount >= 3u)
    {
        pResult->valueType = MICROBIT_SPIRV_VALUE_TYPE_MATRIX;
        pResult->cpuElementType = pState->pCodeCurrent[1];
        pResult->cpuElementCount = pState->pCodeCurrent[2];
        struct R_Microbit_SpirvParserResult* pColumn = R_Microbit_SpirvRunnerResult (pState, pResult->cpuElementType);
        uint32_t columnSize = pColumn ? pColumn->cpuByteSize : 16u;
        pResult->cpuMatrixStride = R_Microbit_SpirvRunnerAlignUp (columnSize, 16u);
        pResult->cpuByteSize = pResult->cpuMatrixStride * pResult->cpuElementCount;
        pResult->cpuAlignment = 16u;
    }
    if (pState->pCodeCurrent[-1] == MICROBIT_SPIRV_RUNNER_OP_TYPE_ARRAY && wordCount >= 3u)
    {
        pResult->valueType = MICROBIT_SPIRV_VALUE_TYPE_ARRAY;
        pResult->cpuElementType = pState->pCodeCurrent[1];
        struct R_Microbit_SpirvParserResult* pElement = R_Microbit_SpirvRunnerResult (pState, pResult->cpuElementType);
        struct R_Microbit_SpirvParserResult* pLength = R_Microbit_SpirvRunnerResult (pState, pState->pCodeCurrent[2]);
        pResult->cpuElementCount = pLength ? pLength->cpuWords[0] : 0u;
        uint32_t elementSize = pElement ? pElement->cpuByteSize : 4u;
        pResult->cpuArrayStride = R_Microbit_SpirvRunnerAlignUp (elementSize, 16u);
        pResult->cpuByteSize = pResult->cpuArrayStride * pResult->cpuElementCount;
        pResult->cpuAlignment = 16u;
    }
    if (pState->pCodeCurrent[-1] == MICROBIT_SPIRV_RUNNER_OP_TYPE_RUNTIME_ARRAY && wordCount >= 2u)
    {
        pResult->valueType = MICROBIT_SPIRV_VALUE_TYPE_RUNTIME_ARRAY;
        pResult->cpuElementType = pState->pCodeCurrent[1];
        pResult->cpuArrayStride = 16u;
        pResult->cpuAlignment = 16u;
    }
    if (pState->pCodeCurrent[-1] == MICROBIT_SPIRV_RUNNER_OP_TYPE_POINTER && wordCount >= 3u)
    {
        pResult->valueType = MICROBIT_SPIRV_VALUE_TYPE_POINTER;
        pResult->cpuElementType = pState->pCodeCurrent[2];
        pResult->cpuByteSize = (uint32_t)sizeof (void*);
        pResult->cpuAlignment = pResult->cpuByteSize;
    }
    if (pState->pCodeCurrent[-1] == MICROBIT_SPIRV_RUNNER_OP_TYPE_STRUCT)
    {
        pResult->valueType = MICROBIT_SPIRV_VALUE_TYPE_STRUCT;
        uint32_t offset = 0u, alignment = 16u;
        pResult->cpuMemberCount = wordCount > 1u ? wordCount - 1u : 0u;
        if (pResult->cpuMemberCount > 16u) pResult->cpuMemberCount = 16u;
        for (uint32_t i = 0u; i < pResult->cpuMemberCount; ++i)
        {
            struct R_Microbit_SpirvParserResult* pMember = R_Microbit_SpirvRunnerResult (pState, pState->pCodeCurrent[1u + i]);
            uint32_t memberAlignment = pMember && pMember->cpuAlignment ? pMember->cpuAlignment : 4u;
            uint32_t memberSize = pMember && pMember->cpuByteSize ? pMember->cpuByteSize : 4u;
            offset = R_Microbit_SpirvRunnerAlignUp (offset, memberAlignment);
            pResult->cpuMemberOffsets[i] = offset;
            offset += memberSize;
            if (memberAlignment > alignment) alignment = memberAlignment;
        }
        pResult->cpuAlignment = R_Microbit_SpirvRunnerAlignUp (alignment, 16u);
        pResult->cpuByteSize = R_Microbit_SpirvRunnerAlignUp (offset, pResult->cpuAlignment);
    }
}

static void
R_Microbit_SpirvRunnerConstant (uint32_t wordCount, struct R_Microbit_SpirvParserState* pState)
{
    if (!R_Microbit_SpirvRunnerHasOperands (wordCount, 3u)) return;
    struct R_Microbit_SpirvParserResult* pType = R_Microbit_SpirvRunnerResult (pState, pState->pCodeCurrent[0]);
    struct R_Microbit_SpirvParserResult* pResult = R_Microbit_SpirvRunnerResult (pState, pState->pCodeCurrent[1]);
    if (!pType || !pResult) return;
    pResult->cpuWordCount = (uint16_t)(wordCount - 2u > 16u ? 16u : wordCount - 2u);
    pResult->cpuComponentCount = 1u;
    memcpy (pResult->cpuWords, pState->pCodeCurrent + 2u, pResult->cpuWordCount * sizeof (uint32_t));
    pResult->cpuValueTag = pType->valueType == MICROBIT_SPIRV_VALUE_TYPE_FLOAT
        ? MICROBIT_SPIRV_RUNNER_VALUE_FLOAT
        : pType->valueType == MICROBIT_SPIRV_VALUE_TYPE_BOOL
            ? MICROBIT_SPIRV_RUNNER_VALUE_BOOL
            : pType->valueSign ? MICROBIT_SPIRV_RUNNER_VALUE_SINT : MICROBIT_SPIRV_RUNNER_VALUE_UINT;
}

static void
R_Microbit_SpirvRunnerVariable (uint32_t wordCount, struct R_Microbit_SpirvParserState* pState)
{
    if (!R_Microbit_SpirvRunnerHasOperands (wordCount, 3u)) return;
    struct R_Microbit_SpirvParserResult* pResult = R_Microbit_SpirvRunnerResult (pState, pState->pCodeCurrent[1]);
    if (!pResult) return;
    pResult->cpuValueTag = MICROBIT_SPIRV_RUNNER_VALUE_UNDEFINED;
    pResult->cpuPointerTarget = pState->pCodeCurrent[1];
    pResult->pointer = pState->pCodeCurrent[1];
}

static void
R_Microbit_SpirvRunnerBinaryArithmetic (uint32_t wordCount, struct R_Microbit_SpirvParserState* pState)
{
    if (!R_Microbit_SpirvRunnerHasOperands (wordCount, 4u)) return;
    struct R_Microbit_SpirvParserResult* pResult = R_Microbit_SpirvRunnerResult (pState, pState->pCodeCurrent[1]);
    struct R_Microbit_SpirvParserResult* pLeft = R_Microbit_SpirvRunnerResult (pState, pState->pCodeCurrent[2]);
    struct R_Microbit_SpirvParserResult* pRight = R_Microbit_SpirvRunnerResult (pState, pState->pCodeCurrent[3]);
    if (!pResult || !pLeft || !pRight) return;
    uint32_t opcode = pState->pCodeCurrent[-1];
    uint32_t count = pLeft->cpuComponentCount ? pLeft->cpuComponentCount : 1u;
    if (count > 16u) count = 16u;
    pResult->cpuComponentCount = (uint8_t)count;
    pResult->cpuWordCount = (uint16_t)count;
    pResult->cpuValueTag = (opcode == MICROBIT_SPIRV_RUNNER_OP_FADD || opcode == MICROBIT_SPIRV_RUNNER_OP_FSUB
        || opcode == MICROBIT_SPIRV_RUNNER_OP_FMUL || opcode == MICROBIT_SPIRV_RUNNER_OP_FDIV
        || opcode == MICROBIT_SPIRV_RUNNER_OP_FREM || opcode == MICROBIT_SPIRV_RUNNER_OP_FMOD)
        ? MICROBIT_SPIRV_RUNNER_VALUE_FLOAT : pLeft->cpuValueTag;
    for (uint32_t i = 0; i < count; ++i)
    {
        uint32_t left = pLeft->cpuWords[i], right = pRight->cpuWords[i];
        if (pResult->cpuValueTag == MICROBIT_SPIRV_RUNNER_VALUE_FLOAT)
        {
            float a, b, value;
            memcpy (&a, &left, sizeof (a)); memcpy (&b, &right, sizeof (b));
            value = opcode == MICROBIT_SPIRV_RUNNER_OP_FADD ? a + b : opcode == MICROBIT_SPIRV_RUNNER_OP_FSUB ? a - b
                : opcode == MICROBIT_SPIRV_RUNNER_OP_FMUL ? a * b : opcode == MICROBIT_SPIRV_RUNNER_OP_FDIV ? a / b
                : fmodf (a, b);
            memcpy (&pResult->cpuWords[i], &value, sizeof (value));
        }
        else pResult->cpuWords[i] = opcode == MICROBIT_SPIRV_RUNNER_OP_IADD ? left + right
            : opcode == MICROBIT_SPIRV_RUNNER_OP_ISUB ? left - right
            : opcode == MICROBIT_SPIRV_RUNNER_OP_IMUL ? left * right
            : opcode == MICROBIT_SPIRV_RUNNER_OP_UDIV || opcode == MICROBIT_SPIRV_RUNNER_OP_SDIV ? (right ? left / right : 0u)
            : opcode == MICROBIT_SPIRV_RUNNER_OP_UMOD || opcode == MICROBIT_SPIRV_RUNNER_OP_SREM || opcode == MICROBIT_SPIRV_RUNNER_OP_SMOD ? (right ? left % right : 0u)
            : 0u;
    }
}

static void
R_Microbit_SpirvRunnerLoad (uint32_t wordCount, struct R_Microbit_SpirvParserState* pState)
{
    if (!R_Microbit_SpirvRunnerHasOperands (wordCount, 3u)) return;
    struct R_Microbit_SpirvParserResult* pResult = R_Microbit_SpirvRunnerResult (pState, pState->pCodeCurrent[1]);
    struct R_Microbit_SpirvParserResult* pPointer = R_Microbit_SpirvRunnerResult (pState, pState->pCodeCurrent[2]);
    if (!pResult || !pPointer) return;
    *pResult = *pPointer;
    pResult->cpuValueTag = pPointer->cpuValueTag == MICROBIT_SPIRV_RUNNER_VALUE_UNDEFINED
        ? MICROBIT_SPIRV_RUNNER_VALUE_UINT : pPointer->cpuValueTag;
}

static void
R_Microbit_SpirvRunnerStore (uint32_t wordCount, struct R_Microbit_SpirvParserState* pState)
{
    if (!R_Microbit_SpirvRunnerHasOperands (wordCount, 2u)) return;
    struct R_Microbit_SpirvParserResult* pPointer = R_Microbit_SpirvRunnerResult (pState, pState->pCodeCurrent[0]);
    struct R_Microbit_SpirvParserResult* pValue = R_Microbit_SpirvRunnerResult (pState, pState->pCodeCurrent[1]);
    if (!pPointer || !pValue) return;
    pPointer->cpuValueTag = pValue->cpuValueTag;
    pPointer->cpuComponentCount = pValue->cpuComponentCount;
    pPointer->cpuWordCount = pValue->cpuWordCount;
    memcpy (pPointer->cpuWords, pValue->cpuWords, sizeof (pPointer->cpuWords));
}

static uint32_t
R_Microbit_SpirvRunnerFloatToUnsigned (uint32_t bits)
{
    uint32_t sign = bits >> 31u;
    uint32_t exponent = (bits >> 23u) & 0xFFu;
    uint32_t mantissa = bits & 0x7FFFFFu;
    if (exponent == 0xFFu) return sign || mantissa ? 0u : UINT32_MAX;
    if (exponent < 127u) return 0u;
    int shift = (int)exponent - 127 - 23;
    uint64_t significand = (uint64_t)(mantissa | 0x800000u);
    uint64_t magnitude = shift >= 0 ? significand << shift : significand >> -shift;
    if (sign) return 0u;
    return magnitude > UINT32_MAX ? UINT32_MAX : (uint32_t)magnitude;
}

static int32_t
R_Microbit_SpirvRunnerFloatToSigned (uint32_t bits)
{
    uint32_t sign = bits >> 31u;
    uint32_t exponent = (bits >> 23u) & 0xFFu;
    uint32_t mantissa = bits & 0x7FFFFFu;
    if (exponent == 0xFFu) return sign ? INT32_MIN : INT32_MAX;
    if (exponent < 127u) return 0;
    int shift = (int)exponent - 127 - 23;
    uint64_t significand = (uint64_t)(mantissa | 0x800000u);
    uint64_t magnitude = shift >= 0 ? significand << shift : significand >> -shift;
    if (sign) return magnitude >= 0x80000000ull ? INT32_MIN : -(int32_t)magnitude;
    return magnitude >= 0x80000000ull ? INT32_MAX : (int32_t)magnitude;
}

static void
R_Microbit_SpirvRunnerCompare (uint32_t wordCount, struct R_Microbit_SpirvParserState* pState)
{
    if (!R_Microbit_SpirvRunnerHasOperands (wordCount, 4u)) return;
    struct R_Microbit_SpirvParserResult* pResult = R_Microbit_SpirvRunnerResult (pState, pState->pCodeCurrent[1]);
    struct R_Microbit_SpirvParserResult* pLeft = R_Microbit_SpirvRunnerResult (pState, pState->pCodeCurrent[2]);
    struct R_Microbit_SpirvParserResult* pRight = R_Microbit_SpirvRunnerResult (pState, pState->pCodeCurrent[3]);
    if (!pResult || !pLeft || !pRight) return;
    uint32_t opcode = pState->pCodeCurrent[-1];
    uint32_t count = pLeft->cpuComponentCount ? pLeft->cpuComponentCount : 1u;
    if (count > 16u) count = 16u;
    pResult->cpuValueTag = MICROBIT_SPIRV_RUNNER_VALUE_BOOL;
    pResult->cpuComponentCount = (uint8_t)count;
    pResult->cpuWordCount = (uint16_t)count;
    for (uint32_t i = 0u; i < count; ++i)
    {
        uint32_t a = pLeft->cpuWords[i], b = pRight->cpuWords[i];
        int comparison;
        int isFloat = opcode >= MICROBIT_SPIRV_RUNNER_OP_FORD_EQUAL;
        if (isFloat)
        {
            float af, bf; memcpy (&af, &a, sizeof (af)); memcpy (&bf, &b, sizeof (bf));
            int unordered = isnan (af) || isnan (bf);
            comparison = opcode == MICROBIT_SPIRV_RUNNER_OP_FORD_EQUAL || opcode == MICROBIT_SPIRV_RUNNER_OP_FUNORD_EQUAL ? af == bf
                : opcode == MICROBIT_SPIRV_RUNNER_OP_FORD_NOT_EQUAL || opcode == MICROBIT_SPIRV_RUNNER_OP_FUNORD_NOT_EQUAL ? af != bf
                : opcode == MICROBIT_SPIRV_RUNNER_OP_FORD_LESS_THAN || opcode == MICROBIT_SPIRV_RUNNER_OP_FUNORD_LESS_THAN ? af < bf
                : opcode == MICROBIT_SPIRV_RUNNER_OP_FORD_GREATER_THAN || opcode == MICROBIT_SPIRV_RUNNER_OP_FUNORD_GREATER_THAN ? af > bf
                : opcode == MICROBIT_SPIRV_RUNNER_OP_FORD_LESS_THAN_EQUAL || opcode == MICROBIT_SPIRV_RUNNER_OP_FUNORD_LESS_THAN_EQUAL ? af <= bf : af >= bf;
            if (opcode >= MICROBIT_SPIRV_RUNNER_OP_FORD_EQUAL && opcode <= MICROBIT_SPIRV_RUNNER_OP_FORD_GREATER_THAN_EQUAL) comparison = comparison && !unordered;
            else comparison = comparison || unordered;
        }
        else if (opcode == MICROBIT_SPIRV_RUNNER_OP_I_EQUAL) comparison = a == b;
        else if (opcode == MICROBIT_SPIRV_RUNNER_OP_I_NOT_EQUAL) comparison = a != b;
        else if (opcode == MICROBIT_SPIRV_RUNNER_OP_U_LESS_THAN) comparison = a < b;
        else if (opcode == MICROBIT_SPIRV_RUNNER_OP_S_LESS_THAN) comparison = (int32_t)a < (int32_t)b;
        else if (opcode == MICROBIT_SPIRV_RUNNER_OP_U_GREATER_THAN) comparison = a > b;
        else if (opcode == MICROBIT_SPIRV_RUNNER_OP_S_GREATER_THAN) comparison = (int32_t)a > (int32_t)b;
        else if (opcode == MICROBIT_SPIRV_RUNNER_OP_U_LESS_THAN_EQUAL) comparison = a <= b;
        else if (opcode == MICROBIT_SPIRV_RUNNER_OP_S_LESS_THAN_EQUAL) comparison = (int32_t)a <= (int32_t)b;
        else if (opcode == MICROBIT_SPIRV_RUNNER_OP_U_GREATER_THAN_EQUAL) comparison = a >= b;
        else comparison = (int32_t)a >= (int32_t)b;
        pResult->cpuWords[i] = comparison ? 1u : 0u;
    }
}

static void
R_Microbit_SpirvRunnerConvert (uint32_t wordCount, struct R_Microbit_SpirvParserState* pState)
{
    if (!R_Microbit_SpirvRunnerHasOperands (wordCount, 3u)) return;
    struct R_Microbit_SpirvParserResult* pType = R_Microbit_SpirvRunnerResult (pState, pState->pCodeCurrent[0]);
    struct R_Microbit_SpirvParserResult* pResult = R_Microbit_SpirvRunnerResult (pState, pState->pCodeCurrent[1]);
    struct R_Microbit_SpirvParserResult* pSource = R_Microbit_SpirvRunnerResult (pState, pState->pCodeCurrent[2]);
    if (!pType || !pResult || !pSource) return;
    uint32_t opcode = pState->pCodeCurrent[-1];
    uint32_t count = pSource->cpuComponentCount ? pSource->cpuComponentCount : 1u;
    if (count > 16u) count = 16u;
    pResult->cpuComponentCount = (uint8_t)count; pResult->cpuWordCount = (uint16_t)count;
    pResult->cpuValueTag = pType->valueType == MICROBIT_SPIRV_VALUE_TYPE_FLOAT ? MICROBIT_SPIRV_RUNNER_VALUE_FLOAT
        : pType->valueType == MICROBIT_SPIRV_VALUE_TYPE_INT ? (pType->valueSign ? MICROBIT_SPIRV_RUNNER_VALUE_SINT : MICROBIT_SPIRV_RUNNER_VALUE_UINT)
        : pSource->cpuValueTag;
    for (uint32_t i = 0u; i < count; ++i)
    {
        uint32_t source = pSource->cpuWords[i], result = source;
        if (opcode == MICROBIT_SPIRV_RUNNER_OP_CONVERT_F_TO_U) result = R_Microbit_SpirvRunnerFloatToUnsigned (source);
        else if (opcode == MICROBIT_SPIRV_RUNNER_OP_CONVERT_F_TO_S) result = (uint32_t)R_Microbit_SpirvRunnerFloatToSigned (source);
        else if (opcode == MICROBIT_SPIRV_RUNNER_OP_CONVERT_S_TO_F || opcode == MICROBIT_SPIRV_RUNNER_OP_CONVERT_U_TO_F)
        {
            float value = opcode == MICROBIT_SPIRV_RUNNER_OP_CONVERT_S_TO_F ? (float)(int32_t)source : (float)source;
            memcpy (&result, &value, sizeof (result));
        }
        else if (opcode == MICROBIT_SPIRV_RUNNER_OP_S_CONVERT && pType->valueBitcount < 32u)
        {
            uint32_t mask = (1u << pType->valueBitcount) - 1u; result &= mask;
            if (result & (1u << (pType->valueBitcount - 1u))) result |= ~mask;
        }
        else if (opcode == MICROBIT_SPIRV_RUNNER_OP_U_CONVERT && pType->valueBitcount < 32u)
            result &= (1u << pType->valueBitcount) - 1u;
        pResult->cpuWords[i] = result;
    }
}

static void
R_Microbit_SpirvRunnerMemberDecorate (uint32_t wordCount, struct R_Microbit_SpirvParserState* pState)
{
    if (!R_Microbit_SpirvRunnerHasOperands (wordCount, 4u)) return;
    struct R_Microbit_SpirvParserResult* pType = R_Microbit_SpirvRunnerResult (pState, pState->pCodeCurrent[0]);
    uint32_t member = pState->pCodeCurrent[1], decoration = pState->pCodeCurrent[2];
    if (pType && member < 16u && decoration == MICROBIT_SPIRV_RUNNER_DECORATION_OFFSET)
        pType->cpuMemberOffsets[member] = pState->pCodeCurrent[3];
}

static void
R_Microbit_SpirvRunnerDecorate (uint32_t wordCount, struct R_Microbit_SpirvParserState* pState)
{
    if (!R_Microbit_SpirvRunnerHasOperands (wordCount, 3u)) return;
    struct R_Microbit_SpirvParserResult* pType = R_Microbit_SpirvRunnerResult (pState, pState->pCodeCurrent[0]);
    if (!pType) return;
    uint32_t decoration = pState->pCodeCurrent[1];
    if (decoration == MICROBIT_SPIRV_RUNNER_DECORATION_ARRAY_STRIDE) pType->cpuArrayStride = pState->pCodeCurrent[2];
    if (decoration == MICROBIT_SPIRV_RUNNER_DECORATION_MATRIX_STRIDE) pType->cpuMatrixStride = pState->pCodeCurrent[2];
}

static void
R_Microbit_SpirvRunnerCompositeConstruct (uint32_t wordCount, struct R_Microbit_SpirvParserState* pState)
{
    if (!R_Microbit_SpirvRunnerHasOperands (wordCount, 3u)) return;
    struct R_Microbit_SpirvParserResult* pResult = R_Microbit_SpirvRunnerResult (pState, pState->pCodeCurrent[1]);
    if (!pResult) return;
    pResult->cpuValueTag = MICROBIT_SPIRV_RUNNER_VALUE_VECTOR;
    pResult->cpuComponentCount = (uint8_t)(wordCount - 2u > 16u ? 16u : wordCount - 2u);
    pResult->cpuWordCount = pResult->cpuComponentCount;
    for (uint32_t i = 0; i < pResult->cpuComponentCount; ++i)
    {
        struct R_Microbit_SpirvParserResult* pPart = R_Microbit_SpirvRunnerResult (pState, pState->pCodeCurrent[2u + i]);
        if (!pPart || pPart->cpuWordCount == 0u) return;
        pResult->cpuWords[i] = pPart->cpuWords[0];
        if (pPart->cpuComponentCount > 1u)
            pResult->cpuWords[i] = pPart->cpuWords[0];
        pResult->cpuValueTag = pPart->cpuValueTag;
    }
}

static void
R_Microbit_SpirvRunnerCompositeExtract (uint32_t wordCount, struct R_Microbit_SpirvParserState* pState)
{
    if (!R_Microbit_SpirvRunnerHasOperands (wordCount, 4u)) return;
    struct R_Microbit_SpirvParserResult* pResult = R_Microbit_SpirvRunnerResult (pState, pState->pCodeCurrent[1]);
    struct R_Microbit_SpirvParserResult* pObject = R_Microbit_SpirvRunnerResult (pState, pState->pCodeCurrent[2]);
    struct R_Microbit_SpirvParserResult* pIndex = R_Microbit_SpirvRunnerResult (pState, pState->pCodeCurrent[3]);
    if (!pResult || !pObject || !pIndex || pIndex->cpuWords[0] >= pObject->cpuWordCount) return;
    pResult->cpuValueTag = pObject->cpuValueTag;
    pResult->cpuComponentCount = 1u;
    pResult->cpuWordCount = 1u;
    pResult->cpuWords[0] = pObject->cpuWords[pIndex->cpuWords[0]];
}

static void
R_Microbit_SpirvRunnerCompositeInsert (uint32_t wordCount, struct R_Microbit_SpirvParserState* pState)
{
    if (!R_Microbit_SpirvRunnerHasOperands (wordCount, 5u)) return;
    struct R_Microbit_SpirvParserResult* pResult = R_Microbit_SpirvRunnerResult (pState, pState->pCodeCurrent[1]);
    struct R_Microbit_SpirvParserResult* pObject = R_Microbit_SpirvRunnerResult (pState, pState->pCodeCurrent[2]);
    struct R_Microbit_SpirvParserResult* pComposite = R_Microbit_SpirvRunnerResult (pState, pState->pCodeCurrent[3]);
    struct R_Microbit_SpirvParserResult* pIndex = R_Microbit_SpirvRunnerResult (pState, pState->pCodeCurrent[4]);
    if (!pResult || !pObject || !pComposite || !pIndex || pIndex->cpuWords[0] >= 16u) return;
    pResult->cpuValueTag = pComposite->cpuValueTag;
    pResult->cpuComponentCount = pComposite->cpuComponentCount;
    pResult->cpuWordCount = pComposite->cpuWordCount;
    memcpy (pResult->cpuWords, pComposite->cpuWords, sizeof (pResult->cpuWords));
    pResult->cpuWords[pIndex->cpuWords[0]] = pObject->cpuWords[0];
}

static void
R_Microbit_SpirvRunnerLabel (uint32_t wordCount, struct R_Microbit_SpirvParserState* pState)
{
    if (!R_Microbit_SpirvRunnerHasOperands (wordCount, 1u)) return;
    pState->currentLabel = pState->pCodeCurrent[0];
}

static void
R_Microbit_SpirvRunnerPhi (uint32_t wordCount, struct R_Microbit_SpirvParserState* pState)
{
    if (!R_Microbit_SpirvRunnerHasOperands (wordCount, 4u)) return;
    struct R_Microbit_SpirvParserResult* pResult = R_Microbit_SpirvRunnerResult (pState, pState->pCodeCurrent[1]);
    if (!pResult) return;
    for (uint32_t i = 2u; i + 1u < wordCount; i += 2u)
    {
        if (pState->pCodeCurrent[i + 1u] == pState->previousLabel)
        {
            struct R_Microbit_SpirvParserResult* pValue = R_Microbit_SpirvRunnerResult (pState, pState->pCodeCurrent[i]);
            if (pValue) *pResult = *pValue;
            return;
        }
    }
}

static const uint32_t*
R_Microbit_SpirvRunnerFindLabel (struct R_Microbit_SpirvParserState* pState, uint32_t labelId)
{
    const uint32_t* pCode = pState->pOwner->pCode;
    const uint32_t* pEnd = pCode + pState->pOwner->codeLength;
    while (pCode < pEnd)
    {
        uint32_t size = pCode[0] >> MICROBIT_SPIRV_RUNNER_WORD_COUNT_SHIFT;
        if (!size || pCode + size > pEnd) return NULL;
        if ((pCode[0] & MICROBIT_SPIRV_RUNNER_OPCODE_MASK) == MICROBIT_SPIRV_RUNNER_OP_LABEL
            && size >= 2u && pCode[1] == labelId) return pCode;
        pCode += size;
    }
    return NULL;
}

static void
R_Microbit_SpirvRunnerBranch (uint32_t wordCount, struct R_Microbit_SpirvParserState* pState)
{
    if (!R_Microbit_SpirvRunnerHasOperands (wordCount, 1u)) return;
    const uint32_t* pTarget = R_Microbit_SpirvRunnerFindLabel (pState, pState->pCodeCurrent[0]);
    R_MICROBIT_ASSERT (pTarget != NULL);
    if (pTarget) { pState->previousLabel = pState->currentLabel; pState->pCodeCurrent = pTarget; pState->didJump = 1u; }
}

static void
R_Microbit_SpirvRunnerBranchConditional (uint32_t wordCount, struct R_Microbit_SpirvParserState* pState)
{
    if (!R_Microbit_SpirvRunnerHasOperands (wordCount, 3u)) return;
    struct R_Microbit_SpirvParserResult* pCondition = R_Microbit_SpirvRunnerResult (pState, pState->pCodeCurrent[0]);
    if (!pCondition) return;
    uint32_t targetId = pCondition->cpuWords[0] ? pState->pCodeCurrent[1] : pState->pCodeCurrent[2];
    const uint32_t* pTarget = R_Microbit_SpirvRunnerFindLabel (pState, targetId);
    R_MICROBIT_ASSERT (pTarget != NULL);
    if (pTarget) { pState->previousLabel = pState->currentLabel; pState->pCodeCurrent = pTarget; pState->didJump = 1u; }
}

static const uint32_t*
R_Microbit_SpirvRunnerFindFunction (struct R_Microbit_SpirvParserState* pState, uint32_t functionId)
{
    const uint32_t* pCode = pState->pOwner->pCode;
    const uint32_t* pEnd = pCode + pState->pOwner->codeLength;
    while (pCode < pEnd)
    {
        uint32_t wordCount = pCode[0] >> MICROBIT_SPIRV_RUNNER_WORD_COUNT_SHIFT;
        if (!wordCount || pCode + wordCount > pEnd) return NULL;
        if ((pCode[0] & MICROBIT_SPIRV_RUNNER_OPCODE_MASK) == MICROBIT_SPIRV_RUNNER_OP_FUNCTION
            && wordCount >= 3u && pCode[2] == functionId)
            return pCode;
        pCode += wordCount;
    }
    return NULL;
}

static int
R_Microbit_SpirvRunnerEnsureCallStack (struct R_Microbit_SpirvParserState* pState)
{
    if (pState->functionStackCount != 0u) return 1;
    pState->functionStackCount = 64u;
    pState->functionStackCurrent = 0u;
    pState->ppFunctionStack = (const uint32_t**)R_CSTL_HeapAlloc (
        pState->functionStackCount * sizeof (*pState->ppFunctionStack));
    pState->ppFunctionStackInfo = (struct R_Microbit_SpirvParserResult**)R_CSTL_HeapAlloc (
        pState->functionStackCount * sizeof (*pState->ppFunctionStackInfo));
    pState->pFunctionStackReturns = (uint32_t*)R_CSTL_HeapAlloc (
        pState->functionStackCount * sizeof (*pState->pFunctionStackReturns));
    return pState->ppFunctionStack && pState->ppFunctionStackInfo && pState->pFunctionStackReturns;
}

static void
R_Microbit_SpirvRunnerReturn (uint32_t wordCount, struct R_Microbit_SpirvParserState* pState)
{
    (void)wordCount;
    if (pState->functionStackCurrent == 0u)
    {
        pState->pCodeCurrent = NULL;
        pState->didJump = 1u;
        return;
    }
    --pState->functionStackCurrent;
    pState->pCodeCurrent = pState->ppFunctionStack[pState->functionStackCurrent];
    pState->didJump = 1u;
}

static void
R_Microbit_SpirvRunnerReturnValue (uint32_t wordCount, struct R_Microbit_SpirvParserState* pState)
{
    if (!R_Microbit_SpirvRunnerHasOperands (wordCount, 1u)) return;
    if (pState->functionStackCurrent > 0u)
    {
        uint32_t destination = pState->pFunctionStackReturns[pState->functionStackCurrent - 1u];
        struct R_Microbit_SpirvParserResult* pSource = R_Microbit_SpirvRunnerResult (pState, pState->pCodeCurrent[0]);
        struct R_Microbit_SpirvParserResult* pDestination = R_Microbit_SpirvRunnerResult (pState, destination);
        if (pSource && pDestination) *pDestination = *pSource;
    }
    R_Microbit_SpirvRunnerReturn (wordCount, pState);
}

static void
R_Microbit_SpirvRunnerFunctionCall (uint32_t wordCount, struct R_Microbit_SpirvParserState* pState)
{
    if (!R_Microbit_SpirvRunnerHasOperands (wordCount, 3u)) return;
    R_MICROBIT_ASSERT (R_Microbit_SpirvRunnerEnsureCallStack (pState));
    if (!R_Microbit_SpirvRunnerEnsureCallStack (pState)
        || pState->functionStackCurrent >= pState->functionStackCount)
        return;
    const uint32_t* pFunction = R_Microbit_SpirvRunnerFindFunction (pState, pState->pCodeCurrent[2]);
    R_MICROBIT_ASSERT (pFunction != NULL);
    if (!pFunction) return;
    pState->ppFunctionStack[pState->functionStackCurrent] = pState->pCodeCurrent + wordCount;
    pState->pFunctionStackReturns[pState->functionStackCurrent] = pState->pCodeCurrent[1];
    ++pState->functionStackCurrent;
    pState->pCodeCurrent = pFunction;
    pState->didJump = 1u;
}

static void
R_Microbit_SpirvRunnerEndExecution (uint32_t wordCount, struct R_Microbit_SpirvParserState* pState)
{
    (void)wordCount;
    pState->pCodeCurrent = NULL;
    pState->didJump = 1u;
}

static void
R_Microbit_SpirvRunnerIgnoreInstruction (uint32_t wordCount, struct R_Microbit_SpirvParserState* pState)
{
    (void)wordCount;
    (void)pState;
}

static void
R_Microbit_SpirvRunnerRegisterStructuralOpcodes (struct R_Microbit_SpirvParserContext* pContext)
{
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_NOP] = R_Microbit_SpirvRunnerIgnoreInstruction;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_FUNCTION] = R_Microbit_SpirvRunnerIgnoreInstruction;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_LABEL] = R_Microbit_SpirvRunnerIgnoreInstruction;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_FUNCTION_END] = R_Microbit_SpirvRunnerReturn;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_RETURN] = R_Microbit_SpirvRunnerReturn;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_RETURN_VALUE] = R_Microbit_SpirvRunnerReturnValue;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_LOOP_MERGE] = R_Microbit_SpirvRunnerIgnoreInstruction;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_SELECTION_MERGE] = R_Microbit_SpirvRunnerIgnoreInstruction;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_PHI] = R_Microbit_SpirvRunnerPhi;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_CONSTANT] = R_Microbit_SpirvRunnerConstant;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_VARIABLE] = R_Microbit_SpirvRunnerVariable;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_LOAD] = R_Microbit_SpirvRunnerLoad;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_STORE] = R_Microbit_SpirvRunnerStore;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_COMPOSITE_CONSTRUCT] = R_Microbit_SpirvRunnerCompositeConstruct;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_COMPOSITE_EXTRACT] = R_Microbit_SpirvRunnerCompositeExtract;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_COMPOSITE_INSERT] = R_Microbit_SpirvRunnerCompositeInsert;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_BRANCH] = R_Microbit_SpirvRunnerBranch;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_BRANCH_CONDITIONAL] = R_Microbit_SpirvRunnerBranchConditional;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_IADD] = R_Microbit_SpirvRunnerBinaryArithmetic;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_FADD] = R_Microbit_SpirvRunnerBinaryArithmetic;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_ISUB] = R_Microbit_SpirvRunnerBinaryArithmetic;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_FSUB] = R_Microbit_SpirvRunnerBinaryArithmetic;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_IMUL] = R_Microbit_SpirvRunnerBinaryArithmetic;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_FMUL] = R_Microbit_SpirvRunnerBinaryArithmetic;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_UDIV] = R_Microbit_SpirvRunnerBinaryArithmetic;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_SDIV] = R_Microbit_SpirvRunnerBinaryArithmetic;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_FDIV] = R_Microbit_SpirvRunnerBinaryArithmetic;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_UMOD] = R_Microbit_SpirvRunnerBinaryArithmetic;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_SREM] = R_Microbit_SpirvRunnerBinaryArithmetic;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_SMOD] = R_Microbit_SpirvRunnerBinaryArithmetic;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_FREM] = R_Microbit_SpirvRunnerBinaryArithmetic;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_FMOD] = R_Microbit_SpirvRunnerBinaryArithmetic;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_FUNCTION_CALL] = R_Microbit_SpirvRunnerFunctionCall;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_I_EQUAL] = R_Microbit_SpirvRunnerCompare;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_I_NOT_EQUAL] = R_Microbit_SpirvRunnerCompare;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_U_LESS_THAN] = R_Microbit_SpirvRunnerCompare;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_S_LESS_THAN] = R_Microbit_SpirvRunnerCompare;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_U_GREATER_THAN] = R_Microbit_SpirvRunnerCompare;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_S_GREATER_THAN] = R_Microbit_SpirvRunnerCompare;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_U_LESS_THAN_EQUAL] = R_Microbit_SpirvRunnerCompare;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_S_LESS_THAN_EQUAL] = R_Microbit_SpirvRunnerCompare;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_U_GREATER_THAN_EQUAL] = R_Microbit_SpirvRunnerCompare;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_S_GREATER_THAN_EQUAL] = R_Microbit_SpirvRunnerCompare;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_FORD_EQUAL] = R_Microbit_SpirvRunnerCompare;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_FORD_NOT_EQUAL] = R_Microbit_SpirvRunnerCompare;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_FORD_LESS_THAN] = R_Microbit_SpirvRunnerCompare;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_FORD_GREATER_THAN] = R_Microbit_SpirvRunnerCompare;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_FORD_LESS_THAN_EQUAL] = R_Microbit_SpirvRunnerCompare;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_FORD_GREATER_THAN_EQUAL] = R_Microbit_SpirvRunnerCompare;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_FUNORD_EQUAL] = R_Microbit_SpirvRunnerCompare;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_FUNORD_NOT_EQUAL] = R_Microbit_SpirvRunnerCompare;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_FUNORD_LESS_THAN] = R_Microbit_SpirvRunnerCompare;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_FUNORD_GREATER_THAN] = R_Microbit_SpirvRunnerCompare;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_FUNORD_LESS_THAN_EQUAL] = R_Microbit_SpirvRunnerCompare;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_FUNORD_GREATER_THAN_EQUAL] = R_Microbit_SpirvRunnerCompare;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_CONVERT_F_TO_U] = R_Microbit_SpirvRunnerConvert;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_CONVERT_F_TO_S] = R_Microbit_SpirvRunnerConvert;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_CONVERT_S_TO_F] = R_Microbit_SpirvRunnerConvert;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_CONVERT_U_TO_F] = R_Microbit_SpirvRunnerConvert;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_U_CONVERT] = R_Microbit_SpirvRunnerConvert;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_S_CONVERT] = R_Microbit_SpirvRunnerConvert;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_F_CONVERT] = R_Microbit_SpirvRunnerConvert;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_BITCAST] = R_Microbit_SpirvRunnerConvert;
    pContext->pOpcodeSetup[MICROBIT_SPIRV_RUNNER_OP_DECORATE] = R_Microbit_SpirvRunnerDecorate;
    pContext->pOpcodeSetup[MICROBIT_SPIRV_RUNNER_OP_MEMBER_DECORATE] = R_Microbit_SpirvRunnerMemberDecorate;
    pContext->pOpcodeSetup[MICROBIT_SPIRV_RUNNER_OP_TYPE_VOID] = R_Microbit_SpirvRunnerSetType;
    pContext->pOpcodeSetup[MICROBIT_SPIRV_RUNNER_OP_TYPE_BOOL] = R_Microbit_SpirvRunnerSetType;
    pContext->pOpcodeSetup[MICROBIT_SPIRV_RUNNER_OP_TYPE_INT] = R_Microbit_SpirvRunnerSetType;
    pContext->pOpcodeSetup[MICROBIT_SPIRV_RUNNER_OP_TYPE_FLOAT] = R_Microbit_SpirvRunnerSetType;
    pContext->pOpcodeSetup[MICROBIT_SPIRV_RUNNER_OP_TYPE_VECTOR] = R_Microbit_SpirvRunnerSetType;
    pContext->pOpcodeSetup[MICROBIT_SPIRV_RUNNER_OP_TYPE_MATRIX] = R_Microbit_SpirvRunnerSetType;
    pContext->pOpcodeSetup[MICROBIT_SPIRV_RUNNER_OP_TYPE_ARRAY] = R_Microbit_SpirvRunnerSetType;
    pContext->pOpcodeSetup[MICROBIT_SPIRV_RUNNER_OP_TYPE_RUNTIME_ARRAY] = R_Microbit_SpirvRunnerSetType;
    pContext->pOpcodeSetup[MICROBIT_SPIRV_RUNNER_OP_TYPE_STRUCT] = R_Microbit_SpirvRunnerSetType;
    pContext->pOpcodeSetup[MICROBIT_SPIRV_RUNNER_OP_TYPE_POINTER] = R_Microbit_SpirvRunnerSetType;
    pContext->pOpcodeSetup[MICROBIT_SPIRV_RUNNER_OP_CONSTANT] = R_Microbit_SpirvRunnerConstant;
    pContext->pOpcodeSetup[MICROBIT_SPIRV_RUNNER_OP_VARIABLE] = R_Microbit_SpirvRunnerVariable;
}

static enum R_Microbit_SpirvRunnerError
R_Microbit_SpirvRunnerFindEntryFunction (
    struct R_Microbit_SpirvParserProgram* pProgram,
    uint32_t                              entryPoint,
    const uint32_t**                      ppEntryCode)
{
    const uint32_t* pCode = pProgram->pCode;
    const uint32_t* pEnd = pCode + pProgram->codeLength;

    while (pCode < pEnd)
    {
        size_t   remainingWords = (size_t)(pEnd - pCode);
        uint32_t instruction = pCode[0];
        uint32_t wordCount = (instruction >> MICROBIT_SPIRV_RUNNER_WORD_COUNT_SHIFT) & 0xFFFFu;
        uint32_t opcode = instruction & MICROBIT_SPIRV_RUNNER_OPCODE_MASK;

        if (wordCount == 0u || wordCount > remainingWords) return MICROBIT_SPIRV_RUNNER_ERROR_INVALID_PROGRAM;

        if (opcode == MICROBIT_SPIRV_RUNNER_OP_FUNCTION)
        {
            if (wordCount < 3u) return MICROBIT_SPIRV_RUNNER_ERROR_INVALID_PROGRAM;
            if (pCode[2] == entryPoint)
            {
                *ppEntryCode = pCode;
                return MICROBIT_SPIRV_RUNNER_OK;
            }
        }

        pCode += wordCount;
    }

    return MICROBIT_SPIRV_RUNNER_ERROR_INVALID_PROGRAM;
}

R_MICROBIT_API const char*
R_Microbit_SpirvRunnerErrorToString (enum R_Microbit_SpirvRunnerError error)
{
    switch (error)
    {
    case MICROBIT_SPIRV_RUNNER_OK:
        return "Success";
    case MICROBIT_SPIRV_RUNNER_ERROR_FAILED:
        return "Operation failed";
    case MICROBIT_SPIRV_RUNNER_ERROR_OUT_OF_MEMORY:
        return "Out of memory";
    case MICROBIT_SPIRV_RUNNER_ERROR_INVALID_ARGUMENT:
        return "Invalid argument";
    case MICROBIT_SPIRV_RUNNER_ERROR_NULL_POINTER:
        return "Null pointer";
    case MICROBIT_SPIRV_RUNNER_ERROR_INVALID_STATE:
        return "Invalid runner state";
    case MICROBIT_SPIRV_RUNNER_ERROR_INVALID_PROGRAM:
        return "Invalid SPIR-V program";
    case MICROBIT_SPIRV_RUNNER_ERROR_UNSUPPORTED_OPCODE:
        return "Unsupported opcode";
    case MICROBIT_SPIRV_RUNNER_ERROR_UNKNOWN:
    default:
        return "Unknown error";
    }
}

static void
R_Microbit_SpirvRunnerInitializeExecutionState (struct R_Microbit_SpirvRunnerExecution* pExecution)
{
    R_MICROBIT_ASSERT (pExecution != NULL);
    if (!pExecution->pState)
    {
        return;
    }

    pExecution->instructionCount = 0u;
    pExecution->stepLimit = 0u;
    pExecution->lastOpcode = 0u;
    pExecution->lastError = MICROBIT_SPIRV_RUNNER_OK;
    pExecution->didComplete = 0u;
}

R_MICROBIT_API struct R_Microbit_SpirvRunnerContext*
R_Microbit_NewSpirvRunnerContext (void)
{
    struct R_Microbit_SpirvRunnerContext* pContext = (struct R_Microbit_SpirvRunnerContext*)R_CSTL_HeapAlloc (
        sizeof (struct R_Microbit_SpirvRunnerContext));
    if (!pContext)
    {
        return NULL;
    }
    memset (pContext, 0, sizeof (struct R_Microbit_SpirvRunnerContext));
    pContext->pParserContext = R_Microbit_NewSpirvParserContext ();
    if (!pContext->pParserContext)
    {
        R_CSTL_HeapFree (pContext);
        return NULL;
    }
    R_Microbit_SpirvRunnerRegisterStructuralOpcodes (pContext->pParserContext);
    pContext->maxInstructions = 4096u;
    pContext->instructionLimit = 4096u;
    return pContext;
}

R_MICROBIT_API void
R_Microbit_DeleteSpirvRunnerContext (struct R_Microbit_SpirvRunnerContext* pContext)
{
    R_MICROBIT_ASSERT (pContext);
    if (pContext->pParserContext)
    {
        R_Microbit_DeleteSpirvParserContext (pContext->pParserContext);
    }
    R_CSTL_HeapFree (pContext);
}

R_MICROBIT_API enum R_Microbit_SpirvRunnerError
R_Microbit_NewSpirvRunnerProgram (
    struct R_Microbit_SpirvRunnerContext*  pContext,
    struct R_Microbit_SpirvParserProgram*  pProgram,
    uint32_t                               entryPoint,
    struct R_Microbit_SpirvRunnerProgram** ppRunnerProgram)
{
    R_MICROBIT_ASSERT (pContext != NULL);
    R_MICROBIT_ASSERT (pProgram != NULL);
    R_MICROBIT_ASSERT (ppRunnerProgram != NULL);

    if (!pContext || !pProgram || !ppRunnerProgram)
    {
        return MICROBIT_SPIRV_RUNNER_ERROR_NULL_POINTER;
    }

    *ppRunnerProgram = NULL;

    if (pProgram->pContext != pContext->pParserContext || pProgram->pCode == NULL
        || pProgram->codeLength == 0u || entryPoint >= pProgram->bound)
    {
        return MICROBIT_SPIRV_RUNNER_ERROR_INVALID_PROGRAM;
    }

    const uint32_t*                  pEntryCode = NULL;
    enum R_Microbit_SpirvRunnerError entryResult
        = R_Microbit_SpirvRunnerFindEntryFunction (pProgram, entryPoint, &pEntryCode);
    if (entryResult != MICROBIT_SPIRV_RUNNER_OK) return entryResult;

    struct R_Microbit_SpirvRunnerProgram* pRunnerProgram
        = (struct R_Microbit_SpirvRunnerProgram*)R_CSTL_HeapAlloc (
            sizeof (struct R_Microbit_SpirvRunnerProgram));
    if (!pRunnerProgram)
    {
        return MICROBIT_SPIRV_RUNNER_ERROR_OUT_OF_MEMORY;
    }

    memset (pRunnerProgram, 0, sizeof (struct R_Microbit_SpirvRunnerProgram));
    pRunnerProgram->pProgram = pProgram;
    pRunnerProgram->pContext = pContext;
    pRunnerProgram->pEntryCode = pEntryCode;
    pRunnerProgram->entryPoint = entryPoint;
    *ppRunnerProgram = pRunnerProgram;
    return MICROBIT_SPIRV_RUNNER_OK;
}

R_MICROBIT_API void
R_Microbit_DeleteSpirvRunnerProgram (struct R_Microbit_SpirvRunnerProgram* pRunnerProgram)
{
    if (!pRunnerProgram)
    {
        return;
    }

    R_CSTL_HeapFree (pRunnerProgram);
}

R_MICROBIT_API enum R_Microbit_SpirvRunnerError
R_Microbit_NewSpirvRunnerExecution (
    struct R_Microbit_SpirvRunnerProgram*    pRunnerProgram,
    struct R_Microbit_SpirvRunnerExecution** ppExecution)
{
    R_MICROBIT_ASSERT (pRunnerProgram != NULL);
    R_MICROBIT_ASSERT (ppExecution != NULL);

    if (!pRunnerProgram || !ppExecution)
    {
        return MICROBIT_SPIRV_RUNNER_ERROR_NULL_POINTER;
    }

    *ppExecution = NULL;
    if (!pRunnerProgram->pProgram || !pRunnerProgram->pContext || !pRunnerProgram->pProgram->pCode
        || pRunnerProgram->pProgram->codeLength == 0u)
    {
        return MICROBIT_SPIRV_RUNNER_ERROR_INVALID_PROGRAM;
    }

    struct R_Microbit_SpirvRunnerExecution* pExecution
        = (struct R_Microbit_SpirvRunnerExecution*)R_CSTL_HeapAlloc (
            sizeof (struct R_Microbit_SpirvRunnerExecution));
    if (!pExecution)
    {
        return MICROBIT_SPIRV_RUNNER_ERROR_OUT_OF_MEMORY;
    }

    memset (pExecution, 0, sizeof (struct R_Microbit_SpirvRunnerExecution));
    pExecution->pState = R_Microbit_NewSpirvParserState (pRunnerProgram->pProgram);
    if (!pExecution->pState)
    {
        R_CSTL_HeapFree (pExecution);
        return MICROBIT_SPIRV_RUNNER_ERROR_INVALID_PROGRAM;
    }

    pExecution->pCodeBegin = pRunnerProgram->pProgram->pCode;
    pExecution->pCodeEnd = pExecution->pCodeBegin + pRunnerProgram->pProgram->codeLength;
    pExecution->pContext = pRunnerProgram->pContext;
    pExecution->pState->pCodeCurrent = pRunnerProgram->pEntryCode;

    R_Microbit_SpirvRunnerInitializeExecutionState (pExecution);
    *ppExecution = pExecution;
    return MICROBIT_SPIRV_RUNNER_OK;
}

R_MICROBIT_API void
R_Microbit_DeleteSpirvRunnerExecution (struct R_Microbit_SpirvRunnerExecution* pExecution)
{
    R_MICROBIT_ASSERT (pExecution);
    if (pExecution->pState)
    {
        R_Microbit_DeleteSpirvParserState (pExecution->pState);
    }
    R_CSTL_HeapFree (pExecution);
}

R_MICROBIT_API enum R_Microbit_SpirvRunnerError
R_Microbit_SpirvRunnerExecute (struct R_Microbit_SpirvRunnerExecution* pExecution, uint32_t maxInstructions)
{
    if (!pExecution || !pExecution->pState)
    {
        return MICROBIT_SPIRV_RUNNER_ERROR_NULL_POINTER;
    }

    uint32_t requestedLimit = maxInstructions > 0u ? maxInstructions : 1u;
    uint32_t configuredLimit = pExecution->pContext->instructionLimit;
    if (configuredLimit == 0u) configuredLimit = pExecution->pContext->maxInstructions;
    if (configuredLimit > 0u && requestedLimit > configuredLimit) requestedLimit = configuredLimit;
    pExecution->stepLimit = requestedLimit;
    pExecution->instructionCount = 0u;
    pExecution->didComplete = 0u;

    for (uint32_t i = 0; i < pExecution->stepLimit; ++i)
    {
        enum R_Microbit_SpirvRunnerError result = R_Microbit_SpirvRunnerStep (pExecution);
        if (result != MICROBIT_SPIRV_RUNNER_OK)
        {
            return result;
        }
        if (pExecution->didComplete) return MICROBIT_SPIRV_RUNNER_OK;
    }

    return MICROBIT_SPIRV_RUNNER_OK;
}

R_MICROBIT_API enum R_Microbit_SpirvRunnerError
R_Microbit_SpirvRunnerStep (struct R_Microbit_SpirvRunnerExecution* pExecution)
{
    if (!pExecution || !pExecution->pState)
    {
        return MICROBIT_SPIRV_RUNNER_ERROR_NULL_POINTER;
    }

    pExecution->lastError = MICROBIT_SPIRV_RUNNER_OK;

    const uint32_t* pCurrent = pExecution->pState->pCodeCurrent;
    if (!pCurrent)
    {
        pExecution->didComplete = 1u;
        return MICROBIT_SPIRV_RUNNER_OK;
    }

    if (pCurrent < pExecution->pCodeBegin || pCurrent > pExecution->pCodeEnd)
    {
        R_CSTL_LOG_ERROR ("Instruction pointer is outside the program");
        pExecution->lastError = MICROBIT_SPIRV_RUNNER_ERROR_INVALID_STATE;
        return MICROBIT_SPIRV_RUNNER_ERROR_INVALID_STATE;
    }
    if (pCurrent == pExecution->pCodeEnd)
    {
        pExecution->didComplete = 1u;
        return MICROBIT_SPIRV_RUNNER_OK;
    }
    size_t   remainingWords = (size_t)(pExecution->pCodeEnd - pCurrent);
    uint32_t instructionWordCount = (pCurrent[0] >> MICROBIT_SPIRV_RUNNER_WORD_COUNT_SHIFT) & 0xFFFFu;
    uint32_t opcode = pCurrent[0] & MICROBIT_SPIRV_RUNNER_OPCODE_MASK;

    if (instructionWordCount == 0u || instructionWordCount > remainingWords)
    {
        R_CSTL_LOG_ERROR ("Truncated SPIR-V instruction");
        pExecution->lastError = MICROBIT_SPIRV_RUNNER_ERROR_INVALID_PROGRAM;
        return MICROBIT_SPIRV_RUNNER_ERROR_INVALID_PROGRAM;
    }

    if (opcode >= 0xFFFFu || pExecution->pState->pContext->pOpcodeExecute[opcode] == NULL)
    {
        R_CSTL_LOG_ERROR ("Unsupported SPIR-V opcode %u", opcode);
        pExecution->lastOpcode = opcode;
        pExecution->lastError = MICROBIT_SPIRV_RUNNER_ERROR_UNSUPPORTED_OPCODE;
        return MICROBIT_SPIRV_RUNNER_ERROR_UNSUPPORTED_OPCODE;
    }

    const uint32_t* previous = pCurrent;
    pExecution->lastOpcode = opcode;
    R_Microbit_SpirvParserStateStepOpcode (pExecution->pState);

    if (pExecution->pState->pCodeCurrent != NULL
        && (pExecution->pState->pCodeCurrent < pExecution->pCodeBegin
            || pExecution->pState->pCodeCurrent > pExecution->pCodeEnd))
    {
        R_CSTL_LOG_ERROR ("Opcode advanced outside the program");
        pExecution->lastError = MICROBIT_SPIRV_RUNNER_ERROR_INVALID_STATE;
        return MICROBIT_SPIRV_RUNNER_ERROR_INVALID_STATE;
    }
    if (pExecution->pState->pCodeCurrent == previous)
    {
        R_CSTL_LOG_ERROR ("Opcode did not advance execution");
        pExecution->lastError = MICROBIT_SPIRV_RUNNER_ERROR_INVALID_STATE;
        return MICROBIT_SPIRV_RUNNER_ERROR_INVALID_STATE;
    }

    pExecution->instructionCount++;
    if (pExecution->pState->pCodeCurrent == pExecution->pCodeEnd) pExecution->didComplete = 1u;
    return MICROBIT_SPIRV_RUNNER_OK;
}
