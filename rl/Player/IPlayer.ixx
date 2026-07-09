export module Rl.Player.IPlayer;

import Rl.Player.PlayerCamera;
import Rl.Player.IPlayerCameraController;
import Rl.Player.IPlayerController;

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
  std::unique_ptr<IPlayerCameraController> cameraControl;

  /* The player controller for input and state handling */
  std::unique_ptr<IPlayerController> playerControl;

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
  { return GetFp(cX); }

  /* Gets vertical coordinate floating-point version of coordinates */
  [[nodiscard]]
  constexpr FpCoord GetYfp() const
  { return GetFp(cY); }

  /* Gets depth coordinate floating-point version of coordinates */
  [[nodiscard]]
  constexpr FpCoord GetZfp() const
  { return GetFp(cZ); }

  IPlayer() noexcept = default;

  /* Creates and configures the Player camera controller */
  virtual void CreateInputCameraController() noexcept = 0;

  /* Creates and configures the Player controller */
  virtual void CreateInputPlayerController() noexcept = 0;

  IPlayer& operator=(const IPlayer& player) = delete;
  IPlayer& operator=(const IPlayer&& player) = delete;

  virtual ~IPlayer() = default;
};

} // namespace Rl::Player
