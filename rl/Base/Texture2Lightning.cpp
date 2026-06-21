#include "rl/Base/Texture2.h"

#include <algorithm>
#include <cmath>
#include <random>

namespace Rl::Providers
{

Texture2* GenerateLightningTexture(Texture2* baseTexture, const TextureProperties& properties)
{
  if (!baseTexture || !baseTexture->IsLoaded())
  {
    return nullptr;
  }

  // Get base texture data
  uint8_t* baseData = baseTexture->GetData();
  int      width    = baseTexture->GetWidth();
  int      height   = baseTexture->GetHeight();
  int      channels = baseTexture->GetChannels();

  if (!baseData || width <= 0 || height <= 0)
  {
    return nullptr;
  }

  // Create new texture data for lightning texture
  int    outputChannels = 1; // AO is single channel
  size_t outputDataSize = width * height * outputChannels;
  auto*  lightningData  = new uint8_t[outputDataSize];

  // Generate lightning texture based on base texture
  // This creates a simple ambient occlusion effect
  for (int y = 0; y < height; ++y)
  {
    for (int x = 0; x < width; ++x)
    {
      int baseIndex   = (y * width + x) * channels;
      int outputIndex = y * width + x;

      // Calculate luminance from base texture
      float  luminance    = 0.0f;
      size_t baseDataSize = width * height * channels;
      if (channels >= 3 && baseIndex + 2 < baseDataSize)
      {
        luminance = 0.299f * baseData[baseIndex] + 0.587f * baseData[baseIndex + 1] +
                    0.114f * baseData[baseIndex + 2];
      }
      else if (channels == 1 && baseIndex < baseDataSize)
      {
        luminance = baseData[baseIndex];
      }
      else if (channels == 2 && baseIndex + 1 < baseDataSize)
      {
        luminance = 0.5f * baseData[baseIndex] + 0.5f * baseData[baseIndex + 1];
      }

      // Convert to 0-1 range
      float normalizedLuminance = luminance / 255.0f;

      // Simple AO calculation based on luminance
      // Darker areas get more occlusion
      float ao = 1.0f - (normalizedLuminance * 0.5f);

      // Add some noise for more natural look
      static std::random_device                    rd;
      static std::mt19937                          gen(rd());
      static std::uniform_real_distribution<float> dis(-0.05f, 0.05f);
      ao += dis(gen);
      ao = std::clamp(ao, 0.0f, 1.0f);

      // Store as 8-bit value with bounds checking
      if (outputIndex < outputDataSize)
      {
        lightningData[outputIndex] = static_cast<uint8_t>(ao * 255.0f);
      }
    }
  }

  // Create new texture with the generated data
  Texture2*         lightningTexture = new Texture2();
  TextureProperties lightningProps   = properties;
  lightningProps.format              = TextureFormat::R8;
  lightningProps.generateMipmaps     = true;

  lightningTexture->LoadFromData(lightningData, width, height, TextureFormat::R8, lightningProps);

  // Clean up temporary data
  delete[] lightningData;

  return lightningTexture;
}

Texture2* GenerateDirectionalLightTexture(
    Texture2* baseTexture, const glm::vec3& lightDirection, const TextureProperties& properties)
{
  if (!baseTexture || !baseTexture->IsLoaded())
  {
    return nullptr;
  }

  // Get base texture data
  uint8_t* baseData = baseTexture->GetData();
  int      width    = baseTexture->GetWidth();
  int      height   = baseTexture->GetHeight();
  int      channels = baseTexture->GetChannels();

  if (!baseData || width <= 0 || height <= 0)
  {
    return nullptr;
  }

  // Create new texture data for lighting texture
  int    outputChannels = 3; // RGB lighting
  size_t outputDataSize = width * height * outputChannels;
  auto*  lightingData   = new uint8_t[outputDataSize];

  // Normalize light direction
  glm::vec3 normalizedLightDir = glm::normalize(lightDirection);

  // Generate directional lighting texture
  for (int y = 0; y < height; ++y)
  {
    for (int x = 0; x < width; ++x)
    {
      int baseIndex   = (y * width + x) * channels;
      int outputIndex = (y * width + x) * outputChannels;

      // Calculate simple normal from texture gradients
      float  nx = 0.0f, ny = 0.0f, nz = 1.0f;
      size_t baseDataSize = width * height * channels;

      if (x > 0 && x < width - 1 && y > 0 && y < height - 1)
      {
        int leftIndex   = (y * width + (x - 1)) * channels;
        int rightIndex  = (y * width + (x + 1)) * channels;
        int topIndex    = ((y - 1) * width + x) * channels;
        int bottomIndex = ((y + 1) * width + x) * channels;

        // Check bounds before accessing
        if (leftIndex >= 0 && leftIndex < baseDataSize && rightIndex >= 0 &&
            rightIndex < baseDataSize && topIndex >= 0 && topIndex < baseDataSize &&
            bottomIndex >= 0 && bottomIndex < baseDataSize)
        {

          float leftLum   = baseData[leftIndex] / 255.0f;
          float rightLum  = baseData[rightIndex] / 255.0f;
          float topLum    = baseData[topIndex] / 255.0f;
          float bottomLum = baseData[bottomIndex] / 255.0f;

          nx = leftLum - rightLum;
          ny = topLum - bottomLum;
          nz = 1.0f;

          // Normalize
          float length = std::sqrt(nx * nx + ny * ny + nz * nz);
          if (length > 0.0f)
          {
            nx /= length;
            ny /= length;
            nz /= length;
          }
        }
      }

      // Calculate lighting intensity
      glm::vec3 normal(nx, ny, nz);
      float     intensity = std::max(0.0f, glm::dot(normal, normalizedLightDir));

      // Apply lighting to base texture color
      float r = 0.0f, g = 0.0f, b = 0.0f;
      if (channels >= 3 && baseIndex + 2 < baseDataSize)
      {
        r = (baseData[baseIndex] / 255.0f) * intensity;
        g = (baseData[baseIndex + 1] / 255.0f) * intensity;
        b = (baseData[baseIndex + 2] / 255.0f) * intensity;
      }
      else if (channels == 1 && baseIndex < baseDataSize)
      {
        float gray = (baseData[baseIndex] / 255.0f) * intensity;
        r = g = b = gray;
      }

      // Store as 8-bit values with bounds checking
      if (outputIndex + 2 < outputDataSize)
      {
        lightingData[outputIndex]     = static_cast<uint8_t>(r * 255.0f);
        lightingData[outputIndex + 1] = static_cast<uint8_t>(g * 255.0f);
        lightingData[outputIndex + 2] = static_cast<uint8_t>(b * 255.0f);
      }
    }
  }

  // Create new texture with the generated data
  Texture2*         lightingTexture = new Texture2();
  TextureProperties lightingProps   = properties;
  lightingProps.format              = TextureFormat::RGB8;
  lightingProps.generateMipmaps     = true;

  lightingTexture->LoadFromData(lightingData, width, height, TextureFormat::RGB8, lightingProps);

  // Clean up temporary data
  delete[] lightingData;

  return lightingTexture;
}

} // namespace Rl::Providers
