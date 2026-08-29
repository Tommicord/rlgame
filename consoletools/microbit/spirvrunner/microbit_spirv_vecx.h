#pragma once

#include <stdint.h>

typedef struct
{
        float x, y;
} R_Microbit_SpirvVec2;
typedef struct
{
        float x, y, z;
} R_Microbit_SpirvVec3;
typedef struct
{
        float x, y, z, w;
} R_Microbit_SpirvVec4;
typedef struct
{
        float v[3][3];
} R_Microbit_SpirvMat3;
typedef struct
{
        float v[4][4];
} R_Microbit_SpirvMat4;

typedef struct
{
        uint32_t x, y;
} R_Microbit_SpirvBVec2;
typedef struct
{
        uint32_t x, y, z;
} R_Microbit_SpirvBVec3;
typedef struct
{
        uint32_t x, y, z, w;
} R_Microbit_SpirvBVec4;

R_Microbit_SpirvVec2 R_Microbit_SpirvVec2Add (R_Microbit_SpirvVec2 a, R_Microbit_SpirvVec2 b);
R_Microbit_SpirvVec2 R_Microbit_SpirvVec2Sub (R_Microbit_SpirvVec2 a, R_Microbit_SpirvVec2 b);
R_Microbit_SpirvVec2 R_Microbit_SpirvVec2Mul (R_Microbit_SpirvVec2 a, R_Microbit_SpirvVec2 b);
R_Microbit_SpirvVec2 R_Microbit_SpirvVec2Div (R_Microbit_SpirvVec2 a, R_Microbit_SpirvVec2 b);
R_Microbit_SpirvVec3 R_Microbit_SpirvVec3Add (R_Microbit_SpirvVec3 a, R_Microbit_SpirvVec3 b);
R_Microbit_SpirvVec3 R_Microbit_SpirvVec3Sub (R_Microbit_SpirvVec3 a, R_Microbit_SpirvVec3 b);
R_Microbit_SpirvVec3 R_Microbit_SpirvVec3Mul (R_Microbit_SpirvVec3 a, R_Microbit_SpirvVec3 b);
R_Microbit_SpirvVec3 R_Microbit_SpirvVec3Div (R_Microbit_SpirvVec3 a, R_Microbit_SpirvVec3 b);
R_Microbit_SpirvVec4 R_Microbit_SpirvVec4Add (R_Microbit_SpirvVec4 a, R_Microbit_SpirvVec4 b);
R_Microbit_SpirvVec4 R_Microbit_SpirvVec4Sub (R_Microbit_SpirvVec4 a, R_Microbit_SpirvVec4 b);
R_Microbit_SpirvVec4 R_Microbit_SpirvVec4Mul (R_Microbit_SpirvVec4 a, R_Microbit_SpirvVec4 b);
R_Microbit_SpirvVec4 R_Microbit_SpirvVec4Div (R_Microbit_SpirvVec4 a, R_Microbit_SpirvVec4 b);

float                R_Microbit_SpirvVec2Dot (R_Microbit_SpirvVec2 a, R_Microbit_SpirvVec2 b);
float                R_Microbit_SpirvVec3Dot (R_Microbit_SpirvVec3 a, R_Microbit_SpirvVec3 b);
float                R_Microbit_SpirvVec4Dot (R_Microbit_SpirvVec4 a, R_Microbit_SpirvVec4 b);
R_Microbit_SpirvVec3 R_Microbit_SpirvVec3Cross (R_Microbit_SpirvVec3 a, R_Microbit_SpirvVec3 b);
R_Microbit_SpirvVec2 R_Microbit_SpirvVec2Normalize (R_Microbit_SpirvVec2 value);
R_Microbit_SpirvVec3 R_Microbit_SpirvVec3Normalize (R_Microbit_SpirvVec3 value);
R_Microbit_SpirvVec4 R_Microbit_SpirvVec4Normalize (R_Microbit_SpirvVec4 value);
float                R_Microbit_SpirvVec2Length (R_Microbit_SpirvVec2 value);
float                R_Microbit_SpirvVec3Length (R_Microbit_SpirvVec3 value);
float                R_Microbit_SpirvVec4Length (R_Microbit_SpirvVec4 value);
R_Microbit_SpirvVec3 R_Microbit_SpirvVec3Reflect (R_Microbit_SpirvVec3 incident, R_Microbit_SpirvVec3 normal);
R_Microbit_SpirvVec3
R_Microbit_SpirvVec3Refract (R_Microbit_SpirvVec3 incident, R_Microbit_SpirvVec3 normal, float eta);

float R_Microbit_SpirvRadians (float degrees);
float R_Microbit_SpirvDegrees (float radians);
float R_Microbit_SpirvMod (float x, float y);
float R_Microbit_SpirvStep (float edge, float x);
float R_Microbit_SpirvSmoothstep (float edge0, float edge1, float x);
float R_Microbit_SpirvRoundEven (float value);
float R_Microbit_SpirvInverseSqrt (float value);

R_Microbit_SpirvMat3 R_Microbit_SpirvMat3Identity (void);
R_Microbit_SpirvMat4 R_Microbit_SpirvMat4Identity (void);
R_Microbit_SpirvMat3 R_Microbit_SpirvMat3Mul (R_Microbit_SpirvMat3 a, R_Microbit_SpirvMat3 b);
R_Microbit_SpirvMat4 R_Microbit_SpirvMat4Mul (R_Microbit_SpirvMat4 a, R_Microbit_SpirvMat4 b);
R_Microbit_SpirvMat3 R_Microbit_SpirvMat3Transpose (R_Microbit_SpirvMat3 value);
R_Microbit_SpirvMat4 R_Microbit_SpirvMat4Transpose (R_Microbit_SpirvMat4 value);
R_Microbit_SpirvVec3 R_Microbit_SpirvMat3MulVec3 (R_Microbit_SpirvMat3 matrix, R_Microbit_SpirvVec3 value);
R_Microbit_SpirvVec4 R_Microbit_SpirvMat4MulVec4 (R_Microbit_SpirvMat4 matrix, R_Microbit_SpirvVec4 value);
int                  R_Microbit_SpirvMat3Inverse (R_Microbit_SpirvMat3 value, R_Microbit_SpirvMat3* result);
int                  R_Microbit_SpirvMat4Inverse (R_Microbit_SpirvMat4 value, R_Microbit_SpirvMat4* result);
