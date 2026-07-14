import Rl.World.ServiceLocator;

import Rl.World.Time.TimeSystem;
import Rl.World.Skybox.SkyboxSystem;
import Rl.World.Chunk.ChunkSystem;

import <memory>;

namespace Rl::World
{

std::shared_ptr<Time::TimeSystem>     WorldServiceLocator::timeSystem   = nullptr;
std::shared_ptr<Skybox::SkyboxSystem> WorldServiceLocator::skyboxSystem = nullptr;
std::shared_ptr<Chunk::ChunkSystem>   WorldServiceLocator::chunkSystem  = nullptr;

void WorldServiceLocator::RegisterTimeSystem(std::shared_ptr<Time::TimeSystem> timeSystem)
{
    WorldServiceLocator::timeSystem = std::move(timeSystem);
}

void WorldServiceLocator::RegisterSkyboxSystem(
    std::shared_ptr<Skybox::SkyboxSystem> skyboxSystem)
{
    WorldServiceLocator::skyboxSystem = std::move(skyboxSystem);
}

void WorldServiceLocator::RegisterChunkSystem(
    std::shared_ptr<Chunk::ChunkSystem> chunkSystem)
{
    WorldServiceLocator::chunkSystem = std::move(chunkSystem);
}

std::shared_ptr<Time::TimeSystem> WorldServiceLocator::GetTimeSystem()
{
    return timeSystem;
}

std::shared_ptr<Skybox::SkyboxSystem> WorldServiceLocator::GetSkyboxSystem()
{
    return skyboxSystem;
}

std::shared_ptr<Chunk::ChunkSystem> WorldServiceLocator::GetChunkSystem()
{
    return chunkSystem;
}

bool WorldServiceLocator::HasTimeSystem()
{
    return timeSystem != nullptr;
}

bool WorldServiceLocator::HasSkyboxSystem()
{
    return skyboxSystem != nullptr;
}

void WorldServiceLocator::UnregisterTimeSystem()
{
    timeSystem.reset();
}

void WorldServiceLocator::UnregisterSkyboxSystem()
{
    skyboxSystem.reset();
}

void WorldServiceLocator::UnregisterChunkSystem()
{
    chunkSystem.reset();
}

void WorldServiceLocator::ClearAll()
{
    timeSystem.reset();
    skyboxSystem.reset();
    chunkSystem.reset();
}

} // namespace Rl::World
