export module Rl.World.Chunk.UnitChunkAccessor;

import Rl.World.Chunk.ChunkInRenderUnits;
import Rl.World.Chunk.ChunkTransaction;
import <cstdint>;
import <memory>;

namespace Rl::World::Chunk
{

/* World position of a unit in block coordinates */
export struct UnitPosition
{
  int32_t worldX;
  int32_t worldY;
  int32_t worldZ;

  UnitPosition() : worldX(0), worldY(0), worldZ(0)
  {
  }
  UnitPosition(int32_t x, int32_t y, int32_t z) : worldX(x), worldY(y), worldZ(z)
  {
  }

  /* Equality operator */
  [[nodiscard]]
  bool operator==(const UnitPosition& other) const
  { return worldX == other.worldX && worldY == other.worldY && worldZ == other.worldZ; }

  /* Inequality operator */
  [[nodiscard]]
  bool operator!=(const UnitPosition& other) const
  { return !(*this == other); }

  /* Add relative offset */
  [[nodiscard]]
  UnitPosition operator+(const UnitPosition& offset) const
  {
    return UnitPosition(
        worldX + offset.worldX, worldY + offset.worldY, worldZ + offset.worldZ);
  }

  /* Subtract relative offset */
  [[nodiscard]]
  UnitPosition operator-(const UnitPosition& offset) const
  {
    return UnitPosition(
        worldX - offset.worldX, worldY - offset.worldY, worldZ - offset.worldZ);
  }
};

/* Relative coordinate offset from a unit */
export struct RelativeOffset
{
  int32_t offsetX;
  int32_t offsetY;
  int32_t offsetZ;

  RelativeOffset() : offsetX(0), offsetY(0), offsetZ(0)
  {
  }
  RelativeOffset(int32_t x, int32_t y, int32_t z) : offsetX(x), offsetY(y), offsetZ(z)
  {
  }

  /* Common relative positions */
  static RelativeOffset Current()
  { return {0, 0, 0}; }
  static RelativeOffset Above()
  { return {0, 1, 0}; }
  static RelativeOffset Below()
  { return {0, -1, 0}; }
  static RelativeOffset North()
  { return {0, 0, -1}; }
  static RelativeOffset South()
  { return {0, 0, 1}; }
  static RelativeOffset East()
  { return {1, 0, 0}; }
  static RelativeOffset West()
  { return {-1, 0, 0}; }

  /* Diagonal directions */
  static RelativeOffset NorthEast()
  { return {1, 0, -1}; }
  static RelativeOffset NorthWest()
  { return {-1, 0, -1}; }
  static RelativeOffset SouthEast()
  { return {1, 0, 1}; }
  static RelativeOffset SouthWest()
  { return {-1, 0, 1}; }

  /* Convert to UnitPosition */
  [[nodiscard]]
  UnitPosition ToAbsolute(const UnitPosition& base) const
  { return {base.worldX + offsetX, base.worldY + offsetY, base.worldZ + offsetZ}; }
};

/* Accessor for units to interact with chunk data using relative coordinates */
export class UnitChunkAccessor
{
  public:
  /* The max Y for a valid vertical position in the world */
  static constexpr long long MAXY = 12000000L; // 12000 KM

  /* The min Y for a valid vertical position in the world */
  static constexpr long long MINY = -4000000L; // 4000 KM

  /* Constructor with chunk system reference and unit position */
  UnitChunkAccessor(ChunkInRenderUnits& chunkSystem, const UnitPosition& unitPosition);

  /* Destructor */
  ~UnitChunkAccessor() = default;

  /* Disable copy operations */
  UnitChunkAccessor(const UnitChunkAccessor&) = delete;
  UnitChunkAccessor& operator=(const UnitChunkAccessor&) = delete;

  /* Enable move operations */
  UnitChunkAccessor(UnitChunkAccessor&& other) noexcept = default;
  UnitChunkAccessor& operator=(UnitChunkAccessor&& other) noexcept = default;

  /* Update unit position (e.g., when unit moves) */
  void UpdatePosition(const UnitPosition& newPosition);

  /* Get current unit position */
  [[nodiscard]]
  UnitPosition GetPosition() const;

  /* Read unit ID at relative offset (0,0,0 is current unit) */
  [[nodiscard]]
  TransactionResult ReadRelative(const RelativeOffset& offset);

  /* Write unit ID at relative offset (0,0,0 is current unit) */
  [[nodiscard]]
  TransactionResult WriteRelative(const RelativeOffset& offset, uint32_t newUnitId);

  /* Convenience methods for common operations */

  /* Read current unit's ID */
  [[nodiscard]]
  TransactionResult ReadCurrent();

  /* Read unit above */
  [[nodiscard]]
  TransactionResult ReadAbove();

  /* Read unit below */
  [[nodiscard]]
  TransactionResult ReadBelow();

  /* Write to unit above */
  [[nodiscard]]
  TransactionResult WriteAbove(uint32_t newUnitId);

  /* Write to unit below */
  [[nodiscard]]
  TransactionResult WriteBelow(uint32_t newUnitId);

  /* Check if relative position is within valid chunk bounds */
  [[nodiscard]]
  bool IsRelativePositionValid(const RelativeOffset& offset) const;

  /* Get absolute position for a relative offset */
  [[nodiscard]]
  UnitPosition GetAbsolutePosition(const RelativeOffset& offset) const;

  /* Check if current unit is on ground (solid block below) */
  [[nodiscard]]
  bool IsOnGround();

  /* Check if space above is empty */
  [[nodiscard]]
  bool IsSpaceAboveEmpty();

  /* Get the chunk system reference */
  [[nodiscard]]
  ChunkInRenderUnits& GetChunkSystem();

  private:
  ChunkInRenderUnits& chunkSystem;
  UnitPosition        currentPosition;

  /* Convert world position to chunk coordinates and local coordinates */
  [[nodiscard]]
  bool WorldToChunkLocal(const UnitPosition& worldPos,
      WorldChunkCoord&                       chunkCoord,
      int32_t&                               localX,
      int32_t&                               localY,
      int32_t&                               localZ) const;
};

} // namespace Rl::World::Chunk
