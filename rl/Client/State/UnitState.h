#pragma once

#include "rl/Base/StateDrawable.h"
#include "rl/Base/StateModel.h"
#include "rl/World/Unit/Unit.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace Rl::Providers
{

class CameraModel;

// Just for data interchange between classes
struct UnitStateResource : StateResource
{
  World::BaseUnit* unit;
  CameraModel*     cameraModel;

  /* Note: due to compatibility with one parameter constructor
   * This can't pass the CameraModel
   * the camera must be set manually or will fail everything
   */
  explicit UnitStateResource(World::BaseUnit& unit) :
      StateResource(), unit(&unit), cameraModel(nullptr)
  {
  }
};

struct UnitStateDrawableVulkan : StateDrawableVulkan
{
  VkBuffer              vertexBuffer               = VK_NULL_HANDLE;
  VkDeviceMemory        vertexBufferMemory         = VK_NULL_HANDLE;
  VkBuffer              indexBuffer                = VK_NULL_HANDLE;
  VkDeviceMemory        indexBufferMemory          = VK_NULL_HANDLE;
  VkBuffer              outputIndexBuffer          = VK_NULL_HANDLE;
  VkDeviceMemory        outputIndexBufferMemory    = VK_NULL_HANDLE;
  VkBuffer              visibleCountBuffer         = VK_NULL_HANDLE;
  VkDeviceMemory        visibleCountBufferMemory   = VK_NULL_HANDLE;
  VkBuffer              indirectDrawBuffer         = VK_NULL_HANDLE;
  VkDeviceMemory        indirectDrawBufferMemory   = VK_NULL_HANDLE;
  VkBuffer              frustumBuffer              = VK_NULL_HANDLE;
  VkDeviceMemory        frustumBufferMemory        = VK_NULL_HANDLE;
  VkPipeline            computePipeline            = VK_NULL_HANDLE;
  VkPipelineLayout      computePipelineLayout      = VK_NULL_HANDLE;
  VkDescriptorSetLayout computeDescriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorSet       computeDescriptorSet       = VK_NULL_HANDLE;
  VkPipeline            pipeline                   = VK_NULL_HANDLE;
  VkPipelineLayout      pipelineLayout             = VK_NULL_HANDLE;
  VkDescriptorSetLayout descriptorSetLayout        = VK_NULL_HANDLE;
  VkDescriptorSet       descriptorSet              = VK_NULL_HANDLE;
  VkDescriptorPool      descriptorPool             = VK_NULL_HANDLE;

  // Placeholder resources for bindings 8 and 9
  VkImage        placeholderLightingTexture       = VK_NULL_HANDLE;
  VkDeviceMemory placeholderLightingTextureMemory = VK_NULL_HANDLE;
  VkImageView    placeholderLightingTextureView   = VK_NULL_HANDLE;
  VkSampler      placeholderLightingSampler       = VK_NULL_HANDLE;
  VkBuffer       placeholderSettingsBuffer        = VK_NULL_HANDLE;
  VkDeviceMemory placeholderSettingsBufferMemory  = VK_NULL_HANDLE;

  // PBR lighting resources (binding 4 and 10)
  VkBuffer       placeholderLightingBuffer       = VK_NULL_HANDLE;
  VkDeviceMemory placeholderLightingBufferMemory = VK_NULL_HANDLE;
  VkImage        placeholderAOTexture            = VK_NULL_HANDLE;
  VkDeviceMemory placeholderAOTextureMemory      = VK_NULL_HANDLE;
  VkImageView    placeholderAOTextureView        = VK_NULL_HANDLE;
  VkSampler      placeholderAOSampler            = VK_NULL_HANDLE;
  VkImage        placeholderNormalTexture        = VK_NULL_HANDLE;
  VkDeviceMemory placeholderNormalTextureMemory  = VK_NULL_HANDLE;
  VkImageView    placeholderNormalTextureView    = VK_NULL_HANDLE;
  VkSampler      placeholderNormalSampler        = VK_NULL_HANDLE;

  // Global sampler for all textures (to avoid sampler allocation limit)
  VkSampler globalTextureSampler = VK_NULL_HANDLE;

  // Generated AO textures for each face
  VkImage        aoTextures[6]       = {VK_NULL_HANDLE};
  VkDeviceMemory aoTexturesMemory[6] = {VK_NULL_HANDLE};
  VkImageView    aoTexturesView[6]   = {VK_NULL_HANDLE};

  // Generated normal textures for each face (binding 11)
  VkImage        normalTextures[6]       = {VK_NULL_HANDLE};
  VkDeviceMemory normalTexturesMemory[6] = {VK_NULL_HANDLE};
  VkImageView    normalTexturesView[6]   = {VK_NULL_HANDLE};

  // Triplanar mapping settings buffer (binding 12)
  VkBuffer       triplanarSettingsBuffer       = VK_NULL_HANDLE;
  VkDeviceMemory triplanarSettingsBufferMemory = VK_NULL_HANDLE;

  // Curvature compute shader buffers
  VkBuffer       curvedVertexBuffer       = VK_NULL_HANDLE;
  VkDeviceMemory curvedVertexBufferMemory = VK_NULL_HANDLE;
  VkBuffer       curvedIndexBuffer       = VK_NULL_HANDLE;
  VkDeviceMemory curvedIndexBufferMemory = VK_NULL_HANDLE;
  VkBuffer       curveCountersBuffer       = VK_NULL_HANDLE;
  VkDeviceMemory curveCountersBufferMemory = VK_NULL_HANDLE;
  VkBuffer       curveIndirectDrawBuffer       = VK_NULL_HANDLE;
  VkDeviceMemory curveIndirectDrawBufferMemory = VK_NULL_HANDLE;
  VkPipeline     curveComputePipeline            = VK_NULL_HANDLE;
  VkPipelineLayout curveComputePipelineLayout      = VK_NULL_HANDLE;
  VkDescriptorSetLayout curveComputeDescriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorSet  curveComputeDescriptorSet       = VK_NULL_HANDLE;

  // Shadow map resources
  VkImage        shadowMapImage       = VK_NULL_HANDLE;
  VkDeviceMemory shadowMapMemory     = VK_NULL_HANDLE;
  VkImageView    shadowMapView        = VK_NULL_HANDLE;
  VkSampler      shadowMapSampler     = VK_NULL_HANDLE;
  VkFramebuffer  shadowMapFramebuffer = VK_NULL_HANDLE;
  VkRenderPass   shadowMapRenderPass  = VK_NULL_HANDLE;

  UnitStateDrawableVulkan() = default;
};

class UnitStateDrawable : public StateDrawable<UnitStateResource, UnitStateDrawableVulkan>
{
  public:
  /* Stores the base class type */
  using Base = StateDrawable<UnitStateResource, UnitStateDrawableVulkan>;

  void OnDraw(UnitStateResource& resource,
      UnitStateDrawableVulkan&   vk,
      Game::VulkanContext&       context) override;
  void OnDrawCompute(
      UnitStateResource& resource, UnitStateDrawableVulkan& vk, Game::VulkanContext& context);
  void OnUpdate(UnitStateResource& resource,
      UnitStateDrawableVulkan&     vk,
      Game::VulkanContext&         context) override;
  void OnCreate(UnitStateResource& resource,
      UnitStateDrawableVulkan&     vk,
      Game::VulkanContext&         context) override;
  void OnDestroy(UnitStateResource& resource,
      UnitStateDrawableVulkan&      vk,
      Game::VulkanContext&          context) override;
  void OnPause() override;
  void OnResume() override;
};

class UnitModel : public StateModel<World::BaseUnit,
                      UnitStateDrawable,
                      UnitStateResource,
                      UnitStateDrawableVulkan>
{
  std::shared_ptr<Providers::UnitStateDrawable>       unitDrawable;
  std::unique_ptr<Providers::UnitStateResource>       unitResource;
  std::unique_ptr<Providers::UnitStateDrawableVulkan> unitVk;
  std::unique_ptr<World::BaseUnit>                    unit;

  public:
  /* Constructs a model of the Camera class */
  explicit UnitModel(Game::VulkanContext& context);

  /* Destructs a CameraModel */
  ~UnitModel() override = default;

  /* Gets the stored camera */
  World::BaseUnit& GetObject() override;

  /* Gets the stored camera */
  UnitStateResource& GetResource() override;

  /* Gets the stored camera */
  UnitStateDrawable& GetDrawable() override;

  /* Gets the stored camera */
  UnitStateDrawableVulkan& GetVulkanState() override;
};

} // namespace Rl::Providers
