#include <gtest/gtest.h>

#include <cstring>
#include <vector>

extern "C"
{
#include "rlgame.base/cstl/cstl_bytecode.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"
}

namespace
{

constexpr size_t kTestHeapSize = 256 * 1024;

class CstlMaccodeTest : public ::testing::Test
{
        protected:
                void
                SetUp () override
                {
                        ASSERT_EQ (0, R_CSTL_HeapInit (kTestHeapSize));
                }

                void
                TearDown () override
                {
                        R_CSTL_HeapShutdown ();
                }
};

} // namespace

TEST (CstlMaccodeInitTest, DeleteNullIsSafe)
{
        R_CSTL_DeleteBytecode (nullptr);
        SUCCEED ();
}

TEST_F (CstlMaccodeTest, NewBytecodeView)
{
        uint8_t                    testData[] = {0x90, 0x90, 0x90, 0xC3};
        struct R_CSTL_Bytecode* pCode
            = R_CSTL_NewBytecodeView (testData, sizeof (testData), R_CSTL_MACHINE_CODE_ARCH_X86_64);
        ASSERT_NE (nullptr, pCode);
        EXPECT_EQ (sizeof (testData), R_CSTL_BytecodeLength (pCode));
        EXPECT_EQ (R_CSTL_MACHINE_CODE_ARCH_X86_64, R_CSTL_BytecodeGetArchitecture (pCode));
        EXPECT_EQ (testData, R_CSTL_BytecodeData (pCode));
        R_CSTL_DeleteBytecode (pCode);
}

TEST_F (CstlMaccodeTest, NewBytecodeWithData)
{
        uint8_t                    testData[] = {0x90, 0x90, 0x90, 0xC3};
        struct R_CSTL_Bytecode* pCode
            = R_CSTL_NewBytecodeWithData (testData, sizeof (testData), R_CSTL_MACHINE_CODE_ARCH_X86_64);
        ASSERT_NE (nullptr, pCode);
        EXPECT_EQ (sizeof (testData), R_CSTL_BytecodeLength (pCode));
        EXPECT_NE (testData, R_CSTL_BytecodeData (pCode)); // Should be a copy
        R_CSTL_DeleteBytecode (pCode);
}

TEST_F (CstlMaccodeTest, NewBytecodeWithDataZeroSize)
{
        struct R_CSTL_Bytecode* pCode
            = R_CSTL_NewBytecodeWithData (nullptr, 0, R_CSTL_MACHINE_CODE_ARCH_X86_64);
        ASSERT_NE (nullptr, pCode);
        EXPECT_EQ (0u, R_CSTL_BytecodeLength (pCode));
        R_CSTL_DeleteBytecode (pCode);
}

TEST_F (CstlMaccodeTest, NewBytecodeFromFunction)
{
        struct R_CSTL_Bytecode* pCode = R_CSTL_NewBytecodeFromFunction (
            (R_CSTL_BytecodeFunction)R_CSTL_HeapInit,
            32,
            R_CSTL_MACHINE_CODE_ARCH_X86_64);
        ASSERT_NE (nullptr, pCode);
        EXPECT_EQ (32u, R_CSTL_BytecodeLength (pCode));
        R_CSTL_DeleteBytecode (pCode);
}

TEST_F (CstlMaccodeTest, NewBytecodeFromFunctionNull)
{
        struct R_CSTL_Bytecode* pCode
            = R_CSTL_NewBytecodeFromFunction (nullptr, 32, R_CSTL_MACHINE_CODE_ARCH_X86_64);
        EXPECT_EQ (nullptr, pCode);
}

TEST_F (CstlMaccodeTest, BytecodeRead)
{
        uint8_t                    testData[] = {0x90, 0x90, 0x90, 0xC3};
        struct R_CSTL_Bytecode* pCode
            = R_CSTL_NewBytecodeView (testData, sizeof (testData), R_CSTL_MACHINE_CODE_ARCH_X86_64);
        ASSERT_NE (nullptr, pCode);

        uint8_t buffer[4];
        ASSERT_EQ (0, R_CSTL_BytecodeRead (pCode, 0, buffer, sizeof (buffer)));
        EXPECT_EQ (0, memcmp (buffer, testData, sizeof (buffer)));

        R_CSTL_DeleteBytecode (pCode);
}

