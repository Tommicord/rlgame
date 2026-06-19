#pragma once

#include <algorithm>
#include <array>
#include <memory>

#include "UnitDynamicTexture.h"
#include "UnitResourceName.h"
#include "rl/Base/Texture2.h"

namespace Rl::World {

using namespace Rl::Providers;

enum class UnitType
{
    Visible,    // Unit Visible
    NoVisible,  // Unit No Visible
    Solid,      // Is Unit Solid
    Liquid,     // Is Unit Solid
};

struct UnitTextureMaterial
{
    std::unique_ptr<Texture2>
             top,
             down,
             left, right,
             front, back;
    ~UnitTextureMaterial();
};

template<class K, class V>
class UnitRegistryKVPair;

class AbstractUnit
{
    /* Internal Field: Stores the count of registered world units */
    static UnitRegistryKVPair<UnitResourceName, AbstractUnit*>& registry;
public:
    /* Stores the properties of the world unit */
    struct
    {
        bool isSolid;
        bool isVisible;
    } props;

    /* Creates a basic WorldUnit, automatically registers the unit */
    template<typename T>
        requires(std::is_base_of_v<AbstractUnit, std::decay_t<T>>)
    explicit AbstractUnit(T* type) noexcept;

    /* Delete a world unit */
    virtual ~AbstractUnit();

    struct PolFence
    {
        float t, d, b, f; // Top, Down, Back, Front
    };

    /* Sets the resistance against TNT of the unit */
    virtual void SetResistance(float resistance);

    /* Sets the light quantity that emits the unit */
    virtual void SetLightEmit(float emit);

    /* Sets the light quantity substraction for going the unit */
    virtual void SetLightOpacity(float opacity);

    /* Sets the unit hardness, how many times wait to break the unit */
    virtual void SetUnitHardness(float resistance);

    /* Sets the right Polygon Fence, the polygons for the rendering of the Unit */
    virtual void SetPolFenceRight(PolFence& fence);

    /* Sets the left Polygon Fence, the polygons for the rendering of the Unit */
    virtual void SetPolFenceLeft(PolFence& fence);

    /* Enables the collision of this unit */
    virtual void EnableCollision();

    /* Disable the collision of this unit */
    virtual void DisableCollision();

    /* Returns true if the collision is enabled for this unit */
    virtual bool IsCollisionEnabled();

    /* Returns true if the unit is visible */
    virtual bool IsVisible();
protected:
    /* Texture of the unit, back, front, left, right, bottom, top */
    std::unique_ptr<UnitTextureMaterial> textures;

    /* Width, Height, Depth of the unit */
    float inWidth, inHeight, inDepth;

    /* Amount of light emitted */
    float lightEmit;

    /* How much light is subtracted for going through this block */
    float lightOpacity;

    /* Indicates how many hits it takes to break a unit */
    float unitHardness;

    /*
     * The T, D, B, F means the direction
     * T = Top, D = Down, B = Back, F = Front
     * The r suffix means right
     *
     * Store Polygons for non-fixed Grid World */
    float polTr, polDr, polBr, polFr;

    /*
     * The T, D, B, F means the direction
     * T = Top, D = Down, B = Back, F = Front
     * The l suffix means right
     *
     * Store Polygons for non-fixed Grid World */
    float polTl, polDl, polBl, polFl;

    /* Property to enable collision */
    bool mustCollide;

    /* Property to make the unit visible */
    bool mustVisible;

    /* Indicates how much this unit can resist explosions */
    float unitResistance;

    /* Indicates if the unit is translucent */
    bool translucent;
private:
    AbstractUnit() = default;
};

template<class K, class V>
class UnitRegisters
{
    static std::vector<UnitRegistryKVPair<K, V>> registry;
public:
    /* Puts a Key-Value pair of Unit register */
    static void PutKV(UnitRegistryKVPair<K, V>& reg) noexcept;

    /* Returns the registry size */
    [[nodiscard]]
    size_t GetRegistrySize();

    /* Returns the current registry */
    [[nodiscard]]
    std::vector<UnitRegistryKVPair<K, V>>& GetRegistry();

    /* This is only to the KV Pair access the PutKV method
     * When a register is created, automatically
     * Adds the KV pair to the registry */
    friend class UnitRegistryKVPair<K, V>;
};

template<class K, class V>
class UnitRegistryKVPair
{
protected:
    K& regKey;
    V& regValue;
public:
    /* Creates a basic register of world unit */
    explicit UnitRegistryKVPair(K& defaultRegKey);

    /* Registers a Unit into the registry */
    void Register(int id, K& key, V& value);

    /* Gets the name we use to identify the object */
    [[nodiscard]]
    static std::optional<K> GetNameForObject(V& value);

    /* Gets the object from the name identifier */
    [[nodiscard]]
    static std::optional<K> GetObject(K name);

    /* Gets the object from the id identifier */
    [[nodiscard]]
    static std::optional<V> GetObjectById(int id);
};

} // namespace Rl::World
