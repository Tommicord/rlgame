#ifndef RL_WORLD_PLANET_H
#define RL_WORLD_PLANET_H

#include "Rl.Base/GameMatrix.h"
#include "Rl.World/PreRegister.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace rl
{

class PrePlanet;

/** Bucket type for storing planets */
struct PlanetBucket
{
    std::vector<PrePlanet*> planets;
};

/** Planet data structure for GPU compute shaders */
struct PostPlanet
{
    Vec3     center; // Planet center in world coordinates
    float    radius; // Planet radius in world units
    Vec3     axis; // Planet rotation axis (normalized)
    float    mass; // Planet mass (for gravity calculations)
    Vec3     velocity; // Planet orbital velocity
    float    gravity; // Surface gravity
    float    baseTemperature; // Base temperature at equator
    float    baseMoisture; // Base moisture at equator
    float    atmosphereHeight; // Atmosphere height
    uint32_t seed; // Planet seed for procedural generation
    uint32_t shapeType; // PlanetShape enum value
    uint32_t _padding[2];
};

/** Maximum temperature value */
constexpr float maxPlanetTemperature = 1.0f;
/** Minimum temperature value */
constexpr float minPlanetTemperature = -1.0f;
/** Maximum moisture value */
constexpr float maxPlanetMoisture = 1.0f;
/** Minimum moisture value */
constexpr float minPlanetMoisture = 0.0f;
/** Maximum atmosphere height value */
constexpr float maxAtmosphereHeight = 16384.0f;
/** Minimum atmosphere height value */
constexpr float minAtmosphereHeight = 0.0f;

/** Register containing planet type information */
class PrePlanetRegister final : public PreRegister
{
  public:
    /** Constructs a PrePlanetRegister with type information
     * @param hash The 8-byte hash block for planet identification
     * @param typeId The unique type identifier */
    constexpr PrePlanetRegister(uint64_t hash, uint32_t typeId) : PreRegister(hash, typeId)
    {
    }
};

/** Planet class representing a celestial body in the game world */
class PrePlanet
{
  public:
    using IType = uint32_t;
    using HType = uint64_t;

    /** Prop containing planet properties */
    struct Prop;
    /** Constructs a PrePlanet with the given register
     * @param planetRegister The planet register containing type information */
    explicit PrePlanet(PrePlanetRegister planetRegister) noexcept;
    /** Virtual destructor */
    virtual ~PrePlanet() noexcept = default;

    /** Get planet center position
     * @return Center position in world coordinates */
    Vec3 getCenter() const;
    /** Set planet center position
     * @param newCenter New center position */
    void setCenter(const Vec3& newCenter);
    /** Get planet radius
     * @return Radius in world units */
    float getRadius() const;
    /** Set planet radius
     * @param newRadius New radius */
    void setRadius(float newRadius);
    /** Get planet mass
     * @return Mass value */
    float getMass() const;
    /** Set planet mass
     * @param newMass New mass */
    void setMass(float newMass);
    /** Get surface gravity
     * @return Surface gravity (m/s^2) */
    float getGravity() const;
    /** Set surface gravity
     * @param newGravity New gravity value */
    void setGravity(float newGravity);
    /** Get base temperature level at equator
     * @return Base temperature (0.0 = cold, 1.0 = hot) */
    float getBaseTemperature() const;
    /** Set base temperature level
     * @param newTemp New base temperature */
    void setBaseTemperature(float newTemp);
    /** Get base moisture level at equator
     * @return Base moisture (0.0 = dry, 1.0 = wet) */
    float getBaseMoisture() const;
    /** Set base moisture level
     * @param newMoisture New base moisture */
    void setBaseMoisture(float newMoisture);
    /** Get atmosphere height
     * @return Atmosphere height in world units */
    float getAtmosphereHeight() const;
    /** Set atmosphere height
     * @param newHeight New atmosphere height */
    void setAtmosphereHeight(float newHeight);
    /** Get planet seed
     * @return Seed for procedural generation */
    uint32_t getSeed() const;
    /** Set planet seed
     * @param newSeed New seed */
    void setSeed(uint32_t newSeed);
    /** Get rotation axis
     * @return Normalized rotation axis vector */
    Vec3 getRotationAxis() const;
    /** Set rotation axis
     * @param axis New rotation axis (will be normalized) */
    void setRotationAxis(const Vec3& axis);
    /** Get orbital velocity
     * @return Orbital velocity vector */
    Vec3 getOrbitalVelocity() const;
    /** Set orbital velocity
     * @param velocity New orbital velocity */
    void setOrbitalVelocity(const Vec3& velocity);
    /** Get rotation angle
     * @return Current rotation angle in radians */
    float getRotationAngle() const;
    /** Set rotation angle
     * @param angle New rotation angle in radians */
    void setRotationAngle(float angle);
    /** Get rotation speed
     * @return Rotation speed in radians per second */
    float getRotationSpeed() const;
    /** Set rotation speed
     * @param speed New rotation speed in radians per second */
    void setRotationSpeed(float speed);
    /** Get planet unique identifier
     * @return Planet ID */
    uint64_t getId() const;
    /** Set planet unique identifier
     * @param id The planet ID to set */
    void setId(uint64_t id);
    /** Returns the unique type identifier
     * @return Type ID */
    IType getTypeId() const;
    /** Returns the planet hash
     * @return Planet hash value */
    HType getHash() const;

    /** Calculate latitude from world position
     * @param worldPosition Position in world coordinates
     * @return Latitude (-1.0 = south pole, 0.0 = equator, 1.0 = north pole)
     */
    float getLatitude(const Vec3& worldPosition) const;
    /** Calculate longitude from world position
     * @param worldPosition Position in world coordinates
     * @return Longitude in radians (-π to π) */
    float getLongitude(const Vec3& worldPosition) const;
    /** Rotate a vector around the planet's rotation axis using Rodrigues'
     * formula
     * @param v Vector to rotate
     * @param angle Rotation angle in radians
     * @return Rotated vector */
    Vec3 rotateVector(const Vec3& v, float angle) const;
    /** Rotate a vector around the planet's rotation axis using current
     * rotation angle
     * @param v Vector to rotate
     * @return Rotated vector */
    Vec3 rotateVector(const Vec3& v) const;
    /** Calculate altitude from world position
     * @param worldPosition Position in world coordinates
     * @return Altitude above surface (negative = below surface) */
    float getAltitude(const Vec3& worldPosition) const;
    /** Calculate distance from planet center
     * @param worldPosition Position in world coordinates
     * @return Distance from center */
    float getDistanceFromCenter(const Vec3& worldPosition) const;
    /** Check if position is within atmosphere
     * @param worldPosition Position in world coordinates
     * @return True if within atmosphere */
    bool isInAtmosphere(const Vec3& worldPosition) const;
    /** Check if position is on planet surface
     * @param worldPosition Position in world coordinates
     * @param tolerance Tolerance for surface detection
     * @return True if on surface */
    bool isOnSurface(const Vec3& worldPosition, float tolerance = 0.1f) const;

    /** Validates if the planet's properties are within acceptable ranges
     * @return True if all properties are valid, false otherwise */
    bool validateProperties() const;
    /** Clamps temperature to valid range
     * @param temperature Temperature value to clamp
     * @return Clamped temperature value */
    static float clampTemperature(float temperature);
    /** Clamps moisture to valid range
     * @param moisture Moisture value to clamp
     * @return Clamped moisture value */
    static float clampMoisture(float moisture);
    /** Clamps atmosphere height to valid range
     * @param height Atmosphere height to clamp
     * @return Clamped atmosphere height value */
    static float clampAtmosphereHeight(float height);
    /** Clamps radius to positive range
     * @param radius Radius value to clamp
     * @return Clamped radius value */
    static float clampRadius(float radius);
    /** Clamps mass to positive range
     * @param mass Mass value to clamp
     * @return Clamped mass value */
    static float clampMass(float mass);
    /** Calculates surface gravity from mass and radius
     * @param mass Planet mass
     * @param radius Planet radius
     * @return Calculated surface gravity */
    static float calculateGravity(float mass, float radius);
    /** Updates gravity based on current mass and radius */
    void updateGravity();
    /** Checks if two planets are equal in properties
     * @param other Planet to compare with
     * @return True if planets have equal properties */
    bool equals(const PrePlanet& other) const;
    /** Returns a string representation of the planet
     * @return String representation */
    std::string toString() const;

  protected:
    /** Sets the unique type identifier
     * @param value The type ID to set */
    void setTypeId(uint32_t value);
    /** Sets the planet hash
     * @param value The hash value to set */
    void setHash(uint64_t value);

  private:
    std::unique_ptr<Prop> prop;
};

/** Prop structure for PrePlanet containing physical and environmental properties */
struct PrePlanet::Prop
{
    Vec3     center; // World position of planet center
    float    radius; // Planet radius
    float    mass; // Planet mass
    float    gravity; // Surface gravity
    float    baseTemperature; // Base temperature at equator
    float    baseMoisture; // Base moisture at equator
    float    atmosphereHeight; // Atmosphere height
    uint32_t seed; // Procedural generation seed
    Vec3     rotationAxis; // Planet rotation axis (normalized)
    Vec3     orbitalVelocity; // Orbital velocity vector
    float    rotationAngle; // Current rotation angle in radians
    float    rotationSpeed; // Rotation speed in radians per second
    uint64_t id; // Unique planet identifier
    uint32_t typeId; // Unique type identifier
    uint64_t hash; // Planet hash value
};

} // namespace rl

#endif // RL_WORLD_PLANET_H
