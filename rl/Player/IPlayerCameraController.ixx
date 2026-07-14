export module Rl.Player.IPlayerCameraController;

import Rl.Player.PlayerCamera;
import Rl.Base.UserInput;

namespace Rl::Player
{

/* Interface for Player camera controller */
export class IPlayerCameraController
{
public:
    virtual ~IPlayerCameraController() = default;

    /* Handles keyboard input for camera movement */
    virtual void OnKeyEvent(const Input::KeyEvent& event) = 0;

    /* Handles mouse button input for camera control mode */
    virtual void OnMouseButtonEvent(const Input::MouseButtonEvent& event) = 0;

    /* Handles mouse movement for camera rotation */
    virtual void OnMouseMoveEvent(const Input::MouseMoveEvent& event) = 0;

    /* Handles mouse scroll for camera zoom */
    virtual void OnMouseScrollEvent(const Input::MouseScrollEvent& event) = 0;

    /* Updates camera state from input */
    virtual void Update() const = 0;
};

} // namespace Rl::Player
