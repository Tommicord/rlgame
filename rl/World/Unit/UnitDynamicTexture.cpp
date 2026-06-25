#include "rl/World/Unit/UnitDynamicTexture.h"
#include "rl/Base/SimplexNoise.h"
#include "rl/Base/Texture2.h"

#include <algorithm>
#include <map>
#include <vector>

#ifdef X86_
#include <immintrin.h>
#endif

#ifdef ARM_
#include <arm_neon.h>
#endif

namespace Rl::World
{

using namespace Rl::Providers;

UnitDynamicTexture::UnitDynamicTexture(Texture2& base, const Seed seed, DynamicOptions& options) :
    options(options), noiseGen{seed}, baseTexture(&base), seed(seed)
{
}

std::vector<float> UnitDynamicTexture::GenNoiseValMap(const float scale) const
{
  std::vector<float> map;
  int                width  = baseTexture->GetWidth();
  int                height = baseTexture->GetHeight();
  map.resize(width * height);

#ifdef X86_
  // SSE/AVX optimized version for x86/x86-64
  __m128    scaleVec    = _mm_set1_ps(scale);
  int       i           = 0;
  const int simdWidth   = 4;
  const int totalPixels = width * height;

  // Process 4 pixels at a time
  for (; i + simdWidth <= totalPixels; i += simdWidth)
  {
    // Calculate x and y coordinates for 4 pixels
    __m128 indices  = _mm_set_ps(i + 3, i + 2, i + 1, i);
    __m128 widthVec = _mm_set1_ps(static_cast<float>(width));
    __m128 yVec     = _mm_floor_ps(_mm_div_ps(indices, widthVec));
    __m128 xVec     = _mm_sub_ps(indices, _mm_mul_ps(yVec, widthVec));

    // Calculate noise coordinates
    __m128 randX = _mm_mul_ps(xVec, scaleVec);
    __m128 randY = _mm_mul_ps(yVec, scaleVec);

    // Generate noise for each coordinate (scalar fallback for noiseGen.eval)
    float xVals[4], yVals[4];
    _mm_storeu_ps(xVals, randX);
    _mm_storeu_ps(yVals, randY);

    for (int j = 0; j < simdWidth; ++j)
    {
      map[i + j] = noiseGen.eval(xVals[j], yVals[j]);
    }
  }

  // Process remaining pixels
  for (; i < totalPixels; ++i)
  {
    int   y     = i / width;
    int   x     = i % width;
    float randX = static_cast<float>(x) * scale;
    float randY = static_cast<float>(y) * scale;
    map[i]      = noiseGen.eval(randX, randY);
  }
#elif defined(ARM_)
  // ARM NEON optimized version
  float32x4_t scaleVec    = vdupq_n_f32(scale);
  int         i           = 0;
  const int   simdWidth   = 4;
  const int   totalPixels = width * height;

  // Process 4 pixels at a time
  for (; i + simdWidth <= totalPixels; i += simdWidth)
  {
    // Calculate x and y coordinates for 4 pixels
    // Don't touch this unreadable code or everything will fail!
    float32x4_t indices  = vsetq_lane_f32(i + 3,
         vsetq_lane_f32(i + 2, vsetq_lane_f32(i + 1, vsetq_lane_f32(i, vdupq_n_f32(0), 0), 1), 2),
         3);
    float32x4_t widthVec = vdupq_n_f32(static_cast<float>(width));
    float32x4_t yVec     = vrndmq_f32(vdivq_f32(indices, widthVec));
    float32x4_t xVec     = vsubq_f32(indices, vmulq_f32(yVec, widthVec));

    // Calculate noise coordinates
    float32x4_t randX = vmulq_f32(xVec, scaleVec);
    float32x4_t randY = vmulq_f32(yVec, scaleVec);

    // Generate noise for each coordinate (scalar fallback for noiseGen.eval)
    float xVals[4], yVals[4];
    vst1q_f32(xVals, randX);
    vst1q_f32(yVals, randY);

    for (int j = 0; j < simdWidth; ++j)
    {
      map[i + j] = noiseGen.eval(xVals[j], yVals[j]);
    }
  }

  // Process remaining pixels
  for (; i < totalPixels; ++i)
  {
    int   y     = i / width;
    int   x     = i % width;
    float randX = static_cast<float>(x) * scale;
    float randY = static_cast<float>(y) * scale;
    map[i]      = noiseGen.eval(randX, randY);
  }
#else
  // Scalar fallback for other architectures
  for (int y = 0; y < height; ++y)
  {
    for (int x = 0; x < width; ++x)
    {
      const size_t index = y * width + x;
      float        randX = static_cast<float>(x) * scale;
      float        randY = static_cast<float>(y) * scale;
      float        noise = noiseGen.eval(randX, randY);
      map[index]         = noise;
    }
  }
#endif

  return map;
}

std::vector<int> UnitDynamicTexture::GetTargetColorMap() const
{
  std::vector<int> clMap;
  uint8_t*         data      = baseTexture->GetData();
  int              width     = baseTexture->GetWidth();
  int              height    = baseTexture->GetHeight();
  int              channels  = baseTexture->GetChannels();
  constexpr int    blockSize = 4;
  int              blocksX   = (width + blockSize - 1) / blockSize;
  int              blocksY   = (height + blockSize - 1) / blockSize;

  for (int blockY = 0; blockY < blocksY; ++blockY)
  {
    for (int blockX = 0; blockX < blocksX; ++blockX)
    {
      std::map<uint32_t, int> colorFreq;
      for (int y = 0; y < blockSize; ++y)
      {
        for (int x = 0; x < blockSize; ++x)
        {
          int pixelX = blockX * blockSize + x;
          int pixelY = blockY * blockSize + y;
          if (pixelX >= width || pixelY >= height)
            continue;
          const int index    = (pixelY * width + pixelX) * channels;
          uint8_t   r        = data[index];
          uint8_t   g        = data[index + 1];
          uint8_t   b        = data[index + 2];
          uint32_t  colorKey = (r << 16) | (g << 8) | b;
          colorFreq[colorKey]++;
        }
      }
      ProcessColorFreqMap(colorFreq, clMap);
    }
  }
  return clMap;
}
void UnitDynamicTexture::ProcessColorFreqMap(
    const std::map<uint32_t, int>& colorFreq, std::vector<int>& outMap)
{
  if (!colorFreq.empty())
  {
    const auto mostCommon = std::ranges::max_element(
        colorFreq, [](const auto& a, const auto& b) { return a.second < b.second; });
    const int colorKey = mostCommon->first;
    outMap.push_back(colorKey);
    return;
  }
  outMap.push_back(0x00000000); // Default black
}

Providers::Texture2* UnitDynamicTexture::GenDynamicTexture()
{
  return GenDynamicTexture(seed);
}

Providers::Texture2* UnitDynamicTexture::GenDynamicTexture(Seed seed)
{
  if (!baseTexture->IsLoaded() || !baseTexture->GetData())
  {
    return nullptr;
  }
  // Update noise generator with new seed
  noiseGen.ResetSeed(seed);
  
  // Get color palette from base texture
  const std::vector<int> colorMap = GetTargetColorMap();
  // Generate noise map
  const std::vector<float> noiseMap = GenNoiseValMap(options.noiseSc);

  int           width     = baseTexture->GetWidth();
  int           height    = baseTexture->GetHeight();
  int           channels  = baseTexture->GetChannels();
  auto*         baseData  = baseTexture->GetData();
  auto*         newData   = new uint8_t[width * height * channels];
  constexpr int blockSize = 4;
  int           blocksX   = (width + blockSize - 1) / blockSize;
  for (int y = 0; y < height; ++y)
  {
    for (int x = 0; x < width; ++x)
    {
      const int index = (y * width + x) * channels;
      // Get base color
      uint8_t r = baseData[index];
      uint8_t g = baseData[index + 1];
      uint8_t b = baseData[index + 2];
      uint8_t a = channels == 4 ? baseData[index + 3] : 255;
      // Get corresponding block color from palette
      int       blockX     = x / blockSize;
      int       blockY     = y / blockSize;
      const int blockIndex = blockY * blocksX + blockX;
      BlendsBaseColorWithPalette(r, g, b, blockIndex, colorMap);
      const int noiseIndex = y * width + x;
      ApplyNoiseVar(r, g, b, noiseIndex, noiseMap);
      // Write to new texture
      newData[index]     = r;
      newData[index + 1] = g;
      newData[index + 2] = b;
      if (channels == 4)
      {
        newData[index + 3] = a;
      }
    }
  }
  return GenTexture(newData, width, height, channels);
}

Texture2* UnitDynamicTexture::GenTexture(
    uint8_t* data, const size_t width, const size_t height, const size_t channels)
{
  // Create new Texture2
  auto* newTexture = new Texture2();
  newTexture->LoadFromData(data, static_cast<int>(width), static_cast<int>(height),
      (channels == 4) ? TextureFormat::RGBA8 : TextureFormat::RGB8, TextureProperties());
  delete[] data;
  return newTexture;
}

uint8_t UnitDynamicTexture::ClampNoiseVar(const int v, const float variation) const
{
  const auto    vFloat = static_cast<float>(v);
  const uint8_t res    = static_cast<uint8_t>(std::clamp(vFloat + variation, 0.0f, 255.0f));
  return res;
}

void UnitDynamicTexture::ApplyNoiseVar(
    uint8_t& r, uint8_t& g, uint8_t& b, int index, const std::vector<float>& noiseMap) const
{
#ifdef X86_
  // SSE/AVX optimized version for x86/x86-64
  if (index + 3 < noiseMap.size())
  {
    __m128 noiseVec    = _mm_loadu_ps(&noiseMap[index]);
    __m128 halfVec     = _mm_set1_ps(0.5f);
    __m128 twoVec      = _mm_set1_ps(2.0f);
    __m128 colorVarVec = _mm_set1_ps(options.colorVar * 255.0f);

    // Calculate variation: (noise - 0.5) * 2.0 * colorVar * 255.0
    __m128 variation = _mm_sub_ps(noiseVec, halfVec);
    variation        = _mm_mul_ps(variation, twoVec);
    variation        = _mm_mul_ps(variation, colorVarVec);

    // Apply to RGB values
    __m128 rgb = _mm_set_ps(0, b, g, r);
    rgb        = _mm_add_ps(rgb, variation);
    rgb        = _mm_max_ps(rgb, _mm_setzero_ps());
    rgb        = _mm_min_ps(rgb, _mm_set1_ps(255.0f));

    float result[4];
    _mm_storeu_ps(result, rgb);
    r = static_cast<uint8_t>(result[3]);
    g = static_cast<uint8_t>(result[2]);
    b = static_cast<uint8_t>(result[1]);
    return;
  }
#elif defined(ARM_)
  // ARM NEON optimized version
  if (index + 3 < noiseMap.size())
  {
    float32x4_t noiseVec    = vld1q_f32(&noiseMap[index]);
    float32x4_t halfVec     = vdupq_n_f32(0.5f);
    float32x4_t twoVec      = vdupq_n_f32(2.0f);
    float32x4_t colorVarVec = vdupq_n_f32(options.colorVar * 255.0f);

    // Calculate variation: (noise - 0.5) * 2.0 * colorVar * 255.0
    float32x4_t variation = vsubq_f32(noiseVec, halfVec);
    variation             = vmulq_f32(variation, twoVec);
    variation             = vmulq_f32(variation, colorVarVec);

    // Apply to RGB values
    float32x4_t rgb =
        vsetq_lane_f32(r, vsetq_lane_f32(g, vsetq_lane_f32(b, vdupq_n_f32(0), 0), 1), 2);
    rgb = vaddq_f32(rgb, variation);
    rgb = vmaxq_f32(rgb, vdupq_n_f32(0.0f));
    rgb = vminq_f32(rgb, vdupq_n_f32(255.0f));

    float result[4];
    vst1q_f32(result, rgb);
    r = static_cast<uint8_t>(result[0]);
    g = static_cast<uint8_t>(result[1]);
    b = static_cast<uint8_t>(result[2]);
    return;
  }
#endif

  // Fallback to scalar implementation
  if (index < noiseMap.size())
  {
    const float noise = noiseMap[index];
    // Use symmetric noise variation around 0 to preserve overall brightness
    const float variation = (noise - 0.5f) * 2.0f * options.colorVar * 255.0f;
    // Apply variation symmetrically to avoid darkening bias
    r = ClampNoiseVar(r, variation);
    g = ClampNoiseVar(g, variation);
    b = ClampNoiseVar(b, variation);
  }
}
void UnitDynamicTexture::BlendsBaseColorWithPalette(
    uint8_t& r, uint8_t& g, uint8_t& b, int index, const std::vector<int>& colorMap) const
{
#ifdef X86_
  // SSE/AVX optimized version for x86/x86-64
  if (index < colorMap.size())
  {
    uint32_t paletteColor = colorMap[index];
    uint8_t  pr           = (paletteColor >> 16) & 0xFF;
    uint8_t  pg           = (paletteColor >> 8) & 0xFF;
    uint8_t  pb           = paletteColor & 0xFF;

    // Blend base color with palette color using configurable blend ratio
    float  blend         = options.paletteBlend;
    __m128 blendVec      = _mm_set1_ps(blend);
    __m128 oneMinusBlend = _mm_set1_ps(1.0f - blend);
    __m128 base          = _mm_set_ps(0, b, g, r);
    __m128 palette       = _mm_set_ps(0, pb, pg, pr);

    // Calculate: base * (1.0 - blend) + palette * blend
    __m128 result       = _mm_mul_ps(base, oneMinusBlend);
    __m128 paletteBlend = _mm_mul_ps(palette, blendVec);
    result              = _mm_add_ps(result, paletteBlend);

    float res[4];
    _mm_storeu_ps(res, result);
    r = static_cast<uint8_t>(res[3]);
    g = static_cast<uint8_t>(res[2]);
    b = static_cast<uint8_t>(res[1]);
    return;
  }
#elif defined(ARM_)
  // ARM NEON optimized version
  if (index < colorMap.size())
  {
    uint32_t paletteColor = colorMap[index];
    uint8_t  pr           = (paletteColor >> 16) & 0xFF;
    uint8_t  pg           = (paletteColor >> 8) & 0xFF;
    uint8_t  pb           = paletteColor & 0xFF;

    // Blend base color with palette color using configurable blend ratio
    float       blend         = options.paletteBlend;
    float32x4_t blendVec      = vdupq_n_f32(blend);
    float32x4_t oneMinusBlend = vdupq_n_f32(1.0f - blend);
    float32x4_t base =
        vsetq_lane_f32(r, vsetq_lane_f32(g, vsetq_lane_f32(b, vdupq_n_f32(0), 0), 1), 2);
    float32x4_t palette =
        vsetq_lane_f32(pr, vsetq_lane_f32(pg, vsetq_lane_f32(pb, vdupq_n_f32(0), 0), 1), 2);

    // Calculate: base * (1.0 - blend) + palette * blend
    float32x4_t result       = vmulq_f32(base, oneMinusBlend);
    float32x4_t paletteBlend = vmulq_f32(palette, blendVec);
    result                   = vaddq_f32(result, paletteBlend);

    float res[4];
    vst1q_f32(res, result);
    r = static_cast<uint8_t>(res[0]);
    g = static_cast<uint8_t>(res[1]);
    b = static_cast<uint8_t>(res[2]);
    return;
  }
#endif
  // Fallback to scalar implementation
  if (index < colorMap.size())
  {
    uint32_t paletteColor = colorMap[index];
    uint8_t  pr           = (paletteColor >> 16) & 0xFF;
    uint8_t  pg           = (paletteColor >> 8) & 0xFF;
    uint8_t  pb           = paletteColor & 0xFF;
    // Blend base color with palette color using configurable blend ratio
    float blend = options.paletteBlend;
    r           = static_cast<uint8_t>(r * (1.0f - blend) + pr * blend);
    g           = static_cast<uint8_t>(g * (1.0f - blend) + pg * blend);
    b           = static_cast<uint8_t>(b * (1.0f - blend) + pb * blend);
  }
}

} // namespace Rl::World
