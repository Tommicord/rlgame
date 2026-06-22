#pragma once

#include <algorithm>
#include <array>
#include <memory>

#include "rl/Base/IUpdatable.h"
#include "rl/Base/Texture2.h"
#include "rl/World/Unit/UnitDynamicTexture.h"
#include "rl/World/Unit/UnitRegistry.h"
#include "rl/World/Unit/UnitResourceName.h"

namespace Rl::World
{

using namespace Rl::Providers;

enum class UnitType
{
  Visible, // Unit Visible
  NoVisible, // Unit No Visible
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
    requires(std::is_base_of_v<BaseUnit, std::decay_t<T>>)
  BaseUnit(T* type) noexcept : BaseUnit()
  {
    static Texture2 texture("unknown.png");
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

  /* Indicates the curvature of the unit polygons, negative or positive
   * Negative values indicates negative curvatures
   * Positive values indicates positive curvatures
   */
  float polCurve;

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
