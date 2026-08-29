#include <gtest/gtest.h>

extern "C" {
#include "rlgame.base/consoletools/microbit/microbit_spirv_parser.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"
}

namespace {

constexpr size_t kTestHeapSize = 256 * 1024;

class MicrobitSpirvParserEntryPointTest : public ::testing::Test {
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

    void DeleteContext(struct R_Microbit_SpirvParserContext* ctx) {
        R_Microbit_DeleteSpirvParserContext(ctx);
    }

    void DeleteProgram(struct R_Microbit_SpirvParserProgram* prog) {
        R_Microbit_DeleteSpirvParserProgram(prog);
    }
};

TEST_F(MicrobitSpirvParserEntryPointTest, EntryPoint_Vertex) {
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
        0x00050036, 0x00000014, 0x00000010, 0x0000000d, 0x0000000c, 0x0000000c,  // OpFunction
        0x000200f8, 0x00000010,
        0x00040036, 0x00000011, 0x00000014, 0x0000000c,
        0x000200f8, 0x00000011,
    };
    struct R_Microbit_SpirvParserProgram* prog = CreateProgram(ctx, spv, 35);

    struct R_Microbit_SpirvParserEntryPoint* ep = R_Microbit_SpirvParserProgramCreateEntryPoint(prog);
    ASSERT_NE(nullptr, ep);
    ep->execModel = MICROBIT_SPIRV_EXECUTION_MODEL_VERTEX;
    ep->id = 0x11;
    ep->name = strdup("main");

    EXPECT_EQ(1u, prog->entryPointCount);
    EXPECT_EQ(MICROBIT_SPIRV_EXECUTION_MODEL_VERTEX, prog->pEntryPoints[0].execModel);
    EXPECT_STREQ("main", prog->pEntryPoints[0].name);

    DeleteProgram(prog);
    DeleteContext(ctx);
}

TEST_F(MicrobitSpirvParserEntryPointTest, EntryPoint_Fragment) {
    struct R_Microbit_SpirvParserContext* ctx = CreateContext();
    const uint32_t spv[] = {
        0x07230203, 0x00010500, 0x000d000b, 0x00000020, 0x00000000,
        0x00020011, 0x00000001,
    };
    struct R_Microbit_SpirvParserProgram* prog = CreateProgram(ctx, spv, 7);

    struct R_Microbit_SpirvParserEntryPoint* ep = R_Microbit_SpirvParserProgramCreateEntryPoint(prog);
    ASSERT_NE(nullptr, ep);
    ep->execModel = MICROBIT_SPIRV_EXECUTION_MODEL_FRAGMENT;
    ep->id = 0x10;
    ep->name = strdup("frag_main");

    EXPECT_EQ(1u, prog->entryPointCount);
    EXPECT_EQ(MICROBIT_SPIRV_EXECUTION_MODEL_FRAGMENT, prog->pEntryPoints[0].execModel);

    DeleteProgram(prog);
    DeleteContext(ctx);
}

TEST_F(MicrobitSpirvParserEntryPointTest, EntryPoint_Compute) {
    struct R_Microbit_SpirvParserContext* ctx = CreateContext();
    const uint32_t spv[] = {
        0x07230203, 0x00010500, 0x000d000b, 0x00000020, 0x00000000,
        0x00020011, 0x00000001,
    };
    struct R_Microbit_SpirvParserProgram* prog = CreateProgram(ctx, spv, 7);

    struct R_Microbit_SpirvParserEntryPoint* ep = R_Microbit_SpirvParserProgramCreateEntryPoint(prog);
    ASSERT_NE(nullptr, ep);
    ep->execModel = MICROBIT_SPIRV_EXECUTION_MODEL_GL_COMPUTE;
    ep->id = 0x20;
    ep->name = strdup("compute_main");
    ep->globalsCount = 2;
    ep->pGlobals = (uint32_t*)R_CSTL_HeapAlloc(2 * sizeof(uint32_t));
    ep->pGlobals[0] = 0x15;
    ep->pGlobals[1] = 0x16;

    EXPECT_EQ(1u, prog->entryPointCount);
    EXPECT_EQ(MICROBIT_SPIRV_EXECUTION_MODEL_GL_COMPUTE, prog->pEntryPoints[0].execModel);
    EXPECT_EQ(2u, prog->pEntryPoints[0].globalsCount);

    DeleteProgram(prog);
    DeleteContext(ctx);
}

