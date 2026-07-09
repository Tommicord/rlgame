import Rl.World.Unit.UnitGrassGrowBehavior;

import Rl.World.Chunk.UnitChunkAccessor;
import Rl.World.ServiceLocator;
import Rl.World.Time.TimeSystem;

import <random>;
import <algorithm>;

namespace Rl::World::Unit
{

UnitGrassGrowBehavior::UnitGrassGrowBehavior(const GrassGrowConfig& config) :
    config(config), rng(std::random_device{}()), lastGrowthFragment(0)
{
}

void UnitGrassGrowBehavior::Update(Chunk::UnitChunkAccessor& accessor)
{
  // Check if enough time has passed
  if (!HasEnoughTimePassed())
    return;
  // Check if grass can grow at current position
  if (!CanGrow(accessor))
    return;
  if (TryGrowUpward(accessor))
  {
    lastGrowthFragment = GetTimeFragment();
    return;
  }
  // If can't grow upward try to spread
  if (TrySpread(accessor))
  {
    lastGrowthFragment = GetTimeFragment();
  }
}

bool UnitGrassGrowBehavior::TryGrowUpward(Chunk::UnitChunkAccessor& accessor)
{
  // Check if we can grow at current position
  if (!IsGrass(accessor, Chunk::RelativeOffset::Current()))
    return false;

  // Check if space above is air
  if (!IsAir(accessor, Chunk::RelativeOffset::Above()))
    return false;

  // Check growth probability
  if (!CheckProbability(config.growthProbability))
    return false;

  // Get current height
  uint32_t currentHeight = GetCurrentHeight(accessor);
  if (currentHeight >= config.maxGrowthHeight)
    return false;

  // Grow grass upward
  return SetGrass(accessor, Chunk::RelativeOffset::Above());
}

bool UnitGrassGrowBehavior::TrySpread(Chunk::UnitChunkAccessor& accessor)
{
  // Check if we can spread from current position
  if (!IsGrass(accessor, Chunk::RelativeOffset::Current()))
    return false;

  // Check spread probability
  if (!CheckProbability(config.spreadProbability))
    return false;

  // Try to spread to adjacent blocks
  Chunk::RelativeOffset directions[] = {Chunk::RelativeOffset::North(),
      Chunk::RelativeOffset::South(), Chunk::RelativeOffset::East(),
      Chunk::RelativeOffset::West(), Chunk::RelativeOffset::NorthEast(),
      Chunk::RelativeOffset::NorthWest(), Chunk::RelativeOffset::SouthEast(),
      Chunk::RelativeOffset::SouthWest()};
  // Shuffle directions for random spread
  std::ranges::shuffle(directions, rng);

  for (const auto& direction : directions)
  {
    // Check if target position is dirt
    if (IsDirt(accessor, direction))
    {
      auto aboveDirt = Chunk::RelativeOffset(
          direction.offsetX, direction.offsetY + 1, direction.offsetZ);
      if (IsAir(accessor, aboveDirt))
      {
        // Spread grass to this position
        return SetGrass(accessor, direction);
      }
    }
  }

  return false;
}

bool UnitGrassGrowBehavior::CanGrow(Chunk::UnitChunkAccessor& accessor) const
{
  // Check if current position is grass
  if (!IsGrass(accessor, Chunk::RelativeOffset::Current()))
    return false;
  // Check if grass is on ground (dirt below)
  if (!IsDirt(accessor, Chunk::RelativeOffset::Below()))
    return false;
  // Check if should only grow during day
  if (config.growOnlyDuringDay)
  {
    auto timeSystem = WorldServiceLocator::GetTimeSystem();
    if (!timeSystem)
      return false;

    if (!timeSystem->IsDay())
      return false;
  }
  return true;
}

uint32_t UnitGrassGrowBehavior::GetCurrentHeight(Chunk::UnitChunkAccessor& accessor) const
{
  uint32_t              height = 0;
  Chunk::RelativeOffset current = Chunk::RelativeOffset::Current();

  // Count consecutive grass blocks going downward
  while (IsGrass(accessor, current))
  {
    height++;
    current =
        Chunk::RelativeOffset(current.offsetX, current.offsetY - 1, current.offsetZ);
    // Safety check to prevent infinite loop
    if (height > config.maxGrowthHeight * 2)
      break;
  }

  return height;
}

void UnitGrassGrowBehavior::UpdateConfig(const GrassGrowConfig& newConfig)
{ config = newConfig; }

const GrassGrowConfig& UnitGrassGrowBehavior::GetConfig() const
{ return config; }

int64_t UnitGrassGrowBehavior::GetLastGrowthFragment() const
{ return lastGrowthFragment; }

bool UnitGrassGrowBehavior::CheckProbability(float probability)
{
  std::uniform_real_distribution<float> dist(0.0f, 1.0f);
  return dist(rng) < probability;
}

bool UnitGrassGrowBehavior::HasEnoughTimePassed() const
{
  auto timeSystem = WorldServiceLocator::GetTimeSystem();
  if (!timeSystem)
    return true; // Allow growth if time system is not available

  int64_t currentFragment = timeSystem->GetTotalElapsedFragments();
  int64_t fragmentsSinceLastGrowth = currentFragment - lastGrowthFragment;

  return fragmentsSinceLastGrowth >= config.growthIntervalFragments;
}

float UnitGrassGrowBehavior::CalculateTimeBasedProbability() const
{
  auto timeSystem = WorldServiceLocator::GetTimeSystem();
  if (!timeSystem)
    return config.growthProbability; // Use default if time system is not available
  const Time::TimeState timeState = timeSystem->GetTimeState();
  if (!timeState.isDay)
  {
    // Lower probability during night
    return config.growthProbability * 0.2f;
  }
  // Higher probability during day, peak at noon
  const float dayProgress = timeState.dayProgress;

  // Probability follows a sine wave, peaking at noon (0.5 progress)
  const float timeMultiplier = std::sin(dayProgress * 3.14159f); // sin(0 to PI)

  // Scale from 0.5x to 1.5x based on time
  return config.growthProbability * (0.5f + timeMultiplier);
}

int64_t UnitGrassGrowBehavior::GetTimeFragment()
{
  auto timeSystem = WorldServiceLocator::GetTimeSystem();
  if (!timeSystem)
    return 0;

  return timeSystem->GetTotalElapsedFragments();
}

bool UnitGrassGrowBehavior::IsDirt(
    Chunk::UnitChunkAccessor& accessor, const Chunk::RelativeOffset& offset) const
{
  auto result = accessor.ReadRelative(offset);
  if (!result.success)
    return false;

  return result.readUnitId == config.dirtUnitId;
}

bool UnitGrassGrowBehavior::IsGrass(
    Chunk::UnitChunkAccessor& accessor, const Chunk::RelativeOffset& offset) const
{
  const auto result = accessor.ReadRelative(offset);
  if (!result.success)
    return false;

  return result.readUnitId == config.grassUnitId;
}

bool UnitGrassGrowBehavior::IsAir(
    Chunk::UnitChunkAccessor& accessor, const Chunk::RelativeOffset& offset) const
{
  const auto result = accessor.ReadRelative(offset);
  if (!result.success)
    return false;

  return result.readUnitId == config.airUnitId;
}

bool UnitGrassGrowBehavior::SetGrass(
    Chunk::UnitChunkAccessor& accessor, const Chunk::RelativeOffset& offset)
{
  auto result = accessor.WriteRelative(offset, config.grassUnitId);
  return result.success;
}

bool UnitGrassGrowBehavior::SetAir(
    Chunk::UnitChunkAccessor& accessor, const Chunk::RelativeOffset& offset)
{
  auto result = accessor.WriteRelative(offset, config.airUnitId);
  return result.success;
}

} // namespace Rl::World::Unit
