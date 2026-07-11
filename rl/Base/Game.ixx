export module Rl.Base.Game;

import Rl.Base.UserInput;
import Rl.Base.Binding;
import Rl.Client.State.UnitState;
import Rl.Player.PlayerCamera;
import Rl.Player.CameraController;
import Rl.World.ServiceUpdaterRegistry;
import Rl.World.ServiceUpdaterRegister;

import <GLFW/glfw3.h>;
import <optional>;
import <vector>;
import <memory>;
import <vulkan/vulkan.hpp>;

namespace Rl::Main
{

export class Game
{
  std::unique_ptr<Providers::UnitModel>          unitModel;
  std::unique_ptr<World::ServiceUpdaterRegistry> serviceUpdaterRegistry;

  public:
  void         Run();
  void         DestroyGraphics();
  void         DestroyResources();
  void         InitGraphics();
  void         InitWindow();
  void         Update();
  void         UpdateServices();
  static Game& GetInstance();
  MainBinding& GetMainBinding();
  ~Game();

  private:
  Game();
  GLFWwindow*       window;
  Input::UserInput& input;
  MainBinding       binding;

  void CreateInstance();
  void CreateSurface();
  void CreateUnitModel();
  void CreateResources();
  void PickPhysicalDevice();
  void CreateLogicalDevice();
  void CreateSwapChain();
  void CreateImageViews();
  void CreateRenderPass();
  void CreateFramebuffers();
  void CreateCommandPool();
  void CreateCommandBuffers();
  void CreatePipelineLayout();
  void CreateSyncObjects();
  void DrawCallback();
  void Draw();

  [[nodiscard]]
  MainBinding::QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;

  [[nodiscard]]
  bool IsDeviceSuitable(VkPhysicalDevice device) const;

  [[nodiscard]]
  bool CheckValidationLayerSupport() const;

  [[nodiscard]]
  std::vector<const char*> GetRequiredExtensions() const;
};

} // namespace Rl::Main
