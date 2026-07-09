import Rl.World.ServiceLocator;

import Rl.World.Time.TimeSystem;
import Rl.World.Skybox.SkyboxSystem;

import <memory>;

namespace Rl::World
{

// Static member initialization
std::shared_ptr<Time::TimeSystem>     WorldServiceLocator::timeSystem = nullptr;
std::shared_ptr<Skybox::SkyboxSystem> WorldServiceLocator::skyboxSystem = nullptr;

void WorldServiceLocator::RegisterTimeSystem(std::shared_ptr<Time::TimeSystem> timeSystem)
{ WorldServiceLocator::timeSystem = std::move(timeSystem); }

void WorldServiceLocator::RegisterSkyboxSystem(
    std::shared_ptr<Skybox::SkyboxSystem> skyboxSystem)
{ WorldServiceLocator::skyboxSystem = std::move(skyboxSystem); }

std::shared_ptr<Time::TimeSystem> WorldServiceLocator::GetTimeSystem()
{ return timeSystem; }

std::shared_ptr<Skybox::SkyboxSystem> WorldServiceLocator::GetSkyboxSystem()
{ return skyboxSystem; }

bool WorldServiceLocator::HasTimeSystem()
{ return timeSystem != nullptr; }

bool WorldServiceLocator::HasSkyboxSystem()
{ return skyboxSystem != nullptr; }

void WorldServiceLocator::UnregisterTimeSystem()
{ timeSystem.reset(); }

void WorldServiceLocator::UnregisterSkyboxSystem()
{ skyboxSystem.reset(); }

void WorldServiceLocator::ClearAll()
{
  timeSystem.reset();
  skyboxSystem.reset();
}

} // namespace Rl::World
