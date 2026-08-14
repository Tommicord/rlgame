#ifndef RL_WORLD_PRE_BIOME_REGISTRY_H
#define RL_WORLD_PRE_BIOME_REGISTRY_H

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include "Rl.World/PreRegistry.h"

namespace rl
{

class PreBiome;

/** Key used to group similar biomes into buckets based on environmental ranges */
struct PreBiomeBucketKey
{
    float elevationRange; /**< Elevation range for the bucket */
    float moistureRange; /**< Moisture range for the bucket */
    float equatorRange; /**< Equator distance range for the bucket */
    float temperatureRange; /**< Temperature range for the bucket */

    /** Compares two bucket keys for equality
     * @param other The other bucket key to compare with
     * @return true if all ranges are equal, false otherwise */
    bool operator==(const PreBiomeBucketKey& other) const
    {
      return elevationRange == other.elevationRange && moistureRange == other.moistureRange &&
             temperatureRange == other.temperatureRange && equatorRange == other.equatorRange;
    }
};

/** Hash function for PreBiomeBucketKey using bitwise operations */
struct PreBiomeBucketKeyHash
{
    /** Sets a hash value for a bucket key
     * @param key The bucket key to hash
     * @return Hash value for the key */
    size_t operator()(const PreBiomeBucketKey& key) const
    {
      return ((~static_cast<uint64_t>(key.equatorRange) & 0xffffffff) ^
              ~static_cast<uint64_t>(key.elevationRange));
    }
};

/** Bucket containing similar PreBiome types grouped by environmental ranges */
struct PreBiomeBucket
{
    PreBiomeBucketKey      key; /**< The key identifying this bucket */
    std::vector<PreBiome*> biomes; /**< Biomes in this bucket (non-owning pointers) */

    /** Adds a biome to this bucket
     * @param biome The biome to add */
    void append(PreBiome& biome);
    /** Sorts the biomes within this bucket */
    void sort();
};

/** Registry for managing and querying PreBiome instances grouped by environmental
 * conditions */
class PreBiomeRegistry final
    : public PreRegistry<PreBiome, PreBiomeBucket, PreBiomeBucketKey, PreBiomeBucketKeyHash>
{
  public:
    /** Constructs an empty PreBiomeRegistry */
    PreBiomeRegistry();
    /** Destroys the registry and all contained biomes */
    ~PreBiomeRegistry() override;

    /** Registers a biome into the appropriate bucket based on its properties
     * @param item The biome to register */
    void registerItem(PreBiome& item) override;

    /** Returns the biomes in the bucket matching the given key
     * @param key The bucket key to look up
     * @return Raw pointers to biomes in the bucket */
    const std::vector<PreBiome*>& getBucket(const PreBiomeBucketKey& key) const override;
    /** Returns all biomes from all buckets in sorted order
     * @return Raw pointers to all sorted biomes */
    std::vector<PreBiome*> getItems() const override;
    /** Returns the count of items in the biome registry
     * @return The count of items in the biome registry */
    size_t getCount() const override;
    /** Finds biomes that match the specified environmental conditions
     * @param elevation The elevation to check
     * @param equator The equator distance to check
     * @param moisture The moisture level to check
     * @param temperature The temperature to check
     * @return Raw pointers to matching biomes */
    std::vector<PreBiome*>
    biomesByCondition(float elevation, float equator, float moisture, float temperature);

  private:
    /** Generates a bucket key for a biome based on its properties
     * @param biome The biome to generate a key for
     * @return The generated bucket key */
    PreBiomeBucketKey genBucketKey(const PreBiome& biome) const;
    /** Generates a bucket key for specific environmental conditions
     * @param elevation The elevation value
     * @param equator The equator distance value
     * @param moisture The moisture value
     * @param temperature The temperature value
     * @return The generated bucket key */
    PreBiomeBucketKey
    genConditionKey(float elevation, float equator, float moisture, float temperature) const;

    /** Normalizes a value to a range bucket
     * @param value The value to normalize
     * @param minValue The minimum value in the range
     * @param maxValue The maximum value in the range
     * @param buckets The number of buckets
     * @return Normalized bucket value */
    float normalizeToRange(float value, float minValue, float maxValue, int buckets) const;

    /** Checks if a bucket is near the specified conditions
     * @param bucketKey The bucket key to check
     * @param conditionKey The condition key to compare against
     * @param maxDiff The maximum allowed difference
     * @return true if bucket is near conditions, false otherwise */
    bool isBucketNearCondition(const PreBiomeBucketKey& bucketKey,
                               const PreBiomeBucketKey& conditionKey,
                               float                    maxDiff = 1.0f) const;
    /** Filters biomes in a bucket that match the specified conditions
     * @param bucket The bucket to filter
     * @param result Output vector for matching biomes
     * @param elevation The elevation to match
     * @param equator The equator distance to match
     * @param moisture The moisture to match
     * @param temperature The temperature to match */
    void biomesInBucket(const PreBiomeBucket&   bucket,
                        std::vector<PreBiome*>& result,
                        float                   elevation,
                        float                   equator,
                        float                   moisture,
                        float                   temperature) const;
};

} // namespace rl

#endif