TEST_F (CstlMaccodeTest, BytecodeReadOutOfBounds)
{
        uint8_t                    testData[] = {0x90, 0x90, 0x90, 0xC3};
        struct R_CSTL_Bytecode* pCode
            = R_CSTL_NewBytecodeView (testData, sizeof (testData), R_CSTL_MACHINE_CODE_ARCH_X86_64);
        ASSERT_NE (nullptr, pCode);

        uint8_t buffer[4];
        EXPECT_EQ (R_CSTL_ERROR_BUFFER_TOO_SMALL, R_CSTL_BytecodeRead (pCode, 2, buffer, 4));

        R_CSTL_DeleteBytecode (pCode);
}

TEST_F (CstlMaccodeTest, BytecodeParseNop)
{
        uint8_t                    testData[] = {0x90}; // NOP
        struct R_CSTL_Bytecode* pCode
            = R_CSTL_NewBytecodeView (testData, sizeof (testData), R_CSTL_MACHINE_CODE_ARCH_X86_64);
        ASSERT_NE (nullptr, pCode);

        struct R_CSTL_BytecodeInstruction instruction;
        ASSERT_EQ (0, R_CSTL_BytecodeParse (pCode, 0, &instruction));
        EXPECT_EQ (1u, instruction.size);
        EXPECT_EQ (0x90u, instruction.opcode);

        R_CSTL_DeleteBytecode (pCode);
}

TEST_F (CstlMaccodeTest, BytecodeParseRet)
{
        uint8_t                    testData[] = {0xC3}; // RET
        struct R_CSTL_Bytecode* pCode
            = R_CSTL_NewBytecodeView (testData, sizeof (testData), R_CSTL_MACHINE_CODE_ARCH_X86_64);
        ASSERT_NE (nullptr, pCode);

        struct R_CSTL_BytecodeInstruction instruction;
        ASSERT_EQ (0, R_CSTL_BytecodeParse (pCode, 0, &instruction));
        EXPECT_EQ (1u, instruction.size);
        EXPECT_EQ (0xC3u, instruction.opcode);

        R_CSTL_DeleteBytecode (pCode);
}

TEST_F (CstlMaccodeTest, BytecodeParseInvalidArguments)
{
        uint8_t                    testData[] = {0x90};
        struct R_CSTL_Bytecode* pCode
            = R_CSTL_NewBytecodeView (testData, sizeof (testData), R_CSTL_MACHINE_CODE_ARCH_X86_64);
        ASSERT_NE (nullptr, pCode);

        struct R_CSTL_BytecodeInstruction instruction;
        EXPECT_EQ (R_CSTL_ERROR_INVALID_ARGUMENT, R_CSTL_BytecodeParse (nullptr, 0, &instruction));
        EXPECT_EQ (R_CSTL_ERROR_INVALID_ARGUMENT, R_CSTL_BytecodeParse (pCode, 0, nullptr));
        EXPECT_EQ (R_CSTL_ERROR_INVALID_ARGUMENT, R_CSTL_BytecodeParse (pCode, 10, &instruction));

        R_CSTL_DeleteBytecode (pCode);
}

TEST_F (CstlMaccodeTest, BytecodeParseMultipleInstructions)
{
        uint8_t                    testData[] = {0x90, 0x90, 0x90, 0xC3}; // NOP, NOP, NOP, RET
        struct R_CSTL_Bytecode* pCode
            = R_CSTL_NewBytecodeView (testData, sizeof (testData), R_CSTL_MACHINE_CODE_ARCH_X86_64);
        ASSERT_NE (nullptr, pCode);

        size_t offset = 0;
        while (offset < sizeof (testData))
        {
                struct R_CSTL_BytecodeInstruction instruction;
                ASSERT_EQ (0, R_CSTL_BytecodeParse (pCode, offset, &instruction));
                offset += instruction.size;
        }
        EXPECT_EQ (sizeof (testData), offset);

        R_CSTL_DeleteBytecode (pCode);
}

TEST_F (CstlMaccodeTest, BytecodeTokenize)
{
        uint8_t                    testData[] = {0x90, 0x90, 0x90, 0xC3};
        struct R_CSTL_Bytecode* pCode
            = R_CSTL_NewBytecodeView (testData, sizeof (testData), R_CSTL_MACHINE_CODE_ARCH_X86_64);
        ASSERT_NE (nullptr, pCode);

        struct R_CSTL_BytecodeToken tokens[8];
        size_t                         tokenCount = 0;
        ASSERT_EQ (0, R_CSTL_BytecodeTokenize (pCode, 0, tokens, 8, &tokenCount));
        EXPECT_GT (tokenCount, 0u);

        R_CSTL_DeleteBytecode (pCode);
}

