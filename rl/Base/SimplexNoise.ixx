export module Rl.Base.SimplexNoise;

import <array>;
import <cstdint>;

namespace Rl::Providers
{

export class AbstractOpenSimplexNoise
{
  public:
  virtual ~AbstractOpenSimplexNoise() = default;
  [[nodiscard]]
  virtual float eval(float x, float y) const = 0;
  [[nodiscard]]
  virtual float eval(float x, float y, float z) const = 0;
};

export class OpenSimplexNoiseGen : public AbstractOpenSimplexNoise
{
  public:
  OpenSimplexNoiseGen();
  OpenSimplexNoiseGen(int64_t seed);
  /* Resets the noise generator with a new seed */
  void ResetSeed(int64_t seed);
  [[nodiscard]]
  float eval(float x, float y) const override;
  [[nodiscard]]
  float eval(float x, float y, float z) const override;

  private:
  const double           stretch2d;
  const double           squish2d;
  const double           stretch3d;
  const double           squish3d;
  const double           norm2d;
  const double           norm3d;
  const long long        defaultSeed;
  std::array<short, 256> perm;
  std::array<short, 256> permGradIndex3d;
  std::array<char, 16>   gradients2d;
  std::array<char, 72>   gradients3d;
  [[nodiscard]]
  double extrapolate(int xsb, int ysb, double dx, double dy) const;
  [[nodiscard]]
  double extrapolate(int xsb, int ysb, int zsb, double dx, double dy, double dz) const;
};

} // namespace Rl::Providers
