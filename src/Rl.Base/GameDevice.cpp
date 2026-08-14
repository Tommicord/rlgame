#include "Rl.Base/GameDevice.h"
#include "Rl.Base/GameError.h"
#include "Rl.Base/GameVulkanQueueSubmitter.h"
#include "Rl.Base/GameVulkanCallback.h"
#include "Rl.Base/IGameDrawable.h"
#include "Rl.Base/MainGame.h"

#include "Rl.Client/Rendering/CompositorManager.h"

#include <cstdint>
#include <cstring>
#include <set>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace rl
{

static const std::vector<const char*> deviceExtensions         = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
static const std::vector<const char*> optionalDeviceExtensions = {
    VK_EXT_DEVICE_FAULT_EXTENSION_NAME, VK_EXT_DEVICE_MEMORY_REPORT_EXTENSION_NAME,
    VK_EXT_DEBUG_MARKER_EXTENSION_NAME};
#ifdef NDEBUG
static constexpr bool _enableValidationLayers = false;
#else
static constexpr bool _enableValidationLayers = true;

// Validation layers are not enabled in release builds
static const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};
#endif
static const uint32_t maxFramesInFlight = 3;
static uint32_t       currentFrame      = 0;

GameDeviceInstance::GameDeviceInstance() : handle(nullptr), isHeadlessInstance(true)
{
  gameDevice.headlessMode = true;
}

GameDeviceInstance::GameDeviceInstance(
#if defined(__ANDROID__)
    MainGameAndroidHandle& handle
#elif defined(_WIN32)
    MainGameWin32Handle& handle
#elif defined(__linux__)
    MainGameLinux& handle
#endif
    ) : handle(&handle), isHeadlessInstance(false)
{
  gameDevice.headlessMode = false;
}

GameDeviceInstance::~GameDeviceInstance()
{
  if (!isHeadlessInstance)
  {
    MainGameDeviceArenaHandler& handler = getGameArena();
    auto&                       arena   = handler.arena;
    arena.destroyDrawCallbacks(*this);
  }
  cleanup();
}

bool GameDeviceInstance::checkValidationLayerSupport()
{
  if (!_enableValidationLayers)
  {
    return false;
  }
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
  for (const char* layerName : validationLayers)
  {
    bool layerFound = false;
    for (const auto& layerProperties : availableLayers)
    {
      if (strcmp(layerName, layerProperties.layerName) == 0)
      {
        layerFound = true;
        break;
      }
    }
    if (!layerFound)
    {
      return false;
    }
  }
  return true;
}

void GameDeviceInstance::init()
{
  createInstance();

  if (!gameDevice.headlessMode)
  {
    createSurface();
  }

  pickPhysicalDevice();
  createLogicalDevice();

  if (!gameDevice.headlessMode)
  {
    createSwapChain();
    createImageViews();
    createRenderPass();
    createFramebuffers();
    createCommandPool();
    createCommandBuffers();
    createSyncObjects();
    setupCompositor();
    setupDrawables();
  }
  else
  {
    createCommandPool();
  }
}

void GameDeviceInstance::cleanup()
{
  if (!gameDevice.headlessMode)
  {
    cleanupSwapChain();
    cleanupSyncObjects();
  }

  vkDestroyCommandPool(gameDevice.device, gameDevice.commandPool, nullptr);
  vkDeviceWaitIdle(gameDevice.device);

  if (!gameDevice.headlessMode)
  {
    vkDestroyRenderPass(gameDevice.device, gameDevice.renderPass, nullptr);
    vkDestroySurfaceKHR(gameDevice.instance, gameDevice.surface, nullptr);
  }
  if (_enableValidationLayers)
  {
    GameVulkanCallback::destroyDebugCallback(gameDevice.instance, gameDevice.debugMessenger);
  }

  vkDestroyDevice(gameDevice.device, nullptr);
  vkDestroyInstance(gameDevice.instance, nullptr);
}

void GameDeviceInstance::setHeadlessMode(bool headless)
{
  gameDevice.headlessMode = headless;
}

bool GameDeviceInstance::isHeadlessMode() const
{
  return gameDevice.headlessMode;
}

void GameDeviceInstance::setupDrawables()
{
  MainGameDeviceArenaHandler& handler = getGameArena();
  auto&                       arena   = handler.arena;
  arena.setupDrawCallbacks(*this);

  CompositorManager::initialize(gameDevice.device, gameDevice.physicalDevice, maxFramesInFlight);
  CompositorManager& compositorManager = CompositorManager::getInstance();
  compositorManager.setup(*this);
}

