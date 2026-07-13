export module Rl.World.Unit.UnitRegistryGPU;

import Rl.World.Unit.UnitRegister;
import Rl.World.Unit.UnitGPUParams;
import Rl.RayLog.Macro;

import <cstdint>;
import <vector>;
import <memory>;
import <type_traits>;
import <vulkan/vulkan.hpp>;

namespace Rl::World
{

class IUnit;

/* GPU unit registry for compute shader access */
export class UnitRegistryGPU
{
  protected:
  static constexpr auto RAYLOG_TAG = "UnitRegistryGPU";

  public:
  UnitRegistryGPU() = default;
  ~UnitRegistryGPU();

  /* Initialize GPU resources for unit registry */
  bool Initialize(VkDevice device, VkPhysicalDevice physicalDevice);

  /* Cleanup GPU resources */
  void Shutdown(VkDevice device);

  /* Register a unit template for GPU access */
  template <typename T> void Register(T* unit)
  {
    if (!initialized)
    {
      RayLog::LogError(RAYLOG_TAG, "Cannot register unit: registry not initialized");
      return;
    }
    if constexpr (std::is_base_of_v<IUnit, T>)
    {
      UnitGPUParams params{};
      params.unitId               = IUnitIdentifiable<T>::GetStaticClassId();
      params.temperature          = T::TEMPERATURE;
      params.moisture             = T::MOISTURE;
      params.roughness            = T::ROUGHNESS;
      params.metallic             = T::METALLIC;
      params.albedoR              = T::ALBEDO_R;
      params.albedoG              = T::ALBEDO_G;
      params.albedoB              = T::ALBEDO_B;
      params.reflectivity         = T::REFLECTIVITY;
      params.refractiveIndex      = T::REFRACTIVE_INDEX;
      params.dirtiness            = T::DIRTINESS;
      params.hardness             = T::HARDNESS;
      params.explosionResistance  = T::EXPLOSION_RESISTANCE;
      params.transparency         = T::TRANSPARENCY;
      params.emissiveIntensity    = T::EMISSIVE_INTENSITY;
      params.subsurfaceScattering = T::SUBSURFACE_SCATTERING;
      params.flammability         = T::FLAMMABILITY;
      params.lightEmit            = T::LIGHT_EMIT;
      params.lightOpacity         = T::LIGHT_OPACITY;
      params.ambientOcclusion     = T::AMBIENT_OCCLUSION;
      params.lightAbsorption      = T::LIGHT_ABSORPTION;
      params.lightScattering      = T::LIGHT_SCATTERING;
      params.humidity             = T::HUMIDITY;
      params.isLiquid             = unit != nullptr && unit->IsLiquid() ? 1u : 0u;
      params.isGas                = unit != nullptr && unit->IsGas() ? 1u : 0u;
      params.isSolid              = unit != nullptr && unit->IsSolid() ? 1u : 0u;
      cpuUnits.emplace_back(params);
      gpuDirty = true;
    }
  }

  /* Update GPU buffer with registered units (call after RegisterUnit) */
  bool UpdateGPUBuffer(VkDevice device, VkCommandBuffer commandBuffer);

  /* Get the GPU buffer handle for compute shader binding */
  [[nodiscard]]
  VkBuffer GetUnitBuffer() const
  { return unitBuffer; }

  /* Get the total number of registered units */
  [[nodiscard]]
  uint32_t GetUnitCount() const
  { return static_cast<uint32_t>(cpuUnits.size()); }

  /* Check if registry is initialized */
  [[nodiscard]]
  bool IsInitialized() const
  { return initialized; }

  /* Get CPU-side unit data for rendering integration */
  [[nodiscard]]
  const std::vector<UnitGPUParams>& GetCPUUnits() const
  { return cpuUnits; }

  /* Disable copy operations */
  UnitRegistryGPU(const UnitRegistryGPU&)            = delete;
  UnitRegistryGPU& operator=(const UnitRegistryGPU&) = delete;

  /* Disable move operations */
  UnitRegistryGPU(UnitRegistryGPU&&)                 = delete;
  UnitRegistryGPU& operator=(UnitRegistryGPU&&)      = delete;

  private:
  /* CPU-side unit data */
  std::vector<UnitGPUParams> cpuUnits;

  /* GPU-side buffer */
  VkBuffer       unitBuffer = VK_NULL_HANDLE;
  VkDeviceMemory unitMemory = VK_NULL_HANDLE;

  /* Staging buffer for data transfer */
  VkBuffer       stagingBuffer = VK_NULL_HANDLE;
  VkDeviceMemory stagingMemory = VK_NULL_HANDLE;

  /* Buffer size */
  VkDeviceSize unitBufferSize = 0;

  /* Initialization state */
  bool initialized = false;
  bool gpuDirty    = false; // Flag to indicate GPU needs update

  /* Vulkan device handles */
  VkDevice         device         = VK_NULL_HANDLE;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
};

} // namespace Rl::World
