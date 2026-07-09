export module Rl.World.Time.TimeSystem;

import <cstdint>;
import <atomic>;

namespace Rl::World::Time
{

/* Time constants */
export constexpr int64_t FRAGMENTS_PER_SECOND = 100;
export constexpr int64_t DAY_DURATION_SECONDS = 1200;
export constexpr int64_t NIGHT_DURATION_SECONDS = 1100;
export constexpr int64_t DAY_DURATION_FRAGMENTS =
    DAY_DURATION_SECONDS * FRAGMENTS_PER_SECOND; // 120000
export constexpr int64_t NIGHT_DURATION_FRAGMENTS =
    NIGHT_DURATION_SECONDS * FRAGMENTS_PER_SECOND; // 110000
export constexpr int64_t FULL_CYCLE_FRAGMENTS =
    DAY_DURATION_FRAGMENTS + NIGHT_DURATION_FRAGMENTS; // 230000

/* Time of day enum */
export enum class TimeOfDay : uint8_t { DAWN = 0, DAY = 1, DUSK = 2, NIGHT = 3 };

/* Time state information */
export struct TimeState
{
  int64_t   currentFragment; // Current fragment in the cycle
  int64_t   totalElapsedFragments; // Total fragments elapsed since system start
  TimeOfDay timeOfDay; // Current time of day
  float     dayProgress; // Progress through current day (0.0-1.0)
  float     nightProgress; // Progress through current night (0.0-1.0)
  uint32_t  currentDay; // Current day number
  bool      isDay; // Whether it's currently day time
};

/* Time System for fragment-based time tracking */
export class TimeSystem
{
  public:
  TimeSystem();
  ~TimeSystem() = default;

  /* Disable copy operations */
  TimeSystem(const TimeSystem&) = delete;
  TimeSystem& operator=(const TimeSystem&) = delete;

  /* Enable move operations */
  TimeSystem(TimeSystem&& other) noexcept = default;
  TimeSystem& operator=(TimeSystem&& other) noexcept = default;

  /* Update time by adding fragments */
  void Update(int64_t fragmentsToAdd);

  /* Get current time state */
  [[nodiscard]]
  TimeState GetTimeState() const;

  /* Get current fragment in cycle */
  [[nodiscard]]
  int64_t GetCurrentFragment() const;

  /* Get total elapsed fragments */
  [[nodiscard]]
  int64_t GetTotalElapsedFragments() const;

  /* Get current time of day */
  [[nodiscard]]
  TimeOfDay GetTimeOfDay() const;

  /* Check if it's currently day */
  [[nodiscard]]
  bool IsDay() const;

  /* Check if it's currently night */
  [[nodiscard]]
  bool IsNight() const;

  /* Get progress through current day (0.0-1.0) */
  [[nodiscard]]
  float GetDayProgress() const;

  /* Get progress through current night (0.0-1.0) */
  [[nodiscard]]
  float GetNightProgress() const;

  /* Get current day number */
  [[nodiscard]]
  uint32_t GetCurrentDay() const;

  /* Reset time system */
  void Reset();

  /* Set time to specific fragment in cycle */
  void SetFragment(int64_t fragment);

  private:
  std::atomic<int64_t>  currentFragment;
  std::atomic<int64_t>  totalElapsedFragments;
  std::atomic<uint32_t> currentDay;

  /* Calculate time of day from fragment */
  [[nodiscard]]
  TimeOfDay CalculateTimeOfDay(int64_t fragment) const;

  /* Calculate day progress (0.0-1.0) */
  [[nodiscard]]
  float CalculateDayProgress(int64_t fragment) const;

  /* Calculate night progress (0.0-1.0) */
  [[nodiscard]]
  float CalculateNightProgress(int64_t fragment) const;

  /* Normalize fragment to cycle range */
  [[nodiscard]]
  int64_t NormalizeFragment(int64_t fragment) const;
};

} // namespace Rl::World::Time