void GameDeviceInstance::setupCompositor()
{
  CompositorManager::getInstance().initialize(gameDevice.device, gameDevice.physicalDevice,
                                              maxFramesInFlight);
}

void GameDeviceInstance::cleanupSwapChain()
{
  for (auto framebuffer : gameDevice.swapChainFramebuffers)
  {
    vkDestroyFramebuffer(gameDevice.device, framebuffer, nullptr);
  }
  gameDevice.swapChainFramebuffers.clear();

  for (auto imageView : gameDevice.swapChainImageViews)
  {
    vkDestroyImageView(gameDevice.device, imageView, nullptr);
  }
  gameDevice.swapChainImageViews.clear();

  if (gameDevice.swapChain != VK_NULL_HANDLE)
  {
    vkDestroySwapchainKHR(gameDevice.device, gameDevice.swapChain, nullptr);
    gameDevice.swapChain = VK_NULL_HANDLE;
  }
  gameDevice.swapChainImages.clear();
}

void GameDeviceInstance::recreateSwapChain()
{
  if (gameDevice.windowMinimized)
  {
    return;
  }
  vkDeviceWaitIdle(gameDevice.device);
  for (auto framebuffer : gameDevice.swapChainFramebuffers)
  {
    vkDestroyFramebuffer(gameDevice.device, framebuffer, nullptr);
  }
  gameDevice.swapChainFramebuffers.clear();

  for (auto imageView : gameDevice.swapChainImageViews)
  {
    vkDestroyImageView(gameDevice.device, imageView, nullptr);
  }
  gameDevice.swapChainImageViews.clear();

  if (gameDevice.swapChain != VK_NULL_HANDLE)
  {
    vkDestroySwapchainKHR(gameDevice.device, gameDevice.swapChain, nullptr);
    gameDevice.swapChain = VK_NULL_HANDLE;
  }
  gameDevice.swapChainImages.clear();

  cleanupSyncObjects();
  createSwapChain();
  createSyncObjects();
  createImageViews();
  createFramebuffers();
  currentFrame = 0;
}

void GameDeviceInstance::tryCreateInstance(VkInstanceCreateInfo createInfo)
{
  VkResult result = vkCreateInstance(&createInfo, nullptr, &gameDevice.instance);
  if (result != VK_SUCCESS)
  {
    GameError::exitWithError("vkCreateInstance", "Failed to create Vulkan instance "
                                                 "(result = " +
                                                     GameError::vulkanResultToString(result) + ")");
  }
}

