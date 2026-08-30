#include <gtest/gtest.h>

extern "C" {
#include "microbit/spirvrunner/microbit_spirv_runner.h"
#include "microbit/microbit_spirv_parser.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"
}

namespace {

constexpr size_t kTestHeapSize = 4 * 1024 * 1024;

class MicrobitSpirvRunnerTest : public ::testing::Test {
protected:
    void SetUp() override {
        ASSERT_EQ(0, R_CSTL_HeapInit(kTestHeapSize));
    }

    void TearDown() override {
        R_CSTL_HeapShutdown();
    }

    struct R_Microbit_SpirvRunnerContext* CreateRunnerContext() {
        return R_Microbit_NewSpirvRunnerContext();
    }
};

TEST_F(MicrobitSpirvRunnerTest, NewRunnerContextNotNull) {
    struct R_Microbit_SpirvRunnerContext* ctx = CreateRunnerContext();
    ASSERT_NE(nullptr, ctx);
    EXPECT_NE(nullptr, ctx->pParserContext);
    EXPECT_EQ(4096u, ctx->maxInstructions);
    EXPECT_EQ(4096u, ctx->instructionLimit);

    R_Microbit_DeleteSpirvRunnerContext(ctx);
}

TEST_F(MicrobitSpirvRunnerTest, NewRunnerProgramNullContext) {
    const uint32_t spv[] = {0x07230203, 0x00010500, 0x000d000b, 0x00000010, 0x00000000};

    struct R_Microbit_SpirvRunnerContext* ctx = CreateRunnerContext();
    struct R_Microbit_SpirvParserContext* parserCtx = R_Microbit_NewSpirvParserContext();
    struct R_Microbit_SpirvParserProgram* prog = R_Microbit_NewSpirvParserProgram(parserCtx, spv, 5);

    struct R_Microbit_SpirvRunnerProgram* runnerProg = nullptr;
    enum R_Microbit_SpirvRunnerError err = R_Microbit_NewSpirvRunnerProgram(
        nullptr, prog, 0, &runnerProg);

    EXPECT_EQ(MICROBIT_SPIRV_RUNNER_ERROR_NULL_POINTER, err);
    EXPECT_EQ(nullptr, runnerProg);

    R_Microbit_DeleteSpirvParserProgram(prog);
    R_Microbit_DeleteSpirvParserContext(parserCtx);
    R_Microbit_DeleteSpirvRunnerContext(ctx);
}

TEST_F(MicrobitSpirvRunnerTest, NewRunnerProgramNullProgram) {
    struct R_Microbit_SpirvRunnerContext* ctx = CreateRunnerContext();

    struct R_Microbit_SpirvRunnerProgram* runnerProg = nullptr;
    enum R_Microbit_SpirvRunnerError err = R_Microbit_NewSpirvRunnerProgram(
        ctx, nullptr, 0, &runnerProg);

    EXPECT_EQ(MICROBIT_SPIRV_RUNNER_ERROR_NULL_POINTER, err);
    EXPECT_EQ(nullptr, runnerProg);

    R_Microbit_DeleteSpirvRunnerContext(ctx);
}

TEST_F(MicrobitSpirvRunnerTest, NewRunnerProgramNullOutput) {
    struct R_Microbit_SpirvRunnerContext* ctx = CreateRunnerContext();
    const uint32_t spv[] = {0x07230203, 0x00010500, 0x000d000b, 0x00000010, 0x00000000};
    struct R_Microbit_SpirvParserContext* parserCtx = R_Microbit_NewSpirvParserContext();
    struct R_Microbit_SpirvParserProgram* prog = R_Microbit_NewSpirvParserProgram(parserCtx, spv, 5);

    enum R_Microbit_SpirvRunnerError err = R_Microbit_NewSpirvRunnerProgram(
        ctx, prog, 0, nullptr);

    EXPECT_EQ(MICROBIT_SPIRV_RUNNER_ERROR_NULL_POINTER, err);

    R_Microbit_DeleteSpirvParserProgram(prog);
    R_Microbit_DeleteSpirvParserContext(parserCtx);
    R_Microbit_DeleteSpirvRunnerContext(ctx);
}

TEST_F(MicrobitSpirvRunnerTest, NewRunnerProgramInvalidProgram) {
    struct R_Microbit_SpirvRunnerContext* ctx = CreateRunnerContext();
    const uint32_t spv[] = {0x07230203, 0x00010500, 0x000d000b, 0x00000010, 0x00000000};
    struct R_Microbit_SpirvParserContext* parserCtx = R_Microbit_NewSpirvParserContext();
    struct R_Microbit_SpirvParserProgram* prog = R_Microbit_NewSpirvParserProgram(parserCtx, spv, 5);

    struct R_Microbit_SpirvRunnerProgram* runnerProg = nullptr;
    enum R_Microbit_SpirvRunnerError err = R_Microbit_NewSpirvRunnerProgram(
        ctx, prog, 999, &runnerProg);  // Invalid entry point

    EXPECT_EQ(MICROBIT_SPIRV_RUNNER_ERROR_INVALID_PROGRAM, err);
    EXPECT_EQ(nullptr, runnerProg);

    R_Microbit_DeleteSpirvParserProgram(prog);
    R_Microbit_DeleteSpirvParserContext(parserCtx);
    R_Microbit_DeleteSpirvRunnerContext(ctx);
}

TEST_F(MicrobitSpirvRunnerTest, NewRunnerProgramWrongParserContext) {
    struct R_Microbit_SpirvRunnerContext* ctx1 = CreateRunnerContext();
    struct R_Microbit_SpirvRunnerContext* ctx2 = CreateRunnerContext();

    const uint32_t spv[] = {0x07230203, 0x00010500, 0x000d000b, 0x00000010, 0x00000000};
    struct R_Microbit_SpirvParserProgram* prog = R_Microbit_NewSpirvParserProgram(ctx2->pParserContext, spv, 5);

    struct R_Microbit_SpirvRunnerProgram* runnerProg = nullptr;
    enum R_Microbit_SpirvRunnerError err = R_Microbit_NewSpirvRunnerProgram(
        ctx1, prog, 0, &runnerProg);

    EXPECT_EQ(MICROBIT_SPIRV_RUNNER_ERROR_INVALID_PROGRAM, err);
    EXPECT_EQ(nullptr, runnerProg);

    R_Microbit_DeleteSpirvParserProgram(prog);
    R_Microbit_DeleteSpirvRunnerContext(ctx1);
    R_Microbit_DeleteSpirvRunnerContext(ctx2);
}

TEST_F(MicrobitSpirvRunnerTest, DeleteRunnerProgramNullSafe) {
    R_Microbit_DeleteSpirvRunnerProgram(nullptr);
    SUCCEED();
}

TEST_F(MicrobitSpirvRunnerTest, NewRunnerExecutionNullProgram) {
    struct R_Microbit_SpirvRunnerExecution* exec = nullptr;
    enum R_Microbit_SpirvRunnerError err = R_Microbit_NewSpirvRunnerExecution(nullptr, &exec);

    EXPECT_EQ(MICROBIT_SPIRV_RUNNER_ERROR_NULL_POINTER, err);
    EXPECT_EQ(nullptr, exec);
}

TEST_F(MicrobitSpirvRunnerTest, NewRunnerExecutionNullOutput) {
    struct R_Microbit_SpirvRunnerContext* ctx = CreateRunnerContext();
    const uint32_t spv[] = {0x07230203, 0x00010500, 0x000d000b, 0x00000010, 0x00000000};
    struct R_Microbit_SpirvParserContext* parserCtx = R_Microbit_NewSpirvParserContext();
    struct R_Microbit_SpirvParserProgram* prog = R_Microbit_NewSpirvParserProgram(parserCtx, spv, 5);
    struct R_Microbit_SpirvRunnerProgram* runnerProg = nullptr;
    R_Microbit_NewSpirvRunnerProgram(ctx, prog, 0, &runnerProg);

    enum R_Microbit_SpirvRunnerError err = R_Microbit_NewSpirvRunnerExecution(runnerProg, nullptr);
    EXPECT_EQ(MICROBIT_SPIRV_RUNNER_ERROR_NULL_POINTER, err);

    R_Microbit_DeleteSpirvRunnerProgram(runnerProg);
    R_Microbit_DeleteSpirvParserProgram(prog);
    R_Microbit_DeleteSpirvParserContext(parserCtx);
    R_Microbit_DeleteSpirvRunnerContext(ctx);
}

TEST_F(MicrobitSpirvRunnerTest, NewRunnerExecutionInvalidProgram) {
    struct R_Microbit_SpirvRunnerContext* ctx = CreateRunnerContext();
    const uint32_t spv[] = {0x07230203, 0x00010500, 0x000d000b, 0x00000010, 0x00000000};
    struct R_Microbit_SpirvParserContext* parserCtx = R_Microbit_NewSpirvParserContext();
    struct R_Microbit_SpirvParserProgram* prog = R_Microbit_NewSpirvParserProgram(parserCtx, spv, 5);
    struct R_Microbit_SpirvRunnerProgram* runnerProg = (struct R_Microbit_SpirvRunnerProgram*)R_CSTL_HeapAlloc(sizeof(*runnerProg));
    memset(runnerProg, 0, sizeof(*runnerProg));

    struct R_Microbit_SpirvRunnerExecution* exec = nullptr;
    enum R_Microbit_SpirvRunnerError err = R_Microbit_NewSpirvRunnerExecution(runnerProg, &exec);

    EXPECT_EQ(MICROBIT_SPIRV_RUNNER_ERROR_INVALID_PROGRAM, err);
    EXPECT_EQ(nullptr, exec);

    R_CSTL_HeapFree(runnerProg);
    R_Microbit_DeleteSpirvParserProgram(prog);
    R_Microbit_DeleteSpirvParserContext(parserCtx);
    R_Microbit_DeleteSpirvRunnerContext(ctx);
}

TEST_F(MicrobitSpirvRunnerTest, DeleteRunnerExecutionNullSafe) {
    R_Microbit_DeleteSpirvRunnerExecution(nullptr);
    SUCCEED();
}

TEST_F(MicrobitSpirvRunnerTest, RunnerExecuteNullExecution) {
    enum R_Microbit_SpirvRunnerError err = R_Microbit_SpirvRunnerExecute(nullptr, 100);
    EXPECT_EQ(MICROBIT_SPIRV_RUNNER_ERROR_NULL_POINTER, err);
}

TEST_F(MicrobitSpirvRunnerTest, RunnerExecuteNullState) {
    struct R_Microbit_SpirvRunnerExecution exec = {0};
    enum R_Microbit_SpirvRunnerError err = R_Microbit_SpirvRunnerExecute(&exec, 100);
    EXPECT_EQ(MICROBIT_SPIRV_RUNNER_ERROR_NULL_POINTER, err);
}

TEST_F(MicrobitSpirvRunnerTest, RunnerStepNullExecution) {
    enum R_Microbit_SpirvRunnerError err = R_Microbit_SpirvRunnerStep(nullptr);
    EXPECT_EQ(MICROBIT_SPIRV_RUNNER_ERROR_NULL_POINTER, err);
}

TEST_F(MicrobitSpirvRunnerTest, RunnerStepNullState) {
    struct R_Microbit_SpirvRunnerExecution exec = {0};
    enum R_Microbit_SpirvRunnerError err = R_Microbit_SpirvRunnerStep(&exec);
    EXPECT_EQ(MICROBIT_SPIRV_RUNNER_ERROR_NULL_POINTER, err);
}

TEST_F(MicrobitSpirvRunnerTest, RunnerStepCompletedExecution) {
    struct R_Microbit_SpirvRunnerExecution exec = {0};
    exec.didComplete = 1;
    exec.pState = (struct R_Microbit_SpirvParserState*)0x1;  // Non-null dummy

    enum R_Microbit_SpirvRunnerError err = R_Microbit_SpirvRunnerStep(&exec);
    EXPECT_EQ(MICROBIT_SPIRV_RUNNER_OK, err);
}

}  // namespace