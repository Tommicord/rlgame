#include <gtest/gtest.h>
#include <cmath>

extern "C" {
#include "microbit/spirvrunner/microbit_spirv_vecx.h"
}

namespace {

class MicrobitSpirvVecxTest : public ::testing::Test {};

TEST_F(MicrobitSpirvVecxTest, Vec2Add) {
    R_Microbit_SpirvVec2 a = {1.0f, 2.0f};
    R_Microbit_SpirvVec2 b = {3.0f, 4.0f};
    R_Microbit_SpirvVec2 result = R_Microbit_SpirvVec2Add(a, b);
    EXPECT_FLOAT_EQ(4.0f, result.x);
    EXPECT_FLOAT_EQ(6.0f, result.y);
}

TEST_F(MicrobitSpirvVecxTest, Vec2Sub) {
    R_Microbit_SpirvVec2 a = {5.0f, 7.0f};
    R_Microbit_SpirvVec2 b = {2.0f, 3.0f};
    R_Microbit_SpirvVec2 result = R_Microbit_SpirvVec2Sub(a, b);
    EXPECT_FLOAT_EQ(3.0f, result.x);
    EXPECT_FLOAT_EQ(4.0f, result.y);
}

TEST_F(MicrobitSpirvVecxTest, Vec2Mul) {
    R_Microbit_SpirvVec2 a = {2.0f, 3.0f};
    R_Microbit_SpirvVec2 b = {4.0f, 5.0f};
    R_Microbit_SpirvVec2 result = R_Microbit_SpirvVec2Mul(a, b);
    EXPECT_FLOAT_EQ(8.0f, result.x);
    EXPECT_FLOAT_EQ(15.0f, result.y);
}

TEST_F(MicrobitSpirvVecxTest, Vec2Div) {
    R_Microbit_SpirvVec2 a = {8.0f, 15.0f};
    R_Microbit_SpirvVec2 b = {2.0f, 3.0f};
    R_Microbit_SpirvVec2 result = R_Microbit_SpirvVec2Div(a, b);
    EXPECT_FLOAT_EQ(4.0f, result.x);
    EXPECT_FLOAT_EQ(5.0f, result.y);
}

TEST_F(MicrobitSpirvVecxTest, Vec3Add) {
    R_Microbit_SpirvVec3 a = {1.0f, 2.0f, 3.0f};
    R_Microbit_SpirvVec3 b = {4.0f, 5.0f, 6.0f};
    R_Microbit_SpirvVec3 result = R_Microbit_SpirvVec3Add(a, b);
    EXPECT_FLOAT_EQ(5.0f, result.x);
    EXPECT_FLOAT_EQ(7.0f, result.y);
    EXPECT_FLOAT_EQ(9.0f, result.z);
}

TEST_F(MicrobitSpirvVecxTest, Vec3Sub) {
    R_Microbit_SpirvVec3 a = {5.0f, 7.0f, 9.0f};
    R_Microbit_SpirvVec3 b = {1.0f, 2.0f, 3.0f};
    R_Microbit_SpirvVec3 result = R_Microbit_SpirvVec3Sub(a, b);
    EXPECT_FLOAT_EQ(4.0f, result.x);
    EXPECT_FLOAT_EQ(5.0f, result.y);
    EXPECT_FLOAT_EQ(6.0f, result.z);
}

TEST_F(MicrobitSpirvVecxTest, Vec3Mul) {
    R_Microbit_SpirvVec3 a = {2.0f, 3.0f, 4.0f};
    R_Microbit_SpirvVec3 b = {5.0f, 6.0f, 7.0f};
    R_Microbit_SpirvVec3 result = R_Microbit_SpirvVec3Mul(a, b);
    EXPECT_FLOAT_EQ(10.0f, result.x);
    EXPECT_FLOAT_EQ(18.0f, result.y);
    EXPECT_FLOAT_EQ(28.0f, result.z);
}

TEST_F(MicrobitSpirvVecxTest, Vec3Div) {
    R_Microbit_SpirvVec3 a = {10.0f, 18.0f, 28.0f};
    R_Microbit_SpirvVec3 b = {2.0f, 3.0f, 4.0f};
    R_Microbit_SpirvVec3 result = R_Microbit_SpirvVec3Div(a, b);
    EXPECT_FLOAT_EQ(5.0f, result.x);
    EXPECT_FLOAT_EQ(6.0f, result.y);
    EXPECT_FLOAT_EQ(7.0f, result.z);
}

TEST_F(MicrobitSpirvVecxTest, Vec4Add) {
    R_Microbit_SpirvVec4 a = {1.0f, 2.0f, 3.0f, 4.0f};
    R_Microbit_SpirvVec4 b = {5.0f, 6.0f, 7.0f, 8.0f};
    R_Microbit_SpirvVec4 result = R_Microbit_SpirvVec4Add(a, b);
    EXPECT_FLOAT_EQ(6.0f, result.x);
    EXPECT_FLOAT_EQ(8.0f, result.y);
    EXPECT_FLOAT_EQ(10.0f, result.z);
    EXPECT_FLOAT_EQ(12.0f, result.w);
}

TEST_F(MicrobitSpirvVecxTest, Vec4Sub) {
    R_Microbit_SpirvVec4 a = {6.0f, 8.0f, 10.0f, 12.0f};
    R_Microbit_SpirvVec4 b = {1.0f, 2.0f, 3.0f, 4.0f};
    R_Microbit_SpirvVec4 result = R_Microbit_SpirvVec4Sub(a, b);
    EXPECT_FLOAT_EQ(5.0f, result.x);
    EXPECT_FLOAT_EQ(6.0f, result.y);
    EXPECT_FLOAT_EQ(7.0f, result.z);
    EXPECT_FLOAT_EQ(8.0f, result.w);
}

TEST_F(MicrobitSpirvVecxTest, Vec4Mul) {
    R_Microbit_SpirvVec4 a = {2.0f, 3.0f, 4.0f, 5.0f};
    R_Microbit_SpirvVec4 b = {2.0f, 2.0f, 2.0f, 2.0f};
    R_Microbit_SpirvVec4 result = R_Microbit_SpirvVec4Mul(a, b);
    EXPECT_FLOAT_EQ(4.0f, result.x);
    EXPECT_FLOAT_EQ(6.0f, result.y);
    EXPECT_FLOAT_EQ(8.0f, result.z);
    EXPECT_FLOAT_EQ(10.0f, result.w);
}

TEST_F(MicrobitSpirvVecxTest, Vec4Div) {
    R_Microbit_SpirvVec4 a = {4.0f, 6.0f, 8.0f, 10.0f};
    R_Microbit_SpirvVec4 b = {2.0f, 2.0f, 2.0f, 2.0f};
    R_Microbit_SpirvVec4 result = R_Microbit_SpirvVec4Div(a, b);
    EXPECT_FLOAT_EQ(2.0f, result.x);
    EXPECT_FLOAT_EQ(3.0f, result.y);
    EXPECT_FLOAT_EQ(4.0f, result.z);
    EXPECT_FLOAT_EQ(5.0f, result.w);
}

TEST_F(MicrobitSpirvVecxTest, Vec2Dot) {
    R_Microbit_SpirvVec2 a = {3.0f, 4.0f};
    R_Microbit_SpirvVec2 b = {5.0f, 12.0f};
    float result = R_Microbit_SpirvVec2Dot(a, b);
    EXPECT_FLOAT_EQ(63.0f, result);
}

TEST_F(MicrobitSpirvVecxTest, Vec3Dot) {
    R_Microbit_SpirvVec3 a = {1.0f, 2.0f, 3.0f};
    R_Microbit_SpirvVec3 b = {4.0f, 5.0f, 6.0f};
    float result = R_Microbit_SpirvVec3Dot(a, b);
    EXPECT_FLOAT_EQ(32.0f, result);
}

TEST_F(MicrobitSpirvVecxTest, Vec4Dot) {
    R_Microbit_SpirvVec4 a = {1.0f, 2.0f, 3.0f, 4.0f};
    R_Microbit_SpirvVec4 b = {2.0f, 3.0f, 4.0f, 5.0f};
    float result = R_Microbit_SpirvVec4Dot(a, b);
    EXPECT_FLOAT_EQ(40.0f, result);
}

TEST_F(MicrobitSpirvVecxTest, Vec3Cross) {
    R_Microbit_SpirvVec3 a = {1.0f, 0.0f, 0.0f};
    R_Microbit_SpirvVec3 b = {0.0f, 1.0f, 0.0f};
    R_Microbit_SpirvVec3 result = R_Microbit_SpirvVec3Cross(a, b);
    EXPECT_FLOAT_EQ(0.0f, result.x);
    EXPECT_FLOAT_EQ(0.0f, result.y);
    EXPECT_FLOAT_EQ(1.0f, result.z);
}

TEST_F(MicrobitSpirvVecxTest, Vec2Normalize) {
    R_Microbit_SpirvVec2 a = {3.0f, 4.0f};
    R_Microbit_SpirvVec2 result = R_Microbit_SpirvVec2Normalize(a);
    EXPECT_NEAR(0.6f, result.x, 0.0001f);
    EXPECT_NEAR(0.8f, result.y, 0.0001f);
}

TEST_F(MicrobitSpirvVecxTest, Vec3Normalize) {
    R_Microbit_SpirvVec3 a = {3.0f, 4.0f, 0.0f};
    R_Microbit_SpirvVec3 result = R_Microbit_SpirvVec3Normalize(a);
    EXPECT_NEAR(0.6f, result.x, 0.0001f);
    EXPECT_NEAR(0.8f, result.y, 0.0001f);
    EXPECT_FLOAT_EQ(0.0f, result.z);
}

TEST_F(MicrobitSpirvVecxTest, Vec4Normalize) {
    R_Microbit_SpirvVec4 a = {1.0f, 2.0f, 2.0f, 0.0f};
    R_Microbit_SpirvVec4 result = R_Microbit_SpirvVec4Normalize(a);
    EXPECT_NEAR(1.0f/3.0f, result.x, 0.0001f);
    EXPECT_NEAR(2.0f/3.0f, result.y, 0.0001f);
    EXPECT_NEAR(2.0f/3.0f, result.z, 0.0001f);
    EXPECT_FLOAT_EQ(0.0f, result.w);
}

TEST_F(MicrobitSpirvVecxTest, Vec2Length) {
    R_Microbit_SpirvVec2 a = {3.0f, 4.0f};
    float result = R_Microbit_SpirvVec2Length(a);
    EXPECT_FLOAT_EQ(5.0f, result);
}

TEST_F(MicrobitSpirvVecxTest, Vec3Length) {
    R_Microbit_SpirvVec3 a = {1.0f, 2.0f, 2.0f};
    float result = R_Microbit_SpirvVec3Length(a);
    EXPECT_FLOAT_EQ(3.0f, result);
}

TEST_F(MicrobitSpirvVecxTest, Vec4Length) {
    R_Microbit_SpirvVec4 a = {1.0f, 2.0f, 2.0f, 0.0f};
    float result = R_Microbit_SpirvVec4Length(a);
    EXPECT_FLOAT_EQ(3.0f, result);
}

TEST_F(MicrobitSpirvVecxTest, Vec3Reflect) {
    R_Microbit_SpirvVec3 incident = {1.0f, -1.0f, 0.0f};
    R_Microbit_SpirvVec3 normal = {0.0f, 1.0f, 0.0f};
    R_Microbit_SpirvVec3 result = R_Microbit_SpirvVec3Reflect(incident, normal);
    EXPECT_FLOAT_EQ(1.0f, result.x);
    EXPECT_FLOAT_EQ(1.0f, result.y);
    EXPECT_FLOAT_EQ(0.0f, result.z);
}

TEST_F(MicrobitSpirvVecxTest, Vec3Refract) {
    R_Microbit_SpirvVec3 incident = {0.0f, -1.0f, 0.0f};
    R_Microbit_SpirvVec3 normal = {0.0f, 1.0f, 0.0f};
    R_Microbit_SpirvVec3 result = R_Microbit_SpirvVec3Refract(incident, normal, 1.0f);
    EXPECT_FLOAT_EQ(0.0f, result.x);
    EXPECT_FLOAT_EQ(-1.0f, result.y);
    EXPECT_FLOAT_EQ(0.0f, result.z);
}

TEST_F(MicrobitSpirvVecxTest, Radians) {
    float result = R_Microbit_SpirvRadians(180.0f);
    EXPECT_NEAR(M_PI, result, 0.0001f);
}

TEST_F(MicrobitSpirvVecxTest, Degrees) {
    float result = R_Microbit_SpirvDegrees(M_PI);
    EXPECT_NEAR(180.0f, result, 0.0001f);
}

TEST_F(MicrobitSpirvVecxTest, Mod) {
    float result = R_Microbit_SpirvMod(5.5f, 2.0f);
    EXPECT_NEAR(1.5f, result, 0.0001f);
}

TEST_F(MicrobitSpirvVecxTest, Step) {
    float result = R_Microbit_SpirvStep(0.5f, 0.3f);
    EXPECT_FLOAT_EQ(0.0f, result);
    result = R_Microbit_SpirvStep(0.5f, 0.7f);
    EXPECT_FLOAT_EQ(1.0f, result);
}

TEST_F(MicrobitSpirvVecxTest, Smoothstep) {
    float result = R_Microbit_SpirvSmoothstep(0.0f, 1.0f, 0.5f);
    EXPECT_NEAR(0.5f, result, 0.0001f);
    result = R_Microbit_SpirvSmoothstep(0.0f, 1.0f, -0.5f);
    EXPECT_FLOAT_EQ(0.0f, result);
    result = R_Microbit_SpirvSmoothstep(0.0f, 1.0f, 1.5f);
    EXPECT_FLOAT_EQ(1.0f, result);
}

TEST_F(MicrobitSpirvVecxTest, RoundEven) {
    float result = R_Microbit_SpirvRoundEven(1.5f);
    EXPECT_FLOAT_EQ(2.0f, result);
    result = R_Microbit_SpirvRoundEven(2.5f);
    EXPECT_FLOAT_EQ(2.0f, result);
}

TEST_F(MicrobitSpirvVecxTest, InverseSqrt) {
    float result = R_Microbit_SpirvInverseSqrt(4.0f);
    EXPECT_NEAR(0.5f, result, 0.0001f);
    result = R_Microbit_SpirvInverseSqrt(9.0f);
    EXPECT_NEAR(1.0f/3.0f, result, 0.0001f);
}

TEST_F(MicrobitSpirvVecxTest, Mat3Identity) {
    R_Microbit_SpirvMat3 m = R_Microbit_SpirvMat3Identity();
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (i == j) EXPECT_FLOAT_EQ(1.0f, m.v[i][j]);
            else EXPECT_FLOAT_EQ(0.0f, m.v[i][j]);
        }
    }
}

