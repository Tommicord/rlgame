#ifndef RL_BASE_MAIN_GAME_DEVICE_H
#define RL_BASE_MAIN_GAME_DEVICE_H

#include "Rl.Base/GameVulkanBuffer.h"
#include "Rl.Base/GameResources.h"
#include "Rl.Base/IGameDrawable.h"

#include "Rl.Client/Rendering/CompositorManager.h"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace rl
{
#if defined(__ANDROID__)
struct MainGameAndroid;
#elif defined(_WIN32)
struct MainGameWin32Handle;
#elif defined(__linux__)
struct MainGameLinux;
#endif
template <size_t N> class GameDrawableArena;

struct MainGame;
struct MainGameDeviceArenaHandler
{
    static constexpr size_t N   = 4096;
    using MainGameDrawableArena = GameDrawableArena<N>;
    MainGameDrawableArena arena;

    MainGameDeviceArenaHandler()                                                   = default;
    MainGameDeviceArenaHandler(const MainGameDeviceArenaHandler& other)            = delete;
    MainGameDeviceArenaHandler& operator=(const MainGameDeviceArenaHandler& other) = delete;
};

struct GameDevice
{
    VkInstance                   instance       = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT     debugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR                 surface        = VK_NULL_HANDLE;
    VkPhysicalDevice             physicalDevice = VK_NULL_HANDLE;
    VkDevice                     device         = VK_NULL_HANDLE;
    VkQueue                      graphicsQueue  = VK_NULL_HANDLE;
    VkQueue                      presentQueue   = VK_NULL_HANDLE;
    VkSwapchainKHR               swapChain      = VK_NULL_HANDLE;
    std::vector<VkImage>         swapChainImages{};
    VkFormat                     swapChainImageFormat{};
    VkExtent2D                   swapChainExtent{};
    std::vector<VkImageView>     swapChainImageViews{};
    std::vector<VkFramebuffer>   swapChainFramebuffers{};
    VkRenderPass                 renderPass  = VK_NULL_HANDLE;
    VkCommandPool                commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers{};
    std::vector<VkSemaphore>     imageAvailableSemaphores{};
    std::vector<VkSemaphore>     renderFinishedSemaphores{};
    std::vector<VkFence>         inFlightFences{};
    VkPipelineLayout             pipelineLayout   = VK_NULL_HANDLE;
    VkPipeline                   graphicsPipeline = VK_NULL_HANDLE;
    bool                         windowMinimized  = false;
    bool                         headlessMode     = false;
    GameVulkanMemoryAllocator*   memoryAllocator  = nullptr;

    struct SwapChainSupport
    {
        VkSurfaceCapabilitiesKHR        capabilities{};
        std::vector<VkSurfaceFormatKHR> formats{};
        std::vector<VkPresentModeKHR>   presentModes{};
    };

    struct QueueFamilyIndices
    {
        uint32_t graphicsFamily = -1;
        uint32_t presentFamily  = -1;
        bool     isComplete() const
        {
          return graphicsFamily != -1 && presentFamily != -1;
        }
    };
    GameDevice() = default;
};
using QueueFamilyIndices = GameDevice::QueueFamilyIndices;
using SwapChainSupport   = GameDevice::SwapChainSupport;

class GameDeviceInstance final
{
#if defined(__ANDROID__)
    MainGameAndroidHandle* handle;
#elif defined(_WIN32)
    MainGameWin32Handle* handle;
#elif defined(__linux__)
    MainGameLinux* handle;
#endif
    friend class MainGame;
    friend class MainGameDrawableArena;

  public:
    GameDeviceInstance();
    GameDeviceInstance(
#if defined(__ANDROID__)
        MainGameAndroidHandle& handle
#elif defined(_WIN32)
        MainGameWin32Handle& handle
#elif defined(__linux__)
        MainGameLinux& handle
#endif
    );

    ~GameDeviceInstance();
    GameDeviceInstance(const GameDeviceInstance&)            = delete;
    GameDeviceInstance& operator=(const GameDeviceInstance&) = delete;
    GameDeviceInstance(GameDeviceInstance&&)                 = default;

    VkCommandBuffer  getCommandBuffer() const;
    VkCommandPool    getCommandPool() const;
    VkSwapchainKHR   getSwapChain() const;
    VkPhysicalDevice getPhysicalDevice() const;
    VkDevice         getDevice() const;
    VkQueue          getGraphicsQueue() const;
    VkQueue          getPresentQueue() const;
    VkExtent2D       getExtent2d() const;
    VkFramebuffer    getFramebuffer() const;
    VkInstance       getInstance() const;
    uint32_t         getGraphicsFamily() const;
    uint32_t         getPresentFamily() const;
    uint32_t         getExtentWidth() const;
    uint32_t         getExtentHeight() const;
    uint32_t         getMaxFramesInFlight() const;
    VkRenderPass     getRenderPass() const;
    uint32_t         getCurrentFrame() const;

    static void                        registerDrawable(IGameDrawable* ptr);
    static MainGameDeviceArenaHandler& getGameArena();
    GameResources&                     getGameResources();

    void init();
    void setHeadlessMode(bool headless);
    bool isHeadlessMode() const;

  private:
    GameDevice    gameDevice;
    GameResources gameResources;
    bool          isHeadlessInstance;

    void       setupDrawables();
    void       setupCompositor();
    void       recreateSwapChain();
    void       cleanup();
    void       cleanupSwapChain();
    void       cleanupSyncObjects();
    bool       checkValidationLayerSupport();
    bool       checkDeviceExtensionSupport(VkPhysicalDevice device);
    bool       isDeviceSuitable(VkPhysicalDevice device);
    void       createInstance();
    void       tryCreateInstance(VkInstanceCreateInfo createInfo);
    void       pickPhysicalDevice();
    void       tryCreateLogicalDevice(VkDeviceCreateInfo createInfo);
    void       createLogicalDevice();
    void       tryCreateSurface(VkWin32SurfaceCreateInfoKHR createInfo);
    void       createSurface();
    VkExtent2D chooseSwapExtent(VkSurfaceCapabilitiesKHR capabilities);
    VkPresentModeKHR
    chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkSurfaceFormatKHR
         chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    void tryCreateSwapChain(VkSwapchainCreateInfoKHR createInfo);
    void createSwapChain();
    void tryCreateImageView(VkImageViewCreateInfo createInfo, VkImageView* imageView);
    void createImageViews();
    void createRenderPass();
    void tryCreateFramebuffer(VkFramebufferCreateInfo createInfo, VkFramebuffer* framebuffer);
    void tryCreateCommandBuffer(VkCommandBufferAllocateInfo allocInfo,
                                VkCommandBuffer*            commandBuffer);
    void tryAcquireNextFrame(uint32_t& imageIndex);
    void createCommandBuffers();
    void createFramebuffers();
    void createGraphicsPipeline();
    void tryCreateCommandPool(VkCommandPoolCreateInfo createInf);
    void createCommandPool();
    void createSyncObjects();
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void updateCallback();
    SwapChainSupport   querySwapChainSupport(VkPhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;
};
} // namespace rl

#endif
