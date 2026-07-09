export module Rl.Player.IPlayerController;

import Rl.Base.UserInput;

namespace Rl::Player
{

/* Interface for Player input handling */
export class IPlayerController
{
  public:
  virtual ~IPlayerController() = default;

  /* Handles keyboard input for player movement */
  virtual void OnKeyEvent(const Input::KeyEvent& event) = 0;

  /* Handles mouse button input for player actions */
  virtual void OnMouseButtonEvent(const Input::MouseButtonEvent& event) = 0;

  /* Handles mouse movement for player interaction */
  virtual void OnMouseMoveEvent(const Input::MouseMoveEvent& event) = 0;

  /* Handles mouse scroll for player actions */
  virtual void OnMouseScrollEvent(const Input::MouseScrollEvent& event) = 0;

  /* Updates player state from input */
  virtual void Update() const = 0;
};

} // namespace Rl::Player
