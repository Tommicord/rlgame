export module Rl.World.Biome.BiomeRegistryGPU;

import Rl.World.Biome.BiomeGPUParams;
import Rl.RayLog.Macro;

import <cstdint>;
import <vector>;
import <memory>;
import <vulkan/vulkan.hpp>;

namespace Rl::World::Biome
{
class IBiome;

/* GPU biome registry for compute shader access */
export class BiomeRegistryGPU
{
protected:
    static constexpr auto RAYLOG_TAG = "BiomeRegistryGPU";

public:
    /* Default constructor, doesn't need to receive anything */
    BiomeRegistryGPU()  = default;
    ~BiomeRegistryGPU() = default;

    /* Initialize GPU resources for biome registry */
    bool Initialize(VkDevice device, VkPhysicalDevice physicalDevice);

    /* Cleanup GPU resources */
    void Shutdown(VkDevice device);

    /* Register a biome for GPU access */
    template <typename T>
    void RegisterBiome(const T& biome);

    /* Update GPU buffer with registered biomes (call after RegisterBiome) */
    bool UpdateGPUBuffer(VkDevice device, VkCommandBuffer commandBuffer);

    /* Get the GPU buffer handle for compute shader binding */
    [[nodiscard]]
    VkBuffer GetBiomeBuffer() const
    {
        return biomeBuffer;
    }

    /* Get the GPU buffer handle for unit rules */
    [[nodiscard]]
    VkBuffer GetUnitRulesBuffer() const
    {
        return unitRulesBuffer;
    }

    /* Get the total number of registered biomes */
    [[nodiscard]]
    uint32_t GetBiomeCount() const
    {
        return static_cast<uint32_t>(cpuBiomes.size());
    }

    /* Get the total number of unit rules across all biomes */
    [[nodiscard]]
    uint32_t GetUnitRuleCount() const
    {
        return static_cast<uint32_t>(cpuUnitRules.size());
    }

    /* Check if registry is initialized */
    [[nodiscard]]
    bool IsInitialized() const
    {
        return initialized;
    }

    /* Disable copy operations */
    BiomeRegistryGPU(const BiomeRegistryGPU&)            = delete;
    BiomeRegistryGPU& operator=(const BiomeRegistryGPU&) = delete;

    /* Disable move operations */
    BiomeRegistryGPU(BiomeRegistryGPU&&)            = delete;
    BiomeRegistryGPU& operator=(BiomeRegistryGPU&&) = delete;

private:
    /* CPU-side biome data */
    std::vector<BiomeGPUParams>   cpuBiomes;
    std::vector<BiomeUnitRuleGPU> cpuUnitRules;

    /* GPU-side buffers */
    VkBuffer       biomeBuffer     = VK_NULL_HANDLE;
    VkDeviceMemory biomeMemory     = VK_NULL_HANDLE;
    VkBuffer       unitRulesBuffer = VK_NULL_HANDLE;
    VkDeviceMemory unitRulesMemory = VK_NULL_HANDLE;

    /* Staging buffers for data transfer */
    VkBuffer       stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;

    /* Buffer sizes */
    VkDeviceSize biomeBufferSize     = 0;
    VkDeviceSize unitRulesBufferSize = 0;

    /* Initialization state */
    bool initialized = false;
    bool gpuDirty    = false; // Flag to indicate GPU needs update

    /* Vulkan device handles */
    VkDevice         device         = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
};

template <typename T>
void BiomeRegistryGPU::RegisterBiome(const T& biome)
{
    if (!initialized)
    {
        RayLog::LogError(RAYLOG_TAG, "Cannot register biome: registry not initialized");
        return;
    }

    // Create GPU params from biome
    BiomeGPUParams params{};
    params.biomeType = biome.GetBiomeType().value;

    // Extract temperature layer
    auto tempLayer              = biome.GetTemperatureNoiseLayer();
    params.temperatureBase      = 0.5f; // Default, can be customized
    params.temperatureVariation = tempLayer.persistence;

    // Extract moisture layer
    auto moistLayer          = biome.GetMoistureNoiseLayer();
    params.moistureBase      = 0.5f; // Default, can be customized
    params.moistureVariation = moistLayer.persistence;

    // Extract elevation layer
    auto elevLayer            = biome.GetElevationNoiseLayer();
    params.elevationBase      = 0.5f; // Default, can be customized
    params.elevationVariation = elevLayer.persistence;

    // Count unit rules
    const auto& rules    = biome.GetUnitRules();
    params.unitRuleCount = static_cast<uint32_t>(rules.size());

    cpuBiomes.push_back(params);

    // Convert unit rules to GPU format
    for (const auto& rule : rules)
    {
        BiomeUnitRuleGPU gpuRule{};
        gpuRule.unitId         = rule.unitId;
        gpuRule.minHeight      = rule.minHeight;
        gpuRule.maxHeight      = rule.maxHeight;
        gpuRule.minTemperature = rule.minTemperature;
        gpuRule.maxTemperature = rule.maxTemperature;
        gpuRule.minMoisture    = rule.minMoisture;
        gpuRule.maxMoisture    = rule.maxMoisture;
        gpuRule.minElevation   = rule.minElevation;
        gpuRule.maxElevation   = rule.maxElevation;
        gpuRule.probability    = rule.probability;
        gpuRule.density        = rule.density;
        gpuRule.padding[0]     = 0.0f;
        gpuRule.padding[1]     = 0.0f;

        cpuUnitRules.push_back(gpuRule);
    }

    gpuDirty = true;
    RayLog::LogInfo(RAYLOG_TAG, "Registered biome: %s (ID: %d, Rules: %d)",
                    biome.GetBiomeName(), params.biomeType, params.unitRuleCount);
}

} // namespace Rl::World::Biome