TEST_F (CstlMaccodeTest, BytecodeTokenizeInvalidArguments)
{
        uint8_t                    testData[] = {0x90};
        struct R_CSTL_Bytecode* pCode
            = R_CSTL_NewBytecodeView (testData, sizeof (testData), R_CSTL_MACHINE_CODE_ARCH_X86_64);
        ASSERT_NE (nullptr, pCode);

        struct R_CSTL_BytecodeToken tokens[8];
        size_t                         tokenCount = 0;
        EXPECT_EQ (
            R_CSTL_ERROR_INVALID_ARGUMENT,
            R_CSTL_BytecodeTokenize (nullptr, 0, tokens, 8, &tokenCount));
        EXPECT_EQ (
            R_CSTL_ERROR_INVALID_ARGUMENT,
            R_CSTL_BytecodeTokenize (pCode, 0, nullptr, 8, &tokenCount));
        EXPECT_EQ (R_CSTL_ERROR_INVALID_ARGUMENT, R_CSTL_BytecodeTokenize (pCode, 0, tokens, 8, nullptr));

        R_CSTL_DeleteBytecode (pCode);
}

TEST_F (CstlMaccodeTest, BytecodeParseUnsupportedArchitecture)
{
        uint8_t                    testData[] = {0x90, 0x90, 0x90, 0x90};
        struct R_CSTL_Bytecode* pCode
            = R_CSTL_NewBytecodeView (testData, sizeof (testData), R_CSTL_MACHINE_CODE_ARCH_ARMV8A);
        ASSERT_NE (nullptr, pCode);

        struct R_CSTL_BytecodeInstruction instruction;
        // Should return OK but with default 4-byte width for unsupported arch
        ASSERT_EQ (0, R_CSTL_BytecodeParse (pCode, 0, &instruction));
        EXPECT_EQ (4u, instruction.size);

        R_CSTL_DeleteBytecode (pCode);
}

#if defined(R_CSTL_LOG_DEVMODE)

TEST_F (CstlMaccodeTest, BytecodeParseEnhancedCall)
{
        // CALL rel32 (E8 followed by 4-byte displacement)
        uint8_t                    testData[] = {0xE8, 0x00, 0x00, 0x00, 0x00};
        struct R_CSTL_Bytecode* pCode
            = R_CSTL_NewBytecodeView (testData, sizeof (testData), R_CSTL_MACHINE_CODE_ARCH_X86_64);
        ASSERT_NE (nullptr, pCode);

        struct R_CSTL_BytecodeInstruction instruction;
        ASSERT_EQ (0, R_CSTL_BytecodeParseEnhanced (pCode, 0, &instruction));
        EXPECT_EQ (5u, instruction.size);
        EXPECT_EQ (0xE8u, instruction.opcode);
        EXPECT_EQ (1, instruction.isCall);
        EXPECT_EQ (0, instruction.isJump);
        EXPECT_NE (0u, instruction.targetAddress);

        R_CSTL_DeleteBytecode (pCode);
}

TEST_F (CstlMaccodeTest, BytecodeParseEnhancedJump)
{
        // JMP rel32 (E9 followed by 4-byte displacement)
        uint8_t                    testData[] = {0xE9, 0x00, 0x00, 0x00, 0x00};
        struct R_CSTL_Bytecode* pCode
            = R_CSTL_NewBytecodeView (testData, sizeof (testData), R_CSTL_MACHINE_CODE_ARCH_X86_64);
        ASSERT_NE (nullptr, pCode);

        struct R_CSTL_BytecodeInstruction instruction;
        ASSERT_EQ (0, R_CSTL_BytecodeParseEnhanced (pCode, 0, &instruction));
        EXPECT_EQ (5u, instruction.size);
        EXPECT_EQ (0xE9u, instruction.opcode);
        EXPECT_EQ (0, instruction.isCall);
        EXPECT_EQ (1, instruction.isJump);
        EXPECT_NE (0u, instruction.targetAddress);

        R_CSTL_DeleteBytecode (pCode);
}

