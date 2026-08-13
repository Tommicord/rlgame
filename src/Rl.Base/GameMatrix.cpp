#include "Rl.Base/GameMatrix.h"

#include <cmath>

namespace rl
{

Vec2::Vec2(const Vec3& other) : x(other.x), y(other.y)
{
}
Vec2::Vec2(const Vec4& other) : x(other.x), y(other.y)
{
}

Vec2 Vec2::operator+(const Vec2& other) const
{
#ifdef _RL_SIMD_X86
        __m128 a      = _mm_set_ps(0.0f, 0.0f, y, x);
        __m128 b      = _mm_set_ps(0.0f, 0.0f, other.y, other.x);
        __m128 result = _mm_add_ps(a, b);
        float  res[4];
        _mm_storeu_ps(res, result);
        return Vec2(res[0], res[1]);
#elif defined(_RL_SIMD_ARM_NEON)
        float32x2_t a      = vld1_f32(&x);
        float32x2_t b      = vld1_f32(&other.x);
        float32x2_t result = vadd_f32(a, b);
        Vec2        res;
        vst1_f32(&res.x, result);
        return res;
#else
        return Vec2(x + other.x, y + other.y);
#endif
}

Vec2 Vec2::operator-(const Vec2& other) const
{
#ifdef _RL_SIMD_X86
        __m128 a      = _mm_set_ps(0.0f, 0.0f, y, x);
        __m128 b      = _mm_set_ps(0.0f, 0.0f, other.y, other.x);
        __m128 result = _mm_sub_ps(a, b);
        float  res[4];
        _mm_storeu_ps(res, result);
        return Vec2(res[0], res[1]);
#elif defined(_RL_SIMD_ARM_NEON)
        float32x2_t a      = vld1_f32(&x);
        float32x2_t b      = vld1_f32(&other.x);
        float32x2_t result = vsub_f32(a, b);
        Vec2        res;
        vst1_f32(&res.x, result);
        return res;
#else
        return Vec2(x - other.x, y - other.y);
#endif
}

Vec2 Vec2::operator*(float scalar) const
{
#ifdef _RL_SIMD_X86
        __m128 a      = _mm_set_ps(0.0f, 0.0f, y, x);
        __m128 s      = _mm_set1_ps(scalar);
        __m128 result = _mm_mul_ps(a, s);
        float  res[4];
        _mm_storeu_ps(res, result);
        return Vec2(res[0], res[1]);
#elif defined(_RL_SIMD_ARM_NEON)
        float32x2_t a      = vld1_f32(&x);
        float32x2_t s      = vdup_n_f32(scalar);
        float32x2_t result = vmul_f32(a, s);
        Vec2        res;
        vst1_f32(&res.x, result);
        return res;
#else
        return Vec2(x * scalar, y * scalar);
#endif
}

Vec2 Vec2::operator/(float scalar) const
{
        return Vec2(x / scalar, y / scalar);
}

Vec2& Vec2::operator+=(const Vec2& other)
{
        x += other.x;
        y += other.y;
        return *this;
}

Vec2& Vec2::operator-=(const Vec2& other)
{
        x -= other.x;
        y -= other.y;
        return *this;
}

Vec2& Vec2::operator*=(float scalar)
{
        x *= scalar;
        y *= scalar;
        return *this;
}

Vec2& Vec2::operator/=(float scalar)
{
        x /= scalar;
        y /= scalar;
        return *this;
}

float Vec2::dot(const Vec2& other) const
{
#ifdef _RL_SIMD_X86
        __m128 a   = _mm_set_ps(0.0f, 0.0f, y, x);
        __m128 b   = _mm_set_ps(0.0f, 0.0f, other.y, other.x);
        __m128 mul = _mm_mul_ps(a, b);
        __m128 sum = _mm_hadd_ps(mul, mul);
        float  res[4];
        _mm_storeu_ps(res, sum);
        return res[0];
#elif defined(_RL_SIMD_ARM_NEON)
        float32x2_t a   = vld1_f32(&x);
        float32x2_t b   = vld1_f32(&other.x);
        float32x2_t mul = vmul_f32(a, b);
        float32x2_t sum = vpadd_f32(mul, mul);
        return vget_lane_f32(sum, 0);
#else
        return x * other.x + y * other.y;
#endif
}

float Vec2::length() const
{
        return std::sqrt(lengthSquared());
}

float Vec2::lengthSquared() const
{
        return x * x + y * y;
}

Vec2 Vec2::normalized() const
{
        float len = length();
        if (len > 0.0f)
        {
                return Vec2(x / len, y / len);
        }
        return Vec2(0.0f, 0.0f);
}

Vec2& Vec2::normalize()
{
        float len = length();
        if (len > 0.0f)
        {
                x /= len;
                y /= len;
        }
        return *this;
}

Vec3::Vec3(const Vec2& other) : x(other.x), y(other.y), z(0.0f)
{
}
Vec3::Vec3(const Vec4& other) : x(other.x), y(other.y), z(other.z)
{
}

Vec3 Vec3::operator+(const Vec3& other) const
{
        return Vec3(x + other.x, y + other.y, z + other.z);
}

Vec3 Vec3::operator-(const Vec3& other) const
{
        return Vec3(x - other.x, y - other.y, z - other.z);
}

Vec3 Vec3::operator*(float scalar) const
{
        return Vec3(x * scalar, y * scalar, z * scalar);
}

Vec3 Vec3::operator/(float scalar) const
{
        return Vec3(x / scalar, y / scalar, z / scalar);
}

Vec3& Vec3::operator+=(const Vec3& other)
{
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
}

Vec3& Vec3::operator-=(const Vec3& other)
{
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
}

Vec3& Vec3::operator*=(float scalar)
{
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
}

Vec3& Vec3::operator/=(float scalar)
{
        x /= scalar;
        y /= scalar;
        z /= scalar;
        return *this;
}

float Vec3::dot(const Vec3& other) const
{
#ifdef _RL_SIMD_X86
        __m128 a   = _mm_set_ps(0.0f, z, y, x);
        __m128 b   = _mm_set_ps(0.0f, other.z, other.y, other.x);
        __m128 mul = _mm_mul_ps(a, b);
        __m128 sum = _mm_hadd_ps(mul, mul);
        sum        = _mm_hadd_ps(sum, sum);
        float res[4];
        _mm_storeu_ps(res, sum);
        return res[0];
#elif defined(_RL_SIMD_ARM_NEON)
        float32x4_t a   = vsetq_lane_f32(0.0f, vld1_f32(&x), 3);
        float32x4_t b   = vsetq_lane_f32(0.0f, vld1_f32(&other.x), 3);
        float32x4_t mul = vmulq_f32(a, b);
        float32x2_t sum = vadd_f32(vget_low_f32(mul), vget_high_f32(mul));
        sum             = vpadd_f32(sum, sum);
        return vget_lane_f32(sum, 0);
#else
        return x * other.x + y * other.y + z * other.z;
#endif
}

Vec3 Vec3::cross(const Vec3& other) const
{
        return Vec3(y * other.z - z * other.y, z * other.x - x * other.z,
                    x * other.y - y * other.x);
}

float Vec3::length() const
{
        return std::sqrt(lengthSquared());
}

float Vec3::lengthSquared() const
{
        return x * x + y * y + z * z;
}

Vec3 Vec3::normalized() const
{
        float len = length();
        if (len > 0.0f)
        {
                return Vec3(x / len, y / len, z / len);
        }
        return Vec3(0.0f, 0.0f, 0.0f);
}

Vec3& Vec3::normalize()
{
        float len = length();
        if (len > 0.0f)
        {
                x /= len;
                y /= len;
                z /= len;
        }
        return *this;
}

// ============================================================================
// Vec4 Implementation
// ============================================================================

Vec4::Vec4(const Vec2& other) : x(other.x), y(other.y), z(0.0f), w(0.0f)
{
}
Vec4::Vec4(const Vec3& other) : x(other.x), y(other.y), z(other.z), w(0.0f)
{
}

Vec4 Vec4::operator+(const Vec4& other) const
{
#ifdef _RL_SIMD_X86
        __m128 a      = _mm_set_ps(w, z, y, x);
        __m128 b      = _mm_set_ps(other.w, other.z, other.y, other.x);
        __m128 result = _mm_add_ps(a, b);
        float  res[4];
        _mm_storeu_ps(res, result);
        return Vec4(res[0], res[1], res[2], res[3]);
#elif defined(_RL_SIMD_ARM_NEON)
        float32x4_t a      = vld1q_f32(&x);
        float32x4_t b      = vld1q_f32(&other.x);
        float32x4_t result = vaddq_f32(a, b);
        Vec4        res;
        vst1q_f32(&res.x, result);
        return res;
#else
        return Vec4(x + other.x, y + other.y, z + other.z, w + other.w);
#endif
}

Vec4 Vec4::operator-(const Vec4& other) const
{
#ifdef _RL_SIMD_X86
        __m128 a      = _mm_set_ps(w, z, y, x);
        __m128 b      = _mm_set_ps(other.w, other.z, other.y, other.x);
        __m128 result = _mm_sub_ps(a, b);
        float  res[4];
        _mm_storeu_ps(res, result);
        return Vec4(res[0], res[1], res[2], res[3]);
#elif defined(_RL_SIMD_ARM_NEON)
        float32x4_t a      = vld1q_f32(&x);
        float32x4_t b      = vld1q_f32(&other.x);
        float32x4_t result = vsubq_f32(a, b);
        Vec4        res;
        vst1q_f32(&res.x, result);
        return res;
#else
        return Vec4(x - other.x, y - other.y, z - other.z, w - other.w);
#endif
}

Vec4 Vec4::operator*(float scalar) const
{
#ifdef _RL_SIMD_X86
        __m128 a      = _mm_set_ps(w, z, y, x);
        __m128 s      = _mm_set1_ps(scalar);
        __m128 result = _mm_mul_ps(a, s);
        float  res[4];
        _mm_storeu_ps(res, result);
        return Vec4(res[0], res[1], res[2], res[3]);
#elif defined(_RL_SIMD_ARM_NEON)
        float32x4_t a      = vld1q_f32(&x);
        float32x4_t s      = vdupq_n_f32(scalar);
        float32x4_t result = vmulq_f32(a, s);
        Vec4        res;
        vst1q_f32(&res.x, result);
        return res;
#else
        return Vec4(x * scalar, y * scalar, z * scalar, w * scalar);
#endif
}

Vec4 Vec4::operator/(float scalar) const
{
        return Vec4(x / scalar, y / scalar, z / scalar, w / scalar);
}

Vec4& Vec4::operator+=(const Vec4& other)
{
        x += other.x;
        y += other.y;
        z += other.z;
        w += other.w;
        return *this;
}

Vec4& Vec4::operator-=(const Vec4& other)
{
        x -= other.x;
        y -= other.y;
        z -= other.z;
        w -= other.w;
        return *this;
}

Vec4& Vec4::operator*=(float scalar)
{
        x *= scalar;
        y *= scalar;
        z *= scalar;
        w *= scalar;
        return *this;
}

Vec4& Vec4::operator/=(float scalar)
{
        x /= scalar;
        y /= scalar;
        z /= scalar;
        w /= scalar;
        return *this;
}

float Vec4::dot(const Vec4& other) const
{
#ifdef _RL_SIMD_X86
        __m128 a   = _mm_set_ps(w, z, y, x);
        __m128 b   = _mm_set_ps(other.w, other.z, other.y, other.x);
        __m128 mul = _mm_mul_ps(a, b);
        __m128 sum = _mm_hadd_ps(mul, mul);
        sum        = _mm_hadd_ps(sum, sum);
        float res[4];
        _mm_storeu_ps(res, sum);
        return res[0];
#elif defined(_RL_SIMD_ARM_NEON)
        float32x4_t a   = vld1q_f32(&x);
        float32x4_t b   = vld1q_f32(&other.x);
        float32x4_t mul = vmulq_f32(a, b);
        float32x2_t sum = vadd_f32(vget_low_f32(mul), vget_high_f32(mul));
        sum             = vpadd_f32(sum, sum);
        return vget_lane_f32(sum, 0);
#else
        return x * other.x + y * other.y + z * other.z + w * other.w;
#endif
}

float Vec4::length() const
{
        return std::sqrt(lengthSquared());
}

float Vec4::lengthSquared() const
{
        return x * x + y * y + z * z + w * w;
}

Vec4 Vec4::normalized() const
{
        float len = length();
        if (len > 0.0f)
        {
                return Vec4(x / len, y / len, z / len, w / len);
        }
        return Vec4(0.0f, 0.0f, 0.0f, 0.0f);
}

Vec4& Vec4::normalize()
{
        float len = length();
        if (len > 0.0f)
        {
                x /= len;
                y /= len;
                z /= len;
                w /= len;
        }
        return *this;
}

Mat2::Mat2()
{
        m[0] = 1.0f;
        m[1] = 0.0f;
        m[2] = 0.0f;
        m[3] = 1.0f;
}

Mat2::Mat2(float m00, float m01, float m10, float m11)
{
        m[0] = m00;
        m[1] = m01;
        m[2] = m10;
        m[3] = m11;
}

float& Mat2::operator()(int row, int col)
{
        return m[row * 2 + col];
}

const float& Mat2::operator()(int row, int col) const
{
        return m[row * 2 + col];
}

Mat2 Mat2::operator+(const Mat2& other) const
{
#ifdef _RL_SIMD_X86
        Mat2   result;
        __m128 a = _mm_loadu_ps(m);
        __m128 b = _mm_loadu_ps(other.m);
        _mm_storeu_ps(result.m, _mm_add_ps(a, b));
        return result;
#elif defined(_RL_SIMD_ARM_NEON)
        Mat2        result;
        float32x2_t a = vld1_f32(m);
        float32x2_t b = vld1_f32(other.m);
        vst1_f32(result.m, vadd_f32(a, b));
        return result;
#else
        Mat2 result;
        for (int i = 0; i < 4; ++i)
                result.m[i] = m[i] + other.m[i];
        return result;
#endif
}

Mat2 Mat2::operator-(const Mat2& other) const
{
#ifdef _RL_SIMD_X86
        Mat2   result;
        __m128 a = _mm_loadu_ps(m);
        __m128 b = _mm_loadu_ps(other.m);
        _mm_storeu_ps(result.m, _mm_sub_ps(a, b));
        return result;
#elif defined(_RL_SIMD_ARM_NEON)
        Mat2        result;
        float32x2_t a = vld1_f32(m);
        float32x2_t b = vld1_f32(other.m);
        vst1_f32(result.m, vsub_f32(a, b));
        return result;
#else
        Mat2 result;
        for (int i = 0; i < 4; ++i)
                result.m[i] = m[i] - other.m[i];
        return result;
#endif
}

Mat2 Mat2::operator*(const Mat2& other) const
{
        Mat2 result;
        result.m[0] = m[0] * other.m[0] + m[1] * other.m[2];
        result.m[1] = m[0] * other.m[1] + m[1] * other.m[3];
        result.m[2] = m[2] * other.m[0] + m[3] * other.m[2];
        result.m[3] = m[2] * other.m[1] + m[3] * other.m[3];
        return result;
}

Mat2 Mat2::operator*(float scalar) const
{
#ifdef _RL_SIMD_X86
        Mat2   result;
        __m128 a = _mm_loadu_ps(m);
        __m128 s = _mm_set1_ps(scalar);
        _mm_storeu_ps(result.m, _mm_mul_ps(a, s));
        return result;
#elif defined(_RL_SIMD_ARM_NEON)
        Mat2        result;
        float32x2_t a = vld1_f32(m);
        float32x2_t s = vdup_n_f32(scalar);
        vst1_f32(result.m, vmul_f32(a, s));
        return result;
#else
        Mat2 result;
        for (int i = 0; i < 4; ++i)
                result.m[i] = m[i] * scalar;
        return result;
#endif
}

Vec2 Mat2::operator*(const Vec2& vec) const
{
        return Vec2(m[0] * vec.x + m[1] * vec.y, m[2] * vec.x + m[3] * vec.y);
}

Mat2 Mat2::transpose() const
{
        return Mat2(m[0], m[2], m[1], m[3]);
}

float Mat2::determinant() const
{
        return m[0] * m[3] - m[1] * m[2];
}

Mat2 Mat2::inverse() const
{
        float det = determinant();
        if (std::abs(det) < 1e-6f)
                return Mat2(); // Return identity if singular

        float invDet = 1.0f / det;
        return Mat2(m[3] * invDet, -m[1] * invDet, -m[2] * invDet, m[0] * invDet);
}

Mat2 Mat2::identity()
{
        return Mat2();
}

Mat3::Mat3()
{
        for (int i = 0; i < 9; ++i)
                m[i] = 0.0f;
        m[0] = 1.0f;
        m[4] = 1.0f;
        m[8] = 1.0f;
}

Mat3::Mat3(float m00,
           float m01,
           float m02,
           float m10,
           float m11,
           float m12,
           float m20,
           float m21,
           float m22)
{
        m[0] = m00;
        m[1] = m01;
        m[2] = m02;
        m[3] = m10;
        m[4] = m11;
        m[5] = m12;
        m[6] = m20;
        m[7] = m21;
        m[8] = m22;
}

float& Mat3::operator()(int row, int col)
{
        return m[row * 3 + col];
}

const float& Mat3::operator()(int row, int col) const
{
        return m[row * 3 + col];
}

Mat3 Mat3::operator+(const Mat3& other) const
{
#ifdef _RL_SIMD_X86
        Mat3   result;
        __m128 a0 = _mm_loadu_ps(m);
        __m128 a1 = _mm_set_ps(0.0f, m[8], m[7], m[6]);
        __m128 b0 = _mm_loadu_ps(other.m);
        __m128 b1 = _mm_set_ps(0.0f, other.m[8], other.m[7], other.m[6]);
        _mm_storeu_ps(result.m, _mm_add_ps(a0, b0));
        _mm_storeu_ps(result.m + 6, _mm_add_ps(a1, b1));
        return result;
#elif defined(_RL_SIMD_ARM_NEON)
        Mat3        result;
        float32x4_t a0 = vld1q_f32(m);
        float32x4_t a1 = vsetq_lane_f32(m[8], vld1q_f32(m + 5), 3);
        float32x4_t b0 = vld1q_f32(other.m);
        float32x4_t b1 = vsetq_lane_f32(other.m[8], vld1q_f32(other.m + 5), 3);
        vst1q_f32(result.m, vaddq_f32(a0, b0));
        vst1q_f32(result.m + 5, vaddq_f32(a1, b1));
        return result;
#else
        Mat3 result;
        for (int i = 0; i < 9; ++i)
                result.m[i] = m[i] + other.m[i];
        return result;
#endif
}

Mat3 Mat3::operator-(const Mat3& other) const
{
#ifdef _RL_SIMD_X86
        Mat3   result;
        __m128 a0 = _mm_loadu_ps(m);
        __m128 a1 = _mm_set_ps(0.0f, m[8], m[7], m[6]);
        __m128 b0 = _mm_loadu_ps(other.m);
        __m128 b1 = _mm_set_ps(0.0f, other.m[8], other.m[7], other.m[6]);
        _mm_storeu_ps(result.m, _mm_sub_ps(a0, b0));
        _mm_storeu_ps(result.m + 6, _mm_sub_ps(a1, b1));
        return result;
#elif defined(_RL_SIMD_ARM_NEON)
        Mat3        result;
        float32x4_t a0 = vld1q_f32(m);
        float32x4_t a1 = vsetq_lane_f32(m[8], vld1q_f32(m + 5), 3);
        float32x4_t b0 = vld1q_f32(other.m);
        float32x4_t b1 = vsetq_lane_f32(other.m[8], vld1q_f32(other.m + 5), 3);
        vst1q_f32(result.m, vsubq_f32(a0, b0));
        vst1q_f32(result.m + 5, vsubq_f32(a1, b1));
        return result;
#else
        Mat3 result;
        for (int i = 0; i < 9; ++i)
                result.m[i] = m[i] - other.m[i];
        return result;
#endif
}

Mat3 Mat3::operator*(const Mat3& other) const
{
        Mat3 result;
        for (int row = 0; row < 3; ++row)
        {
                for (int col = 0; col < 3; ++col)
                {
                        result.m[row * 3 + col] = 0.0f;
                        for (int k = 0; k < 3; ++k)
                                result.m[row * 3 + col] += m[row * 3 + k] * other.m[k * 3 + col];
                }
        }
        return result;
}

Mat3 Mat3::operator*(float scalar) const
{
#ifdef _RL_SIMD_X86
        Mat3   result;
        __m128 a0 = _mm_loadu_ps(m);
        __m128 a1 = _mm_set_ps(0.0f, m[8], m[7], m[6]);
        __m128 s  = _mm_set1_ps(scalar);
        _mm_storeu_ps(result.m, _mm_mul_ps(a0, s));
        _mm_storeu_ps(result.m + 6, _mm_mul_ps(a1, s));
        return result;
#elif defined(_RL_SIMD_ARM_NEON)
        Mat3        result;
        float32x4_t a0 = vld1q_f32(m);
        float32x4_t a1 = vsetq_lane_f32(m[8], vld1q_f32(m + 5), 3);
        float32x4_t s  = vdupq_n_f32(scalar);
        vst1q_f32(result.m, vmulq_f32(a0, s));
        vst1q_f32(result.m + 5, vmulq_f32(a1, s));
        return result;
#else
        Mat3 result;
        for (int i = 0; i < 9; ++i)
                result.m[i] = m[i] * scalar;
        return result;
#endif
}

Vec3 Mat3::operator*(const Vec3& vec) const
{
        return Vec3(m[0] * vec.x + m[1] * vec.y + m[2] * vec.z,
                    m[3] * vec.x + m[4] * vec.y + m[5] * vec.z,
                    m[6] * vec.x + m[7] * vec.y + m[8] * vec.z);
}

Mat3 Mat3::transpose() const
{
        return Mat3(m[0], m[3], m[6], m[1], m[4], m[7], m[2], m[5], m[8]);
}

float Mat3::determinant() const
{
        return m[0] * (m[4] * m[8] - m[5] * m[7]) - m[1] * (m[3] * m[8] - m[5] * m[6]) +
               m[2] * (m[3] * m[7] - m[4] * m[6]);
}

Mat3 Mat3::inverse() const
{
        float det = determinant();
        if (std::abs(det) < 1e-6f)
                return Mat3(); // Return identity if singular

        float invDet = 1.0f / det;

        Mat3 result;
        result.m[0] = (m[4] * m[8] - m[5] * m[7]) * invDet;
        result.m[1] = (m[2] * m[7] - m[1] * m[8]) * invDet;
        result.m[2] = (m[1] * m[5] - m[2] * m[4]) * invDet;
        result.m[3] = (m[5] * m[6] - m[3] * m[8]) * invDet;
        result.m[4] = (m[0] * m[8] - m[2] * m[6]) * invDet;
        result.m[5] = (m[2] * m[3] - m[0] * m[5]) * invDet;
        result.m[6] = (m[3] * m[7] - m[4] * m[6]) * invDet;
        result.m[7] = (m[1] * m[6] - m[0] * m[7]) * invDet;
        result.m[8] = (m[0] * m[4] - m[1] * m[3]) * invDet;

        return result;
}

Mat3 Mat3::identity()
{
        return Mat3();
}

Mat4::Mat4()
{
        for (int i = 0; i < 16; ++i)
                m[i] = 0.0f;
        m[0]  = 1.0f;
        m[5]  = 1.0f;
        m[10] = 1.0f;
        m[15] = 1.0f;
}

Mat4::Mat4(const Mat4& other)
{
        for (int i = 0; i < 16; ++i)
                m[i] = other.m[i];
}

Mat4::Mat4(Mat4&& other) noexcept
{
        for (int i = 0; i < 16; ++i)
                m[i] = other.m[i];
}

Mat4& Mat4::operator=(const Mat4& other)
{
        for (int i = 0; i < 16; ++i)
                m[i] = other.m[i];
        return *this;
}

Mat4& Mat4::operator=(Mat4&& other) noexcept
{
        for (int i = 0; i < 16; ++i)
                m[i] = other.m[i];
        return *this;
}

Mat4::Mat4(float m00,
           float m01,
           float m02,
           float m03,
           float m10,
           float m11,
           float m12,
           float m13,
           float m20,
           float m21,
           float m22,
           float m23,
           float m30,
           float m31,
           float m32,
           float m33)
{
        m[0]  = m00;
        m[1]  = m01;
        m[2]  = m02;
        m[3]  = m03;
        m[4]  = m10;
        m[5]  = m11;
        m[6]  = m12;
        m[7]  = m13;
        m[8]  = m20;
        m[9]  = m21;
        m[10] = m22;
        m[11] = m23;
        m[12] = m30;
        m[13] = m31;
        m[14] = m32;
        m[15] = m33;
}

float& Mat4::operator()(int row, int col)
{
        return m[row * 4 + col];
}

const float& Mat4::operator()(int row, int col) const
{
        return m[row * 4 + col];
}

Mat4 Mat4::operator+(const Mat4& other) const
{
#ifdef _RL_SIMD_X86
        Mat4   result;
        __m128 a0 = _mm_loadu_ps(m);
        __m128 a1 = _mm_loadu_ps(m + 4);
        __m128 a2 = _mm_loadu_ps(m + 8);
        __m128 a3 = _mm_loadu_ps(m + 12);
        __m128 b0 = _mm_loadu_ps(other.m);
        __m128 b1 = _mm_loadu_ps(other.m + 4);
        __m128 b2 = _mm_loadu_ps(other.m + 8);
        __m128 b3 = _mm_loadu_ps(other.m + 12);
        _mm_storeu_ps(result.m, _mm_add_ps(a0, b0));
        _mm_storeu_ps(result.m + 4, _mm_add_ps(a1, b1));
        _mm_storeu_ps(result.m + 8, _mm_add_ps(a2, b2));
        _mm_storeu_ps(result.m + 12, _mm_add_ps(a3, b3));
        return result;
#elif defined(_RL_SIMD_ARM_NEON)
        Mat4        result;
        float32x4_t a0 = vld1q_f32(m);
        float32x4_t a1 = vld1q_f32(m + 4);
        float32x4_t a2 = vld1q_f32(m + 8);
        float32x4_t a3 = vld1q_f32(m + 12);
        float32x4_t b0 = vld1q_f32(other.m);
        float32x4_t b1 = vld1q_f32(other.m + 4);
        float32x4_t b2 = vld1q_f32(other.m + 8);
        float32x4_t b3 = vld1q_f32(other.m + 12);
        vst1q_f32(result.m, vaddq_f32(a0, b0));
        vst1q_f32(result.m + 4, vaddq_f32(a1, b1));
        vst1q_f32(result.m + 8, vaddq_f32(a2, b2));
        vst1q_f32(result.m + 12, vaddq_f32(a3, b3));
        return result;
#else
        Mat4 result = Mat4(); // Explicitly call constructor to initialize
        for (int i = 0; i < 16; ++i)
                result.m[i] = m[i] + other.m[i];
        return result;
#endif
}

Mat4 Mat4::operator-(const Mat4& other) const
{
#ifdef _RL_SIMD_X86
        Mat4   result;
        __m128 a0 = _mm_loadu_ps(m);
        __m128 a1 = _mm_loadu_ps(m + 4);
        __m128 a2 = _mm_loadu_ps(m + 8);
        __m128 a3 = _mm_loadu_ps(m + 12);
        __m128 b0 = _mm_loadu_ps(other.m);
        __m128 b1 = _mm_loadu_ps(other.m + 4);
        __m128 b2 = _mm_loadu_ps(other.m + 8);
        __m128 b3 = _mm_loadu_ps(other.m + 12);
        _mm_storeu_ps(result.m, _mm_sub_ps(a0, b0));
        _mm_storeu_ps(result.m + 4, _mm_sub_ps(a1, b1));
        _mm_storeu_ps(result.m + 8, _mm_sub_ps(a2, b2));
        _mm_storeu_ps(result.m + 12, _mm_sub_ps(a3, b3));
        return result;
#elif defined(_RL_SIMD_ARM_NEON)
        Mat4        result;
        float32x4_t a0 = vld1q_f32(m);
        float32x4_t a1 = vld1q_f32(m + 4);
        float32x4_t a2 = vld1q_f32(m + 8);
        float32x4_t a3 = vld1q_f32(m + 12);
        float32x4_t b0 = vld1q_f32(other.m);
        float32x4_t b1 = vld1q_f32(other.m + 4);
        float32x4_t b2 = vld1q_f32(other.m + 8);
        float32x4_t b3 = vld1q_f32(other.m + 12);
        vst1q_f32(result.m, vsubq_f32(a0, b0));
        vst1q_f32(result.m + 4, vsubq_f32(a1, b1));
        vst1q_f32(result.m + 8, vsubq_f32(a2, b2));
        vst1q_f32(result.m + 12, vsubq_f32(a3, b3));
        return result;
#else
        Mat4 result = Mat4(); // Explicitly call constructor to initialize
        for (int i = 0; i < 16; ++i)
                result.m[i] = m[i] - other.m[i];
        return result;
#endif
}

Mat4 Mat4::operator*(const Mat4& other) const
{
#ifdef _RL_SIMD_X86
        __m128 a0 = _mm_loadu_ps(m); // this row 0
        __m128 a1 = _mm_loadu_ps(m + 4); // this row 1
        __m128 a2 = _mm_loadu_ps(m + 8); // this row 2
        __m128 a3 = _mm_loadu_ps(m + 12); // this row 3

        __m128 b0 = _mm_loadu_ps(other.m); // other row 0
        __m128 b1 = _mm_loadu_ps(other.m + 4); // other row 1
        __m128 b2 = _mm_loadu_ps(other.m + 8); // other row 2
        __m128 b3 = _mm_loadu_ps(other.m + 12); // other row 3

        __m128 r0 = _mm_mul_ps(_mm_shuffle_ps(a0, a0, _MM_SHUFFLE(0, 0, 0, 0)), b0);
        r0        = _mm_add_ps(r0, _mm_mul_ps(_mm_shuffle_ps(a0, a0, _MM_SHUFFLE(1, 1, 1, 1)), b1));
        r0        = _mm_add_ps(r0, _mm_mul_ps(_mm_shuffle_ps(a0, a0, _MM_SHUFFLE(2, 2, 2, 2)), b2));
        r0        = _mm_add_ps(r0, _mm_mul_ps(_mm_shuffle_ps(a0, a0, _MM_SHUFFLE(3, 3, 3, 3)), b3));

        __m128 r1 = _mm_mul_ps(_mm_shuffle_ps(a1, a1, _MM_SHUFFLE(0, 0, 0, 0)), b0);
        r1        = _mm_add_ps(r1, _mm_mul_ps(_mm_shuffle_ps(a1, a1, _MM_SHUFFLE(1, 1, 1, 1)), b1));
        r1        = _mm_add_ps(r1, _mm_mul_ps(_mm_shuffle_ps(a1, a1, _MM_SHUFFLE(2, 2, 2, 2)), b2));
        r1        = _mm_add_ps(r1, _mm_mul_ps(_mm_shuffle_ps(a1, a1, _MM_SHUFFLE(3, 3, 3, 3)), b3));

        __m128 r2 = _mm_mul_ps(_mm_shuffle_ps(a2, a2, _MM_SHUFFLE(0, 0, 0, 0)), b0);
        r2        = _mm_add_ps(r2, _mm_mul_ps(_mm_shuffle_ps(a2, a2, _MM_SHUFFLE(1, 1, 1, 1)), b1));
        r2        = _mm_add_ps(r2, _mm_mul_ps(_mm_shuffle_ps(a2, a2, _MM_SHUFFLE(2, 2, 2, 2)), b2));
        r2        = _mm_add_ps(r2, _mm_mul_ps(_mm_shuffle_ps(a2, a2, _MM_SHUFFLE(3, 3, 3, 3)), b3));

        __m128 r3 = _mm_mul_ps(_mm_shuffle_ps(a3, a3, _MM_SHUFFLE(0, 0, 0, 0)), b0);
        r3        = _mm_add_ps(r3, _mm_mul_ps(_mm_shuffle_ps(a3, a3, _MM_SHUFFLE(1, 1, 1, 1)), b1));
        r3        = _mm_add_ps(r3, _mm_mul_ps(_mm_shuffle_ps(a3, a3, _MM_SHUFFLE(2, 2, 2, 2)), b2));
        r3        = _mm_add_ps(r3, _mm_mul_ps(_mm_shuffle_ps(a3, a3, _MM_SHUFFLE(3, 3, 3, 3)), b3));

        Mat4 result = Mat4(); // Explicitly call constructor to initialize
        _mm_storeu_ps(result.m, r0);
        _mm_storeu_ps(result.m + 4, r1);
        _mm_storeu_ps(result.m + 8, r2);
        _mm_storeu_ps(result.m + 12, r3);

        return result;
#elif defined(_RL_SIMD_ARM_NEON)
        Mat4        result;
        float32x4_t row0 = vld1q_f32(m);
        float32x4_t row1 = vld1q_f32(m + 4);
        float32x4_t row2 = vld1q_f32(m + 8);
        float32x4_t row3 = vld1q_f32(m + 12);

        float32x4_t col0 = vcombine_f32(vld1_f32(other.m), vld1_f32(other.m + 8));
        float32x4_t col1 = vcombine_f32(vld1_f32(other.m + 1), vld1_f32(other.m + 9));
        float32x4_t col2 = vcombine_f32(vld1_f32(other.m + 2), vld1_f32(other.m + 10));
        float32x4_t col3 = vcombine_f32(vld1_f32(other.m + 3), vld1_f32(other.m + 11));

        float32x4_t dot0 = vmulq_f32(row0, col0);
        float32x4_t dot1 = vmulq_f32(row0, col1);
        float32x4_t dot2 = vmulq_f32(row0, col2);
        float32x4_t dot3 = vmulq_f32(row0, col3);

        float32x2_t sum0 = vadd_f32(vget_low_f32(dot0), vget_high_f32(dot0));
        float32x2_t sum1 = vadd_f32(vget_low_f32(dot1), vget_high_f32(dot1));
        float32x2_t sum2 = vadd_f32(vget_low_f32(dot2), vget_high_f32(dot2));
        float32x2_t sum3 = vadd_f32(vget_low_f32(dot3), vget_high_f32(dot3));

        sum0 = vpadd_f32(sum0, sum0);
        sum1 = vpadd_f32(sum1, sum1);
        sum2 = vpadd_f32(sum2, sum2);
        sum3 = vpadd_f32(sum3, sum3);

        result.m[0] = vget_lane_f32(sum0, 0);
        result.m[1] = vget_lane_f32(sum1, 0);
        result.m[2] = vget_lane_f32(sum2, 0);
        result.m[3] = vget_lane_f32(sum3, 0);

        // Row 1
        dot0 = vmulq_f32(row1, col0);
        dot1 = vmulq_f32(row1, col1);
        dot2 = vmulq_f32(row1, col2);
        dot3 = vmulq_f32(row1, col3);

        sum0 = vadd_f32(vget_low_f32(dot0), vget_high_f32(dot0));
        sum1 = vadd_f32(vget_low_f32(dot1), vget_high_f32(dot1));
        sum2 = vadd_f32(vget_low_f32(dot2), vget_high_f32(dot2));
        sum3 = vadd_f32(vget_low_f32(dot3), vget_high_f32(dot3));

        sum0 = vpadd_f32(sum0, sum0);
        sum1 = vpadd_f32(sum1, sum1);
        sum2 = vpadd_f32(sum2, sum2);
        sum3 = vpadd_f32(sum3, sum3);

        result.m[4] = vget_lane_f32(sum0, 0);
        result.m[5] = vget_lane_f32(sum1, 0);
        result.m[6] = vget_lane_f32(sum2, 0);
        result.m[7] = vget_lane_f32(sum3, 0);

        // Row 2
        dot0 = vmulq_f32(row2, col0);
        dot1 = vmulq_f32(row2, col1);
        dot2 = vmulq_f32(row2, col2);
        dot3 = vmulq_f32(row2, col3);

        sum0 = vadd_f32(vget_low_f32(dot0), vget_high_f32(dot0));
        sum1 = vadd_f32(vget_low_f32(dot1), vget_high_f32(dot1));
        sum2 = vadd_f32(vget_low_f32(dot2), vget_high_f32(dot2));
        sum3 = vadd_f32(vget_low_f32(dot3), vget_high_f32(dot3));

        sum0 = vpadd_f32(sum0, sum0);
        sum1 = vpadd_f32(sum1, sum1);
        sum2 = vpadd_f32(sum2, sum2);
        sum3 = vpadd_f32(sum3, sum3);

        result.m[8]  = vget_lane_f32(sum0, 0);
        result.m[9]  = vget_lane_f32(sum1, 0);
        result.m[10] = vget_lane_f32(sum2, 0);
        result.m[11] = vget_lane_f32(sum3, 0);

        // Row 3
        dot0 = vmulq_f32(row3, col0);
        dot1 = vmulq_f32(row3, col1);
        dot2 = vmulq_f32(row3, col2);
        dot3 = vmulq_f32(row3, col3);

        sum0 = vadd_f32(vget_low_f32(dot0), vget_high_f32(dot0));
        sum1 = vadd_f32(vget_low_f32(dot1), vget_high_f32(dot1));
        sum2 = vadd_f32(vget_low_f32(dot2), vget_high_f32(dot2));
        sum3 = vadd_f32(vget_low_f32(dot3), vget_high_f32(dot3));

        sum0 = vpadd_f32(sum0, sum0);
        sum1 = vpadd_f32(sum1, sum1);
        sum2 = vpadd_f32(sum2, sum2);
        sum3 = vpadd_f32(sum3, sum3);

        result.m[12] = vget_lane_f32(sum0, 0);
        result.m[13] = vget_lane_f32(sum1, 0);
        result.m[14] = vget_lane_f32(sum2, 0);
        result.m[15] = vget_lane_f32(sum3, 0);

        return result;
#else
        Mat4 result = Mat4(); // Explicitly call constructor to initialize
        for (int row = 0; row < 4; ++row)
        {
                for (int col = 0; col < 4; ++col)
                {
                        result.m[row * 4 + col] = 0.0f;
                        for (int k = 0; k < 4; ++k)
                                result.m[row * 4 + col] += m[row * 4 + k] * other.m[k * 4 + col];
                }
        }
        return result;
#endif
}

Mat4 Mat4::operator*(float scalar) const
{
#ifdef _RL_SIMD_X86
        Mat4   result;
        __m128 s  = _mm_set1_ps(scalar);
        __m128 a0 = _mm_loadu_ps(m);
        __m128 a1 = _mm_loadu_ps(m + 4);
        __m128 a2 = _mm_loadu_ps(m + 8);
        __m128 a3 = _mm_loadu_ps(m + 12);
        _mm_storeu_ps(result.m, _mm_mul_ps(a0, s));
        _mm_storeu_ps(result.m + 4, _mm_mul_ps(a1, s));
        _mm_storeu_ps(result.m + 8, _mm_mul_ps(a2, s));
        _mm_storeu_ps(result.m + 12, _mm_mul_ps(a3, s));
        return result;
#elif defined(_RL_SIMD_ARM_NEON)
        Mat4        result;
        float32x4_t s  = vdupq_n_f32(scalar);
        float32x4_t a0 = vld1q_f32(m);
        float32x4_t a1 = vld1q_f32(m + 4);
        float32x4_t a2 = vld1q_f32(m + 8);
        float32x4_t a3 = vld1q_f32(m + 12);
        vst1q_f32(result.m, vmulq_f32(a0, s));
        vst1q_f32(result.m + 4, vmulq_f32(a1, s));
        vst1q_f32(result.m + 8, vmulq_f32(a2, s));
        vst1q_f32(result.m + 12, vmulq_f32(a3, s));
        return result;
#else
        Mat4 result = Mat4(); // Explicitly call constructor to initialize
        for (int i = 0; i < 16; ++i)
                result.m[i] = m[i] * scalar;
        return result;
#endif
}

Vec4 Mat4::operator*(const Vec4& vec) const
{
        // Column-major matrix multiplication
        return Vec4(m[0] * vec.x + m[4] * vec.y + m[8] * vec.z + m[12] * vec.w,
                    m[1] * vec.x + m[5] * vec.y + m[9] * vec.z + m[13] * vec.w,
                    m[2] * vec.x + m[6] * vec.y + m[10] * vec.z + m[14] * vec.w,
                    m[3] * vec.x + m[7] * vec.y + m[11] * vec.z + m[15] * vec.w);
}

Mat4 Mat4::transpose() const
{
#ifdef _RL_SIMD_X86
        Mat4   result;
        __m128 r0 = _mm_loadu_ps(m);
        __m128 r1 = _mm_loadu_ps(m + 4);
        __m128 r2 = _mm_loadu_ps(m + 8);
        __m128 r3 = _mm_loadu_ps(m + 12);

        _MM_TRANSPOSE4_PS(r0, r1, r2, r3);

        _mm_storeu_ps(result.m, r0);
        _mm_storeu_ps(result.m + 4, r1);
        _mm_storeu_ps(result.m + 8, r2);
        _mm_storeu_ps(result.m + 12, r3);
        return result;
#elif defined(_RL_SIMD_ARM_NEON)
        Mat4          result;
        float32x4x2_t rows01 = vzipq_f32(vld1q_f32(m), vld1q_f32(m + 4));
        float32x4x2_t rows23 = vzipq_f32(vld1q_f32(m + 8), vld1q_f32(m + 12));
        float32x4x2_t cols02 = vzipq_f32(rows01.val[0], rows23.val[0]);
        float32x4x2_t cols13 = vzipq_f32(rows01.val[1], rows23.val[1]);

        vst1q_f32(result.m, cols02.val[0]);
        vst1q_f32(result.m + 4, cols13.val[0]);
        vst1q_f32(result.m + 8, cols02.val[1]);
        vst1q_f32(result.m + 12, cols13.val[1]);
        return result;
#else
        return Mat4(m[0], m[4], m[8], m[12], m[1], m[5], m[9], m[13], m[2], m[6], m[10], m[14],
                    m[3], m[7], m[11], m[15]);
#endif
}

float Mat4::determinant() const
{
        float det = 0.0f;

        // Cofactor for element (0,0)
        det +=
            m[0] * (m[5] * (m[10] * m[15] - m[11] * m[14]) - m[6] * (m[9] * m[15] - m[11] * m[13]) +
                    m[7] * (m[9] * m[14] - m[10] * m[13]));

        // Cofactor for element (0,1)
        det -=
            m[1] * (m[4] * (m[10] * m[15] - m[11] * m[14]) - m[6] * (m[8] * m[15] - m[11] * m[12]) +
                    m[7] * (m[8] * m[14] - m[10] * m[12]));

        // Cofactor for element (0,2)
        det +=
            m[2] * (m[4] * (m[9] * m[15] - m[11] * m[13]) - m[5] * (m[8] * m[15] - m[11] * m[12]) +
                    m[7] * (m[8] * m[13] - m[9] * m[12]));

        // Cofactor for element (0,3)
        det -=
            m[3] * (m[4] * (m[9] * m[14] - m[10] * m[13]) - m[5] * (m[8] * m[14] - m[10] * m[12]) +
                    m[6] * (m[8] * m[13] - m[9] * m[12]));

        return det;
}

Mat4 Mat4::inverse() const
{
        float det = determinant();
        if (std::abs(det) < 1e-6f)
                return Mat4(); // Return identity if singular

        float invDet = 1.0f / det;
        Mat4  result;

        // Compute cofactors and transpose (adjugate matrix)
        result.m[0] =
            (m[5] * (m[10] * m[15] - m[11] * m[14]) - m[6] * (m[9] * m[15] - m[11] * m[13]) +
             m[7] * (m[9] * m[14] - m[10] * m[13])) *
            invDet;

        result.m[1] =
            (m[1] * (m[10] * m[15] - m[11] * m[14]) - m[2] * (m[9] * m[15] - m[11] * m[13]) +
             m[3] * (m[9] * m[14] - m[10] * m[13])) *
            -invDet;

        result.m[2] = (m[1] * (m[6] * m[15] - m[7] * m[14]) - m[2] * (m[5] * m[15] - m[7] * m[13]) +
                       m[3] * (m[5] * m[14] - m[6] * m[13])) *
                      invDet;

        result.m[3] = (m[1] * (m[6] * m[11] - m[7] * m[10]) - m[2] * (m[5] * m[11] - m[7] * m[9]) +
                       m[3] * (m[5] * m[10] - m[6] * m[9])) *
                      -invDet;

        result.m[4] =
            (m[4] * (m[10] * m[15] - m[11] * m[14]) - m[6] * (m[8] * m[15] - m[11] * m[12]) +
             m[7] * (m[8] * m[14] - m[10] * m[12])) *
            -invDet;

        result.m[5] =
            (m[0] * (m[10] * m[15] - m[11] * m[14]) - m[2] * (m[8] * m[15] - m[11] * m[12]) +
             m[3] * (m[8] * m[14] - m[10] * m[12])) *
            invDet;

        result.m[6] = (m[0] * (m[6] * m[15] - m[7] * m[14]) - m[2] * (m[4] * m[15] - m[7] * m[12]) +
                       m[3] * (m[4] * m[14] - m[6] * m[12])) *
                      -invDet;

        result.m[7] = (m[0] * (m[6] * m[11] - m[7] * m[10]) - m[2] * (m[4] * m[11] - m[7] * m[9]) +
                       m[3] * (m[4] * m[10] - m[6] * m[9])) *
                      invDet;

        result.m[8] =
            (m[4] * (m[9] * m[15] - m[11] * m[13]) - m[5] * (m[8] * m[15] - m[11] * m[12]) +
             m[7] * (m[8] * m[13] - m[9] * m[12])) *
            invDet;

        result.m[9] =
            (m[0] * (m[9] * m[15] - m[11] * m[13]) - m[1] * (m[8] * m[15] - m[11] * m[12]) +
             m[3] * (m[8] * m[13] - m[9] * m[12])) *
            -invDet;

        result.m[10] =
            (m[0] * (m[5] * m[15] - m[7] * m[13]) - m[1] * (m[4] * m[15] - m[7] * m[12]) +
             m[3] * (m[4] * m[13] - m[5] * m[12])) *
            invDet;

        result.m[11] = (m[0] * (m[5] * m[11] - m[7] * m[9]) - m[1] * (m[4] * m[11] - m[7] * m[8]) +
                        m[3] * (m[4] * m[9] - m[5] * m[8])) *
                       -invDet;

        result.m[12] =
            (m[4] * (m[9] * m[14] - m[10] * m[13]) - m[5] * (m[8] * m[14] - m[10] * m[12]) +
             m[6] * (m[8] * m[13] - m[9] * m[12])) *
            -invDet;

        result.m[13] =
            (m[0] * (m[9] * m[14] - m[10] * m[13]) - m[1] * (m[8] * m[14] - m[10] * m[12]) +
             m[2] * (m[8] * m[13] - m[9] * m[12])) *
            invDet;

        result.m[14] =
            (m[0] * (m[5] * m[14] - m[6] * m[13]) - m[1] * (m[4] * m[14] - m[6] * m[12]) +
             m[2] * (m[4] * m[13] - m[5] * m[12])) *
            -invDet;

        result.m[15] = (m[0] * (m[5] * m[10] - m[6] * m[9]) - m[1] * (m[4] * m[10] - m[6] * m[8]) +
                        m[2] * (m[4] * m[9] - m[5] * m[8])) *
                       invDet;

        return result;
}

Mat4 Mat4::identity()
{
        return Mat4();
}

Mat4 Mat4::translation(float x, float y, float z)
{
        Mat4 result  = identity();
        result.m[12] = x;
        result.m[13] = y;
        result.m[14] = z;
        return result;
}

Mat4 Mat4::rotationX(float angle)
{
        float c      = std::cos(angle);
        float s      = std::sin(angle);
        Mat4  result = identity();
        result.m[5]  = c;
        result.m[9]  = s;
        result.m[6]  = -s;
        result.m[10] = c;
        return result;
}

Mat4 Mat4::rotationY(float angle)
{
        float c      = std::cos(angle);
        float s      = std::sin(angle);
        Mat4  result = identity();
        result.m[0]  = c;
        result.m[8]  = s;
        result.m[2]  = -s;
        result.m[10] = c;
        return result;
}

Mat4 Mat4::rotationZ(float angle)
{
        float c      = std::cos(angle);
        float s      = std::sin(angle);
        Mat4  result = identity();
        result.m[0]  = c;
        result.m[4]  = s;
        result.m[1]  = -s;
        result.m[5]  = c;
        return result;
}

Mat4 Mat4::scale(float x, float y, float z)
{
        Mat4 result  = identity();
        result.m[0]  = x;
        result.m[5]  = y;
        result.m[10] = z;
        return result;
}

Mat4 Mat4::perspective(float fov, float aspect, float near, float far)
{
        float tanHalfFov = std::tan(fov / 2.0f);
        Mat4  result     = Mat4();
        result.m[0]      = 1.0f / (aspect * tanHalfFov);
        result.m[5]      = 1.0f / tanHalfFov;
        result.m[10]     = far / (near - far);
        result.m[11]     = -1.0f;
        result.m[14]     = -(far * near) / (far - near);
        result.m[15]     = 0.0f;
        result.m[1] = result.m[2] = result.m[3] = 0.0f;
        result.m[4] = result.m[6] = result.m[7] = 0.0f;
        result.m[8] = result.m[9] = result.m[12] = result.m[13] = 0.0f;
        return result;
}

Mat4 Mat4::infinitePerspective(float fov, float aspect, float near)
{
        float tanHalfFov = std::tan(fov / 2.0f);
        Mat4  result     = Mat4();
        result.m[0]      = 1.0f / (aspect * tanHalfFov);
        result.m[5]      = 1.0f / tanHalfFov;
        result.m[10]     = -1.0f;
        result.m[11]     = -1.0f;
        result.m[14]     = -2.0f * near;
        result.m[15]     = 0.0f;
        result.m[1] = result.m[2] = result.m[3] = 0.0f;
        result.m[4] = result.m[6] = result.m[7] = 0.0f;
        result.m[8] = result.m[9] = result.m[12] = result.m[13] = 0.0f;
        return result;
}

Mat4 Mat4::lookAt(const Vec3& eye, const Vec3& center, const Vec3& up)
{
        Vec3 f = (center - eye).normalized();
        Vec3 s = up.cross(f).normalized();
        Vec3 u = f.cross(s);

        Mat4 result  = Mat4::identity();
        result.m[0]  = s.x;
        result.m[4]  = s.y;
        result.m[8]  = s.z;
        result.m[12] = -s.dot(eye);

        result.m[1]  = u.x;
        result.m[5]  = u.y;
        result.m[9]  = u.z;
        result.m[13] = -u.dot(eye);

        result.m[2]  = -f.x;
        result.m[6]  = -f.y;
        result.m[10] = -f.z;
        result.m[14] = f.dot(eye);

        result.m[3]  = 0.0f;
        result.m[7]  = 0.0f;
        result.m[11] = 0.0f;
        result.m[15] = 1.0f;

        return result;
}

Quaternion Quaternion::fromAxisAngle(const Vec3& axis, float angle)
{
        Vec3  normalizedAxis = axis.normalized();
        float halfAngle      = angle * 0.5f;
        float sinHalf        = std::sin(halfAngle);
        return Quaternion(normalizedAxis.x * sinHalf, normalizedAxis.y * sinHalf,
                          normalizedAxis.z * sinHalf, std::cos(halfAngle));
}

Quaternion Quaternion::fromEuler(float pitch, float yaw, float roll)
{
        float cy = std::cos(yaw * 0.5f);
        float sy = std::sin(yaw * 0.5f);
        float cp = std::cos(pitch * 0.5f);
        float sp = std::sin(pitch * 0.5f);
        float cr = std::cos(roll * 0.5f);
        float sr = std::sin(roll * 0.5f);

        return Quaternion(sr * cp * cy - cr * sp * sy, cr * sp * cy + sr * cp * sy,
                          cr * cp * sy - sr * sp * cy, cr * cp * cy + sr * sp * sy);
}

Quaternion Quaternion::fromMat3(const Mat3& mat)
{
        float      trace = mat.m[0] + mat.m[4] + mat.m[8];
        Quaternion q;

        if (trace > 0.0f)
        {
                float s = std::sqrt(trace + 1.0f) * 2.0f;
                q.w     = 0.25f * s;
                q.x     = (mat.m[7] - mat.m[5]) / s;
                q.y     = (mat.m[2] - mat.m[6]) / s;
                q.z     = (mat.m[3] - mat.m[1]) / s;
        }
        else if (mat.m[0] > mat.m[4] && mat.m[0] > mat.m[8])
        {
                float s = std::sqrt(1.0f + mat.m[0] - mat.m[4] - mat.m[8]) * 2.0f;
                q.w     = (mat.m[7] - mat.m[5]) / s;
                q.x     = 0.25f * s;
                q.y     = (mat.m[1] + mat.m[3]) / s;
                q.z     = (mat.m[2] + mat.m[6]) / s;
        }
        else if (mat.m[4] > mat.m[8])
        {
                float s = std::sqrt(1.0f + mat.m[4] - mat.m[0] - mat.m[8]) * 2.0f;
                q.w     = (mat.m[2] - mat.m[6]) / s;
                q.x     = (mat.m[1] + mat.m[3]) / s;
                q.y     = 0.25f * s;
                q.z     = (mat.m[5] + mat.m[7]) / s;
        }
        else
        {
                float s = std::sqrt(1.0f + mat.m[8] - mat.m[0] - mat.m[4]) * 2.0f;
                q.w     = (mat.m[3] - mat.m[1]) / s;
                q.x     = (mat.m[2] + mat.m[6]) / s;
                q.y     = (mat.m[5] + mat.m[7]) / s;
                q.z     = 0.25f * s;
        }

        return q.normalized();
}

Quaternion Quaternion::fromMat4(const Mat4& mat)
{
        return fromMat3(Mat3(mat.m[0], mat.m[1], mat.m[2], mat.m[4], mat.m[5], mat.m[6], mat.m[8],
                             mat.m[9], mat.m[10]));
}

Quaternion Quaternion::operator+() const
{
        return *this;
}

Quaternion Quaternion::operator-() const
{
        return Quaternion(-x, -y, -z, -w);
}

Quaternion Quaternion::operator+(const Quaternion& other) const
{
#ifdef _RL_SIMD_X86
        __m128 a      = _mm_set_ps(w, z, y, x);
        __m128 b      = _mm_set_ps(other.w, other.z, other.y, other.x);
        __m128 result = _mm_add_ps(a, b);
        float  res[4];
        _mm_storeu_ps(res, result);
        return Quaternion(res[0], res[1], res[2], res[3]);
#elif defined(_RL_SIMD_ARM_NEON)
        float32x4_t a      = vld1q_f32(&x);
        float32x4_t b      = vld1q_f32(&other.x);
        float32x4_t result = vaddq_f32(a, b);
        Quaternion  res;
        vst1q_f32(&res.x, result);
        return res;
#else
        return Quaternion(x + other.x, y + other.y, z + other.z, w + other.w);
#endif
}

Quaternion Quaternion::operator-(const Quaternion& other) const
{
#ifdef _RL_SIMD_X86
        __m128 a      = _mm_set_ps(w, z, y, x);
        __m128 b      = _mm_set_ps(other.w, other.z, other.y, other.x);
        __m128 result = _mm_sub_ps(a, b);
        float  res[4];
        _mm_storeu_ps(res, result);
        return Quaternion(res[0], res[1], res[2], res[3]);
#elif defined(_RL_SIMD_ARM_NEON)
        float32x4_t a      = vld1q_f32(&x);
        float32x4_t b      = vld1q_f32(&other.x);
        float32x4_t result = vsubq_f32(a, b);
        Quaternion  res;
        vst1q_f32(&res.x, result);
        return res;
#else
        return Quaternion(x - other.x, y - other.y, z - other.z, w - other.w);
#endif
}

Quaternion Quaternion::operator*(const Quaternion& other) const
{
        return Quaternion(w * other.x + x * other.w + y * other.z - z * other.y,
                          w * other.y - x * other.z + y * other.w + z * other.x,
                          w * other.z + x * other.y - y * other.x + z * other.w,
                          w * other.w - x * other.x - y * other.y - z * other.z);
}

Quaternion Quaternion::operator*(float scalar) const
{
#ifdef _RL_SIMD_X86
        __m128 a      = _mm_set_ps(w, z, y, x);
        __m128 s      = _mm_set1_ps(scalar);
        __m128 result = _mm_mul_ps(a, s);
        float  res[4];
        _mm_storeu_ps(res, result);
        return Quaternion(res[0], res[1], res[2], res[3]);
#elif defined(_RL_SIMD_ARM_NEON)
        float32x4_t a      = vld1q_f32(&x);
        float32x4_t s      = vdupq_n_f32(scalar);
        float32x4_t result = vmulq_f32(a, s);
        Quaternion  res;
        vst1q_f32(&res.x, result);
        return res;
#else
        return Quaternion(x * scalar, y * scalar, z * scalar, w * scalar);
#endif
}

Quaternion Quaternion::operator/(float scalar) const
{
        return Quaternion(x / scalar, y / scalar, z / scalar, w / scalar);
}

Quaternion& Quaternion::operator+=(const Quaternion& other)
{
        x += other.x;
        y += other.y;
        z += other.z;
        w += other.w;
        return *this;
}

Quaternion& Quaternion::operator-=(const Quaternion& other)
{
        x -= other.x;
        y -= other.y;
        z -= other.z;
        w -= other.w;
        return *this;
}

Quaternion& Quaternion::operator*=(const Quaternion& other)
{
        *this = *this * other;
        return *this;
}

Quaternion& Quaternion::operator*=(float scalar)
{
        x *= scalar;
        y *= scalar;
        z *= scalar;
        w *= scalar;
        return *this;
}

Quaternion& Quaternion::operator/=(float scalar)
{
        x /= scalar;
        y /= scalar;
        z /= scalar;
        w /= scalar;
        return *this;
}

Quaternion Quaternion::conjugate() const
{
        return Quaternion(-x, -y, -z, w);
}

Quaternion Quaternion::inverse() const
{
        float lenSq = lengthSquared();
        if (lenSq > 0.0f)
        {
                return conjugate() / lenSq;
        }
        return Quaternion();
}

float Quaternion::dot(const Quaternion& other) const
{
#ifdef _RL_SIMD_X86
        __m128 a   = _mm_set_ps(w, z, y, x);
        __m128 b   = _mm_set_ps(other.w, other.z, other.y, other.x);
        __m128 mul = _mm_mul_ps(a, b);
        __m128 sum = _mm_hadd_ps(mul, mul);
        sum        = _mm_hadd_ps(sum, sum);
        float res[4];
        _mm_storeu_ps(res, sum);
        return res[0];
#elif defined(_RL_SIMD_ARM_NEON)
        float32x4_t a   = vld1q_f32(&x);
        float32x4_t b   = vld1q_f32(&other.x);
        float32x4_t mul = vmulq_f32(a, b);
        float32x2_t sum = vadd_f32(vget_low_f32(mul), vget_high_f32(mul));
        sum             = vpadd_f32(sum, sum);
        return vget_lane_f32(sum, 0);
#else
        return x * other.x + y * other.y + z * other.z + w * other.w;
#endif
}

float Quaternion::length() const
{
        return std::sqrt(lengthSquared());
}

float Quaternion::lengthSquared() const
{
        return x * x + y * y + z * z + w * w;
}

Quaternion Quaternion::normalized() const
{
        float len = length();
        if (len > 0.0f)
        {
                return Quaternion(x / len, y / len, z / len, w / len);
        }
        return Quaternion();
}

Quaternion& Quaternion::normalize()
{
        float len = length();
        if (len > 0.0f)
        {
                x /= len;
                y /= len;
                z /= len;
                w /= len;
        }
        return *this;
}

Vec3 Quaternion::rotate(const Vec3& vec) const
{
        Quaternion v(vec.x, vec.y, vec.z, 0.0f);
        Quaternion result = *this * v * conjugate();
        return Vec3(result.x, result.y, result.z);
}

Mat3 Quaternion::toMat3() const
{
        float xx = x * x;
        float yy = y * y;
        float zz = z * z;
        float xy = x * y;
        float xz = x * z;
        float yz = y * z;
        float wx = w * x;
        float wy = w * y;
        float wz = w * z;

        return Mat3(1.0f - 2.0f * (yy + zz), 2.0f * (xy - wz), 2.0f * (xz + wy), 2.0f * (xy + wz),
                    1.0f - 2.0f * (xx + zz), 2.0f * (yz - wx), 2.0f * (xz - wy), 2.0f * (yz + wx),
                    1.0f - 2.0f * (xx + yy));
}

Mat4 Quaternion::toMat4() const
{
        Mat3 rot     = toMat3();
        Mat4 result  = Mat4::identity();
        result.m[0]  = rot.m[0];
        result.m[1]  = rot.m[1];
        result.m[2]  = rot.m[2];
        result.m[4]  = rot.m[3];
        result.m[5]  = rot.m[4];
        result.m[6]  = rot.m[5];
        result.m[8]  = rot.m[6];
        result.m[9]  = rot.m[7];
        result.m[10] = rot.m[8];
        return result;
}

Vec3 Quaternion::toEuler() const
{
        Vec3 euler;

        float sinrCosp = 2.0f * (w * x + y * z);
        float cosrCosp = 1.0f - 2.0f * (x * x + y * y);
        euler.x        = std::atan2(sinrCosp, cosrCosp);

        float sinp = 2.0f * (w * y - z * x);
        if (std::abs(sinp) >= 1.0f)
        {
                euler.y = std::copysign(3.14159265358979323846f / 2.0f, sinp);
        }
        else
        {
                euler.y = std::asin(sinp);
        }

        float sinyCosp = 2.0f * (w * z + x * y);
        float cosyCosp = 1.0f - 2.0f * (y * y + z * z);
        euler.z        = std::atan2(sinyCosp, cosyCosp);

        return euler;
}

Quaternion Quaternion::identity()
{
        return Quaternion();
}

Quaternion Quaternion::slerp(const Quaternion& a, const Quaternion& b, float t)
{
        float dot = a.dot(b);

        Quaternion bTemp;
        if (dot < 0.0f)
        {
                bTemp = -b;
                dot   = -dot;
        }
        else
        {
                bTemp = b;
        }

        if (dot > 0.9995f)
        {
                return nlerp(a, bTemp, t);
        }
        float theta0    = std::acos(dot);
        float theta     = theta0 * t;
        float sinTheta  = std::sin(theta);
        float sinTheta0 = std::sin(theta0);
        float s0        = std::cos(theta) - dot * sinTheta / sinTheta0;
        float s1        = sinTheta / sinTheta0;

        return a * s0 + bTemp * s1;
}

Quaternion Quaternion::nlerp(const Quaternion& a, const Quaternion& b, float t)
{
        return (a + (b - a) * t).normalized();
}

} // namespace rl
