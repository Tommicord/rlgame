export module Rl.World.Biome;

import Rl.Base.Game;
import Rl.Base.Binding;
import Rl.World.Biome.BiomeRegistryGPU;
import Rl.RayLog.Macro;

import <cstdint>;
import <string>;
import <vector>;
import <memory>;
import <algorithm>;
import <cmath>;

namespace Rl::World::Biome
{
/* Noise layer configuration for biome generation.
   These parameters control the per-biome noise sources used by the GPU
   biome classifier and height/temperature/moisture generation pipeline. */
export struct BiomeNoiseLayer
{
    float scale; // Noise frequency scale. Typical range: 0.1 - 10.0. Higher values
                 // produce finer noise detail.
    float offsetX; // X offset for noise sampling. Any float value can be used to shift
                   // the noise pattern.
    float offsetY; // Y offset for noise sampling. Use this to avoid visible tiling or
                   // repeat artifacts.
    float offsetZ; // Z offset for noise sampling. Used for 3D sampling when combining
                   // layers.
    uint32_t octaves; // FBM octaves. Typical range: 1 - 8. More octaves add detail at
                      // higher cost.
    float persistence; // FBM persistence. Typical 0.0 - 1.0. Lower values reduce
                       // high-frequency amplitude.
    float lacunarity; // FBM lacunarity. Typical values: 1.5 - 3.0. Higher values increase
                      // frequency each octave.
    uint32_t noiseType; // Noise type selector: 0 = Standard, 1 = FBM, 2 = Ridged, 3 =
                        // Turbulence.
    float weight; // Layer weight in final classification. Typical range: 0.0 - 1.0.
};

export struct BiomeType
{
    uint32_t value; // Unique biome identifier used by the GPU registry.
};

/* Unit generation rule for a biome.
   Each rule is evaluated in order to decide which unit should be placed
   based on the current biome conditions and density/probability settings. */
export struct BiomeUnitRule
{
    uint32_t unitId; // Unit ID to place when this rule matches.
    float minHeight; // Minimum world height for the unit. Use 0.0 to enable from ground
                     // level.
    float maxHeight; // Maximum world height for the unit. Use a large value to disable
                     // the upper bound.
    float minTemperature; // Minimum temperature threshold. Expected range: 0.0 - 1.0.
    float maxTemperature; // Maximum temperature threshold. Expected range: 0.0 - 1.0.
    float minMoisture; // Minimum moisture threshold. Expected range: 0.0 - 1.0.
    float maxMoisture; // Maximum moisture threshold. Expected range: 0.0 - 1.0.
    float minElevation; // Minimum elevation threshold. Expected range: 0.0 - 1.0.
    float maxElevation; // Maximum elevation threshold. Expected range: 0.0 - 1.0.
    float probability; // Probability of placement. Expected range: 0.0 (never) - 1.0
                       // (always).
    float density; // Density of placement when this rule is chosen. Expected range: 0.0
                   // - 1.0.
};

/* Pure virtual interface for biome definition */
export class IBiome
{
    /* The biome GPU registry */
    inline static auto registryGPU = std::make_shared<BiomeRegistryGPU>();

protected:
    static constexpr auto RAYLOG_TAG = "Biome";

public:
    IBiome() noexcept = default;

    template <typename Derived> explicit IBiome(Derived* ptr) noexcept
    {
        if (!registryGPU)
        {
            RayLog::LogDebug(
                RAYLOG_TAG,
                "Cannot initialize in registry biome: registryGPU not initialized");
            return;
        }

        if (!registryGPU->IsInitialized())
        {
            const Main::MainBinding& binding = Main::Game::GetInstance().GetMainBinding();
            registryGPU->Initialize(binding.device, binding.physicalDevice);
        }
    }

    template <typename Derived> void RegisterDerivedBiome(Derived* ptr) const
    {
        if (!registryGPU)
        {
            RayLog::LogDebug(
                RAYLOG_TAG, "Cannot register derived biome: registryGPU not initialized");
            return;
        }
        if (!ptr)
        {
            RayLog::LogDebug(RAYLOG_TAG, "Cannot register derived biome: null pointer");
            return;
        }
        registryGPU->RegisterBiome(*ptr);
    }

