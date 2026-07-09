import Rl.World.ServiceUpdaterRegister;
import Rl.World.ServiceLocator;
import Rl.World.ServiceUpdaterRegistry;
import Rl.World.ServiceUpdater;
import Rl.RayLog.Macro;

import <cstdint>;
import <memory>;
import <string>;

namespace Rl::World
{

ServiceUpdaterRegister::ServiceUpdaterRegister(ServiceUpdaterRegistry& registry)
    : registry(registry)
{
}

void ServiceUpdaterRegister::RegisterTimeSystemUpdater(const std::string& name, int64_t fragmentsPerUpdate)
{
  auto timeSystem = WorldServiceLocator::GetTimeSystem();
  if (!timeSystem)
  {
    RayLog::LogWarning("ServiceUpdaterRegister", "TimeSystem not available in ServiceLocator, skipping registration");
    return;
  }
  auto updater = std::make_shared<TimeSystemUpdater>(timeSystem, fragmentsPerUpdate);
  registry.RegisterUpdater(name, updater);
  RayLog::LogInfo("ServiceUpdaterRegister", "Registered TimeSystem updater with fragmentsPerUpdate: %d", fragmentsPerUpdate);
}

void ServiceUpdaterRegister::RegisterSkyboxSystemUpdater(const std::string& name)
{
  auto skyboxSystem = WorldServiceLocator::GetSkyboxSystem();
  if (!skyboxSystem)
    return;
  
  auto updater = std::make_shared<SkyboxSystemUpdater>(skyboxSystem);
  registry.RegisterUpdater(name, updater);
}

void ServiceUpdaterRegister::RegisterPlayerServicesUpdater(const std::string& name)
{
  auto updater = std::make_shared<PlayerServicesUpdater>();
  registry.RegisterUpdater(name, updater);
}

void ServiceUpdaterRegister::RegisterCustomUpdater(const std::string& name, std::shared_ptr<ServiceUpdater> updater)
{
  if (updater)
  {
    registry.RegisterUpdater(name, updater);
  }
}

void ServiceUpdaterRegister::RegisterFromServiceLocator(int64_t timeFragmentsPerUpdate)
{
  // Register TimeSystem if available
  if (WorldServiceLocator::HasTimeSystem())
  {
    RegisterTimeSystemUpdater("TimeSystem", timeFragmentsPerUpdate);
  }
  // Register SkyboxSystem if available
  if (WorldServiceLocator::HasSkyboxSystem())
  {
    RegisterSkyboxSystemUpdater("SkyboxSystem");
  }
  // Register Player services
  RegisterPlayerServicesUpdater("PlayerServices");
}

void RegisterStandardServices(ServiceUpdaterRegistry& registry, std::int64_t timeFragmentsPerUpdate)
{
  ServiceUpdaterRegister registerer(registry);
  registerer.RegisterFromServiceLocator(timeFragmentsPerUpdate);
}

} // namespace Rl::World