void GameDeviceInstance::createInstance()
{
  if (_enableValidationLayers && !checkValidationLayerSupport())
  {
    throw std::runtime_error("Validation layers requested but not available");
  }
  VkApplicationInfo appInfo{};
  appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName   = "rlgame";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName        = "No Engine";
  appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion         = VK_API_VERSION_1_1;
  appInfo.pNext              = nullptr;

  bool hasValidationFeatures = false;
  bool hasGpuAssisted        = false;

  if (_enableValidationLayers)
  {
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

    for (const auto& ext : availableExtensions)
    {
      if (strcmp(ext.extensionName, VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME) == 0)
      {
        hasValidationFeatures = true;
      }
      if (strcmp(ext.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0)
      {
        hasGpuAssisted = true;
      }
    }
  }

  VkValidationFeaturesEXT validationFeatures{};
  if (_enableValidationLayers && hasValidationFeatures)
  {
    validationFeatures.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;

    std::vector<VkValidationFeatureEnableEXT> validationFeaturesEnable;
    validationFeaturesEnable.emplace_back(
        VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT);
    validationFeaturesEnable.emplace_back(VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT);

    if (hasGpuAssisted)
    {
      validationFeaturesEnable.emplace_back(VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT);
      validationFeaturesEnable.emplace_back(
          VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT);
    }

    validationFeatures.enabledValidationFeatureCount =
        static_cast<uint32_t>(validationFeaturesEnable.size());
    validationFeatures.pEnabledValidationFeatures = validationFeaturesEnable.data();
  }

  VkInstanceCreateInfo createInfo{};
  createInfo.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;
  if (_enableValidationLayers)
  {
    createInfo.enabledLayerCount   = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
    if (hasValidationFeatures)
    {
      createInfo.pNext = &validationFeatures;
    }
  }
  else
  {
    createInfo.enabledLayerCount = 0;
  }

  // For portability, we can add the VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR
  // flag to the instance creation flags. this is useful for running on platforms that
  // may not support Vulkan like MacOS Metal
  // When tested using RenderDoc doesn't support VK_KHR_portability_enumeration so we skip
  bool runningUnderRenderDoc = (getenv("RENDERDOC_CAPTURE") != nullptr);
  if (!runningUnderRenderDoc)
  {
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
  }

  std::vector<const char*> requiredExtensions;

  if (!gameDevice.headlessMode)
  {
#if defined(_WIN32)
#define RL_KHR_SURFACE VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#elif defined(__linux__)
#define RL_KHR_SURFACE VK_KHR_XCB_SURFACE_EXTENSION_NAME
#elif defined(__ANDROID__)
#define RL_KHR_SURFACE VK_KHR_ANDROID_SURFACE_EXTENSION_NAME
#endif
    requiredExtensions = {VK_KHR_SURFACE_EXTENSION_NAME};
    if (!runningUnderRenderDoc)
    {
      requiredExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    }
  }
  else
  {
    // In headless mode we only need portability extension (unless running under
    // RenderDoc)
    if (!runningUnderRenderDoc)
    {
      requiredExtensions = {VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME};
    }
  }

#ifndef NDEBUG
  requiredExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  requiredExtensions.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif

  if (!gameDevice.headlessMode)
  {
    requiredExtensions.emplace_back(RL_KHR_SURFACE);
  }
  if (hasValidationFeatures)
  {
    requiredExtensions.emplace_back(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);
  }

  createInfo.enabledExtensionCount   = static_cast<uint32_t>(requiredExtensions.size());
  createInfo.ppEnabledExtensionNames = requiredExtensions.data();

  if (!gameDevice.headlessMode)
  {
#undef RL_KHR_SURFACE
  }
  try
  {
    tryCreateInstance(createInfo);
  }
  catch (const std::runtime_error& e)
  {
    GameError::exitWithError("vkCreateInstance", e.what());
  }
  if (_enableValidationLayers)
  {
    gameDevice.debugMessenger = GameVulkanCallback::setupDebugCallback(gameDevice.instance);
  }
}

void GameDeviceInstance::pickPhysicalDevice()
{
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(gameDevice.instance, &deviceCount, nullptr);
  if (deviceCount == 0)
  {
    throw std::runtime_error("Failed to find GPUs with Vulkan support");
  }
  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(gameDevice.instance, &deviceCount, devices.data());
  for (const auto& dev : devices)
  {
    if (isDeviceSuitable(dev))
    {
      gameDevice.physicalDevice = dev;
      break;
    }
  }
  if (gameDevice.physicalDevice == VK_NULL_HANDLE)
  {
    throw std::runtime_error("Failed to find a suitable GPU");
  }
}

void GameDeviceInstance::createRenderPass()
{
  VkAttachmentDescription colorAttachment{};
  colorAttachment.format         = gameDevice.swapChainImageFormat;
  colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef{};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments    = &colorAttachmentRef;

  VkSubpassDependency dependency{};
  dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass    = 0;
  dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments    = &colorAttachment;
  renderPassInfo.subpassCount    = 1;
  renderPassInfo.pSubpasses      = &subpass;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies   = &dependency;

  VkResult result =
      vkCreateRenderPass(gameDevice.device, &renderPassInfo, nullptr, &gameDevice.renderPass);
  if (result != VK_SUCCESS)
  {
    GameError::exitWithError("vkCreateRenderPass", "Failed to create Vulkan Render Pass "
                                                   "(result = " +
                                                       GameError::vulkanResultToString(result) +
                                                       ")");
  }
}

void GameDeviceInstance::tryCreateFramebuffer(VkFramebufferCreateInfo createInfo,
                                              VkFramebuffer*          framebuffer)
{
  VkResult result = vkCreateFramebuffer(gameDevice.device, &createInfo, nullptr, framebuffer);
  if (result != VK_SUCCESS)
  {
    GameError::exitWithError("vkCreateFramebuffer", "Failed to create Vulkan Framebuffer "
                                                    "(result = " +
                                                        GameError::vulkanResultToString(result) +
                                                        ")");
  }
}

void GameDeviceInstance::tryCreateCommandBuffer(VkCommandBufferAllocateInfo allocInfo,
                                                VkCommandBuffer*            commandBuffer)
{
  VkResult result = vkAllocateCommandBuffers(gameDevice.device, &allocInfo, commandBuffer);
  if (result != VK_SUCCESS)
  {
    GameError::exitWithError("vkAllocateCommandBuffers",
                             "Failed to allocate Vulkan command buffer "
                             "(result = " +
                                 GameError::vulkanResultToString(result) + ")");
  }
}

void GameDeviceInstance::tryAcquireNextFrame(uint32_t& imageIndex)
{
  VkResult result = vkAcquireNextImageKHR(gameDevice.device, gameDevice.swapChain, UINT64_MAX,
                                          gameDevice.imageAvailableSemaphores[currentFrame],
                                          VK_NULL_HANDLE, &imageIndex);
  if (result != VK_SUCCESS)
  {
    switch (result)
    {
    case VK_ERROR_OUT_OF_DATE_KHR:
      {
        recreateSwapChain();
        return;
      }
    case VK_SUBOPTIMAL_KHR:
    case VK_NOT_READY:
      {
        return;
      }
    case VK_ERROR_SURFACE_LOST_KHR:
      {
        recreateSwapChain();
        return;
      }
    default:
      {
        GameError::exitWithError("vkAcquireNextImageKHR", "Failed to acquire next frame");
      }
    }
  }
}

SwapChainSupport GameDeviceInstance::querySwapChainSupport(VkPhysicalDevice device)
{
  SwapChainSupport details;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, gameDevice.surface, &details.capabilities);

  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, gameDevice.surface, &formatCount, nullptr);

  if (formatCount != 0)
  {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, gameDevice.surface, &formatCount,
                                         details.formats.data());
  }

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, gameDevice.surface, &presentModeCount, nullptr);

  if (presentModeCount != 0)
  {
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, gameDevice.surface, &presentModeCount,
                                              details.presentModes.data());
  }
  return details;
}

