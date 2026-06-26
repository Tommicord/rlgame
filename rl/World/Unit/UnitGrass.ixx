export module Rl.World.Unit.UnitGrass;

import Rl.World.Unit;
import Rl.World.Unit.UnitRegister;
import <type_traits>;

namespace Rl::World
{

export class IUnitGrowable
{
  public:
  /* Destructs a IUnitGrowable object */
  virtual ~IUnitGrowable() = default;

  /* Returns if the Grass unit can grow */
  virtual bool InGrowState() = 0;

  /* Returns if the Grass unit can grow */
  virtual void Grow() = 0;
};

export class UnitGrass final : public IUnit<UnitGrass>,
                               public IUnitGrowable,
                               public IUnitIdentifiable<UnitGrass>
{
  public:
  UnitGrass() noexcept;

  protected:
  bool InGrowState() override;
  void Grow() override;
};

} // namespace Rl::World
