#include "Rl.Chunk/WorldHeightmapUnpackRImg.h"
#include "Rl.Chunk/WorldHeightmap.h"

namespace rl
{

void unpackR8ToFloatSIMD(const uint8_t* pixelData, std::vector<WorldHeightmapData>& output)
{
  const size_t count = output.size();

  const uint8_t* elevationData   = pixelData;
  const uint8_t* temperatureData = pixelData + count;
  const uint8_t* moistureData    = pixelData + count * 2;

  size_t i = 0;

#if defined(_RL_SIMD_AVX2)
  // Process in chunks of 8 but ensure don't read past the end
  const size_t avx2Count = (count / 8) * 8;
  for (; i < avx2Count; i += 8)
  {
    __m128i eBytes = _mm_loadu_si128(reinterpret_cast<const __m128i*>(elevationData + i));
    __m128i tBytes = _mm_loadu_si128(reinterpret_cast<const __m128i*>(temperatureData + i));
    __m128i mBytes = _mm_loadu_si128(reinterpret_cast<const __m128i*>(moistureData + i));

    __m256i eint = _mm256_cvtepu8_epi32(eBytes);
    __m256i tint = _mm256_cvtepu8_epi32(tBytes);
    __m256i mint = _mm256_cvtepu8_epi32(mBytes);

    __m256 eFloat = _mm256_cvtepi32_ps(eint);
    __m256 tFloat = _mm256_cvtepi32_ps(tint);
    __m256 mFloat = _mm256_cvtepi32_ps(mint);

    __m256 inv255 = _mm256_set1_ps(1.0f / 255.0f);
    eFloat        = _mm256_mul_ps(eFloat, inv255);
    tFloat        = _mm256_mul_ps(tFloat, inv255);
    mFloat        = _mm256_mul_ps(mFloat, inv255);

    __m256 one = _mm256_set1_ps(1.0f);
    eFloat     = _mm256_sub_ps(one, eFloat);

    float eArray[8], tArray[8], mArray[8];
    _mm256_storeu_ps(eArray, eFloat);
    _mm256_storeu_ps(tArray, tFloat);
    _mm256_storeu_ps(mArray, mFloat);

    for (int j = 0; j < 8; ++j)
    {
      output[i + j].elevation   = eArray[j];
      output[i + j].temperature = tArray[j];
      output[i + j].moisture    = mArray[j];
    }
  }
#elif defined(_RL_SIMD_SSE)
  const size_t sseCount = (count / 4) * 4;
  for (; i < sseCount; i += 4)
  {
    __m128i eBytes = _mm_loadu_si128(reinterpret_cast<const __m128i*>(elevationData + i));
    __m128i tBytes = _mm_loadu_si128(reinterpret_cast<const __m128i*>(temperatureData + i));
    __m128i mBytes = _mm_loadu_si128(reinterpret_cast<const __m128i*>(moistureData + i));

    __m128i eintLow  = _mm_cvtepu8_epi32(eBytes);
    __m128i eintHigh = _mm_cvtepu8_epi32(_mm_srli_si128(eBytes, 4));
    __m128i tintLow  = _mm_cvtepu8_epi32(tBytes);
    __m128i tintHigh = _mm_cvtepu8_epi32(_mm_srli_si128(tBytes, 4));
    __m128i mintLow  = _mm_cvtepu8_epi32(mBytes);
    __m128i mintHigh = _mm_cvtepu8_epi32(_mm_srli_si128(mBytes, 4));

    __m128 eFloatLow  = _mm_cvtepi32_ps(eintLow);
    __m128 eFloatHigh = _mm_cvtepi32_ps(eintHigh);
    __m128 tFloatLow  = _mm_cvtepi32_ps(tintLow);
    __m128 tFloatHigh = _mm_cvtepi32_ps(tintHigh);
    __m128 mFloatLow  = _mm_cvtepi32_ps(mintLow);
    __m128 mFloatHigh = _mm_cvtepi32_ps(mintHigh);

    __m128 inv255 = _mm_set1_ps(1.0f / 255.0f);
    eFloatLow     = _mm_mul_ps(eFloatLow, inv255);
    eFloatHigh    = _mm_mul_ps(eFloatHigh, inv255);
    tFloatLow     = _mm_mul_ps(tFloatLow, inv255);
    tFloatHigh    = _mm_mul_ps(tFloatHigh, inv255);
    mFloatLow     = _mm_mul_ps(mFloatLow, inv255);
    mFloatHigh    = _mm_mul_ps(mFloatHigh, inv255);

    __m128 one = _mm_set1_ps(1.0f);
    eFloatLow  = _mm_sub_ps(one, eFloatLow);
    eFloatHigh = _mm_sub_ps(one, eFloatHigh);

    float eArray[4], tArray[4], mArray[4];
    _mm_storeu_ps(eArray, eFloatLow);
    _mm_storeu_ps(tArray, tFloatLow);
    _mm_storeu_ps(mArray, mFloatLow);

    for (int j = 0; j < 4; ++j)
    {
      output[i + j].elevation   = eArray[j];
      output[i + j].temperature = tArray[j];
      output[i + j].moisture    = mArray[j];
    }
  }
#elif defined(_RL_SIMD_ARM_NEON)
  const size_t neonCount = (count / 8) * 8;
  for (; i < neonCount; i += 8)
  {
    uint8x8_t eBytes = vld1_u8(elevationData + i);
    uint8x8_t tBytes = vld1_u8(temperatureData + i);
    uint8x8_t mBytes = vld1_u8(moistureData + i);

    float32x4_t eFloatLow  = vcvtq_f32_u32(vmovl_u16(vget_low_u16(vmovl_u8(eBytes))));
    float32x4_t eFloatHigh = vcvtq_f32_u32(vmovl_u16(vget_high_u16(vmovl_u8(eBytes))));
    float32x4_t tFloatLow  = vcvtq_f32_u32(vmovl_u16(vget_low_u16(vmovl_u8(tBytes))));
    float32x4_t tFloatHigh = vcvtq_f32_u32(vmovl_u16(vget_high_u16(vmovl_u8(tBytes))));
    float32x4_t mFloatLow  = vcvtq_f32_u32(vmovl_u16(vget_low_u16(vmovl_u8(mBytes))));
    float32x4_t mFloatHigh = vcvtq_f32_u32(vmovl_u16(vget_high_u16(vmovl_u8(mBytes))));

    float32x4_t inv255 = vdupq_n_f32(1.0f / 255.0f);
    eFloatLow          = vmulq_f32(eFloatLow, inv255);
    eFloatHigh         = vmulq_f32(eFloatHigh, inv255);
    tFloatLow          = vmulq_f32(tFloatLow, inv255);
    tFloatHigh         = vmulq_f32(tFloatHigh, inv255);
    mFloatLow          = vmulq_f32(mFloatLow, inv255);
    mFloatHigh         = vmulq_f32(mFloatHigh, inv255);

    float32x4_t one = vdupq_n_f32(1.0f);
    eFloatLow       = vsubq_f32(one, eFloatLow);
    eFloatHigh      = vsubq_f32(one, eFloatHigh);

    float eArray[8], tArray[8], mArray[8];
    vst1q_f32(eArray, eFloatLow);
    vst1q_f32(eArray + 4, eFloatHigh);
    vst1q_f32(tArray, tFloatLow);
    vst1q_f32(tArray + 4, tFloatHigh);
    vst1q_f32(mArray, mFloatLow);
    vst1q_f32(mArray + 4, mFloatHigh);

    for (int j = 0; j < 8; ++j)
    {
      output[i + j].elevation   = eArray[j];
      output[i + j].temperature = tArray[j];
      output[i + j].moisture    = mArray[j];
    }
  }
#endif
  for (; i < count; ++i)
  {
    output[i].elevation   = 1.0f - (elevationData[i] / 255.0f);
    output[i].temperature = temperatureData[i] / 255.0f;
    output[i].moisture    = moistureData[i] / 255.0f;
  }
}

} // namespace rl
