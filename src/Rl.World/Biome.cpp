#include "Rl.World/Biome.h"
#include "Rl.World/PreUnitRegistry.h"

#include <cstdio>
#include <string>

namespace rl
{

PreBiome::PreBiome(PreBiomeRegister biomeRegister, PreUnitRegistry& unitRegistry) noexcept :
    prop(std::make_unique<Prop>()), spawnables(biomeMaxSpawnablesCount),
    pSpawnableRegistry(&unitRegistry)
{
  memset(prop.get(), 0, sizeof(Prop));
  prop->hash   = biomeRegister.getHash();
  prop->typeId = biomeRegister.getId();
}

bool PreBiome::rangesOverlap(float start1, float end1, float start2, float end2)
{
  return (start1 <= end1) && (start2 < end2);
}

float PreBiome::getStartTemperature() const
{
  return prop->startTemperature;
}

float PreBiome::getEndTemperature() const
{
  return prop->endTemperature;
}

float PreBiome::getStartMoisture() const
{
  return prop->startMoisture;
}

float PreBiome::getEndMoisture() const
{
  return prop->endMoisture;
}

float PreBiome::getStartEquator() const
{
  return prop->startEquator;
}

float PreBiome::getEndEquator() const
{
  return prop->endEquator;
}

float PreBiome::getStartElevation() const
{
  return prop->startElevation;
}

float PreBiome::getEndElevation() const
{
  return prop->endElevation;
}

PreBiome::HType PreBiome::getHash()
{
  return prop->hash;
}

PreBiome::IType PreBiome::getTypeId() const
{
  return prop->typeId;
}

void PreBiome::setStartTemperature(float value)
{
  if (value < -1.0f)
  {
    value = -1.0f;
  }
  if (value > 1.0f)
  {
    value = 1.0f;
  }
  prop->startTemperature = value;
}

void PreBiome::setEndTemperature(float value)
{
  if (value > 1.0f)
  {
    value = 1.0f;
  }
  if (value < -1.0f)
  {
    value = -1.0f;
  }
  prop->endTemperature = value;
}

void PreBiome::setStartMoisture(float value)
{
  if (value < 0.0f)
  {
    value = 0.0f;
  }
  if (value > 1.0f)
  {
    value = 1.0f;
  }
  prop->startMoisture = value;
}

void PreBiome::setEndMoisture(float value)
{
  if (value > 1.0f)
  {
    value = 1.0f;
  }
  if (value < 0.0f)
  {
    value = 0.0f;
  }
  prop->endMoisture = value;
}

void PreBiome::setStartEquator(float value)
{
  if (value < -1.0f)
  {
    value = -1.0f;
  }
  if (value > 1.0f)
  {
    value = 1.0f;
  }
  prop->startEquator = value;
}

void PreBiome::setEndEquator(float value)
{
  if (value > 1.0f)
  {
    value = 1.0f;
  }
  if (value < -1.0f)
  {
    value = -1.0f;
  }
  prop->endEquator = value;
}

void PreBiome::setStartElevation(float value)
{
  if (value < 0.0f)
  {
    value = 0.0f;
  }
  if (value > 1.0f)
  {
    value = 1.0f;
  }
  prop->startElevation = value;
}

void PreBiome::setEndElevation(float value)
{
  if (value > 1.0f)
  {
    value = 1.0f;
  }
  if (value < 0.0f)
  {
    value = 0.0f;
  }
  prop->endElevation = value;
}

void PreBiome::unitsFromRegistry()
{
  const auto& allUnits = pSpawnableRegistry->getItems();
  auto        it       = allUnits.begin();
  while (it != allUnits.end())
  {
    PreUnit* unit = *it;
    if (!unit)
    {
      ++it;
      continue;
    }
    tryAppendUnit(unit);
    ++it;
  }
}

size_t PreBiome::getSpawnableCount() const
{
  return spawnables.size();
}

const PreBiomeSpawnables& PreBiome::getSpawnables() const
{
  return spawnables;
}

bool PreBiome::canSpawnUnit(PreUnit::IType unitId) const
{
  auto it = spawnables.begin();
  while (it != spawnables.end())
  {
    PreUnit::IType k = it->first;
    if (k == unitId)
    {
      return true;
    }
    ++it;
  }
  return false;
}

void PreBiome::tryAppendUnit(PreUnit* pUnit)
{
  float avgTemperature = (prop->startTemperature + prop->endTemperature) / 2.0f;
  float avgMoisture    = (prop->startMoisture + prop->endMoisture) / 2.0f;
  float avgEquator     = (prop->startEquator + prop->endEquator) / 2.0f;
  float avgElevation   = (prop->startElevation + prop->endElevation) / 2.0f;

  if (pUnit->canGenerateBy(avgElevation, avgEquator, avgMoisture, avgTemperature))
  {
    checkOverlappingEmplace(pUnit);
  }
}

void PreBiome::checkOverlappingEmplace(PreUnit* pUnit)
{
  bool b1 = rangesOverlap(pUnit->getTemperatureStart(), pUnit->getTemperatureEnd(),
                          prop->startTemperature, prop->endTemperature);
  bool b2 = rangesOverlap(pUnit->getMoistureStart(), pUnit->getMoistureEnd(), prop->startMoisture,
                          prop->endMoisture);
  bool b3 = rangesOverlap(pUnit->getElevationStart(), pUnit->getElevationEnd(),
                          prop->startElevation, prop->endElevation);
  bool b4 = rangesOverlap(pUnit->getEquatorStart(), pUnit->getEquatorEnd(), prop->startEquator,
                          prop->endEquator);
  if (b1 && b2 && b3 && b4)
  {
    spawnables.emplace_back(pUnit->getTypeId(), pUnit);
  }
}

bool PreBiome::validateSpecs() const
{
  if (prop->startTemperature > prop->endTemperature)
    return false;
  if (prop->startMoisture > prop->endMoisture)
    return false;
  if (prop->startEquator > prop->endEquator)
    return false;
  if (prop->startElevation > prop->endElevation)
    return false;
  if (prop->startTemperature < minTemperature || prop->endTemperature > maxTemperature)
    return false;
  if (prop->startMoisture < minMoisture || prop->endMoisture > maxMoisture)
    return false;
  if (prop->startEquator < minEquator || prop->endEquator > maxEquator)
    return false;
  if (prop->startElevation < minElevation || prop->endElevation > maxElevation)
    return false;
  return true;
}

float PreBiome::clampTemperature(float temperature)
{
  if (temperature < minTemperature)
    return minTemperature;
  if (temperature > maxTemperature)
    return maxTemperature;
  return temperature;
}

float PreBiome::clampMoisture(float moisture)
{
  if (moisture < minMoisture)
    return minMoisture;
  if (moisture > maxMoisture)
    return maxMoisture;
  return moisture;
}

float PreBiome::clampEquator(float equator)
{
  if (equator < minEquator)
    return minEquator;
  if (equator > maxEquator)
    return maxEquator;
  return equator;
}

float PreBiome::clampElevation(float elevation)
{
  if (elevation < minElevation)
    return minElevation;
  if (elevation > maxElevation)
    return maxElevation;
  return elevation;
}

bool PreBiome::matchesConditions(float temperature,
                                 float moisture,
                                 float equator,
                                 float elevation) const
{
  if (temperature < prop->startTemperature || temperature > prop->endTemperature)
    return false;
  if (moisture < prop->startMoisture || moisture > prop->endMoisture)
    return false;
  if (equator < prop->startEquator || equator > prop->endEquator)
    return false;
  if (elevation < prop->startElevation || elevation > prop->endElevation)
    return false;
  return true;
}

std::string PreBiome::toString() const
{
  char buffer[512];
  snprintf(buffer, sizeof(buffer),
           "PreBiome[typeId=%u, temp=(%.2f,%.2f), moisture=(%.2f,%.2f), "
           "equator=(%.2f,%.2f), elevation=(%.2f,%.2f), spawnables=%zu]",
           prop->typeId, prop->startTemperature, prop->endTemperature, prop->startMoisture,
           prop->endMoisture, prop->startEquator, prop->endEquator, prop->startElevation,
           prop->endElevation, spawnables.size());
  return std::string(buffer);
}

} // namespace rl
