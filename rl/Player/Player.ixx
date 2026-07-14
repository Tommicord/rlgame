export module Rl.Player;

import Rl.Base.UserInput;
import Rl.Player.PlayerCamera;
import Rl.Player.PlayerController;
import Rl.Player.CameraController;
import Rl.Player.IPlayer;

namespace Rl::Player
{

export class Player final : public IPlayer
{
public:
    /* Constructs a Player instance */
    Player() noexcept;

    /* Creates and configures the Player camera controller */
    void CreateInputCamera() noexcept override;

    /* Creates and configures the Player controller */
    void CreateInputPlayer() noexcept override;

    ~Player() override;
};

} // namespace Rl::Player
