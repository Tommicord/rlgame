#include <benchmark/benchmark.h>

extern "C" {
#include "microbit/spirvrunner/microbit_spirv_runner.h"
#include "microbit/microbit_spirv_parser.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"
}

extern const uint32_t testTriangleVert_size;
extern const uint32_t testTriangleVert_data[];
extern const uint32_t testTriangleFrag_size;
extern const uint32_t testTriangleFrag_data[];
extern const uint32_t testTriangle3dVert_size;
extern const uint32_t testTriangle3dVert_data[];
extern const uint32_t testTriangle3dFrag_size;
extern const uint32_t testTriangle3dFrag_data[];
extern const uint32_t testMeshVert_size;
extern const uint32_t testMeshVert_data[];
extern const uint32_t testPbrFrag_size;
extern const uint32_t testPbrFrag_data[];
extern const uint32_t testComputeComp_size;
extern const uint32_t testComputeComp_data[];
extern const uint32_t testComputeVert_size;
extern const uint32_t testComputeVert_data[];
extern const uint32_t testGeometryGeom_size;
extern const uint32_t testGeometryGeom_data[];
extern const uint32_t testTessTesc_size;
extern const uint32_t testTessTesc_data[];
extern const uint32_t testTessTese_size;
extern const uint32_t testTessTese_data[];

namespace {

constexpr size_t kBenchHeapSize = 16 * 1024 * 1024;

struct RunnerFixture : public benchmark::Fixture {
    struct R_Microbit_SpirvRunnerContext* ctx = nullptr;
    struct R_Microbit_SpirvParserProgram* vertProg = nullptr;
    struct R_Microbit_SpirvParserProgram* fragProg = nullptr;
    struct R_Microbit_SpirvRunnerProgram* vertRunnerProg = nullptr;
    struct R_Microbit_SpirvRunnerProgram* fragRunnerProg = nullptr;
    struct R_Microbit_SpirvRunnerExecution* vertExec = nullptr;
    struct R_Microbit_SpirvRunnerExecution* fragExec = nullptr;

    void SetUp(::benchmark::State& state) override {
        if (R_CSTL_HeapInit(kBenchHeapSize) != 0) {
            state.SkipWithError("Heap init failed");
            return;
        }
        ctx = R_Microbit_NewSpirvRunnerContext();
        if (!ctx) {
            R_CSTL_HeapShutdown();
            state.SkipWithError("Runner context creation failed");
            return;
        }
    }

    void TearDown(const ::benchmark::State&) override {
        if (vertExec) R_Microbit_DeleteSpirvRunnerExecution(vertExec);
        if (fragExec) R_Microbit_DeleteSpirvRunnerExecution(fragExec);
        if (vertRunnerProg) R_Microbit_DeleteSpirvRunnerProgram(vertRunnerProg);
        if (fragRunnerProg) R_Microbit_DeleteSpirvRunnerProgram(fragRunnerProg);
        if (vertProg) R_Microbit_DeleteSpirvParserProgram(vertProg);
        if (fragProg) R_Microbit_DeleteSpirvParserProgram(fragProg);
        if (ctx) R_Microbit_DeleteSpirvRunnerContext(ctx);
        R_CSTL_HeapShutdown();
    }

    struct R_Microbit_SpirvParserProgram* CreateProgram(const uint32_t* data, uint32_t size) {
        return R_Microbit_NewSpirvParserProgram(ctx->pParserContext, data, size / 4);
    }

    struct R_Microbit_SpirvRunnerProgram* CreateRunnerProgram(struct R_Microbit_SpirvParserProgram* prog, uint32_t entryPoint) {
        struct R_Microbit_SpirvRunnerProgram* runnerProg = nullptr;
        R_Microbit_NewSpirvRunnerProgram(ctx, prog, entryPoint, &runnerProg);
        return runnerProg;
    }

