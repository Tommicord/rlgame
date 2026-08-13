#include "Rl.World/PreBiomeRegistry.h"
#include "Rl.Log/Log.h"
#include "Rl.World/Biome.h"

#include <algorithm>
#include <mutex>
#include <numeric>
#include <stdexcept>

namespace rl
{

void PreBiomeBucket::append(PreBiome& biome)
{
        biomes.emplace_back(&biome);
}

void PreBiomeBucket::sort()
{
        std::sort(biomes.begin(), biomes.end(),
                  [](const PreBiome* a, const PreBiome* b) { return a < b; });
}

PreBiomeRegistry::PreBiomeRegistry()
{
        // hashTable is automatically initialized by base class
}

PreBiomeRegistry::~PreBiomeRegistry()
{
        clearHashBuckets();
}

void PreBiomeRegistry::registerItem(PreBiome& item)
{
        std::scoped_lock  lock(registryMutex);
        PreBiomeBucketKey key    = genBucketKey(item);
        PreBiomeBucket*   bucket = hashTable.find(key);
        if (bucket == nullptr)
        {
                PreBiomeBucket newBucket;
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

const std::vector<PreBiome*>& PreBiomeRegistry::getBucket(const PreBiomeBucketKey& key) const
{
        std::scoped_lock      lock(registryMutex);
        const PreBiomeBucket* bucket = hashTable.find(key);
        if (bucket == nullptr)
        {
                static const std::vector<PreBiome*> empty;
                return empty;
        }
        return bucket->biomes;
}

std::vector<PreBiome*> PreBiomeRegistry::getItems() const
{
        std::scoped_lock       lock(registryMutex);
        std::vector<PreBiome*> result;

        for (const auto& key : bucketOrder)
        {
                PreBiomeBucket* bucket = const_cast<PreBiomeBucket*>(hashTable.find(key));
                if (bucket != nullptr)
                {
                        bucket->sort();

                        result.insert(result.end(), bucket->biomes.begin(), bucket->biomes.end());
                }
        }
        return result;
}

size_t PreBiomeRegistry::getCount() const
{
        std::scoped_lock lock(registryMutex);
        size_t           count = 0;

        for (const auto& key : bucketOrder)
        {
                const PreBiomeBucket* bucket = hashTable.find(key);
                if (bucket != nullptr)
                {
                        count += bucket->biomes.size();
                }
        }
        return count;
}

std::vector<PreBiome*> PreBiomeRegistry::biomesByCondition(float elevation,
                                                           float equator,
                                                           float moisture,
                                                           float temperature)
{

        std::scoped_lock       lock(registryMutex);
        std::vector<PreBiome*> result;
        PreBiomeBucketKey conditionKey = genConditionKey(elevation, equator, moisture, temperature);

        const PreBiomeBucket* exactBucket = hashTable.find(conditionKey);
        if (exactBucket != nullptr)
        {
                biomesInBucket(*exactBucket, result, elevation, equator, moisture, temperature);
        }

        std::vector<PreBiomeBucketKey> allKeys = hashTable.getAllKeys();
        for (const auto& key : allKeys)
        {
                if (key == conditionKey)
                        continue;

                if (isBucketNearCondition(key, conditionKey))
                {
                        const PreBiomeBucket* bucket = hashTable.find(key);
                        if (bucket != nullptr)
                        {
                                biomesInBucket(*bucket, result, elevation, equator, moisture,
                                               temperature);
                        }
                }
        }

        return result;
}

static constexpr int _bucketRange = 16;

PreBiomeBucketKey PreBiomeRegistry::genBucketKey(const PreBiome& biome) const
{
        PreBiomeBucketKey key{};

        float avgElevation = (biome.getStartElevation() + biome.getEndElevation()) / 2.0f;
        key.elevationRange = normalizeToRange(avgElevation, 0.0f, 1.0f, _bucketRange);
        float avgMoisture  = (biome.getStartMoisture() + biome.getEndMoisture()) / 2.0f;
        key.moistureRange  = normalizeToRange(avgMoisture, 0.0f, 1.0f, _bucketRange);

        float avgTemperature = (biome.getStartTemperature() + biome.getEndTemperature()) / 2.0f;
        key.temperatureRange = normalizeToRange(avgTemperature, -1.0f, 1.0f, _bucketRange);

        float avgEquator = (biome.getStartEquator() + biome.getEndEquator()) / 2.0f;
        key.equatorRange = normalizeToRange(avgEquator, -1.0f, 1.0f, _bucketRange);
        return key;
}

PreBiomeBucketKey PreBiomeRegistry::genConditionKey(float elevation,
                                                    float equator,
                                                    float moisture,
                                                    float temperature) const
{
        PreBiomeBucketKey key{};
        key.elevationRange   = normalizeToRange(elevation, 0.0f, 1.0f, _bucketRange);
        key.moistureRange    = normalizeToRange(moisture, 0.0f, 1.0f, _bucketRange);
        key.temperatureRange = normalizeToRange(temperature, -1.0f, 1.0f, _bucketRange);
        key.equatorRange     = normalizeToRange(equator, -1.0f, 1.0f, _bucketRange);
        return key;
}

bool PreBiomeRegistry::isBucketNearCondition(const PreBiomeBucketKey& bucketKey,
                                             const PreBiomeBucketKey& conditionKey,
                                             float                    maxDiff) const
{
        float elevationDiff = std::abs(bucketKey.elevationRange - conditionKey.elevationRange);
        float moistureDiff  = std::abs(bucketKey.moistureRange - conditionKey.moistureRange);
        float equatorDiff   = std::abs(bucketKey.equatorRange - conditionKey.equatorRange);
        float tempDiff      = std::abs(bucketKey.temperatureRange - conditionKey.temperatureRange);

        return elevationDiff <= maxDiff && moistureDiff <= maxDiff && equatorDiff <= maxDiff &&
               tempDiff <= maxDiff;
}

void PreBiomeRegistry::biomesInBucket(const PreBiomeBucket&   bucket,
                                      std::vector<PreBiome*>& result,
                                      float                   elevation,
                                      float                   equator,
                                      float                   moisture,
                                      float                   temperature) const
{
        for (const auto& biome : bucket.biomes)
        {
                bool elevationMatch = elevation >= biome->getStartElevation() &&
                                      elevation <= biome->getEndElevation();
                bool moistureMatch =
                    moisture >= biome->getStartMoisture() && moisture <= biome->getEndMoisture();
                bool temperatureMatch = temperature >= biome->getStartTemperature() &&
                                        temperature <= biome->getEndTemperature();
                bool equatorMatch =
                    equator >= biome->getStartEquator() && equator <= biome->getEndEquator();

                if (elevationMatch && moistureMatch && temperatureMatch && equatorMatch)
                {
                        result.emplace_back(biome);
                }
        }
}

float PreBiomeRegistry::normalizeToRange(float value,
                                         float minValue,
                                         float maxValue,
                                         int   buckets) const
{
        value       = value < minValue ? minValue : value > maxValue ? maxValue : value;
        float range = maxValue - minValue;
        if (range == 0.0f)
                return 0.0f;
        float normalized  = (value - minValue) / range;
        float bucketIndex = normalized * static_cast<float>(buckets - 1);
        return std::round(bucketIndex);
}

} // namespace rl
