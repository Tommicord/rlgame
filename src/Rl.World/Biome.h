#ifndef RL_WORLD_BIOME_H
#define RL_WORLD_BIOME_H

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "Rl.World/PreBiomeRegister.h"
#include "Rl.World/PreBiomeRegistry.h"
#include "Rl.World/PreUnitRegistry.h"
#include "Rl.World/Unit.h"

namespace rl
{

using PreBiomeSpawnables        = std::vector<std::pair<PreUnit::IType, PreUnit*>>;
using PreBiomeSpawnableRegistry = PreUnitRegistry;

/** Max size for spawnables in the PreBiome class */
constexpr size_t biomeMaxSpawnablesCount = 256;

class PreUnitRegistry;
class PreBiome
{
  public:
    using IType = uint32_t;
    using HType = uint64_t;

    /** Environmental specifications for the biome */
    struct Prop;

    /** Constructs a PreBiome with specifications
     * @param biomeRegister The biome register containing type information
     * @param unitRegistry Reference to the unit registry to select units from
     * @param biomeSpecs Environmental specifications for this biome */
    PreBiome(PreBiomeRegister biomeRegister, PreUnitRegistry& unitRegistry) noexcept;
    PreBiome(const PreBiome& other)            = delete;
    PreBiome& operator=(const PreBiome& other) = delete;

    void setStartTemperature(float value);
    void setEndTemperature(float value);
    void setStartMoisture(float value);
    void setEndMoisture(float value);
    void setStartEquator(float value);
    void setEndEquator(float value);
    void setStartElevation(float value);
    void setEndElevation(float value);

    float getStartTemperature() const;
    float getEndTemperature() const;
    float getStartMoisture() const;
    float getEndMoisture() const;
    float getStartEquator() const;
    float getEndEquator() const;
    float getStartElevation() const;
    float getEndElevation() const;

    /** Returns the hash identifier
     * @return Hash */
    HType getHash();

    /** Returns the unique type identifier
     * @return Type ID */
    IType getTypeId() const;

    /** Select and register appropriate units from the unit registry for this
     * biome The algorithm evaluates each unit in the registry and selects
     * those that can generate within the biome's environmental specifications
     */
    void unitsFromRegistry();

    /** Get the number of spawnable units in this biome
     * @return Number of units that can spawn in this biome */
    size_t getSpawnableCount() const;

    /** Get all spawnable units
     * @return Reference to the vector of spawnable unit pairs (type ID, unit
     * pointer)
     */
    const PreBiomeSpawnables& getSpawnables() const;

    /** Check if a specific unit type can spawn in this biome
     * @param unitId The type ID of the unit to check
     * @return True if the unit can spawn in this biome, false otherwise */
    bool canSpawnUnit(PreUnit::IType unitId) const;

    /** Validates if the biome's specifications are within acceptable ranges
     * @return True if all specifications are valid, false otherwise */
    bool validateSpecs() const;
    /** Clamps temperature to valid range
     * @param temperature Temperature value to clamp
     * @return Clamped temperature value */
    static float clampTemperature(float temperature);
    /** Clamps moisture to valid range
     * @param moisture Moisture value to clamp
     * @return Clamped moisture value */
    static float clampMoisture(float moisture);
    /** Clamps equator distance to valid range
     * @param equator Equator distance value to clamp
     * @return Clamped equator distance value */
    static float clampEquator(float equator);
    /** Clamps elevation to valid range
     * @param elevation Elevation value to clamp
     * @return Clamped elevation value */
    static float clampElevation(float elevation);
    /** Checks if environmental conditions match biome specifications
     * @param temperature Temperature to check
     * @param moisture Moisture to check
     * @param equator Equator distance to check
     * @param elevation Elevation to check
     * @return True if conditions match biome specifications */
    bool matchesConditions(float temperature, float moisture, float equator, float elevation) const;
    /** Returns a string representation of the biome
     * @return String representation */
    std::string toString() const;

  private:
    /** Attempt to append a unit to the spawnables list if it matches biome
     * specifications
     * @param unit The unit to evaluate and potentially add */
    void tryAppendUnit(PreUnit* pUnit);

    /** Check if a unit's environmental ranges overlap with the biome's
     * specifications
     * @param unit The unit to check for overlap
     * @return True if the unit's ranges overlap with biome ranges, false
     * otherwise */
    void checkOverlappingEmplace(PreUnit* pUnit);

    /** Check if two ranges overlap */
    bool rangesOverlap(float start1, float end1, float start2, float end2);

    PreBiomeSpawnableRegistry* pSpawnableRegistry;
    PreBiomeSpawnables         spawnables;
    std::unique_ptr<Prop>      prop;
};

struct PreBiome::Prop
{
    float startTemperature; /**< Minimum temperature for this
                                    biome */
    float endTemperature; /**< Maximum temperature for this
                                    biome */
    float startMoisture; /**< Minimum moisture for this biome */
    float endMoisture; /**< Maximum moisture for this biome */
    float startEquator; /**< Minimum equator distance for this
                            biome */
    float endEquator; /**< Maximum equator distance for this
                            biome */
    float startElevation; /**< Minimum elevation for this
                            biome */
    float    endElevation; /**< Maximum elevation for this biome */
    uint32_t typeId;
    uint64_t hash;
};

} // namespace rl

#endif // RL_WORLD_BIOME_H
