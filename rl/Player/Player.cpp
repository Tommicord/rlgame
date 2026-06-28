import Rl.Player;
import Rl.Player.IPlayer;
import Rl.Player.PlayerCamera;
import Rl.Player.PlayerProvider;
import Rl.Player.PlayerController;
import Rl.Player.CameraController;
import Rl.Base.UserInput;
import Rl.Base.Game;

import <memory>;

namespace Rl::Player
{

std::unique_ptr<IPlayer> PlayerProvider::player = nullptr;

IPlayer& PlayerProvider::GetInstance()
{
  if (!player)
  {
    player = std::make_unique<Player>();
  }
  return *player;
}

void Player::CreateInputCameraController() noexcept
{
  // Set camera aspect
  const auto  binding = Main::Game::GetInstance().GetMainBinding();
  const float aspect = static_cast<float>(binding.swapChainExtent.width) /
                       static_cast<float>(binding.swapChainExtent.height);
  camera->SetAspectRatio(aspect);

  // For now this
  camera->SetEyePosition(IPlayerCamera::Eye{GetXfp(), GetYfp(), GetZfp()});
  cameraControl = std::make_unique<PlayerCameraController>(*camera);
}

void Player::CreateInputPlayerController() noexcept
{
  playerControl = std::make_unique<PlayerController>(*this);
}

} // namespace Rl::Player
