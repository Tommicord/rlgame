import Rl.World.Chunk.UnitChunkBuffer;

import <cstring>;
import <optional>;
import <memory>;

namespace Rl::World::Chunk
{

UnitChunkBuffer::UnitChunkBuffer() noexcept
{
  buffer.b = std::make_unique<int[]>(W * H * D);
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
  buffer.b = std::make_unique<int[]>(W * H * D);
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
  buffer.b = std::make_unique<int[]>(W * H * D);
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
    return buffer.b[IndexMap3d2<W, H>(x, y, z)];
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


bool UnitChunkBuffer::IsValid() const
{
  return !!buffer.b;
}

UnitChunkBuffer::ChunkCoord UnitChunkBuffer::GetDimensions() const
{
  return {W, H, D};
}

bool IndexVal(const int& x, const int max)
{
  return x >= 0 && x < max;
}

} // namespace Rl::World::Chunk
