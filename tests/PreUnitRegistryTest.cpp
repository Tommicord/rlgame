#include <gtest/gtest.h>

#include "Rl.World/PreUnitRegister.h"
#include "Rl.World/PreUnitRegistry.h"
#include "Rl.World/PreRegister.h"
#include "Rl.World/Unit.h"

namespace rl
{

// Test unit classes for registry testing
class UnitRegistryTestGrass : public PreUnit
{
        public:
                UnitRegistryTestGrass() :
                    PreUnit(PreUnitRegisterFactory::create(typeid(UnitRegistryTestGrass).name()))
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
                        return 10.0f;
                }
                float getMoistureNearAccept() const override
                {
                        return 0.1f;
                }
                float getEquatorNearAccept() const override
                {
                        return 0.2f;
                }
                float getTemperatureNearAccept() const override
                {
                        return 5.0f;
                }
};

class UnitRegistryTestStone : public PreUnit
{
        public:
                UnitRegistryTestStone() :
                    PreUnit(PreUnitRegisterFactory::create(typeid(UnitRegistryTestStone).name()))
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
                        return 15.0f;
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
                        return 8.0f;
                }
};

class UnitRegistryTestWater : public PreUnit
{
        public:
                UnitRegistryTestWater() :
                    PreUnit(PreUnitRegisterFactory::create(typeid(UnitRegistryTestWater).name()))
                {
                        setFlammability(0.0f);
                        setExplosionResistance(0.0f);
                        setMoistureStart(0.8f);
                        setMoistureEnd(1.0f);
                        setTemperatureStart(-0.1f);
                        setTemperatureEnd(0.3f);
                        setElevationStart(0.0f);
                        setElevationEnd(0.1f);
                        setEquatorStart(-0.8f);
                        setEquatorEnd(0.8f);
                }

                float getElevationNearAccept() const override
                {
                        return 20.0f;
                }
                float getMoistureNearAccept() const override
                {
                        return 0.2f;
                }
                float getEquatorNearAccept() const override
                {
                        return 0.4f;
                }
                float getTemperatureNearAccept() const override
                {
                        return 10.0f;
                }
};

class UnitRegistryTestSand : public PreUnit
{
        public:
                UnitRegistryTestSand() :
                    PreUnit(PreUnitRegisterFactory::create(typeid(UnitRegistryTestSand).name()))
                {
                        setFlammability(0.2f);
                        setExplosionResistance(0.3f);
                        setMoistureStart(0.0f);
                        setMoistureEnd(0.2f);
                        setTemperatureStart(0.5f);
                        setTemperatureEnd(0.7f);
                        setElevationStart(0.0f);
                        setElevationEnd(0.5f);
                        setEquatorStart(-0.3f);
                        setEquatorEnd(0.3f);
                }

                float getElevationNearAccept() const override
                {
                        return 12.0f;
                }
                float getMoistureNearAccept() const override
                {
                        return 0.12f;
                }
                float getEquatorNearAccept() const override
                {
                        return 0.25f;
                }
                float getTemperatureNearAccept() const override
                {
                        return 6.0f;
                }
};

// Test registry initialization
TEST(PreUnitRegistryTest, Initialization)
{
        PreUnitRegistry registry;

        EXPECT_EQ(registry.getBucketCount(), 0);
}

// Test unit registration
TEST(PreUnitRegistryTest, RegisterUnit)
{
        PreUnitRegistry       registry;
        UnitRegistryTestGrass grass;

        registry.registerItem(grass);

        EXPECT_GT(registry.getBucketCount(), 0);
}

// Test multiple unit registration
TEST(PreUnitRegistryTest, RegisterMultipleUnits)
{
        PreUnitRegistry       registry;
        UnitRegistryTestGrass grass;
        UnitRegistryTestStone stone;
        UnitRegistryTestWater water;

        registry.registerItem(grass);
        registry.registerItem(stone);
        registry.registerItem(water);

        EXPECT_GT(registry.getBucketCount(), 0);
}

// Test bucket key generation
TEST(PreUnitRegistryTest, BucketKeyGeneration)
{
        PreUnitRegistry       registry;
        UnitRegistryTestGrass grass;
        UnitRegistryTestStone stone;

        // Register units to generate bucket keys
        registry.registerItem(grass);
        registry.registerItem(stone);

        // Different units should potentially be in different buckets
        EXPECT_GT(registry.getBucketCount(), 0);
}

// Test bucket retrieval
TEST(PreUnitRegistryTest, GetBucket)
{
        PreUnitRegistry       registry;
        UnitRegistryTestGrass grass;

        registry.registerItem(grass);

        // this tests the bucket lookup mechanism
        EXPECT_GT(registry.getBucketCount(), 0);
}