void GameDeviceInstance::createFramebuffers()
{
  gameDevice.swapChainFramebuffers.resize(gameDevice.swapChainImageViews.size());
  for (size_t i = 0; i < gameDevice.swapChainFramebuffers.size(); ++i)
  {
    VkImageView attachments[] = {gameDevice.swapChainImageViews[i]};

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass      = gameDevice.renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments    = attachments;
    framebufferInfo.width           = gameDevice.swapChainExtent.width;
    framebufferInfo.height          = gameDevice.swapChainExtent.height;
    framebufferInfo.layers          = 1;
    tryCreateFramebuffer(framebufferInfo, &gameDevice.swapChainFramebuffers[i]);
  }
}

void GameDeviceInstance::createCommandBuffers()
{
  gameDevice.commandBuffers.resize(maxFramesInFlight);
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool        = gameDevice.commandPool;
  allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = (uint32_t)gameDevice.commandBuffers.size();
  tryCreateCommandBuffer(allocInfo, gameDevice.commandBuffers.data());
}

void GameDeviceInstance::tryCreateCommandPool(VkCommandPoolCreateInfo createInfo)
{
  VkResult result =
      vkCreateCommandPool(gameDevice.device, &createInfo, nullptr, &gameDevice.commandPool);
  if (result != VK_SUCCESS)
  {
    GameError::exitWithError("vkCreateCommandPool", "Failed to create Vulkan Command Pool "
                                                    "(result = " +
                                                        GameError::vulkanResultToString(result) +
                                                        ")");
  }
}

void GameDeviceInstance::createCommandPool()
{
  QueueFamilyIndices queueFamilyIndices = findQueueFamilies(gameDevice.physicalDevice);

  VkCommandPoolCreateInfo createInfo{};
  createInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  createInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  createInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
  tryCreateCommandPool(createInfo);
}

void GameDeviceInstance::createSyncObjects()
{
  gameDevice.imageAvailableSemaphores.resize(maxFramesInFlight);
  gameDevice.renderFinishedSemaphores.resize(gameDevice.swapChainImages.size());
  gameDevice.inFlightFences.resize(maxFramesInFlight);

  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  semaphoreInfo.pNext = nullptr;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.pNext = nullptr;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < maxFramesInFlight; ++i)
  {
    if (vkCreateSemaphore(gameDevice.device, &semaphoreInfo, nullptr,
                          &gameDevice.imageAvailableSemaphores[i]) != VK_SUCCESS)
    {
      GameError::exitWithError("vkCreateSemaphore", "Failed to create Vulkan Semaphores");
    }
  }
  for (size_t i = 0; i < gameDevice.swapChainImages.size(); ++i)
  {
    if (vkCreateSemaphore(gameDevice.device, &semaphoreInfo, nullptr,
                          &gameDevice.renderFinishedSemaphores[i]) != VK_SUCCESS)
    {
      GameError::exitWithError("vkCreateSemaphore", "Failed to create Vulkan Semaphores");
    }
  }
  for (size_t i = 0; i < maxFramesInFlight; ++i)
  {
    if (vkCreateFence(gameDevice.device, &fenceInfo, nullptr, &gameDevice.inFlightFences[i]) !=
        VK_SUCCESS)
    {
      GameError::exitWithError("vkCreateFence", "Failed to create Vulkan Fences");
    }
  }
}

