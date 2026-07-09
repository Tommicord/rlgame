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
  { RegisterDerived<UnitAir>(*this); }

  private:
  [[nodiscard]]
  unsigned short GetDerivedClassId() const override
  { return IUnitIdentifiable<UnitAir>::GetClassId(); }

  [[nodiscard]]
  std::string_view GetDerivedClassName() const override
  { return IUnitIdentifiable<UnitAir>::SimpleClassName(); }
};

} // namespace Rl::World
