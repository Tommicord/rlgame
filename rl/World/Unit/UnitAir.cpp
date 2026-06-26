import Rl.World.Unit.UnitAir;
import Rl.World.Unit;

namespace Rl::World
{

UnitAir::UnitAir() noexcept :
    IUnit(IUnitIdentifiable::GetClassId()), IUnitIdentifiable()
{
}

} // namespace Rl::World
