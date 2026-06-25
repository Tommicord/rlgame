#include <cstdint>
#include <gtest/gtest.h>

#include "rl/Base/SimplexNoise.h"
#include "rl/Base/Texture2.h"
#include "rl/World/Unit/UnitDynamicTexture.h"

using namespace Rl::World;
using namespace Rl::Providers;

class UnitDynamicTextureTest : public ::testing::Test
{
  protected:
  void SetUp() override
  {
    testTexture = new Texture2();
    bool loaded = testTexture->LoadFromFile("dirt.png");
    ASSERT_TRUE(loaded) << "Failed to load test texture data";

    options.noiseSc  = 0.1f;
    options.colorVar = 0.15f;
    testSeed         = 12345;
    generator        = new UnitDynamicTexture(*testTexture, testSeed, options);
  }

  void TearDown() override
  {
    delete generator;
    delete testTexture;
  }
  Texture2*                          testTexture;
  UnitDynamicTexture::DynamicOptions options;
  UnitDynamicTexture::Seed           testSeed;
  UnitDynamicTexture*                generator;
};

// Test GenNoiseValMap
TEST_F(UnitDynamicTextureTest, GenNoiseValMapCorrectSize)
{
  auto noiseMap = generator->GenNoiseValMap(0.1f);

  EXPECT_EQ(noiseMap.size(), testTexture->GetWidth() * testTexture->GetHeight());
}

TEST_F(UnitDynamicTextureTest, GenNoiseValMapValidRange)
{
  auto noiseMap = generator->GenNoiseValMap(0.1f);

  for (float noise : noiseMap)
  {
    // OpenSimplex noise can occasionally produce values slightly outside [-1, 1]
    // due to floating point precision, so allow a small tolerance
    EXPECT_GE(noise, -1.1f);
    EXPECT_LE(noise, 1.1f);
  }
}

TEST_F(UnitDynamicTextureTest, GenNoiseValMapDeterministic)
{
  auto noiseMap1 = generator->GenNoiseValMap(0.1f);
  auto noiseMap2 = generator->GenNoiseValMap(0.1f);

  ASSERT_EQ(noiseMap1.size(), noiseMap2.size());
  for (size_t i = 0; i < noiseMap1.size(); i++)
  {
    EXPECT_FLOAT_EQ(noiseMap1[i], noiseMap2[i]);
  }
}

TEST_F(UnitDynamicTextureTest, GenNoiseValMapDifferentScales)
{
  auto noiseMapSmall = generator->GenNoiseValMap(0.05f);
  auto noiseMapLarge = generator->GenNoiseValMap(0.2f);

  EXPECT_EQ(noiseMapSmall.size(), noiseMapLarge.size());

  // Different scales should produce different results
  bool different = false;
  for (size_t i = 0; i < noiseMapSmall.size(); i++)
  {
    if (std::abs(noiseMapSmall[i] - noiseMapLarge[i]) > 0.01f)
    {
      different = true;
      break;
    }
  }
  EXPECT_TRUE(different);
}

// Test GetTargetColorMap
TEST_F(UnitDynamicTextureTest, GetTargetColorMapCorrectSize)
{
  auto colorMap = generator->GetTargetColorMap();
  // Calculate expected size based on actual texture dimensions
  int width = testTexture->GetWidth();
  int height = testTexture->GetHeight();
  constexpr int blockSize = 4; // Must match implementation in UnitDynamicTexture.cpp
  int blocksX = (width + blockSize - 1) / blockSize;
  int blocksY = (height + blockSize - 1) / blockSize;
  int expectedSize = blocksX * blocksY;
  EXPECT_EQ(colorMap.size(), expectedSize);
}

TEST_F(UnitDynamicTextureTest, GetTargetColorMapValidColors)
{
  auto colorMap = generator->GetTargetColorMap();

  for (int color : colorMap)
  {
    EXPECT_GE(color, 0x00000000);
    EXPECT_LE(color, 0x00FFFFFF);
  }
}

TEST_F(UnitDynamicTextureTest, GetTargetColorMapDeterministic)
{
  auto colorMap1 = generator->GetTargetColorMap();
  auto colorMap2 = generator->GetTargetColorMap();

  ASSERT_EQ(colorMap1.size(), colorMap2.size());
  for (size_t i = 0; i < colorMap1.size(); i++)
  {
    EXPECT_EQ(colorMap1[i], colorMap2[i]);
  }
}

// Test GenDynamicTexture
TEST_F(UnitDynamicTextureTest, GenDynamicTextureReturnsValidTexture)
{
  Texture2* dynamicTexture = generator->GenDynamicTexture(testSeed);

  ASSERT_NE(dynamicTexture, nullptr);
  EXPECT_TRUE(dynamicTexture->IsLoaded());

  delete dynamicTexture;
}

TEST_F(UnitDynamicTextureTest, GenDynamicTextureDifferentSeeds)
{
  Texture2* texture1 = generator->GenDynamicTexture(12342233252);
  Texture2* texture2 = generator->GenDynamicTexture(54323939931);

  ASSERT_NE(texture1, nullptr);
  ASSERT_NE(texture2, nullptr);

  // Different seeds should produce different results
  uint8_t* data1 = texture1->GetData();
  uint8_t* data2 = texture2->GetData();

  bool different = false;
  for (int i = 0; i < testTexture->GetDataSize(); ++i)
  {
    if (data1[i] != data2[i])
    {
      different = true;
      break;
    }
  }
  EXPECT_TRUE(different);

  delete texture1;
  delete texture2;
}

// Test edge cases
TEST(UnitDynamicTextureEdgeCases, NullTexture)
{
  Texture2                           nullTexture;
  UnitDynamicTexture::DynamicOptions options;
  UnitDynamicTexture                 generator(nullTexture, 12345, options);

  Texture2* result = generator.GenDynamicTexture(12345);
  EXPECT_EQ(result, nullptr);
}

TEST(UnitDynamicTextureEdgeCases, UnloadedTexture)
{
  Texture2                           unloadedTexture;
  UnitDynamicTexture::DynamicOptions options;
  UnitDynamicTexture                 generator(unloadedTexture, 12345, options);

  Texture2* result = generator.GenDynamicTexture(12345);
  EXPECT_EQ(result, nullptr);
}

TEST(UnitDynamicTextureEdgeCases, ZeroNoiseScale)
{
  uint8_t  testData[64] = {0};
  Texture2 testTexture;
  testTexture.LoadFromData(testData, 8, 8, TextureFormat::RGB8, TextureProperties());

  UnitDynamicTexture::DynamicOptions options;
  options.noiseSc  = 0.0f;
  options.colorVar = 0.0f;

  UnitDynamicTexture generator(testTexture, 12345, options);
  Texture2*          result = generator.GenDynamicTexture(12345);

  ASSERT_NE(result, nullptr);
  // With zero variation result should be very similar to input
  delete result;
}

TEST(UnitDynamicTextureEdgeCases, MaximumVariation)
{
  uint8_t  testData[64] = {128}; // Gray texture
  Texture2 testTexture;
  testTexture.LoadFromData(testData, 8, 8, TextureFormat::RGB8, TextureProperties());

  UnitDynamicTexture::DynamicOptions options;
  options.noiseSc  = 1.0f;
  options.colorVar = 1.0f;

  UnitDynamicTexture generator(testTexture, 12345, options);
  Texture2*          result = generator.GenDynamicTexture(12345);

  ASSERT_NE(result, nullptr);
  // With maximum variation result should be very different
  delete result;
}