void GameDeviceInstance::cleanupSyncObjects()
{
  for (size_t i = 0; i < gameDevice.imageAvailableSemaphores.size(); ++i)
  {
    vkDestroySemaphore(gameDevice.device, gameDevice.imageAvailableSemaphores[i], nullptr);
  }
  for (size_t i = 0; i < gameDevice.renderFinishedSemaphores.size(); ++i)
  {
    vkDestroySemaphore(gameDevice.device, gameDevice.renderFinishedSemaphores[i], nullptr);
  }
  for (size_t i = 0; i < gameDevice.inFlightFences.size(); ++i)
  {
    vkDestroyFence(gameDevice.device, gameDevice.inFlightFences[i], nullptr);
  }

  gameDevice.imageAvailableSemaphores.clear();
  gameDevice.renderFinishedSemaphores.clear();
  gameDevice.inFlightFences.clear();
}

void GameDeviceInstance::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
  {
    GameError::exitWithError("vkBeginCommandBuffer",
                             "Failed to begin recording Vulkan Command Buffer");
  }

  VkImageMemoryBarrier preBarrier{};
  preBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  preBarrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
  preBarrier.newLayout                       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  preBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  preBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  preBarrier.image                           = gameDevice.swapChainImages[imageIndex];
  preBarrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  preBarrier.subresourceRange.baseMipLevel   = 0;
  preBarrier.subresourceRange.levelCount     = 1;
  preBarrier.subresourceRange.baseArrayLayer = 0;
  preBarrier.subresourceRange.layerCount     = 1;
  preBarrier.srcAccessMask                   = 0;
  preBarrier.dstAccessMask                   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                       VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1,
                       &preBarrier);

  MainGameDeviceArenaHandler& handler = getGameArena();
  auto&                       arena   = handler.arena;
  arena.callDrawCallbacks(*this);

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass        = gameDevice.renderPass;
  renderPassInfo.framebuffer       = gameDevice.swapChainFramebuffers[currentFrame];
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = gameDevice.swapChainExtent;

  VkClearValue clearColor        = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues    = &clearColor;

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport{};
  viewport.x        = 0.0f;
  viewport.y        = 0.0f;
  viewport.width    = static_cast<float>(gameDevice.swapChainExtent.width);
  viewport.height   = static_cast<float>(gameDevice.swapChainExtent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = gameDevice.swapChainExtent;
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  arena.callCompositingCallbacks(*this);

  vkCmdEndRenderPass(commandBuffer);
  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
  {
    GameError::exitWithError("vkEndCommandBuffer", "Failed to record Vulkan Command Buffer");
  }
}

void GameDeviceInstance::updateCallback()
{
  vkWaitForFences(gameDevice.device, 1, &gameDevice.inFlightFences[currentFrame], VK_TRUE,
                  UINT64_MAX);
  uint32_t imageIndex;
  tryAcquireNextFrame(imageIndex);
  vkResetFences(gameDevice.device, 1, &gameDevice.inFlightFences[currentFrame]);

  vkResetCommandBuffer(gameDevice.commandBuffers[currentFrame], 0);
  recordCommandBuffer(gameDevice.commandBuffers[currentFrame], imageIndex);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore          waitSemaphores[]   = {gameDevice.imageAvailableSemaphores[currentFrame]};
  VkSemaphore          signalSemaphores[] = {gameDevice.renderFinishedSemaphores[imageIndex]};
  VkPipelineStageFlags waitStages[]       = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount           = 1;
  submitInfo.pWaitSemaphores              = waitSemaphores;
  submitInfo.pWaitDstStageMask            = waitStages;
  submitInfo.commandBufferCount           = 1;
  submitInfo.pCommandBuffers              = &gameDevice.commandBuffers[currentFrame];
  submitInfo.signalSemaphoreCount         = 1;
  submitInfo.pSignalSemaphores            = signalSemaphores;

  GameVulkanQueueSubmitter::submit(gameDevice.graphicsQueue, &submitInfo,
                                   gameDevice.inFlightFences[currentFrame]);

  VkSwapchainKHR   swapChains[] = {gameDevice.swapChain};
  VkPresentInfoKHR presentInfo{};
  presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores    = signalSemaphores;
  presentInfo.swapchainCount     = 1;
  presentInfo.pSwapchains        = swapChains;
  presentInfo.pImageIndices      = &imageIndex;

  vkQueuePresentKHR(gameDevice.presentQueue, &presentInfo);
  currentFrame = (currentFrame + 1) % maxFramesInFlight;
}

