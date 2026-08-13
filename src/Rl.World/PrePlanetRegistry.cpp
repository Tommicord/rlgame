#include <algorithm>
#include <string>
#include <cstdint>

#include "Rl.World/PrePlanetRegistry.h"

namespace rl
{

void PrePlanetBucket::append(PrePlanet& planet)
{
        planets.emplace_back(&planet);
}

void PrePlanetBucket::sort()
{
        std::sort(planets.begin(), planets.end(),
                  [](const PrePlanet* a, const PrePlanet* b) { return a < b; });
}

PrePlanetRegistry::PrePlanetRegistry()
{
        // hashTable is automatically initialized by base class
}

PrePlanetRegistry::~PrePlanetRegistry()
{
        clearHashBuckets();
}

void PrePlanetRegistry::registerItem(PrePlanet& item)
{
        std::scoped_lock     lock(registryMutex);
        PrePlanetBucketKey   key    = genBucketKey(item);
        PrePlanetBucket*     bucket = hashTable.find(key);
        if (bucket == nullptr)
        {
                PrePlanetBucket newBucket;
                newBucket.key = key;
                newBucket.append(item);
                hashTable.insert(key, newBucket);
                bucketOrder.emplace_back(key);
        }
        else
        {
                bucket->append(item);
        }
}

const std::vector<PrePlanet*>& PrePlanetRegistry::getBucket(const PrePlanetBucketKey& key) const
{
        std::scoped_lock        lock(registryMutex);
        const PrePlanetBucket* bucket = hashTable.find(key);
        if (bucket == nullptr)
        {
                static const std::vector<PrePlanet*> empty;
                return empty;
        }
        return bucket->planets;
}

std::vector<PrePlanet*> PrePlanetRegistry::getItems() const
{
        std::scoped_lock       lock(registryMutex);
        std::vector<PrePlanet*> result;

        for (const auto& key : bucketOrder)
        {
                PrePlanetBucket* bucket = const_cast<PrePlanetBucket*>(hashTable.find(key));
                if (bucket != nullptr)
                {
                        bucket->sort();
                        result.insert(result.end(), bucket->planets.begin(), bucket->planets.end());
                }
        }
        return result;
}

size_t PrePlanetRegistry::getCount() const
{
        std::scoped_lock lock(registryMutex);
        size_t count = 0;
        for (const auto& key : bucketOrder)
        {
                const PrePlanetBucket* bucket = hashTable.find(key);
                if (bucket != nullptr)
                {
                        count += bucket->planets.size();
                }
        }
        return count;
}

PrePlanetBucketKey PrePlanetRegistry::genBucketKey(const PrePlanet& planet) const
{
        PrePlanetBucketKey key{};
        key.baseTemperature = planet.getBaseTemperature();
        key.baseMoisture    = planet.getBaseMoisture();
        key.atmosphereHeight = planet.getAtmosphereHeight();
        return key;
}

} // namespace rl
