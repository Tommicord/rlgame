import Rl.World.Unit.UnitAir;
import Rl.World.Unit;

namespace Rl::World
{

UnitAir::UnitAir() noexcept :
    IUnit(IUnitIdentifiable<UnitAir>::GetClassId()), IUnitIdentifiable<UnitAir>()
{
  RegisterDerived<UnitAir>(*this);
}

} // namespace Rl::World
