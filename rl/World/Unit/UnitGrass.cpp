#include "rl/World/Unit/UnitGrass.h"
#include "rl/World/Unit/Unit.h"

namespace Rl::World {

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