TEST_F(MicrobitSpirvVecxTest, Mat4Identity) {
    R_Microbit_SpirvMat4 m = R_Microbit_SpirvMat4Identity();
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (i == j) EXPECT_FLOAT_EQ(1.0f, m.v[i][j]);
            else EXPECT_FLOAT_EQ(0.0f, m.v[i][j]);
        }
    }
}

TEST_F(MicrobitSpirvVecxTest, Mat3Mul) {
    R_Microbit_SpirvMat3 a = R_Microbit_SpirvMat3Identity();
    R_Microbit_SpirvMat3 b = R_Microbit_SpirvMat3Identity();
    a.v[0][0] = 2.0f;
    b.v[0][0] = 3.0f;
    R_Microbit_SpirvMat3 result = R_Microbit_SpirvMat3Mul(a, b);
    EXPECT_FLOAT_EQ(6.0f, result.v[0][0]);
    EXPECT_FLOAT_EQ(1.0f, result.v[1][1]);
    EXPECT_FLOAT_EQ(1.0f, result.v[2][2]);
}

TEST_F(MicrobitSpirvVecxTest, Mat4Mul) {
    R_Microbit_SpirvMat4 a = R_Microbit_SpirvMat4Identity();
    R_Microbit_SpirvMat4 b = R_Microbit_SpirvMat4Identity();
    a.v[0][0] = 2.0f;
    b.v[0][0] = 3.0f;
    R_Microbit_SpirvMat4 result = R_Microbit_SpirvMat4Mul(a, b);
    EXPECT_FLOAT_EQ(6.0f, result.v[0][0]);
    EXPECT_FLOAT_EQ(1.0f, result.v[1][1]);
}