QueueFamilyIndices GameDeviceInstance::findQueueFamilies(VkPhysicalDevice device) const
{
  QueueFamilyIndices indices;
  uint32_t           queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
  size_t i = 0;
  for (const auto& queueFamily : queueFamilies)
  {
    if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
    {
      indices.graphicsFamily = i;
    }
    if (!gameDevice.headlessMode)
    {
      VkBool32 presentSupport = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(device, i, gameDevice.surface, &presentSupport);

      if (presentSupport)
      {
        indices.presentFamily = i;
      }
    }
    else
    {
      indices.presentFamily = indices.graphicsFamily;
    }
    if (indices.isComplete())
    {
      break;
    }
    i++;
  }
  return indices;
}

bool GameDeviceInstance::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
  uint32_t extensionCount = 0;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                       availableExtensions.data());

  std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
  if (gameDevice.headlessMode)
  {
    requiredExtensions.erase(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
  }

  for (const auto& extension : availableExtensions)
  {
    requiredExtensions.erase(extension.extensionName);
  }

  return requiredExtensions.empty();
}

bool GameDeviceInstance::isDeviceSuitable(VkPhysicalDevice device)
{
  QueueFamilyIndices indices = findQueueFamilies(device);

  bool extensionsSupported = checkDeviceExtensionSupport(device);
  bool swapChain           = false;

  if (!gameDevice.headlessMode)
  {
    if (extensionsSupported)
    {
      SwapChainSupport swapChainSupport = querySwapChainSupport(device);
      swapChain = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }
  }
  else
  {
    swapChain = true;
  }

  return indices.isComplete() && extensionsSupported && swapChain;
}

void GameDeviceInstance::tryCreateLogicalDevice(VkDeviceCreateInfo createInfo)
{
  VkResult result =
      vkCreateDevice(gameDevice.physicalDevice, &createInfo, nullptr, &gameDevice.device);
  if (result != VK_SUCCESS)
  {
    GameError::exitWithError("vkCreateDevice",
                             "Failed to create Vulkan Logical Device "
                             "(result = " +
                                 GameError::vulkanResultToString(result) + ")",
                             VK_NULL_HANDLE, gameDevice.physicalDevice, gameDevice.instance);
  }
}

void GameDeviceInstance::createLogicalDevice()
{
  QueueFamilyIndices indices = findQueueFamilies(gameDevice.physicalDevice);

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily, indices.presentFamily};
  float              queuePriority       = 1.0f;
  for (uint32_t queueFamily : uniqueQueueFamilies)
  {
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount       = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.emplace_back(queueCreateInfo);
  }
  VkPhysicalDeviceFeatures deviceFeatures{};
  VkDeviceCreateInfo       createInfo{};
  createInfo.sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.pQueueCreateInfos    = queueCreateInfos.data();
  createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
  createInfo.pEnabledFeatures     = &deviceFeatures;

  std::vector<const char*> enabledExtensions;
  if (!gameDevice.headlessMode)
  {
    enabledExtensions.insert(enabledExtensions.end(), deviceExtensions.begin(),
                             deviceExtensions.end());
  }

  uint32_t extensionCount = 0;
  vkEnumerateDeviceExtensionProperties(gameDevice.physicalDevice, nullptr, &extensionCount,
                                       nullptr);
  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(gameDevice.physicalDevice, nullptr, &extensionCount,
                                       availableExtensions.data());

  for (const char* optionalExt : optionalDeviceExtensions)
  {
    for (const auto& ext : availableExtensions)
    {
      if (strcmp(ext.extensionName, optionalExt) == 0)
      {
        enabledExtensions.push_back(optionalExt);
        break;
      }
    }
  }

  createInfo.enabledExtensionCount   = static_cast<uint32_t>(enabledExtensions.size());
  createInfo.ppEnabledExtensionNames = enabledExtensions.data();

  if (_enableValidationLayers)
  {
    createInfo.enabledLayerCount   = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
  }
  else
  {
    createInfo.enabledLayerCount = 0;
  }
  tryCreateLogicalDevice(createInfo);

  vkGetDeviceQueue(gameDevice.device, indices.graphicsFamily, 0, &gameDevice.graphicsQueue);
  vkGetDeviceQueue(gameDevice.device, indices.presentFamily, 0, &gameDevice.presentQueue);
}

#if defined(_WIN32)

void GameDeviceInstance::tryCreateSurface(VkWin32SurfaceCreateInfoKHR createInfo)
{
  VkResult result =
      vkCreateWin32SurfaceKHR(gameDevice.instance, &createInfo, nullptr, &gameDevice.surface);
  if (result != VK_SUCCESS)
  {
    GameError::exitWithError("vkCreateWin32SurfaceKHR",
                             "Failed to create Vulkan Win32 Surface "
                             "(result = " +
                                 GameError::vulkanResultToString(result) + ")");
  }
}

