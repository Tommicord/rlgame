#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <optional>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace Rl::Game {

struct VulkanContext
{
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;
    VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
    VkFence inFlightFence = VK_NULL_HANDLE;
    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
        [[nodiscard]]
        bool isComplete() const { return graphicsFamily.has_value() && presentFamily.has_value(); }
    };
    QueueFamilyIndices queueFamilyIndices;
};

class Game {
public:
    void Run();
    void Tick();
    void Render();
    void CleanupGraphics() const;
    void InitGraphics();
    void InitWindow();
    void GetDeltaTime();
    void InputHandle();
    void UpdateEntities();
    void UpdateCamera();
    void UpdatePhysics();
    void UpdateAudio();
    void UpdateUI();
    void UpdateLogic();
    void UpdateRender();
    static Game& GetInstance();
    VulkanContext& GetVulkanContext();
    ~Game();
private:
    using Window = GLFWwindow;
    using Context = VulkanContext;
    Game() = default;
    Window* vkWindow = nullptr;
    Context vkContext;
    static constexpr int width = 1800;
    static constexpr int height = 900;
    void CreateInstance();
    void CreateSurface();
    void PickPhysicalDevice();
    void CreateLogicalDevice();
    void CreateSwapChain();
    void CreateImageViews();
    void CreateRenderPass();
    void CreateGraphicsPipeline();
    void CreateFramebuffers();
    void CreateCommandPool();
    void CreateCommandBuffers();
    void CreateSyncObjects();
    void DrawFrame() const;
    void RecreateSwapChain();
    VulkanContext::QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;
    bool IsDeviceSuitable(VkPhysicalDevice device);
    bool CheckValidationLayerSupport();
    std::vector<const char*> GetRequiredExtensions();
};

} // namespace Rl::Game
