import Rl.World.Skybox.SkyboxSystem;

import <glm/gtc/constants.hpp>;
import <cmath>;

namespace Rl::World::Skybox
{

SkyboxSystem::SkyboxSystem(Time::TimeSystem& timeSystem)
    : timeSystem(timeSystem), currentPreset(GetDefaultSkyboxPreset()),
      rng(std::random_device{}()), lastDayNumber(0)
{
  // Initialize skybox state
  currentState.sun.direction = glm::vec3(0.5f, 0.8f, 0.6f);
  currentState.sun.color = currentPreset.sunColor;
  currentState.sun.intensity = 3.5f;
  currentState.sun.elevation = glm::radians(45.0f);
  currentState.sun.azimuth = glm::radians(0.0f);
  currentState.skyColor = currentPreset.daySkyColor;
  currentState.groundColor = currentPreset.groundColor;
  currentState.ambientStrength = 0.15f;
  currentState.exposure = 1.25f;
  currentState.isDay = true;
}

void SkyboxSystem::Update()
{
  Time::TimeState timeState = timeSystem.GetTimeState();
  
  // Check if we need to update color preset for new day
  if (timeState.currentDay != lastDayNumber)
  {
    UpdateColorPresetForNewDay();
    lastDayNumber = timeState.currentDay;
  }
  
  // Calculate sun position
  currentState.sun = CalculateSunPosition(timeState.currentFragment);
  
  // Calculate sky color
  currentState.skyColor = CalculateSkyColor(timeState.currentFragment);
  
  // Calculate ground color
  currentState.groundColor = CalculateGroundColor(timeState.currentFragment);
  
  // Calculate ambient strength
  currentState.ambientStrength = CalculateAmbientStrength(timeState.currentFragment);
  
  // Calculate exposure
  currentState.exposure = CalculateExposure(timeState.currentFragment);
  
  // Update day/night state
  currentState.isDay = timeState.isDay;
}

SkyboxState SkyboxSystem::GetSkyboxState() const
{
  return currentState;
}

SunProperties SkyboxSystem::GetSunProperties() const
{
  return currentState.sun;
}

glm::vec3 SkyboxSystem::GetSkyColor() const
{
  return currentState.skyColor;
}

glm::vec3 SkyboxSystem::GetGroundColor() const
{
  return currentState.groundColor;
}

float SkyboxSystem::GetAmbientStrength() const
{
  return currentState.ambientStrength;
}

float SkyboxSystem::GetExposure() const
{
  return currentState.exposure;
}

void SkyboxSystem::SetColorPreset(const SkyboxColorPreset& preset)
{
  currentPreset = preset;
}

SkyboxColorPreset SkyboxSystem::GenerateRandomColorPreset()
{
  SkyboxColorPreset preset = GetDefaultSkyboxPreset();
  
  // Generate random sunset color variation
  preset.duskSkyColor = GenerateColorVariation(preset.duskSkyColor, 0.3f);
  
  // Generate random dawn color variation
  preset.dawnSkyColor = GenerateColorVariation(preset.dawnSkyColor, 0.2f);
  
  // Slight variation in day sky color
  preset.daySkyColor = GenerateColorVariation(preset.daySkyColor, 0.1f);
  
  return preset;
}

void SkyboxSystem::UpdateColorPresetForNewDay()
{
  currentPreset = GenerateRandomColorPreset();
}

SunProperties SkyboxSystem::CalculateSunPosition(int64_t fragment) const
{
  SunProperties props;
  
  // Check if it's day time
  if (fragment >= Time::DAY_DURATION_FRAGMENTS)
  {
    // Night time - sun is below horizon
    props.direction = glm::vec3(0.0f, -1.0f, 0.0f);
    props.color = glm::vec3(0.0f, 0.0f, 0.0f);
    props.intensity = 0.0f;
    props.elevation = -glm::radians(90.0f);
    props.azimuth = 0.0f;
    return props;
  }
  
  // Calculate day progress (0.0 to 1.0)
  float dayProgress = static_cast<float>(fragment) / static_cast<float>(Time::DAY_DURATION_FRAGMENTS);
  
  // Calculate sun elevation (arc across the sky)
  // Sun rises at 0.0 progress, reaches peak at 0.5, sets at 1.0
  props.elevation = CalculateSunElevation(dayProgress);
  
  // Calculate sun azimuth (east to west movement)
  props.azimuth = CalculateSunAzimuth(dayProgress);
  
  // Convert spherical coordinates to Cartesian direction
  float cosElevation = std::cos(props.elevation);
  props.direction.x = cosElevation * std::sin(props.azimuth);
  props.direction.y = std::sin(props.elevation);
  props.direction.z = cosElevation * std::cos(props.azimuth);
  
  // Normalize direction
  props.direction = glm::normalize(props.direction);
  
  // Sun color is white as specified
  props.color = currentPreset.sunColor;
  
  // Sun intensity varies with elevation
  // Peak intensity at noon (elevation = 90 degrees)
  props.intensity = 3.5f * std::max(0.0f, std::sin(props.elevation));
  
  return props;
}

float SkyboxSystem::CalculateSunElevation(float dayProgress) const
{
  // Sun elevation follows a sine wave
  // Rises from horizon (0 degrees) at dawn
  // Reaches peak (90 degrees) at noon
  // Sets at horizon (0 degrees) at dusk
  
  // Map dayProgress (0.0-1.0) to angle (0 to PI)
  float angle = dayProgress * glm::pi<float>();
  
  // Elevation follows sine pattern: sin(angle) * maxElevation
  // Max elevation is 90 degrees (PI/2 radians)
  float maxElevation = glm::radians(90.0f);
  
  return std::sin(angle) * maxElevation;
}

float SkyboxSystem::CalculateSunAzimuth(float dayProgress) const
{
  // Sun azimuth moves from east to west
  // East = -90 degrees (-PI/2), West = +90 degrees (PI/2)
  
  // Map dayProgress (0.0-1.0) to azimuth (-PI/2 to PI/2)
  float minAzimuth = glm::radians(-90.0f);
  float maxAzimuth = glm::radians(90.0f);
  
  return minAzimuth + dayProgress * (maxAzimuth - minAzimuth);
}

glm::vec3 SkyboxSystem::InterpolateSkyColor(const glm::vec3& color1, const glm::vec3& color2, float t) const
{
  // Clamp t to [0, 1]
  t = glm::clamp(t, 0.0f, 1.0f);
  
  // Linear interpolation
  return glm::mix(color1, color2, t);
}

glm::vec3 SkyboxSystem::CalculateSkyColor(int64_t fragment) const
{
  Time::TimeOfDay timeOfDay = timeSystem.GetTimeOfDay();
  
  switch (timeOfDay)
  {
    case Time::TimeOfDay::DAWN:
    {
      // Interpolate from night to dawn
      float dawnProgress = static_cast<float>(fragment) / 
                          static_cast<float>(Time::DAY_DURATION_FRAGMENTS * 0.1f);
      return InterpolateSkyColor(currentPreset.nightSkyColor, currentPreset.dawnSkyColor, dawnProgress);
    }
    case Time::TimeOfDay::DAY:
    {
      // Interpolate from dawn to day, then day to dusk
      float dayProgress = static_cast<float>(fragment) / 
                         static_cast<float>(Time::DAY_DURATION_FRAGMENTS);
      
      if (dayProgress < 0.5f)
      {
        // Dawn to noon
        float t = dayProgress / 0.5f;
        return InterpolateSkyColor(currentPreset.dawnSkyColor, currentPreset.daySkyColor, t);
      }
      else
      {
        // Noon to dusk
        float t = (dayProgress - 0.5f) / 0.5f;
        return InterpolateSkyColor(currentPreset.daySkyColor, currentPreset.duskSkyColor, t);
      }
    }
    case Time::TimeOfDay::DUSK:
    {
      // Interpolate from dusk to night
      float duskStart = Time::DAY_DURATION_FRAGMENTS * 0.9f;
      float duskProgress = static_cast<float>(fragment - duskStart) / 
                          static_cast<float>(Time::DAY_DURATION_FRAGMENTS * 0.1f);
      return InterpolateSkyColor(currentPreset.duskSkyColor, currentPreset.nightSkyColor, duskProgress);
    }
    case Time::TimeOfDay::NIGHT:
    {
      // Night color
      return currentPreset.nightSkyColor;
    }
    default:
      return currentPreset.daySkyColor;
  }
}

glm::vec3 SkyboxSystem::CalculateGroundColor(int64_t fragment) const
{
  // Ground color varies based on sky color (reflection)
  glm::vec3 skyColor = CalculateSkyColor(fragment);
  
  // Ground is darker and more saturated version of sky
  return skyColor * 0.3f;
}

float SkyboxSystem::CalculateAmbientStrength(int64_t fragment) const
{
  // Ambient strength varies with sun elevation
  // Higher during day, lower during night
  
  if (fragment >= Time::DAY_DURATION_FRAGMENTS)
  {
    // Night time - low ambient
    return 0.05f;
  }
  
  // Day time - ambient varies with sun elevation
  float dayProgress = static_cast<float>(fragment) / static_cast<float>(Time::DAY_DURATION_FRAGMENTS);
  float elevation = CalculateSunElevation(dayProgress);
  
  // Ambient is proportional to sin(elevation)
  return 0.15f * std::max(0.0f, std::sin(elevation));
}

float SkyboxSystem::CalculateExposure(int64_t fragment) const
{
  // Exposure varies with time of day
  // Higher during day, lower during night
  
  if (fragment >= Time::DAY_DURATION_FRAGMENTS)
  {
    // Night time - low exposure
    return 0.5f;
  }
  
  // Day time - exposure varies with sun elevation
  float dayProgress = static_cast<float>(fragment) / static_cast<float>(Time::DAY_DURATION_FRAGMENTS);
  float elevation = CalculateSunElevation(dayProgress);
  
  // Exposure is proportional to sin(elevation)
  return 0.5f + 0.75f * std::max(0.0f, std::sin(elevation));
}

glm::vec3 SkyboxSystem::GenerateColorVariation(const glm::vec3& baseColor, float variationAmount)
{
  std::uniform_real_distribution<float> dist(-variationAmount, variationAmount);
  
  glm::vec3 variedColor;
  variedColor.r = glm::clamp(baseColor.r + dist(rng), 0.0f, 1.0f);
  variedColor.g = glm::clamp(baseColor.g + dist(rng), 0.0f, 1.0f);
  variedColor.b = glm::clamp(baseColor.b + dist(rng), 0.0f, 1.0f);
  
  return variedColor;
}

} // namespace Rl::World::Skybox
