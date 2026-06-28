import Rl.Player.CameraController;
import Rl.Player.PlayerCamera;
import Rl.Base.UserInput;

import <glm/glm.hpp>;
import <cmath>;

namespace Rl::Player
{

PlayerCameraController::PlayerCameraController(IPlayerCamera& camera) noexcept :
    PlayerCameraInput(*this), camera(camera)
{
}

void PlayerCameraController::OnKeyEvent(const Input::KeyEvent& event)
{
  if (event.action == Input::Action::Press || event.action == Input::Action::Repeat)
  {
    switch (event.key)
    {
    case Input::Key::W:
      moveForward = true;
      break;
    case Input::Key::S:
      moveBackward = true;
      break;
    case Input::Key::A:
      moveLeft = true;
      break;
    case Input::Key::D:
      moveRight = true;
      break;
    case Input::Key::Space:
      moveUp = true;
      break;
    case Input::Key::LeftShift:
      moveDown = true;
      break;
    default:
      break;
    }
  }
  else if (event.action == Input::Action::Release)
  {
    switch (event.key)
    {
    case Input::Key::W:
      moveForward = false;
      break;
    case Input::Key::S:
      moveBackward = false;
      break;
    case Input::Key::A:
      moveLeft = false;
      break;
    case Input::Key::D:
      moveRight = false;
      break;
    case Input::Key::Space:
      moveUp = false;
      break;
    case Input::Key::LeftShift:
      moveDown = false;
      break;
    default:
      break;
    }
  }
}

void PlayerCameraController::OnMouseButtonEvent(const Input::MouseButtonEvent& event)
{
  if (event.button == Input::MouseButton::Right)
  {
    mouseCaptured = (event.action == Input::Action::Press);
  }
}

void PlayerCameraController::OnMouseMoveEvent(const Input::MouseMoveEvent& event)
{
  if (!mouseCaptured)
  {
    lastMouseX = event.x;
    lastMouseY = event.y;
    return;
  }

  const double deltaX = event.x - lastMouseX;
  const double deltaY = event.y - lastMouseY;

  lastMouseX = event.x;
  lastMouseY = event.y;

  const auto yawOffset = static_cast<float>(deltaX * lookSensitivity);
  const auto pitchOffset = static_cast<float>(deltaY * lookSensitivity);

  IPlayerCamera::Eye rotation{};
  rotation.x = pitchOffset;
  rotation.y = yawOffset;
  rotation.z = 0.0;

  camera.SetRotateXYZ(rotation);
}

void PlayerCameraController::OnMouseScrollEvent(const Input::MouseScrollEvent& event)
{
  const auto  zoomOffset = static_cast<float>(event.yOffset * 0.1f);
  const float newZoom = glm::clamp(camera.zoom + zoomOffset, 0.1f, 10.0f);
  camera.SetZoom(newZoom);
}

void PlayerCameraController::Update() const
{
  glm::vec3 movement(0.0f);

  if (moveForward)
    movement.z += 1.0f;
  if (moveBackward)
    movement.z -= 1.0f;
  if (moveRight)
    movement.x += 1.0f;
  if (moveLeft)
    movement.x -= 1.0f;
  if (moveUp)
    movement.y += 1.0f;
  if (moveDown)
    movement.y -= 1.0f;

  if (glm::length(movement) > 0.0f)
  {
    movement = glm::normalize(movement) * moveSpeed;

    IPlayerCamera::Eye position{};
    position.x = camera.eye.x + movement.x;
    position.y = camera.eye.y + movement.y;
    position.z = camera.eye.z + movement.z;

    camera.SetEyePosition(position);
  }
}

} // namespace Rl::Player
