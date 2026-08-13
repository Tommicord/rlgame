#ifndef RL_WORLD_PRE_PLANET_REGISTRY_H
#define RL_WORLD_PRE_PLANET_REGISTRY_H

#include "Rl.World/Planet.h"

#include <cstdint>
#include <mutex>
#include <vector>

#include "Rl.World/PreRegistry.h"

namespace rl
{

class PrePlanet;

/** Key used to group similar planets into buckets based on environmental ranges */
struct PrePlanetBucketKey
{
                float baseTemperature; /**< Base temperature for the bucket */
                float baseMoisture; /**< Base moisture for the bucket */
                float atmosphereHeight; /**< Atmosphere height for the bucket */

                /** Compares two bucket keys for equality
                 * @param other The other bucket key to compare with
                 * @return true if all ranges are equal, false otherwise */
                bool operator==(const PrePlanetBucketKey& other) const
                {
                        return baseTemperature == other.baseTemperature &&
                               baseMoisture == other.baseMoisture &&
                               atmosphereHeight == other.atmosphereHeight;
                }
};

/** Hash function for PrePlanetBucketKey using bitwise operations */
struct PrePlanetBucketKeyHash
{
                /** Sets a hash value for a bucket key
                 * @param key The bucket key to hash
                 * @return Hash value for the key */
                size_t operator()(const PrePlanetBucketKey& key) const
                {
                        return ((~static_cast<uint64_t>(key.baseTemperature) & 0xffffffff) ^
                                 ~static_cast<uint64_t>(key.baseMoisture) -
                                  static_cast<uint64_t>(key.atmosphereHeight));
                }
};

/** Bucket containing similar PrePlanets */
struct PrePlanetBucket
{
                PrePlanetBucketKey      key; /**< The key identifying this bucket */
                std::vector<PrePlanet*> planets; /**< Planets in this bucket (non-owning pointers) */

                /** Adds a planet to this bucket
                 * @param planet The planet to add */
                void append(PrePlanet& planet);
                /** Sorts the planets within this bucket */
                void sort();
};

/** Planet registry for managing multiple planets */
class PrePlanetRegistry final
    : public PreRegistry<PrePlanet, PrePlanetBucket, PrePlanetBucketKey, PrePlanetBucketKeyHash>
{
        public:
                PrePlanetRegistry();
                PrePlanetRegistry(const PrePlanetRegistry& other)            = delete;
                PrePlanetRegistry& operator=(const PrePlanetRegistry& other) = delete;
                ~PrePlanetRegistry() override;

                void registerItem(PrePlanet& item) override;
                const std::vector<PrePlanet*>&
                                        getBucket(const PrePlanetBucketKey& key) const override;
                std::vector<PrePlanet*> getItems() const override;
                size_t                  getCount() const override;
                /** Generates a bucket key for a planet based on its properties
                 * @param planet The planet to generate a key for
                 * @return The generated bucket key */
                PrePlanetBucketKey genBucketKey(const PrePlanet& planet) const;
};

} // namespace rl

#endif
