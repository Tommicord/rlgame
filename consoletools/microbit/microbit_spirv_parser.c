#include "microbit/microbit_spirv_parser.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"
#include "rlgame.base/cstl/cstl_log.h"

#include <string.h>
#include <stdlib.h>
#include <math.h>

#define MICROBIT_SPIRV_READ_WORD(spv) *((spv)++)
#define MICROBIT_SPIRV_SKIP_WORD(spv) ((spv)++)

static size_t
R_Microbit_SpirvParserGetInstructionWordCount (const uint32_t* pInstruction, size_t availableWords)
{
    MICROBIT_SPIRV_ASSERT (pInstruction != NULL);
    MICROBIT_SPIRV_ASSERT (availableWords > 0u);

    uint32_t encodedWordCount = (pInstruction[0] >> MICROBIT_SPIRV_WORD_COUNT_SHIFT) & 0xFFFFu;
    MICROBIT_SPIRV_ASSERT (encodedWordCount >= 1u);
    MICROBIT_SPIRV_ASSERT (encodedWordCount <= availableWords);

    return (size_t)encodedWordCount;
}

MICROBIT_SPIRV_API const char*
R_Microbit_SpirvParser_ErrorToString (enum R_Microbit_SpirvParserError error)
{
    switch (error)
    {
    case MICROBIT_SPIRV_OK:
        return "Success";
    case MICROBIT_SPIRV_ERROR_FAILED:
        return "Operation failed";
    case MICROBIT_SPIRV_ERROR_OUT_OF_MEMORY:
        return "Out of memory";
    case MICROBIT_SPIRV_ERROR_INVALID_ARGUMENT:
        return "Invalid argument";
    case MICROBIT_SPIRV_ERROR_NULL_POINTER:
        return "Null pointer";
    case MICROBIT_SPIRV_ERROR_INVALID_MAGIC:
        return "Invalid SPIR-V magic number";
    case MICROBIT_SPIRV_ERROR_INVALID_VERSION:
        return "Invalid SPIR-V version";
    case MICROBIT_SPIRV_ERROR_INVALID_DATA:
        return "Invalid SPIR-V data";
    case MICROBIT_SPIRV_ERROR_UNSUPPORTED_OPCODE:
        return "Unsupported opcode";
    case MICROBIT_SPIRV_ERROR_UNKNOWN:
    default:
        return "Unknown error";
    }
}

MICROBIT_SPIRV_API enum R_Microbit_SpirvParserError
R_Microbit_SpirvParserValidateHeader (const uint32_t* pData, size_t dataLength)
{
    MICROBIT_SPIRV_ASSERT (pData);
    MICROBIT_SPIRV_ASSERT (dataLength >= 5);

    uint32_t magic = pData[0];
    if (magic != MICROBIT_SPIRV_MAGIC_NUMBER)
    {
        R_CSTL_LOG_ERROR ("R_Microbit_SpirvParserValidateHeader: Invalid magic number");
        R_CSTL_LOG_ERROR ("  Expected: 0x%08X", MICROBIT_SPIRV_MAGIC_NUMBER);
        R_CSTL_LOG_ERROR ("  Found: 0x%08X", magic);
        return MICROBIT_SPIRV_ERROR_INVALID_MAGIC;
    }

    uint32_t version = pData[1];
    if (version != MICROBIT_SPIRV_VERSION_1_5)
    {
        R_CSTL_LOG_ERROR ("R_Microbit_SpirvParserValidateHeader: Unsupported version");
        R_CSTL_LOG_ERROR ("  Expected: 0x%08X", MICROBIT_SPIRV_VERSION_1_5);
        R_CSTL_LOG_ERROR ("  Found: 0x%08X", version);
        return MICROBIT_SPIRV_ERROR_INVALID_VERSION;
    }

    return MICROBIT_SPIRV_OK;
}

MICROBIT_SPIRV_API struct R_Microbit_SpirvParserContext*
R_Microbit_NewSpirvParserContext (void)
{
    struct R_Microbit_SpirvParserContext* pCtx = (struct R_Microbit_SpirvParserContext*)R_CSTL_HeapAlloc (
        sizeof (struct R_Microbit_SpirvParserContext));
    if (!pCtx)
    {
        return NULL;
    }

    memset (pCtx, 0, sizeof (struct R_Microbit_SpirvParserContext));

    return pCtx;
}

