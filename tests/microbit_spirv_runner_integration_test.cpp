#include <gtest/gtest.h>

extern "C" {
#include "microbit/spirvrunner/microbit_spirv_runner.h"
#include "microbit/microbit_spirv_parser.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"
}

extern const uint32_t testTriangleFrag_size;
extern const uint32_t testTriangleFrag_data[];
extern const uint32_t testTriangleVert_size;
extern const uint32_t testTriangleVert_data[];
extern const uint32_t testTriangle3dFrag_size;
extern const uint32_t testTriangle3dFrag_data[];
extern const uint32_t testTriangle3dVert_size;
extern const uint32_t testTriangle3dVert_data[];

namespace {

constexpr size_t kTestHeapSize = 4 * 1024 * 1024;

class MicrobitSpirvRunnerIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        ASSERT_EQ(0, R_CSTL_HeapInit(kTestHeapSize));
    }

    void TearDown() override {
        R_CSTL_HeapShutdown();
    }

    struct R_Microbit_SpirvRunnerContext* CreateContext() {
        return R_Microbit_NewSpirvRunnerContext();
    }

    struct R_Microbit_SpirvParserProgram* CreateProgram(struct R_Microbit_SpirvRunnerContext* ctx, const uint32_t* spv, size_t len) {
        return R_Microbit_NewSpirvParserProgram(ctx->pParserContext, spv, len);
    }

    struct R_Microbit_SpirvRunnerProgram* CreateRunnerProgram(struct R_Microbit_SpirvRunnerContext* ctx, struct R_Microbit_SpirvParserProgram* prog, uint32_t entryPoint) {
        struct R_Microbit_SpirvRunnerProgram* runnerProg = nullptr;
        R_Microbit_NewSpirvRunnerProgram(ctx, prog, entryPoint, &runnerProg);
        return runnerProg;
    }

    struct R_Microbit_SpirvRunnerExecution* CreateExecution(struct R_Microbit_SpirvRunnerProgram* runnerProg) {
        struct R_Microbit_SpirvRunnerExecution* exec = nullptr;
        R_Microbit_NewSpirvRunnerExecution(runnerProg, &exec);
        return exec;
    }

    void Cleanup(struct R_Microbit_SpirvRunnerExecution* exec, struct R_Microbit_SpirvRunnerProgram* rprog,
                 struct R_Microbit_SpirvParserProgram* prog, struct R_Microbit_SpirvRunnerContext* ctx) {
        if (exec) R_Microbit_DeleteSpirvRunnerExecution(exec);
        if (rprog) R_Microbit_DeleteSpirvRunnerProgram(rprog);
        if (prog) R_Microbit_DeleteSpirvParserProgram(prog);
        if (ctx) R_Microbit_DeleteSpirvRunnerContext(ctx);
    }

    struct R_Microbit_SpirvParserProgram* CreateProgramFromEmbedded(struct R_Microbit_SpirvRunnerContext* ctx, const uint32_t* data, uint32_t size) {
        return R_Microbit_NewSpirvParserProgram(ctx->pParserContext, data, size / 4);
    }
};

TEST_F(MicrobitSpirvRunnerIntegrationTest, TriangleVertexShader_Basic) {
    struct R_Microbit_SpirvRunnerContext* ctx = CreateContext();
    struct R_Microbit_SpirvParserProgram* prog = CreateProgramFromEmbedded(ctx, testTriangleVert_data, testTriangleVert_size);
    ASSERT_NE(nullptr, prog);

    struct R_Microbit_SpirvRunnerProgram* rprog = CreateRunnerProgram(ctx, prog, 0x5);
    struct R_Microbit_SpirvRunnerExecution* exec = CreateExecution(rprog);

    ASSERT_NE(nullptr, exec);
    enum R_Microbit_SpirvRunnerError err = R_Microbit_SpirvRunnerExecute(exec, 10000);
    EXPECT_EQ(MICROBIT_SPIRV_RUNNER_OK, err);

    Cleanup(exec, rprog, prog, ctx);
}

TEST_F(MicrobitSpirvRunnerIntegrationTest, TriangleFragmentShader_Basic) {
    struct R_Microbit_SpirvRunnerContext* ctx = CreateContext();
    struct R_Microbit_SpirvParserProgram* prog = CreateProgramFromEmbedded(ctx, testTriangleFrag_data, testTriangleFrag_size);
    ASSERT_NE(nullptr, prog);

    struct R_Microbit_SpirvRunnerProgram* rprog = CreateRunnerProgram(ctx, prog, 0x5);
    struct R_Microbit_SpirvRunnerExecution* exec = CreateExecution(rprog);

    ASSERT_NE(nullptr, exec);
    enum R_Microbit_SpirvRunnerError err = R_Microbit_SpirvRunnerExecute(exec, 10000);
    EXPECT_EQ(MICROBIT_SPIRV_RUNNER_OK, err);

    Cleanup(exec, rprog, prog, ctx);
}

