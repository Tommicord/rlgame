import Rl.World.ServiceUpdater;

import Rl.Player.PlayerProvider;
import Rl.World.Time.TimeSystem;
import Rl.World.Skybox.SkyboxSystem;
import Rl.World.Chunk.ChunkSystem;
import Rl.RayLog.Macro;

import <algorithm>;
import <memory>;
import <functional>;
import <string>;

namespace Rl::World
{

TimeSystemUpdater::TimeSystemUpdater(std::shared_ptr<Time::TimeSystem> timeSystem,
                                     const int64_t fragmentsPerUpdate) :
    timeSystem(std::move(timeSystem)), fragmentsPerUpdate(fragmentsPerUpdate)
{
    RayLog::LogInfo(GetRayLogTag(),
                    "TimeSystemUpdater initialized with fragmentsPerUpdate: %d",
                    fragmentsPerUpdate);
}

void TimeSystemUpdater::Update()
{
    if (timeSystem)
    {
        timeSystem->Update(fragmentsPerUpdate);
    }
    else
    {
        RayLog::LogError(GetRayLogTag(),
                         "TimeSystemUpdater::Update called with null TimeSystem");
    }
}

std::string TimeSystemUpdater::GetServiceName() const
{
    return "TimeSystem";
}

SkyboxSystemUpdater::SkyboxSystemUpdater(
    std::shared_ptr<Skybox::SkyboxSystem> skyboxSystem) :
    skyboxSystem(std::move(skyboxSystem))
{
    RayLog::LogInfo(GetRayLogTag(), "SkyboxSystemUpdater initialized");
}

void SkyboxSystemUpdater::Update()
{
    if (skyboxSystem)
    {
        skyboxSystem->Update();
    }
    else
    {
        RayLog::LogError(GetRayLogTag(),
                         "SkyboxSystemUpdater::Update called with null SkyboxSystem");
    }
}

std::string SkyboxSystemUpdater::GetServiceName() const
{
    return "SkyboxSystem";
}

ChunkSystemUpdater::ChunkSystemUpdater(std::shared_ptr<Chunk::ChunkSystem> chunkSystem) :
    chunkSystem(std::move(chunkSystem))
{
}

void ChunkSystemUpdater::Update()
{
    if (chunkSystem)
    {
        // TODO: Implement here the missing code
    }
    else
    {
        RayLog::LogError(GetRayLogTag(),
                         "ChunkSystemUpdater::Update called with null ChunkSystem");
    }
}

std::string ChunkSystemUpdater::GetServiceName() const
{
    return "ChunkSystem";
}

PlayerServicesUpdater::PlayerServicesUpdater()
{
    RayLog::LogInfo(GetRayLogTag(), "PlayerServicesUpdater initialized");
}

void PlayerServicesUpdater::Update()
{
    auto& player = Player::PlayerProvider::GetInstance();

    if (player.cameraControl)
    {
        player.cameraControl->Update();
    }

    if (player.playerControl)
    {
        player.playerControl->Update();
    }
}

std::string PlayerServicesUpdater::GetServiceName() const
{
    return "PlayerServices";
}

} // namespace Rl::World
