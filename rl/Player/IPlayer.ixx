export module Rl.Player.IPlayer;

import Rl.Player.PlayerCamera;
import Rl.Player.PlayerController;
import Rl.Player.CameraController;

import <memory>;

namespace Rl::Player
{

export class IPlayer
{
  public:
  /* The 64-bit coordinate type (1 meter = 1000)
   * To avoid floating-point math errors
   */
  using Coord = long long;

  /* Floating-point double precision type of the Coord */
  using FpCoord = double;

  /* The current X, Y, Z player coordinates */
  Coord cX, cY, cZ;

  /* The unique, current camera player */
  std::unique_ptr<IPlayerCamera> camera;

  /* The player camera controller for input handling */
  std::unique_ptr<PlayerCameraController> cameraControl;

  /* The player controller for input and state handling */
  std::unique_ptr<PlayerController> playerControl;

  /* Gets coordinate floating-point version of coordinates */
  [[nodiscard]]
  constexpr FpCoord GetFp(const Coord coord) const
  {
    Coord max = coord * 4294967LL;
    Coord bits = max >> 32;
    return static_cast<FpCoord>(bits); // Divide by 1000
  }

  /* Gets horizontal coordinate floating-point version of coordinates */
  [[nodiscard]]
  constexpr FpCoord GetXfp() const
  {
    return GetFp(cX);
  }

  /* Gets vertical coordinate floating-point version of coordinates */
  [[nodiscard]]
  constexpr FpCoord GetYfp() const
  {
    return GetFp(cY);
  }

  /* Gets depth coordinate floating-point version of coordinates */
  [[nodiscard]]
  constexpr FpCoord GetZfp() const
  {
    return GetFp(cZ);
  }

  IPlayer() noexcept
  {
    CreateInputCameraController();
    CreateInputPlayerController();

    cX = cY = cZ = 0L;
  }

  /* Creates and configures the Player camera controller */
  void CreateInputCameraController() noexcept;

  /* Creates and configures the Player controller */
  void CreateInputPlayerController() noexcept;

  IPlayer& operator=(const IPlayer& player) = delete;
  IPlayer& operator=(const IPlayer&& player) = delete;
};

} // namespace Rl::Player
