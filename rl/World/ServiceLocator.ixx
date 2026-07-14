export module Rl.World.ServiceLocator;

import Rl.World.Time.TimeSystem;
import Rl.World.Skybox.SkyboxSystem;
import Rl.World.Chunk.ChunkSystem;

import <memory>;
import <stdexcept>;

namespace Rl::World
{

/* Service Locator for accessing world systems */
export class WorldServiceLocator
{
public:
    /* Register Time System */
    static void RegisterTimeSystem(std::shared_ptr<Time::TimeSystem> timeSystem);

    /* Register Skybox System */
    static void RegisterSkyboxSystem(std::shared_ptr<Skybox::SkyboxSystem> skyboxSystem);

    /* Register Chunk System */
    static void RegisterChunkSystem(std::shared_ptr<Chunk::ChunkSystem> chunkSystem);

    /* Get Time System */
    [[nodiscard]]
    static std::shared_ptr<Time::TimeSystem> GetTimeSystem();

    /* Get Skybox System */
    [[nodiscard]]
    static std::shared_ptr<Skybox::SkyboxSystem> GetSkyboxSystem();

    /* Get Chunk System */
    [[nodiscard]]
    static std::shared_ptr<Chunk::ChunkSystem> GetChunkSystem();

    /* Check if Time System is registered */
    [[nodiscard]]
    static bool HasTimeSystem();

    /* Check if Skybox System is registered */
    [[nodiscard]]
    static bool HasSkyboxSystem();

    /* Unregister Time System */
    static void UnregisterTimeSystem();

    /* Unregister Skybox System */
    static void UnregisterSkyboxSystem();

    /* Unregister Chunk System */
    static void UnregisterChunkSystem();

    /* Clear all registered services */
    static void ClearAll();

private:
    static std::shared_ptr<Time::TimeSystem>     timeSystem;
    static std::shared_ptr<Skybox::SkyboxSystem> skyboxSystem;
    static std::shared_ptr<Chunk::ChunkSystem>   chunkSystem;
};

} // namespace Rl::World
