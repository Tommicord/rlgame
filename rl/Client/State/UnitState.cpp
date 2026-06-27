import Rl.Client.State.UnitState;
import Rl.Client.State.CameraState;
import Rl.World.Unit;
import Rl.World.Unit.UnitGrass;
import Rl.World.Unit.UnitRegister;
import Rl.Base.Binding;
import Rl.Base.Game;

import <memory>;

namespace Rl::Providers
{

UnitModel::UnitModel(Game::MainBinding& context) : IStateModel(context)
{
  unitDrawable = std::make_shared<UnitStateDrawable>();
  unitBinding  = std::make_unique<UnitStateBinding>();
  unit         = std::make_unique<World::UnitGrass>();
  unitResource = std::make_unique<UnitStateResource>(*unit);
  unitDrawable->OnCreate(*unitResource, *unitBinding, context);
}

World::IUnit& UnitModel::GetObjectRef() const
{
  return *unit;
}

UnitStateResource& UnitModel::GetResource() const
{
  return *unitResource;
}

UnitStateDrawable& UnitModel::GetDrawable() const
{
  return *unitDrawable;
}

UnitStateBinding& UnitModel::GetBinding() const
{
  return *unitBinding;
}

} // namespace Rl::Providers
