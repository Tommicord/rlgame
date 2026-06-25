import Rl.World.Unit.UnitGrass;
import Rl.World.Unit.Unit;

namespace Rl::World
{

UnitGrass::UnitGrass() noexcept : BaseUnit(this)
{
}

bool UnitGrass::InGrowState()
{
  return true;
}

void UnitGrass::Grow()
{
}

// Explicit template instantiation
template BaseUnit::BaseUnit(UnitGrass*) noexcept;

} // namespace Rl::World