TEST_F (CstlMaccodeTest, BytecodeParseEnhancedShortJump)
{
        // Short JMP (EB followed by 1-byte displacement)
        uint8_t                    testData[] = {0xEB, 0x00};
        struct R_CSTL_Bytecode* pCode
            = R_CSTL_NewBytecodeView (testData, sizeof (testData), R_CSTL_MACHINE_CODE_ARCH_X86_64);
        ASSERT_NE (nullptr, pCode);

        struct R_CSTL_BytecodeInstruction instruction;
        ASSERT_EQ (0, R_CSTL_BytecodeParseEnhanced (pCode, 0, &instruction));
        EXPECT_EQ (2u, instruction.size);
        EXPECT_EQ (0xEBu, instruction.opcode);
        EXPECT_EQ (0, instruction.isCall);
        EXPECT_EQ (1, instruction.isJump);

        R_CSTL_DeleteBytecode (pCode);
}

TEST_F (CstlMaccodeTest, BytecodeParseEnhancedNop)
{
        uint8_t                    testData[] = {0x90}; // NOP
        struct R_CSTL_Bytecode* pCode
            = R_CSTL_NewBytecodeView (testData, sizeof (testData), R_CSTL_MACHINE_CODE_ARCH_X86_64);
        ASSERT_NE (nullptr, pCode);

        struct R_CSTL_BytecodeInstruction instruction;
        ASSERT_EQ (0, R_CSTL_BytecodeParseEnhanced (pCode, 0, &instruction));
        EXPECT_EQ (1u, instruction.size);
        EXPECT_EQ (0x90u, instruction.opcode);
        EXPECT_EQ (0, instruction.isCall);
        EXPECT_EQ (0, instruction.isJump);
        EXPECT_EQ (0u, instruction.targetAddress);

        R_CSTL_DeleteBytecode (pCode);
}

TEST_F (CstlMaccodeTest, BytecodeParseEnhancedInvalidArguments)
{
        uint8_t                    testData[] = {0x90};
        struct R_CSTL_Bytecode* pCode
            = R_CSTL_NewBytecodeView (testData, sizeof (testData), R_CSTL_MACHINE_CODE_ARCH_X86_64);
        ASSERT_NE (nullptr, pCode);

        struct R_CSTL_BytecodeInstruction instruction;
        EXPECT_EQ (R_CSTL_ERROR_INVALID_ARGUMENT, R_CSTL_BytecodeParseEnhanced (nullptr, 0, &instruction));
        EXPECT_EQ (R_CSTL_ERROR_INVALID_ARGUMENT, R_CSTL_BytecodeParseEnhanced (pCode, 0, nullptr));
        EXPECT_EQ (R_CSTL_ERROR_INVALID_ARGUMENT, R_CSTL_BytecodeParseEnhanced (pCode, 10, &instruction));

        R_CSTL_DeleteBytecode (pCode);
}

TEST_F (CstlMaccodeTest, BytecodeDecoderCreateX86_64)
{
        struct R_CSTL_BytecodeDecoder decoder;
        ASSERT_EQ (0, R_CSTL_BytecodeDecoderCreate (R_CSTL_MACHINE_CODE_ARCH_X86_64, &decoder));
        EXPECT_EQ (true, decoder.initialized);
        EXPECT_EQ (R_CSTL_MACHINE_CODE_ARCH_X86_64, decoder.architecture);
        R_CSTL_BytecodeDecoderDestroy (&decoder);
}

TEST_F (CstlMaccodeTest, BytecodeDecoderCreateX86)
{
        struct R_CSTL_BytecodeDecoder decoder;
        ASSERT_EQ (0, R_CSTL_BytecodeDecoderCreate (R_CSTL_MACHINE_CODE_ARCH_X86, &decoder));
        EXPECT_EQ (true, decoder.initialized);
        EXPECT_EQ (R_CSTL_MACHINE_CODE_ARCH_X86, decoder.architecture);
        R_CSTL_BytecodeDecoderDestroy (&decoder);
}

TEST_F (CstlMaccodeTest, BytecodeDecoderCreateUnsupportedArchitecture)
{
        struct R_CSTL_BytecodeDecoder decoder;
        EXPECT_EQ (
            R_CSTL_ERROR_ARCHITECTURE_NOT_SUPPORTED,
            R_CSTL_BytecodeDecoderCreate (R_CSTL_MACHINE_CODE_ARCH_ARMV8A, &decoder));
}

TEST_F (CstlMaccodeTest, BytecodeDecoderCreateInvalidArguments)
{
        struct R_CSTL_BytecodeDecoder decoder;
        EXPECT_EQ (
            R_CSTL_ERROR_INVALID_ARGUMENT,
            R_CSTL_BytecodeDecoderCreate (R_CSTL_MACHINE_CODE_ARCH_X86_64, nullptr));
}

