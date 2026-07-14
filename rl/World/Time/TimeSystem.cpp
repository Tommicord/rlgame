import Rl.World.Time.TimeSystem;

namespace Rl::World::Time
{

TimeSystem::TimeSystem() : currentFragment(0), totalElapsedFragments(0), currentDay(0)
{
}

void TimeSystem::Update(int64_t fragmentsToAdd)
{
    if (fragmentsToAdd <= 0)
        return;

    int64_t oldFragment = currentFragment.load(std::memory_order_acquire);
    int64_t newFragment = oldFragment + fragmentsToAdd;
    int64_t newTotal =
        totalElapsedFragments.load(std::memory_order_acquire) + fragmentsToAdd;

    // Check if we've completed a full cycle
    if (newFragment >= FULL_CYCLE_FRAGMENTS)
    {
        newFragment = newFragment % FULL_CYCLE_FRAGMENTS;
        currentDay.fetch_add(1, std::memory_order_release);
    }

    currentFragment.store(newFragment, std::memory_order_release);
    totalElapsedFragments.store(newTotal, std::memory_order_release);
}

TimeState TimeSystem::GetTimeState() const
{
    TimeState state;
    state.currentFragment       = currentFragment.load(std::memory_order_acquire);
    state.totalElapsedFragments = totalElapsedFragments.load(std::memory_order_acquire);
    state.timeOfDay             = CalculateTimeOfDay(state.currentFragment);
    state.dayProgress           = CalculateDayProgress(state.currentFragment);
    state.nightProgress         = CalculateNightProgress(state.currentFragment);
    state.currentDay            = currentDay.load(std::memory_order_acquire);
    state.isDay                 = IsDay();
    return state;
}

int64_t TimeSystem::GetCurrentFragment() const
{
    return currentFragment.load(std::memory_order_acquire);
}

int64_t TimeSystem::GetTotalElapsedFragments() const
{
    return totalElapsedFragments.load(std::memory_order_acquire);
}

TimeOfDay TimeSystem::GetTimeOfDay() const
{
    return CalculateTimeOfDay(currentFragment.load(std::memory_order_acquire));
}

bool TimeSystem::IsDay() const
{
    int64_t fragment = currentFragment.load(std::memory_order_acquire);
    return fragment < DAY_DURATION_FRAGMENTS;
}

bool TimeSystem::IsNight() const
{
    return !IsDay();
}

float TimeSystem::GetDayProgress() const
{
    return CalculateDayProgress(currentFragment.load(std::memory_order_acquire));
}

float TimeSystem::GetNightProgress() const
{
    return CalculateNightProgress(currentFragment.load(std::memory_order_acquire));
}

uint32_t TimeSystem::GetCurrentDay() const
{
    return currentDay.load(std::memory_order_acquire);
}

void TimeSystem::Reset()
{
    currentFragment.store(0, std::memory_order_release);
    totalElapsedFragments.store(0, std::memory_order_release);
    currentDay.store(0, std::memory_order_release);
}

void TimeSystem::SetFragment(int64_t fragment)
{
    int64_t normalized = NormalizeFragment(fragment);
    currentFragment.store(normalized, std::memory_order_release);
}

TimeOfDay TimeSystem::CalculateTimeOfDay(int64_t fragment) const
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

float TimeSystem::CalculateDayProgress(int64_t fragment) const
{
    if (fragment >= DAY_DURATION_FRAGMENTS)
        return 0.0f; // Not in day time

    return static_cast<float>(fragment) / static_cast<float>(DAY_DURATION_FRAGMENTS);
}

float TimeSystem::CalculateNightProgress(int64_t fragment) const
{
    if (fragment < DAY_DURATION_FRAGMENTS)
        return 0.0f; // Not in night time

    int64_t nightFragment = fragment - DAY_DURATION_FRAGMENTS;
    return static_cast<float>(nightFragment) /
           static_cast<float>(NIGHT_DURATION_FRAGMENTS);
}

int64_t TimeSystem::NormalizeFragment(int64_t fragment) const
{
    if (fragment < 0)
        fragment = 0;

    return fragment % FULL_CYCLE_FRAGMENTS;
}

} // namespace Rl::World::Time
