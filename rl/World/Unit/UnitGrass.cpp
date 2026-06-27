import Rl.World.Unit.UnitGrass;
import Rl.World.Unit;

namespace Rl::World
{

UnitGrass::UnitGrass() noexcept :
    IUnit(IUnitIdentifiable<UnitGrass>::GetClassId()), IUnitGrowable(), IUnitIdentifiable<UnitGrass>()
{
  RegisterDerived<UnitGrass>(*this);
}

bool UnitGrass::InGrowState()
{
  return true;
}

void UnitGrass::Grow()
{
}

} // namespace Rl::World