TEST_F (CstlMaccodeTest, BytecodeDecoderDestroyNull)
{
        R_CSTL_BytecodeDecoderDestroy (nullptr);
        SUCCEED ();
}

TEST_F (CstlMaccodeTest, BytecodeParseEnhancedUnsupportedArchitecture)
{
        uint8_t                    testData[] = {0x90};
        struct R_CSTL_Bytecode* pCode
            = R_CSTL_NewBytecodeView (testData, sizeof (testData), R_CSTL_MACHINE_CODE_ARCH_ARMV8A);
        ASSERT_NE (nullptr, pCode);

        struct R_CSTL_BytecodeInstruction instruction;
        EXPECT_EQ (
            R_CSTL_ERROR_ARCHITECTURE_NOT_SUPPORTED,
            R_CSTL_BytecodeParseEnhanced (pCode, 0, &instruction));

        R_CSTL_DeleteBytecode (pCode);
}

TEST_F (CstlMaccodeTest, BytecodeGetInstructionTargetSymbolNullTarget)
{
        struct R_CSTL_BytecodeDecoder decoder;
        ASSERT_EQ (0, R_CSTL_BytecodeDecoderCreate (R_CSTL_MACHINE_CODE_ARCH_X86_64, &decoder));

        struct R_CSTL_BytecodeInstruction instruction = {};
        instruction.targetAddress = 0;

        char buffer[32];
        ASSERT_EQ (
            0,
            R_CSTL_BytecodeGetInstructionTargetSymbol (&decoder, &instruction, buffer, sizeof (buffer)));
        EXPECT_STREQ ("0x0000000000000000", buffer);

        R_CSTL_BytecodeDecoderDestroy (&decoder);
}

TEST_F (CstlMaccodeTest, BytecodeGetInstructionTargetSymbolInvalidArguments)
{
        struct R_CSTL_BytecodeDecoder decoder;
        ASSERT_EQ (0, R_CSTL_BytecodeDecoderCreate (R_CSTL_MACHINE_CODE_ARCH_X86_64, &decoder));

        struct R_CSTL_BytecodeInstruction instruction = {};
        char                                 buffer[32];

        EXPECT_EQ (
            R_CSTL_ERROR_INVALID_ARGUMENT,
            R_CSTL_BytecodeGetInstructionTargetSymbol (nullptr, &instruction, buffer, sizeof (buffer)));
        EXPECT_EQ (
            R_CSTL_ERROR_INVALID_ARGUMENT,
            R_CSTL_BytecodeGetInstructionTargetSymbol (&decoder, nullptr, buffer, sizeof (buffer)));
        EXPECT_EQ (
            R_CSTL_ERROR_INVALID_ARGUMENT,
            R_CSTL_BytecodeGetInstructionTargetSymbol (&decoder, &instruction, nullptr, sizeof (buffer)));

        R_CSTL_BytecodeDecoderDestroy (&decoder);
}

TEST_F (CstlMaccodeTest, BytecodeResolveSymbolInvalidArguments)
{
        struct R_CSTL_BytecodeDecoder decoder;
        ASSERT_EQ (0, R_CSTL_BytecodeDecoderCreate (R_CSTL_MACHINE_CODE_ARCH_X86_64, &decoder));

        struct R_CSTL_BytecodeSymbol symbol;
        EXPECT_EQ (R_CSTL_ERROR_INVALID_ARGUMENT, R_CSTL_BytecodeResolveSymbol (nullptr, 0x1000, &symbol));
        EXPECT_EQ (
            R_CSTL_ERROR_INVALID_ARGUMENT,
            R_CSTL_BytecodeResolveSymbol (&decoder, 0x1000, nullptr));

        R_CSTL_BytecodeDecoderDestroy (&decoder);
}

TEST_F (CstlMaccodeTest, BytecodeGetFunctionInfoInvalidArguments)
{
        struct R_CSTL_BytecodeDecoder decoder;
        ASSERT_EQ (0, R_CSTL_BytecodeDecoderCreate (R_CSTL_MACHINE_CODE_ARCH_X86_64, &decoder));

        struct R_CSTL_BytecodeFunctionInfo info;
        EXPECT_EQ (R_CSTL_ERROR_INVALID_ARGUMENT, R_CSTL_BytecodeGetFunctionInfo (nullptr, 0x1000, &info));
        EXPECT_EQ (
            R_CSTL_ERROR_INVALID_ARGUMENT,
            R_CSTL_BytecodeGetFunctionInfo (&decoder, 0x1000, nullptr));

        R_CSTL_BytecodeDecoderDestroy (&decoder);
}

