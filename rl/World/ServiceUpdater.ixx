export module Rl.World.ServiceUpdater;

import Rl.Base.IUpdatable;
import Rl.World.Time.TimeSystem;
import Rl.World.Skybox.SkyboxSystem;
import Rl.World.Chunk.ChunkSystem;
import Rl.Player.PlayerProvider;

import <memory>;
import <vector>;
import <functional>;
import <string>;

namespace Rl::World
{

/* Base class for service updaters */
export class ServiceUpdater : public Providers::IUpdatable
{
protected:
    static constexpr auto RAYLOG_TAG = "ServiceUpdater";

public:
    ~ServiceUpdater() override = default;

    /* Update the service */
    void Update() override = 0;

    /* Gets the internal RayLog logging tag */
    std::string GetRayLogTag() const
    {
        return RAYLOG_TAG;
    }

    /* Get the service name for debugging */
    [[nodiscard]]
    virtual std::string GetServiceName() const = 0;
};

/* Convenience updater for TimeSystem */
export class TimeSystemUpdater final : public ServiceUpdater
{
public:
    explicit TimeSystemUpdater(std::shared_ptr<Time::TimeSystem> timeSystem,
                               int64_t                           fragmentsPerUpdate);
    ~TimeSystemUpdater() override = default;

    void Update() override;
    [[nodiscard]]
    std::string GetServiceName() const override;

private:
    std::shared_ptr<Time::TimeSystem> timeSystem;
    int64_t                           fragmentsPerUpdate;
};

/* Convenience updater for SkyboxSystem */
export class SkyboxSystemUpdater final : public ServiceUpdater
{
public:
    explicit SkyboxSystemUpdater(std::shared_ptr<Skybox::SkyboxSystem> skyboxSystem);
    ~SkyboxSystemUpdater() override = default;

    void Update() override;
    [[nodiscard]]
    std::string GetServiceName() const override;

private:
    std::shared_ptr<Skybox::SkyboxSystem> skyboxSystem;
};

/* Convenience updater for ChunkSystem */
export class ChunkSystemUpdater final : public ServiceUpdater
{
public:
    explicit ChunkSystemUpdater(std::shared_ptr<Chunk::ChunkSystem> chunkSystem);
    ~ChunkSystemUpdater() override = default;

    void Update() override;
    [[nodiscard]]
    std::string GetServiceName() const override;

private:
    std::shared_ptr<Chunk::ChunkSystem> chunkSystem;
};

/* Convenience updater for Player services */
export class PlayerServicesUpdater final : public ServiceUpdater
{
public:
    PlayerServicesUpdater();
    ~PlayerServicesUpdater() override = default;

    void Update() override;
    [[nodiscard]]
    std::string GetServiceName() const override;
};

} // namespace Rl::World
