export module Rl.World.Chunk.UnitChunkBuffer;

import <cstdint>;
import <cstring>;
import <memory>;
import <optional>;

namespace Rl::World::Chunk
{

/* Chunk Storage data */
export class UnitChunkBuffer
{
  public:

  using BufferUnit  = int;
  /* Stores the chunk buffer dimensions */
  template <int W, int H, int D>
  struct ChunkBufferSizes
  {
    int w = W, h = H, d = D;
  };
  /* Represents the chunk buffer */
  struct ChunkBuffer
  {
    /* Stores array of Unit id's */
    std::unique_ptr<BufferUnit[]> b;

    /* Gets the Unit buffer */
    [[nodiscard]]
   BufferUnit* Get() const noexcept
    {
      return b.get();
    }

    /* Deletes the Unit buffer */
    void Delete() noexcept
    {
      b.reset();
    }
  };

  /* Represents a 3D coordinate in the chunk */
  struct ChunkCoord
  {
    int x, y, z;
  };

  /*
   * A 64x64x128x4 Array of 2 MB is cacheable in the GPU L2 cache
   * This is an ideal size for a Chunk of Units
   * Probably this will make a bit more fast
   */
  static constexpr int W = 64;
  static constexpr int H = 128;
  static constexpr int D = 64;

  using BufferSizes = ChunkBufferSizes<W, H, D>;
  using Buffer      = ChunkBuffer;
  BufferSizes bufferSizes;
  Buffer      buffer;

  /* Flag to track if buffer is owned (for move semantics) */
  bool owner = true;

  UnitChunkBuffer() noexcept;
  ~UnitChunkBuffer();

  /* Copy constructor */
  UnitChunkBuffer(const UnitChunkBuffer& other);
  /* Move constructor */
  UnitChunkBuffer(UnitChunkBuffer&& other) noexcept;
  /* Copy assignment */
  UnitChunkBuffer& operator=(const UnitChunkBuffer& other);
  /* Move assignment */
  UnitChunkBuffer& operator=(UnitChunkBuffer&& other) noexcept;

  /* Clears the entire chunk buffer (sets all to Air Unit id) */
  void Clear();

  /* Gets a Unit block id at 3D coordinate in the chunk buffer */
  [[nodiscard]]
  std::optional<BufferUnit> GetUnitIdXYZ(int x, int y, int z) const;

  /* Gets a Unit block id using ChunkCoord */
  [[nodiscard]]
  std::optional<BufferUnit> GetUnitId(const ChunkCoord& coord) const;

  /* Checks if a coordinate is within chunk bounds */
  [[nodiscard]]
  bool IsInBounds(int x, int y, int z) const;

  /* Checks if a ChunkCoord is within chunk bounds */
  [[nodiscard]]
  bool IsInBounds(const ChunkCoord& coord) const;

  /* Gets the total number of blocks in the chunk */
  static [[nodiscard]]
  constexpr int GetTotalBlocks()
  {
    return W * H * D;
  }

  /* Gets the raw buffer pointer (for GPU transfer) */
  [[nodiscard]]
  BufferUnit* GetRaw()
  {
    return buffer.Get();
  }

  /* Gets the raw buffer pointer (const version) */
  [[nodiscard]]
  const BufferUnit* GetRaw() const
  {
    return buffer.Get();
  }

  /* Gets the buffer size in bytes */
  [[nodiscard]]
  constexpr size_t GetBufferSizeBytes() const
  {
    return static_cast<size_t>(W * H * D) * sizeof(BufferUnit);
  }

  /* Checks if the buffer is valid (allocated) */
  [[nodiscard]]
  bool IsValid() const;

  /* Gets chunk dimensions as ChunkCoord */
  [[nodiscard]]
  ChunkCoord GetDimensions() const;
};

/* Maps a 3D coordinate to the chunk buffer array pos */
export template<int W, int H>
[[nodiscard]]
int IndexMap3d2(int x, int y, int z)
{
  return x + (y * W) + (z * W * H);
}

/* Maps a ChunkCoord to the chunk buffer array pos */
export template<int W, int H>
[[nodiscard]]
int IndexMap3d2(const UnitChunkBuffer::ChunkCoord& coord)
{
  return IndexMap3d2<W, H>(coord.x, coord.y, coord.z);
}

/* Checks a coordinate (is invalid if out of bounds in the chunk buffer) */
export bool IndexVal(const int& x, int max);

} // namespace Rl::World::Chunk