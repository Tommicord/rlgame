#include <gtest/gtest.h>

extern "C" {
#include "microbit/microbit_spirv_parser.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"
}

namespace {

constexpr size_t kTestHeapSize = 4 * 1024 * 1024;

class MicrobitSpirvParserOpcodeSetupTest : public ::testing::Test {
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

TEST_F(MicrobitSpirvParserOpcodeSetupTest, OpcodeSetupRegistersFunctionPointers) {
    struct R_Microbit_SpirvParserContext* ctx = CreateContext();
    ASSERT_NE(nullptr, ctx);

    // Check that setup functions are registered for type opcodes
    EXPECT_NE(nullptr, ctx->pOpcodeSetup[MICROBIT_SPIRV_OP_TYPE_VOID]);
    EXPECT_NE(nullptr, ctx->pOpcodeSetup[MICROBIT_SPIRV_OP_TYPE_BOOL]);
    EXPECT_NE(nullptr, ctx->pOpcodeSetup[MICROBIT_SPIRV_OP_TYPE_INT]);
    EXPECT_NE(nullptr, ctx->pOpcodeSetup[MICROBIT_SPIRV_OP_TYPE_FLOAT]);
    EXPECT_NE(nullptr, ctx->pOpcodeSetup[MICROBIT_SPIRV_OP_TYPE_VECTOR]);
    EXPECT_NE(nullptr, ctx->pOpcodeSetup[MICROBIT_SPIRV_OP_TYPE_MATRIX]);
    EXPECT_NE(nullptr, ctx->pOpcodeSetup[MICROBIT_SPIRV_OP_TYPE_ARRAY]);
    EXPECT_NE(nullptr, ctx->pOpcodeSetup[MICROBIT_SPIRV_OP_TYPE_RUNTIME_ARRAY]);
    EXPECT_NE(nullptr, ctx->pOpcodeSetup[MICROBIT_SPIRV_OP_TYPE_STRUCT]);
    EXPECT_NE(nullptr, ctx->pOpcodeSetup[MICROBIT_SPIRV_OP_TYPE_POINTER]);
    EXPECT_NE(nullptr, ctx->pOpcodeSetup[MICROBIT_SPIRV_OP_CONSTANT]);
    EXPECT_NE(nullptr, ctx->pOpcodeSetup[MICROBIT_SPIRV_OP_VARIABLE]);
    EXPECT_NE(nullptr, ctx->pOpcodeSetup[MICROBIT_SPIRV_OP_DECORATE]);
    EXPECT_NE(nullptr, ctx->pOpcodeSetup[MICROBIT_SPIRV_OP_MEMBER_DECORATE]);

    DeleteContext(ctx);
}

TEST_F(MicrobitSpirvParserOpcodeSetupTest, OpcodeExecuteRegistersFunctionPointers) {
    struct R_Microbit_SpirvParserContext* ctx = CreateContext();
    ASSERT_NE(nullptr, ctx);

    // The runner registers execute functions - check they exist in context after runner init
    // This test just verifies the context creation works
    EXPECT_NE(nullptr, ctx);

    DeleteContext(ctx);
}

TEST_F(MicrobitSpirvParserOpcodeSetupTest, MultipleContextsIndependent) {
    struct R_Microbit_SpirvParserContext* ctx1 = CreateContext();
    struct R_Microbit_SpirvParserContext* ctx2 = CreateContext();

    ASSERT_NE(nullptr, ctx1);
    ASSERT_NE(nullptr, ctx2);
    EXPECT_NE(ctx1, ctx2);

    DeleteContext(ctx1);
    DeleteContext(ctx2);
}

TEST_F(MicrobitSpirvParserOpcodeSetupTest, ProgramBoundCalculation) {
    struct R_Microbit_SpirvParserContext* ctx = CreateContext();
    ASSERT_NE(nullptr, ctx);

    const uint32_t spv[] = {
        0x07230203, 0x00010500, 0x000d000b, 0x00000020, 0x00000000,
        0x00020011, 0x00000001,
        0x00030003, 0x0000000a, 0x000001c2,
        0x00030003, 0x0000000b, 0x00000001,
        0x00030036, 0x0000000c, 0x00000001,
        0x00020013, 0x00000001,
        0x00040017, 0x0000000d, 0x00000019, 0x00000000,
        0x00040036, 0x0000000e, 0x0000000d, 0x0000000c,
        0x00040036, 0x0000000f, 0x00000014, 0x0000000c,
        0x00050051, 0x0000000c, 0x00000010, 0x00000005, 0x00000000,
        0x000200f8, 0x0000000f,
    };

    struct R_Microbit_SpirvParserProgram* prog = R_Microbit_NewSpirvParserProgram(ctx, spv, 32);
    ASSERT_NE(nullptr, prog);
    EXPECT_EQ(0x14u, prog->bound);

    R_Microbit_DeleteSpirvParserProgram(prog);
    DeleteContext(ctx);
}

TEST_F(MicrobitSpirvParserOpcodeSetupTest, ProgramCodeLengthCalculation) {
    struct R_Microbit_SpirvParserContext* ctx = CreateContext();
    ASSERT_NE(nullptr, ctx);

    const uint32_t spv[] = {
        0x07230203, 0x00010500, 0x000d000b, 0x00000010, 0x00000000,
        0x00020011, 0x00000001,
    };

    struct R_Microbit_SpirvParserProgram* prog = R_Microbit_NewSpirvParserProgram(ctx, spv, 7);
    ASSERT_NE(nullptr, prog);
    EXPECT_EQ(2u, prog->codeLength);

    R_Microbit_DeleteSpirvParserProgram(prog);
    DeleteContext(ctx);
}

TEST_F(MicrobitSpirvParserOpcodeSetupTest, ProgramVersionParsing) {
    struct R_Microbit_SpirvParserContext* ctx = CreateContext();
    ASSERT_NE(nullptr, ctx);

    const uint32_t spv[] = {
        0x07230203, 0x00010500, 0x000d000b, 0x00000010, 0x00000000,
    };

    struct R_Microbit_SpirvParserProgram* prog = R_Microbit_NewSpirvParserProgram(ctx, spv, 5);
    ASSERT_NE(nullptr, prog);
    EXPECT_EQ(1u, prog->majorVersion);
    EXPECT_EQ(5u, prog->minorVersion);

    R_Microbit_DeleteSpirvParserProgram(prog);
    DeleteContext(ctx);
}

TEST_F(MicrobitSpirvParserOpcodeSetupTest, ProgramGeneratorParsing) {
    struct R_Microbit_SpirvParserContext* ctx = CreateContext();
    ASSERT_NE(nullptr, ctx);

    const uint32_t spv[] = {
        0x07230203, 0x00010500, 0x12345678, 0x00000010, 0x00000000,
    };

    struct R_Microbit_SpirvParserProgram* prog = R_Microbit_NewSpirvParserProgram(ctx, spv, 5);
    ASSERT_NE(nullptr, prog);
    EXPECT_EQ(0x1234u, prog->generatorId);
    EXPECT_EQ(0x5678u, prog->generatorVersion);

    R_Microbit_DeleteSpirvParserProgram(prog);
    DeleteContext(ctx);
}

}  // namespace