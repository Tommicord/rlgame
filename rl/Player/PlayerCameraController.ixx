export module Rl.Player.CameraController;

import Rl.Player.PlayerCamera;
import Rl.Base.UserInput;

namespace Rl::Player
{

/* Controls camera rotation and movement from input */
export class PlayerCameraController final : public PlayerCameraInput
{
public:
  /* Constructs controller with camera reference */
  explicit PlayerCameraController(IPlayerCamera& camera) noexcept;

  /* Handles keyboard input for camera movement */
  void OnKeyEvent(const Input::KeyEvent& event) override;

  /* Handles mouse button input for camera control mode */
  void OnMouseButtonEvent(const Input::MouseButtonEvent& event) override;

  /* Handles mouse movement for camera rotation */
  void OnMouseMoveEvent(const Input::MouseMoveEvent& event) override;

  /* Handles mouse scroll for camera zoom */
  void OnMouseScrollEvent(const Input::MouseScrollEvent& event) override;

  /* Updates camera state from input */
  void Update() const;

private:
  /* The player camera object reference */
  IPlayerCamera& camera;

  bool           mouseCaptured{false};
  double         lastMouseX{0.0};
  double         lastMouseY{0.0};
  float          moveSpeed{5.0f};
  float          lookSensitivity{0.1f};
  bool           moveForward{false};
  bool           moveBackward{false};
  bool           moveLeft{false};
  bool           moveRight{false};
  bool           moveUp{false};
  bool           moveDown{false};
};

}