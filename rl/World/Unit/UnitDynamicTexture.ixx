export module Rl.World.Unit.UnitDynamicTexture;

import Rl.Base.SimplexNoise;
import Rl.Base.Texture2;
import <cstddef>;
import <cstdint>;
import <map>;
import <memory>;
import <optional>;
import <vector>;

namespace Rl::World
{

export class AbstractUnitDynamicTexture
{
  public:
  virtual ~AbstractUnitDynamicTexture() = default;

  /* Options for the texture */
  struct DynamicOptions
  {
    /* Stores the noise scale */
    float noiseSc = 0.2f;
    /* Stores the color variation */
    float colorVar = 0.05f;
    /* Stores the palette blend ratio (0.0 = no blending, 1.0 = full blend) */
    float paletteBlend = 0.3f;
  };
  /* Describes a 64-bit seed value for the randomize texture process */
  using Seed = long long;

  /* Generates a randomized texture from the base texture */
  virtual Providers::Texture2* GenDynamicTexture(Seed seed) = 0;
};

export class UnitDynamicTexture : public AbstractUnitDynamicTexture
{
  /* Stores the color frequency map */
  std::map<uint32_t, int> clFreqMap;

  public:
  /* Options for randomizing the texture adding noise */
  DynamicOptions& options;

  /* The Simplex noise generator */
  Providers::OpenSimplexNoiseGen noiseGen;

  /* The base texture that will help to generate the random texture */
  Providers::Texture2* baseTexture;

  /* The seed for randomness */
  Seed seed;

  /* Constructs a basic dynamic texture for Unit with a Seed */
  UnitDynamicTexture(Providers::Texture2& base, Seed seed, DynamicOptions& options);
  /* Generates a noise map for the random texture
   * For example, a texture of 32x32 will generate
   * Coherent to the texture noise of a 32x32 array
   */
  [[nodiscard]]
  std::vector<float> GenNoiseValMap(float scale) const;
  /* Gets the color frequency map of the base texture
   * It divides the texture into blocks of 4x4 pixels
   * and counts the frequency of each color in each block
   */
  [[nodiscard]]
  std::vector<int> GetTargetColorMap() const;
  /* Processes the color frequency map and returns the most frequent color */
  static void ProcessColorFreqMap(
      const std::map<uint32_t, int>& colorFreq, std::vector<int>& outMap);
  /* Generates the dynamic texture from the base texture */
  [[nodiscard]]
  Providers::Texture2* GenDynamicTexture(Seed seed) override;
  /* Generates the dynamic texture using the seed from constructor */
  [[nodiscard]]
  Providers::Texture2* GenDynamicTexture();

  protected:
  /* Creates a texture for the new data of the new generated texture bytes */
  [[nodiscard]]
  Providers::Texture2* GenTexture(uint8_t* data, size_t width, size_t height, size_t channels);
  /* Clamps the noise variation to a range of 0-255 */
  [[nodiscard]]
  uint8_t ClampNoiseVar(int v, float variation) const;

  /* Applies the Noise variation to RGB color */
  void ApplyNoiseVar(
      uint8_t& r, uint8_t& g, uint8_t& b, int index, const std::vector<float>& noiseMap) const;
  /* Blends the base color with the most common palette color */
  void BlendsBaseColorWithPalette(
      uint8_t& r, uint8_t& g, uint8_t& b, int index, const std::vector<int>& colorMap) const;
};

} // namespace Rl::World