TEST_F(MicrobitSpirvParserEntryPointTest, MultipleEntryPoints) {
    struct R_Microbit_SpirvParserContext* ctx = CreateContext();
    const uint32_t spv[] = {
        0x07230203, 0x00010500, 0x000d000b, 0x00000020, 0x00000000,
        0x00020011, 0x00000001,
    };
    struct R_Microbit_SpirvParserProgram* prog = CreateProgram(ctx, spv, 7);

    struct R_Microbit_SpirvParserEntryPoint* ep1 = R_Microbit_SpirvParserProgramCreateEntryPoint(prog);
    ASSERT_NE(nullptr, ep1);
    ep1->execModel = MICROBIT_SPIRV_EXECUTION_MODEL_VERTEX;
    ep1->id = 0x10;
    ep1->name = strdup("vert_main");

    struct R_Microbit_SpirvParserEntryPoint* ep2 = R_Microbit_SpirvParserProgramCreateEntryPoint(prog);
    ASSERT_NE(nullptr, ep2);
    ep2->execModel = MICROBIT_SPIRV_EXECUTION_MODEL_FRAGMENT;
    ep2->id = 0x20;
    ep2->name = strdup("frag_main");

    EXPECT_EQ(2u, prog->entryPointCount);
    EXPECT_EQ(MICROBIT_SPIRV_EXECUTION_MODEL_VERTEX, prog->pEntryPoints[0].execModel);
    EXPECT_EQ(MICROBIT_SPIRV_EXECUTION_MODEL_FRAGMENT, prog->pEntryPoints[1].execModel);

    DeleteProgram(prog);
    DeleteContext(ctx);
}

TEST_F(MicrobitSpirvParserEntryPointTest, Capability_Shader) {
    struct R_Microbit_SpirvParserContext* ctx = CreateContext();
    const uint32_t spv[] = {
        0x07230203, 0x00010500, 0x000d000b, 0x00000020, 0x00000000,
        0x00020011, 0x00000001,
    };
    struct R_Microbit_SpirvParserProgram* prog = CreateProgram(ctx, spv, 7);

    R_Microbit_SpirvParserProgramAddCapability(prog, 0);  // Shader
    EXPECT_EQ(1u, prog->capabilityCount);
    EXPECT_EQ(0u, prog->pCapabilities[0]);

    DeleteProgram(prog);
    DeleteContext(ctx);
}

TEST_F(MicrobitSpirvParserEntryPointTest, Capability_Multiple) {
    struct R_Microbit_SpirvParserContext* ctx = CreateContext();
    const uint32_t spv[] = {
        0x07230203, 0x00010500, 0x000d000b, 0x00000020, 0x00000000,
        0x00020011, 0x00000001,
    };
    struct R_Microbit_SpirvParserProgram* prog = CreateProgram(ctx, spv, 7);

    R_Microbit_SpirvParserProgramAddCapability(prog, 0);  // Shader
    R_Microbit_SpirvParserProgramAddCapability(prog, 1);  // Matrix
    R_Microbit_SpirvParserProgramAddCapability(prog, 2);  // Geometry
    R_Microbit_SpirvParserProgramAddCapability(prog, 3);  // Tessellation

    EXPECT_EQ(4u, prog->capabilityCount);
    EXPECT_EQ(0u, prog->pCapabilities[0]);
    EXPECT_EQ(1u, prog->pCapabilities[1]);
    EXPECT_EQ(2u, prog->pCapabilities[2]);
    EXPECT_EQ(3u, prog->pCapabilities[3]);

    DeleteProgram(prog);
    DeleteContext(ctx);
}

TEST_F(MicrobitSpirvParserEntryPointTest, ExecutionMode_OriginUpperLeft) {
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
        0x000200f8, 0x0000000f,
    };
    struct R_Microbit_SpirvParserProgram* prog = CreateProgram(ctx, spv, 32);

    // Execution mode is parsed during setup phase
    EXPECT_EQ(0u, prog->localSizeX);
    EXPECT_EQ(0u, prog->localSizeY);
    EXPECT_EQ(0u, prog->localSizeZ);

    DeleteProgram(prog);
    DeleteContext(ctx);
}

TEST_F(MicrobitSpirvParserEntryPointTest, Extension_Add) {
    struct R_Microbit_SpirvParserContext* ctx = CreateContext();
    const uint32_t spv[] = {
        0x07230203, 0x00010500, 0x000d000b, 0x00000020, 0x00000000,
        0x00020011, 0x00000001,
    };
    struct R_Microbit_SpirvParserProgram* prog = CreateProgram(ctx, spv, 7);

    char* ext = R_Microbit_SpirvParserProgramAddExtension(prog, 10);
    ASSERT_NE(nullptr, ext);
    EXPECT_EQ(1u, prog->extensionCount);
    strcpy(ext, "TestExt");

    DeleteProgram(prog);
    DeleteContext(ctx);
}

}  // namespace