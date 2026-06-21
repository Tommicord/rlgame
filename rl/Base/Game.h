#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <optional>
#include <vector>
#include <vulkan/vulkan.h>

#include "rl/Base/InputReceiver.h"
#include "rl/Client/State/CameraState.h"
#include "rl/Client/State/UnitState.h"

namespace Rl::Game
{

struct VulkanContext
{
  VkInstance                   instance       = VK_NULL_HANDLE;
  VkPhysicalDevice             physicalDevice = VK_NULL_HANDLE;
  VkDevice                     device         = VK_NULL_HANDLE;
  VkQueue                      graphicsQueue  = VK_NULL_HANDLE;
  VkQueue                      presentQueue   = VK_NULL_HANDLE;
  VkSurfaceKHR                 surface        = VK_NULL_HANDLE;
  VkSwapchainKHR               swapChain      = VK_NULL_HANDLE;
  std::vector<VkImage>         swapChainImages;
  VkFormat                     swapChainImageFormat;
  VkExtent2D                   swapChainExtent;
  std::vector<VkImageView>     swapChainImageViews;
  VkRenderPass                 renderPass          = VK_NULL_HANDLE;
  VkDescriptorSetLayout        descriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorSet              descriptorSet       = VK_NULL_HANDLE;
  VkPipelineLayout             pipelineLayout      = VK_NULL_HANDLE;
  std::vector<VkFramebuffer>   swapChainFramebuffers;
  VkCommandPool                commandPool = VK_NULL_HANDLE;
  std::vector<VkCommandBuffer> commandBuffers;
  VkSemaphore                  imageAvailableSemaphore = VK_NULL_HANDLE;
  std::vector<VkSemaphore>     renderFinishedSemaphores;
  VkFence                      inFlightFence = VK_NULL_HANDLE;
  struct QueueFamilyIndices
  {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    [[nodiscard]]
    bool isComplete() const
    {
      return graphicsFamily.has_value() && presentFamily.has_value();
    }
  };
  QueueFamilyIndices queueFamilyIndices;
};

class Game : public Input::InputObserver
{
  Input::InputReceiver&                   input;
  std::unique_ptr<Providers::CameraModel> cameraModel;
  std::unique_ptr<Providers::UnitModel>   unitModel;

  public:
  using Window  = GLFWwindow;
  using Context = VulkanContext;
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
  VulkanContext& GetVulkanContext();
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
  Window*                           vkWindow = nullptr;
  Context                           vkContext;
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
  VulkanContext::QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;
  bool                              IsDeviceSuitable(VkPhysicalDevice device);
  bool                              CheckValidationLayerSupport();
  std::vector<const char*>          GetRequiredExtensions();
};

} // namespace Rl::Game
