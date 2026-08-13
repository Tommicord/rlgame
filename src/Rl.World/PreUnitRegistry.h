#ifndef RL_WORLD_UNIT_REGISTRY_H
#define RL_WORLD_UNIT_REGISTRY_H

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include "Rl.World/PreRegistry.h"

namespace rl
{

class PreUnit;

/** Key used to group similar units into buckets based on environmental ranges */
struct PreUnitBucketKey
{
                float elevationRange; /**< Elevation range for the bucket */
                float moistureRange; /**< Moisture range for the bucket */
                float equatorRange; /**< Equator distance range for the bucket */
                float temperatureRange; /**< Temperature range for the bucket */

                /** Compares two bucket keys for equality
                 * @param other The other bucket key to compare with
                 * @return true if all ranges are equal, false otherwise */
                bool operator==(const PreUnitBucketKey& other) const
                {
                        return elevationRange == other.elevationRange &&
                               moistureRange == other.moistureRange &&
                               temperatureRange == other.temperatureRange &&
                               equatorRange == other.equatorRange;
                }
};

/** Hash function for PreUnitBucketKey using bitwise operations */
struct PreUnitBucketKeyHash
{
                /** Sets a hash value for a bucket key
                 * @param key The bucket key to hash
                 * @return Hash value for the key */
                size_t operator()(const PreUnitBucketKey& key) const
                {
                        return ((~static_cast<uint64_t>(key.equatorRange) & 0xffffffff) ^
                                ~static_cast<uint64_t>(key.elevationRange));
                }
};

/** Bucket containing similar PreUnit types grouped by environmental ranges */
struct PreUnitBucket
{
                PreUnitBucketKey      key; /**< The key identifying this bucket */
                std::vector<PreUnit*> units; /**< Units in this bucket (non-owning pointers) */

                /** Adds a unit to this bucket
                 * @param unit The unit to add */
                void append(PreUnit& unit);
                /** Sorts the units within this bucket */
                void sort();
};

/** Registry for managing and querying PreUnit instances grouped by environmental conditions */
class PreUnitRegistry final
    : public PreRegistry<PreUnit, PreUnitBucket, PreUnitBucketKey, PreUnitBucketKeyHash>
{
        public:
                /** Constructs an empty PreUnitRegistry */
                PreUnitRegistry();
                /** Destroys the registry and all contained units */
                ~PreUnitRegistry() override;

                /** Registers a unit into the appropriate bucket based on its properties
                 * @param item The unit to register */
                void registerItem(PreUnit& item) override;

                /** Returns the units in the bucket matching the given key
                 * @param key The bucket key to look up
                 * @return Raw pointers to units in the bucket */
                const std::vector<PreUnit*>& getBucket(const PreUnitBucketKey& key) const override;
                /** Returns all units from all buckets in sorted order
                 * @return Raw pointers to all sorted units */
                std::vector<PreUnit*> getItems() const override;
                /** Returns the count of items in the unit registry
                 * @return The count of items in the unit registry */
                size_t getCount() const override;
                /** Finds units that can generate at the specified environmental
                 * conditions
                 * @param elevation The elevation to check
                 * @param equator The equator distance to check
                 * @param moisture The moisture level to check
                 * @param temperature The temperature to check
                 * @return Raw pointers to matching units */
                std::vector<PreUnit*> unitsByCondition(float elevation,
                                                       float equator,
                                                       float moisture,
                                                       float temperature) const;

        private:
                /** Generates a bucket key for a unit based on its properties
                 * @param unit The unit to generate a key for
                 * @return The generated bucket key */
                PreUnitBucketKey genBucketKey(const PreUnit& unit) const;
                /** Generates a bucket key for specific environmental conditions
                 * @param elevation The elevation value
                 * @param equator The equator distance value
                 * @param moisture The moisture value
                 * @param temperature The temperature value
                 * @return The generated bucket key */
                PreUnitBucketKey genConditionKey(float elevation,
                                                 float equator,
                                                 float moisture,
                                                 float temperature) const;

                /** Validates that a unit's properties are within acceptable bounds
                 * @param unit The unit to validate
                 * @return true if unit is valid, false otherwise */
                bool validateUnitBounds(const PreUnit& unit) const;
                /** Checks if a value is within the specified range
                 * @param value The value to check
                 * @param min The minimum allowed value
                 * @param max The maximum allowed value
                 * @return true if value is in range, false otherwise */
                bool isWithinBounds(float value, float min, float max) const;

                /** Normalizes a value to a range bucket
                 * @param value The value to normalize
                 * @param minValue The minimum value in the range
                 * @param maxValue The maximum value in the range
                 * @param buckets The number of buckets
                 * @return Normalized bucket value */
                float
                normalizeToRange(float value, float minValue, float maxValue, int buckets) const;

                /** Checks if a bucket is near the specified conditions
                 * @param bucketKey The bucket key to check
                 * @param conditionKey The condition key to compare against
                 * @param maxDiff The maximum allowed difference
                 * @return true if bucket is near conditions, false otherwise */
                bool isBucketNearCondition(const PreUnitBucketKey& bucketKey,
                                           const PreUnitBucketKey& conditionKey,
                                           float                   maxDiff = 1.0f) const;
                /** Filters units in a bucket that match the specified conditions
                 * @param bucket The bucket to filter
                 * @param result Output vector for matching units
                 * @param elevation The elevation to match
                 * @param equator The equator distance to match
                 * @param moisture The moisture to match
                 * @param temperature The temperature to match */
                void unitsInBucket(const PreUnitBucket&   bucket,
                                   std::vector<PreUnit*>& result,
                                   float                  elevation,
                                   float                  equator,
                                   float                  moisture,
                                   float                  temperature) const;
};

} // namespace rl

#endif