MICROBIT_SPIRV_API void
R_Microbit_DeleteSpirvParserContext (struct R_Microbit_SpirvParserContext* pCtx)
{
    MICROBIT_SPIRV_ASSERT (pCtx);
    R_CSTL_HeapFree (pCtx);
}

MICROBIT_SPIRV_API struct R_Microbit_SpirvParserProgram*
R_Microbit_NewSpirvParserProgram (
    struct R_Microbit_SpirvParserContext* pCtx,
    const uint32_t*                       pSpv,
    size_t                                spvLength)
{
    MICROBIT_SPIRV_ASSERT (pCtx);
    MICROBIT_SPIRV_ASSERT (pSpv);
    MICROBIT_SPIRV_ASSERT (spvLength >= 5);

    enum R_Microbit_SpirvParserError validation = R_Microbit_SpirvParserValidateHeader (pSpv, spvLength);
    if (validation != MICROBIT_SPIRV_OK)
    {
        return NULL;
    }

    struct R_Microbit_SpirvParserProgram* pProg = (struct R_Microbit_SpirvParserProgram*)R_CSTL_HeapAlloc (
        sizeof (struct R_Microbit_SpirvParserProgram));
    if (!pProg)
    {
        return NULL;
    }

    memset (pProg, 0, sizeof (struct R_Microbit_SpirvParserProgram));

    pProg->pContext = pCtx;

    const uint32_t* spvPtr = pSpv;
    MICROBIT_SPIRV_SKIP_WORD (spvPtr); // magic

    uint32_t version = MICROBIT_SPIRV_READ_WORD (spvPtr);
    pProg->majorVersion = (version & 0x00FF0000) >> 16;
    pProg->minorVersion = (version & 0x0000FF00) >> 8;

    uint32_t generator = MICROBIT_SPIRV_READ_WORD (spvPtr);
    pProg->generatorId = (generator & 0xFFFF0000) >> 16;
    pProg->generatorVersion = (generator & 0x0000FFFF);

    pProg->bound = MICROBIT_SPIRV_READ_WORD (spvPtr);

    MICROBIT_SPIRV_SKIP_WORD (spvPtr); // skip schema (reserved)
    pProg->codeLength = spvLength - 5;
    pProg->pCode = spvPtr;

    pProg->localSizeX = 1;
    pProg->localSizeY = 1;
    pProg->localSizeZ = 1;

    return pProg;
}

MICROBIT_SPIRV_API char*
R_Microbit_SpirvParserProgramAddExtension (struct R_Microbit_SpirvParserProgram* pProg, uint32_t length)
{
    MICROBIT_SPIRV_ASSERT (pProg);

    pProg->extensionCount++;
    pProg->ppExtensions
        = (char**)R_CSTL_HeapRealloc (pProg->ppExtensions, pProg->extensionCount * sizeof (char*));
    if (!pProg->ppExtensions)
    {
        return NULL;
    }

    char* pExt = (char*)R_CSTL_HeapAlloc (length * sizeof (uint32_t) + 1);
    if (!pExt)
    {
        return NULL;
    }

    pProg->ppExtensions[pProg->extensionCount - 1] = pExt;
    return pExt;
}

MICROBIT_SPIRV_API struct R_Microbit_SpirvParserEntryPoint*
R_Microbit_NewSpirvParserProgramEntryPoint (struct R_Microbit_SpirvParserProgram* pProg)
{
    MICROBIT_SPIRV_ASSERT (pProg);

    pProg->entryPointCount++;
    pProg->pEntryPoints = (struct R_Microbit_SpirvParserEntryPoint*)R_CSTL_HeapRealloc (
        pProg->pEntryPoints,
        pProg->entryPointCount * sizeof (struct R_Microbit_SpirvParserEntryPoint));
    if (!pProg->pEntryPoints)
    {
        return NULL;
    }

    return &pProg->pEntryPoints[pProg->entryPointCount - 1];
}

MICROBIT_SPIRV_API void
R_Microbit_SpirvParserProgramAddCapability (struct R_Microbit_SpirvParserProgram* pProg, uint32_t cap)
{
    MICROBIT_SPIRV_ASSERT (pProg);

    pProg->capabilityCount++;
    pProg->pCapabilities
        = (uint32_t*)R_CSTL_HeapRealloc (pProg->pCapabilities, pProg->capabilityCount * sizeof (uint32_t));
    if (pProg->pCapabilities)
    {
        pProg->pCapabilities[pProg->capabilityCount - 1] = cap;
    }
}

