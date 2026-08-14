#include "Rl.Base/GameResources.h"
#include "Rl.Base/GameTime.h"
#include "Rl.Player/Player.h"
#include "Rl.Player/PlayerController.h"

#include <cstddef>
#include <cstdint>

namespace rl
{

Player& GameResources::getPlayer()
{
  return playerResource.getPlayer();
}

void GameResources::updateCallback()
{
  fragmentTimeSystemResource.updateCallback();
  playerResource.updateCallback();
}

void PlayerInstance::updateCallback()
{
  player.updateState(GameTime::getDeltaTime());
}

void FragmentTimeInstance::updateCallback()
{
  constexpr uint32_t fragmentsToAdd = 1;
  fragmentTimeSystem.updateTime(fragmentsToAdd);
}

} // namespace rl