TEST_F(MicrobitSpirvVecxTest, Mat3Transpose) {
    R_Microbit_SpirvMat3 m = R_Microbit_SpirvMat3Identity();
    m.v[0][1] = 5.0f;
    m.v[1][2] = 7.0f;
    R_Microbit_SpirvMat3 result = R_Microbit_SpirvMat3Transpose(m);
    EXPECT_FLOAT_EQ(5.0f, result.v[1][0]);
    EXPECT_FLOAT_EQ(7.0f, result.v[2][1]);
}

TEST_F(MicrobitSpirvVecxTest, Mat4Transpose) {
    R_Microbit_SpirvMat4 m = R_Microbit_SpirvMat4Identity();
    m.v[0][1] = 5.0f;
    m.v[1][2] = 7.0f;
    R_Microbit_SpirvMat4 result = R_Microbit_SpirvMat4Transpose(m);
    EXPECT_FLOAT_EQ(5.0f, result.v[1][0]);
    EXPECT_FLOAT_EQ(7.0f, result.v[2][1]);
}

TEST_F(MicrobitSpirvVecxTest, Mat3MulVec3) {
    R_Microbit_SpirvMat3 m = R_Microbit_SpirvMat3Identity();
    m.v[0][0] = 2.0f;
    m.v[1][1] = 3.0f;
    R_Microbit_SpirvVec3 v = {1.0f, 2.0f, 3.0f};
    R_Microbit_SpirvVec3 result = R_Microbit_SpirvMat3MulVec3(m, v);
    EXPECT_FLOAT_EQ(2.0f, result.x);
    EXPECT_FLOAT_EQ(6.0f, result.y);
    EXPECT_FLOAT_EQ(3.0f, result.z);
}

