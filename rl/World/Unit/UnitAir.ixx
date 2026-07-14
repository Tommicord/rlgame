export module Rl.World.Unit.UnitAir;

import Rl.World.Unit;
import Rl.World.Unit.UnitRegister;

import <type_traits>;
import <string_view>;

namespace Rl::World
{

export class UnitAir final : public IUnit,
                             public IUnitIdentifiable<UnitAir>
{
public:
    UnitAir() noexcept :
        IUnit(IUnitIdentifiable<UnitAir>::GetClassId()), IUnitIdentifiable<UnitAir>()
    {
        RegisterDerived<UnitAir>(*this);
    }

    static constexpr float LIGHT_EMIT            = 0.0f;
    static constexpr float LIGHT_OPACITY         = 0.0f;
    static constexpr float AMBIENT_OCCLUSION     = 0.0f;
    static constexpr float LIGHT_ABSORPTION      = 0.0f;
    static constexpr float LIGHT_SCATTERING      = 0.1f;
    static constexpr float ROUGHNESS             = 0.0f;
    static constexpr float METALLIC              = 0.0f;
    static constexpr float ALBEDO_R              = 0.9f;
    static constexpr float ALBEDO_G              = 0.95f;
    static constexpr float ALBEDO_B              = 1.0f;
    static constexpr float REFLECTIVITY          = 0.0f;
    static constexpr float REFRACTIVE_INDEX      = 1.0003f;
    static constexpr float DIRTINESS             = 0.0f;
    static constexpr float MOISTURE              = 0.5f;
    static constexpr float TEMPERATURE           = 20.0f;
    static constexpr float HUMIDITY              = 0.5f;
    static constexpr float HARDNESS              = 0.0f;
    static constexpr float EXPLOSION_RESISTANCE  = 0.0f;
    static constexpr float TRANSPARENCY          = 1.0f;
    static constexpr float EMISSIVE_INTENSITY    = 0.0f;
    static constexpr float SUBSURFACE_SCATTERING = 0.0f;
    static constexpr float FLAMMABILITY          = 0.0f;

    [[nodiscard]]
    static constexpr float GetLightEmit()
    {
        return LIGHT_EMIT;
    }

    [[nodiscard]]
    static constexpr float GetLightOpacity()
    {
        return LIGHT_OPACITY;
    }

    [[nodiscard]]
    static constexpr float GetAmbientOcclusion()
    {
        return AMBIENT_OCCLUSION;
    }

    [[nodiscard]]
    static constexpr float GetLightAbsorption()
    {
        return LIGHT_ABSORPTION;
    }

    [[nodiscard]]
    static constexpr float GetLightScattering()
    {
        return LIGHT_SCATTERING;
    }

    [[nodiscard]]
    static constexpr float GetRoughness()
    {
        return ROUGHNESS;
    }

    [[nodiscard]]
    static constexpr float GetMetallic()
    {
        return METALLIC;
    }

    [[nodiscard]]
    static constexpr float GetAlbedoR()
    {
        return ALBEDO_R;
    }

    [[nodiscard]]
    static constexpr float GetAlbedoG()
    {
        return ALBEDO_G;
    }

    [[nodiscard]]
    static constexpr float GetAlbedoB()
    {
        return ALBEDO_B;
    }

    [[nodiscard]]
    static constexpr float GetReflectivity()
    {
        return REFLECTIVITY;
    }

    [[nodiscard]]
    static constexpr float GetRefractiveIndex()
    {
        return REFRACTIVE_INDEX;
    }

    [[nodiscard]]
    static constexpr float GetDirtiness()
    {
        return DIRTINESS;
    }

    [[nodiscard]]
    static constexpr float GetMoisture()
    {
        return MOISTURE;
    }

    [[nodiscard]]
    static constexpr float GetTemperature()
    {
        return TEMPERATURE;
    }

    [[nodiscard]]
    static constexpr float GetHumidity()
    {
        return HUMIDITY;
    }

    [[nodiscard]]
    static constexpr float GetHardness()
    {
        return HARDNESS;
    }

    [[nodiscard]]
    static constexpr float GetExplosionResistance()
    {
        return EXPLOSION_RESISTANCE;
    }

    [[nodiscard]]
    static constexpr float GetTransparency()
    {
        return TRANSPARENCY;
    }

    [[nodiscard]]
    static constexpr float GetEmissiveIntensity()
    {
        return EMISSIVE_INTENSITY;
    }

    [[nodiscard]]
    static constexpr float GetSubsurfaceScattering()
    {
        return SUBSURFACE_SCATTERING;
    }

    [[nodiscard]]
    static constexpr float GetFlammability()
    {
        return FLAMMABILITY;
    }

    [[nodiscard]]
    static constexpr bool IsLiquid()
    {
        return false;
    }

    [[nodiscard]]
    static constexpr bool IsGas()
    {
        return true;
    }

    [[nodiscard]]
    static constexpr bool IsSolid()
    {
        return false;
    }

    [[nodiscard]]
    unsigned short GetDerivedClassId() const override
    {
        return IUnitIdentifiable<UnitAir>::GetClassId();
    }

    [[nodiscard]]
    std::string_view GetDerivedClassName() const override
    {
        return IUnitIdentifiable<UnitAir>::SimpleClassName();
    }
};

} // namespace Rl::World