MICROBIT_SPIRV_API void
R_Microbit_DeleteSpirvParserProgram (struct R_Microbit_SpirvParserProgram* pProg)
{
    if (!pProg)
    {
        return;
    }

    for (uint32_t i = 0; i < pProg->entryPointCount; i++)
    {
        if (pProg->pEntryPoints[i].globalsCount > 0)
        {
            R_CSTL_HeapFree (pProg->pEntryPoints[i].pGlobals);
        }
        if (pProg->pEntryPoints[i].name)
        {
            R_CSTL_HeapFree (pProg->pEntryPoints[i].name);
        }
    }
    if (pProg->entryPointCount > 0)
    {
        R_CSTL_HeapFree (pProg->pEntryPoints);
    }

    for (uint32_t i = 0; i < pProg->extensionCount; i++)
    {
        if (pProg->ppExtensions[i])
        {
            R_CSTL_HeapFree (pProg->ppExtensions[i]);
        }
    }
    if (pProg->extensionCount > 0)
    {
        R_CSTL_HeapFree (pProg->ppExtensions);
    }

    if (pProg->capabilityCount > 0)
    {
        R_CSTL_HeapFree (pProg->pCapabilities);
    }

    if (pProg->fileCount > 0)
    {
        R_CSTL_HeapFree (pProg->pFiles);
    }

    for (uint32_t i = 0; i < pProg->importCount; i++)
    {
        if (pProg->ppImports[i])
        {
            R_CSTL_HeapFree (pProg->ppImports[i]);
        }
    }
    if (pProg->importCount > 0)
    {
        R_CSTL_HeapFree (pProg->ppImports);
    }

    R_CSTL_HeapFree (pProg);
}

MICROBIT_SPIRV_API struct R_Microbit_SpirvParserState*
R_Microbit_NewSpirvParserState (struct R_Microbit_SpirvParserProgram* pProg)
{
    MICROBIT_SPIRV_ASSERT (pProg);

    struct R_Microbit_SpirvParserState* pState
        = (struct R_Microbit_SpirvParserState*)R_CSTL_HeapAlloc (sizeof (struct R_Microbit_SpirvParserState));
    if (!pState)
    {
        return NULL;
    }

    memset (pState, 0, sizeof (struct R_Microbit_SpirvParserState));

    pState->pOwner = pProg;
    pState->pCodeCurrent = pProg->pCode;
    pState->pResults = (struct R_Microbit_SpirvParserResult*)R_CSTL_HeapAlloc (
        (pProg->bound + 1) * sizeof (struct R_Microbit_SpirvParserResult));
    if (!pState->pResults)
    {
        R_CSTL_HeapFree (pState);
        return NULL;
    }

    memset (pState->pResults, 0, (pProg->bound + 1) * sizeof (struct R_Microbit_SpirvParserResult));

    pState->pCurrentFile = NULL;
    pState->currentLine = (uint32_t)-1;
    pState->currentColumn = (uint32_t)-1;
    pState->currentParameter = 0;
    pState->returnId = (uint32_t)-1;
    pState->functionStackCount = 0;
    pState->functionStackCurrent = 0;
    pState->ppFunctionStack = NULL;
    pState->pContext = pProg->pContext;
    pState->pAnalyzer = NULL;
    pState->derivativeIsGroupMember = 0;

    for (size_t i = 0; i < pProg->bound; i++)
    {
        pState->pResults[i].storageClass = MICROBIT_SPIRV_STORAGE_CLASS_MAX;
    }

    const uint32_t* pCodeEnd = pProg->pCode + pProg->codeLength;
    while (pState->pCodeCurrent < pCodeEnd)
    {
        size_t remainingWords = (size_t)(pCodeEnd - pState->pCodeCurrent);
        MICROBIT_SPIRV_ASSERT (remainingWords > 0u);

        uint32_t        opcodeData = MICROBIT_SPIRV_READ_WORD (pState->pCodeCurrent);
        uint32_t        encodedWordCount = (opcodeData >> MICROBIT_SPIRV_WORD_COUNT_SHIFT) & 0xFFFFu;
        uint32_t        wordCount = encodedWordCount == 0u ? 0u : (encodedWordCount - 1u);
        uint32_t        opcode = (opcodeData & MICROBIT_SPIRV_OPCODE_MASK);
        const uint32_t* pCurCode = pState->pCodeCurrent;

        if (encodedWordCount == 0u || encodedWordCount > remainingWords)
        {
            R_CSTL_LOG_ERROR (
                "R_Microbit_SpirvParser_StateCreate: Invalid opcode stream; instruction truncated at offset "
                "%zu",
                (size_t)(pState->pCodeCurrent - pProg->pCode));
            break;
        }

        if (opcode < 0xFFFF && pState->pContext->pOpcodeSetup[opcode] != NULL)
        {
            pState->pContext->pOpcodeSetup[opcode](wordCount, pState);
        }

        pState->pCodeCurrent = (pCurCode + encodedWordCount);
    }

    return pState;
}

