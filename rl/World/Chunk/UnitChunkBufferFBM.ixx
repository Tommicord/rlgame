export module Rl.World.Chunk.UnitChunkBufferFBM;

import Rl.World.Chunk.UnitGPUSimplexNoise;

import <cstddef>;
import <cstdint>;

namespace Rl::World::Chunk
{
class GPUSimplexParam
{
    /* Helper to create standard FBM noise parameters */
    static SimplexNoisePushConstants CreateFBM(uint32_t dimension,
                                               float    scale,
                                               float    offsetX,
                                               float    offsetY,
                                               float    offsetZ,
                                               uint32_t width,
                                               uint32_t height,
                                               uint32_t depth,
                                               uint32_t octaves,
                                               float    persistence,
                                               float    lacunarity)
    {
        SimplexNoisePushConstants params{};
        params.dimension   = dimension;
        params.scale       = scale;
        params.offsetX     = offsetX;
        params.offsetY     = offsetY;
        params.offsetZ     = offsetZ;
        params.width       = width;
        params.height      = height;
        params.depth       = depth;
        params.time        = 0;
        params.octaves     = octaves;
        params.persistence = persistence;
        params.lacunarity  = lacunarity;
        params.noiseType   = 1; // FBM
        return params;
    }

    /* Helper to create ridged noise parameters */
    static SimplexNoisePushConstants CreateRidged(uint32_t dimension,
                                                  float    scale,
                                                  float    offsetX,
                                                  float    offsetY,
                                                  float    offsetZ,
                                                  uint32_t width,
                                                  uint32_t height,
                                                  uint32_t depth,
                                                  uint32_t octaves,
                                                  float    persistence,
                                                  float    lacunarity)
    {
        SimplexNoisePushConstants params{};
        params.dimension   = dimension;
        params.scale       = scale;
        params.offsetX     = offsetX;
        params.offsetY     = offsetY;
        params.offsetZ     = offsetZ;
        params.width       = width;
        params.height      = height;
        params.depth       = depth;
        params.time        = 0;
        params.octaves     = octaves;
        params.persistence = persistence;
        params.lacunarity  = lacunarity;
        params.noiseType   = 2; // Ridged
        return params;
    }

    /* Helper to create turbulence noise parameters */
    static SimplexNoisePushConstants CreateTurbulence(uint32_t dimension,
                                                      float    scale,
                                                      float    offsetX,
                                                      float    offsetY,
                                                      float    offsetZ,
                                                      uint32_t width,
                                                      uint32_t height,
                                                      uint32_t depth,
                                                      uint32_t octaves,
                                                      float    persistence,
                                                      float    lacunarity)
    {
        SimplexNoisePushConstants params{};
        params.dimension   = dimension;
        params.scale       = scale;
        params.offsetX     = offsetX;
        params.offsetY     = offsetY;
        params.offsetZ     = offsetZ;
        params.width       = width;
        params.height      = height;
        params.depth       = depth;
        params.time        = 0;
        params.octaves     = octaves;
        params.persistence = persistence;
        params.lacunarity  = lacunarity;
        params.noiseType   = 3; // Turbulence
        return params;
    }
};

} // namespace Rl::World::Chunk
