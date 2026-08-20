#pragma once

#include "rlgame.base/cstl/cstl_platform.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

enum R_CSTL_BytecodeArchitecture
{
        R_CSTL_MACHINE_CODE_ARCH_X86 = 0,
        R_CSTL_MACHINE_CODE_ARCH_X86_64,
        R_CSTL_MACHINE_CODE_ARCH_ARMV8A,
        R_CSTL_MACHINE_CODE_ARCH_ARMEABI_V7A,
        R_CSTL_MACHINE_CODE_ARCH_RISC,
};

enum R_CSTL_BytecodeTokenKind
{
        R_CSTL_MACHINE_CODE_TOKEN_OPCODE = 0,
        R_CSTL_MACHINE_CODE_TOKEN_OPERAND,
        R_CSTL_MACHINE_CODE_TOKEN_REGISTER,
        R_CSTL_MACHINE_CODE_TOKEN_IMMEDIATE,
        R_CSTL_MACHINE_CODE_TOKEN_RELATIVE_ADDRESS,
        R_CSTL_MACHINE_CODE_TOKEN_RIP_ADDRESSING,
        R_CSTL_MACHINE_CODE_TOKEN_ABSOLUTE_ADDRESS,
        R_CSTL_MACHINE_CODE_TOKEN_UNKNOWN,
};

struct R_CSTL_Bytecode;

struct R_CSTL_BytecodeToken
{
                enum R_CSTL_BytecodeTokenKind kind;
                size_t                           offset;
                uint8_t                          size;
                uint64_t                         value;
};

struct R_CSTL_BytecodeInstruction
{
                size_t   offset;
                uint8_t  size;
                uint8_t  opcodeSize;
                uint8_t  operandCount;
                uint8_t  bytes[16];
                uint64_t opcode;
                uint64_t targetAddress;
                uint8_t  isCall;
                uint8_t  isJump;
                uint8_t  rexPrefix;
                uint8_t  legacyPrefixes;
                uint8_t  hasRex;
                uint8_t  rexW;
                uint8_t  rexR;
                uint8_t  rexX;
                uint8_t  rexB;
};

typedef void (*R_CSTL_BytecodeFunction) (void);

struct R_CSTL_BytecodeSymbol
{
                uint64_t    address;
                const char* pName;
                size_t      nameSize;
                uint64_t    size;
};

struct R_CSTL_BytecodeDecoder
{
                void*                               pPlatformHandle;
                bool                                initialized;
                enum R_CSTL_BytecodeArchitecture architecture;
};

struct R_CSTL_BytecodeFunctionInfo
{
                uint64_t    startAddress;
                uint64_t    endAddress;
                const char* pName;
                size_t      nameSize;
};

/** @brief Create a non-owning view of machine-code bytes. */
R_CSTL_API struct R_CSTL_Bytecode* R_CSTL_NewBytecodeView (
    const void*                         pCode,
    size_t                              sizeBytes,
    enum R_CSTL_BytecodeArchitecture architecture);

/** @brief Copy machine-code bytes into an owned inspection buffer. */
R_CSTL_API struct R_CSTL_Bytecode* R_CSTL_NewBytecodeWithData (
    const uint8_t*                      pCode,
    size_t                              sizeBytes,
    enum R_CSTL_BytecodeArchitecture architecture);

/** @brief Create a non-owning view starting at a function address. */
R_CSTL_API struct R_CSTL_Bytecode* R_CSTL_NewBytecodeFromFunction (
    R_CSTL_BytecodeFunction          pFunction,
    size_t                              sizeBytes,
    enum R_CSTL_BytecodeArchitecture architecture);

R_CSTL_API void R_CSTL_DeleteBytecode (struct R_CSTL_Bytecode* pBytecode);

/** @brief Read bytes from the bounded machine-code view. */
R_CSTL_API int R_CSTL_BytecodeRead (
    const struct R_CSTL_Bytecode* pBytecode,
    size_t                           offset,
    uint8_t*                         pOutBytes,
    size_t                           sizeBytes);

/** @brief Parse one instruction at offset. */
R_CSTL_API int R_CSTL_BytecodeParse (
    const struct R_CSTL_Bytecode*      pBytecode,
    size_t                                offset,
    struct R_CSTL_BytecodeInstruction* pOutInstruction);

/** @brief Tokenize one instruction into a caller-provided token buffer. */
R_CSTL_API int R_CSTL_BytecodeTokenize (
    const struct R_CSTL_Bytecode* pBytecode,
    size_t                           offset,
    struct R_CSTL_BytecodeToken*  pTokens,
    size_t                           tokenCapacity,
    size_t*                          pOutTokenCount);

R_CSTL_API const uint8_t* R_CSTL_BytecodeData (const struct R_CSTL_Bytecode* pBytecode);
R_CSTL_API size_t         R_CSTL_BytecodeLength (const struct R_CSTL_Bytecode* pBytecode);
R_CSTL_API enum R_CSTL_BytecodeArchitecture
R_CSTL_BytecodeGetArchitecture (const struct R_CSTL_Bytecode* pBytecode);

#if defined(R_CSTL_LOG_DEVMODE)

/** @brief Create a machine code decoder for symbol resolution */
R_CSTL_API int R_CSTL_BytecodeDecoderCreate (
    enum R_CSTL_BytecodeArchitecture architecture,
    struct R_CSTL_BytecodeDecoder*   pOutDecoder);

/** @brief Destroy a machine code decoder and release resources */
R_CSTL_API void R_CSTL_BytecodeDecoderDestroy (struct R_CSTL_BytecodeDecoder* pDecoder);

/** @brief Resolve a symbol name from an address */
R_CSTL_API int R_CSTL_BytecodeResolveSymbol (
    const struct R_CSTL_BytecodeDecoder* pDecoder,
    uint64_t                                address,
    struct R_CSTL_BytecodeSymbol*        pOutSymbol);

/** @brief Get function information for a given address */
R_CSTL_API int R_CSTL_BytecodeGetFunctionInfo (
    const struct R_CSTL_BytecodeDecoder* pDecoder,
    uint64_t                                address,
    struct R_CSTL_BytecodeFunctionInfo*  pOutInfo);

/** @brief Check if a function contains a call to a specific symbol */
R_CSTL_API int R_CSTL_BytecodeFunctionContainsSymbol (
    const struct R_CSTL_BytecodeDecoder* pDecoder,
    R_CSTL_BytecodeFunction              pFunction,
    size_t                                  functionSize,
    const char*                             pSymbolName,
    int*                                    pOutFound);

/** @brief Parse instruction with enhanced CALL/JMP target extraction */
R_CSTL_API int R_CSTL_BytecodeParseEnhanced (
    const struct R_CSTL_Bytecode*      pBytecode,
    size_t                                offset,
    struct R_CSTL_BytecodeInstruction* pOutInstruction);

/** @brief Get symbol name for an instruction's target address */
R_CSTL_API int R_CSTL_BytecodeGetInstructionTargetSymbol (
    const struct R_CSTL_BytecodeDecoder*     pDecoder,
    const struct R_CSTL_BytecodeInstruction* pInstruction,
    char*                                       pOutBuffer,
    size_t                                      bufferSize);

#endif