MICROBIT_SPIRV_API void
R_Microbit_DeleteSpirvParserState (struct R_Microbit_SpirvParserState* pState)
{
    if (!pState)
    {
        return;
    }

    if (pState->pResults)
    {
        for (uint32_t i = 0; i <= pState->pOwner->bound; i++)
        {
            if (pState->pResults[i].pName)
            {
                R_CSTL_HeapFree (pState->pResults[i].pName);
            }
            if (pState->pResults[i].memberNameCount > 0)
            {
                for (uint32_t j = 0; j < pState->pResults[i].memberNameCount; j++)
                {
                    if (pState->pResults[i].memberName[j])
                    {
                        R_CSTL_HeapFree (pState->pResults[i].memberName[j]);
                    }
                }
                R_CSTL_HeapFree (pState->pResults[i].memberName);
            }
            if (pState->pResults[i].pParams)
            {
                R_CSTL_HeapFree (pState->pResults[i].pParams);
            }
            if (pState->pResults[i].decorationCount > 0)
            {
                R_CSTL_HeapFree (pState->pResults[i].decorations);
            }
            if (pState->pResults[i].pImageInfo)
            {
                R_CSTL_HeapFree (pState->pResults[i].pImageInfo);
            }
            if (pState->pResults[i].pMembers)
            {
                R_CSTL_HeapFree (pState->pResults[i].pMembers);
            }
        }
        R_CSTL_HeapFree (pState->pResults);
    }

    if (pState->ppFunctionStack)
    {
        R_CSTL_HeapFree (pState->ppFunctionStack);
    }
    if (pState->ppFunctionStackInfo)
    {
        R_CSTL_HeapFree (pState->ppFunctionStackInfo);
    }
    if (pState->pFunctionStackReturns)
    {
        R_CSTL_HeapFree (pState->pFunctionStackReturns);
    }
    if (pState->pFunctionStackCfg)
    {
        R_CSTL_HeapFree (pState->pFunctionStackCfg);
    }
    if (pState->pFunctionStackCfgParent)
    {
        R_CSTL_HeapFree (pState->pFunctionStackCfgParent);
    }

    if (pState->pDerivativeGroupX)
    {
        R_Microbit_DeleteSpirvParserState (pState->pDerivativeGroupX);
    }
    if (pState->pDerivativeGroupY)
    {
        R_Microbit_DeleteSpirvParserState (pState->pDerivativeGroupY);
    }
    if (pState->pDerivativeGroupD)
    {
        R_Microbit_DeleteSpirvParserState (pState->pDerivativeGroupD);
    }

    R_CSTL_HeapFree (pState);
}

MICROBIT_SPIRV_API void
R_Microbit_SpirvParserStateSetExtension (
    struct R_Microbit_SpirvParserState* pState,
    const char*                         pName,
    void*                               pExt)
{
    MICROBIT_SPIRV_ASSERT (pState);
    MICROBIT_SPIRV_ASSERT (pName);

    struct R_Microbit_SpirvParserResult* pResult = R_Microbit_SpirvParserStateGetResult (pState, pName);
    if (pResult)
    {
        pResult->pExtension = pExt;
    }
}

