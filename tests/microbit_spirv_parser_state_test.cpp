#include <gtest/gtest.h>

extern "C" {
#include "microbit/microbit_spirv_parser.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"
}

namespace {

constexpr size_t kTestHeapSize = 256 * 1024;

class MicrobitSpirvParserStateTest : public ::testing::Test {
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

    struct R_Microbit_SpirvParserProgram* CreateProgram(const uint32_t* spv, size_t length) {
        struct R_Microbit_SpirvParserContext* ctx = CreateContext();
        struct R_Microbit_SpirvParserProgram* prog = R_Microbit_NewSpirvParserProgram(ctx, spv, length);
        return prog;
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

TEST_F(MicrobitSpirvParserStateTest, NewState_ValidProgram) {
    const uint32_t spv[] = {
        0x07230203, 0x00010500, 0x000d000b, 0x00000010, 0x00000000,
        0x00020011, 0x00000001,  // OpCapability Shader
    };

    struct R_Microbit_SpirvParserContext* ctx = CreateContext();
    struct R_Microbit_SpirvParserProgram* prog = CreateProgram(spv, 7);
    struct R_Microbit_SpirvParserState* state = CreateState(prog);

    ASSERT_NE(nullptr, state);
    EXPECT_EQ(prog, state->pOwner);
    EXPECT_EQ(0u, state->instructionCount);

    Cleanup(state, prog, ctx);
}

TEST_F(MicrobitSpirvParserStateTest, NewState_NullProgramReturnsNull) {
    struct R_Microbit_SpirvParserState* state = R_Microbit_NewSpirvParserState(nullptr);
    EXPECT_EQ(nullptr, state);
}

TEST_F(MicrobitSpirvParserStateTest, DeleteState_NullSafe) {
    R_Microbit_DeleteSpirvParserState(nullptr);
    SUCCEED();
}

TEST_F(MicrobitSpirvParserStateTest, StateSetExtension) {
    const uint32_t spv[] = {
        0x07230203, 0x00010500, 0x000d000b, 0x00000010, 0x00000000,
        0x00020011, 0x00000001,
    };

    struct R_Microbit_SpirvParserContext* ctx = CreateContext();
    struct R_Microbit_SpirvParserProgram* prog = CreateProgram(spv, 7);
    struct R_Microbit_SpirvParserState* state = CreateState(prog);
    ASSERT_NE(nullptr, state);

    int extValue = 42;
    R_Microbit_SpirvParserStateSetExtension(state, "TestExt", &extValue);

    Cleanup(state, prog, ctx);
}

TEST_F(MicrobitSpirvParserStateTest, StatePrepare) {
    const uint32_t spv[] = {
        0x07230203, 0x00010500, 0x000d000b, 0x00000020, 0x00000000,
        0x00020011, 0x00000001,  // OpCapability Shader
        0x00030002, 0x0000000a, 0x00000000,  // OpExtInstImport "GLSL.std.450"
        0x00030003, 0x0000000b, 0x000001c2,  // OpMemoryModel Logical GLSL450
        0x00030036, 0x0000000c, 0x00000001,  // OpEntryPoint Vertex %main "main"
        0x00020013, 0x00000001,              // OpExecutionMode %main OriginUpperLeft
        0x00030003, 0x0000000d, 0x00000019,  // OpTypeVoid
        0x00040017, 0x0000000e, 0x0000000d, 0x00000000,  // OpTypeFunction void
        0x00040036, 0x0000000f, 0x0000000e, 0x0000000c,  // OpFunction %main None %void
        0x000200f8, 0x0000000f,              // OpFunctionEnd
    };

    struct R_Microbit_SpirvParserContext* ctx = CreateContext();
    struct R_Microbit_SpirvParserProgram* prog = CreateProgram(spv, 34);
    struct R_Microbit_SpirvParserState* state = CreateState(prog);
    ASSERT_NE(nullptr, state);

    R_Microbit_SpirvParserStatePrepare(state, 0x0f);  // Function %main

    EXPECT_EQ(0u, state->functionStackCurrent);
    EXPECT_EQ(0u, state->didJump);
    EXPECT_EQ(0u, state->discarded);
    EXPECT_EQ(0u, state->instructionCount);

    Cleanup(state, prog, ctx);
}

TEST_F(MicrobitSpirvParserStateTest, StateSetFragCoord) {
    const uint32_t spv[] = {
        0x07230203, 0x00010500, 0x000d000b, 0x00000010, 0x00000000,
        0x00020011, 0x00000001,
    };

    struct R_Microbit_SpirvParserContext* ctx = CreateContext();
    struct R_Microbit_SpirvParserProgram* prog = CreateProgram(spv, 7);
    struct R_Microbit_SpirvParserState* state = CreateState(prog);
    ASSERT_NE(nullptr, state);

    R_Microbit_SpirvParserStateSetFragCoord(state, 100.0f, 200.0f, 0.5f, 1.0f);

    EXPECT_FLOAT_EQ(100.0f, state->fragCoord[0]);
    EXPECT_FLOAT_EQ(200.0f, state->fragCoord[1]);
    EXPECT_FLOAT_EQ(0.5f, state->fragCoord[2]);
    EXPECT_FLOAT_EQ(1.0f, state->fragCoord[3]);

    Cleanup(state, prog, ctx);
}

TEST_F(MicrobitSpirvParserStateTest, StateStepOpcode_NullStateNoOp) {
    R_Microbit_SpirvParserStateStepOpcode(nullptr);
    SUCCEED();
}

TEST_F(MicrobitSpirvParserStateTest, StateStepInto) {
    const uint32_t spv[] = {
        0x07230203, 0x00010500, 0x000d000b, 0x00000010, 0x00000000,
        0x00020011, 0x00000001,
    };

    struct R_Microbit_SpirvParserContext* ctx = CreateContext();
    struct R_Microbit_SpirvParserProgram* prog = CreateProgram(spv, 7);
    struct R_Microbit_SpirvParserState* state = CreateState(prog);
    ASSERT_NE(nullptr, state);

    R_Microbit_SpirvParserStateStepInto(state);

    Cleanup(state, prog, ctx);
}

TEST_F(MicrobitSpirvParserStateTest, StateJumpTo) {
    const uint32_t spv[] = {
        0x07230203, 0x00010500, 0x000d000b, 0x00000010, 0x00000000,
        0x00020011, 0x00000001,
    };

    struct R_Microbit_SpirvParserContext* ctx = CreateContext();
    struct R_Microbit_SpirvParserProgram* prog = CreateProgram(spv, 7);
    struct R_Microbit_SpirvParserState* state = CreateState(prog);
    ASSERT_NE(nullptr, state);

    R_Microbit_SpirvParserStateJumpTo(state, 0);

    Cleanup(state, prog, ctx);
}

TEST_F(MicrobitSpirvParserStateTest, StateJumpToInstruction) {
    const uint32_t spv[] = {
        0x07230203, 0x00010500, 0x000d000b, 0x00000010, 0x00000000,
        0x00020011, 0x00000001,
    };

    struct R_Microbit_SpirvParserContext* ctx = CreateContext();
    struct R_Microbit_SpirvParserProgram* prog = CreateProgram(spv, 7);
    struct R_Microbit_SpirvParserState* state = CreateState(prog);
    ASSERT_NE(nullptr, state);

    R_Microbit_SpirvParserStateJumpToInstruction(state, 5);

    Cleanup(state, prog, ctx);
}

TEST_F(MicrobitSpirvParserStateTest, StateGetResultLocationNotFound) {
    const uint32_t spv[] = {
        0x07230203, 0x00010500, 0x000d000b, 0x00000010, 0x00000000,
        0x00020011, 0x00000001,
    };

    struct R_Microbit_SpirvParserContext* ctx = CreateContext();
    struct R_Microbit_SpirvParserProgram* prog = CreateProgram(spv, 7);
    struct R_Microbit_SpirvParserState* state = CreateState(prog);
    ASSERT_NE(nullptr, state);

    uint32_t loc = R_Microbit_SpirvParserStateGetResultLocation(state, "nonexistent");
    EXPECT_EQ(0xFFFFFFFFu, loc);

    Cleanup(state, prog, ctx);
}

TEST_F(MicrobitSpirvParserStateTest, StateGetResultNullName) {
    const uint32_t spv[] = {
        0x07230203, 0x00010500, 0x000d000b, 0x00000010, 0x00000000,
        0x00020011, 0x00000001,
    };

    struct R_Microbit_SpirvParserContext* ctx = CreateContext();
    struct R_Microbit_SpirvParserProgram* prog = CreateProgram(spv, 7);
    struct R_Microbit_SpirvParserState* state = CreateState(prog);
    ASSERT_NE(nullptr, state);

    struct R_Microbit_SpirvParserResult* result = R_Microbit_SpirvParserStateGetResult(state, nullptr);
    EXPECT_EQ(nullptr, result);

    Cleanup(state, prog, ctx);
}

TEST_F(MicrobitSpirvParserStateTest, StateGetResultWithValue) {
    const uint32_t spv[] = {
        0x07230203, 0x00010500, 0x000d000b, 0x00000010, 0x00000000,
        0x00020011, 0x00000001,
    };

    struct R_Microbit_SpirvParserContext* ctx = CreateContext();
    struct R_Microbit_SpirvParserProgram* prog = CreateProgram(spv, 7);
    struct R_Microbit_SpirvParserState* state = CreateState(prog);
    ASSERT_NE(nullptr, state);

    struct R_Microbit_SpirvParserResult* result = R_Microbit_SpirvParserStateGetResultWithValue(state, "test");
    EXPECT_EQ(nullptr, result);

    Cleanup(state, prog, ctx);
}

TEST_F(MicrobitSpirvParserStateTest, StateGetLocalResult) {
    const uint32_t spv[] = {
        0x07230203, 0x00010500, 0x000d000b, 0x00000010, 0x00000000,
        0x00020011, 0x00000001,
    };

    struct R_Microbit_SpirvParserContext* ctx = CreateContext();
    struct R_Microbit_SpirvParserProgram* prog = CreateProgram(spv, 7);
    struct R_Microbit_SpirvParserState* state = CreateState(prog);
    ASSERT_NE(nullptr, state);

    struct R_Microbit_SpirvParserResult dummyFn;
    struct R_Microbit_SpirvParserResult* result = R_Microbit_SpirvParserStateGetLocalResult(state, &dummyFn, "local");
    EXPECT_EQ(nullptr, result);

    Cleanup(state, prog, ctx);
}

TEST_F(MicrobitSpirvParserStateTest, StateGetObjectMember) {
    const uint32_t spv[] = {
        0x07230203, 0x00010500, 0x000d000b, 0x00000010, 0x00000000,
        0x00020011, 0x00000001,
    };

    struct R_Microbit_SpirvParserContext* ctx = CreateContext();
    struct R_Microbit_SpirvParserProgram* prog = CreateProgram(spv, 7);
    struct R_Microbit_SpirvParserState* state = CreateState(prog);
    ASSERT_NE(nullptr, state);

    struct R_Microbit_SpirvParserResult dummyVar;
    struct R_Microbit_SpirvParserMember* member = R_Microbit_SpirvParserStateGetObjectMember(state, &dummyVar, "member");
    EXPECT_EQ(nullptr, member);

    Cleanup(state, prog, ctx);
}

TEST_F(MicrobitSpirvParserStateTest, StateCallFunction) {
    const uint32_t spv[] = {
        0x07230203, 0x00010500, 0x000d000b, 0x00000020, 0x00000000,
        0x00020011, 0x00000001,
        0x00030002, 0x0000000a, 0x00000000,
        0x00030003, 0x0000000b, 0x000001c2,
        0x00030036, 0x0000000c, 0x00000001,
        0x00020013, 0x00000001,
        0x00030003, 0x0000000d, 0x00000019,
        0x00040017, 0x0000000e, 0x0000000d, 0x00000000,
        0x00040036, 0x0000000f, 0x0000000e, 0x0000000c,
        0x000200f8, 0x0000000f,
    };

    struct R_Microbit_SpirvParserContext* ctx = CreateContext();
    struct R_Microbit_SpirvParserProgram* prog = CreateProgram(spv, 34);
    struct R_Microbit_SpirvParserState* state = CreateState(prog);
    ASSERT_NE(nullptr, state);

    R_Microbit_SpirvParserStatePrepare(state, 0x0f);
    R_Microbit_SpirvParserStateCallFunction(state);

    EXPECT_EQ(nullptr, state->pCodeCurrent);

    Cleanup(state, prog, ctx);
}

}  // namespace