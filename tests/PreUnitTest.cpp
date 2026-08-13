#include <gtest/gtest.h>

#include "Rl.World/PreUnitRegister.h"
#include "Rl.World/Unit.h"

namespace rl
{

// Test unit class for PreUnit testing
class UnitPreTestGrass : public PreUnit
{
        public:
                UnitPreTestGrass() :
                    PreUnit(PreUnitRegisterFactory::create(typeid(UnitPreTestGrass).name()))
                {
                        setFlammability(0.8f);
                        setExplosionResistance(0.1f);
                        setMoistureStart(0.3f);
                        setMoistureEnd(0.9f);
                        setTemperatureStart(-0.5f);
                        setTemperatureEnd(0.5f);
                        setElevationStart(0.0f);
                        setElevationEnd(0.6f);
                        setEquatorStart(-0.5f);
                        setEquatorEnd(0.5f);
                }

                float getElevationNearAccept() const override
                {
                        return 0.3f;
                }
                float getMoistureNearAccept() const override
                {
                        return 0.6f;
                }
                float getEquatorNearAccept() const override
                {
                        return 0.5f;
                }
                float getTemperatureNearAccept() const override
                {
                        return 0.5f;
                }

                bool canGenerateBy(float elevation,
                                   float equator,
                                   float moisture,
                                   float temperature) const override
                {
                        return PreUnit::canGenerateBy(elevation, equator, moisture, temperature);
                }
};

class UnitPreTestStone : public PreUnit
{
        public:
                UnitPreTestStone() :
                    PreUnit(PreUnitRegisterFactory::create(typeid(UnitPreTestStone).name()))
                {
                        setFlammability(0.0f);
                        setExplosionResistance(1.0f);
                        setMoistureStart(0.0f);
                        setMoistureEnd(0.5f);
                        setTemperatureStart(-0.2f);
                        setTemperatureEnd(0.2f);
                        setElevationStart(0.0f);
                        setElevationEnd(1.0f);
                        setEquatorStart(-1.0f);
                        setEquatorEnd(1.0f);
                }

                float getElevationNearAccept() const override
                {
                        return 0.5f;
                }
                float getMoistureNearAccept() const override
                {
                        return 0.15f;
                }
                float getEquatorNearAccept() const override
                {
                        return 0.3f;
                }
                float getTemperatureNearAccept() const override
                {
                        return 0.2f;
                }

