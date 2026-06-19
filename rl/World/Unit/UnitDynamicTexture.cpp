#include "rl/World/Unit/UnitDynamicTexture.h"
#include "rl/Base/SimplexNoise.h"
#include "rl/Base/Texture2.h"

#include <algorithm>
#include <vector>
#include <map>

namespace Rl::World {
using namespace Rl::Providers;

UnitDynamicTexture::UnitDynamicTexture(Texture2& base, const Seed seed, DynamicOptions& options) :
    options(options), noiseGen{seed}, baseTexture(&base), seed(seed)
{
}

std::vector<float> UnitDynamicTexture::GenNoiseValMap(const float scale) const
{
    std::vector<float> map;
    int width = baseTexture->GetWidth();
    int height = baseTexture->GetHeight();
    map.resize(width * height);
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            const size_t index = y * width + x;
            float randX = static_cast<float>(x) * scale;
            float randY = static_cast<float>(y) * scale;
            float noise = noiseGen.eval(randX, randY);
            map[index] = noise;
        }
    }
    return map;
}

std::vector<int> UnitDynamicTexture::GetTargetColorMap() const
{
    std::vector<int> clMap;
    uint8_t* data = baseTexture->GetData();
    int width = baseTexture->GetWidth();
    int height = baseTexture->GetHeight();
    int channels = baseTexture->GetChannels();
    constexpr int blockSize = 4;
    int blocksX = (width + blockSize - 1) / blockSize;
    int blocksY = (height + blockSize - 1) / blockSize;

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
                    const int index = (pixelY * width + pixelX) * channels;
                    uint8_t r = data[index];
                    uint8_t g = data[index + 1];
                    uint8_t b = data[index + 2];
                    uint32_t colorKey = (r << 16) | (g << 8) | b;;
                    colorFreq[colorKey]++;
                }
            }
            ProcessColorFreqMap(colorFreq, clMap);
        }
    }
    return clMap;
}
void UnitDynamicTexture::ProcessColorFreqMap(const std::map<uint32_t, int>& colorFreq,
                                             std::vector<int>& outMap)
{
    if (!colorFreq.empty())
    {
        const auto mostCommon =
            std::ranges::max_element(colorFreq,
                [](const auto& a, const auto& b)
                {
                    return a.second < b.second;
                }
            );
        const int colorKey = mostCommon->first;
        outMap.push_back(colorKey);
        return;
    }
    outMap.push_back(0x00000000); // Default black
}
Providers::Texture2 *UnitDynamicTexture::GenDynamicTexture(Seed seed)
{
    if (!baseTexture->IsLoaded() || !baseTexture->GetData())
    {
        return nullptr;
    }
    // Update noise generator with new seed
    noiseGen.~OpenSimplexNoiseGen();
    new (&noiseGen)
        OpenSimplexNoiseGen(seed);
    // Get color palette from base texture
    const std::vector<int> colorMap = GetTargetColorMap();
    // Generate noise map
    const std::vector<float> noiseMap = GenNoiseValMap(options.noiseSc);

    int width = baseTexture->GetWidth();
    int height = baseTexture->GetHeight();
    int channels = baseTexture->GetChannels();
    auto* baseData = baseTexture->GetData();
    auto* newData = new uint8_t[width * height * channels];
    constexpr int blockSize = 4;
    int blocksX = (width + blockSize - 1) / blockSize;
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
            int blockX = x / blockSize;
            int blockY = y / blockSize;
            const int blockIndex = blockY * blocksX + blockX;
            BlendsBaseColorWithPalette(r, g, b, blockIndex, colorMap);
            const int noiseIndex = y * width + x;
            ApplyNoiseVar(r, g, b, noiseIndex, noiseMap);
            // Write to new texture
            newData[index] = r;
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
                                            uint8_t* data,
                                            const size_t width, const size_t height,
                                            const size_t channels)
{
    // Create new Texture2
    auto* newTexture = new Texture2();
    newTexture->LoadFromData(
        data,
        static_cast<int>(width), static_cast<int>(height),
        (channels == 4) ? TextureFormat::RGBA8 : TextureFormat::RGB8,
        TextureProperties());
    delete[] data;
    return newTexture;
}

uint8_t UnitDynamicTexture::ClampNoiseVar(const int v, const float variation) const
{
    const auto vFloat = static_cast<float>(v);
    const uint8_t res = static_cast<uint8_t>(
        std::clamp(vFloat + variation, 0.0f, 255.0f));
    return res;
}

void UnitDynamicTexture::ApplyNoiseVar(
                                    uint8_t& r, uint8_t& g, uint8_t& b,
                                    int index,
                                    const std::vector<float>& noiseMap) const
{
    if (index < noiseMap.size())
    {
        const float noise = noiseMap[index];
        const float variation =
            (noise - 0.5f) * 2.0f * options.colorVar * 255.0f;
        r = ClampNoiseVar(r, variation);
        g = ClampNoiseVar(g, variation);
        b = ClampNoiseVar(b, variation);
    }
}
void UnitDynamicTexture::BlendsBaseColorWithPalette(uint8_t& r, uint8_t& g, uint8_t& b, int index,
                                                    const std::vector<int>& colorMap) const
{
    if (index < colorMap.size())
    {
        uint32_t paletteColor = colorMap[index];
        uint8_t pr = (paletteColor >> 16) & 0xFF;
        uint8_t pg = (paletteColor >> 8) & 0xFF;
        uint8_t pb = paletteColor & 0xFF;
        // Blend base color with palette color
        r = static_cast<uint8_t>((r + pr) / 2);
        g = static_cast<uint8_t>((g + pg) / 2);
        b = static_cast<uint8_t>((b + pb) / 2);
    }
}

} // namespace Rl::World
