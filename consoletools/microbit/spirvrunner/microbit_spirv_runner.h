#pragma once

#include "microbit/microbit_spirv_parser.h"

#if defined(_WIN32)
#ifdef R_MICROBIT_SPIRV_RUNNER_BUILDING_DLL
#define MICROBIT_SPIRV_RUNNER_API __declspec (dllexport)
#else
#define MICROBIT_SPIRV_RUNNER_API __declspec (dllimport)
#endif
#else
#define MICROBIT_SPIRV_RUNNER_API
#endif

#if defined(R_DEVMODE)
#define MICROBIT_SPIRV_RUNNER_DEBUG
#endif

#if defined(MICROBIT_SPIRV_RUNNER_DEBUG)
#include <assert.h>
#define MICROBIT_SPIRV_RUNNER_ASSERT(condition) assert (condition)
#else
#define MICROBIT_SPIRV_RUNNER_ASSERT(condition) ((void)0)
#endif

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

MICROBIT_SPIRV_RUNNER_API const char*
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

MICROBIT_SPIRV_RUNNER_API struct R_Microbit_SpirvRunnerContext* R_Microbit_NewSpirvRunnerContext (void);

MICROBIT_SPIRV_RUNNER_API void
R_Microbit_DeleteSpirvRunnerContext (struct R_Microbit_SpirvRunnerContext* pContext);

MICROBIT_SPIRV_RUNNER_API enum R_Microbit_SpirvRunnerError R_Microbit_NewSpirvRunnerProgram (
    struct R_Microbit_SpirvRunnerContext*  pContext,
    struct R_Microbit_SpirvParserProgram*  pProgram,
    uint32_t                               entryPoint,
    struct R_Microbit_SpirvRunnerProgram** ppRunnerProgram);

MICROBIT_SPIRV_RUNNER_API void
R_Microbit_DeleteSpirvRunnerProgram (struct R_Microbit_SpirvRunnerProgram* pRunnerProgram);

MICROBIT_SPIRV_RUNNER_API enum R_Microbit_SpirvRunnerError R_Microbit_NewSpirvRunnerExecution (
    struct R_Microbit_SpirvRunnerProgram*    pRunnerProgram,
    struct R_Microbit_SpirvRunnerExecution** ppExecution);

MICROBIT_SPIRV_RUNNER_API void
R_Microbit_DeleteSpirvRunnerExecution (struct R_Microbit_SpirvRunnerExecution* pExecution);

MICROBIT_SPIRV_RUNNER_API enum R_Microbit_SpirvRunnerError
R_Microbit_SpirvRunnerExecute (struct R_Microbit_SpirvRunnerExecution* pExecution, uint32_t maxInstructions);

MICROBIT_SPIRV_RUNNER_API enum R_Microbit_SpirvRunnerError
R_Microbit_SpirvRunnerStep (struct R_Microbit_SpirvRunnerExecution* pExecution);
