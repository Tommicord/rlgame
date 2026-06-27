export module Rl.World.Unit;

import Rl.Base.IUpdatable;
import Rl.Base.Texture2;
import Rl.World.Unit.UnitRegister;
import Rl.World.Unit.UnitDynamicTexture;
import Rl.World.Unit.UnitPropertyStrategy;
import Rl.World.Unit.UnitRegistry;
import Rl.World.Unit.UnitResourceName;

import <algorithm>;
import <string_view>;
import <array>;
import <memory>;
import <vector>;
import <cstring>;

namespace Rl::World
{

using namespace Rl::Providers;

export enum class UnitType {
  VISIBLE, // Unit Visible
  NVISIBLE, // Unit No Visible
  SOLID, // Is Unit Solid
  LIQUID, // Is Unit Solid
};

export class UnitTextureMaterial
{
  /* Stores if all fields are initialized */
  bool hasMaterial = false;

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

// Forward reference to default block
class UnitAir;

export class IUnit : public IUpdatable
{
  /* The registry value type */
  using RegistryV = IUnit*;

  /* Internal Field: Stores the count of registered world units */
  using Registry = UnitRegistryPair3<UnitResourceName, RegistryV>;
  inline static std::vector<std::string_view> defaultName = {std::string_view("Unknown")};
  inline static auto registry = Registry(UnitResourceName(defaultName));

  public:
  /* Stores the properties of the world unit */
  struct
  {
    bool isSolid;
    bool isVisible;
  } props;

  /* Creates a basic WorldUnit, automatically registers the unit */
  explicit IUnit(unsigned short id) noexcept : IUnit()
  {
    static Texture2 texture("rl.unit.Unknown");

    textures = std::make_unique<UnitTextureMaterial>();
    textures->front = textures->back = &texture;
    textures->left = textures->right = &texture;
    textures->top = textures->down = &texture;
  }
  IUnit(const IUnit& other) = delete;
  IUnit(const IUnit&& other) = delete;
  IUnit& operator=(const IUnit& other) = delete;

  /* Delete a world unit */
  ~IUnit() override;

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
  void SetPolFenceRight(const PolFence& fence);

  /* Sets the left Polygon Fence, the polygons for the rendering of the Unit */
  void SetPolFenceLeft(const PolFence& fence);

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
  /* Registers in compile-time a Unit id into the registry */
  template <typename Derived> static void RegisterDerived(Derived& ptr)
  {
    static unsigned short         id = IUnitIdentifiable<Derived>::GetStaticClassId();
    std::vector<std::string_view> v;
    v.reserve(1);
    v.push_back(IUnitIdentifiable<Derived>::SimpleClassName());
    UnitResourceName resourceName(v);
    // Store the pointer of the object
    // When used, this will be completely constructed
    // We pass *this from the derived
    // class to the RegisterDerived method
    IUnit* base = &ptr;
    registry.Register(id, resourceName, base);
  }

  /* Virtual function for derived classes to provide their class ID */
  [[nodiscard]]
  virtual unsigned short GetDerivedClassId() const = 0;

  /* Virtual function for derived classes to provide their class name */
  [[nodiscard]]
  virtual std::string_view GetDerivedClassName() const = 0;

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
  /* Private IUnit constructor, only to init the fields */
  IUnit() = default;
};

// Implementations for IUnit
IUnit::~IUnit()
{
  textures.reset();
}

UnitTextureMaterial& IUnit::GetMaterial() const
{
  return *textures;
}

void IUnit::SetResistance(const float resistance)
{
  this->unitResistance = resistance;
}

void IUnit::SetLightEmit(const float emit)
{
  this->lightEmit = emit;
}

void IUnit::SetLightOpacity(const float opacity)
{
  this->lightOpacity = opacity;
}

void IUnit::SetUnitHardness(const float hardness)
{
  this->unitHardness = hardness;
}

void IUnit::SetPolFenceRight(const PolFence& fence)
{
  std::memcpy(&polTr, &fence, sizeof(fence));
}

void IUnit::SetPolFenceLeft(const PolFence& fence)
{
  std::memcpy(&polTl, &fence, sizeof(fence));
}

void IUnit::SetPolCurve(const float curve)
{
  this->polCurveV = curve;
}

void IUnit::EnableCollision()
{
  mustCollide = true;
}

void IUnit::DisableCollision()
{
  mustCollide = false;
}

bool IUnit::IsCollisionEnabled() const
{
  return mustCollide;
}

bool IUnit::IsVisible() const
{
  return mustVisible;
}

void IUnit::Update()
{
}

} // namespace Rl::World