TEST_F(MicrobitSpirvRunnerIntegrationTest, Triangle3DVertexShader_Basic) {
    struct R_Microbit_SpirvRunnerContext* ctx = CreateContext();
    struct R_Microbit_SpirvParserProgram* prog = CreateProgramFromEmbedded(ctx, testTriangle3dVert_data, testTriangle3dVert_size);
    ASSERT_NE(nullptr, prog);

    struct R_Microbit_SpirvRunnerProgram* rprog = CreateRunnerProgram(ctx, prog, 0x5);
    struct R_Microbit_SpirvRunnerExecution* exec = CreateExecution(rprog);

    ASSERT_NE(nullptr, exec);
    enum R_Microbit_SpirvRunnerError err = R_Microbit_SpirvRunnerExecute(exec, 10000);
    EXPECT_EQ(MICROBIT_SPIRV_RUNNER_OK, err);

    Cleanup(exec, rprog, prog, ctx);
}

TEST_F(MicrobitSpirvRunnerIntegrationTest, Triangle3DFragmentShader_Basic) {
    struct R_Microbit_SpirvRunnerContext* ctx = CreateContext();
    struct R_Microbit_SpirvParserProgram* prog = CreateProgramFromEmbedded(ctx, testTriangle3dFrag_data, testTriangle3dFrag_size);
    ASSERT_NE(nullptr, prog);

    struct R_Microbit_SpirvRunnerProgram* rprog = CreateRunnerProgram(ctx, prog, 0x5);
    struct R_Microbit_SpirvRunnerExecution* exec = CreateExecution(rprog);

    ASSERT_NE(nullptr, exec);
    enum R_Microbit_SpirvRunnerError err = R_Microbit_SpirvRunnerExecute(exec, 10000);
    EXPECT_EQ(MICROBIT_SPIRV_RUNNER_OK, err);

    Cleanup(exec, rprog, prog, ctx);
}

TEST_F(MicrobitSpirvRunnerIntegrationTest, RunnerStepByStep) {
    struct R_Microbit_SpirvRunnerContext* ctx = CreateContext();
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
        0x00050051, 0x0000000c, 0x00000011, 0x00000005, 0x00000003,
        0x0005004e, 0x0000000c, 0x00000012, 0x00000010, 0x00000011,
        0x000200f8, 0x0000000f,
    };

    struct R_Microbit_SpirvParserProgram* prog = CreateProgram(ctx, spv, 38);
    struct R_Microbit_SpirvRunnerProgram* rprog = CreateRunnerProgram(ctx, prog, 0x0e);
    struct R_Microbit_SpirvRunnerExecution* exec = CreateExecution(rprog);

    ASSERT_NE(nullptr, exec);

    enum R_Microbit_SpirvRunnerError err = MICROBIT_SPIRV_RUNNER_OK;
    uint32_t steps = 0;
    while (!exec->didComplete && steps < 100) {
        err = R_Microbit_SpirvRunnerStep(exec);
        EXPECT_EQ(MICROBIT_SPIRV_RUNNER_OK, err);
        steps++;
    }

    EXPECT_EQ(MICROBIT_SPIRV_RUNNER_OK, err);
    EXPECT_TRUE(exec->didComplete);
    EXPECT_GT(exec->instructionCount, 0u);

    Cleanup(exec, rprog, prog, ctx);
}

TEST_F(MicrobitSpirvRunnerIntegrationTest, RunnerInstructionLimit) {
    struct R_Microbit_SpirvRunnerContext* ctx = CreateContext();
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
        0x00050051, 0x0000000c, 0x00000011, 0x00000005, 0x00000003,
        0x0005004e, 0x0000000c, 0x00000012, 0x00000010, 0x00000011,
        0x000200f8, 0x0000000f,
    };

    struct R_Microbit_SpirvParserProgram* prog = CreateProgram(ctx, spv, 38);
    struct R_Microbit_SpirvRunnerProgram* rprog = CreateRunnerProgram(ctx, prog, 0x0e);
    struct R_Microbit_SpirvRunnerExecution* exec = CreateExecution(rprog);

    ASSERT_NE(nullptr, exec);

    enum R_Microbit_SpirvRunnerError err = R_Microbit_SpirvRunnerExecute(exec, 1);
    EXPECT_EQ(MICROBIT_SPIRV_RUNNER_OK, err);
    EXPECT_LT(exec->instructionCount, 5u);

    Cleanup(exec, rprog, prog, ctx);
}

}  // namespace