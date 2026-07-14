export module Rl.Player.PlayerController;

import Rl.Base.UserInput;
import Rl.Player.IPlayerController;
import Rl.Player.IPlayer;

namespace Rl::Player
{

/* Controls player movement from input */
export class PlayerController final : public IPlayerController
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
    void Update() const override;

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
