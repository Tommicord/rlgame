export module Rl.Client.State.UnitState;

import Rl.Base.IDrawable;
import Rl.Base.IModel;
import Rl.Base.Binding;
import Rl.World.Unit;
import Rl.Client.State.CameraState;

import <cstdint>;
import <memory>;
import <string>;
import <vector>;
import <optional>;
import <vulkan/vulkan.hpp>;

namespace Rl::Providers
{

// Just for data interchange between classes
export struct UnitStateResource final : public IStateResource
{
  World::IUnit& unit;
  CameraModel*   camera;

  explicit UnitStateResource(World::IUnit& unit) :
      IStateResource(), unit(unit), camera(nullptr)
  {
  }
};

export struct UnitStateBinding final : public IStateDrawableBinding
{
  VkBuffer              vertexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory        vertexBufferMemory = VK_NULL_HANDLE;
  VkBuffer              indexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory        indexBufferMemory = VK_NULL_HANDLE;
  VkBuffer              outputIndexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory        outputIndexBufferMemory = VK_NULL_HANDLE;
  VkBuffer              visibleCountBuffer = VK_NULL_HANDLE;
  VkDeviceMemory        visibleCountBufferMemory = VK_NULL_HANDLE;
  VkBuffer              indirectDrawBuffer = VK_NULL_HANDLE;
  VkDeviceMemory        indirectDrawBufferMemory = VK_NULL_HANDLE;
  VkBuffer              frustumBuffer = VK_NULL_HANDLE;
  VkDeviceMemory        frustumBufferMemory = VK_NULL_HANDLE;
  VkPipeline            computePipeline = VK_NULL_HANDLE;
  VkPipelineLayout      computePipelineLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout computeDescriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorSet       computeDescriptorSet = VK_NULL_HANDLE;
  VkPipeline            pipeline = VK_NULL_HANDLE;
  VkPipelineLayout      pipelineLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorSet       descriptorSet = VK_NULL_HANDLE;
  VkDescriptorPool      descriptorPool = VK_NULL_HANDLE;

  // Placeholder resources for bindings 8 and 9
  VkImage        placeholderLightingTexture = VK_NULL_HANDLE;
  VkDeviceMemory placeholderLightingTextureMemory = VK_NULL_HANDLE;
  VkImageView    placeholderLightingTextureView = VK_NULL_HANDLE;
  VkSampler      placeholderLightingSampler = VK_NULL_HANDLE;
  VkBuffer       placeholderSettingsBuffer = VK_NULL_HANDLE;
  VkDeviceMemory placeholderSettingsBufferMemory = VK_NULL_HANDLE;

  // PBR lighting resources (binding 4 and 10)
  VkBuffer       placeholderLightingBuffer = VK_NULL_HANDLE;
  VkDeviceMemory placeholderLightingBufferMemory = VK_NULL_HANDLE;
  VkImage        placeholderAOTexture = VK_NULL_HANDLE;
  VkDeviceMemory placeholderAOTextureMemory = VK_NULL_HANDLE;
  VkImageView    placeholderAOTextureView = VK_NULL_HANDLE;
  VkSampler      placeholderAOSampler = VK_NULL_HANDLE;
  VkImage        placeholderNormalTexture = VK_NULL_HANDLE;
  VkDeviceMemory placeholderNormalTextureMemory = VK_NULL_HANDLE;
  VkImageView    placeholderNormalTextureView = VK_NULL_HANDLE;
  VkSampler      placeholderNormalSampler = VK_NULL_HANDLE;

  // Global sampler for all textures (to avoid sampler allocation limit)
  VkSampler globalTextureSampler = VK_NULL_HANDLE;

  // Generated AO textures for each face
  VkImage        aoTextures[6] = {VK_NULL_HANDLE};
  VkDeviceMemory aoTexturesMemory[6] = {VK_NULL_HANDLE};
  VkImageView    aoTexturesView[6] = {VK_NULL_HANDLE};

