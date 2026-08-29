#pragma once

#include "microbit/microbit_spirv_parser.h"

enum R_Microbit_SpirvRunnerError
{
    MICROBIT_SPIRV_RUNNER_OK = 0,
    MICROBIT_SPIRV_RUNNER_ERROR_FAILED = -1,
    MICROBIT_SPIRV_RUNNER_ERROR_OUT_OF_MEMORY = -2,
    MICROBIT_SPIRV_RUNNER_ERROR_INVALID_ARGUMENT = -3,
    MICROBIT_SPIRV_RUNNER_ERROR_NULL_POINTER = -4,
    MICROBIT_SPIRV_RUNNER_ERROR_INVALID_STATE = -5,
    MICROBIT_SPIRV_RUNNER_ERROR_INVALID_PROGRAM = -6,
    MICROBIT_SPIRV_RUNNER_ERROR_UNSUPPORTED_OPCODE = -7,
    MICROBIT_SPIRV_RUNNER_ERROR_UNKNOWN = -99
};

R_MICROBIT_API const char*
R_Microbit_SpirvRunnerErrorToString (enum R_Microbit_SpirvRunnerError error);

struct R_Microbit_SpirvRunnerContext
{
        struct R_Microbit_SpirvParserContext* pParserContext;
        uint32_t                              maxInstructions;
        uint32_t                              instructionLimit;
};

struct R_Microbit_SpirvRunnerProgram
{
        struct R_Microbit_SpirvParserProgram* pProgram;
        struct R_Microbit_SpirvRunnerContext* pContext;
        const uint32_t*                       pEntryCode;
        uint32_t                              entryPoint;
};

struct R_Microbit_SpirvRunnerExecution
{
        struct R_Microbit_SpirvParserState*   pState;
        struct R_Microbit_SpirvRunnerContext* pContext;
        const uint32_t*                       pCodeBegin;
        const uint32_t*                       pCodeEnd;
        uint32_t                              instructionCount;
        uint32_t                              stepLimit;
        uint32_t                              lastOpcode;
        enum R_Microbit_SpirvRunnerError      lastError;
        uint8_t                               didComplete;
};

R_MICROBIT_API struct R_Microbit_SpirvRunnerContext* R_Microbit_NewSpirvRunnerContext (void);

R_MICROBIT_API void
R_Microbit_DeleteSpirvRunnerContext (struct R_Microbit_SpirvRunnerContext* pContext);

R_MICROBIT_API enum R_Microbit_SpirvRunnerError R_Microbit_NewSpirvRunnerProgram (
    struct R_Microbit_SpirvRunnerContext*  pContext,
    struct R_Microbit_SpirvParserProgram*  pProgram,
    uint32_t                               entryPoint,
    struct R_Microbit_SpirvRunnerProgram** ppRunnerProgram);

R_MICROBIT_API void
R_Microbit_DeleteSpirvRunnerProgram (struct R_Microbit_SpirvRunnerProgram* pRunnerProgram);

R_MICROBIT_API enum R_Microbit_SpirvRunnerError R_Microbit_NewSpirvRunnerExecution (
    struct R_Microbit_SpirvRunnerProgram*    pRunnerProgram,
    struct R_Microbit_SpirvRunnerExecution** ppExecution);

R_MICROBIT_API void
R_Microbit_DeleteSpirvRunnerExecution (struct R_Microbit_SpirvRunnerExecution* pExecution);

R_MICROBIT_API enum R_Microbit_SpirvRunnerError
R_Microbit_SpirvRunnerExecute (struct R_Microbit_SpirvRunnerExecution* pExecution, uint32_t maxInstructions);

R_MICROBIT_API enum R_Microbit_SpirvRunnerError
R_Microbit_SpirvRunnerStep (struct R_Microbit_SpirvRunnerExecution* pExecution);
