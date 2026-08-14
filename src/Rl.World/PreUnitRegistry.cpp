#include "Rl.World/PreUnitRegistry.h"
#include "Rl.Log/Log.h"
#include "Rl.World/Unit.h"

#include <algorithm>
#include <mutex>
#include <numeric>
#include <stdexcept>

namespace rl
{

void PreUnitBucket::append(PreUnit& unit)
{
  units.emplace_back(&unit);
}

void PreUnitBucket::sort()
{
  std::sort(units.begin(), units.end(),
            [](const PreUnit* a, const PreUnit* b) { return a->getTypeId() < b->getTypeId(); });
}

PreUnitRegistry::PreUnitRegistry()
{
  // hashTable is automatically initialized by base class
}

PreUnitRegistry::~PreUnitRegistry()
{
  clearHashBuckets();
}

void PreUnitRegistry::registerItem(PreUnit& item)
{
  std::scoped_lock lock(registryMutex);
  bool             validBounds = validateUnitBounds(item);
  if (!validBounds)
  {
    return;
  }

  PreUnitBucketKey key    = genBucketKey(item);
  PreUnitBucket*   bucket = hashTable.find(key);
  if (bucket == nullptr)
  {
    PreUnitBucket newBucket;
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

const std::vector<PreUnit*>& PreUnitRegistry::getBucket(const PreUnitBucketKey& key) const
{
  std::scoped_lock     lock(registryMutex);
  const PreUnitBucket* bucket = hashTable.find(key);
  if (bucket == nullptr)
  {
    static const std::vector<PreUnit*> empty;
    return empty;
  }
  return bucket->units;
}

std::vector<PreUnit*> PreUnitRegistry::getItems() const
{
  std::scoped_lock      lock(registryMutex);
  std::vector<PreUnit*> result;

  for (const auto& key : bucketOrder)
  {
    PreUnitBucket* bucket = const_cast<PreUnitBucket*>(hashTable.find(key));
    if (bucket != nullptr)
    {
      bucket->sort();

      result.insert(result.end(), bucket->units.begin(), bucket->units.end());
    }
  }
  return result;
}

size_t PreUnitRegistry::getCount() const
{
  std::scoped_lock lock(registryMutex);
  size_t           count = 0;

  for (const auto& key : bucketOrder)
  {
    const PreUnitBucket* bucket = hashTable.find(key);
    if (bucket != nullptr)
    {
      count += bucket->units.size();
    }
  }
  return count;
}

std::vector<PreUnit*> PreUnitRegistry::unitsByCondition(float elevation,
                                                        float equator,
                                                        float moisture,
                                                        float temperature) const
{

  std::scoped_lock      lock(registryMutex);
  std::vector<PreUnit*> result;
  PreUnitBucketKey      conditionKey = genConditionKey(elevation, equator, moisture, temperature);

  const PreUnitBucket* exactBucket = hashTable.find(conditionKey);
  if (exactBucket != nullptr)
  {
    unitsInBucket(*exactBucket, result, elevation, equator, moisture, temperature);
  }

  std::vector<PreUnitBucketKey> allKeys = hashTable.getAllKeys();
  for (const auto& key : allKeys)
  {
    if (key == conditionKey)
      continue;

    if (isBucketNearCondition(key, conditionKey))
    {
      const PreUnitBucket* bucket = hashTable.find(key);
      if (bucket != nullptr)
      {
        unitsInBucket(*bucket, result, elevation, equator, moisture, temperature);
      }
    }
  }

  return result;
}

bool PreUnitRegistry::validateUnitBounds(const PreUnit& unit) const
{
  float tempStart = unit.getTemperatureStart();
  float tempEnd   = unit.getTemperatureEnd();
  if (!isWithinBounds(tempStart, minTemperature, maxTemperature) ||
      !isWithinBounds(tempEnd, minTemperature, maxTemperature))
  {
    return false;
  }

  float moistureStart = unit.getMoistureStart();
  float moistureEnd   = unit.getMoistureEnd();
  if (!isWithinBounds(moistureStart, minMoisture, maxMoisture) ||
      !isWithinBounds(moistureEnd, minMoisture, maxMoisture))
  {
    return false;
  }

  float flammability = unit.getFlammability();
  if (!isWithinBounds(flammability, minFlamability, maxFlammability))
  {
    return false;
  }

  float elevStart = unit.getElevationStart();
  float elevEnd   = unit.getElevationEnd();
  if (!isWithinBounds(elevStart, minElevation, maxElevation) ||
      !isWithinBounds(elevEnd, minElevation, maxElevation))
  {
    return false;
  }

  return true;
}

bool PreUnitRegistry::isWithinBounds(float value, float min, float max) const
{
  return min <= value && value <= max;
}

static constexpr int _bucketRange = 16;

PreUnitBucketKey PreUnitRegistry::genBucketKey(const PreUnit& unit) const
{
  PreUnitBucketKey key{};

  float avgElevation = (unit.getElevationStart() + unit.getElevationEnd()) / 2.0f;
  key.elevationRange = normalizeToRange(avgElevation, minElevation, maxElevation, _bucketRange);
  float avgMoisture  = (unit.getMoistureStart() + unit.getMoistureEnd()) / 2.0f;
  key.moistureRange  = normalizeToRange(avgMoisture, minMoisture, maxMoisture, _bucketRange);

  float avgTemperature = (unit.getTemperatureStart() + unit.getTemperatureEnd()) / 2.0f;
  key.temperatureRange =
      normalizeToRange(avgTemperature, minTemperature, maxTemperature, _bucketRange);

  float avgEquator = (unit.getEquatorStart() + unit.getEquatorEnd()) / 2.0f;
  key.equatorRange = normalizeToRange(avgEquator, minEquator, maxEquator, _bucketRange);
  return key;
}

PreUnitBucketKey PreUnitRegistry::genConditionKey(float elevation,
                                                  float equator,
                                                  float moisture,
                                                  float temperature) const
{
  PreUnitBucketKey key{};
  key.elevationRange = normalizeToRange(elevation, minElevation, maxElevation, _bucketRange);
  key.moistureRange  = normalizeToRange(moisture, minMoisture, maxMoisture, _bucketRange);
  key.temperatureRange =
      normalizeToRange(temperature, minTemperature, maxTemperature, _bucketRange);
  key.equatorRange = normalizeToRange(equator, minEquator, maxEquator, _bucketRange);
  return key;
}

bool PreUnitRegistry::isBucketNearCondition(const PreUnitBucketKey& bucketKey,
                                            const PreUnitBucketKey& conditionKey,
                                            float                   maxDiff) const
{
  float elevationDiff = std::abs(bucketKey.elevationRange - conditionKey.elevationRange);
  float moistureDiff  = std::abs(bucketKey.moistureRange - conditionKey.moistureRange);
  float equatorDiff   = std::abs(bucketKey.equatorRange - conditionKey.equatorRange);
  float tempDiff      = std::abs(bucketKey.temperatureRange - conditionKey.temperatureRange);

  return elevationDiff <= maxDiff && moistureDiff <= maxDiff && equatorDiff <= maxDiff &&
         tempDiff <= maxDiff;
}

void PreUnitRegistry::unitsInBucket(const PreUnitBucket&   bucket,
                                    std::vector<PreUnit*>& result,
                                    float                  elevation,
                                    float                  equator,
                                    float                  moisture,
                                    float                  temperature) const
{
  for (const auto& unit : bucket.units)
  {
    if (unit->canGenerateBy(elevation, equator, moisture, temperature))
    {
      result.emplace_back(unit);
    }
  }
}

float PreUnitRegistry::normalizeToRange(float value,
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
