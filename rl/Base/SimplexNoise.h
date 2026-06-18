#pragma once

#include <array>
#include <cstdint>

namespace Rl::Providers {

class AbstractOpenSimplexNoise
{
public:
    virtual ~AbstractOpenSimplexNoise() = default;
    virtual float eval(float x, float y) const = 0;
    virtual float eval(float x, float y, float z) const = 0;
    virtual float eval(float x, float y, float z, float w) const = 0;
};

class OpenSimplexNoiseGen :
    public AbstractOpenSimplexNoise
{
public:
    OpenSimplexNoiseGen();
    OpenSimplexNoiseGen(int64_t seed);
    [[nodiscard]]
    float eval(float x, float y) const override;
    [[nodiscard]]
    float eval(float x, float y, float z) const override;
    [[nodiscard]]
    float eval(float x, float y, float z, float w ) const override;
private:
    const double m_stretch2d;
    const double m_squish2d;
    const double m_stretch3d;
    const double m_squish3d;
    const double m_stretch4d;
    const double m_squish4d;
    const double m_norm2d;
    const double m_norm3d;
    const double m_norm4d;
    const long m_defaultSeed;
    std::array<short, 256> m_perm;
    std::array<short, 256> m_permGradIndex3d;
    std::array<char, 16> m_gradients2d;
    std::array<char, 72> m_gradients3d;
    std::array<char, 256> m_gradients4d;
    double extrapolate(int xsb, int ysb, double dx, double dy) const;
    double extrapolate(int xsb, int ysb, int zsb, double dx, double dy, double dz) const;
    double extrapolate(int xsb, int ysb, int zsb, int wsb, double dx, double dy, double dz, double dw) const;
};

} // namespace Rl::Providers