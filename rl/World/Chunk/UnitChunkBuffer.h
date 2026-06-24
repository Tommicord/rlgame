#pragma once

#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>

namespace Rl::World
{

/* Chunk Storage data */
class UnitChunkBuffer
{
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
    std::unique_ptr<int> b;

    /* Gets the Unit buffer */
    int *Get() const noexcept
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

  public:

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
  std::optional<int> GetUnitIdXYZ(int x, int y, int z) const;

  /* Gets a Unit block id using ChunkCoord */
  [[nodiscard]]
  std::optional<int> GetUnitId(const ChunkCoord& coord) const;

  /* Checks if a coordinate is within chunk bounds */
  [[nodiscard]]
  bool IsInBounds(int x, int y, int z) const;

  /* Checks if a ChunkCoord is within chunk bounds */
  [[nodiscard]]
  bool IsInBounds(const ChunkCoord& coord) const;

  /* Gets the total number of blocks in the chunk */
  [[nodiscard]]
  constexpr int GetTotalBlocks() const;

  /* Gets the raw buffer pointer (for GPU transfer) */
  [[nodiscard]]
  int* GetRaw();

  /* Gets the raw buffer pointer (const version) */
  [[nodiscard]]
  const int* GetRaw() const;

  /* Gets the buffer size in bytes */
  [[nodiscard]]
  constexpr size_t GetBufferSizeBytes() const;

  /* Checks if the buffer is valid (allocated) */
  [[nodiscard]]
  bool IsValid() const;

  /* Gets chunk dimensions as ChunkCoord */
  [[nodiscard]]
  ChunkCoord GetDimensions() const;

protected:
  /* Maps a 3D coordinate to the chunk buffer array pos */
  [[nodiscard]]
  int IndexMap3d2(int x, int y, int z) const;

  /* Maps a ChunkCoord to the chunk buffer array pos */
  [[nodiscard]]
  int IndexMap3d2(const ChunkCoord& coord) const;

  /* Checks a coordinate (is invalid if out of bounds in the chunk buffer) */
  [[nodiscard]]
  bool IndexVal(int& x, int max) const;
};

} // namespace Rl::World
