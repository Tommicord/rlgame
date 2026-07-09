export module Rl.World.ServiceUpdaterRegistry;

import Rl.World.ServiceUpdater;
import <memory>;
import <vector>;
import <string>;
import <unordered_map>;

namespace Rl::World
{

/* Registry for managing service updaters */
export class ServiceUpdaterRegistry
{
  public:
  ServiceUpdaterRegistry() = default;
  ~ServiceUpdaterRegistry() = default;

  ServiceUpdaterRegistry(const ServiceUpdaterRegistry&) = delete;
  ServiceUpdaterRegistry& operator=(const ServiceUpdaterRegistry&) = delete;

  /* Enable move operations */
  ServiceUpdaterRegistry(ServiceUpdaterRegistry&& other) noexcept = default;
  ServiceUpdaterRegistry& operator=(ServiceUpdaterRegistry&& other) noexcept = default;

  /* Register a service updater */
  void RegisterUpdater(const std::string& name, std::shared_ptr<ServiceUpdater> updater);

  /* Unregister a service updater by name */
  void UnregisterUpdater(const std::string& name);

  /* Get a service updater by name */
  [[nodiscard]]
  std::shared_ptr<ServiceUpdater> GetUpdater(const std::string& name) const;

  /* Check if a service updater is registered */
  [[nodiscard]]
  bool HasUpdater(const std::string& name) const;

  /* Update all registered service updaters */
  void UpdateAll();

  /* Get the count of registered updaters */
  [[nodiscard]]
  size_t GetUpdaterCount() const;

  /* Clear all registered updaters */
  void ClearAll();

  /* Get all updater names */
  [[nodiscard]]
  std::vector<std::string> GetUpdaterNames() const;

  private:
  std::unordered_map<std::string, std::shared_ptr<ServiceUpdater>> updaters;
};

} // namespace Rl::World
