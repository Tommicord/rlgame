import Rl.World.ServiceUpdater;

import Rl.Player.PlayerProvider;
import Rl.World.Time.TimeSystem;
import Rl.World.Skybox.SkyboxSystem;

import <algorithm>;
import <memory>;
import <functional>;

namespace Rl::World
{

template<typename T>
TypedServiceUpdater<T>::TypedServiceUpdater(std::shared_ptr<T> service, std::function<void(T&)> updateFunc)
    : service(std::move(service)), updateFunc(std::move(updateFunc))
{
}

template<typename T>
void TypedServiceUpdater<T>::Update()
{
  if (service && updateFunc)
  {
    updateFunc(*service);
  }
}

template<typename T>
const char* TypedServiceUpdater<T>::GetServiceName() const
{
  return "TypedService";
}

TimeSystemUpdater::TimeSystemUpdater(std::shared_ptr<Time::TimeSystem> timeSystem, int64_t fragmentsPerUpdate)
    : timeSystem(std::move(timeSystem)), fragmentsPerUpdate(fragmentsPerUpdate)
{
}

void TimeSystemUpdater::Update()
{
  if (timeSystem)
  {
    timeSystem->Update(fragmentsPerUpdate);
  }
}

const char* TimeSystemUpdater::GetServiceName() const
{
  return "TimeSystem";
}

SkyboxSystemUpdater::SkyboxSystemUpdater(std::shared_ptr<Skybox::SkyboxSystem> skyboxSystem)
    : skyboxSystem(std::move(skyboxSystem))
{
}

void SkyboxSystemUpdater::Update()
{
  if (skyboxSystem)
  {
    skyboxSystem->Update();
  }
}

const char* SkyboxSystemUpdater::GetServiceName() const
{
  return "SkyboxSystem";
}

PlayerServicesUpdater::PlayerServicesUpdater()
{
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

const char* PlayerServicesUpdater::GetServiceName() const
{
  return "PlayerServices";
}

// Explicit template instantiations
template class TypedServiceUpdater<Time::TimeSystem>;
template class TypedServiceUpdater<Skybox::SkyboxSystem>;

} // namespace Rl::World
