#include "rl/World/Unit/UnitDynamicTexture.h"

namespace Rl::World {

using namespace Rl::Providers;

UnitDynamicTexture::UnitDynamicTexture(Texture2& base,
                                       const Seed seed,
                                       DynamicOptions& options) :
    options(options),
    noiseGen{seed},
    baseTexture(base), seed(seed)
{
}

std::vector<float> UnitDynamicTexture::GenNoiseValMap(const float scale) const
{
    std::vector<float> map;
    int width = baseTexture.GetWidth();
    int height = baseTexture.GetHeight();
    map.reserve(width * height);
    int channels = baseTexture.GetChannels();
    for (int y = 0; y < width; ++y)
    {
        for (int x = 0; x < height; ++x)
        {
            const size_t index = (y * width + x) * channels;
            float randX =
                static_cast<float>(x) * scale;
            float randY =
                static_cast<float>(y) * scale;
            float noise = noiseGen.eval(randX, randY);
            map[index] = noise;
        }
    }
    return map;
}

} // namespace Rl::World
