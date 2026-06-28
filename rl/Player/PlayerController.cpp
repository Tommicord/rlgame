import Rl.Player.PlayerController;
import Rl.Player;
import Rl.Player.IPlayer;
import Rl.Base.UserInput;

namespace Rl::Player
{

void PlayerController::OnKeyEvent(const Input::KeyEvent& event)
{
  using namespace Input;

  if (event.action == Action::Press || event.action == Action::Repeat)
  {
    switch (event.key)
    {
    case Key::W:
      moveForward = true;
      break;
    case Key::S:
      moveBackward = true;
      break;
    case Key::A:
      moveLeft = true;
      break;
    case Key::D:
      moveRight = true;
      break;
    case Key::Space:
      moveUp = true;
      break;
    case Key::LeftShift:
      moveDown = true;
      break;
    default:
      break;
    }
  }
  else if (event.action == Action::Release)
  {
    switch (event.key)
    {
    case Key::W:
      moveForward = false;
      break;
    case Key::S:
      moveBackward = false;
      break;
    case Key::A:
      moveLeft = false;
      break;
    case Key::D:
      moveRight = false;
      break;
    case Key::Space:
      moveUp = false;
      break;
    case Key::LeftShift:
      moveDown = false;
      break;
    default:
      break;
    }
  }
}

void PlayerController::OnMouseButtonEvent(const Input::MouseButtonEvent& event)
{
  (void)event;
}

void PlayerController::OnMouseMoveEvent(const Input::MouseMoveEvent& event)
{
  (void)event;
}

void PlayerController::OnMouseScrollEvent(const Input::MouseScrollEvent& event)
{
  (void)event;
}

void PlayerController::Update() const
{
  constexpr double coordScale = 1000.0;

  if (moveForward)
    player.cZ += static_cast<IPlayer::Coord>(moveSpeed * coordScale);
  if (moveBackward)
    player.cZ -= static_cast<IPlayer::Coord>(moveSpeed * coordScale);
  if (moveRight)
    player.cX += static_cast<IPlayer::Coord>(moveSpeed * coordScale);
  if (moveLeft)
    player.cX -= static_cast<IPlayer::Coord>(moveSpeed * coordScale);
  if (moveUp)
    player.cY += static_cast<IPlayer::Coord>(moveSpeed * coordScale);
  if (moveDown)
    player.cY -= static_cast<IPlayer::Coord>(moveSpeed * coordScale);
}

} // namespace Rl::Player