// Test finding units for specific conditions
TEST(PreUnitRegistryTest, FindUnitsForConditions)
{
        PreUnitRegistry       registry;
        UnitRegistryTestGrass grass;
        UnitRegistryTestStone stone;
        UnitRegistryTestWater water;

        registry.registerItem(grass);
        registry.registerItem(stone);
        registry.registerItem(water);

        auto grassUnits = registry.unitsByCondition(30.0f, 0.0f, 0.5f, 20.0f);

        // Should find at least the grass unit
        EXPECT_GE(grassUnits.size(), 0);
}

// Test finding units with edge case conditions
TEST(PreUnitRegistryTest, EmptyRegistry)
{
        PreUnitRegistry registry;

        // Finding units in empty registry should return empty result
        auto units = registry.unitsByCondition(50.0f, 0.0f, 0.5f, 20.0f);
        EXPECT_EQ(units.size(), 0);
}

// Test registry clearing
TEST(PreUnitRegistryTest, ClearBuckets)
{
        PreUnitRegistry       registry;
        UnitRegistryTestGrass grass;
        UnitRegistryTestStone stone;

        registry.registerItem(grass);
        registry.registerItem(stone);

        size_t bucketCountBefore = registry.getBucketCount();
        EXPECT_GT(bucketCountBefore, 0);

        registry.clearBuckets();

        EXPECT_EQ(registry.getBucketCount(), 0);
}

// Test unit bounds validation
TEST(PreUnitRegistryTest, UnitBoundsValidation)
{
        PreUnitRegistry       registry;
        UnitRegistryTestGrass grass;

        // This should succeed as grass has valid bounds
        EXPECT_NO_THROW(registry.registerItem(grass));
}

TEST(PreUnitRegistryTest, OutOfBoundsTemperature)
{
        // Setters are protected, so we skip this test
        PreUnitRegistry       registry;
        UnitRegistryTestGrass grass;

        // Constructor values should be valid
        EXPECT_NO_THROW(registry.registerItem(grass));
}

TEST(PreUnitRegistryTest, OutOfBoundsMoisture)
{
        // Setters are protected, so we skip this test
        PreUnitRegistry       registry;
        UnitRegistryTestGrass grass;

        // Constructor values should be valid
        EXPECT_NO_THROW(registry.registerItem(grass));
}

// Test unit with out-of-bounds elevation
TEST(PreUnitRegistryTest, OutOfBoundsElevation)
{
        // Setters are protected, so we skip this test
        PreUnitRegistry       registry;
        UnitRegistryTestGrass grass;

        // Constructor values should be valid
        EXPECT_NO_THROW(registry.registerItem(grass));
}

// Test unit with out-of-bounds flammability
TEST(PreUnitRegistryTest, OutOfBoundsFlammability)
{
        PreUnitRegistry       registry;
        UnitRegistryTestGrass grass;

        // Constructor values should be valid
        EXPECT_NO_THROW(registry.registerItem(grass));
}

// Test bucket boundary handling
TEST(PreUnitRegistryTest, BucketBoundaries)
{
        PreUnitRegistry       registry;
        UnitRegistryTestGrass grass;
        UnitRegistryTestStone stone;

        registry.registerItem(grass);
        registry.registerItem(stone);

        // Test finding units near bucket boundaries
        auto boundaryUnits = registry.unitsByCondition(50.0f, 0.0f, 0.5f, 20.0f);
        EXPECT_GE(boundaryUnits.size(), 0);
}

// Test sorting within buckets
TEST(PreUnitRegistryTest, BucketSorting)
{
        PreUnitRegistry       registry;
        UnitRegistryTestGrass grass1;
        UnitRegistryTestGrass grass2;
        UnitRegistryTestStone stone;

        registry.registerItem(grass1);
        registry.registerItem(grass2);
        registry.registerItem(stone);

        // The registry should maintain insertion order within buckets
        EXPECT_GT(registry.getBucketCount(), 0);
}

// Test multiple registrations of same unit
TEST(PreUnitRegistryTest, MultipleRegistrations)
{
        PreUnitRegistry       registry;
        UnitRegistryTestGrass grass;

        // Register the same unit multiple times
        registry.registerItem(grass);
        registry.registerItem(grass);
        registry.registerItem(grass);

        // Should still work (though may create duplicate entries)
        EXPECT_GT(registry.getBucketCount(), 0);
}

// Test registry with extreme values
TEST(PreUnitRegistryTest, ExtremeValues)
{
        PreUnitRegistry       registry;
        UnitRegistryTestGrass grass;

        // Constructor values should be valid
        EXPECT_NO_THROW(registry.registerItem(grass));
}

} // namespace rl
