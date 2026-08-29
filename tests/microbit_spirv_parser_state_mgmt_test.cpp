#include <gtest/gtest.h>

extern "C" {
#include "microbit/microbit_spirv_parser.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"
}

namespace {

constexpr size_t kTestHeapSize = 256 * 1024;

class MicrobitSpirvParserStateManagementTest : public ::testing::Test {
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

    struct R_Microbit_SpirvParserProgram* CreateProgram(struct R_Microbit_SpirvParserContext* ctx, const uint32_t* spv, size_t len) {
        return R_Microbit_NewSpirvParserProgram(ctx, spv, len);
    }

    struct R_Microbit_SpirvParserState* CreateState(struct R_Microbit_SpirvParserProgram* prog) {
        return R_Microbit_NewSpirvParserState(prog);
    }

    void Cleanup(struct R_Microbit_SpirvParserState* state, struct R_Microbit_SpirvParserProgram* prog,
                 struct R_Microbit_SpirvParserContext* ctx) {
        if (state) R_Microbit_DeleteSpirvParserState(state);
        if (prog) R_Microbit_DeleteSpirvParserProgram(prog);
        if (ctx) R_Microbit_DeleteSpirvParserContext(ctx);
    }
};

TEST_F(MicrobitSpirvParserStateManagementTest, StateResultsArrayAllocation) {
    struct R_Microbit_SpirvParserContext* ctx = CreateContext();
    const uint32_t spv[] = {
        0x07230203, 0x00010500, 0x000d000b, 0x00000020, 0x00000000,
        0x00020011, 0x00000001,
    };
    struct R_Microbit_SpirvParserProgram* prog = CreateProgram(ctx, spv, 7);
    struct R_Microbit_SpirvParserState* state = CreateState(prog);

    ASSERT_NE(nullptr, state);
    EXPECT_NE(nullptr, state->pResults);
    EXPECT_EQ(prog->bound + 1, 0x21u);  // bound is 0x20, so 0x21 results

    Cleanup(state, prog, ctx);
}

TEST_F(MicrobitSpirvParserStateManagementTest, StateInitialization_StorageClassMax) {
    struct R_Microbit_SpirvParserContext* ctx = CreateContext();
    const uint32_t spv[] = {
        0x07230203, 0x00010500, 0x000d000b, 0x00000010, 0x00000000,
        0x00020011, 0x00000001,
    };
    struct R_Microbit_SpirvParserProgram* prog = CreateProgram(ctx, spv, 7);
    struct R_Microbit_SpirvParserState* state = CreateState(prog);

    ASSERT_NE(nullptr, state);
    EXPECT_EQ(MICROBIT_SPIRV_STORAGE_CLASS_MAX, state->pResults[0].storageClass);
    EXPECT_EQ(MICROBIT_SPIRV_STORAGE_CLASS_MAX, state->pResults[5].storageClass);

    Cleanup(state, prog, ctx);
}

TEST_F(MicrobitSpirvParserStateManagementTest, StateFunctionStackAllocation) {
    struct R_Microbit_SpirvParserContext* ctx = CreateContext();
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
    struct R_Microbit_SpirvParserProgram* prog = CreateProgram(ctx, spv, 32);
    struct R_Microbit_SpirvParserState* state = CreateState(prog);

    ASSERT_NE(nullptr, state);
    EXPECT_EQ(0u, state->functionStackCount);
    EXPECT_EQ(0u, state->functionStackCurrent);
    EXPECT_EQ(nullptr, state->ppFunctionStack);

    R_Microbit_SpirvParserStatePrepare(state, 0x0f);
    EXPECT_EQ(10u, state->functionStackCount);
    EXPECT_NE(nullptr, state->ppFunctionStack);
    EXPECT_NE(nullptr, state->ppFunctionStackInfo);
    EXPECT_NE(nullptr, state->pFunctionStackReturns);
    EXPECT_NE(nullptr, state->pFunctionStackCfg);
    EXPECT_NE(nullptr, state->pFunctionStackCfgParent);

    Cleanup(state, prog, ctx);
}

TEST_F(MicrobitSpirvParserStateManagementTest, StateDerivativeGroups) {
    struct R_Microbit_SpirvParserContext* ctx = CreateContext();
    const uint32_t spv[] = {
        0x07230203, 0x00010500, 0x000d000b, 0x00000010, 0x00000000,
        0x00020011, 0x00000001,
    };
    struct R_Microbit_SpirvParserProgram* prog = CreateProgram(ctx, spv, 7);
    struct R_Microbit_SpirvParserState* state = CreateState(prog);

    ASSERT_NE(nullptr, state);
    EXPECT_EQ(nullptr, state->pDerivativeGroupX);
    EXPECT_EQ(nullptr, state->pDerivativeGroupY);
    EXPECT_EQ(nullptr, state->pDerivativeGroupD);
    EXPECT_EQ(0u, state->derivativeIsGroupMember);
    EXPECT_EQ(0u, state->derivativeUsed);

    Cleanup(state, prog, ctx);
}

TEST_F(MicrobitSpirvParserStateManagementTest, StateInstructionCount) {
    struct R_Microbit_SpirvParserContext* ctx = CreateContext();
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
    struct R_Microbit_SpirvParserProgram* prog = CreateProgram(ctx, spv, 32);
    struct R_Microbit_SpirvParserState* state = CreateState(prog);

    ASSERT_NE(nullptr, state);
    EXPECT_EQ(0u, state->instructionCount);

    R_Microbit_SpirvParserStatePrepare(state, 0x0f);
    EXPECT_EQ(0u, state->instructionCount);

    Cleanup(state, prog, ctx);
}

TEST_F(MicrobitSpirvParserStateManagementTest, StateCodeCurrentAdvances) {
    struct R_Microbit_SpirvParserContext* ctx = CreateContext();
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
    struct R_Microbit_SpirvParserProgram* prog = CreateProgram(ctx, spv, 32);
    struct R_Microbit_SpirvParserState* state = CreateState(prog);

    ASSERT_NE(nullptr, state);
    const uint32_t* initialCode = state->pCodeCurrent;
    EXPECT_NE(nullptr, initialCode);

    R_Microbit_SpirvParserStatePrepare(state, 0x0f);
    EXPECT_NE(initialCode, state->pCodeCurrent);

    Cleanup(state, prog, ctx);
}

TEST_F(MicrobitSpirvParserStateManagementTest, StateJumpToInstruction) {
    struct R_Microbit_SpirvParserContext* ctx = CreateContext();
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
    struct R_Microbit_SpirvParserProgram* prog = CreateProgram(ctx, spv, 32);
    struct R_Microbit_SpirvParserState* state = CreateState(prog);

    ASSERT_NE(nullptr, state);
    R_Microbit_SpirvParserStatePrepare(state, 0x0f);
    uint32_t initialCount = state->instructionCount;

    R_Microbit_SpirvParserStateJumpToInstruction(state, initialCount + 1);
    EXPECT_GE(state->instructionCount, initialCount);

    Cleanup(state, prog, ctx);
}

TEST_F(MicrobitSpirvParserStateManagementTest, StateGetResultByName) {
    struct R_Microbit_SpirvParserContext* ctx = CreateContext();
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
    struct R_Microbit_SpirvParserProgram* prog = CreateProgram(ctx, spv, 32);
    struct R_Microbit_SpirvParserState* state = CreateState(prog);

    ASSERT_NE(nullptr, state);
    struct R_Microbit_SpirvParserResult* result = R_Microbit_SpirvParserStateGetResult(state, "nonexistent");
    EXPECT_EQ(nullptr, result);

    Cleanup(state, prog, ctx);
}

}  // namespace