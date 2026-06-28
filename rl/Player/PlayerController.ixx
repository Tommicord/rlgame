export module Rl.Player.PlayerController;

import Rl.Base.UserInput;

namespace Rl::Player
{

/* Forward reference to avoid circular dependencies */
class IPlayer;

/* Interface for Player input handling */
export struct PlayerInput : Input::IInputObserver
{
  PlayerInput() : IInputObserver(*this)
  {
  }
  void OnKeyEvent(const Input::KeyEvent& event) override = 0;
  void OnMouseButtonEvent(const Input::MouseButtonEvent& event) override = 0;
  void OnMouseMoveEvent(const Input::MouseMoveEvent& event) override = 0;
  void OnMouseScrollEvent(const Input::MouseScrollEvent& event) override = 0;
};

/* Controls player movement from input */
export class PlayerController final : public PlayerInput
{
  public:
  /* Constructs controller with player reference */
  explicit PlayerController(IPlayer& player) noexcept : player(player)
  {
  }

  /* Handles keyboard input for player movement */
  void OnKeyEvent(const Input::KeyEvent& event) override;

  /* Handles mouse button input for player actions */
  void OnMouseButtonEvent(const Input::MouseButtonEvent& event) override;

  /* Handles mouse movement for player interaction */
  void OnMouseMoveEvent(const Input::MouseMoveEvent& event) override;

  /* Handles mouse scroll for player actions */
  void OnMouseScrollEvent(const Input::MouseScrollEvent& event) override;

  /* Updates player state from input */
  void Update() const;

  private:
  /* The player object reference */
  IPlayer& player;

  /* The player movement speed */
  float moveSpeed{10.0f};

  /* The player is moving forward */
  bool moveForward{false};

  /* The player is moving backward */
  bool moveBackward{false};

  /* The player is moving left */
  bool moveLeft{false};

  /* The player is moving right */
  bool moveRight{false};

  /* The player is moving up */
  bool moveUp{false};

  /* The player is moving down */
  bool moveDown{false};
};

} // namespace Rl::Player