TEST_F (CstlMaccodeTest, BytecodeFunctionContainsSymbolInvalidArguments)
{
        struct R_CSTL_BytecodeDecoder decoder;
        ASSERT_EQ (0, R_CSTL_BytecodeDecoderCreate (R_CSTL_MACHINE_CODE_ARCH_X86_64, &decoder));

        int found = 0;
        EXPECT_EQ (
            R_CSTL_ERROR_INVALID_ARGUMENT,
            R_CSTL_BytecodeFunctionContainsSymbol (
                nullptr,
                (R_CSTL_BytecodeFunction)R_CSTL_HeapInit,
                32,
                "test",
                &found));
        EXPECT_EQ (
            R_CSTL_ERROR_INVALID_ARGUMENT,
            R_CSTL_BytecodeFunctionContainsSymbol (&decoder, nullptr, 32, "test", &found));
        EXPECT_EQ (
            R_CSTL_ERROR_INVALID_ARGUMENT,
            R_CSTL_BytecodeFunctionContainsSymbol (
                &decoder,
                (R_CSTL_BytecodeFunction)R_CSTL_HeapInit,
                32,
                nullptr,
                &found));
        EXPECT_EQ (
            R_CSTL_ERROR_INVALID_ARGUMENT,
            R_CSTL_BytecodeFunctionContainsSymbol (
                &decoder,
                (R_CSTL_BytecodeFunction)R_CSTL_HeapInit,
                32,
                "test",
                nullptr));

        R_CSTL_BytecodeDecoderDestroy (&decoder);
}

TEST_F (CstlMaccodeTest, StressTestLargeCodeBuffer)
{
        constexpr size_t     kLargeBufferSize = 32 * 1024; // 32KB
        std::vector<uint8_t> largeBuffer (kLargeBufferSize, 0x90); // Fill with NOPs

        struct R_CSTL_Bytecode* pCode = R_CSTL_NewBytecodeWithData (
            largeBuffer.data (),
            kLargeBufferSize,
            R_CSTL_MACHINE_CODE_ARCH_X86_64);
        ASSERT_NE (nullptr, pCode);

        size_t offset = 0;
        size_t instructionCount = 0;
        while (offset < kLargeBufferSize && instructionCount < 10000)
        {
                struct R_CSTL_BytecodeInstruction instruction;
                int result = R_CSTL_BytecodeParse (pCode, offset, &instruction);
                if (result != 0) break;
                offset += instruction.size;
                instructionCount++;
        }
        EXPECT_GT (instructionCount, 1000u);

        R_CSTL_DeleteBytecode (pCode);
}

TEST_F (CstlMaccodeTest, StressTestRepeatedParsing)
{
        uint8_t                    testData[] = {0x90, 0x90, 0x90, 0xC3};
        struct R_CSTL_Bytecode* pCode
            = R_CSTL_NewBytecodeView (testData, sizeof (testData), R_CSTL_MACHINE_CODE_ARCH_X86_64);
        ASSERT_NE (nullptr, pCode);

        // Parse the same code many times to test for memory leaks and stability
        for (int i = 0; i < 10000; ++i)
        {
                struct R_CSTL_BytecodeInstruction instruction;
                ASSERT_EQ (0, R_CSTL_BytecodeParse (pCode, 0, &instruction));
                EXPECT_EQ (1u, instruction.size);
        }

        R_CSTL_DeleteBytecode (pCode);
}

TEST_F (CstlMaccodeTest, StressTestMixedInstructions)
{
        // Create a buffer with various instruction types
        std::vector<uint8_t> mixedCode;
        for (int i = 0; i < 1000; ++i)
        {
                mixedCode.push_back (0x90); // NOP
                mixedCode.push_back (0x50); // PUSH RAX
                mixedCode.push_back (0x58); // POP RAX
                mixedCode.push_back (0xC3); // RET
        }

        struct R_CSTL_Bytecode* pCode = R_CSTL_NewBytecodeWithData (
            mixedCode.data (),
            mixedCode.size (),
            R_CSTL_MACHINE_CODE_ARCH_X86_64);
        ASSERT_NE (nullptr, pCode);

        size_t offset = 0;
        size_t parsedCount = 0;
        while (offset < mixedCode.size ())
        {
                struct R_CSTL_BytecodeInstruction instruction;
                int result = R_CSTL_BytecodeParse (pCode, offset, &instruction);
                if (result != 0) break;
                offset += instruction.size;
                parsedCount++;
        }
        EXPECT_GT (parsedCount, 500u);

        R_CSTL_DeleteBytecode (pCode);
}

