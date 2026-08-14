#ifndef RL_WORLD_FRAGMENT_TIME_SYSTEM_H
#define RL_WORLD_FRAGMENT_TIME_SYSTEM_H

#include <atomic>
#include <cstdint>

namespace rl
{

/** Number of time fragments per second */
constexpr int64_t FRAGMENTS_PER_SECOND = 100;
/** Duration of a day in seconds */
constexpr int64_t DAY_DURATION_SECONDS = 2000;
/** Duration of a night in seconds */
constexpr int64_t NIGHT_DURATION_SECONDS   = 1800;
constexpr int64_t DAY_DURATION_FRAGMENTS   = DAY_DURATION_SECONDS * FRAGMENTS_PER_SECOND;
constexpr int64_t NIGHT_DURATION_FRAGMENTS = NIGHT_DURATION_SECONDS * FRAGMENTS_PER_SECOND;
constexpr int64_t FULL_CYCLE_FRAGMENTS     = DAY_DURATION_FRAGMENTS + NIGHT_DURATION_FRAGMENTS;

/** Represents the time of day */
enum class TimeOfDay : uint8_t
{
  DAWN  = 0, /**< Dawn period */
  DAY   = 1, /**< Day period */
  DUSK  = 2, /**< Dusk period */
  NIGHT = 3 /**< Night period */
};
/** Represents the current state of the fragment time system */
struct FragmentTimeState
{
    int64_t currentFragment; /**< Current fragment within the day-night cycle */
    int64_t totalElapsedFragments; /**< Total fragments elapsed since system
                                      start */
    TimeOfDay timeOfDay; /**< Current time of day */
    float     dayProgress; /**< Progress through current day (0.0-1.0) */
    float     nightProgress; /**< Progress through current night (0.0-1.0) */
    uint32_t  currentDay; /**< Current day number */
    bool      isDay; /**< Whether it is currently day time */
};

/** interface for providing access to a FragmentTimeSystem instance */
class FragmentTimeSystem;
class IFragmentTimeSystemProvider
{
  public:
    virtual ~IFragmentTimeSystemProvider() = default;

    /** Returns a reference to the fragment time system */
    virtual FragmentTimeSystem& getFragmentTimeSystem() = 0;
    /** Returns a const reference to the fragment time system */
    virtual const FragmentTimeSystem& getFragmentTimeSystem() const = 0;
};

/** Manages time using a fragment-based system with day-night cycles */
class FragmentTimeSystem
{
  public:
    /** Constructs a new fragment time system initialized to fragment 0 */
    FragmentTimeSystem();
    /** Default destructor */
    ~FragmentTimeSystem() = default;

    FragmentTimeSystem(const FragmentTimeSystem&)            = delete;
    FragmentTimeSystem& operator=(const FragmentTimeSystem&) = delete;

    FragmentTimeSystem(FragmentTimeSystem&& other) noexcept            = default;
    FragmentTimeSystem& operator=(FragmentTimeSystem&& other) noexcept = default;

    /** Advances the time by the specified number of fragments
     * @param fragmentsToAdd The number of fragments to add to the current
     * time */
    void updateTime(int64_t fragmentsToAdd);

    /** Returns the current time state
     * @return FragmentTimeState containing all current time information */
    [[nodiscard]]
    FragmentTimeState getTimeState() const;

    /** Returns the current fragment within the day-night cycle
     * @return Current fragment number (0 to FULL_CYCLE_FRAGMENTS-1) */
    [[nodiscard]]
    int64_t getCurrentFragment() const;

    /** Returns the total number of fragments elapsed since system start
     * @return Total elapsed fragments */
    [[nodiscard]]
    int64_t getTotalElapsedFragments() const;

    /** Returns the current time of day
     * @return TimeOfDay enum value (DAWN, DAY, DUSK, or NIGHT) */
    [[nodiscard]]
    TimeOfDay getTimeOfDay() const;

    /** Returns whether it is currently day time
     * @return true if it is day, false otherwise */
    [[nodiscard]]
    bool isDay() const;

    /** Returns whether it is currently night time
     * @return true if it is night, false otherwise */
    [[nodiscard]]
    bool isNight() const;

    /** Returns the progress through the current day (0.0 to 1.0)
     * @return Day progress value where 0.0 is dawn and 1.0 is dusk */
    [[nodiscard]]
    float getDayProgress() const;

    /** Returns the progress through the current night (0.0 to 1.0)
     * @return Night progress value where 0.0 is dusk and 1.0 is dawn */
    [[nodiscard]]
    float getNightProgress() const;

    /** Returns the current day number
     * @return Current day number starting from 0 */
    [[nodiscard]]
    uint32_t getCurrentDay() const;

    /** Resets the time system to its initial state (fragment 0, day 0) */
    void resetSystem();

    /** Sets the current fragment to a specific value
     * @param fragment The fragment number to set (will be normalized to cycle
     * range)
     */
    void setFragment(int64_t fragment);

  private:
    std::atomic<int64_t>  currentFragment;
    std::atomic<int64_t>  totalElapsedFragments;
    std::atomic<uint32_t> currentDay;

    /** Calculates the time of day for a given fragment
     * @param fragment The fragment to calculate time of day for
     * @return TimeOfDay enum value */
    [[nodiscard]]
    TimeOfDay timeOfDay(int64_t fragment) const;

    /** Calculates the day progress for a given fragment
     * @param fragment The fragment to calculate progress for
     * @return Day progress value (0.0-1.0) */
    [[nodiscard]]
    float dayProgress(int64_t fragment) const;

    /** Calculates the night progress for a given fragment
     * @param fragment The fragment to calculate progress for
     * @return Night progress value (0.0-1.0) */
    [[nodiscard]]
    float nightProgress(int64_t fragment) const;

    /** Normalizes a fragment to the valid cycle range
     * @param fragment The fragment to normalize
     * @return Normalized fragment in range [0, FULL_CYCLE_FRAGMENTS) */
    [[nodiscard]]
    int64_t normalizeFragment(int64_t fragment) const;
};

} // namespace rl

#endif
