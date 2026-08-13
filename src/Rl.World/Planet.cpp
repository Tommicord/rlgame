#include "Rl.World/Planet.h"

#include <cmath>
#include <cstring>

namespace rl
{

PrePlanet::PrePlanet(PrePlanetRegister planetRegister) noexcept :
    prop(std::make_unique<Prop>())
{
    memset(prop.get(), 0, sizeof(Prop));
    prop->hash   = planetRegister.getHash();
    prop->typeId = planetRegister.getId();
}

Vec3 PrePlanet::getCenter() const
{
    return prop->center;
}

void PrePlanet::setCenter(const Vec3& newCenter)
{
    prop->center = newCenter;
}

float PrePlanet::getRadius() const
{
    return prop->radius;
}

void PrePlanet::setRadius(float newRadius)
{
    prop->radius = newRadius;
}

float PrePlanet::getMass() const
{
    return prop->mass;
}

void PrePlanet::setMass(float newMass)
{
    prop->mass = newMass;
}

float PrePlanet::getGravity() const
{
    return prop->gravity;
}

void PrePlanet::setGravity(float newGravity)
{
    prop->gravity = newGravity;
}

float PrePlanet::getBaseTemperature() const
{
    return prop->baseTemperature;
}

void PrePlanet::setBaseTemperature(float newTemp)
{
    prop->baseTemperature = newTemp;
}

float PrePlanet::getBaseMoisture() const
{
    return prop->baseMoisture;
}

void PrePlanet::setBaseMoisture(float newMoisture)
{
    prop->baseMoisture = newMoisture;
}

float PrePlanet::getAtmosphereHeight() const
{
    return prop->atmosphereHeight;
}

void PrePlanet::setAtmosphereHeight(float newHeight)
{
    prop->atmosphereHeight = newHeight;
}

uint32_t PrePlanet::getSeed() const
{
    return prop->seed;
}

void PrePlanet::setSeed(uint32_t newSeed)
{
    prop->seed = newSeed;
}

Vec3 PrePlanet::getRotationAxis() const
{
    return prop->rotationAxis;
}

void PrePlanet::setRotationAxis(const Vec3& axis)
{
    float length = std::sqrt(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);
    if (length > 0.0001f)
    {
        prop->rotationAxis.x = axis.x / length;
        prop->rotationAxis.y = axis.y / length;
        prop->rotationAxis.z = axis.z / length;
    }
    else
    {
        prop->rotationAxis = Vec3(0.0f, 1.0f, 0.0f); // Default to Y-axis
    }
}

Vec3 PrePlanet::getOrbitalVelocity() const
{
    return prop->orbitalVelocity;
}

void PrePlanet::setOrbitalVelocity(const Vec3& velocity)
{
    prop->orbitalVelocity = velocity;
}

float PrePlanet::getRotationAngle() const
{
    return prop->rotationAngle;
}

void PrePlanet::setRotationAngle(float angle)
{
    prop->rotationAngle = angle;
}

float PrePlanet::getRotationSpeed() const
{
    return prop->rotationSpeed;
}

void PrePlanet::setRotationSpeed(float speed)
{
    prop->rotationSpeed = speed;
}

uint64_t PrePlanet::getId() const
{
    return prop->id;
}

void PrePlanet::setId(uint64_t id)
{
    prop->id = id;
}

PrePlanet::IType PrePlanet::getTypeId() const
{
    return prop->typeId;
}

PrePlanet::HType PrePlanet::getHash() const
{
    return prop->hash;
}

void PrePlanet::setTypeId(uint32_t value)
{
    prop->typeId = value;
}

void PrePlanet::setHash(uint64_t value)
{
    prop->hash = value;
}

float PrePlanet::getLatitude(const Vec3& worldPosition) const
{
    Vec3  relativePos = worldPosition - prop->center;
    float distance = std::sqrt(relativePos.x * relativePos.x + relativePos.y * relativePos.y +
                               relativePos.z * relativePos.z);

    if (distance < 0.0001f)
    {
        return 0.0f;
    }
    float invDistance = 1.0f / distance;
    Vec3  direction(relativePos.x * invDistance, relativePos.y * invDistance,
                    relativePos.z * invDistance);

    // -1.0 = south pole, 0.0 = equator, 1.0 = north pole
    return direction.y;
}

float PrePlanet::getLongitude(const Vec3& worldPosition) const
{
    Vec3 relativePos = worldPosition - prop->center;

    float x = relativePos.x;
    float z = relativePos.z;
    return std::atan2(z, x);
}

Vec3 PrePlanet::rotateVector(const Vec3& v, float angle) const
{
    // Rodrigues' rotation formula
    float cosTheta = std::cos(angle);
    float sinTheta = std::sin(angle);

    // v cos(θ)
    Vec3 term1 = Vec3(v.x * cosTheta, v.y * cosTheta, v.z * cosTheta);

    // (k × v) sin(θ)
    Vec3 cross = Vec3(prop->rotationAxis.y * v.z - prop->rotationAxis.z * v.y,
                      prop->rotationAxis.z * v.x - prop->rotationAxis.x * v.z,
                      prop->rotationAxis.x * v.y - prop->rotationAxis.y * v.x);
    Vec3 term2 = Vec3(cross.x * sinTheta, cross.y * sinTheta, cross.z * sinTheta);

    // k(k · v)(1 - cos(θ))
    float dot = prop->rotationAxis.x * v.x + prop->rotationAxis.y * v.y + prop->rotationAxis.z * v.z;
    Vec3  term3 =
        Vec3(prop->rotationAxis.x * dot * (1.0f - cosTheta),
             prop->rotationAxis.y * dot * (1.0f - cosTheta),
             prop->rotationAxis.z * dot * (1.0f - cosTheta));

    return Vec3(term1.x + term2.x + term3.x, term1.y + term2.y + term3.y,
                term1.z + term2.z + term3.z);
}

Vec3 PrePlanet::rotateVector(const Vec3& v) const
{
    return rotateVector(v, prop->rotationAngle);
}

float PrePlanet::getAltitude(const Vec3& worldPosition) const
{
    float distance = getDistanceFromCenter(worldPosition);
    return distance - prop->radius;
}

float PrePlanet::getDistanceFromCenter(const Vec3& worldPosition) const
{
    Vec3 relativePos = worldPosition - prop->center;
    return std::sqrt(relativePos.x * relativePos.x + relativePos.y * relativePos.y +
                     relativePos.z * relativePos.z);
}

bool PrePlanet::isInAtmosphere(const Vec3& worldPosition) const
{
    float altitude = getAltitude(worldPosition);
    return altitude >= 0.0f && altitude <= prop->atmosphereHeight;
}

bool PrePlanet::isOnSurface(const Vec3& worldPosition, float tolerance) const
{
    float altitude = getAltitude(worldPosition);
    return std::abs(altitude) <= tolerance;
}

bool PrePlanet::validateProperties() const
{
    if (prop->radius <= 0.0f)
        return false;
    if (prop->mass <= 0.0f)
        return false;
    if (prop->gravity < 0.0f)
        return false;
    if (prop->baseTemperature < minPlanetTemperature || prop->baseTemperature > maxPlanetTemperature)
        return false;
    if (prop->baseMoisture < minPlanetMoisture || prop->baseMoisture > maxPlanetMoisture)
        return false;
    if (prop->atmosphereHeight < minAtmosphereHeight || prop->atmosphereHeight > maxAtmosphereHeight)
        return false;
    return true;
}

float PrePlanet::clampTemperature(float temperature)
{
    if (temperature < minPlanetTemperature)
        return minPlanetTemperature;
    if (temperature > maxPlanetTemperature)
        return maxPlanetTemperature;
    return temperature;
}

float PrePlanet::clampMoisture(float moisture)
{
    if (moisture < minPlanetMoisture)
        return minPlanetMoisture;
    if (moisture > maxPlanetMoisture)
        return maxPlanetMoisture;
    return moisture;
}

float PrePlanet::clampAtmosphereHeight(float height)
{
    if (height < minAtmosphereHeight)
        return minAtmosphereHeight;
    if (height > maxAtmosphereHeight)
        return maxAtmosphereHeight;
    return height;
}

float PrePlanet::clampRadius(float radius)
{
    if (radius <= 0.0f)
        return 1.0f; // Minimum reasonable radius
    return radius;
}

float PrePlanet::clampMass(float mass)
{
    if (mass <= 0.0f)
        return 1.0f; // Minimum reasonable mass
    return mass;
}

float PrePlanet::calculateGravity(float mass, float radius)
{
    if (radius <= 0.0f)
        return 0.0f;
    const float G = 6.674e-11f; // Gravitational constant
    return G * mass / (radius * radius);
}

void PrePlanet::updateGravity()
{
    prop->gravity = calculateGravity(prop->mass, prop->radius);
}

bool PrePlanet::equals(const PrePlanet& other) const
{
    if (prop->id != other.prop->id)
        return false;
    if (prop->center.x != other.prop->center.x || prop->center.y != other.prop->center.y ||
        prop->center.z != other.prop->center.z)
        return false;
    if (prop->radius != other.prop->radius)
        return false;
    if (prop->mass != other.prop->mass)
        return false;
    if (prop->gravity != other.prop->gravity)
        return false;
    if (prop->baseTemperature != other.prop->baseTemperature)
        return false;
    if (prop->baseMoisture != other.prop->baseMoisture)
        return false;
    if (prop->atmosphereHeight != other.prop->atmosphereHeight)
        return false;
    if (prop->seed != other.prop->seed)
        return false;
    return true;
}

std::string PrePlanet::toString() const
{
    char buffer[512];
    snprintf(buffer, sizeof(buffer),
             "PrePlanet[id=%llu, center=(%.2f,%.2f,%.2f), radius=%.2f, mass=%.2e, gravity=%.2f, "
             "temp=%.2f, moisture=%.2f, atmos=%.2f, seed=%u]",
             prop->id, prop->center.x, prop->center.y, prop->center.z, prop->radius, prop->mass,
             prop->gravity, prop->baseTemperature, prop->baseMoisture, prop->atmosphereHeight,
             prop->seed);
    return std::string(buffer);
}

} // namespace rl
