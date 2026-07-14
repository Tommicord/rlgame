export module Rl.Base.Game;

import Rl.Base.UserInput;
import Rl.Base.Binding;
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
private:
    std::unique_ptr<World::ServiceUpdaterRegistry> serviceUpdaterRegistry;

public:
    void         Run();
    void         DestroyGraphics();
    void         DestroyResources();
    void         InitGraphics();
    void         InitWindow();
    void         Init();
    void         Update();
    void         UpdateServices();
    static Game& GetInstance();
    MainBinding& GetMainBinding();
    void         SetHeadless(bool headless);
    ~Game();

    Game();
    GLFWwindow*       window;
    Input::UserInput& input;
    MainBinding       binding;
    bool              headless;

    /* Debug messenger */
    VkDebugUtilsMessengerEXT debugMessenger;

    void CreateInstance();
    void CreateSurface();
    void CreateResources();
    void PickPhysicalDevice();
    void CreateLogicalDevice();
    void CreateSwapChain();
    void CreateImageViews();
    void CreateRenderPass();
    void CreateFramebuffers();
    void CreateCommandPool();
    void CreateCommandBuffers();
    void CreateSyncObjects();
    void SetupDebugMessenger();
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