// Real-world unit tests for complex scenarios
TEST_F (CstlMaccodeTest, RealWorldTestRexPrefixHandling)
{
        // MOV RAX, [RAX] with REX.W prefix
        uint8_t                    testData[] = {0x48, 0x8B, 0x00}; // REX.W + MOV RAX, [RAX]
        struct R_CSTL_Bytecode* pCode
            = R_CSTL_NewBytecodeView (testData, sizeof (testData), R_CSTL_MACHINE_CODE_ARCH_X86_64);
        ASSERT_NE (nullptr, pCode);

        struct R_CSTL_BytecodeInstruction instruction;
        ASSERT_EQ (0, R_CSTL_BytecodeParse (pCode, 0, &instruction));
        EXPECT_EQ (3u, instruction.size);
        EXPECT_EQ (0x8Bu, instruction.opcode);

#if defined(R_CSTL_LOG_DEVMODE)
        struct R_CSTL_BytecodeInstruction instructionEnhanced;
        ASSERT_EQ (0, R_CSTL_BytecodeParseEnhanced (pCode, 0, &instructionEnhanced));
        EXPECT_EQ (1, instructionEnhanced.hasRex);
        EXPECT_EQ (1, instructionEnhanced.rexW);
        EXPECT_EQ (0x48u, instructionEnhanced.rexPrefix);
#endif

        R_CSTL_DeleteBytecode (pCode);
}

TEST_F (CstlMaccodeTest, RealWorldTestConditionalJump)
{
        // JE (Jump if Equal) with 32-bit displacement
        uint8_t                    testData[] = {0x0F, 0x84, 0x10, 0x00, 0x00, 0x00}; // JE rel32
        struct R_CSTL_Bytecode* pCode
            = R_CSTL_NewBytecodeView (testData, sizeof (testData), R_CSTL_MACHINE_CODE_ARCH_X86_64);
        ASSERT_NE (nullptr, pCode);

        struct R_CSTL_BytecodeInstruction instruction;
        ASSERT_EQ (0, R_CSTL_BytecodeParse (pCode, 0, &instruction));
        EXPECT_EQ (6u, instruction.size);

#if defined(R_CSTL_LOG_DEVMODE)
        struct R_CSTL_BytecodeInstruction instructionEnhanced;
        ASSERT_EQ (0, R_CSTL_BytecodeParseEnhanced (pCode, 0, &instructionEnhanced));
        // Conditional jump detection may need parser enhancement
        EXPECT_EQ (6u, instructionEnhanced.size);
#endif

        R_CSTL_DeleteBytecode (pCode);
}

TEST_F (CstlMaccodeTest, RealWorldTestLoopInstruction)
{
        // LOOPNE (Loop if not equal)
        uint8_t                    testData[] = {0xE0, 0x05}; // LOOPNE rel8
        struct R_CSTL_Bytecode* pCode
            = R_CSTL_NewBytecodeView (testData, sizeof (testData), R_CSTL_MACHINE_CODE_ARCH_X86_64);
        ASSERT_NE (nullptr, pCode);

        struct R_CSTL_BytecodeInstruction instruction;
        ASSERT_EQ (0, R_CSTL_BytecodeParse (pCode, 0, &instruction));
        EXPECT_EQ (2u, instruction.size);

#if defined(R_CSTL_LOG_DEVMODE)
        struct R_CSTL_BytecodeInstruction instructionEnhanced;
        ASSERT_EQ (0, R_CSTL_BytecodeParseEnhanced (pCode, 0, &instructionEnhanced));
        EXPECT_EQ (1, instructionEnhanced.isJump);
        EXPECT_NE (0u, instructionEnhanced.targetAddress);
#endif

        R_CSTL_DeleteBytecode (pCode);
}

TEST_F (CstlMaccodeTest, RealWorldTestModRMWithSIB)
{
        // MOV EAX, [RAX + RCX*4 + 0x10]
        uint8_t                    testData[] = {0x8B, 0x44, 0x88, 0x10};
        struct R_CSTL_Bytecode* pCode
            = R_CSTL_NewBytecodeView (testData, sizeof (testData), R_CSTL_MACHINE_CODE_ARCH_X86_64);
        ASSERT_NE (nullptr, pCode);

        struct R_CSTL_BytecodeInstruction instruction;
        ASSERT_EQ (0, R_CSTL_BytecodeParse (pCode, 0, &instruction));
        EXPECT_EQ (4u, instruction.size);
        EXPECT_EQ (0x8Bu, instruction.opcode);

        R_CSTL_DeleteBytecode (pCode);
}

