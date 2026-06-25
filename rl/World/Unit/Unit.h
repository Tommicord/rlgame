#pragma once

#include <algorithm>
#include <array>
#include <memory>

#include "rl/Base/IUpdatable.h"
#include "rl/Base/Texture2.h"
#include "rl/World/Unit/UnitDynamicTexture.h"
#include "rl/World/Unit/UnitPropertyStrategy.h"
#include "rl/World/Unit/UnitRegistry.h"
#include "rl/World/Unit/UnitResourceName.h"

namespace Rl::World
{

using namespace Rl::Providers;

enum class UnitType
{
  Visible, // Unit Visible
  NotVisible, // Unit No Visible
  Solid, // Is Unit Solid
  Liquid, // Is Unit Solid
};

class UnitTextureMaterial
{
  private:
  bool hasTexture = false;

  public:
  Texture2 *top, *down, *left, *right, *front, *back;
  UnitTextureMaterial() = default;
  UnitTextureMaterial(Texture2* top,
      Texture2*                 down,
      Texture2*                 left,
      Texture2*                 right,
      Texture2*                 front,
      Texture2*                 back);
  ~UnitTextureMaterial();
};

class BaseUnit : IUpdatable
{
  /* Internal Field: Stores the count of registered world units */
  using Registry                 = UnitRegistryKVPair<UnitResourceName, BaseUnit*>;
  inline static auto defaultName = std::vector({"Unknown"});
  inline static auto registry    = Registry(UnitResourceName(defaultName));

  public:
  /* Stores the properties of the world unit */
  struct
  {
    bool isSolid;
    bool isVisible;
  } props;

  /* Creates a basic WorldUnit, automatically registers the unit */
  template <typename T>
    requires IsDerivedUnit<T>
  BaseUnit(T* type) noexcept : BaseUnit()
  {
    static Texture2 texture("Unknown.png");
    using pair = UnitRegistryKVPair<UnitResourceName, BaseUnit*>;
    int id     = 1;
    if (pair::GetObjectById(id).has_value())
    {
      while (pair::GetObjectById(id).has_value())
      {
        id++;
      }
    }
    if (!pair::GetObjectById(id).has_value())
    {
      std::vector<const char*> v;
      v.reserve(1);
      v.push_back(typeid(T).name());
      UnitResourceName resourceName(v);
      BaseUnit*        base = type;
      registry.Register(id, resourceName, base);
    };
    textures        = std::make_unique<UnitTextureMaterial>();
    textures->front = textures->back = &texture;
    textures->left = textures->right = &texture;
    textures->top = textures->down = &texture;
  }
  /* Delete a world unit */
  ~BaseUnit() override;

  struct PolFence
  {
    float t, d, b, f; // Top, Down, Back, Front
  };
  [[nodiscard]]
  UnitTextureMaterial& GetMaterial() const;

  /* Sets the resistance against TNT of the unit */
  void SetResistance(float resistance);

  /* Sets the light quantity that emits the unit */
  void SetLightEmit(float emit);

  /* Sets the light quantity substraction for going the unit */
  void SetLightOpacity(float opacity);

  /* Sets the unit hardness, how many times wait to break the unit */
  void SetUnitHardness(float hardness);

  /* Sets the right Polygon Fence, the polygons for the rendering of the Unit */
  void SetPolFenceRight(PolFence& fence);

  /* Sets the left Polygon Fence, the polygons for the rendering of the Unit */
  void SetPolFenceLeft(PolFence& fence);

  /* Sets the Polygon curve */
  void SetPolCurve(float curve);

  /* Enables the collision of this unit */
  void EnableCollision();

  /* Disable the collision of this unit */
  void DisableCollision();

  /* Returns true if the collision is enabled for this unit */
  [[nodiscard]]
  bool IsCollisionEnabled() const;

  /* Returns true if the unit is visible */
  [[nodiscard]]
  bool IsVisible() const;

  /* Updates the Base Unit properties */
  void Update() override;

  /* Gets the light emission from property strategy */
  [[nodiscard]]
  virtual float GetStrategyLightEmit() const
  {
    return 0.0f;
  }

  /* Gets the light opacity from property strategy */
  [[nodiscard]]
  virtual float GetStrategyLightOpacity() const
  {
    return 1.0f;
  }

  /* Gets the ambient occlusion from property strategy */
  [[nodiscard]]
  virtual float GetStrategyAmbientOcclusion() const
  {
    return 1.0f;
  }

  /* Gets the roughness from property strategy */
  [[nodiscard]]
  virtual float GetStrategyRoughness() const
  {
    return 0.5f;
  }

  /* Gets the metallic from property strategy */
  [[nodiscard]]
  virtual float GetStrategyMetallic() const
  {
    return 0.0f;
  }

  /* Gets the albedo R from property strategy */
  [[nodiscard]]
  virtual float GetStrategyAlbedoR() const
  {
    return 1.0f;
  }

  /* Gets the albedo G from property strategy */
  [[nodiscard]]
  virtual float GetStrategyAlbedoG() const
  {
    return 1.0f;
  }

  /* Gets the albedo B from property strategy */
  [[nodiscard]]
  virtual float GetStrategyAlbedoB() const
  {
    return 1.0f;
  }

  /* Gets the dirtiness from property strategy */
  [[nodiscard]]
  virtual float GetStrategyDirtiness() const
  {
    return 0.0f;
  }

  /* Gets the wetness from property strategy */
  [[nodiscard]]
  virtual float GetStrategyWetness() const
  {
    return 0.0f;
  }

  /* Gets the temperature from property strategy */
  [[nodiscard]]
  virtual float GetStrategyTemperature() const
  {
    return 20.0f;
  }

  /* Gets the hardness from property strategy */
  [[nodiscard]]
  virtual float GetStrategyHardness() const
  {
    return 1.0f;
  }

  /* Gets the explosion resistance from property strategy */
  [[nodiscard]]
  virtual float GetStrategyExplosionResistance() const
  {
    return 0.0f;
  }

  /* Gets the transparency from property strategy */
  [[nodiscard]]
  virtual float GetStrategyTransparency() const
  {
    return 0.0f;
  }

  /* Gets the flammability from property strategy */
  [[nodiscard]]
  virtual float GetStrategyFlammability() const
  {
    return 0.0f;
  }

  /* Checks if unit is liquid from property strategy */
  [[nodiscard]]
  virtual bool IsStrategyLiquid() const
  {
    return false;
  }

  /* Checks if unit is gas from property strategy */
  [[nodiscard]]
  virtual bool IsStrategyGas() const
  {
    return false;
  }

  /* Checks if unit is solid from property strategy */
  [[nodiscard]]
  virtual bool IsStrategySolid() const
  {
    return true;
  }

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

  /* Indicates the vertical curvature of the unit polygons, negative or positive */
  float polCurveV;

  /* Indicates the horizontal curvature of the unit polygons, negative or positive */
  float polCurveH;

  /* Property to enable collision */
  bool mustCollide;

  /* Property to make the unit visible */
  bool mustVisible;

  /* Indicates how much this unit can resist explosions */
  float unitResistance;

  /* Indicates if the unit is translucent */
  bool translucent;

  private:
  BaseUnit() = default;
};

} // namespace Rl::World