    virtual ~IBiome() = default;

    static BiomeRegistryGPU* GetGPUIDRegistry()
    {
        return registryGPU.get();
    }

    [[nodiscard]]
    BiomeNoiseLayer GetTemperatureNoiseLayer() const
    {
        return temperatureLayer;
    }

    [[nodiscard]]
    BiomeNoiseLayer GetMoistureNoiseLayer() const
    {
        return moistureLayer;
    }

    [[nodiscard]]
    BiomeNoiseLayer GetElevationNoiseLayer() const
    {
        return elevationLayer;
    }

    [[nodiscard]]
    const std::vector<BiomeUnitRule>& GetUnitRules() const
    {
        return unitRules;
    }

    /* Normalize derived biome parameters into expected 0.0 - 1.0 ranges.
       This helper will clamp or scale configured values so the biome
       generation pipeline receives normalized parameters from custom biomes. */
    template <typename Derived> static void Normalize(Derived* ptr)
    {
        if (!ptr)
        {
            return;
        }

        NormalizeNoiseLayer(ptr->temperatureLayer);
        NormalizeNoiseLayer(ptr->moistureLayer);
        NormalizeNoiseLayer(ptr->elevationLayer);

        NormalizeThresholds(ptr->minTemperature, ptr->maxTemperature);
        NormalizeThresholds(ptr->minMoisture, ptr->maxMoisture);
        NormalizeThresholds(ptr->minElevation, ptr->maxElevation);

        for (auto& rule : ptr->unitRules)
        {
            NormalizeUnitRule(rule);
        }
    }

    /* Default classification based on threshold ranges */
    [[nodiscard]]
    bool BelongsToBiome(float temperature, float moisture, float elevation) const
    {
        return (temperature >= minTemperature && temperature <= maxTemperature) &&
               (moisture >= minMoisture && moisture <= maxMoisture) &&
               (elevation >= minElevation && elevation <= maxElevation);
    }

    /* Default unit selection based on matching rules */
    [[nodiscard]]
    uint32_t GetDominantUnit(float temperature,
                             float moisture,
                             float elevation,
                             float height) const
    {
        uint32_t bestUnitId = 0; // Default to air/unknown
        float    bestScore  = 0.0f;

        for (const auto& rule : unitRules)
        {
            if (height < rule.minHeight || height > rule.maxHeight)
                continue;
            if (temperature < rule.minTemperature || temperature > rule.maxTemperature)
                continue;
            if (moisture < rule.minMoisture || moisture > rule.maxMoisture)
                continue;
            if (elevation < rule.minElevation || elevation > rule.maxElevation)
                continue;
            float score = rule.probability * rule.density;

            if (score > bestScore)
            {
                bestScore  = score;
                bestUnitId = rule.unitId;
            }
        }

        return bestUnitId;
    }

    // Configuration methods
    void SetTemperatureNoiseLayer(const BiomeNoiseLayer& layer)
    {
        temperatureLayer = layer;
    }

    void SetMoistureNoiseLayer(const BiomeNoiseLayer& layer)
    {
        moistureLayer = layer;
    }

    void SetElevationNoiseLayer(const BiomeNoiseLayer& layer)
    {
        elevationLayer = layer;
    }

    void AddUnitRule(const BiomeUnitRule& rule)
    {
        unitRules.push_back(rule);
    }

    void SetTemperatureThresholds(float minTemp, float maxTemp)
    {
        minTemperature = minTemp;
        maxTemperature = maxTemp;
    }

    void SetMoistureThresholds(float minMoist, float maxMoist)
    {
        minMoisture = minMoist;
        maxMoisture = maxMoist;
    }

    void SetElevationThresholds(float minElev, float maxElev)
    {
        minElevation = minElev;
        maxElevation = maxElev;
    }

private:
    static float Clamp01(float value)
    {
        return std::clamp(value, 0.0f, 1.0f);
    }