TEST_F(MicrobitSpirvVecxTest, Mat4MulVec4) {
    R_Microbit_SpirvMat4 m = R_Microbit_SpirvMat4Identity();
    m.v[0][0] = 2.0f;
    m.v[1][1] = 3.0f;
    R_Microbit_SpirvVec4 v = {1.0f, 2.0f, 3.0f, 4.0f};
    R_Microbit_SpirvVec4 result = R_Microbit_SpirvMat4MulVec4(m, v);
    EXPECT_FLOAT_EQ(2.0f, result.x);
    EXPECT_FLOAT_EQ(6.0f, result.y);
    EXPECT_FLOAT_EQ(3.0f, result.z);
    EXPECT_FLOAT_EQ(4.0f, result.w);
}

TEST_F(MicrobitSpirvVecxTest, Mat3Inverse) {
    R_Microbit_SpirvMat3 m = R_Microbit_SpirvMat3Identity();
    m.v[0][0] = 2.0f;
    m.v[1][1] = 3.0f;
    m.v[2][2] = 4.0f;
    R_Microbit_SpirvMat3 inv;
    int result = R_Microbit_SpirvMat3Inverse(m, &inv);
    EXPECT_EQ(1, result);
    EXPECT_NEAR(0.5f, inv.v[0][0], 0.0001f);
    EXPECT_NEAR(1.0f/3.0f, inv.v[1][1], 0.0001f);
    EXPECT_NEAR(0.25f, inv.v[2][2], 0.0001f);
}

TEST_F(MicrobitSpirvVecxTest, Mat4Inverse) {
    R_Microbit_SpirvMat4 m = R_Microbit_SpirvMat4Identity();
    m.v[0][0] = 2.0f;
    m.v[1][1] = 3.0f;
    m.v[2][2] = 4.0f;
    m.v[3][3] = 5.0f;
    R_Microbit_SpirvMat4 inv;
    int result = R_Microbit_SpirvMat4Inverse(m, &inv);
    EXPECT_EQ(1, result);
    EXPECT_NEAR(0.5f, inv.v[0][0], 0.0001f);
    EXPECT_NEAR(1.0f/3.0f, inv.v[1][1], 0.0001f);
    EXPECT_NEAR(0.25f, inv.v[2][2], 0.0001f);
    EXPECT_NEAR(0.2f, inv.v[3][3], 0.0001f);
}

}  // namespace