export module Rl.World.Skybox.SkyboxSystem;

import Rl.World.Time.TimeSystem;
import <glm/glm.hpp>;
import <cstdint>;
import <random>;
import <memory>;

namespace Rl::World::Skybox
{

/* Skybox color presets for different times of day */
export struct SkyboxColorPreset
{
  glm::vec3 dawnSkyColor;
  glm::vec3 daySkyColor;
  glm::vec3 duskSkyColor;
  glm::vec3 nightSkyColor;
  glm::vec3 sunColor;
  glm::vec3 groundColor;
};

/* Sun position and properties */
export struct SunProperties
{
  glm::vec3 direction; // Sun direction vector (normalized)
  glm::vec3 color; // Sun color (RGB)
  float     intensity; // Sun intensity
  float     elevation; // Sun elevation angle in radians
  float     azimuth; // Sun azimuth angle in radians
};

/* Skybox state for rendering */
export struct SkyboxState
{
  SunProperties sun;
  glm::vec3     skyColor;
  glm::vec3     groundColor;
  float         ambientStrength;
  float         exposure;
  bool          isDay;
};

/* Skybox System for managing sky and sun properties */
export class SkyboxSystem
{
  public:
  explicit SkyboxSystem(Time::TimeSystem& timeSystem);
  ~SkyboxSystem() = default;

  /* Disable copy operations */
  SkyboxSystem(const SkyboxSystem&) = delete;
  SkyboxSystem& operator=(const SkyboxSystem&) = delete;

  /* Enable move operations */
  SkyboxSystem(SkyboxSystem&& other) noexcept = default;
  SkyboxSystem& operator=(SkyboxSystem&& other) noexcept = default;

  /* Update skybox based on current time */
  void Update();

  /* Get current skybox state */
  [[nodiscard]]
  SkyboxState GetSkyboxState() const;

  /* Get sun properties */
  [[nodiscard]]
  SunProperties GetSunProperties() const;

  /* Get sky color */
  [[nodiscard]]
  glm::vec3 GetSkyColor() const;

  /* Get ground color */
  [[nodiscard]]
  glm::vec3 GetGroundColor() const;

  /* Get ambient strength */
  [[nodiscard]]
  float GetAmbientStrength() const;

  /* Get exposure */
  [[nodiscard]]
  float GetExposure() const;

  /* Set color preset for current day */
  void SetColorPreset(const SkyboxColorPreset& preset);

  /* Generate random color preset for variant sunsets */
  [[nodiscard]]
  SkyboxColorPreset GenerateRandomColorPreset();

  /* Update color preset for new day */
  void UpdateColorPresetForNewDay();

  private:
  Time::TimeSystem& timeSystem;
  SkyboxColorPreset currentPreset;
  SkyboxState       currentState;
  std::mt19937      rng;
  uint32_t          lastDayNumber;

  /* Calculate sun position based on time of day */
  [[nodiscard]]
  SunProperties CalculateSunPosition(int64_t fragment) const;

  /* Calculate sun elevation angle */
  [[nodiscard]]
  float CalculateSunElevation(float dayProgress) const;

  /* Calculate sun azimuth angle */
  [[nodiscard]]
  float CalculateSunAzimuth(float dayProgress) const;

  /* Interpolate between colors based on time */
  [[nodiscard]]
  glm::vec3 InterpolateSkyColor(
      const glm::vec3& color1, const glm::vec3& color2, float t) const;

  /* Calculate sky color based on time */
  [[nodiscard]]
  glm::vec3 CalculateSkyColor(int64_t fragment) const;

  /* Calculate ground color based on time */
  [[nodiscard]]
  glm::vec3 CalculateGroundColor(int64_t fragment) const;

  /* Calculate ambient strength based on time */
  [[nodiscard]]
  float CalculateAmbientStrength(int64_t fragment) const;

  /* Calculate exposure based on time */
  [[nodiscard]]
  float CalculateExposure(int64_t fragment) const;

  /* Generate random color variation */
  [[nodiscard]]
  glm::vec3 GenerateColorVariation(const glm::vec3& baseColor, float variationAmount);
};

/* Default skybox color preset */
export SkyboxColorPreset GetDefaultSkyboxPreset()
{
  SkyboxColorPreset preset;
  preset.dawnSkyColor = glm::vec3(0.8f, 0.5f, 0.3f); // Orange-ish dawn
  preset.daySkyColor = glm::vec3(0.53f, 0.81f, 0.92f); // Blue day sky
  preset.duskSkyColor = glm::vec3(0.9f, 0.4f, 0.2f); // Reddish sunset
  preset.nightSkyColor = glm::vec3(0.05f, 0.05f, 0.15f); // Dark blue night
  preset.sunColor = glm::vec3(1.0f, 0.95f, 0.8f); // Warm white sun
  preset.groundColor = glm::vec3(0.15f, 0.12f, 0.1f); // Dark ground
  return preset;
}

} // namespace Rl::World::Skybox