void GameDeviceInstance::createSurface()
{
  VkWin32SurfaceCreateInfoKHR createInfo{};
  createInfo.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
  createInfo.hwnd      = handle->hwnd;
  createInfo.hinstance = handle->hInstance;
  tryCreateSurface(createInfo);
}

#else
// TODO: Implement surface creation for other platforms (Linux, Android, etc.)
#endif

VkSurfaceFormatKHR
GameDeviceInstance::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
  for (const auto& availableFormat : availableFormats)
  {
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
        availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
    {
      return availableFormat;
    }
  }

  return availableFormats[0];
}

VkPresentModeKHR GameDeviceInstance::chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR>& availablePresentModes)
{
  for (const auto& availablePresentMode : availablePresentModes)
  {
    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
    {
      return availablePresentMode;
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D GameDeviceInstance::chooseSwapExtent(VkSurfaceCapabilitiesKHR capabilities)
{
  if (capabilities.currentExtent.width != UINT32_MAX)
  {
    if (capabilities.currentExtent.width == 0 || capabilities.currentExtent.height == 0)
    {
      throw;
    }
    return capabilities.currentExtent;
  }
  else
  {
#if defined(_WIN32)
    uint32_t width, height;
    RECT     rect{};
    if (GetClientRect(handle->hwnd, &rect))
    {
      width  = rect.right;
      height = rect.bottom;
    }
#else
    // Placeholder for now
    uint32_t width = 0, height = 0;
#endif
    if (width == 0 || height == 0)
    {
      throw;
    }
    VkExtent2D actualExtent = {width, height};

    actualExtent.width  = (actualExtent.width < capabilities.minImageExtent.width)
                              ? capabilities.minImageExtent.width
                          : (actualExtent.width > capabilities.maxImageExtent.width)
                              ? capabilities.maxImageExtent.width
                              : actualExtent.width;
    actualExtent.height = (actualExtent.height < capabilities.minImageExtent.height)
                              ? capabilities.minImageExtent.height
                          : (actualExtent.height > capabilities.maxImageExtent.height)
                              ? capabilities.maxImageExtent.height
                              : actualExtent.height;
    return actualExtent;
  }
}

void GameDeviceInstance::tryCreateSwapChain(VkSwapchainCreateInfoKHR createInfo)
{
  VkResult result =
      vkCreateSwapchainKHR(gameDevice.device, &createInfo, nullptr, &gameDevice.swapChain);
  if (result != VK_SUCCESS)
  {
    GameError::exitWithError("vkCreateSwapchainKHR", "Failed to create Vulkan Swap Chain "
                                                     "(result = " +
                                                         GameError::vulkanResultToString(result) +
                                                         ")");
  }
}

void GameDeviceInstance::createSwapChain()
{
  SwapChainSupport   swapChainSupport = querySwapChainSupport(gameDevice.physicalDevice);
  VkSurfaceFormatKHR surfaceFormat    = chooseSwapSurfaceFormat(swapChainSupport.formats);
  VkPresentModeKHR   presentMode      = chooseSwapPresentMode(swapChainSupport.presentModes);
  VkExtent2D         extent           = chooseSwapExtent(swapChainSupport.capabilities);

  uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
  if (swapChainSupport.capabilities.maxImageCount > 0 &&
      imageCount > swapChainSupport.capabilities.maxImageCount)
  {
    imageCount = swapChainSupport.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo{};
  createInfo.sType   = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = gameDevice.surface;

  createInfo.minImageCount    = imageCount;
  createInfo.imageFormat      = surfaceFormat.format;
  createInfo.imageColorSpace  = surfaceFormat.colorSpace;
  createInfo.imageExtent      = extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  QueueFamilyIndices indices              = findQueueFamilies(gameDevice.physicalDevice);
  uint32_t           queueFamilyIndices[] = {indices.graphicsFamily, indices.presentFamily};
  if (indices.graphicsFamily != indices.presentFamily)
  {
    createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices   = queueFamilyIndices;
  }
  else
  {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }
  createInfo.preTransform   = swapChainSupport.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode    = presentMode;
  createInfo.clipped        = VK_TRUE;
  createInfo.oldSwapchain   = gameDevice.swapChain;

  tryCreateSwapChain(createInfo);

  vkGetSwapchainImagesKHR(gameDevice.device, gameDevice.swapChain, &imageCount, nullptr);
  gameDevice.swapChainImages.resize(imageCount);
  vkGetSwapchainImagesKHR(gameDevice.device, gameDevice.swapChain, &imageCount,
                          gameDevice.swapChainImages.data());

  gameDevice.swapChainImageFormat = surfaceFormat.format;
  gameDevice.swapChainExtent      = extent;
}

void GameDeviceInstance::tryCreateImageView(VkImageViewCreateInfo createInfo,
                                            VkImageView*          imageView)
{
  VkResult result = vkCreateImageView(gameDevice.device, &createInfo, nullptr, imageView);
  if (result != VK_SUCCESS)
  {
    GameError::exitWithError("vkCreateImageView", "Failed to create Vulkan Image View "
                                                  "(result = " +
                                                      GameError::vulkanResultToString(result) +
                                                      ")");
  }
}

void GameDeviceInstance::createImageViews()
{
  gameDevice.swapChainImageViews.resize(gameDevice.swapChainImages.size());

  for (size_t i = 0; i < gameDevice.swapChainImages.size(); i++)
  {
    VkImageViewCreateInfo createInfo{};
    createInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image                           = gameDevice.swapChainImages[i];
    createInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format                          = gameDevice.swapChainImageFormat;
    createInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseMipLevel   = 0;
    createInfo.subresourceRange.levelCount     = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount     = 1;
    tryCreateImageView(createInfo, &gameDevice.swapChainImageViews[i]);
  }
}

void GameDeviceInstance::registerDrawable(IGameDrawable* ptr)
{
  MainGameDeviceArenaHandler& handler = getGameArena();
  auto&                       arena   = handler.arena;
  arena.pushDrawableAddress(*ptr);
}

MainGameDeviceArenaHandler& GameDeviceInstance::getGameArena()
{
  static std::unique_ptr<MainGameDeviceArenaHandler> arena =
      std::make_unique<MainGameDeviceArenaHandler>();
  return *arena;
}

GameResources& GameDeviceInstance::getGameResources()
{
  return gameResources;
}

VkCommandPool GameDeviceInstance::getCommandPool() const
{
  return gameDevice.commandPool;
}
VkCommandBuffer GameDeviceInstance::getCommandBuffer() const
{
  return gameDevice.commandBuffers[currentFrame];
}
VkSwapchainKHR GameDeviceInstance::getSwapChain() const
{
  return gameDevice.swapChain;
}
VkPhysicalDevice GameDeviceInstance::getPhysicalDevice() const
{
  return gameDevice.physicalDevice;
}
VkDevice GameDeviceInstance::getDevice() const
{
  return gameDevice.device;
}
VkQueue GameDeviceInstance::getGraphicsQueue() const
{
  return gameDevice.graphicsQueue;
}
VkQueue GameDeviceInstance::getPresentQueue() const
{
  return gameDevice.presentQueue;
}
VkInstance GameDeviceInstance::getInstance() const
{
  return gameDevice.instance;
}
VkExtent2D GameDeviceInstance::getExtent2d() const
{
  return gameDevice.swapChainExtent;
}
VkFramebuffer GameDeviceInstance::getFramebuffer() const
{
  return gameDevice.swapChainFramebuffers[currentFrame];
}
uint32_t GameDeviceInstance::getGraphicsFamily() const
{
  QueueFamilyIndices indices = findQueueFamilies(gameDevice.physicalDevice);
  return indices.graphicsFamily;
}
uint32_t GameDeviceInstance::getPresentFamily() const
{
  QueueFamilyIndices indices = findQueueFamilies(gameDevice.physicalDevice);
  return indices.presentFamily;
}
uint32_t GameDeviceInstance::getExtentWidth() const
{
#if defined(_WIN32)
  uint32_t width;
  RECT     rect{};
  if (GetClientRect(handle->hwnd, &rect))
  {
    width = rect.right;
  }
#else
  // Placeholder for now
  uint32_t width = 0;
#endif
  return width;
}
uint32_t GameDeviceInstance::getExtentHeight() const
{
#if defined(_WIN32)
  uint32_t height;
  RECT     rect{};
  if (GetClientRect(handle->hwnd, &rect))
  {
    height = rect.bottom;
  }
#else
  // Placeholder for now
  uint32_t height = 0;
#endif
  return height;
}
VkRenderPass GameDeviceInstance::getRenderPass() const
{
  return gameDevice.renderPass;
}

uint32_t GameDeviceInstance::getMaxFramesInFlight() const
{
  return maxFramesInFlight;
}

uint32_t GameDeviceInstance::getCurrentFrame() const
{
  return currentFrame;
}

} // namespace rl