  // Generated normal textures for each face (binding 11)
  VkImage        normalTextures[6] = {VK_NULL_HANDLE};
  VkDeviceMemory normalTexturesMemory[6] = {VK_NULL_HANDLE};
  VkImageView    normalTexturesView[6] = {VK_NULL_HANDLE};

  // Triplanar mapping settings buffer (binding 12)
  VkBuffer       triplanarSettingsBuffer = VK_NULL_HANDLE;
  VkDeviceMemory triplanarSettingsBufferMemory = VK_NULL_HANDLE;

  // Curvature compute shader buffers
  VkBuffer              curvedVertexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory        curvedVertexBufferMemory = VK_NULL_HANDLE;
  VkBuffer              curvedIndexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory        curvedIndexBufferMemory = VK_NULL_HANDLE;
  VkBuffer              curveCountersBuffer = VK_NULL_HANDLE;
  VkDeviceMemory        curveCountersBufferMemory = VK_NULL_HANDLE;
  VkBuffer              curveIndirectDrawBuffer = VK_NULL_HANDLE;
  VkDeviceMemory        curveIndirectDrawBufferMemory = VK_NULL_HANDLE;
  VkPipeline            curveComputePipeline = VK_NULL_HANDLE;
  VkPipelineLayout      curveComputePipelineLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout curveComputeDescriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorSet       curveComputeDescriptorSet = VK_NULL_HANDLE;

  // Shadow map resources
  VkImage          shadowMapImage = VK_NULL_HANDLE;
  VkDeviceMemory   shadowMapMemory = VK_NULL_HANDLE;
  VkImageView      shadowMapView = VK_NULL_HANDLE;
  VkSampler        shadowMapSampler = VK_NULL_HANDLE;
  VkFramebuffer    shadowMapFramebuffer = VK_NULL_HANDLE;
  VkRenderPass     shadowMapRenderPass = VK_NULL_HANDLE;
  VkPipeline       shadowPipeline = VK_NULL_HANDLE;
  VkPipelineLayout shadowPipelineLayout = VK_NULL_HANDLE;
  bool             shadowMapInitialized = false;

  UnitStateBinding() = default;
};

export class UnitStateDrawable final : public IStateDrawable<UnitStateResource, UnitStateBinding>
{
  public:
  /* Stores the base class type */
  using Base = IStateDrawable;

  void OnDraw(
      UnitStateResource& resource, UnitStateBinding& vk, Game::MainBinding& context) override;
  void OnDrawCompute(
      UnitStateResource& resource, UnitStateBinding& vk, Game::MainBinding& context) override;
  void OnUpdate(
      UnitStateResource& resource, UnitStateBinding& vk, Game::MainBinding& context) override;
  void OnCreate(
      UnitStateResource& resource, UnitStateBinding& vk, Game::MainBinding& context) override;
  void OnDestroy(
      UnitStateResource& resource, UnitStateBinding& vk, Game::MainBinding& context) override;
  void OnPause() override;
  void OnResume() override;
};

export class UnitModel final
    : public IStateModel<World::IUnit, UnitStateDrawable, UnitStateResource, UnitStateBinding>
{
  std::shared_ptr<UnitStateDrawable> unitDrawable;
  std::unique_ptr<UnitStateResource> unitResource;
  std::unique_ptr<UnitStateBinding>  unitBinding;
  std::unique_ptr<World::IUnit>    unit;

  public:
  /* Constructs a model of the Camera class */
  explicit UnitModel(Game::MainBinding& context);

  /* Destructs a CameraModel */
  ~UnitModel() override = default;

  /* Gets the stored camera */
  [[nodiscard]]
  World::IUnit& GetObjectRef() const override;

  /* Gets the stored camera */
  [[nodiscard]]
  UnitStateResource& GetResource() const override;

  /* Gets the stored camera */
  [[nodiscard]]
  UnitStateDrawable& GetDrawable() const override;

  /* Gets the stored camera */
  [[nodiscard]]
  UnitStateBinding& GetBinding() const override;
};

} // namespace Rl::Providers
