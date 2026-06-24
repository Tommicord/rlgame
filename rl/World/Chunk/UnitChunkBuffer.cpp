#include "rl/World/Chunk/UnitChunkBuffer.h"

#include <cstring>

namespace Rl::World
{

UnitChunkBuffer::UnitChunkBuffer() noexcept
{
  buffer.b = std::make_unique<int>(W * H * D);
  std::memset(buffer.Get(), 0, sizeof(int) * W * H * D);
}

UnitChunkBuffer::~UnitChunkBuffer()
{
  if (owner && buffer.b)
  {
    buffer.Delete();
  }
}

UnitChunkBuffer::UnitChunkBuffer(const UnitChunkBuffer& other)
{
  buffer.b = std::make_unique<int>(W * H * D);
  std::memcpy(buffer.Get(), other.buffer.Get(), sizeof(int) * W * H * D);
}

UnitChunkBuffer::UnitChunkBuffer(UnitChunkBuffer&& other) noexcept :
    bufferSizes(other.bufferSizes), buffer(std::move(other.buffer)), owner(other.owner)
{
  other.buffer.b = nullptr;
  other.owner    = false;
}

UnitChunkBuffer& UnitChunkBuffer::operator=(const UnitChunkBuffer& other)
{
  if (this == &other)
    return *this;

  if (owner && buffer.b)
  {
    buffer.Delete();
  }
  buffer.b = std::make_unique<int>(W * H * D);
  std::memcpy(buffer.Get(), other.buffer.Get(), sizeof(int) * W * H * D);
  owner = true;
  return *this;
}

UnitChunkBuffer& UnitChunkBuffer::operator=(UnitChunkBuffer&& other) noexcept
{
  if (this == &other)
    return *this;

  if (owner && buffer.b)
  {
    buffer.b.release();
  }
  bufferSizes = other.bufferSizes;
  buffer      = std::move(other.buffer);
  owner       = other.owner;

  other.buffer.b = nullptr;
  other.owner    = false;

  return *this;
}

void UnitChunkBuffer::Clear()
{
  if (IsValid())
  {
    std::memset(buffer.b.get(), 0, sizeof(int) * W * H * D);
  }
}

std::optional<int> UnitChunkBuffer::GetUnitIdXYZ(int x, int y, int z) const
{
  if (!buffer.Get())
    return std::nullopt;
  if (IndexVal(x, W) && IndexVal(y, H) && IndexVal(z, D))
  {
    return buffer.Get()[IndexMap3d2(x, y, z)];
  }
  return std::nullopt;
}

std::optional<int> UnitChunkBuffer::GetUnitId(const ChunkCoord& coord) const
{
  return GetUnitIdXYZ(coord.x, coord.y, coord.z);
}

bool UnitChunkBuffer::IsInBounds(int x, int y, int z) const
{
  return IndexVal(x, W) && IndexVal(y, H) && IndexVal(z, D);
}

bool UnitChunkBuffer::IsInBounds(const ChunkCoord& coord) const
{
  return IsInBounds(coord.x, coord.y, coord.z);
}

constexpr int UnitChunkBuffer::GetTotalBlocks() const
{
  return W * H * D;
}

int* UnitChunkBuffer::GetRaw()
{
  return buffer.b.get();
}

const int* UnitChunkBuffer::GetRaw() const
{
  return buffer.b.get();
}

constexpr size_t UnitChunkBuffer::GetBufferSizeBytes() const
{
  return static_cast<size_t>(W * H * D) * sizeof(int);
}

bool UnitChunkBuffer::IsValid() const
{
  return !!buffer.b;
}

UnitChunkBuffer::ChunkCoord UnitChunkBuffer::GetDimensions() const
{
  return {W, H, D};
}

int UnitChunkBuffer::IndexMap3d2(int x, int y, int z) const
{
  return x + (y * W) + (z * W * H);
}

int UnitChunkBuffer::IndexMap3d2(const ChunkCoord& coord) const
{
  return IndexMap3d2(coord.x, coord.y, coord.z);
}

bool UnitChunkBuffer::IndexVal(int& x, int max) const
{
  return x >= 0 && x < max;
}

} // namespace Rl::World
