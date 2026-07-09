export module Rl.World.Unit.UnitGrass;

import Rl.World.Unit;
import Rl.World.Unit.UnitRegister;
import Rl.World.Unit.UnitGrassGrowBehavior;
import Rl.World.Chunk.UnitChunkAccessor;
import <type_traits>;
import <string_view>;
import <memory>;
import Rl.World.Unit.UnitGrassGrowBehavior;

namespace Rl::World
{

export class IUnitGrowable
{
  public:
  /* Destructs a IUnitGrowable object */
  virtual ~IUnitGrowable() = default;

  /* Returns if the Grass unit can grow */
  virtual bool InGrowState() = 0;
};

export class UnitGrass final : public IUnit,
                               public IUnitGrowable,
                               public IUnitIdentifiable<UnitGrass>
{
  public:
  explicit UnitGrass(const Unit::GrassGrowConfig& config = Unit::GetGrassConfig()) noexcept;
  ~UnitGrass() override = default;

  /* Disable copy operations */
  UnitGrass(const UnitGrass&) = delete;
  UnitGrass& operator=(const UnitGrass&) = delete;

  /* Enable move operations */
  UnitGrass(UnitGrass&&) noexcept = default;
  UnitGrass& operator=(UnitGrass&&) noexcept = default;

  /* Update grass growth behavior */
  void Update(Chunk::UnitChunkAccessor& accessor) const;

  /* Update growth configuration */
  void UpdateConfig(const Unit::GrassGrowConfig& newConfig);

  /* Get current growth configuration */
  [[nodiscard]]
  const Unit::GrassGrowConfig& GetConfig() const;

  protected:
  bool UnitGrass::InGrowState() override;

  private:
  [[nodiscard]]
  unsigned short GetDerivedClassId() const override
  {
    return IUnitIdentifiable<UnitGrass>::GetClassId();
  }

  [[nodiscard]]
  std::string_view GetDerivedClassName() const override
  {
    return IUnitIdentifiable<UnitGrass>::SimpleClassName();
  }

  std::unique_ptr<Unit::UnitGrassGrowBehavior> growBehavior;
};

} // namespace Rl::World
