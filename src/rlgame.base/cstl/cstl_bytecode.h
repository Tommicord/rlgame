#pragma once

#include "rlgame.base/cstl/cstl_platform.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

struct r_cstl_mutex;

enum r_cstl_bytecode_architecture
{
    R_CSTL_BYTECODE_ARCH_X86 = 0,
    R_CSTL_BYTECODE_ARCH_X86_64,
    R_CSTL_BYTECODE_ARCH_ARMV8A,
    R_CSTL_BYTECODE_ARCH_ARMEABI_V7A,
    R_CSTL_BYTECODE_ARCH_RISC,
};

enum r_cstl_bytecode_token_kind
{
    R_CSTL_BYTECODE_TOKEN_OPCODE = 0,
    R_CSTL_BYTECODE_TOKEN_OPERAND,
    R_CSTL_BYTECODE_TOKEN_REGISTER,
    R_CSTL_BYTECODE_TOKEN_IMMEDIATE,
    R_CSTL_BYTECODE_TOKEN_RELATIVE_ADDRESS,
    R_CSTL_BYTECODE_TOKEN_RIP_ADDRESSING,
    R_CSTL_BYTECODE_TOKEN_ABSOLUTE_ADDRESS,
    R_CSTL_BYTECODE_TOKEN_UNKNOWN,
};

struct r_cstl_bytecode;

struct r_cstl_bytecode_token
{
        enum r_cstl_bytecode_token_kind kind;
        size_t                        offset;
        uint8_t                       size;
        uint64_t                      value;
};

struct r_cstl_bytecode_instruction
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

typedef void (*r_cstl_bytecode_function) (void);

struct r_cstl_bytecode_symbol
{
        uint64_t    address;
        const char* pName;
        size_t      nameSize;
        uint64_t    size;
};

struct r_cstl_bytecode_decoder
{
        void*                            pPlatformHandle;
        bool                             initialized;
        enum r_cstl_bytecode_architecture architecture;
};

struct r_cstl_bytecode_function_info
{
        uint64_t    startAddress;
        uint64_t    endAddress;
        const char* pName;
        size_t      nameSize;
};

/** @brief Create a non-owning view of machine-code bytes. */
R_CSTL_API struct r_cstl_bytecode*
r_cstl_new_bytecode_view (const void* pCode, size_t sizeBytes, enum r_cstl_bytecode_architecture architecture);

/** @brief Copy machine-code bytes into an owned inspection buffer. */
R_CSTL_API struct r_cstl_bytecode* r_cstl_new_bytecode_with_data (
    const uint8_t*                   pCode,
    size_t                           sizeBytes,
    enum r_cstl_bytecode_architecture architecture);

/** @brief Create a non-owning view starting at a function address. */
R_CSTL_API struct r_cstl_bytecode* r_cstl_new_bytecode_from_function (
    r_cstl_bytecode_function          pFunction,
    size_t                           sizeBytes,
    enum r_cstl_bytecode_architecture architecture);

R_CSTL_API void r_cstl_delete_bytecode (struct r_cstl_bytecode* pBytecode);

/** @brief Read bytes from the bounded machine-code view. */
R_CSTL_API int r_cstl_bytecode_read (
    const struct r_cstl_bytecode* pBytecode,
    size_t                        offset,
    uint8_t*                      pOutBytes,
    size_t                        sizeBytes);

/** @brief Parse one instruction at offset. */
R_CSTL_API int r_cstl_bytecode_parse (
    const struct r_cstl_bytecode*      pBytecode,
    size_t                             offset,
    struct r_cstl_bytecode_instruction* pOutInstruction);

/** @brief Tokenize one instruction into a caller-provided token buffer. */
R_CSTL_API int r_cstl_bytecode_tokenize (
    const struct r_cstl_bytecode* pBytecode,
    size_t                        offset,
    struct r_cstl_bytecode_token*  pTokens,
    size_t                        tokenCapacity,
    size_t*                       pOutTokenCount);

R_CSTL_API const uint8_t* r_cstl_bytecode_data (const struct r_cstl_bytecode* pBytecode);
R_CSTL_API size_t         r_cstl_bytecode_length (const struct r_cstl_bytecode* pBytecode);
R_CSTL_API enum r_cstl_bytecode_architecture
r_cstl_bytecode_get_architecture (const struct r_cstl_bytecode* pBytecode);

#if defined(R_LOG)

/** @brief Create a machine code decoder for symbol resolution */
R_CSTL_API int r_cstl_bytecode_decoder_create (
    enum r_cstl_bytecode_architecture architecture,
    struct r_cstl_bytecode_decoder*   pOutDecoder);

/** @brief Destroy a machine code decoder and release resources */
R_CSTL_API void r_cstl_delete_bytecode_decoder (struct r_cstl_bytecode_decoder* pDecoder);

/** @brief Resolve a symbol name from an address */
R_CSTL_API int r_cstl_bytecode_resolve_symbol (
    const struct r_cstl_bytecode_decoder* pDecoder,
    uint64_t                             address,
    struct r_cstl_bytecode_symbol*        pOutSymbol);

/** @brief Get function information for a given address */
R_CSTL_API int r_cstl_bytecode_get_function_info (
    const struct r_cstl_bytecode_decoder* pDecoder,
    uint64_t                             address,
    struct r_cstl_bytecode_function_info*  pOutInfo);

/** @brief Check if a function contains a call to a specific symbol */
R_CSTL_API int r_cstl_bytecode_function_contains_symbol (
    const struct r_cstl_bytecode_decoder* pDecoder,
    r_cstl_bytecode_function              pFunction,
    size_t                               functionSize,
    const char*                          pSymbolName,
    int*                                 pOutFound);

/** @brief Parse instruction with enhanced CALL/JMP target extraction */
R_CSTL_API int r_cstl_bytecode_parse_enhanced (
    const struct r_cstl_bytecode*      pBytecode,
    size_t                             offset,
    struct r_cstl_bytecode_instruction* pOutInstruction);

/** @brief Get symbol name for an instruction's target address */
R_CSTL_API int r_cstl_bytecode_get_instruction_target_symbol (
    const struct r_cstl_bytecode_decoder*     pDecoder,
    const struct r_cstl_bytecode_instruction* pInstruction,
    char*                                    pOutBuffer,
    size_t                                   bufferSize);

#endif
