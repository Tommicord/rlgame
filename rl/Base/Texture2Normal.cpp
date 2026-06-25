import <algorithm>;
import <cmath>;
import Rl.Base.Texture2;

namespace Rl::Providers
{

Texture2* GenerateNormalTexture(Texture2* baseTexture, const Texture2Properties& properties)
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

  // Create new texture data for normal map (RGB format)
  int    outputChannels = 3; // Normal map is RGB
  size_t outputDataSize = width * height * outputChannels;
  auto*  normalData     = new uint8_t[outputDataSize];

  // Generate normal map from base texture using Sobel filter
  for (int y = 0; y < height; ++y)
  {
    for (int x = 0; x < width; ++x)
    {
      // Calculate luminance from base texture
      auto getLuminance = [&](int px, int py) -> float
      {
        px                  = std::clamp(px, 0, width - 1);
        py                  = std::clamp(py, 0, height - 1);
        int    baseIndex    = (py * width + px) * channels;
        size_t baseDataSize = width * height * channels;

        if (channels >= 3 && baseIndex + 2 < baseDataSize)
        {
          return 0.299f * baseData[baseIndex] + 0.587f * baseData[baseIndex + 1] +
                 0.114f * baseData[baseIndex + 2];
        }
        else if (channels == 1 && baseIndex < baseDataSize)
        {
          return baseData[baseIndex];
        }
        else if (channels == 2 && baseIndex + 1 < baseDataSize)
        {
          return 0.5f * baseData[baseIndex] + 0.5f * baseData[baseIndex + 1];
        }
        return 0.0f;
      };

      // Sobel filter for gradient calculation
      float left   = getLuminance(x - 1, y);
      float right  = getLuminance(x + 1, y);
      float top    = getLuminance(x, y - 1);
      float bottom = getLuminance(x, y + 1);

      // Calculate gradients
      float dx = (right - left) * 0.5f;
      float dy = (bottom - top) * 0.5f;

      // Calculate normal from gradients (tangent space)
      // The normal points in the direction of the gradient
      float nx = -dx;
      float ny = -dy;
      float nz = 1.0f;

      // Normalize the normal vector
      float length = std::sqrt(nx * nx + ny * ny + nz * nz);
      if (length > 0.0001f)
      {
        nx /= length;
        ny /= length;
        nz /= length;
      }

      // Convert from [-1, 1] to [0, 1] range for storage
      nx = nx * 0.5f + 0.5f;
      ny = ny * 0.5f + 0.5f;
      nz = nz * 0.5f + 0.5f;

      // Store as 8-bit RGB values
      int outputIndex = (y * width + x) * outputChannels;
      if (outputIndex + 2 < outputDataSize)
      {
        normalData[outputIndex]     = static_cast<uint8_t>(nx * 255.0f);
        normalData[outputIndex + 1] = static_cast<uint8_t>(ny * 255.0f);
        normalData[outputIndex + 2] = static_cast<uint8_t>(nz * 255.0f);
      }
    }
  }

  // Create new texture with the generated normal map data
  Texture2*         normalTexture = new Texture2();
  Texture2Properties normalProps   = properties;
  normalProps.format              = Texture2Format::RGBA8; // Use RGBA8 for blit support
  normalProps.generateMipmaps     = true;
  normalProps.sRGB                = false; // Normal maps should not be in sRGB space

  // Convert RGB to RGBA for Vulkan blit compatibility
  size_t rgbaDataSize = width * height * 4;
  auto*  rgbaData     = new uint8_t[rgbaDataSize];
  for (int i = 0; i < width * height; ++i)
  {
    rgbaData[i * 4]     = normalData[i * 3];
    rgbaData[i * 4 + 1] = normalData[i * 3 + 1];
    rgbaData[i * 4 + 2] = normalData[i * 3 + 2];
    rgbaData[i * 4 + 3] = 255; // Alpha = 1.0
  }

  normalTexture->LoadFromData(rgbaData, width, height, Texture2Format::RGBA8, normalProps);
  delete[] rgbaData;

  // Clean up temporary data
  delete[] normalData;

  return normalTexture;
}

} // namespace Rl::Providers
