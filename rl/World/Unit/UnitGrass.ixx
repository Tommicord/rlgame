export module Rl.World.Unit.UnitGrass;

import Rl.World.Unit;
import Rl.World.Unit.UnitRegister;

import <type_traits>;
import <string_view>;

namespace Rl::World
{

export class IUnitGrowable
{
  public:
  /* Destructs a IUnitGrowable object */
  virtual ~IUnitGrowable() = default;

  /* Returns if the Grass unit can grow */
  virtual bool InGrowState() = 0;

  /* Returns if the Grass unit can grow */
  virtual void Grow() = 0;
};

export class UnitGrass final : public IUnit,
                               public IUnitGrowable,
                               public IUnitIdentifiable<UnitGrass>
{
  public:
  UnitGrass() noexcept :
      IUnit(IUnitIdentifiable<UnitGrass>::GetClassId()), IUnitGrowable(), IUnitIdentifiable<UnitGrass>()
  {
    RegisterDerived<UnitGrass>(*this);
  }

  protected:
  bool UnitGrass::InGrowState() override
  {
    return true;
  }

  void UnitGrass::Grow() override
  {
  }

  private:
  [[nodiscard]]
  unsigned short GetDerivedClassId() const override
  {
    return IUnitIdentifiable<UnitGrass>::GetClassId();
  }

  [[nodiscard]]
  std::string_view GetDerivedClassName() const override
  {
    return IUnitIdentifiable<UnitGrass>::SimpleClassName();
  }
};

} // namespace Rl::World