    struct R_Microbit_SpirvRunnerExecution* CreateExecution(struct R_Microbit_SpirvRunnerProgram* runnerProg) {
        struct R_Microbit_SpirvRunnerExecution* exec = nullptr;
        R_Microbit_NewSpirvRunnerExecution(runnerProg, &exec);
        return exec;
    }
};

// Benchmark: Parse and execute simple vertex shader
BENCHMARK_DEFINE_F(RunnerFixture, SimpleVertexShader_ParseAndExecute)(benchmark::State& state) {
    vertProg = CreateProgram(testTriangleVert_data, testTriangleVert_size);
    if (!vertProg) { state.SkipWithError("Failed to create vertex program"); return; }
    
    vertRunnerProg = CreateRunnerProgram(vertProg, 0x5); // Entry point ID from test shaders
    if (!vertRunnerProg) { state.SkipWithError("Failed to create runner program"); return; }
    
    vertExec = CreateExecution(vertRunnerProg);
    if (!vertExec) { state.SkipWithError("Failed to create execution"); return; }

    for (auto _ : state) {
        // Recreate execution for each iteration to ensure clean state
        R_Microbit_DeleteSpirvRunnerExecution(vertExec);
        vertExec = CreateExecution(vertRunnerProg);
        if (!vertExec) { state.SkipWithError("Failed to recreate execution"); return; }
        enum R_Microbit_SpirvRunnerError err = R_Microbit_SpirvRunnerExecute(vertExec, 10000);
        if (err != MICROBIT_SPIRV_RUNNER_OK) {
            char buf[128];
            snprintf(buf, sizeof(buf), "Execution failed with error %d", err);
            state.SkipWithError(buf);
            return;
        }
        benchmark::DoNotOptimize(vertExec->instructionCount);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(RunnerFixture, SimpleVertexShader_ParseAndExecute)->Unit(benchmark::kMicrosecond);

// Benchmark: Parse and execute simple fragment shader
BENCHMARK_DEFINE_F(RunnerFixture, SimpleFragmentShader_ParseAndExecute)(benchmark::State& state) {
    fragProg = CreateProgram(testTriangleFrag_data, testTriangleFrag_size);
    if (!fragProg) { state.SkipWithError("Failed to create fragment program"); return; }
    
    fragRunnerProg = CreateRunnerProgram(fragProg, 0x5);
    if (!fragRunnerProg) { state.SkipWithError("Failed to create runner program"); return; }
    
    fragExec = CreateExecution(fragRunnerProg);
    if (!fragExec) { state.SkipWithError("Failed to create execution"); return; }

    for (auto _ : state) {
        // Recreate execution for each iteration to ensure clean state
        R_Microbit_DeleteSpirvRunnerExecution(fragExec);
        fragExec = CreateExecution(fragRunnerProg);
        if (!fragExec) { state.SkipWithError("Failed to recreate execution"); return; }
        enum R_Microbit_SpirvRunnerError err = R_Microbit_SpirvRunnerExecute(fragExec, 10000);
        if (err != MICROBIT_SPIRV_RUNNER_OK) {
            char buf[128];
            snprintf(buf, sizeof(buf), "Execution failed with error %d", err);
            state.SkipWithError(buf);
            return;
        }
        benchmark::DoNotOptimize(fragExec->instructionCount);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(RunnerFixture, SimpleFragmentShader_ParseAndExecute)->Unit(benchmark::kMicrosecond);

// Benchmark: Parse and execute 3D vertex shader
BENCHMARK_DEFINE_F(RunnerFixture, MeshVertexShader_ParseAndExecute)(benchmark::State& state) {
    vertProg = CreateProgram(testTriangle3dVert_data, testTriangle3dVert_size);
    if (!vertProg) { state.SkipWithError("Failed to create mesh vertex program"); return; }
    
    vertRunnerProg = CreateRunnerProgram(vertProg, 0x5);
    if (!vertRunnerProg) { state.SkipWithError("Failed to create runner program"); return; }
    
    vertExec = CreateExecution(vertRunnerProg);
    if (!vertExec) { state.SkipWithError("Failed to create execution"); return; }

    for (auto _ : state) {
        // Recreate execution for each iteration to ensure clean state
        R_Microbit_DeleteSpirvRunnerExecution(vertExec);
        vertExec = CreateExecution(vertRunnerProg);
        if (!vertExec) { state.SkipWithError("Failed to recreate execution"); return; }
        enum R_Microbit_SpirvRunnerError err = R_Microbit_SpirvRunnerExecute(vertExec, 10000);
        if (err != MICROBIT_SPIRV_RUNNER_OK) {
            char buf[128];
            snprintf(buf, sizeof(buf), "Execution failed with error %d", err);
            state.SkipWithError(buf);
            return;
        }
        benchmark::DoNotOptimize(vertExec->instructionCount);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(RunnerFixture, MeshVertexShader_ParseAndExecute)->Unit(benchmark::kMicrosecond);

// Benchmark: Parse and execute 3D fragment shader
BENCHMARK_DEFINE_F(RunnerFixture, MeshFragmentShader_ParseAndExecute)(benchmark::State& state) {
    fragProg = CreateProgram(testTriangle3dFrag_data, testTriangle3dFrag_size);
    if (!fragProg) { state.SkipWithError("Failed to create mesh fragment program"); return; }
    
    fragRunnerProg = CreateRunnerProgram(fragProg, 0x5);
    if (!fragRunnerProg) { state.SkipWithError("Failed to create runner program"); return; }
    
    fragExec = CreateExecution(fragRunnerProg);
    if (!fragExec) { state.SkipWithError("Failed to create execution"); return; }

    for (auto _ : state) {
        // Recreate execution for each iteration to ensure clean state
        R_Microbit_DeleteSpirvRunnerExecution(fragExec);
        fragExec = CreateExecution(fragRunnerProg);
        if (!fragExec) { state.SkipWithError("Failed to recreate execution"); return; }
        enum R_Microbit_SpirvRunnerError err = R_Microbit_SpirvRunnerExecute(fragExec, 10000);
        if (err != MICROBIT_SPIRV_RUNNER_OK) {
            char buf[128];
            snprintf(buf, sizeof(buf), "Execution failed with error %d", err);
            state.SkipWithError(buf);
            return;
        }
        benchmark::DoNotOptimize(fragExec->instructionCount);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(RunnerFixture, MeshFragmentShader_ParseAndExecute)->Unit(benchmark::kMicrosecond);

// Benchmark: Parse and execute mesh vertex shader (with matrices)
BENCHMARK_DEFINE_F(RunnerFixture, MeshWithMatrices_ParseAndExecute)(benchmark::State& state) {
    vertProg = CreateProgram(testMeshVert_data, testMeshVert_size);
    if (!vertProg) { state.SkipWithError("Failed to create mesh vertex program"); return; }
    
    vertRunnerProg = CreateRunnerProgram(vertProg, 0x5);
    if (!vertRunnerProg) { state.SkipWithError("Failed to create runner program"); return; }
    
    vertExec = CreateExecution(vertRunnerProg);
    if (!vertExec) { state.SkipWithError("Failed to create execution"); return; }

    for (auto _ : state) {
        // Recreate execution for each iteration to ensure clean state
        R_Microbit_DeleteSpirvRunnerExecution(vertExec);
        vertExec = CreateExecution(vertRunnerProg);
        if (!vertExec) { state.SkipWithError("Failed to recreate execution"); return; }
        enum R_Microbit_SpirvRunnerError err = R_Microbit_SpirvRunnerExecute(vertExec, 10000);
        if (err != MICROBIT_SPIRV_RUNNER_OK) {
            char buf[128];
            snprintf(buf, sizeof(buf), "Execution failed with error %d", err);
            state.SkipWithError(buf);
            return;
        }
        benchmark::DoNotOptimize(vertExec->instructionCount);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(RunnerFixture, MeshWithMatrices_ParseAndExecute)->Unit(benchmark::kMicrosecond);

// Benchmark: Parse and execute PBR fragment shader (complex)
BENCHMARK_DEFINE_F(RunnerFixture, PbrFragmentShader_ParseAndExecute)(benchmark::State& state) {
    fragProg = CreateProgram(testPbrFrag_data, testPbrFrag_size);
    if (!fragProg) { state.SkipWithError("Failed to create PBR fragment program"); return; }
    
    fragRunnerProg = CreateRunnerProgram(fragProg, 0x5);
    if (!fragRunnerProg) { state.SkipWithError("Failed to create runner program"); return; }
    
    fragExec = CreateExecution(fragRunnerProg);
    if (!fragExec) { state.SkipWithError("Failed to create execution"); return; }

    for (auto _ : state) {
        // Recreate execution for each iteration to ensure clean state
        R_Microbit_DeleteSpirvRunnerExecution(fragExec);
        fragExec = CreateExecution(fragRunnerProg);
        if (!fragExec) { state.SkipWithError("Failed to recreate execution"); return; }
        enum R_Microbit_SpirvRunnerError err = R_Microbit_SpirvRunnerExecute(fragExec, 10000);
        if (err != MICROBIT_SPIRV_RUNNER_OK) {
            char buf[128];
            snprintf(buf, sizeof(buf), "Execution failed with error %d", err);
            state.SkipWithError(buf);
            return;
        }
        benchmark::DoNotOptimize(fragExec->instructionCount);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(RunnerFixture, PbrFragmentShader_ParseAndExecute)->Unit(benchmark::kMicrosecond);

// Benchmark: Step-by-step execution (instruction level)
BENCHMARK_DEFINE_F(RunnerFixture, SimpleVertexShader_StepByStep)(benchmark::State& state) {
    vertProg = CreateProgram(testTriangleVert_data, testTriangleVert_size);
    if (!vertProg) { state.SkipWithError("Failed to create vertex program"); return; }
    
    vertRunnerProg = CreateRunnerProgram(vertProg, 0x5);
    if (!vertRunnerProg) { state.SkipWithError("Failed to create runner program"); return; }
    
    vertExec = CreateExecution(vertRunnerProg);
    if (!vertExec) { state.SkipWithError("Failed to create execution"); return; }

    for (auto _ : state) {
        vertExec->instructionCount = 0;
        vertExec->didComplete = 0;
        vertExec->pState->pCodeCurrent = vertRunnerProg->pEntryCode;
        vertExec->pState->functionStackCurrent = 0;
        vertExec->pState->didJump = 0;
        vertExec->pState->discarded = 0;
        
        while (!vertExec->didComplete && vertExec->instructionCount < 10000) {
            enum R_Microbit_SpirvRunnerError err = R_Microbit_SpirvRunnerStep(vertExec);
            if (err != MICROBIT_SPIRV_RUNNER_OK) {
                state.SkipWithError("Step execution failed");
                return;
            }
        }
        benchmark::DoNotOptimize(vertExec->instructionCount);
    }
    state.SetItemsProcessed(state.iterations() * vertExec->instructionCount);
}
BENCHMARK_REGISTER_F(RunnerFixture, SimpleVertexShader_StepByStep)->Unit(benchmark::kMicrosecond);

// Benchmark: Context creation overhead
BENCHMARK_DEFINE_F(RunnerFixture, ContextCreation)(benchmark::State& state) {
    for (auto _ : state) {
        struct R_Microbit_SpirvRunnerContext* newCtx = R_Microbit_NewSpirvRunnerContext();
        benchmark::DoNotOptimize(newCtx);
        if (newCtx) R_Microbit_DeleteSpirvRunnerContext(newCtx);
    }
}
BENCHMARK_REGISTER_F(RunnerFixture, ContextCreation)->Unit(benchmark::kMicrosecond);

// Benchmark: Program creation overhead
BENCHMARK_DEFINE_F(RunnerFixture, ProgramCreation)(benchmark::State& state) {
    for (auto _ : state) {
        struct R_Microbit_SpirvParserProgram* prog = CreateProgram(testTriangleVert_data, testTriangleVert_size);
        benchmark::DoNotOptimize(prog);
        if (prog) R_Microbit_DeleteSpirvParserProgram(prog);
    }
}
BENCHMARK_REGISTER_F(RunnerFixture, ProgramCreation)->Unit(benchmark::kMicrosecond);

// Benchmark: Runner program creation overhead
BENCHMARK_DEFINE_F(RunnerFixture, RunnerProgramCreation)(benchmark::State& state) {
    vertProg = CreateProgram(testTriangleVert_data, testTriangleVert_size);
    if (!vertProg) { state.SkipWithError("Failed to create vertex program"); return; }

    for (auto _ : state) {
        struct R_Microbit_SpirvRunnerProgram* runnerProg = CreateRunnerProgram(vertProg, 0x5);
        benchmark::DoNotOptimize(runnerProg);
        if (runnerProg) R_Microbit_DeleteSpirvRunnerProgram(runnerProg);
    }
    R_Microbit_DeleteSpirvParserProgram(vertProg);
    vertProg = nullptr;
}
BENCHMARK_REGISTER_F(RunnerFixture, RunnerProgramCreation)->Unit(benchmark::kMicrosecond);

// Benchmark: Execution creation overhead
BENCHMARK_DEFINE_F(RunnerFixture, ExecutionCreation)(benchmark::State& state) {
    vertProg = CreateProgram(testTriangleVert_data, testTriangleVert_size);
    if (!vertProg) { state.SkipWithError("Failed to create vertex program"); return; }
    vertRunnerProg = CreateRunnerProgram(vertProg, 0x5);
    if (!vertRunnerProg) { state.SkipWithError("Failed to create runner program"); return; }

    for (auto _ : state) {
        struct R_Microbit_SpirvRunnerExecution* exec = CreateExecution(vertRunnerProg);
        benchmark::DoNotOptimize(exec);
        if (exec) R_Microbit_DeleteSpirvRunnerExecution(exec);
    }
    R_Microbit_DeleteSpirvRunnerProgram(vertRunnerProg);
    R_Microbit_DeleteSpirvParserProgram(vertProg);
    vertRunnerProg = nullptr;
    vertProg = nullptr;
}
BENCHMARK_REGISTER_F(RunnerFixture, ExecutionCreation)->Unit(benchmark::kMicrosecond);

// Benchmark: Full pipeline (context -> program -> runner -> execute)
BENCHMARK_DEFINE_F(RunnerFixture, FullPipeline_SimpleVertex)(benchmark::State& state) {
    for (auto _ : state) {
        struct R_Microbit_SpirvRunnerContext* newCtx = R_Microbit_NewSpirvRunnerContext();
        if (!newCtx) { state.SkipWithError("Context creation failed"); return; }
        
        struct R_Microbit_SpirvParserProgram* prog = R_Microbit_NewSpirvParserProgram(newCtx->pParserContext, testTriangleVert_data, testTriangleVert_size / 4);
        if (!prog) { 
            R_Microbit_DeleteSpirvRunnerContext(newCtx);
            state.SkipWithError("Program creation failed"); 
            return; 
        }
        
        struct R_Microbit_SpirvRunnerProgram* runnerProg = nullptr;
        R_Microbit_NewSpirvRunnerProgram(newCtx, prog, 0x5, &runnerProg);
        if (!runnerProg) {
            R_Microbit_DeleteSpirvParserProgram(prog);
            R_Microbit_DeleteSpirvRunnerContext(newCtx);
            state.SkipWithError("Runner program creation failed");
            return;
        }
        
        struct R_Microbit_SpirvRunnerExecution* exec = nullptr;
        R_Microbit_NewSpirvRunnerExecution(runnerProg, &exec);
        if (!exec) {
            R_Microbit_DeleteSpirvRunnerProgram(runnerProg);
            R_Microbit_DeleteSpirvParserProgram(prog);
            R_Microbit_DeleteSpirvRunnerContext(newCtx);
            state.SkipWithError("Execution creation failed");
            return;
        }
        
        enum R_Microbit_SpirvRunnerError err = R_Microbit_SpirvRunnerExecute(exec, 10000);
        if (err != MICROBIT_SPIRV_RUNNER_OK) {
            R_Microbit_DeleteSpirvRunnerExecution(exec);
            R_Microbit_DeleteSpirvRunnerProgram(runnerProg);
            R_Microbit_DeleteSpirvParserProgram(prog);
            R_Microbit_DeleteSpirvRunnerContext(newCtx);
            state.SkipWithError("Execution failed");
            return;
        }
        
        benchmark::DoNotOptimize(exec->instructionCount);
        
        R_Microbit_DeleteSpirvRunnerExecution(exec);
        R_Microbit_DeleteSpirvRunnerProgram(runnerProg);
        R_Microbit_DeleteSpirvParserProgram(prog);
        R_Microbit_DeleteSpirvRunnerContext(newCtx);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(RunnerFixture, FullPipeline_SimpleVertex)->Unit(benchmark::kMicrosecond);

// Benchmark: Repeated executions without recreation
BENCHMARK_DEFINE_F(RunnerFixture, RepeatedExecution_SimpleVertex)(benchmark::State& state) {
    vertProg = CreateProgram(testTriangleVert_data, testTriangleVert_size);
    if (!vertProg) { state.SkipWithError("Failed to create vertex program"); return; }
    vertRunnerProg = CreateRunnerProgram(vertProg, 0x5);
    if (!vertRunnerProg) { state.SkipWithError("Failed to create runner program"); return; }
    vertExec = CreateExecution(vertRunnerProg);
    if (!vertExec) { state.SkipWithError("Failed to create execution"); return; }

    for (auto _ : state) {
        // Recreate execution for each iteration to ensure clean state
        R_Microbit_DeleteSpirvRunnerExecution(vertExec);
        vertExec = CreateExecution(vertRunnerProg);
        if (!vertExec) { state.SkipWithError("Failed to recreate execution"); return; }
        enum R_Microbit_SpirvRunnerError err = R_Microbit_SpirvRunnerExecute(vertExec, 10000);
        if (err != MICROBIT_SPIRV_RUNNER_OK) {
            char buf[128];
            snprintf(buf, sizeof(buf), "Execution failed with error %d", err);
            state.SkipWithError(buf);
            return;
        }
        benchmark::DoNotOptimize(vertExec->instructionCount);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(RunnerFixture, RepeatedExecution_SimpleVertex)->Unit(benchmark::kMicrosecond);

} // namespace