export module Rl.World.Unit.UnitGrassGrowBehavior;

import Rl.World.Chunk.UnitChunkAccessor;
import Rl.World.Time.TimeSystem;
import Rl.World.ServiceLocator;

import <cstdint>;
import <random>;

namespace Rl::World::Unit
{

/* Configuration for grass growth behavior */
export struct GrassGrowConfig
{
    /* Unit ID for grass blocks */
    uint32_t grassUnitId;

    /* Unit ID for dirt blocks */
    uint32_t dirtUnitId;

    /* Unit ID for air blocks */
    uint32_t airUnitId;

    /* Maximum height grass can grow */
    uint32_t maxGrowthHeight;

    /* Probability of growth per tick (0.0-1.0) */
    float growthProbability = 0.15f;

    /* Probability of spreading to adjacent blocks */
    float spreadProbability = 0.2f;

    /* Whether grass requires light to grow */
    bool requireLight = true;

    /*
     * Minimum fragments between growth attempts (default 6 seconds = 600 fragments)
     */
    int64_t growthIntervalFragments = 600;

    /* Only grow during day time */
    bool growOnlyDuringDay = true;
};

/* Gets the Grass config singleton */
export GrassGrowConfig GetGrassConfig()
{
    static GrassGrowConfig config;
    return config;
}

/* Grass growth behavior for units */
export class UnitGrassGrowBehavior
{
public:
    explicit UnitGrassGrowBehavior(const GrassGrowConfig& config);
    ~UnitGrassGrowBehavior() = default;

    /* Disable copy operations */
    UnitGrassGrowBehavior(const UnitGrassGrowBehavior&)            = delete;
    UnitGrassGrowBehavior& operator=(const UnitGrassGrowBehavior&) = delete;

    /* Enable move operations */
    UnitGrassGrowBehavior(UnitGrassGrowBehavior&& other) noexcept            = default;
    UnitGrassGrowBehavior& operator=(UnitGrassGrowBehavior&& other) noexcept = default;

    /* Update grass growth for a unit at the given position */
    void Update(Chunk::UnitChunkAccessor& accessor);

    /* Try to grow grass upward */
    [[nodiscard]]
    bool TryGrowUpward(Chunk::UnitChunkAccessor& accessor);

    /* Try to spread grass to adjacent blocks */
    [[nodiscard]]
    bool TrySpread(Chunk::UnitChunkAccessor& accessor);

    /* Check if grass can grow at current position */
    [[nodiscard]]
    bool CanGrow(Chunk::UnitChunkAccessor& accessor) const;

    /* Get current growth height */
    [[nodiscard]]
    uint32_t GetCurrentHeight(Chunk::UnitChunkAccessor& accessor) const;

    /* Update configuration */
    void UpdateConfig(const GrassGrowConfig& newConfig);

    /* Get current configuration */
    [[nodiscard]]
    const GrassGrowConfig& GetConfig() const;

    /* Get last growth fragment time */
    [[nodiscard]]
    int64_t GetLastGrowthFragment() const;

    /* Get elapsed fragments time */
    [[nodiscard]]
    int64_t GetTimeFragment();

private:
    GrassGrowConfig config;
    std::mt19937    rng; // Random number generator
    int64_t         lastGrowthFragment; // Fragment count of last growth attempt

    /* Check random probability */
    [[nodiscard]]
    bool CheckProbability(float probability);

    /* Check if enough time has passed for growth */
    [[nodiscard]]
    bool HasEnoughTimePassed() const;

    /* Calculate growth probability based on time */
    [[nodiscard]]
    float CalculateTimeBasedProbability() const;

    /* Check if block at relative position is dirt */
    [[nodiscard]]
    bool IsDirt(Chunk::UnitChunkAccessor&    accessor,
                const Chunk::RelativeOffset& offset) const;

    /* Check if block at relative position is grass */
    [[nodiscard]]
    bool IsGrass(Chunk::UnitChunkAccessor&    accessor,
                 const Chunk::RelativeOffset& offset) const;

    /* Check if block at relative position is air */
    [[nodiscard]]
    bool IsAir(Chunk::UnitChunkAccessor&    accessor,
               const Chunk::RelativeOffset& offset) const;

    /* Set block at relative position to grass */
    [[nodiscard]]
    bool SetGrass(Chunk::UnitChunkAccessor&    accessor,
                  const Chunk::RelativeOffset& offset);

    /* Set block at relative position to air */
    [[nodiscard]]
    bool SetAir(Chunk::UnitChunkAccessor& accessor, const Chunk::RelativeOffset& offset);
};

} // namespace Rl::World::Unit
