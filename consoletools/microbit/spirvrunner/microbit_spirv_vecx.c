#include "microbit/spirvrunner/microbit_spirv_vecx.h"

#include <float.h>
#include <math.h>
#include <string.h>

#if defined(__SSE2__)
#include <emmintrin.h>
#endif

#define R_SPIRV_PI 3.14159265358979323846f

static R_Microbit_SpirvVec4
R_SpirvVec4Apply (R_Microbit_SpirvVec4 a, R_Microbit_SpirvVec4 b, int op)
{
#if defined(__SSE2__)
    __m128               av = _mm_loadu_ps (&a.x);
    __m128               bv = _mm_loadu_ps (&b.x);
    __m128               rv = op == 0   ? _mm_add_ps (av, bv)
                              : op == 1 ? _mm_sub_ps (av, bv)
                              : op == 2 ? _mm_mul_ps (av, bv)
                                        : _mm_div_ps (av, bv);
    R_Microbit_SpirvVec4 result;
    _mm_storeu_ps (&result.x, rv);
    return result;
#else
    R_Microbit_SpirvVec4 result = {0};
    const float*         ap = &a.x;
    const float*         bp = &b.x;
    float*               rp = &result.x;
    for (int i = 0; i < 4; ++i)
        rp[i] = op == 0 ? ap[i] + bp[i] : op == 1 ? ap[i] - bp[i] : op == 2 ? ap[i] * bp[i] : ap[i] / bp[i];
    return result;
#endif
}

#define R_SPIRV_VEC_FUNCS(N, Type)                                                                           \
    Type R_Microbit_SpirvVec##N##Add (Type a, Type b)                                                        \
    {                                                                                                        \
        Type         r;                                                                                      \
        float*       rp = &r.x;                                                                              \
        const float* ap = &a.x;                                                                              \
        const float* bp = &b.x;                                                                              \
        for (int i = 0; i < N; ++i)                                                                          \
            rp[i] = ap[i] + bp[i];                                                                           \
        return r;                                                                                            \
    }                                                                                                        \
    Type R_Microbit_SpirvVec##N##Sub (Type a, Type b)                                                        \
    {                                                                                                        \
        Type         r;                                                                                      \
        float*       rp = &r.x;                                                                              \
        const float* ap = &a.x;                                                                              \
        const float* bp = &b.x;                                                                              \
        for (int i = 0; i < N; ++i)                                                                          \
            rp[i] = ap[i] - bp[i];                                                                           \
        return r;                                                                                            \
    }                                                                                                        \
    Type R_Microbit_SpirvVec##N##Mul (Type a, Type b)                                                        \
    {                                                                                                        \
        Type         r;                                                                                      \
        float*       rp = &r.x;                                                                              \
        const float* ap = &a.x;                                                                              \
        const float* bp = &b.x;                                                                              \
        for (int i = 0; i < N; ++i)                                                                          \
            rp[i] = ap[i] * bp[i];                                                                           \
        return r;                                                                                            \
    }                                                                                                        \
    Type R_Microbit_SpirvVec##N##Div (Type a, Type b)                                                        \
    {                                                                                                        \
        Type         r;                                                                                      \
        float*       rp = &r.x;                                                                              \
        const float* ap = &a.x;                                                                              \
        const float* bp = &b.x;                                                                              \
        for (int i = 0; i < N; ++i)                                                                          \
            rp[i] = ap[i] / bp[i];                                                                           \
        return r;                                                                                            \
    }

R_SPIRV_VEC_FUNCS (2, R_Microbit_SpirvVec2)
R_SPIRV_VEC_FUNCS (3, R_Microbit_SpirvVec3)

R_Microbit_SpirvVec4
R_Microbit_SpirvVec4Add (R_Microbit_SpirvVec4 a, R_Microbit_SpirvVec4 b)
{
    return R_SpirvVec4Apply (a, b, 0);
}
R_Microbit_SpirvVec4
R_Microbit_SpirvVec4Sub (R_Microbit_SpirvVec4 a, R_Microbit_SpirvVec4 b)
{
    return R_SpirvVec4Apply (a, b, 1);
}
R_Microbit_SpirvVec4
R_Microbit_SpirvVec4Mul (R_Microbit_SpirvVec4 a, R_Microbit_SpirvVec4 b)
{
    return R_SpirvVec4Apply (a, b, 2);
}
R_Microbit_SpirvVec4
R_Microbit_SpirvVec4Div (R_Microbit_SpirvVec4 a, R_Microbit_SpirvVec4 b)
{
    return R_SpirvVec4Apply (a, b, 3);
}

