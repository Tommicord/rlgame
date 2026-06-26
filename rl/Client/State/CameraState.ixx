export module Rl.Client.State.CameraState;

import Rl.Base.IDrawable;
import Rl.Base.IModel;
import Rl.Base.Binding;
import Rl.World.Camera;

import <cstdint>;
import <memory>;
import <string>;
import <vector>;
import <vulkan/vulkan.hpp>;

namespace Rl::Providers
{

export struct CameraStateResource final : IStateResource
{
  World::Camera& cam;
  explicit CameraStateResource(World::Camera& camera) : IStateResource(), cam(camera)
  {
  }
};

export struct CameraStateBinding final : IStateDrawableBinding
{
  VkBuffer              uniformBuffer = VK_NULL_HANDLE;
  VkDeviceMemory        uniformBufferMemory = VK_NULL_HANDLE;
  VkDescriptorSet       descriptorSet = VK_NULL_HANDLE;
  VkDescriptorPool      descriptorPool = VK_NULL_HANDLE;
  VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
  CameraStateBinding() = default;
};

export class CameraStateDrawable final
    : public IStateDrawable<CameraStateResource, CameraStateBinding>
{
  public:
  /* Stores the base class type */
  using Base = IStateDrawable;

  void OnDraw(
      CameraStateResource& resource, CameraStateBinding& vk, Game::MainBinding& context) override;
  void OnDrawCompute(
      CameraStateResource& resource, CameraStateBinding& vk, Game::MainBinding& context) override;
  void OnUpdate(
      CameraStateResource& resource, CameraStateBinding& vk, Game::MainBinding& context) override;
  void OnCreate(
      CameraStateResource& resource, CameraStateBinding& vk, Game::MainBinding& context) override;
  void OnDestroy(
      CameraStateResource& resource, CameraStateBinding& vk, Game::MainBinding& context) override;
  void OnPause() override;
  void OnResume() override;
};

export class CameraModel final : public IStateModel<World::Camera,
                                     CameraStateDrawable,
                                     CameraStateResource,
                                     CameraStateBinding>
{
  std::shared_ptr<CameraStateDrawable> cameraDrawable;
  std::unique_ptr<CameraStateResource> cameraResource;
  std::unique_ptr<CameraStateBinding>  cameraBinding;
  std::unique_ptr<World::Camera>       camera;

  public:
  /* Constructs a model of the Camera class */
  explicit CameraModel(Game::MainBinding& context);

  /* Destructs a CameraModel */
  ~CameraModel() override = default;

  /* Gets the stored camera */
  [[nodiscard]]
  World::Camera& GetObject() const override;

  /* Gets the stored camera */
  [[nodiscard]]
  CameraStateResource& GetResource() const override;

  /* Gets the stored camera */
  [[nodiscard]]
  CameraStateDrawable& GetDrawable() const override;

  /* Gets the stored camera */
  [[nodiscard]]
  CameraStateBinding& GetBinding() const override;
};

} // namespace Rl::Providers