MICROBIT_SPIRV_API void
R_Microbit_SpirvParserStatePrepare (struct R_Microbit_SpirvParserState* pState, uint32_t fnLocation)
{
    MICROBIT_SPIRV_ASSERT (pState);
    MICROBIT_SPIRV_ASSERT (fnLocation < pState->pOwner->bound);

    pState->pCodeCurrent = pState->pResults[fnLocation].pSourceLocation;
    pState->pCurrentFunction = &pState->pResults[fnLocation];

    if (pState->functionStackCount == 0)
    {
        pState->functionStackCount = 10;
        pState->ppFunctionStack
            = (const uint32_t**)R_CSTL_HeapAlloc (pState->functionStackCount * sizeof (const uint32_t*));
        pState->ppFunctionStackInfo = (struct R_Microbit_SpirvParserResult**)R_CSTL_HeapAlloc (
            pState->functionStackCount * sizeof (struct R_Microbit_SpirvParserResult*));
        pState->pFunctionStackReturns
            = (uint32_t*)R_CSTL_HeapAlloc (pState->functionStackCount * sizeof (uint32_t));
        pState->pFunctionStackCfg
            = (uint32_t*)R_CSTL_HeapAlloc (pState->functionStackCount * sizeof (uint32_t));
        pState->pFunctionStackCfgParent
            = (uint32_t*)R_CSTL_HeapAlloc (pState->functionStackCount * sizeof (uint32_t));
    }

    pState->functionStackCurrent = 0;
    pState->ppFunctionStack[0] = pState->pCodeCurrent;
    pState->ppFunctionStackInfo[0] = pState->pCurrentFunction;
    pState->pFunctionStackCfg[0] = 0;
    pState->pFunctionStackCfgParent[0] = 0;
    pState->didJump = 0;
    pState->discarded = 0;
    pState->instructionCount = 0;
}

MICROBIT_SPIRV_API void
R_Microbit_SpirvParserStateSetFragCoord (
    struct R_Microbit_SpirvParserState* pState,
    float                               x,
    float                               y,
    float                               z,
    float                               w)
{
    MICROBIT_SPIRV_ASSERT (pState);

    pState->fragCoord[0] = x;
    pState->fragCoord[1] = y;
    pState->fragCoord[2] = z;
    pState->fragCoord[3] = w;
}

MICROBIT_SPIRV_API void
R_Microbit_SpirvParserStateStepOpcode (struct R_Microbit_SpirvParserState* pState)
{
    MICROBIT_SPIRV_ASSERT (pState);
    MICROBIT_SPIRV_ASSERT (pState->pCodeCurrent);

    uint32_t opcodeData = MICROBIT_SPIRV_READ_WORD (pState->pCodeCurrent);
    uint32_t wordCount
        = ((opcodeData & (~MICROBIT_SPIRV_OPCODE_MASK)) >> MICROBIT_SPIRV_WORD_COUNT_SHIFT) - 1;
    uint32_t        opcode = (opcodeData & MICROBIT_SPIRV_OPCODE_MASK);
    const uint32_t* pCurCode = pState->pCodeCurrent;

    if (opcode < 0xFFFF && pState->pContext->pOpcodeExecute[opcode] != NULL)
    {
        pState->pContext->pOpcodeExecute[opcode](wordCount, pState);
        if (opcode != MICROBIT_SPIRV_OP_LINE && opcode != MICROBIT_SPIRV_OP_NO_LINE)
        {
            pState->instructionCount++;
        }
    }

    if (!pState->didJump)
    {
        pState->pCodeCurrent = (pCurCode + wordCount);
    }
    else
    {
        pState->didJump = 0;
    }
}

MICROBIT_SPIRV_API void
R_Microbit_SpirvParserStateStepInto (struct R_Microbit_SpirvParserState* pState)
{
    MICROBIT_SPIRV_ASSERT (pState);

    uint32_t ln = pState->currentLine;
    while (ln == pState->currentLine && pState->pCodeCurrent)
    {
        R_Microbit_SpirvParserStateStepOpcode (pState);
    }
}

MICROBIT_SPIRV_API void
R_Microbit_SpirvParserStateJumpTo (struct R_Microbit_SpirvParserState* pState, uint32_t line)
{
    MICROBIT_SPIRV_ASSERT (pState);

    while (line != pState->currentLine && pState->pCodeCurrent)
    {
        R_Microbit_SpirvParserStateStepOpcode (pState);
    }
}

MICROBIT_SPIRV_API void
R_Microbit_SpirvParserStateJumpToInstruction (struct R_Microbit_SpirvParserState* pState, uint32_t inst)
{
    MICROBIT_SPIRV_ASSERT (pState);

    while (pState->instructionCount < inst && pState->pCodeCurrent)
    {
        R_Microbit_SpirvParserStateStepOpcode (pState);
    }
}

