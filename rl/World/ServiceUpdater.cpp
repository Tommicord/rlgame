import Rl.World.ServiceUpdater;

import Rl.Player.PlayerProvider;
import Rl.World.Time.TimeSystem;
import Rl.World.Skybox.SkyboxSystem;
import Rl.RayLog.Macro;

import <algorithm>;
import <memory>;
import <functional>;
import <string>;

namespace Rl::World
{

template <typename T>
TypedServiceUpdater<T>::TypedServiceUpdater(
    std::shared_ptr<T> service, std::function<void(T&)> updateFunc) :
    service(std::move(service)), updateFunc(std::move(updateFunc))
{ RayLog::LogDebug("ServiceUpdater", "TypedServiceUpdater initialized"); }

template <typename T> void TypedServiceUpdater<T>::Update()
{
  if (service && updateFunc)
  {
    updateFunc(*service);
  }
  else
  {
    RayLog::LogWarning("ServiceUpdater",
        "TypedServiceUpdater::Update called with null service or update function");
  }
}

template <typename T> std::string TypedServiceUpdater<T>::GetServiceName() const
{ return "TypedService"; }

TimeSystemUpdater::TimeSystemUpdater(
    std::shared_ptr<Time::TimeSystem> timeSystem, int64_t fragmentsPerUpdate) :
    timeSystem(std::move(timeSystem)), fragmentsPerUpdate(fragmentsPerUpdate)
{
  RayLog::LogInfo("ServiceUpdater",
      "TimeSystemUpdater initialized with fragmentsPerUpdate: %d", fragmentsPerUpdate);
}

void TimeSystemUpdater::Update()
{
  if (timeSystem)
  {
    timeSystem->Update(fragmentsPerUpdate);
  }
  else
  {
    RayLog::LogError(
        "ServiceUpdater", "TimeSystemUpdater::Update called with null TimeSystem");
  }
}

std::string TimeSystemUpdater::GetServiceName() const
{ return "TimeSystem"; }

SkyboxSystemUpdater::SkyboxSystemUpdater(
    std::shared_ptr<Skybox::SkyboxSystem> skyboxSystem) :
    skyboxSystem(std::move(skyboxSystem))
{ RayLog::LogInfo("ServiceUpdater", "SkyboxSystemUpdater initialized"); }

void SkyboxSystemUpdater::Update()
{
  if (skyboxSystem)
  {
    skyboxSystem->Update();
  }
  else
  {
    RayLog::LogError(
        "ServiceUpdater", "SkyboxSystemUpdater::Update called with null SkyboxSystem");
  }
}

std::string SkyboxSystemUpdater::GetServiceName() const
{ return "SkyboxSystem"; }

PlayerServicesUpdater::PlayerServicesUpdater()
{ RayLog::LogInfo("ServiceUpdater", "PlayerServicesUpdater initialized"); }

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
{ return "PlayerServices"; }

template class TypedServiceUpdater<Time::TimeSystem>;
template class TypedServiceUpdater<Skybox::SkyboxSystem>;

} // namespace Rl::World
