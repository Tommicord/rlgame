#include "Rl.Chunk/ChunkBuffer.h"
#include "Rl.World/Unit.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <tuple>
#include <vector>

namespace rl
{

template <typename IdType>
IdType ChunkBuffer<IdType>::Heap::fetchXYZ(size_t x, size_t y, size_t z) const noexcept
{
  return pHeapBase[x + (y * wSize) + (z * wSize * hSize)];
}

template <typename IdType>
void ChunkBuffer<IdType>::Heap::setXYZ(size_t x, size_t y, size_t z, IdType value) noexcept
{
  pHeapBase[x + (y * wSize) + (z * wSize * hSize)] = value;
}

template <typename IdType>
std::optional<IdType> ChunkBuffer<IdType>::Heap::fetchIndex(size_t index) const noexcept
{
  if (!isValidIndex(index))
  {
    return std::nullopt;
  }
  return pHeapBase[index];
}

template <typename IdType>
std::vector<IdType> ChunkBuffer<IdType>::Heap::fetchBlock(size_t index, size_t count) noexcept
{
  std::vector<IdType> result;
  if (!isValidIndex(index))
  {
    return result;
  }

  size_t available = getElementCount() - index;
  size_t toCopy    = (count < available) ? count : available;

  result.reserve(toCopy);
  const IdType* it = begin() + index;
  for (size_t i = 0; i < toCopy; ++i)
  {
    result.push_back(it[i]);
  }
  return result;
}

template <typename IdType>
bool ChunkBuffer<IdType>::Heap::isValidXYZ(size_t x, size_t y, size_t z) const noexcept
{
  return (x < wSize && y < hSize && z < dSize);
}

template <typename IdType> bool ChunkBuffer<IdType>::Heap::isValidIndex(size_t index) const noexcept
{
  return (index < getElementCount());
}

template <typename IdType> size_t ChunkBuffer<IdType>::Heap::getSizeInBytes() const noexcept
{
  return getElementCount() * sizeof(IdType);
}

template <typename IdType> size_t ChunkBuffer<IdType>::Heap::getElementCount() const noexcept
{
  return wSize * hSize * dSize;
}

template <typename IdType>
std::tuple<size_t, size_t, size_t> ChunkBuffer<IdType>::Heap::getDimensions() const noexcept
{
  return std::make_tuple(wSize, hSize, dSize);
}

template <typename IdType> void ChunkBuffer<IdType>::Heap::fill(IdType value) noexcept
{
  std::fill(begin(), end(), value);
}

template <typename IdType>
void ChunkBuffer<IdType>::Heap::copyFrom(const IdType* source, size_t count) noexcept
{
  if (source == nullptr || count == 0)
  {
    return;
  }
  size_t toCopy = (count < getElementCount()) ? count : getElementCount();
  memcpy(pHeapBase, source, toCopy * sizeof(IdType));
}

template <typename IdType>
void ChunkBuffer<IdType>::Heap::copyTo(IdType* destination, size_t count) const noexcept
{
  if (destination == nullptr || count == 0)
  {
    return;
  }
  size_t toCopy = (count < getElementCount()) ? count : getElementCount();
  memcpy(destination, pHeapBase, toCopy * sizeof(IdType));
}

template struct ChunkBuffer<PreUnit::IType>::Heap;

} // namespace rl
