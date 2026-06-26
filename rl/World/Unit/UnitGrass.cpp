import Rl.World.Unit.UnitGrass;
import Rl.World.Unit;

namespace Rl::World
{

UnitGrass::UnitGrass() noexcept :
    IUnit(IUnitIdentifiable::GetClassId()), IUnitGrowable(), IUnitIdentifiable()
{
}

bool UnitGrass::InGrowState()
{
  return true;
}

void UnitGrass::Grow()
{
}

} // namespace Rl::World
