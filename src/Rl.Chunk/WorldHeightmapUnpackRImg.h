#ifndef RL_CHUNK_WORLD_HEIGHTMAP_UNPACK_RIMG_H
#define RL_CHUNK_WORLD_HEIGHTMAP_UNPACK_RIMG_H

#include <vector>
#include <cstdint>
#include <cstddef>

namespace rl
{

// Forward declaration of ChunkHeightmapData struct
struct WorldHeightmapData;

/**
 * @brief Unpacks R8 (8-bit red channel) pixel data into a vector of floats.
 * @param pixelData Pointer to the input pixel data (R8 format).
 * @param output Reference to the output vector where the unpacked float values will be stored.
 * @note The output vector must be pre-sized to the appropriate number of elements before calling
 * this function.
 */
void unpackR8ToFloatSIMD(const uint8_t* pixelData, std::vector<WorldHeightmapData>& output);

} // namespace rl

#endif // RL_CHUNK_WORLD_HEIGHTMAP_UNPACK_RIMG_H
