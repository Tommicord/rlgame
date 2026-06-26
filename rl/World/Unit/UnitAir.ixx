export module Rl.World.Unit.UnitAir;

import Rl.World.Unit;
import Rl.World.Unit.UnitRegister;
import <type_traits>;

namespace Rl::World
{

export class UnitAir final : public IUnit<UnitAir>,
                             public IUnitIdentifiable<UnitAir>
{
  public:
  UnitAir() noexcept;
};

} // namespace Rl::World
