import Rl.World.Unit.UnitGrass;
import Rl.World.Unit.UnitGrassGrowBehavior;
import Rl.World.Chunk.UnitChunkAccessor;

import <memory>;

namespace Rl::World
{

UnitGrass::UnitGrass(const Unit::GrassGrowConfig& config) noexcept :
    IUnit(IUnitIdentifiable<UnitGrass>::GetClassId()), IUnitGrowable(),
    IUnitIdentifiable<UnitGrass>(),
    growBehavior(std::make_unique<Unit::UnitGrassGrowBehavior>(config))
{ RegisterDerived<UnitGrass>(*this); }

void UnitGrass::Update(Chunk::UnitChunkAccessor& accessor) const
{
  if (growBehavior)
  {
    growBehavior->Update(accessor);
  }
}

void UnitGrass::UpdateConfig(const Unit::GrassGrowConfig& newConfig)
{
  if (growBehavior)
  {
    growBehavior->UpdateConfig(newConfig);
  }
}

const Unit::GrassGrowConfig& UnitGrass::GetConfig() const
{
  if (growBehavior)
  {
    return growBehavior->GetConfig();
  }
  // Return default config if behavior is not initialized
  static Unit::GrassGrowConfig defaultConfig = Unit::GetGrassConfig();
  return defaultConfig;
}

bool UnitGrass::InGrowState()
{
  if (growBehavior)
  {
    // Check if grass can grow at current position
    // This would need a UnitChunkAccessor to properly check
    // For now, return true as default behavior
    return true;
  }
  return true;
}

} // namespace Rl::World