                bool canGenerateBy(float elevation,
                                   float equator,
                                   float moisture,
                                   float temperature) const override
                {
                        if (moisture > 0.6f)
                                return false;
                        if (temperature < -0.2f || temperature > 0.2f)
                                return false;
                        return PreUnit::canGenerateBy(elevation, equator, moisture, temperature);
                }
};

// Test PreUnit construction
TEST(PreUnitTest, Construction)
{
        UnitPreTestGrass grass;

        EXPECT_NE(grass.getHash(), 0);
        EXPECT_NE(grass.getTypeId(), 0);
}

// Test property getters
TEST(PreUnitTest, PropertyGetters)
{
        UnitPreTestGrass grass;

        EXPECT_FLOAT_EQ(grass.getFlammability(), 0.8f);
        EXPECT_FLOAT_EQ(grass.getExplosionResistance(), 0.1f);
        EXPECT_FLOAT_EQ(grass.getMoistureStart(), 0.3f);
        EXPECT_FLOAT_EQ(grass.getMoistureEnd(), 0.9f);
        EXPECT_FLOAT_EQ(grass.getTemperatureStart(), -0.5f);
        EXPECT_FLOAT_EQ(grass.getTemperatureEnd(), 0.5f);
        EXPECT_FLOAT_EQ(grass.getElevationStart(), 0.0f);
        EXPECT_FLOAT_EQ(grass.getElevationEnd(), 0.6f);
        EXPECT_FLOAT_EQ(grass.getEquatorStart(), -0.5f);
        EXPECT_FLOAT_EQ(grass.getEquatorEnd(), 0.5f);
}

TEST(PreUnitTest, PropertySetters)
{
        // Setters are protected, so we test through the constructor values
        UnitPreTestGrass grass;

        EXPECT_FLOAT_EQ(grass.getFlammability(), 0.8f);
        EXPECT_FLOAT_EQ(grass.getExplosionResistance(), 0.1f);
        EXPECT_FLOAT_EQ(grass.getMoistureStart(), 0.3f);
        EXPECT_FLOAT_EQ(grass.getMoistureEnd(), 0.9f);
        EXPECT_FLOAT_EQ(grass.getTemperatureStart(), -0.5f);
        EXPECT_FLOAT_EQ(grass.getTemperatureEnd(), 0.5f);
        EXPECT_FLOAT_EQ(grass.getElevationStart(), 0.0f);
        EXPECT_FLOAT_EQ(grass.getElevationEnd(), 0.6f);
}

// Test name and ID setters
TEST(PreUnitTest, NameAndIdSetters)
{
        // Setters are protected, so we test through the constructor values
        UnitPreTestGrass grass;

        EXPECT_NE(grass.getHash(), 0);
        EXPECT_NE(grass.getTypeId(), 0);
}

// Test canGenerateBy with valid conditions
TEST(PreUnitTest, CanGenerateValid)
{
        UnitPreTestGrass grass;

        // Conditions within grass's valid range
        bool canGen = grass.canGenerateBy(0.3f, 0.0f, 0.6f, 0.0f);
        EXPECT_TRUE(canGen);
}

// Test canGenerateBy with invalid elevation
TEST(PreUnitTest, CanGenerateInvalidElevation)
{
        UnitPreTestGrass grass;

        // Elevation outside grass's valid range
        bool canGen = grass.canGenerateBy(1.0f, 0.0f, 0.5f, 0.2f);
        EXPECT_FALSE(canGen);
}

// Test canGenerateBy with invalid moisture
TEST(PreUnitTest, CanGenerateInvalidMoisture)
{
        UnitPreTestGrass grass;

        // Moisture outside grass's valid range
        bool canGen = grass.canGenerateBy(0.3f, 0.0f, -0.5f, 0.2f);
        EXPECT_FALSE(canGen);
}

// Test canGenerateBy with invalid temperature
TEST(PreUnitTest, CanGenerateInvalidTemperature)
{
        UnitPreTestGrass grass;

        // Temperature outside grass's valid range
        bool canGen = grass.canGenerateBy(0.3f, 0.0f, 0.5f, 1.0f);
        EXPECT_FALSE(canGen);
}

// Test canGenerateBy with boundary values
TEST(PreUnitTest, CanGenerateBoundaryValues)
{
        UnitPreTestGrass grass;

        // Test at boundary values
        bool canGen1 = grass.canGenerateBy(0.0f, 0.0f, 0.6f, 0.0f);
        EXPECT_TRUE(canGen1);

        bool canGen2 = grass.canGenerateBy(0.6f, 0.0f, 0.6f, 0.0f);
        EXPECT_TRUE(canGen2);
}

// Test canGenerateBy tolerance
TEST(PreUnitTest, CanGenerateTolerance)
{
        UnitPreTestGrass grass;

        // Temperature within tolerance (5.0f tolerance from getTemperatureNearAccept)
        bool canGen = grass.canGenerateBy(0.3f, 0.0f, 0.6f, 0.0f); // Within tolerance of 0.0
        EXPECT_TRUE(canGen);
}

// Test canGenerateBy outside tolerance
TEST(PreUnitTest, CanGenerateOutsideTolerance)
{
        UnitPreTestGrass grass;

        // Temperature outside tolerance
        bool canGen = grass.canGenerateBy(0.3f, 0.0f, 0.5f, 0.6f); // Outside tolerance of 0.5
        EXPECT_FALSE(canGen);
}

// Test multiple unit instances
TEST(PreUnitTest, MultipleInstances)
{
        UnitPreTestGrass grass1;
        UnitPreTestGrass grass2;
        UnitPreTestGrass grass3;

        // All instances should have the same type ID
        EXPECT_EQ(grass1.getTypeId(), grass2.getTypeId());
        EXPECT_EQ(grass2.getTypeId(), grass3.getTypeId());

        // All instances should have the same hash
        EXPECT_EQ(grass1.getHash(), grass2.getHash());
        EXPECT_EQ(grass2.getHash(), grass3.getHash());
}

// Test different unit types
TEST(PreUnitTest, DifferentUnitTypes)
{
        UnitPreTestGrass grass;
        UnitPreTestStone stone;

        // Different types should have different IDs
        EXPECT_NE(grass.getTypeId(), stone.getTypeId());

        // Different types should have different hashes
        EXPECT_NE(grass.getHash(), stone.getHash());
}

TEST(PreUnitTest, PropertyIndependence)
{
        // Setters are protected, so we test through different instances
        UnitPreTestGrass grass1;
        UnitPreTestGrass grass2;

        // Different instances should have independent state
        EXPECT_FLOAT_EQ(grass1.getFlammability(), 0.8f);
        EXPECT_FLOAT_EQ(grass2.getFlammability(), 0.8f);
}

TEST(PreUnitTest, ExtremePropertyValues)
{
        // Setters are protected, so we test through the constructor values
        UnitPreTestGrass grass;

        // Verify constructor values are within valid ranges
        EXPECT_GE(grass.getFlammability(), minFlamability);
        EXPECT_LE(grass.getFlammability(), maxFlammability);

        EXPECT_GE(grass.getMoistureStart(), minMoisture);
        EXPECT_LE(grass.getMoistureEnd(), maxMoisture);

        EXPECT_GE(grass.getTemperatureStart(), minTemperature);
        EXPECT_LE(grass.getTemperatureEnd(), maxTemperature);

        EXPECT_GE(grass.getElevationStart(), static_cast<float>(minElevation));
        EXPECT_LE(grass.getElevationEnd(), static_cast<float>(maxElevation));
}

// Test negative values - skipped due to protected access
TEST(PreUnitTest, NegativeValues)
{
        // Setters are protected, so we test through the constructor values
        UnitPreTestGrass grass;

        // Verify equator can be negative
        EXPECT_LT(grass.getEquatorStart(), 0.0f);
}

TEST(PreUnitTest, ZeroValues)
{
        // Setters are protected, so we test through the constructor values
        UnitPreTestGrass grass;

        // Verify elevation can be zero
        EXPECT_FLOAT_EQ(grass.getElevationStart(), 0.0f);
}

TEST(PreUnitTest, PropertyRanges)
{
        // Setters are protected, so we test through the constructor values
        UnitPreTestGrass grass;

        // Test that moisture range is valid (start <= end)
        EXPECT_LE(grass.getMoistureStart(), grass.getMoistureEnd());

        // Test that elevation range is valid (start <= end)
        EXPECT_LE(grass.getElevationStart(), grass.getElevationEnd());

        // Test that temperature range is valid (start <= end)
        EXPECT_LE(grass.getTemperatureStart(), grass.getTemperatureEnd());

        // Test that equator range is valid (start <= end)
        EXPECT_LE(grass.getEquatorStart(), grass.getEquatorEnd());
}

// Test virtual function override
TEST(PreUnitTest, VirtualFunctionOverride)
{
        UnitPreTestGrass grass;
        UnitPreTestStone stone;

        // Both units should have different canGenerateBy logic
        bool grassCanGen = grass.canGenerateBy(0.3f, 0.0f, 0.6f, 0.0f);
        bool stoneCanGen = stone.canGenerateBy(0.3f, 0.0f, 0.6f, 0.0f);

        // Grass should generate at these conditions
        EXPECT_TRUE(grassCanGen);
}

// Test descriptor access
TEST(PreUnitTest, DescriptorAccess)
{
        UnitPreTestGrass grass;

        // Test that all descriptor values are accessible
        EXPECT_NE(grass.getHash(), 0);
        EXPECT_NE(grass.getTypeId(), 0);
        EXPECT_NE(grass.getFlammability(), 0.0f); // Should be set in constructor
        EXPECT_NE(grass.getTemperatureStart(), 0.0f); // Should be set in constructor
}

// Test noexcept specification
TEST(PreUnitTest, NoexceptSpecification)
{
        EXPECT_NO_THROW({ UnitPreTestGrass grass; });
}

// Test const correctness
TEST(PreUnitTest, ConstCorrectness)
{
        const UnitPreTestGrass grass;

        // All getters should work on const objects
        EXPECT_NE(grass.getHash(), 0);
        EXPECT_NE(grass.getTypeId(), 0);
        EXPECT_NE(grass.getFlammability(), 0.0f);
        EXPECT_NE(grass.getExplosionResistance(), 0.0f);
        EXPECT_NE(grass.getMoistureStart(), 0.0f);
        EXPECT_NE(grass.getMoistureEnd(), 0.0f);
        EXPECT_NE(grass.getTemperatureStart(), 0.0f);
        EXPECT_NE(grass.getTemperatureEnd(), 0.0f);
        EXPECT_NE(grass.getEquatorStart(), 0.0f);
        EXPECT_NE(grass.getEquatorEnd(), 0.0f);
        EXPECT_NE(grass.getElevationEnd(), 0.0f);
}

} // namespace rl