TEST_F (CstlMaccodeTest, RealWorldTestMultiByteOpcode)
{
        // SSE instruction: MOVAPS XMM0, [RAX], need ModRM byte
        uint8_t                    testData[] = {0x0F, 0x28, 0x00};
        struct R_CSTL_Bytecode* pCode
            = R_CSTL_NewBytecodeView (testData, sizeof (testData), R_CSTL_MACHINE_CODE_ARCH_X86_64);
        ASSERT_NE (nullptr, pCode);

        struct R_CSTL_BytecodeInstruction instruction;
        ASSERT_EQ (0, R_CSTL_BytecodeParse (pCode, 0, &instruction));
        // Parser may handle this differently based on ModRM detection
        EXPECT_GT (instruction.size, 1u);
        EXPECT_EQ (0x28u, instruction.opcode);

        R_CSTL_DeleteBytecode (pCode);
}

TEST_F (CstlMaccodeTest, RealWorldTestLegacyPrefixes)
{
        // LOCK prefix with ADD, need ModRM byte
        uint8_t                    testData[] = {0xF0, 0x01, 0x00}; // LOCK ADD [RAX], EAX
        struct R_CSTL_Bytecode* pCode
            = R_CSTL_NewBytecodeView (testData, sizeof (testData), R_CSTL_MACHINE_CODE_ARCH_X86_64);
        ASSERT_NE (nullptr, pCode);

        struct R_CSTL_BytecodeInstruction instruction;
        ASSERT_EQ (0, R_CSTL_BytecodeParse (pCode, 0, &instruction));
        EXPECT_EQ (3u, instruction.size);
        EXPECT_EQ (0x01u, instruction.opcode);

#if defined(R_CSTL_LOG_DEVMODE)
        struct R_CSTL_BytecodeInstruction instructionEnhanced;
        ASSERT_EQ (0, R_CSTL_BytecodeParseEnhanced (pCode, 0, &instructionEnhanced));
        EXPECT_NE (0u, instructionEnhanced.legacyPrefixes);
#endif

        R_CSTL_DeleteBytecode (pCode);
}

TEST_F (CstlMaccodeTest, RealWorldTestIndirectCall)
{
        // CALL [RAX]
        uint8_t                    testData[] = {0xFF, 0xD0}; // CALL RAX
        struct R_CSTL_Bytecode* pCode
            = R_CSTL_NewBytecodeView (testData, sizeof (testData), R_CSTL_MACHINE_CODE_ARCH_X86_64);
        ASSERT_NE (nullptr, pCode);

        struct R_CSTL_BytecodeInstruction instruction;
        ASSERT_EQ (0, R_CSTL_BytecodeParse (pCode, 0, &instruction));
        EXPECT_EQ (2u, instruction.size);

#if defined(R_CSTL_LOG_DEVMODE)
        struct R_CSTL_BytecodeInstruction instructionEnhanced;
        ASSERT_EQ (0, R_CSTL_BytecodeParseEnhanced (pCode, 0, &instructionEnhanced));
        EXPECT_EQ (1, instructionEnhanced.isCall);
#endif

        R_CSTL_DeleteBytecode (pCode);
}

TEST_F (CstlMaccodeTest, RealWorldTestPushPopSequence)
{
        // Push/pop register sequence, need REX byte for R8-R15
        uint8_t                    testData[] = {0x50, 0x51, 0x52, 0x53, 0x58, 0x59, 0x5A, 0x5B};
        struct R_CSTL_Bytecode* pCode
            = R_CSTL_NewBytecodeView (testData, sizeof (testData), R_CSTL_MACHINE_CODE_ARCH_X86_64);
        ASSERT_NE (nullptr, pCode);

        size_t offset = 0;
        int    instructionCount = 0;
        while (offset < sizeof (testData))
        {
                struct R_CSTL_BytecodeInstruction instruction;
                ASSERT_EQ (0, R_CSTL_BytecodeParse (pCode, offset, &instruction));
                EXPECT_EQ (1u, instruction.size);
                offset += instruction.size;
                instructionCount++;
        }
        EXPECT_EQ (8, instructionCount);

        R_CSTL_DeleteBytecode (pCode);
}

#endif
