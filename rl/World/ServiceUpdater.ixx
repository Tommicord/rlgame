export module Rl.World.ServiceUpdater;

import Rl.Base.IUpdatable;
import Rl.World.Time.TimeSystem;
import Rl.World.Skybox.SkyboxSystem;
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
  public:
  ~ServiceUpdater() override = default;

  /* Update the service */
  void Update() override = 0;

  /* Get the service name for debugging */
  [[nodiscard]]
  virtual std::string GetServiceName() const = 0;
};

/* Template for creating service updaters */
export template <typename T> class TypedServiceUpdater : public ServiceUpdater
{
  public:
  explicit TypedServiceUpdater(
      std::shared_ptr<T> service, std::function<void(T&)> updateFunc);
  ~TypedServiceUpdater() override = default;

  void Update() override;
  [[nodiscard]]
  std::string GetServiceName() const override;

  private:
  std::shared_ptr<T>      service;
  std::function<void(T&)> updateFunc;
};

/* Convenience updater for TimeSystem */
export class TimeSystemUpdater : public ServiceUpdater
{
  public:
  explicit TimeSystemUpdater(
      std::shared_ptr<Time::TimeSystem> timeSystem, int64_t fragmentsPerUpdate);
  ~TimeSystemUpdater() override = default;

  void Update() override;
  [[nodiscard]]
  std::string GetServiceName() const override;

  private:
  std::shared_ptr<Time::TimeSystem> timeSystem;
  int64_t                           fragmentsPerUpdate;
};

/* Convenience updater for SkyboxSystem */
export class SkyboxSystemUpdater : public ServiceUpdater
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

/* Convenience updater for Player services */
export class PlayerServicesUpdater : public ServiceUpdater
{
  public:
  PlayerServicesUpdater();
  ~PlayerServicesUpdater() override = default;

  void Update() override;
  [[nodiscard]]
  std::string GetServiceName() const override;
};

} // namespace Rl::World
