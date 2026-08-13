#include <gtest/gtest.h>
#include "Rl.World/Planet.h"
#include "Rl.World/PrePlanetRegistry.h"

namespace rl
{

// Test Planet construction
TEST(PlanetTest, Construction)
{
        PrePlanetRegister registerInfo(0x1234567890ABCDEF, 1);
        PrePlanet         planet(registerInfo);

        planet.setCenter(Vec3(0.0f, 0.0f, 0.0f));
        planet.setRadius(6371.0f);
        planet.setMass(5.97e24f);
        planet.setSeed(12345);
        planet.setId(0);

        EXPECT_EQ(planet.getId(), 0);
        EXPECT_FLOAT_EQ(planet.getCenter().x, 0.0f);
        EXPECT_FLOAT_EQ(planet.getCenter().y, 0.0f);
        EXPECT_FLOAT_EQ(planet.getCenter().z, 0.0f);
        EXPECT_FLOAT_EQ(planet.getRadius(), 6371.0f);
        EXPECT_FLOAT_EQ(planet.getMass(), 5.97e24f);
        EXPECT_EQ(planet.getSeed(), 12345);
}

// Test Planet setters
TEST(PlanetTest, Setters)
{
        PrePlanetRegister registerInfo(0x1234567890ABCDEF, 1);
        PrePlanet planet(registerInfo);

        planet.setCenter(Vec3(10.0f, 20.0f, 30.0f));
        EXPECT_FLOAT_EQ(planet.getCenter().x, 10.0f);
        EXPECT_FLOAT_EQ(planet.getCenter().y, 20.0f);
        EXPECT_FLOAT_EQ(planet.getCenter().z, 30.0f);

        planet.setRadius(7000.0f);
        EXPECT_FLOAT_EQ(planet.getRadius(), 7000.0f);

        planet.setMass(6.0e24f);
        EXPECT_FLOAT_EQ(planet.getMass(), 6.0e24f);

        planet.setGravity(15.0f);
        EXPECT_FLOAT_EQ(planet.getGravity(), 15.0f);

        planet.setBaseTemperature(0.8f);
        EXPECT_FLOAT_EQ(planet.getBaseTemperature(), 0.8f);

        planet.setBaseMoisture(0.3f);
        EXPECT_FLOAT_EQ(planet.getBaseMoisture(), 0.3f);

        planet.setAtmosphereHeight(200.0f);
        EXPECT_FLOAT_EQ(planet.getAtmosphereHeight(), 200.0f);

        planet.setSeed(54321);
        EXPECT_EQ(planet.getSeed(), 54321);
}

// Test rotation axis setter
TEST(PlanetTest, RotationAxisSetter)
{
        PrePlanetRegister registerInfo(0x1234567890ABCDEF, 1);
        PrePlanet planet(registerInfo);

        planet.setRotationAxis(Vec3(1.0f, 0.0f, 0.0f));
        Vec3 axis = planet.getRotationAxis();
        EXPECT_FLOAT_EQ(axis.x, 1.0f);
        EXPECT_FLOAT_EQ(axis.y, 0.0f);
        EXPECT_FLOAT_EQ(axis.z, 0.0f);

        // Test normalization
        planet.setRotationAxis(Vec3(2.0f, 0.0f, 0.0f));
        axis = planet.getRotationAxis();
        EXPECT_FLOAT_EQ(axis.x, 1.0f);

        // Test zero vector defaults to Y-axis
        planet.setRotationAxis(Vec3(0.0f, 0.0f, 0.0f));
        axis = planet.getRotationAxis();
        EXPECT_FLOAT_EQ(axis.x, 0.0f);
        EXPECT_FLOAT_EQ(axis.y, 1.0f);
        EXPECT_FLOAT_EQ(axis.z, 0.0f);
}

// Test orbital velocity setter
TEST(PlanetTest, OrbitalVelocitySetter)
{
        PrePlanetRegister registerInfo(0x1234567890ABCDEF, 1);
        PrePlanet planet(registerInfo);

        planet.setOrbitalVelocity(Vec3(1000.0f, 2000.0f, 3000.0f));
        Vec3 velocity = planet.getOrbitalVelocity();
        EXPECT_FLOAT_EQ(velocity.x, 1000.0f);
        EXPECT_FLOAT_EQ(velocity.y, 2000.0f);
        EXPECT_FLOAT_EQ(velocity.z, 3000.0f);
}

// Test rotation angle and speed setters
TEST(PlanetTest, RotationAngleSpeedSetters)
{
        PrePlanetRegister registerInfo(0x1234567890ABCDEF, 1);
        PrePlanet planet(registerInfo);

        planet.setRotationAngle(1.5f);
        EXPECT_FLOAT_EQ(planet.getRotationAngle(), 1.5f);

        planet.setRotationSpeed(0.0001f);
        EXPECT_FLOAT_EQ(planet.getRotationSpeed(), 0.0001f);
}

// Test latitude calculation
TEST(PlanetTest, LatitudeCalculation)
{
        PrePlanetRegister registerInfo(0x1234567890ABCDEF, 1);
        PrePlanet planet(registerInfo);
        planet.setCenter(Vec3(0.0f, 0.0f, 0.0f));
        planet.setRadius(1000.0f);

        // At equator (Y = 0)
        float lat1 = planet.getLatitude(Vec3(1000.0f, 0.0f, 0.0f));
        EXPECT_NEAR(lat1, 0.0f, 0.001f);

        // At north pole (Y = radius)
        float lat2 = planet.getLatitude(Vec3(0.0f, 1000.0f, 0.0f));
        EXPECT_NEAR(lat2, 1.0f, 0.001f);

        // At south pole (Y = -radius)
        float lat3 = planet.getLatitude(Vec3(0.0f, -1000.0f, 0.0f));
        EXPECT_NEAR(lat3, -1.0f, 0.001f);

        // At 45 degrees north
        float lat4 = planet.getLatitude(Vec3(707.1f, 707.1f, 0.0f));
        EXPECT_NEAR(lat4, 0.707f, 0.01f);
}

// Test longitude calculation
TEST(PlanetTest, LongitudeCalculation)
{
        PrePlanetRegister registerInfo(0x1234567890ABCDEF, 1);
        PrePlanet planet(registerInfo);
        planet.setCenter(Vec3(0.0f, 0.0f, 0.0f));
        planet.setRadius(1000.0f);

        // At 0 degrees (positive X axis)
        float lon1 = planet.getLongitude(Vec3(1000.0f, 0.0f, 0.0f));
        EXPECT_NEAR(lon1, 0.0f, 0.001f);

        // At 90 degrees (positive Z axis)
        float lon2 = planet.getLongitude(Vec3(0.0f, 0.0f, 1000.0f));
        EXPECT_NEAR(lon2, 1.571f, 0.001f); // π/2
}

// Test altitude calculation
TEST(PlanetTest, AltitudeCalculation)
{
        PrePlanetRegister registerInfo(0x1234567890ABCDEF, 1);
        PrePlanet planet(registerInfo);
        planet.setCenter(Vec3(0.0f, 0.0f, 0.0f));
        planet.setRadius(1000.0f);

        // On surface
        float alt1 = planet.getAltitude(Vec3(1000.0f, 0.0f, 0.0f));
        EXPECT_NEAR(alt1, 0.0f, 0.001f);

        // Above surface
        float alt2 = planet.getAltitude(Vec3(1500.0f, 0.0f, 0.0f));
        EXPECT_NEAR(alt2, 500.0f, 0.001f);

        // Below surface
        float alt3 = planet.getAltitude(Vec3(500.0f, 0.0f, 0.0f));
        EXPECT_NEAR(alt3, -500.0f, 0.001f);
}

// Test distance from center
TEST(PlanetTest, DistanceFromCenter)
{
        PrePlanetRegister registerInfo(0x1234567890ABCDEF, 1);
        PrePlanet planet(registerInfo);
        planet.setCenter(Vec3(100.0f, 200.0f, 300.0f));
        planet.setRadius(1000.0f);

        float dist1 = planet.getDistanceFromCenter(Vec3(100.0f, 200.0f, 300.0f));
        EXPECT_NEAR(dist1, 0.0f, 0.001f);

        float dist2 = planet.getDistanceFromCenter(Vec3(1100.0f, 200.0f, 300.0f));
        EXPECT_NEAR(dist2, 1000.0f, 0.001f);
}

// Test atmosphere detection
TEST(PlanetTest, AtmosphereDetection)
{
        PrePlanetRegister registerInfo(0x1234567890ABCDEF, 1);
        PrePlanet planet(registerInfo);
        planet.setCenter(Vec3(0.0f, 0.0f, 0.0f));
        planet.setRadius(1000.0f);
        planet.setAtmosphereHeight(100.0f);

        EXPECT_TRUE(planet.isInAtmosphere(Vec3(1050.0f, 0.0f, 0.0f)));
        EXPECT_TRUE(planet.isInAtmosphere(Vec3(1000.0f, 0.0f, 0.0f)));
        EXPECT_FALSE(planet.isInAtmosphere(Vec3(1101.0f, 0.0f, 0.0f)));
        EXPECT_FALSE(planet.isInAtmosphere(Vec3(900.0f, 0.0f, 0.0f)));
}

// Test surface detection
TEST(PlanetTest, SurfaceDetection)
{
        PrePlanetRegister registerInfo(0x1234567890ABCDEF, 1);
        PrePlanet planet(registerInfo);
        planet.setCenter(Vec3(0.0f, 0.0f, 0.0f));
        planet.setRadius(1000.0f);

        EXPECT_TRUE(planet.isOnSurface(Vec3(1000.0f, 0.0f, 0.0f)));
        EXPECT_TRUE(planet.isOnSurface(Vec3(1000.1f, 0.0f, 0.0f)));
        EXPECT_TRUE(planet.isOnSurface(Vec3(999.9f, 0.0f, 0.0f)));
        EXPECT_FALSE(planet.isOnSurface(Vec3(1100.0f, 0.0f, 0.0f)));
}

// Test Rodrigues' rotation formula
TEST(PlanetTest, RodriguesZeroRotation)
{
        PrePlanetRegister registerInfo(0x1234567890ABCDEF, 1);
        PrePlanet planet(registerInfo);
        planet.setRotationAxis(Vec3(0.0f, 1.0f, 0.0f));

        Vec3 original(1.0f, 0.0f, 0.0f);
        Vec3 rotated = planet.rotateVector(original, 0.0f);

        EXPECT_NEAR(rotated.x, original.x, 0.001f);
        EXPECT_NEAR(rotated.y, original.y, 0.001f);
        EXPECT_NEAR(rotated.z, original.z, 0.001f);
}

// Test Rodrigues' rotation formula
TEST(PlanetTest, Rodrigues90DegreeRotation)
{
        PrePlanetRegister registerInfo(0x1234567890ABCDEF, 1);
        PrePlanet planet(registerInfo);
        planet.setRotationAxis(Vec3(0.0f, 1.0f, 0.0f));

        Vec3 original(1.0f, 0.0f, 0.0f);
        Vec3 rotated = planet.rotateVector(original, 1.571f); // π/2

        // Should rotate to (0, 0, -1) based on right-hand rule
        EXPECT_NEAR(rotated.x, 0.0f, 0.01f);
        EXPECT_NEAR(rotated.y, 0.0f, 0.01f);
        EXPECT_NEAR(rotated.z, -1.0f, 0.01f);
}

// Test Rodrigues' rotation formula
TEST(PlanetTest, Rodrigues180DegreeRotation)
{
        PrePlanetRegister registerInfo(0x1234567890ABCDEF, 1);
        PrePlanet planet(registerInfo);
        planet.setRotationAxis(Vec3(0.0f, 1.0f, 0.0f));

        Vec3 original(1.0f, 0.0f, 0.0f);
        Vec3 rotated = planet.rotateVector(original, 3.142f); // π

        // Should rotate to (-1, 0, 0)
        EXPECT_NEAR(rotated.x, -1.0f, 0.01f);
        EXPECT_NEAR(rotated.y, 0.0f, 0.01f);
        EXPECT_NEAR(rotated.z, 0.0f, 0.01f);
}

// Test Rodrigues' rotation formula
TEST(PlanetTest, RodriguesArbitraryAxis)
{
        PrePlanetRegister registerInfo(0x1234567890ABCDEF, 1);
        PrePlanet planet(registerInfo);
        planet.setRotationAxis(Vec3(1.0f, 1.0f, 0.0f));

        Vec3 original(1.0f, 0.0f, 0.0f);
        Vec3 rotated = planet.rotateVector(original, 1.571f);

        // Vector should be rotated, not equal to original
        EXPECT_NE(rotated.x, original.x);
}

// Test rotation using current rotation angle
TEST(PlanetTest, RotateVectorWithCurrentAngle)
{
        PrePlanetRegister registerInfo(0x1234567890ABCDEF, 1);
        PrePlanet planet(registerInfo);
        planet.setRotationAxis(Vec3(0.0f, 1.0f, 0.0f));
        planet.setRotationAngle(1.571f);

        Vec3 original(1.0f, 0.0f, 0.0f);
        Vec3 rotated = planet.rotateVector(original);

        EXPECT_NEAR(rotated.x, 0.0f, 0.01f);
        EXPECT_NEAR(rotated.z, -1.0f, 0.01f);
}

// Test PlanetRegistry construction
TEST(PlanetRegistryTest, Construction)
{
        PrePlanetRegistry registry;

        EXPECT_EQ(registry.getCount(), 0);
}

// Test PlanetRegistry registration
TEST(PlanetRegistryTest, Registration)
{
        PrePlanetRegistry registry;

        PrePlanetRegister registerInfo1(0x1234567890ABCDEF, 1);
        PrePlanet planet1(registerInfo1);
        planet1.setCenter(Vec3(0.0f, 0.0f, 0.0f));
        planet1.setRadius(6371.0f);
        planet1.setMass(5.97e24f);
        planet1.setId(0);
        registry.registerItem(planet1);

        EXPECT_EQ(registry.getCount(), 1);

        PrePlanetRegister registerInfo2(0x1234567890ABCDEF, 2);
        PrePlanet planet2(registerInfo2);
        planet2.setCenter(Vec3(100.0f, 0.0f, 0.0f));
        planet2.setRadius(5000.0f);
        planet2.setMass(4.0e24f);
        planet2.setId(1);
        registry.registerItem(planet2);

        EXPECT_EQ(registry.getCount(), 2);
}

// Test PlanetRegistry getBucket
TEST(PlanetRegistryTest, GetBucket)
{
        PrePlanetRegistry registry;

        PrePlanetRegister registerInfo(0x1234567890ABCDEF, 1);
        PrePlanet planet1(registerInfo);
        planet1.setCenter(Vec3(0.0f, 0.0f, 0.0f));
        planet1.setRadius(6371.0f);
        planet1.setMass(5.97e24f);
        planet1.setId(0);
        registry.registerItem(planet1);

        PrePlanetBucketKey key = registry.genBucketKey(planet1);
        const std::vector<PrePlanet*>& bucket = registry.getBucket(key);
        EXPECT_EQ(bucket.size(), 1);
        EXPECT_EQ(bucket[0]->getId(), 0);
        EXPECT_FLOAT_EQ(bucket[0]->getRadius(), 6371.0f);
}

// Test PlanetRegistry getItems
TEST(PlanetRegistryTest, GetItems)
{
        PrePlanetRegistry registry;

        PrePlanetRegister registerInfo1(0x1234567890ABCDEF, 1);
        PrePlanet planet1(registerInfo1);
        planet1.setCenter(Vec3(0.0f, 0.0f, 0.0f));
        planet1.setRadius(6371.0f);
        planet1.setMass(5.97e24f);
        planet1.setId(0);

        PrePlanetRegister registerInfo2(0x1234567890ABCDEF, 2);
        PrePlanet planet2(registerInfo2);
        planet2.setCenter(Vec3(100.0f, 0.0f, 0.0f));
        planet2.setRadius(5000.0f);
        planet2.setMass(4.0e24f);
        planet2.setId(1);

        registry.registerItem(planet1);
        registry.registerItem(planet2);

        std::vector<PrePlanet*> planets = registry.getItems();
        EXPECT_EQ(planets.size(), 2);
}

// Test PlanetRegistry clear
TEST(PlanetRegistryTest, Clear)
{
        PrePlanetRegistry registry;

        PrePlanetRegister registerInfo1(0x1234567890ABCDEF, 1);
        PrePlanet planet1(registerInfo1);
        planet1.setCenter(Vec3(0.0f, 0.0f, 0.0f));
        planet1.setRadius(6371.0f);
        planet1.setMass(5.97e24f);
        planet1.setId(0);

        PrePlanetRegister registerInfo2(0x1234567890ABCDEF, 2);
        PrePlanet planet2(registerInfo2);
        planet2.setCenter(Vec3(100.0f, 0.0f, 0.0f));
        planet2.setRadius(5000.0f);
        planet2.setMass(4.0e24f);
        planet2.setId(1);

        registry.registerItem(planet1);
        registry.registerItem(planet2);

        EXPECT_EQ(registry.getCount(), 2);

        registry.clearBuckets();

        EXPECT_EQ(registry.getCount(), 0);
}

// Test PlanetRegistry with multiple planets
TEST(PlanetRegistryTest, MultiplePlanets)
{
        PrePlanetRegistry registry;

        PrePlanetRegister registerInfo1(0x1234567890ABCDEF, 1);
        PrePlanet planet1(registerInfo1);
        planet1.setCenter(Vec3(0.0f, 0.0f, 0.0f));
        planet1.setRadius(1000.0f);
        planet1.setMass(1.0e24f);
        planet1.setId(0);

        PrePlanetRegister registerInfo2(0x1234567890ABCDEF, 2);
        PrePlanet planet2(registerInfo2);
        planet2.setCenter(Vec3(100.0f, 0.0f, 0.0f));
        planet2.setRadius(1000.0f);
        planet2.setMass(1.0e24f);
        planet2.setId(1);

        PrePlanetRegister registerInfo3(0x1234567890ABCDEF, 3);
        PrePlanet planet3(registerInfo3);
        planet3.setCenter(Vec3(200.0f, 0.0f, 0.0f));
        planet3.setRadius(1000.0f);
        planet3.setMass(1.0e24f);
        planet3.setId(2);

        registry.registerItem(planet1);
        registry.registerItem(planet2);
        registry.registerItem(planet3);

        EXPECT_EQ(registry.getCount(), 3);
}

// Test PlanetRegistry property preservation
TEST(PlanetRegistryTest, PropertyPreservation)
{
        PrePlanetRegistry registry;

        PrePlanetRegister registerInfo(0x1234567890ABCDEF, 1);
        PrePlanet planet(registerInfo);
        planet.setCenter(Vec3(0.0f, 0.0f, 0.0f));
        planet.setRadius(6371.0f);
        planet.setMass(5.97e24f);
        planet.setId(0);
        planet.setGravity(15.0f);
        planet.setBaseTemperature(0.8f);
        planet.setBaseMoisture(0.3f);
        planet.setAtmosphereHeight(200.0f);
        planet.setRotationAxis(Vec3(1.0f, 0.0f, 0.0f));
        planet.setOrbitalVelocity(Vec3(1000.0f, 2000.0f, 3000.0f));
        planet.setRotationAngle(1.5f);
        planet.setRotationSpeed(0.0001f);

        registry.registerItem(planet);

        std::vector<PrePlanet*> items = registry.getItems();
        ASSERT_EQ(items.size(), 1);

        EXPECT_FLOAT_EQ(items[0]->getGravity(), 15.0f);
        EXPECT_FLOAT_EQ(items[0]->getBaseTemperature(), 0.8f);
        EXPECT_FLOAT_EQ(items[0]->getBaseMoisture(), 0.3f);
        EXPECT_FLOAT_EQ(items[0]->getAtmosphereHeight(), 200.0f);
        EXPECT_FLOAT_EQ(items[0]->getRotationAngle(), 1.5f);
        EXPECT_FLOAT_EQ(items[0]->getRotationSpeed(), 0.0001f);
}

// Test PlanetRegistry PreRegistry interface
TEST(PlanetRegistryTest, PreRegistryInterface)
{
        PrePlanetRegistry registry;

        PrePlanetRegister registerInfo(0x1234567890ABCDEF, 1);
        PrePlanet planet1(registerInfo);
        planet1.setCenter(Vec3(0.0f, 0.0f, 0.0f));
        planet1.setRadius(6371.0f);
        planet1.setMass(5.97e24f);
        planet1.setId(0);
        registry.registerItem(planet1);

        // Test getBucket
        PrePlanetBucketKey key = registry.genBucketKey(planet1);
        const std::vector<PrePlanet*>& bucket = registry.getBucket(key);
        EXPECT_EQ(bucket.size(), 1);
        EXPECT_EQ(bucket[0]->getId(), 0);

        // Test getItems
        std::vector<PrePlanet*> items = registry.getItems();
        EXPECT_EQ(items.size(), 1);
        EXPECT_EQ(items[0]->getId(), 0);
}

// Test noexcept specification
TEST(PlanetTest, NothrowConstruction)
{
        EXPECT_NO_THROW({ PrePlanetRegister registerInfo(0x1234567890ABCDEF, 1); PrePlanet planet(registerInfo); });
}

} // namespace rl
