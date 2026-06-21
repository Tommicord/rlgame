#include "rl/Client/State/UnitState.h"
#include "rl/World/Unit/UnitGrass.h"

namespace Rl::Providers
{

UnitModel::UnitModel(Game::VulkanContext& context) : StateModel(context)
{
  unitDrawable = std::make_shared<UnitStateDrawable>();
  unitVk       = std::make_unique<UnitStateDrawableVulkan>();

  // Create a default unit (e.g., grass)
  unit         = std::make_unique<World::UnitGrass>();
  unitResource = std::make_unique<UnitStateResource>(*unit);

  // Initialize Vulkan resources
  unitDrawable->OnCreate(*unitResource, *unitVk, context);
}

World::BaseUnit& UnitModel::GetObject()
{
  return *unit;
}

UnitStateResource& UnitModel::GetResource()
{
  return *unitResource;
}

UnitStateDrawable& UnitModel::GetDrawable()
{
  return *unitDrawable;
}

UnitStateDrawableVulkan& UnitModel::GetVulkanState()
{
  return *unitVk;
}

} // namespace Rl::Providers
