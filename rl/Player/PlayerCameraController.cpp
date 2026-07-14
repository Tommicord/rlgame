import Rl.Player.CameraController;
import Rl.Player.PlayerCamera;
import Rl.Base.UserInput;

import <glm/glm.hpp>;
import <cmath>;
import <algorithm>;

namespace Rl::Player
{

PlayerCameraController::PlayerCameraController(IPlayerCamera& camera) noexcept :
    IPlayerCameraController(*this), IInputObserver(*this), camera(camera)
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
}

void PlayerCameraController::OnMouseMoveEvent(const Input::MouseMoveEvent& event)
{
    const auto yawOffset   = static_cast<float>(event.x * lookSensitivity);
    const auto pitchOffset = static_cast<float>(event.y * lookSensitivity);

    if (event.x != 0.0 || event.y != 0.0)
    {
        IPlayerCamera::Eye rotation{};
        rotation.x = camera.pitch + pitchOffset;
        // Prevent gimbal lock
        rotation.x = std::clamp(rotation.x, -89.0, 89.0);
        rotation.y = camera.yaw + yawOffset;
        rotation.z = 0.0;
        camera.SetRotateXYZ(rotation);
    }
}

void PlayerCameraController::OnMouseScrollEvent(const Input::MouseScrollEvent& event)
{
    const float zoomFactor = 0.25f;
    const auto  zoomOffset = static_cast<float>(event.yOffset * zoomFactor);
    const float newZoom    = glm::clamp(camera.zoom + zoomOffset, 0.5f, 3.0f);
    camera.SetZoom(newZoom);
}

void PlayerCameraController::Update() const
{
    glm::vec3 movement(0.0f);
    glm::vec3 right = camera.GetRightVector();
    glm::vec3 up    = camera.GetUpVector();

    // Project right onto horizontal plane (remove Y component)
    glm::vec3 horizontalRight = glm::vec3(right.x, 0.0f, right.z);
    if (glm::length(horizontalRight) > 0.0f)
        horizontalRight = glm::normalize(horizontalRight);

    // Calculate horizontal front from horizontal right (cross with world up, negated)
    glm::vec3 horizontalFront =
        glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), horizontalRight));

    if (moveForward)
        movement += horizontalFront;
    if (moveBackward)
        movement -= horizontalFront;
    if (moveRight)
        movement += horizontalRight;
    if (moveLeft)
        movement -= horizontalRight;
    if (moveUp)
        movement -= up;
    if (moveDown)
        movement += up;

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
