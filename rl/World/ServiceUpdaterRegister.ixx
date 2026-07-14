export module Rl.World.ServiceUpdaterRegister;

import Rl.World.ServiceUpdaterRegistry;
import Rl.World.ServiceUpdater;
import Rl.World.Time.TimeSystem;
import Rl.World.Skybox.SkyboxSystem;
import Rl.World.ServiceLocator;
import <memory>;
import <string>;

namespace Rl::World
{

/* Helper class for registering service updaters */
export class ServiceUpdaterRegister
{
public:
    explicit ServiceUpdaterRegister(ServiceUpdaterRegistry& registry);
    ~ServiceUpdaterRegister() = default;

    /* Disable copy operations */
    ServiceUpdaterRegister(const ServiceUpdaterRegister&)            = delete;
    ServiceUpdaterRegister& operator=(const ServiceUpdaterRegister&) = delete;

    /* Enable move operations */
    ServiceUpdaterRegister(ServiceUpdaterRegister&& other) noexcept            = delete;
    ServiceUpdaterRegister& operator=(ServiceUpdaterRegister&& other) noexcept = delete;

    /* Register TimeSystem updater */
    void RegisterTimeSystemUpdater(const std::string& name, int64_t fragmentsPerUpdate);

    /* Register SkyboxSystem updater */
    void RegisterSkyboxSystemUpdater(const std::string& name);

    /* Register Player services updater */
    void RegisterPlayerServicesUpdater(const std::string& name);

    /* Register Chunk services updater */
    void RegisterChunkServicesUpdater(const std::string& name);

    /* Register custom updater */
    void RegisterCustomUpdater(const std::string&              name,
                               std::shared_ptr<ServiceUpdater> updater);

    /* Register from ServiceLocator (auto-detects registered services) */
    void RegisterFromServiceLocator(int64_t timeFragmentsPerUpdate = 1);

private:
    ServiceUpdaterRegistry& registry;
};

/* Convenience function to register all standard services */
export void RegisterStandardServices(ServiceUpdaterRegistry& registry,
                                     int64_t                 timeFragmentsPerUpdate = 1);

} // namespace Rl::World