    static float NormalizeScaledValue(float value, float divisor)
    {
        if (divisor == 0.0f)
        {
            return 0.0f;
        }
        return Clamp01(value / divisor);
    }

    static float NormalizePeriodic(float value)
    {
        float normalized = std::fmod(value, 1.0f);
        if (normalized < 0.0f)
        {
            normalized += 1.0f;
        }
        return normalized;
    }

    static void NormalizeNoiseLayer(BiomeNoiseLayer& layer)
    {
        layer.scale   = NormalizeScaledValue(layer.scale, 10.0f);
        layer.offsetX = NormalizePeriodic(layer.offsetX);
        layer.offsetY = NormalizePeriodic(layer.offsetY);
        layer.offsetZ = NormalizePeriodic(layer.offsetZ);
        layer.octaves = static_cast<uint32_t>(
            Clamp01(static_cast<float>(layer.octaves) / 8.0f) * 7.0f + 1.0f);
        layer.persistence = Clamp01(layer.persistence);
        layer.lacunarity  = NormalizeScaledValue(layer.lacunarity, 3.0f);
        layer.noiseType   = static_cast<uint32_t>(
            Clamp01(static_cast<float>(layer.noiseType) / 3.0f) * 3.0f + 0.5f);
        layer.noiseType = std::min(layer.noiseType, static_cast<uint32_t>(3));
        layer.weight    = Clamp01(layer.weight);
    }

    static void NormalizeThresholds(float& minValue, float& maxValue)
    {
        minValue = Clamp01(minValue);
        maxValue = Clamp01(maxValue);
        if (maxValue < minValue)
        {
            std::swap(minValue, maxValue);
        }
    }

    static void NormalizeUnitRule(BiomeUnitRule& rule)
    {
        rule.minHeight = Clamp01(rule.minHeight);
        rule.maxHeight = Clamp01(rule.maxHeight);
        if (rule.maxHeight < rule.minHeight)
        {
            std::swap(rule.minHeight, rule.maxHeight);
        }

        rule.minTemperature = Clamp01(rule.minTemperature);
        rule.maxTemperature = Clamp01(rule.maxTemperature);
        if (rule.maxTemperature < rule.minTemperature)
        {
            std::swap(rule.minTemperature, rule.maxTemperature);
        }

        rule.minMoisture = Clamp01(rule.minMoisture);
        rule.maxMoisture = Clamp01(rule.maxMoisture);
        if (rule.maxMoisture < rule.minMoisture)
        {
            std::swap(rule.minMoisture, rule.maxMoisture);
        }

        rule.minElevation = Clamp01(rule.minElevation);
        rule.maxElevation = Clamp01(rule.maxElevation);
        if (rule.maxElevation < rule.minElevation)
        {
            std::swap(rule.minElevation, rule.maxElevation);
        }

        rule.probability = Clamp01(rule.probability);
        rule.density     = Clamp01(rule.density);
    }

protected:
    BiomeType   biomeType;
    const char* biomeName;

    BiomeNoiseLayer            temperatureLayer{};
    BiomeNoiseLayer            moistureLayer{};
    BiomeNoiseLayer            elevationLayer{};
    std::vector<BiomeUnitRule> unitRules;

    // Threshold ranges for biome classification
    float minTemperature =
        0.0f; // Lower bound for biome temperature classification (0.0-1.0)
    float maxTemperature =
        1.0f; // Upper bound for biome temperature classification (0.0-1.0)
    float minMoisture  = 0.0f; // Lower bound for biome moisture classification (0.0-1.0)
    float maxMoisture  = 1.0f; // Upper bound for biome moisture classification (0.0-1.0)
    float minElevation = 0.0f; // Lower bound for biome elevation classification (0.0-1.0)
    float maxElevation = 1.0f; // Upper bound for biome elevation classification (0.0-1.0)
};

} // namespace Rl::World::Biome
