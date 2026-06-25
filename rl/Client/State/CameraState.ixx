export module Rl.Client.State.CameraState;

import Rl.Base.StateDrawable;
import Rl.Base.StateModel;
import Rl.World.Camera;
import <cstdint>;
import <memory>;
import <string>;
import <vector>;
import <vulkan/vulkan.hpp>;

namespace Rl::Providers
{

// Just for data interchange between classes
export struct CameraStateResource : StateResource
{
  World::Camera* cam;
  explicit CameraStateResource(World::Camera& camera) : StateResource(), cam(&camera)
  {
  }
};

export struct CameraStateDrawableVulkan : StateDrawableVulkan
{
  VkBuffer              uniformBuffer       = VK_NULL_HANDLE;
  VkDeviceMemory        uniformBufferMemory = VK_NULL_HANDLE;
  VkDescriptorSet       descriptorSet       = VK_NULL_HANDLE;
  VkDescriptorPool      descriptorPool      = VK_NULL_HANDLE;
  VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
  CameraStateDrawableVulkan()               = default;
};

export class CameraStateDrawable : public StateDrawable<CameraStateResource, CameraStateDrawableVulkan>
{
  public:
  /* Stores the base class type */
  using Base = StateDrawable<CameraStateResource, CameraStateDrawableVulkan>;

  void OnDraw(CameraStateResource& resource,
      CameraStateDrawableVulkan&   vk,
      Game::VulkanContext&         context) override;
  void OnDrawCompute(
      CameraStateResource& resource, CameraStateDrawableVulkan& vk, Game::VulkanContext& context);
  void OnUpdate(CameraStateResource& resource,
      CameraStateDrawableVulkan&     vk,
      Game::VulkanContext&           context) override;
  void OnCreate(CameraStateResource& resource,
      CameraStateDrawableVulkan&     vk,
      Game::VulkanContext&           context) override;
  void OnDestroy(CameraStateResource& resource,
      CameraStateDrawableVulkan&      vk,
      Game::VulkanContext&            context) override;
  void OnPause() override;
  void OnResume() override;
};

export class CameraModel : public StateModel<World::Camera,
                        CameraStateDrawable,
                        CameraStateResource,
                        CameraStateDrawableVulkan>
{
  std::shared_ptr<Providers::CameraStateDrawable>       cameraDrawable;
  std::unique_ptr<Providers::CameraStateResource>       cameraResource;
  std::unique_ptr<Providers::CameraStateDrawableVulkan> cameraVk;
  std::unique_ptr<World::Camera>                        camera;

  public:
  /* Constructs a model of the Camera class */
  explicit CameraModel(Game::VulkanContext& context);

  /* Destructs a CameraModel */
  ~CameraModel() override = default;

  /* Gets the stored camera */
  World::Camera& GetObject() override;

  /* Gets the stored camera */
  CameraStateResource& GetResource() override;

  /* Gets the stored camera */
  CameraStateDrawable& GetDrawable() override;

  /* Gets the stored camera */
  CameraStateDrawableVulkan& GetVulkanState() override;
};

} // namespace Rl::Providers
