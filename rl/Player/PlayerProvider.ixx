export module Rl.Player.PlayerProvider;

import Rl.Player;
import Rl.Player.IPlayer;

import <memory>;

namespace Rl::Player
{

/* Provides a unique current player instance for the Game instance */
export class PlayerProvider
{
public:
  /* Gets the singleton player instance */
  [[nodiscard]]
  static IPlayer& GetInstance();

  PlayerProvider(const PlayerProvider&)            = delete;
  PlayerProvider& operator=(const PlayerProvider&) = delete;
  PlayerProvider(PlayerProvider&&)                 = delete;
  PlayerProvider& operator=(PlayerProvider&&)      = delete;

private:
  PlayerProvider() = default;
  ~PlayerProvider() = default;

  /* The unknown player type provider */
  static std::unique_ptr<IPlayer> player;
};

} // namespace Rl::Player