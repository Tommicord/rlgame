#include "Rl.World/Unit.h"
#include "Rl.World/PreRegister.h"
#include "Rl.World/PreUnitRegister.h"
#include "Rl.World/PreUnitRegistry.h"

#include <cmath>
#include <cstdio>
#include <string>

namespace rl
{

std::unique_ptr<PreUnitRegistry> PreUnit::preUnitRegistry = std::make_unique<PreUnitRegistry>();

PreUnit::PreUnit(PreRegister unitRegister) noexcept : prop(std::make_unique<Prop>())
{
  memset(prop.get(), 0, sizeof(Prop));
  prop->hash   = unitRegister.getHash();
  prop->typeId = unitRegister.getId();
}

bool PreUnit::canGenerateByElevation(float elevation) const
{
  float target = (getElevationStart() + getElevationEnd()) / 2.0f;
  float near   = getElevationNearAccept();
  return std::abs(target - elevation) <= near;
}

bool PreUnit::canGenerateByMoisture(float moisture) const
{
  float target = (getMoistureStart() + getMoistureEnd()) / 2.0f;
  float near   = getMoistureNearAccept();
  return std::abs(target - moisture) <= near;
}

bool PreUnit::canGenerateByEquator(float equator) const
{
  float target = (getEquatorStart() + getEquatorEnd()) / 2.0f;
  float near   = getEquatorNearAccept();
  return std::abs(target - equator) <= near;
}

bool PreUnit::canGenerateByTemperature(float temperature) const
{
  float target = (getTemperatureStart() + getTemperatureEnd()) / 2.0f;
  float near   = getTemperatureNearAccept();
  return std::abs(target - temperature) <= near;
}

bool PreUnit::canGenerateBy(float elevation, float equator, float moisture, float temperature) const
{
  bool b1 = canGenerateByElevation(elevation);
  bool b2 = canGenerateByMoisture(moisture);
  bool b3 = canGenerateByTemperature(temperature);
  bool b4 = canGenerateByEquator(equator);
  return b1 && b2 && b3 && b4;
}

float PreUnit::getFlammability() const
{
  return prop->flammability;
}
float PreUnit::getExplosionResistance() const
{
  return prop->explosionResistance;
}
float PreUnit::getMoistureStart() const
{
  return prop->moistureStart;
}
float PreUnit::getMoistureEnd() const
{
  return prop->moistureEnd;
}
float PreUnit::getEquatorStart() const
{
  return prop->equatorStart;
}
float PreUnit::getEquatorEnd() const
{
  return prop->equatorEnd;
}
float PreUnit::getTemperatureStart() const
{
  return prop->temperatureStart;
}
float PreUnit::getTemperatureEnd() const
{
  return prop->temperatureEnd;
}
float PreUnit::getElevationStart() const
{
  return prop->elevationStart;
}
float PreUnit::getElevationEnd() const
{
  return prop->elevationEnd;
}
uint32_t PreUnit::getTypeId() const
{
  return prop->typeId;
}
uint64_t PreUnit::getHash() const
{
  return prop->hash;
}
void PreUnit::setFlammability(float value)
{
  prop->flammability = value;
}
void PreUnit::setExplosionResistance(float value)
{
  prop->explosionResistance = value;
}
void PreUnit::setMoistureStart(float value)
{
  if (value < 0.0f)
  {
    value = 0.0f;
  }
  if (value > 1.0f)
  {
    value = 1.0f;
  }
  prop->moistureStart = value;
}
void PreUnit::setMoistureEnd(float value)
{
  if (value > 1.0f)
  {
    value = 1.0f;
  }
  if (value < 0.0f)
  {
    value = 0.0f;
  }
  prop->moistureEnd = value;
}
void PreUnit::setTemperatureStart(float value)
{
  if (value < -1.0f)
  {
    value = -1.0f;
  }
  if (value > 1.0f)
  {
    value = 1.0f;
  }
  prop->temperatureStart = value;
}
void PreUnit::setTemperatureEnd(float value)
{
  if (value > 1.0f)
  {
    value = 1.0f;
  }
  if (value < -1.0f)
  {
    value = -1.0f;
  }
  prop->temperatureEnd = value;
}
void PreUnit::setEquatorStart(float value)
{
  if (value < -1.0f)
  {
    value = -1.0f;
  }
  if (value > 1.0f)
  {
    value = 1.0f;
  }
  prop->equatorStart = value;
}
void PreUnit::setEquatorEnd(float value)
{
  if (value > 1.0f)
  {
    value = 1.0f;
  }
  if (value < -1.0f)
  {
    value = -1.0f;
  }
  prop->equatorEnd = value;
}
void PreUnit::setElevationStart(float value)
{
  if (value < 0.0f)
  {
    value = 0.0f;
  }
  if (value > 1.0f)
  {
    value = 1.0f;
  }
  prop->elevationStart = value;
}
void PreUnit::setElevationEnd(float value)
{
  if (value > 1.0f)
  {
    value = 1.0f;
  }
  if (value < 0.0f)
  {
    value = 0.0f;
  }
  prop->elevationEnd = value;
}
void PreUnit::setTypeId(uint32_t value)
{
  prop->typeId = value;
}
void PreUnit::setHash(uint64_t value)
{
  prop->hash = value;
}

bool PreUnit::validateProperties() const
{
  if (prop->temperatureStart > prop->temperatureEnd)
    return false;
  if (prop->moistureStart > prop->moistureEnd)
    return false;
  if (prop->equatorStart > prop->equatorEnd)
    return false;
  if (prop->elevationStart > prop->elevationEnd)
    return false;
  if (prop->temperatureStart < minTemperature || prop->temperatureEnd > maxTemperature)
    return false;
  if (prop->moistureStart < minMoisture || prop->moistureEnd > maxMoisture)
    return false;
  if (prop->equatorStart < minEquator || prop->equatorEnd > maxEquator)
    return false;
  if (prop->elevationStart < minElevation || prop->elevationEnd > maxElevation)
    return false;
  if (prop->flammability < minFlamability || prop->flammability > maxFlammability)
    return false;
  if (prop->explosionResistance < 0.0f)
    return false;
  return true;
}

float PreUnit::clampTemperature(float temperature)
{
  if (temperature < minTemperature)
    return minTemperature;
  if (temperature > maxTemperature)
    return maxTemperature;
  return temperature;
}

float PreUnit::clampMoisture(float moisture)
{
  if (moisture < minMoisture)
    return minMoisture;
  if (moisture > maxMoisture)
    return maxMoisture;
  return moisture;
}

float PreUnit::clampEquator(float equator)
{
  if (equator < minEquator)
    return minEquator;
  if (equator > maxEquator)
    return maxEquator;
  return equator;
}

float PreUnit::clampElevation(float elevation)
{
  if (elevation < minElevation)
    return minElevation;
  if (elevation > maxElevation)
    return maxElevation;
  return elevation;
}

float PreUnit::clampFlammability(float flammability)
{
  if (flammability < minFlamability)
    return minFlamability;
  if (flammability > maxFlammability)
    return maxFlammability;
  return flammability;
}

float PreUnit::clampExplosionResistance(float resistance)
{
  if (resistance < 0.0f)
    return 0.0f;
  return resistance;
}

bool PreUnit::matchesRequirements(float elevation,
                                  float equator,
                                  float moisture,
                                  float temperature) const
{
  if (elevation < prop->elevationStart || elevation > prop->elevationEnd)
    return false;
  if (equator < prop->equatorStart || equator > prop->equatorEnd)
    return false;
  if (moisture < prop->moistureStart || moisture > prop->moistureEnd)
    return false;
  if (temperature < prop->temperatureStart || temperature > prop->temperatureEnd)
    return false;
  return true;
}

std::string PreUnit::toString() const
{
  char buffer[512];
  snprintf(buffer, sizeof(buffer),
           "PreUnit[typeId=%u, hash=%llu, temp=(%.2f,%.2f), moisture=(%.2f,%.2f), "
           "equator=(%.2f,%.2f), elevation=(%.2f,%.2f), flammability=%.2f, explosionRes=%.2f]",
           prop->typeId, prop->hash, prop->temperatureStart, prop->temperatureEnd,
           prop->moistureStart, prop->moistureEnd, prop->equatorStart, prop->equatorEnd,
           prop->elevationStart, prop->elevationEnd, prop->flammability, prop->explosionResistance);
  return std::string(buffer);
}

} // namespace rl
