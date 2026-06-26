export module Rl.Base.Game;

import Rl.Base.InputReceiver;
import Rl.Base.Binding;
import Rl.Client.State.CameraState;
import Rl.Client.State.UnitState;

import <GLFW/glfw3.h>;
import <optional>;
import <vector>;
import <memory>;
import <vulkan/vulkan.hpp>;

namespace Rl::Game
{

export class Game : public Input::InputObserver
{
  Input::InputReceiver&                   input;
  std::unique_ptr<Providers::CameraModel> cameraModel;
  std::unique_ptr<Providers::UnitModel>   unitModel;

  public:
  using Window  = GLFWwindow;
  using Context = MainBinding;
  void           Run();
  void           Tick();
  void           CleanupGraphics();
  void           CleanupResources();
  void           InitInputReceiverObserver();
  void           InitGraphics();
  void           InitWindow();
  void           GetDeltaTime();
  void           InputHandle();
  void           UpdateEntities();
  void           UpdateCamera();
  void           UpdatePhysics();
  void           UpdateAudio();
  void           UpdateUI();
  void           UpdateLogic();
  void           UpdateRender();
  static Game&   GetInstance();
  MainBinding& GetVulkanContext();
  void           OnKeyEvent(const Input::KeyEvent& event) override;
  void           OnMouseButtonEvent(const Input::MouseButtonEvent& event) override;
  void           OnMouseMoveEvent(const Input::MouseMoveEvent& event) override;
  void           OnMouseScrollEvent(const Input::MouseScrollEvent& event) override;
  ~Game() override;

  private:
  /* Width of the window */
  static constexpr int width = 1800;

  /* Height of the window */
  static constexpr int height = 900;

  Game();
  Window*                           window = nullptr;
  Context                           binding;
  void                              CreateInstance();
  void                              CreateSurface();
  void                              CreateCameraModel();
  void                              CreateUnitModel();
  void                              CreateResources();
  void                              PickPhysicalDevice();
  void                              CreateLogicalDevice();
  void                              CreateSwapChain();
  void                              CreateImageViews();
  void                              CreateRenderPass();
  void                              CreateFramebuffers();
  void                              CreateCommandPool();
  void                              CreateCommandBuffers();
  void                              CreatePipelineLayout();
  void                              CreateSyncObjects();
  void                              DrawUI();
  void                              DrawFrame();
  MainBinding::QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;
  bool                              IsDeviceSuitable(VkPhysicalDevice device);
  bool                              CheckValidationLayerSupport();
  std::vector<const char*>          GetRequiredExtensions();
};

} // namespace Rl::Game
