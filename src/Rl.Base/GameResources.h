#ifndef RL_BASE_MAIN_GAME_RESOURCES_H
#define RL_BASE_MAIN_GAME_RESOURCES_H

#include "Rl.Player/Player.h"
#include "Rl.Player/PlayerController.h"
#include "Rl.World/FragmentTime.h"

namespace rl
{

class GameDevice;
class GameResources;
struct MainGame;
struct IMainGame;

class PlayerInstance final : public IPlayerProvider
{
        private:
                Player player;

        public:
                PlayerInstance() noexcept = default;
                Player& getPlayer() override
                {
                        return player;
                }
                const Player& getPlayer() const override
                {
                        return player;
                }
                void updateCallback();
};

class FragmentTimeInstance final : public IFragmentTimeSystemProvider
{
        private:
                FragmentTimeSystem fragmentTimeSystem;

        public:
                FragmentTimeInstance() = default;
                FragmentTimeSystem& getFragmentTimeSystem() override
                {
                        return fragmentTimeSystem;
                }
                const FragmentTimeSystem& getFragmentTimeSystem() const override
                {
                        return fragmentTimeSystem;
                }
                void updateCallback();
};

class GameResources final
{
                friend class MainGame;
                friend class GameDeviceInstance;
        private:
                PlayerInstance             playerResource;
                FragmentTimeInstance fragmentTimeSystemResource;

        public:
                GameResources() noexcept
                {
                }
                ~GameResources() = default;

                Player&             getPlayer();
                FragmentTimeSystem& getFragmentTimeSystem();

                void updateCallback();
};

} // namespace rl

#endif
