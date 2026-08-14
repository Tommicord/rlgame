#ifndef RL_BASE_GAME_MATRIX_H
#define RL_BASE_GAME_MATRIX_H

#include <cmath>
#include <cstdint>

#ifdef _RL_SIMD_X86
#include <immintrin.h>
#elif defined(_RL_SIMD_ARM_NEON)
#include <arm_neon.h>
#endif

namespace rl
{

class Vec2;
class Vec3;
class Vec4;
class Mat2;
class Mat3;
class Mat4;
class Quaternion;

class Vec2
{
  public:
    float x, y;

    Vec2() : x(0.0f), y(0.0f)
    {
    }
    Vec2(float x, float y) : x(x), y(y)
    {
    }
    Vec2(const Vec3& other);
    Vec2(const Vec4& other);

    Vec2  operator+(const Vec2& other) const;
    Vec2  operator-(const Vec2& other) const;
    Vec2  operator*(float scalar) const;
    Vec2  operator/(float scalar) const;
    Vec2& operator+=(const Vec2& other);
    Vec2& operator-=(const Vec2& other);
    Vec2& operator*=(float scalar);
    Vec2& operator/=(float scalar);

    float dot(const Vec2& other) const;
    float length() const;
    float lengthSquared() const;
    Vec2  normalized() const;
    Vec2& normalize();
};

class Vec3
{
  public:
    float x, y, z;

    Vec3() : x(0.0f), y(0.0f), z(0.0f)
    {
    }
    Vec3(float x, float y, float z) : x(x), y(y), z(z)
    {
    }
    Vec3(const Vec2& other);
    Vec3(const Vec4& other);

    Vec3  operator+(const Vec3& other) const;
    Vec3  operator-(const Vec3& other) const;
    Vec3  operator*(float scalar) const;
    Vec3  operator/(float scalar) const;
    Vec3& operator+=(const Vec3& other);
    Vec3& operator-=(const Vec3& other);
    Vec3& operator*=(float scalar);
    Vec3& operator/=(float scalar);

    float dot(const Vec3& other) const;
    Vec3  cross(const Vec3& other) const;
    float length() const;
    float lengthSquared() const;
    Vec3  normalized() const;
    Vec3& normalize();
};

class Vec4
{
  public:
    float x, y, z, w;

    Vec4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f)
    {
    }
    Vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w)
    {
    }
    Vec4(const Vec2& other);
    Vec4(const Vec3& other);

    Vec4  operator+(const Vec4& other) const;
    Vec4  operator-(const Vec4& other) const;
    Vec4  operator*(float scalar) const;
    Vec4  operator/(float scalar) const;
    Vec4& operator+=(const Vec4& other);
    Vec4& operator-=(const Vec4& other);
    Vec4& operator*=(float scalar);
    Vec4& operator/=(float scalar);

    float dot(const Vec4& other) const;
    float length() const;
    float lengthSquared() const;
    Vec4  normalized() const;
    Vec4& normalize();
};

class Mat2
{
  public:
    float m[4]; // Row-major: m00, m01, m10, m11

    Mat2();
    Mat2(float m00, float m01, float m10, float m11);

    float&       operator()(int row, int col);
    const float& operator()(int row, int col) const;

    Mat2 operator+(const Mat2& other) const;
    Mat2 operator-(const Mat2& other) const;
    Mat2 operator*(const Mat2& other) const;
    Mat2 operator*(float scalar) const;
    Vec2 operator*(const Vec2& vec) const;

    Mat2        transpose() const;
    float       determinant() const;
    Mat2        inverse() const;
    static Mat2 identity();
};

class Mat3
{
  public:
    float m[9]; // Row-major: m00, m01, m02, m10, m11, m12, m20, m21, m22

    Mat3();
    Mat3(float m00,
         float m01,
         float m02,
         float m10,
         float m11,
         float m12,
         float m20,
         float m21,
         float m22);

    float&       operator()(int row, int col);
    const float& operator()(int row, int col) const;

    Mat3 operator+(const Mat3& other) const;
    Mat3 operator-(const Mat3& other) const;
    Mat3 operator*(const Mat3& other) const;
    Mat3 operator*(float scalar) const;
    Vec3 operator*(const Vec3& vec) const;

    Mat3        transpose() const;
    float       determinant() const;
    Mat3        inverse() const;
    static Mat3 identity();
};

class Mat4
{
  public:
    float m[16]; // Row-major: m00, m01, m02, m03, m10, m11, m12, m13, m20,
                 // m21, m22, m23, m30, m31, m32, m33

    Mat4();
    Mat4(const Mat4& other);
    Mat4(Mat4&& other) noexcept;
    Mat4& operator=(const Mat4& other);
    Mat4& operator=(Mat4&& other) noexcept;
    Mat4(float m00,
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
         float m33);

    float&       operator()(int row, int col);
    const float& operator()(int row, int col) const;

    Mat4 operator+(const Mat4& other) const;
    Mat4 operator-(const Mat4& other) const;
    Mat4 operator*(const Mat4& other) const;
    Mat4 operator*(float scalar) const;
    Vec4 operator*(const Vec4& vec) const;

    Mat4        transpose() const;
    float       determinant() const;
    Mat4        inverse() const;
    static Mat4 identity();
    static Mat4 translation(float x, float y, float z);
    static Mat4 rotationX(float angle);
    static Mat4 rotationY(float angle);
    static Mat4 rotationZ(float angle);
    static Mat4 scale(float x, float y, float z);
    static Mat4 perspective(float fov, float aspect, float near, float far);
    static Mat4 infinitePerspective(float fov, float aspect, float near);
    static Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up);
};

class Quaternion
{
  public:
    float x, y, z, w;

    Quaternion() : x(0.0f), y(0.0f), z(0.0f), w(1.0f)
    {
    }
    Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w)
    {
    }

    static Quaternion fromAxisAngle(const Vec3& axis, float angle);
    static Quaternion fromEuler(float pitch, float yaw, float roll);
    static Quaternion fromMat3(const Mat3& mat);
    static Quaternion fromMat4(const Mat4& mat);

    Quaternion  operator+() const;
    Quaternion  operator-() const;
    Quaternion  operator+(const Quaternion& other) const;
    Quaternion  operator-(const Quaternion& other) const;
    Quaternion  operator*(const Quaternion& other) const;
    Quaternion  operator*(float scalar) const;
    Quaternion  operator/(float scalar) const;
    Quaternion& operator+=(const Quaternion& other);
    Quaternion& operator-=(const Quaternion& other);
    Quaternion& operator*=(const Quaternion& other);
    Quaternion& operator*=(float scalar);
    Quaternion& operator/=(float scalar);

    Quaternion  conjugate() const;
    Quaternion  inverse() const;
    float       dot(const Quaternion& other) const;
    float       length() const;
    float       lengthSquared() const;
    Quaternion  normalized() const;
    Quaternion& normalize();

    Vec3 rotate(const Vec3& vec) const;
    Mat3 toMat3() const;
    Mat4 toMat4() const;
    Vec3 toEuler() const;

    static Quaternion identity();
    static Quaternion slerp(const Quaternion& a, const Quaternion& b, float t);
    static Quaternion nlerp(const Quaternion& a, const Quaternion& b, float t);
};

} // namespace rl

#endif // RL_BASE_GAME_MATRIX_H
