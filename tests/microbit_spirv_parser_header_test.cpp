#include <gtest/gtest.h>

extern "C" {
#include "microbit/microbit_spirv_parser.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"
}

namespace {

constexpr size_t kTestHeapSize = 256 * 1024;

class MicrobitSpirvParserHeaderTest : public ::testing::Test {
protected:
    void SetUp() override {
        ASSERT_EQ(0, R_CSTL_HeapInit(kTestHeapSize));
    }

    void TearDown() override {
        R_CSTL_HeapShutdown();
    }

    struct R_Microbit_SpirvParserContext* CreateContext() {
        return R_Microbit_NewSpirvParserContext();
    }

    void DeleteContext(struct R_Microbit_SpirvParserContext* ctx) {
        R_Microbit_DeleteSpirvParserContext(ctx);
    }
};

TEST_F(MicrobitSpirvParserHeaderTest, ValidateHeaderValidMagicAndVersion) {
    const uint32_t validSpv[] = {
        0x07230203,  // Magic number
        0x00010500,  // Version 1.5
        0x000d000b,  // Generator
        0x00000013,  // Bound
        0x00000000   // Schema
    };

    enum R_Microbit_SpirvParserError result = R_Microbit_SpirvParserValidateHeader(validSpv, 5);
    EXPECT_EQ(MICROBIT_SPIRV_OK, result);
}

TEST_F(MicrobitSpirvParserHeaderTest, ValidateHeaderInvalidMagic) {
    const uint32_t invalidSpv[] = {
        0x07230204,  // Wrong magic
        0x00010500,
        0x000d000b,
        0x00000013,
        0x00000000
    };

    enum R_Microbit_SpirvParserError result = R_Microbit_SpirvParserValidateHeader(invalidSpv, 5);
    EXPECT_EQ(MICROBIT_SPIRV_ERROR_INVALID_MAGIC, result);
}

TEST_F(MicrobitSpirvParserHeaderTest, ValidateHeaderInvalidVersion) {
    const uint32_t invalidSpv[] = {
        0x07230203,
        0x00010600,  // Version 1.6 (unsupported)
        0x000d000b,
        0x00000013,
        0x00000000
    };

    enum R_Microbit_SpirvParserError result = R_Microbit_SpirvParserValidateHeader(invalidSpv, 5);
    EXPECT_EQ(MICROBIT_SPIRV_ERROR_INVALID_VERSION, result);
}

TEST_F(MicrobitSpirvParserHeaderTest, ValidateHeaderTooShortData) {
    const uint32_t shortSpv[] = {
        0x07230203,
        0x00010500
    };

    enum R_Microbit_SpirvParserError result = R_Microbit_SpirvParserValidateHeader(shortSpv, 2);
    EXPECT_EQ(MICROBIT_SPIRV_ERROR_INVALID_DATA, result);
}

TEST_F(MicrobitSpirvParserHeaderTest, ErrorToString_AllCodes) {
    EXPECT_STREQ("Success", R_Microbit_SpirvParserErrorToString(MICROBIT_SPIRV_OK));
    EXPECT_STREQ("Operation failed", R_Microbit_SpirvParserErrorToString(MICROBIT_SPIRV_ERROR_FAILED));
    EXPECT_STREQ("Out of memory", R_Microbit_SpirvParserErrorToString(MICROBIT_SPIRV_ERROR_OUT_OF_MEMORY));
    EXPECT_STREQ("Invalid argument", R_Microbit_SpirvParserErrorToString(MICROBIT_SPIRV_ERROR_INVALID_ARGUMENT));
    EXPECT_STREQ("Null pointer", R_Microbit_SpirvParserErrorToString(MICROBIT_SPIRV_ERROR_NULL_POINTER));
    EXPECT_STREQ("Invalid SPIR-V magic number", R_Microbit_SpirvParserErrorToString(MICROBIT_SPIRV_ERROR_INVALID_MAGIC));
    EXPECT_STREQ("Invalid SPIR-V version", R_Microbit_SpirvParserErrorToString(MICROBIT_SPIRV_ERROR_INVALID_VERSION));
    EXPECT_STREQ("Invalid SPIR-V data", R_Microbit_SpirvParserErrorToString(MICROBIT_SPIRV_ERROR_INVALID_DATA));
    EXPECT_STREQ("Unsupported opcode", R_Microbit_SpirvParserErrorToString(MICROBIT_SPIRV_ERROR_UNSUPPORTED_OPCODE));
    EXPECT_STREQ("Unknown error", R_Microbit_SpirvParserErrorToString(MICROBIT_SPIRV_ERROR_UNKNOWN));
}

TEST_F(MicrobitSpirvParserHeaderTest, NewContextNotNull) {
    struct R_Microbit_SpirvParserContext* ctx = CreateContext();
    ASSERT_NE(nullptr, ctx);
    DeleteContext(ctx);
}

TEST_F(MicrobitSpirvParserHeaderTest, NewContextDeleteNullSafe) {
    DeleteContext(nullptr);
    SUCCEED();
}

TEST_F(MicrobitSpirvParserHeaderTest, NewProgramValidSpv) {
    struct R_Microbit_SpirvParserContext* ctx = CreateContext();
    ASSERT_NE(nullptr, ctx);

    const uint32_t validSpv[] = {
        0x07230203,  // Magic
        0x00010500,  // Version 1.5
        0x000d000b,  // Generator
        0x00000010,  // Bound
        0x00000000,  // Schema
    };

    struct R_Microbit_SpirvParserProgram* prog = R_Microbit_NewSpirvParserProgram(ctx, validSpv, 5);
    ASSERT_NE(nullptr, prog);
    EXPECT_EQ(1, prog->majorVersion);
    EXPECT_EQ(5, prog->minorVersion);

    R_Microbit_DeleteSpirvParserProgram(prog);
    DeleteContext(ctx);
}

TEST_F(MicrobitSpirvParserHeaderTest, NewProgramInvalidHeaderReturnsNull) {
    struct R_Microbit_SpirvParserContext* ctx = CreateContext();
    ASSERT_NE(nullptr, ctx);

    const uint32_t invalidSpv[] = {
        0x07230204,  // Wrong magic
        0x00010500,
        0x000d000b,
        0x00000010,
        0x00000000
    };

    struct R_Microbit_SpirvParserProgram* prog = R_Microbit_NewSpirvParserProgram(ctx, invalidSpv, 5);
    EXPECT_EQ(nullptr, prog);

    DeleteContext(ctx);
}

TEST_F(MicrobitSpirvParserHeaderTest, NewProgramNullContextReturnsNull) {
    const uint32_t validSpv[] = {
        0x07230203, 0x00010500, 0x000d000b, 0x00000010, 0x00000000
    };

    struct R_Microbit_SpirvParserProgram* prog = R_Microbit_NewSpirvParserProgram(nullptr, validSpv, 5);
    EXPECT_EQ(nullptr, prog);
}

TEST_F(MicrobitSpirvParserHeaderTest, NewProgramNullSpvReturnsNull) {
    struct R_Microbit_SpirvParserContext* ctx = CreateContext();
    ASSERT_NE(nullptr, ctx);

    struct R_Microbit_SpirvParserProgram* prog = R_Microbit_NewSpirvParserProgram(ctx, nullptr, 5);
    EXPECT_EQ(nullptr, prog);

    DeleteContext(ctx);
}

TEST_F(MicrobitSpirvParserHeaderTest, NewProgramShortSpvReturnsNull) {
    struct R_Microbit_SpirvParserContext* ctx = CreateContext();
    ASSERT_NE(nullptr, ctx);

    const uint32_t shortSpv[] = {0x07230203, 0x00010500};

    struct R_Microbit_SpirvParserProgram* prog = R_Microbit_NewSpirvParserProgram(ctx, shortSpv, 2);
    EXPECT_EQ(nullptr, prog);

    DeleteContext(ctx);
}

TEST_F(MicrobitSpirvParserHeaderTest, ProgramAddExtension) {
    struct R_Microbit_SpirvParserContext* ctx = CreateContext();
    ASSERT_NE(nullptr, ctx);

    const uint32_t validSpv[] = {0x07230203, 0x00010500, 0x000d000b, 0x00000010, 0x00000000};
    struct R_Microbit_SpirvParserProgram* prog = R_Microbit_NewSpirvParserProgram(ctx, validSpv, 5);
    ASSERT_NE(nullptr, prog);

    char* ext = R_Microbit_SpirvParserProgramAddExtension(prog, 10);
    ASSERT_NE(nullptr, ext);
    EXPECT_EQ(1u, prog->extensionCount);

    R_Microbit_DeleteSpirvParserProgram(prog);
    DeleteContext(ctx);
}

TEST_F(MicrobitSpirvParserHeaderTest, ProgramCreateEntryPoint) {
    struct R_Microbit_SpirvParserContext* ctx = CreateContext();
    ASSERT_NE(nullptr, ctx);

    const uint32_t validSpv[] = {0x07230203, 0x00010500, 0x000d000b, 0x00000010, 0x00000000};
    struct R_Microbit_SpirvParserProgram* prog = R_Microbit_NewSpirvParserProgram(ctx, validSpv, 5);
    ASSERT_NE(nullptr, prog);

    struct R_Microbit_SpirvParserEntryPoint* ep = R_Microbit_NewSpirvParserProgramEntryPoint(prog);
    ASSERT_NE(nullptr, ep);
    EXPECT_EQ(1u, prog->entryPointCount);

    R_Microbit_DeleteSpirvParserProgram(prog);
    DeleteContext(ctx);
}

TEST_F(MicrobitSpirvParserHeaderTest, ProgramAddCapability) {
    struct R_Microbit_SpirvParserContext* ctx = CreateContext();
    ASSERT_NE(nullptr, ctx);

    const uint32_t validSpv[] = {0x07230203, 0x00010500, 0x000d000b, 0x00000010, 0x00000000};
    struct R_Microbit_SpirvParserProgram* prog = R_Microbit_NewSpirvParserProgram(ctx, validSpv, 5);
    ASSERT_NE(nullptr, prog);

    R_Microbit_SpirvParserProgramAddCapability(prog, 0);  // Shader capability
    EXPECT_EQ(1u, prog->capabilityCount);
    EXPECT_EQ(0u, prog->pCapabilities[0]);

    R_Microbit_DeleteSpirvParserProgram(prog);
    DeleteContext(ctx);
}

TEST_F(MicrobitSpirvParserHeaderTest, DeleteProgramNullSafe) {
    R_Microbit_DeleteSpirvParserProgram(nullptr);
    SUCCEED();
}

}  // namespace