MICROBIT_SPIRV_API void
R_Microbit_SpirvParserStateCallFunction (struct R_Microbit_SpirvParserState* pState)
{
    MICROBIT_SPIRV_ASSERT (pState);
    MICROBIT_SPIRV_ASSERT (pState->pCodeCurrent);

    const uint32_t* pCurCode = pState->pCodeCurrent;

    while (pState->pCodeCurrent)
    {
        uint32_t opcodeData = MICROBIT_SPIRV_READ_WORD (pState->pCodeCurrent);
        uint32_t wordCount
            = ((opcodeData & (~MICROBIT_SPIRV_OPCODE_MASK)) >> MICROBIT_SPIRV_WORD_COUNT_SHIFT) - 1;
        uint32_t opcode = (opcodeData & MICROBIT_SPIRV_OPCODE_MASK);
        pCurCode = pState->pCodeCurrent;

        if (opcode < 0xFFFF && pState->pContext->pOpcodeExecute[opcode] != NULL)
        {
            pState->pContext->pOpcodeExecute[opcode](wordCount, pState);
            if (opcode != MICROBIT_SPIRV_OP_LINE && opcode != MICROBIT_SPIRV_OP_NO_LINE)
            {
                pState->instructionCount++;
            }
        }

        if (!pState->didJump)
        {
            pState->pCodeCurrent = (pCurCode + wordCount);
        }
        else
        {
            pState->didJump = 0;
        }
    }
}

MICROBIT_SPIRV_API uint32_t
R_Microbit_SpirvParserStateGetResultLocation (struct R_Microbit_SpirvParserState* pState, const char* pName)
{
    MICROBIT_SPIRV_ASSERT (pState);
    MICROBIT_SPIRV_ASSERT (pName);

    for (uint32_t i = 0; i < pState->pOwner->bound; i++)
    {
        if (pState->pResults[i].pName && strcmp (pState->pResults[i].pName, pName) == 0)
        {
            return i;
        }
    }

    return (uint32_t)-1;
}

MICROBIT_SPIRV_API struct R_Microbit_SpirvParserResult*
R_Microbit_SpirvParserStateGetResult (struct R_Microbit_SpirvParserState* pState, const char* pName)
{
    MICROBIT_SPIRV_ASSERT (pState);
    MICROBIT_SPIRV_ASSERT (pName);

    uint32_t location = R_Microbit_SpirvParserStateGetResultLocation (pState, pName);
    if (location == (uint32_t)-1)
    {
        return NULL;
    }

    return &pState->pResults[location];
}

MICROBIT_SPIRV_API struct R_Microbit_SpirvParserResult*
R_Microbit_SpirvParserStateGetResultWithValue (struct R_Microbit_SpirvParserState* pState, const char* pName)
{
    return R_Microbit_SpirvParserStateGetResult (pState, pName);
}

MICROBIT_SPIRV_API struct R_Microbit_SpirvParserResult*
R_Microbit_SpirvParserStateGetLocalResult (
    struct R_Microbit_SpirvParserState*  pState,
    struct R_Microbit_SpirvParserResult* pFn,
    const char*                          pName)
{
    MICROBIT_SPIRV_ASSERT (pState);
    MICROBIT_SPIRV_ASSERT (pFn);
    MICROBIT_SPIRV_ASSERT (pName);

    for (uint32_t i = 0; i < pState->pOwner->bound; i++)
    {
        if (pState->pResults[i].pName && strcmp (pState->pResults[i].pName, pName) == 0)
        {
            if (pState->pResults[i].owner == pFn)
            {
                return &pState->pResults[i];
            }
        }
    }

    return NULL;
}

MICROBIT_SPIRV_API struct R_Microbit_SpirvParserMember*
R_Microbit_SpirvParserStateGetObjectMember (
    struct R_Microbit_SpirvParserState*  pState,
    struct R_Microbit_SpirvParserResult* pVar,
    const char*                          pMemberName)
{
    MICROBIT_SPIRV_ASSERT (pState);
    MICROBIT_SPIRV_ASSERT (pVar);
    MICROBIT_SPIRV_ASSERT (pMemberName);

    for (uint32_t i = 0; i < pVar->memberNameCount; i++)
    {
        if (pVar->memberName[i] && strcmp (pVar->memberName[i], pMemberName) == 0)
        {
            return &pVar->pMembers[i];
        }
    }
    return NULL;
}