float
R_Microbit_SpirvVec2Dot (R_Microbit_SpirvVec2 a, R_Microbit_SpirvVec2 b)
{
    return a.x * b.x + a.y * b.y;
}
float
R_Microbit_SpirvVec3Dot (R_Microbit_SpirvVec3 a, R_Microbit_SpirvVec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
float
R_Microbit_SpirvVec4Dot (R_Microbit_SpirvVec4 a, R_Microbit_SpirvVec4 b)
{
#if defined(__SSE2__)
    __m128 product = _mm_mul_ps (_mm_loadu_ps (&a.x), _mm_loadu_ps (&b.x));
    __m128 high = _mm_movehl_ps (product, product);
    __m128 sum = _mm_add_ps (product, high);
    sum = _mm_add_ss (sum, _mm_shuffle_ps (sum, sum, 1));
    return _mm_cvtss_f32 (sum);
#else
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
#endif
}
R_Microbit_SpirvVec3
R_Microbit_SpirvVec3Cross (R_Microbit_SpirvVec3 a, R_Microbit_SpirvVec3 b)
{
    R_Microbit_SpirvVec3 r = {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
    return r;
}

#define R_SPIRV_NORM(N, Type, Dot)                                                                           \
    Type R_Microbit_SpirvVec##N##Normalize (Type v)                                                          \
    {                                                                                                        \
        float length = sqrtf (Dot (v, v));                                                                   \
        if (length <= FLT_MIN) return (Type){0};                                                             \
        float  inverse = 1.0f / length;                                                                      \
        float* p = &v.x;                                                                                     \
        for (int i = 0; i < N; ++i)                                                                          \
            p[i] *= inverse;                                                                                 \
        return v;                                                                                            \
    }                                                                                                        \
    float R_Microbit_SpirvVec##N##Length (Type v) { return sqrtf (Dot (v, v)); }
R_SPIRV_NORM (2, R_Microbit_SpirvVec2, R_Microbit_SpirvVec2Dot)
R_SPIRV_NORM (3, R_Microbit_SpirvVec3, R_Microbit_SpirvVec3Dot)
R_SPIRV_NORM (4, R_Microbit_SpirvVec4, R_Microbit_SpirvVec4Dot)

R_Microbit_SpirvVec3
R_Microbit_SpirvVec3Reflect (R_Microbit_SpirvVec3 incident, R_Microbit_SpirvVec3 normal)
{
    float scale = 2.0f * R_Microbit_SpirvVec3Dot (normal, incident);
    return R_Microbit_SpirvVec3Sub (
        incident,
        (R_Microbit_SpirvVec3){normal.x * scale, normal.y * scale, normal.z * scale});
}
R_Microbit_SpirvVec3
R_Microbit_SpirvVec3Refract (R_Microbit_SpirvVec3 incident, R_Microbit_SpirvVec3 normal, float eta)
{
    float c = R_Microbit_SpirvVec3Dot (normal, incident);
    float k = 1.0f - eta * eta * (1.0f - c * c);
    if (k < 0.0f) return (R_Microbit_SpirvVec3){0};
    float s = eta * c + sqrtf (k);
    return (R_Microbit_SpirvVec3){eta * incident.x - s * normal.x,
                                  eta * incident.y - s * normal.y,
                                  eta * incident.z - s * normal.z};
}

float
R_Microbit_SpirvRadians (float v)
{
    return v * (R_SPIRV_PI / 180.0f);
}
float
R_Microbit_SpirvDegrees (float v)
{
    return v * (180.0f / R_SPIRV_PI);
}
float
R_Microbit_SpirvMod (float x, float y)
{
    return x - y * floorf (x / y);
}
float
R_Microbit_SpirvStep (float edge, float x)
{
    return x < edge ? 0.0f : 1.0f;
}
float
R_Microbit_SpirvSmoothstep (float a, float b, float x)
{
    float t = fminf (fmaxf ((x - a) / (b - a), 0.0f), 1.0f);
    return t * t * (3.0f - 2.0f * t);
}
float
R_Microbit_SpirvRoundEven (float v)
{
    return nearbyintf (v);
}
float
R_Microbit_SpirvInverseSqrt (float v)
{
    return 1.0f / sqrtf (v);
}

R_Microbit_SpirvMat3
R_Microbit_SpirvMat3Identity (void)
{
    R_Microbit_SpirvMat3 m = {0};
    for (int i = 0; i < 3; ++i)
        m.v[i][i] = 1.0f;
    return m;
}
R_Microbit_SpirvMat4
R_Microbit_SpirvMat4Identity (void)
{
    R_Microbit_SpirvMat4 m = {0};
    for (int i = 0; i < 4; ++i)
        m.v[i][i] = 1.0f;
    return m;
}
#define R_SPIRV_MAT_MUL(N, Type)                                                                             \
    Type R_Microbit_SpirvMat##N##Mul (Type a, Type b)                                                        \
    {                                                                                                        \
        Type r = {0};                                                                                        \
        for (int i = 0; i < N; ++i)                                                                          \
            for (int j = 0; j < N; ++j)                                                                      \
                for (int k = 0; k < N; ++k)                                                                  \
                    r.v[i][j] += a.v[i][k] * b.v[k][j];                                                      \
        return r;                                                                                            \
    }                                                                                                        \
    Type R_Microbit_SpirvMat##N##Transpose (Type a)                                                          \
    {                                                                                                        \
        Type r;                                                                                              \
        for (int i = 0; i < N; ++i)                                                                          \
            for (int j = 0; j < N; ++j)                                                                      \
                r.v[i][j] = a.v[j][i];                                                                       \
        return r;                                                                                            \
    }
R_SPIRV_MAT_MUL (3, R_Microbit_SpirvMat3)
R_SPIRV_MAT_MUL (4, R_Microbit_SpirvMat4)
R_Microbit_SpirvVec3
R_Microbit_SpirvMat3MulVec3 (R_Microbit_SpirvMat3 m, R_Microbit_SpirvVec3 v)
{
    return (R_Microbit_SpirvVec3){m.v[0][0] * v.x + m.v[0][1] * v.y + m.v[0][2] * v.z,
                                  m.v[1][0] * v.x + m.v[1][1] * v.y + m.v[1][2] * v.z,
                                  m.v[2][0] * v.x + m.v[2][1] * v.y + m.v[2][2] * v.z};
}
R_Microbit_SpirvVec4
R_Microbit_SpirvMat4MulVec4 (R_Microbit_SpirvMat4 m, R_Microbit_SpirvVec4 v)
{
    R_Microbit_SpirvVec4 r;
    float*               p = &r.x;
    float*               q = &v.x;
    for (int i = 0; i < 4; ++i)
    {
        p[i] = 0;
        for (int j = 0; j < 4; ++j)
            p[i] += m.v[i][j] * q[j];
    }
    return r;
}
int
R_Microbit_SpirvMat3Inverse (R_Microbit_SpirvMat3 a, R_Microbit_SpirvMat3* out)
{
    if (!out) return 0;
    float d = a.v[0][0] * (a.v[1][1] * a.v[2][2] - a.v[1][2] * a.v[2][1])
              - a.v[0][1] * (a.v[1][0] * a.v[2][2] - a.v[1][2] * a.v[2][0])
              + a.v[0][2] * (a.v[1][0] * a.v[2][1] - a.v[1][1] * a.v[2][0]);
    if (fabsf (d) <= FLT_EPSILON) return 0;
    float q = 1.0f / d;
    out->v[0][0] = (a.v[1][1] * a.v[2][2] - a.v[1][2] * a.v[2][1]) * q;
    out->v[0][1] = (a.v[0][2] * a.v[2][1] - a.v[0][1] * a.v[2][2]) * q;
    out->v[0][2] = (a.v[0][1] * a.v[1][2] - a.v[0][2] * a.v[1][1]) * q;
    out->v[1][0] = (a.v[1][2] * a.v[2][0] - a.v[1][0] * a.v[2][2]) * q;
    out->v[1][1] = (a.v[0][0] * a.v[2][2] - a.v[0][2] * a.v[2][0]) * q;
    out->v[1][2] = (a.v[0][2] * a.v[1][0] - a.v[0][0] * a.v[1][2]) * q;
    out->v[2][0] = (a.v[1][0] * a.v[2][1] - a.v[1][1] * a.v[2][0]) * q;
    out->v[2][1] = (a.v[0][1] * a.v[2][0] - a.v[0][0] * a.v[2][1]) * q;
    out->v[2][2] = (a.v[0][0] * a.v[1][1] - a.v[0][1] * a.v[1][0]) * q;
    return 1;
}
int
R_Microbit_SpirvMat4Inverse (R_Microbit_SpirvMat4 a, R_Microbit_SpirvMat4* out)
{
    if (!out) return 0;
    R_Microbit_SpirvMat4 aug = a;
    R_Microbit_SpirvMat4 inv = R_Microbit_SpirvMat4Identity ();
    for (int i = 0; i < 4; ++i)
    {
        int pivot = i;
        for (int r = i + 1; r < 4; ++r)
            if (fabsf (aug.v[r][i]) > fabsf (aug.v[pivot][i])) pivot = r;
        if (fabsf (aug.v[pivot][i]) <= FLT_EPSILON) return 0;
        for (int c = 0; c < 4; ++c)
        {
            float t = aug.v[i][c];
            aug.v[i][c] = aug.v[pivot][c];
            aug.v[pivot][c] = t;
            t = inv.v[i][c];
            inv.v[i][c] = inv.v[pivot][c];
            inv.v[pivot][c] = t;
        }
        float q = 1.0f / aug.v[i][i];
        for (int c = 0; c < 4; ++c)
        {
            aug.v[i][c] *= q;
            inv.v[i][c] *= q;
        }
        for (int r = 0; r < 4; ++r)
            if (r != i)
            {
                q = aug.v[r][i];
                for (int c = 0; c < 4; ++c)
                {
                    aug.v[r][c] -= q * aug.v[i][c];
                    inv.v[r][c] -= q * inv.v[i][c];
                }
            }
    }
    *out = inv;
    return 1;
}
