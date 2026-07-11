import Rl.World.ServiceUpdaterRegister;
import Rl.World.ServiceLocator;
import Rl.World.ServiceUpdaterRegistry;
import Rl.World.ServiceUpdater;
import Rl.RayLog.Macro;

import <cstdint>;
import <memory>;
import <string>;
import Rl.World.Time.TimeSystem;
import Rl.World.Skybox.SkyboxSystem;
import Rl.World.Chunk.ChunkSystem;
import Rl.World.Chunk.ChunkInRenderUnits;
#include "glm/gtx/iteration.hpp"

namespace Rl::World
{

ServiceUpdaterRegister::ServiceUpdaterRegister(ServiceUpdaterRegistry& registry) :
    registry(registry)
{
}

void ServiceUpdaterRegister::RegisterTimeSystemUpdater(const std::string& name,
                                                       int64_t fragmentsPerUpdate)
{
  auto timeSystem = WorldServiceLocator::GetTimeSystem();
  if (!timeSystem)
  {
    auto time = std::make_shared<Time::TimeSystem>();
    WorldServiceLocator::RegisterTimeSystem(time);
    timeSystem = WorldServiceLocator::GetTimeSystem();
  }
  const auto updater =
      std::make_shared<TimeSystemUpdater>(timeSystem, fragmentsPerUpdate);
  registry.RegisterUpdater(name, updater);
}

void ServiceUpdaterRegister::RegisterSkyboxSystemUpdater(const std::string& name)
{
  auto skyboxSystem = WorldServiceLocator::GetSkyboxSystem();
  if (!skyboxSystem)
  {
    const auto timeSystem = WorldServiceLocator::GetTimeSystem();
    if (!timeSystem)
    {
      return;
    }
    WorldServiceLocator::RegisterSkyboxSystem(
        std::make_shared<Skybox::SkyboxSystem>(*timeSystem));
    skyboxSystem = WorldServiceLocator::GetSkyboxSystem();
  }
  const auto updater = std::make_shared<SkyboxSystemUpdater>(skyboxSystem);
  registry.RegisterUpdater(name, updater);
}

void ServiceUpdaterRegister::RegisterPlayerServicesUpdater(const std::string& name)
{
  auto updater = std::make_shared<PlayerServicesUpdater>();
  registry.RegisterUpdater(name, updater);
}

void ServiceUpdaterRegister::RegisterChunkServicesUpdater(const std::string& name)
{
  auto chunkSystem = WorldServiceLocator::GetChunkSystem();
  if (!chunkSystem)
  {
    constexpr int renderDist = 128;
    auto          chunkStore{renderDist, renderDist, renderDist};
    auto          chunkSystemNew{*chunkStore, chunkStore->GetRenderDistance()};
    WorldServiceLocator::RegisterChunkSystem(
        std::make_shared<Chunk::ChunkSystem>(chunkSystemNew));
    chunkSystem = WorldServiceLocator::GetChunkSystem();
  }
  auto updater = std::make_shared<ChunkSystemUpdater>(chunkSystem);
  registry.RegisterUpdater(name, updater);
}

void ServiceUpdaterRegister::RegisterCustomUpdater(
    const std::string& name, std::shared_ptr<ServiceUpdater> updater)
{
  if (updater)
  {
    registry.RegisterUpdater(name, updater);
  }
}

void ServiceUpdaterRegister::RegisterFromServiceLocator(int64_t timeFragmentsPerUpdate)
{
  RegisterTimeSystemUpdater("TimeSystem", timeFragmentsPerUpdate);
  RegisterSkyboxSystemUpdater("SkyboxSystem");
  RegisterPlayerServicesUpdater("PlayerServices");
}

void RegisterStandardServices(ServiceUpdaterRegistry& registry,
                              std::int64_t            timeFragmentsPerUpdate)
{
  ServiceUpdaterRegister registerer(registry);
  registerer.RegisterFromServiceLocator(timeFragmentsPerUpdate);
}

} // namespace Rl::World
