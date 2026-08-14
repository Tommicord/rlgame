#include "Rl.World/FragmentTime.h"

namespace rl
{

FragmentTimeSystem::FragmentTimeSystem() :
    currentFragment(0), totalElapsedFragments(0), currentDay(0)
{
}

void FragmentTimeSystem::updateTime(int64_t fragmentsToAdd)
{
  int64_t oldFragment = currentFragment.load(std::memory_order_acquire);
  int64_t newFragment = oldFragment + fragmentsToAdd;
  int64_t newTotal    = totalElapsedFragments.load(std::memory_order_acquire) + fragmentsToAdd;

  // Check if completed a full cycle
  if (newFragment >= FULL_CYCLE_FRAGMENTS)
  {
    newFragment = newFragment % FULL_CYCLE_FRAGMENTS;
    currentDay.fetch_add(1, std::memory_order_release);
  }

  currentFragment.store(newFragment, std::memory_order_release);
  totalElapsedFragments.store(newTotal, std::memory_order_release);
}

FragmentTimeState FragmentTimeSystem::getTimeState() const
{
  FragmentTimeState state;
  state.currentFragment       = currentFragment.load(std::memory_order_acquire);
  state.totalElapsedFragments = totalElapsedFragments.load(std::memory_order_acquire);
  state.timeOfDay             = timeOfDay(state.currentFragment);
  state.dayProgress           = dayProgress(state.currentFragment);
  state.nightProgress         = nightProgress(state.currentFragment);
  state.currentDay            = currentDay.load(std::memory_order_acquire);
  state.isDay                 = isDay();
  return state;
}

int64_t FragmentTimeSystem::getCurrentFragment() const
{
  return currentFragment.load(std::memory_order_acquire);
}

int64_t FragmentTimeSystem::getTotalElapsedFragments() const
{
  return totalElapsedFragments.load(std::memory_order_acquire);
}

TimeOfDay FragmentTimeSystem::getTimeOfDay() const
{
  return timeOfDay(currentFragment.load(std::memory_order_acquire));
}

bool FragmentTimeSystem::isDay() const
{
  int64_t fragment = currentFragment.load(std::memory_order_acquire);
  return fragment < DAY_DURATION_FRAGMENTS;
}

bool FragmentTimeSystem::isNight() const
{
  return !isDay();
}

float FragmentTimeSystem::getDayProgress() const
{
  return dayProgress(currentFragment.load(std::memory_order_acquire));
}

float FragmentTimeSystem::getNightProgress() const
{
  return nightProgress(currentFragment.load(std::memory_order_acquire));
}

uint32_t FragmentTimeSystem::getCurrentDay() const
{
  return currentDay.load(std::memory_order_acquire);
}

void FragmentTimeSystem::resetSystem()
{
  currentFragment.store(0, std::memory_order_release);
  totalElapsedFragments.store(0, std::memory_order_release);
  currentDay.store(0, std::memory_order_release);
}

void FragmentTimeSystem::setFragment(int64_t fragment)
{
  int64_t normalized = normalizeFragment(fragment);
  currentFragment.store(normalized, std::memory_order_release);
}

TimeOfDay FragmentTimeSystem::timeOfDay(int64_t fragment) const
{
  if (fragment < DAY_DURATION_FRAGMENTS * 0.1f) // First 10% of day
    return TimeOfDay::DAWN;
  else if (fragment < DAY_DURATION_FRAGMENTS * 0.9f) // 10% to 90% of day
    return TimeOfDay::DAY;
  else if (fragment < DAY_DURATION_FRAGMENTS) // Last 10% of day
    return TimeOfDay::DUSK;
  else // Night time
    return TimeOfDay::NIGHT;
}

float FragmentTimeSystem::dayProgress(int64_t fragment) const
{
  if (fragment >= DAY_DURATION_FRAGMENTS)
    return 0.0f; // Not in day time

  return static_cast<float>(fragment) / static_cast<float>(DAY_DURATION_FRAGMENTS);
}

float FragmentTimeSystem::nightProgress(int64_t fragment) const
{
  if (fragment < DAY_DURATION_FRAGMENTS)
    return 0.0f;
  int64_t nightFragment = fragment - DAY_DURATION_FRAGMENTS;
  return static_cast<float>(nightFragment) / static_cast<float>(NIGHT_DURATION_FRAGMENTS);
}

int64_t FragmentTimeSystem::normalizeFragment(int64_t fragment) const
{
  if (fragment < 0)
    fragment = 0;

  return fragment % FULL_CYCLE_FRAGMENTS;
}

} // namespace rl
