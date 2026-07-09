export module Rl.World.Unit.UnitMantle;

import Rl.World.Unit;
import Rl.World.Unit.UnitRegister;
import Rl.World.Chunk.UnitChunkAccessor;
import Rl.World.Unit.UnitGrassGrowBehavior;

import <type_traits>;
import <string_view>;
import <memory>;

namespace Rl::World
{

export class UnitDeepMantle final : public IUnit,
                                    public IUnitIdentifiable<UnitDeepMantle>
{
  public:
  explicit UnitDeepMantle() noexcept :
      IUnit(IUnitIdentifiable<UnitDeepMantle>::GetClassId()),
      IUnitIdentifiable<UnitDeepMantle>()
  { RegisterDerived<UnitDeepMantle>(*this); }

  ~UnitDeepMantle() override = default;

  /* Disable copy operations */
  UnitDeepMantle(const UnitDeepMantle&) = delete;
  UnitDeepMantle& operator=(const UnitDeepMantle&) = delete;

  /* Enable move operations */
  UnitDeepMantle(UnitDeepMantle&&) noexcept = delete;
  UnitDeepMantle& operator=(UnitDeepMantle&&) noexcept = delete;

  /* Update grass growth behavior */
  void Update(Chunk::UnitChunkAccessor& accessor) const;

  /* Get current growth configuration */
  [[nodiscard]]
  const Unit::GrassGrowConfig& GetConfig() const;

  protected:
  [[nodiscard]]
  unsigned short GetDerivedClassId() const override
  { return IUnitIdentifiable<UnitDeepMantle>::GetClassId(); }

  [[nodiscard]]
  std::string_view GetDerivedClassName() const override
  { return IUnitIdentifiable<UnitDeepMantle>::SimpleClassName(); }
};

export class UnitMantle final : public IUnit,
                                public IUnitIdentifiable<UnitMantle>
{
  public:
  explicit UnitMantle() noexcept :
      IUnit(IUnitIdentifiable<UnitMantle>::GetClassId()), IUnitIdentifiable<UnitMantle>()
  { RegisterDerived<UnitMantle>(*this); }

  ~UnitMantle() override = default;

  /* Disable copy operations */
  UnitMantle(const UnitMantle&) = delete;
  UnitMantle& operator=(const UnitMantle&) = delete;

  /* Enable move operations */
  UnitMantle(UnitMantle&&) noexcept = delete;
  UnitMantle& operator=(UnitMantle&&) noexcept = delete;

  /* Update grass growth behavior */
  void Update(Chunk::UnitChunkAccessor& accessor) const;

  /* Get current growth configuration */
  [[nodiscard]]
  const Unit::GrassGrowConfig& GetConfig() const;

  protected:
  [[nodiscard]]
  unsigned short GetDerivedClassId() const override
  { return IUnitIdentifiable<UnitMantle>::GetClassId(); }

  [[nodiscard]]
  std::string_view GetDerivedClassName() const override
  { return IUnitIdentifiable<UnitMantle>::SimpleClassName(); }
};

} // namespace Rl::World
