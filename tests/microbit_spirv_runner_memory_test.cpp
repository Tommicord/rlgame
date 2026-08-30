#include <gtest/gtest.h>

extern "C" {
#include "microbit/spirvrunner/microbit_spirv_runner.h"
#include "microbit/microbit_spirv_parser.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"
}

namespace {

constexpr size_t kTestHeapSize = 4 * 1024 * 1024;

class MicrobitSpirvRunnerMemoryTest : public ::testing::Test {
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
};

TEST_F(MicrobitSpirvRunnerMemoryTest, VariablePrivate) {
    const uint32_t spv[] = {
        0x07230203, 0x00010500, 0x000d000b, 0x00000020, 0x00000000,
        0x00020011, 0x00000001,
        0x00030003, 0x0000000a, 0x000001c2,
        0x00030003, 0x0000000b, 0x00000001,
        0x00030036, 0x0000000c, 0x00000001,
        0x00020013, 0x00000001,
        0x00040017, 0x0000000d, 0x00000019, 0x00000000,
        0x00040036, 0x0000000e, 0x0000000d, 0x0000000c,
        0x00040036, 0x00000014, 0x0000000f, 0x0000000c,  // OpFunction %func (result type 0x14, function 0x0f, control 0x0c)
        0x0003003e, 0x00000015,  // OpLabel %label15
        0x00040021, 0x00000007, 0x00000014, 0x00000000,  // OpVariable %ptr Function
        0x00050051, 0x00000007, 0x00000010, 0x00000005, 0x00000000,  // OpStore
        0x0004003d, 0x00000007, 0x00000011, 0x00000010,  // OpLoad
        0x000200f8, 0x0000000f,
        0x00030036, 0x0000000f, 0x00000001,  // OpEntryPoint Vertex %func "main"
    };

    struct R_Microbit_SpirvRunnerContext* ctx = CreateContext();
    struct R_Microbit_SpirvParserProgram* prog = CreateProgram(ctx, spv, 46);
    struct R_Microbit_SpirvRunnerProgram* rprog = CreateRunnerProgram(ctx, prog, 0x0f);
    struct R_Microbit_SpirvRunnerExecution* exec = CreateExecution(rprog);

    ASSERT_NE(nullptr, exec);
    enum R_Microbit_SpirvRunnerError err = R_Microbit_SpirvRunnerExecute(exec, 1000);
    EXPECT_EQ(MICROBIT_SPIRV_RUNNER_OK, err);

    struct R_Microbit_SpirvParserResult* result = R_Microbit_SpirvParserStateGetResult(exec->pState, "11");
    ASSERT_NE(nullptr, result);
    EXPECT_EQ(3u, result->cpuWords[0]);

    Cleanup(exec, rprog, prog, ctx);
}

TEST_F(MicrobitSpirvRunnerMemoryTest, LoadStoreVector) {
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
        0x00050036, 0x0000000c, 0x00000010, 0x00000005, 0x00000000,  // OpConstantComposite
        0x00040021, 0x00000007, 0x00000014, 0x00000000,
        0x00050051, 0x00000007, 0x00000011, 0x00000005, 0x00000000,
        0x0004003d, 0x00000007, 0x00000012, 0x00000011,
        0x000200f8, 0x0000000f,
    };

    struct R_Microbit_SpirvRunnerContext* ctx = CreateContext();
    struct R_Microbit_SpirvParserProgram* prog = CreateProgram(ctx, spv, 44);
    struct R_Microbit_SpirvRunnerProgram* rprog = CreateRunnerProgram(ctx, prog, 0x0e);
    struct R_Microbit_SpirvRunnerExecution* exec = CreateExecution(rprog);

    ASSERT_NE(nullptr, exec);
    enum R_Microbit_SpirvRunnerError err = R_Microbit_SpirvRunnerExecute(exec, 1000);
    EXPECT_EQ(MICROBIT_SPIRV_RUNNER_OK, err);

    struct R_Microbit_SpirvParserResult* result = R_Microbit_SpirvParserStateGetResult(exec->pState, "12");
    ASSERT_NE(nullptr, result);
    EXPECT_EQ(2u, result->cpuComponentCount);

    Cleanup(exec, rprog, prog, ctx);
}

TEST_F(MicrobitSpirvRunnerMemoryTest, AccessChainStruct) {
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
        0x00050046, 0x00000014, 0x00000015, 0x0000000c, 0x00000000,  // OpAccessChain
        0x00050051, 0x0000000c, 0x00000016, 0x00000005, 0x00000000,
        0x00050051, 0x00000015, 0x00000017, 0x00000005, 0x00000000,
        0x0004003d, 0x00000015, 0x00000018, 0x00000017,
        0x000200f8, 0x0000000f,
    };

    struct R_Microbit_SpirvRunnerContext* ctx = CreateContext();
    struct R_Microbit_SpirvParserProgram* prog = CreateProgram(ctx, spv, 44);
    struct R_Microbit_SpirvRunnerProgram* rprog = CreateRunnerProgram(ctx, prog, 0x0e);
    struct R_Microbit_SpirvRunnerExecution* exec = CreateExecution(rprog);

    ASSERT_NE(nullptr, exec);
    enum R_Microbit_SpirvRunnerError err = R_Microbit_SpirvRunnerExecute(exec, 1000);
    EXPECT_EQ(MICROBIT_SPIRV_RUNNER_OK, err);

    Cleanup(exec, rprog, prog, ctx);
}

}  // namespace