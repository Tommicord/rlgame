import Rl.World.Chunk.UnitChunkAccessor;

import Rl.World.Chunk.UnitChunkBuffer;
import Rl.World.Chunk.ChunkInRenderUnits;
import Rl.World.Chunk.ChunkTransaction;

import <stdexcept>;

namespace Rl::World::Chunk
{

UnitChunkAccessor::UnitChunkAccessor(
    ChunkInRenderUnits& chunkSystem, const UnitPosition& unitPosition) :
    chunkSystem(chunkSystem), currentPosition(unitPosition)
{
}

void UnitChunkAccessor::UpdatePosition(const UnitPosition& newPosition)
{ currentPosition = newPosition; }

UnitPosition UnitChunkAccessor::GetPosition() const
{ return currentPosition; }

TransactionResult UnitChunkAccessor::ReadRelative(const RelativeOffset& offset)
{
  UnitPosition absolutePos = GetAbsolutePosition(offset);

  if (!IsRelativePositionValid(offset))
  {
    return TransactionResult::Error(
        "Relative position is out of bounds", ValidationResult::INVALID_COORDINATES);
  }

  WorldChunkCoord chunkCoord{};
  int32_t         localX, localY, localZ;

  if (!WorldToChunkLocal(absolutePos, chunkCoord, localX, localY, localZ))
  {
    return TransactionResult::Error(
        "Failed to convert world position to chunk coordinates", ValidationResult::VALID);
  }

  return chunkSystem.ReadUnitId(chunkCoord, localX, localY, localZ);
}

TransactionResult UnitChunkAccessor::WriteRelative(
    const RelativeOffset& offset, uint32_t newUnitId)
{
  UnitPosition absolutePos = GetAbsolutePosition(offset);

  if (!IsRelativePositionValid(offset))
  {
    return TransactionResult::Error(
        "Relative position is out of bounds", ValidationResult::INVALID_COORDINATES);
  }

  WorldChunkCoord chunkCoord{};
  int32_t         localX, localY, localZ;

  if (!WorldToChunkLocal(absolutePos, chunkCoord, localX, localY, localZ))
  {
    return TransactionResult::Error(
        "Failed to convert world position to chunk coordinates", ValidationResult::VALID);
  }

  return chunkSystem.WriteUnitId(chunkCoord, localX, localY, localZ, newUnitId);
}

TransactionResult UnitChunkAccessor::ReadCurrent()
{ return ReadRelative(RelativeOffset::Current()); }

TransactionResult UnitChunkAccessor::ReadAbove()
{ return ReadRelative(RelativeOffset::Above()); }

TransactionResult UnitChunkAccessor::ReadBelow()
{ return ReadRelative(RelativeOffset::Below()); }

TransactionResult UnitChunkAccessor::WriteAbove(uint32_t newUnitId)
{ return WriteRelative(RelativeOffset::Above(), newUnitId); }

TransactionResult UnitChunkAccessor::WriteBelow(uint32_t newUnitId)
{ return WriteRelative(RelativeOffset::Below(), newUnitId); }

bool UnitChunkAccessor::IsRelativePositionValid(const RelativeOffset& offset) const
{
  UnitPosition absolutePos = GetAbsolutePosition(offset);
  const auto& [x, y, z] = absolutePos;

  if (y > MAXY || y < MINY)
    return false;

  WorldChunkCoord chunkCoord{};
  int32_t localX, localY, localZ;
  if (!WorldToChunkLocal(absolutePos, chunkCoord, localX, localY, localZ))
    return false;

  if (!chunkSystem.IsInRenderDistance(chunkCoord))
    return false;

  return true;
}

UnitPosition UnitChunkAccessor::GetAbsolutePosition(const RelativeOffset& offset) const
{ return offset.ToAbsolute(currentPosition); }

bool UnitChunkAccessor::IsOnGround()
{
  auto belowResult = ReadBelow();
  if (!belowResult.success)
    return false;

  // Check if the block below is solid (non-zero unit ID typically means solid)
  // This is a simplified check - you may want to check specific unit properties
  return belowResult.readUnitId != 0;
}

bool UnitChunkAccessor::IsSpaceAboveEmpty()
{
  auto aboveResult = ReadAbove();
  if (!aboveResult.success)
    return false;

  // Check if the space above is empty (unit ID 0 typically means air)
  return aboveResult.readUnitId == 0;
}

ChunkInRenderUnits& UnitChunkAccessor::GetChunkSystem()
{ return chunkSystem; }

bool UnitChunkAccessor::WorldToChunkLocal(const UnitPosition& worldPos,
    WorldChunkCoord&                                          chunkCoord,
    int32_t&                                                  localX,
    int32_t&                                                  localY,
    int32_t&                                                  localZ) const
{
  // Convert world coordinates to chunk coordinates
  // Assuming chunks are UnitChunkBuffer::W x UnitChunkBuffer::H x UnitChunkBuffer::D in
  // size
  chunkCoord.chunkX = worldPos.worldX / UnitChunkBuffer::W;
  chunkCoord.chunkY = worldPos.worldY / UnitChunkBuffer::H;
  chunkCoord.chunkZ = worldPos.worldZ / UnitChunkBuffer::D;

  // Handle negative coordinates
  if (worldPos.worldX < 0)
    chunkCoord.chunkX--;
  if (worldPos.worldY < 0)
    chunkCoord.chunkY--;
  if (worldPos.worldZ < 0)
    chunkCoord.chunkZ--;

  // Calculate local coordinates within the chunk
  localX = worldPos.worldX - (chunkCoord.chunkX * UnitChunkBuffer::W);
  localY = worldPos.worldY - (chunkCoord.chunkY * UnitChunkBuffer::H);
  localZ = worldPos.worldZ - (chunkCoord.chunkZ * UnitChunkBuffer::D);

  // Handle negative local coordinates
  if (localX < 0)
    localX += UnitChunkBuffer::W;
  if (localY < 0)
    localY += UnitChunkBuffer::H;
  if (localZ < 0)
    localZ += UnitChunkBuffer::D;

  return true;
}

} // namespace Rl::World::Chunk
