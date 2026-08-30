#include <gtest/gtest.h>

extern "C" {
#include "microbit/microbit_spirv_parser.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"
}

namespace {

constexpr size_t kTestHeapSize = 4 * 1024 * 1024;

class MicrobitSpirvParserDecorationsTest : public ::testing::Test {
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

TEST_F(MicrobitSpirvParserDecorationsTest, DecorateArrayStride) {
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
        0x00050041, 0x00000014, 0x00000015, 0x0000000c, 0x00000000,  // OpTypeArray
        0x00040047, 0x00000015, 0x00000024, 0x00000010,  // OpDecorate ArrayStride 16
        0x000200f8, 0x0000000f,
    };
    struct R_Microbit_SpirvParserProgram* prog = CreateProgram(ctx, spv, 35);
    ASSERT_NE(nullptr, prog);

    DeleteProgram(prog);
    DeleteContext(ctx);
}

TEST_F(MicrobitSpirvParserDecorationsTest, DecorateMatrixStride) {
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
        0x00050042, 0x00000014, 0x00000015, 0x0000000c, 0x00000004,  // OpTypeMatrix
        0x00040047, 0x00000015, 0x00000025, 0x00000010,  // OpDecorate MatrixStride 16
        0x000200f8, 0x0000000f,
    };
    struct R_Microbit_SpirvParserProgram* prog = CreateProgram(ctx, spv, 35);
    ASSERT_NE(nullptr, prog);

    DeleteProgram(prog);
    DeleteContext(ctx);
}

TEST_F(MicrobitSpirvParserDecorationsTest, MemberDecorateOffset) {
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
        0x0004003b, 0x00000014, 0x00000015, 0x00000000,  // OpTypeStruct
        0x00050048, 0x00000014, 0x00000000, 0x00000023, 0x00000010,  // OpMemberDecorate Offset
        0x000200f8, 0x0000000f,
    };
    struct R_Microbit_SpirvParserProgram* prog = CreateProgram(ctx, spv, 35);
    ASSERT_NE(nullptr, prog);

    DeleteProgram(prog);
    DeleteContext(ctx);
}

TEST_F(MicrobitSpirvParserDecorationsTest, DecorateLocation) {
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
        0x00040047, 0x00000014, 0x0000001e, 0x00000000,  // OpDecorate Location 0
        0x000200f8, 0x0000000f,
    };
    struct R_Microbit_SpirvParserProgram* prog = CreateProgram(ctx, spv, 32);
    ASSERT_NE(nullptr, prog);

    DeleteProgram(prog);
    DeleteContext(ctx);
}

TEST_F(MicrobitSpirvParserDecorationsTest, DecorateBinding) {
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
        0x00040047, 0x00000014, 0x00000021, 0x00000000,  // OpDecorate Binding 0
        0x000200f8, 0x0000000f,
    };
    struct R_Microbit_SpirvParserProgram* prog = CreateProgram(ctx, spv, 32);
    ASSERT_NE(nullptr, prog);

    DeleteProgram(prog);
    DeleteContext(ctx);
}

TEST_F(MicrobitSpirvParserDecorationsTest, DecorateDescriptorSet) {
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
        0x00040047, 0x00000014, 0x00000022, 0x00000000,  // OpDecorate DescriptorSet 0
        0x000200f8, 0x0000000f,
    };
    struct R_Microbit_SpirvParserProgram* prog = CreateProgram(ctx, spv, 32);
    ASSERT_NE(nullptr, prog);

    DeleteProgram(prog);
    DeleteContext(ctx);
}

TEST_F(MicrobitSpirvParserDecorationsTest, DecorateBuiltIn) {
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
        0x00040047, 0x00000014, 0x0000000b, 0x00000001,  // OpDecorate BuiltIn Position
        0x000200f8, 0x0000000f,
    };
    struct R_Microbit_SpirvParserProgram* prog = CreateProgram(ctx, spv, 32);
    ASSERT_NE(nullptr, prog);

    DeleteProgram(prog);
    DeleteContext(ctx);
}

TEST_F(MicrobitSpirvParserDecorationsTest, DecorateOffset) {
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
        0x00040047, 0x00000014, 0x00000023, 0x00000010,  // OpDecorate Offset 16
        0x000200f8, 0x0000000f,
    };
    struct R_Microbit_SpirvParserProgram* prog = CreateProgram(ctx, spv, 32);
    ASSERT_NE(nullptr, prog);

    DeleteProgram(prog);
    DeleteContext(ctx);
}

}  // namespace