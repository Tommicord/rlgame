export module Rl.Base.Game;

import Rl.Base.UserInput;
import Rl.Base.Binding;
import Rl.Client.State.UnitState;
import Rl.Player.PlayerCamera;
import Rl.Player.CameraController;

import <GLFW/glfw3.h>;
import <optional>;
import <vector>;
import <memory>;
import <vulkan/vulkan.hpp>;

namespace Rl::Main
{

/* The GLFWwindow alias type */
export using WindowT = GLFWwindow;

/* The MainBinding alias type */
export using ContextT = MainBinding;

/* The UserInput alias type */
export using InputT = Input::UserInput;

/* Width of the window */
export constexpr unsigned int width = 1800;

/* Height of the window */
export constexpr unsigned int height = 900;

/* The unique input receiver in the Game */
export InputT& input = Input::UserInput::GetInstance();

export class Game
{

  std::unique_ptr<Providers::UnitModel> unitModel;

  public:
  void         Run();
  void         DestroyGraphics();
  void         DestroyResources();
  void         InitGraphics();
  void         InitWindow();
  void         UpdateModels();
  static Game& GetInstance();
  MainBinding& GetMainBinding();
  ~Game();

  private:
  Game();
  WindowT* window;
  InputT&  input;
  ContextT binding;

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
  void DrawModels();
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

} // namespace Rl::Game
