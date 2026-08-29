#include "microbit/spirvrunner/microbit_spirv_runner.h"
#include "microbit/microbit_platform.h"

#include "rlgame.base/cstl/cstl_heap_allocator.h"
#include "rlgame.base/cstl/cstl_log.h"

#include <string.h>

#define MICROBIT_SPIRV_RUNNER_OPCODE_MASK      0xFFFFu
#define MICROBIT_SPIRV_RUNNER_WORD_COUNT_SHIFT 16u
#define MICROBIT_SPIRV_RUNNER_OP_NOP           0u
#define MICROBIT_SPIRV_RUNNER_OP_FUNCTION      54u
#define MICROBIT_SPIRV_RUNNER_OP_FUNCTION_END  56u
#define MICROBIT_SPIRV_RUNNER_OP_LABEL         248u
#define MICROBIT_SPIRV_RUNNER_OP_RETURN        253u
#define MICROBIT_SPIRV_RUNNER_OP_RETURN_VALUE  254u

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
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_FUNCTION_END] = R_Microbit_SpirvRunnerEndExecution;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_RETURN] = R_Microbit_SpirvRunnerEndExecution;
    pContext->pOpcodeExecute[MICROBIT_SPIRV_RUNNER_OP_RETURN_VALUE] = R_Microbit_SpirvRunnerEndExecution;
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

MICROBIT_SPIRV_RUNNER_API const char*
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
    MICROBIT_SPIRV_RUNNER_ASSERT (pExecution != NULL);
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

MICROBIT_SPIRV_RUNNER_API struct R_Microbit_SpirvRunnerContext*
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

MICROBIT_SPIRV_RUNNER_API void
R_Microbit_DeleteSpirvRunnerContext (struct R_Microbit_SpirvRunnerContext* pContext)
{
    R_MICROBIT_ASSERT (pContext);
    if (pContext->pParserContext)
    {
        R_Microbit_DeleteSpirvParserContext (pContext->pParserContext);
    }
    R_CSTL_HeapFree (pContext);
}

MICROBIT_SPIRV_RUNNER_API enum R_Microbit_SpirvRunnerError
R_Microbit_NewSpirvRunnerProgram (
    struct R_Microbit_SpirvRunnerContext*  pContext,
    struct R_Microbit_SpirvParserProgram*  pProgram,
    uint32_t                               entryPoint,
    struct R_Microbit_SpirvRunnerProgram** ppRunnerProgram)
{
    MICROBIT_SPIRV_RUNNER_ASSERT (pContext != NULL);
    MICROBIT_SPIRV_RUNNER_ASSERT (pProgram != NULL);
    MICROBIT_SPIRV_RUNNER_ASSERT (ppRunnerProgram != NULL);

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

MICROBIT_SPIRV_RUNNER_API void
R_Microbit_DeleteSpirvRunnerProgram (struct R_Microbit_SpirvRunnerProgram* pRunnerProgram)
{
    if (!pRunnerProgram)
    {
        return;
    }

    R_CSTL_HeapFree (pRunnerProgram);
}

MICROBIT_SPIRV_RUNNER_API enum R_Microbit_SpirvRunnerError
R_Microbit_NewSpirvRunnerExecution (
    struct R_Microbit_SpirvRunnerProgram*    pRunnerProgram,
    struct R_Microbit_SpirvRunnerExecution** ppExecution)
{
    MICROBIT_SPIRV_RUNNER_ASSERT (pRunnerProgram != NULL);
    MICROBIT_SPIRV_RUNNER_ASSERT (ppExecution != NULL);

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

MICROBIT_SPIRV_RUNNER_API void
R_Microbit_DeleteSpirvRunnerExecution (struct R_Microbit_SpirvRunnerExecution* pExecution)
{
    R_MICROBIT_ASSERT (pExecution);
    if (pExecution->pState)
    {
        R_Microbit_DeleteSpirvParserState (pExecution->pState);
    }
    R_CSTL_HeapFree (pExecution);
}

MICROBIT_SPIRV_RUNNER_API enum R_Microbit_SpirvRunnerError
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

MICROBIT_SPIRV_RUNNER_API enum R_Microbit_SpirvRunnerError
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
