export module Rl.World.Unit.UnitTextureFactory;

import Rl.World.Unit.UnitResourceName;
import Rl.World.Unit;
import Rl.RayLog.Macro;

import <string>;
import <vector>;

namespace Rl::World
{

export class UnitTextureFactory
{
protected:
    /* The RayLog tag for logging */
    static constexpr auto RAY_LOG_TAG = "Unit:TextureFactory";

    /* The fallback texture length (4096 = 16 * 16 * 4) */
    static constexpr int DEFAULT_TEXTURE2_LENGTH = 4096;

    /* Checks if a Unit resource name identifier like "rl.unit.UnitGrass" is valid */
    static bool IsValidResourceName(const UnitResourceName& name);

    /* Gets the default unit texture when fails the texture loading of a unit */
    static std::array<uint8_t, DEFAULT_TEXTURE2_LENGTH> GetDefaultUnitTexture();

public:
    /* Provides automatically the texture for a Unit from it resource name */
    static void FromUnit(unsigned short unit);

protected:
    explicit UnitTextureFactory() = delete;
};

} // namespace Rl::World
