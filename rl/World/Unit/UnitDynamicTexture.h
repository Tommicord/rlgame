#pragma once

#include "rl/Base/SimplexNoise.h"
#include "rl/Base/Texture2.h"

namespace Rl::World {

class AbstractUnitDynamicTexture
{
public:
    virtual ~AbstractUnitDynamicTexture() = default;

    struct DynamicOptions : Providers::Texture2
    {
        int octave = 8;
        float persistence = 0.5f;
        float lacunarity = 1.25f;
    };
    /* Describes a 64-bit seed value for the randomize texture process */
    using Seed = long long;

    /* Generates a randomized texture from the base texture */
    virtual void GenDynamicTexture(Seed seed);
};

class UnitDynamicTexture : public AbstractUnitDynamicTexture
{
public:
    /* Options for randomizing the texture adding noise */
    DynamicOptions& options;

    /* The Simplex noise generator */
    Providers::OpenSimplexNoiseGen noiseGen;

    /* The base texture that will help to generate the random texture */
    Providers::Texture2& baseTexture;

    /* The seed for randomness */
    Seed seed;

    /* Constructs a basic dynamic texture for Unit with a Seed */
    UnitDynamicTexture(
        Providers::Texture2& base,
        Seed seed,
        DynamicOptions& options
        );
    /* Generates a noise map for the random texture
     * For example, a texture of 32x32 will generate
     * Coherent to the texture noise of a 32x32 array
     */
    std::vector<float> GenNoiseValMap(float scale) const;

};

}