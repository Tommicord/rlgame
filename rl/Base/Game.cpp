import Rl.Base.Game;
import Rl.Base.Shader;
import Rl.Base.Binding;
import Rl.Base.UserInput;
import Rl.Client.State.UnitState;
import Rl.Player.PlayerCamera;
import Rl.Player.CameraController;

import <algorithm>;
import <cstdint>;
import <cstdlib>;
import <iostream>;
import <set>;
import <GLFW/glfw3.h>;
import <stdexcept>;
import <vulkan/vulkan.hpp>;
import <glm/glm.hpp>;
import <vector>;

namespace Rl::Main
{

using namespace Rl::Providers;

const std::vector validationLayers = {"VK_LAYER_KHRONOS_validation"};

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

Game::Game() : input(Input::UserInput::GetInstance())
{
}

Game::~Game()
{
  input.Stop();
  DestroyGraphics();
}

void Game::Run()
{
  InitWindow();
  InitGraphics();

  while (!glfwWindowShouldClose(window))
  {
    glfwPollEvents();
    Draw();
  }
  vkDeviceWaitIdle(binding.device);
}

void Game::DestroyGraphics()
{
  DestroyResources();
  vkDestroySemaphore(binding.device, binding.imageAvailableSemaphore, nullptr);
  for (const auto semaphore : binding.renderFinishedSemaphores)
  {
    vkDestroySemaphore(binding.device, semaphore, nullptr);
  }
  vkDestroyFence(binding.device, binding.inFlightFence, nullptr);
  vkDestroyCommandPool(binding.device, binding.commandPool, nullptr);
  vkDestroyPipelineLayout(binding.device, binding.pipelineLayout, nullptr);
  vkDestroyDescriptorSetLayout(binding.device, binding.descriptorSetLayout, nullptr);
  vkDestroyRenderPass(binding.device, binding.renderPass, nullptr);
  for (const auto framebuffer : binding.swapChainFramebuffers)
  {
    vkDestroyFramebuffer(binding.device, framebuffer, nullptr);
  }
  for (const auto imageView : binding.swapChainImageViews)
  {
    vkDestroyImageView(binding.device, imageView, nullptr);
  }
  vkDestroySwapchainKHR(binding.device, binding.swapChain, nullptr);
  vkDestroySurfaceKHR(binding.instance, binding.surface, nullptr);
  vkDestroyDevice(binding.device, nullptr);
  vkDestroyInstance(binding.instance, nullptr);
  glfwDestroyWindow(window);
  glfwTerminate();
}
void Game::DestroyResources()
{
  unitModel.reset();
}

void Game::InitGraphics()
{
  CreateInstance();
  CreateSurface();
  PickPhysicalDevice();
  CreateLogicalDevice();
  CreateSwapChain();
  CreateImageViews();
  CreateRenderPass();
  CreatePipelineLayout();
  CreateResources();
  CreateFramebuffers();
  CreateCommandPool();
  CreateCommandBuffers();
  CreateSyncObjects();
}

void Game::InitWindow()
{
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  window = glfwCreateWindow(width, height, "The Real Game", nullptr, nullptr);
  GLFWmonitor*       monitor = glfwGetPrimaryMonitor();
  const GLFWvidmode* mode = glfwGetVideoMode(monitor);
  glfwSetWindowPos(window, (mode->width - width) / 2, (mode->height - height) / 2);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwSetCursorPos(window, width / 2.0, height / 2.0);

  glfwSetCursorPosCallback(window,
      [](GLFWwindow* window, const double xpos, const double ypos)
      {
        const Game&           game = GetInstance();
        Input::MouseMoveEvent event{};
        event.x = xpos;
        event.y = ypos;
        game.input.NotifyMouseMoveEvent(event);
      });

  glfwSetKeyCallback(window,
      [](GLFWwindow* window, const int key, const int scancode, const int action,
          const int mods)
      {
        const Game&     game = GetInstance();
        Input::KeyEvent event{};
        event.key = Input::GlfwKeyToInputKey(key);
        event.action = static_cast<Input::Action>(action);
        event.modifiers = mods;
        game.input.NotifyKeyEvent(event);
      });

  glfwSetMouseButtonCallback(window,
      [](GLFWwindow* window, const int button, const int action, const int mods)
      {
        const Game&             game = GetInstance();
        Input::MouseButtonEvent event{};
        event.button = static_cast<Input::MouseButton>(button);
        event.action = static_cast<Input::Action>(action);
        event.modifiers = mods;
        game.input.NotifyMouseButtonEvent(event);
      });

  glfwSetScrollCallback(window,
      [](GLFWwindow* window, const double xoffset, const double yoffset)
      {
        const Game&             game = GetInstance();
        Input::MouseScrollEvent event{};
        event.xOffset = xoffset;
        event.yOffset = yoffset;
        game.input.NotifyMouseScrollEvent(event);
      });
}

void Game::UpdateModels()
{
  unitModel->Update(binding);
}

Game& Game::GetInstance()
{
  static Game game;
  return game;
}

MainBinding& Game::GetMainBinding()
{
  return this->binding;
}

void Game::CreateInstance()
{
  if (!CheckValidationLayerSupport())
  {
    throw std::runtime_error("Validation layers requested but not available");
  }
  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "RL";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "No Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;

  const auto extensions = GetRequiredExtensions();
  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();
  if (enableValidationLayers)
  {
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
  }
  else
  {
    createInfo.enabledLayerCount = 0;
  }

  if (vkCreateInstance(&createInfo, nullptr, &binding.instance) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create instance");
  }
}

void Game::CreateSurface()
{
  if (glfwCreateWindowSurface(binding.instance, window, nullptr, &binding.surface) !=
      VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create window surface");
  }
}

void Game::CreateUnitModel()
{
  unitModel = std::make_unique<UnitModel>(binding);
}

void Game::CreateResources()
{
  CreateUnitModel();
}

void Game::PickPhysicalDevice()
{
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(binding.instance, &deviceCount, nullptr);

  if (deviceCount == 0)
  {
    throw std::runtime_error("Failed to find GPUs with Vulkan support");
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(binding.instance, &deviceCount, devices.data());

  for (const auto& device : devices)
  {
    if (IsDeviceSuitable(device))
    {
      binding.physicalDevice = device;
      break;
    }
  }

  if (binding.physicalDevice == VK_NULL_HANDLE)
  {
    throw std::runtime_error("Failed to find a suitable GPU");
  }
}

void Game::CreateLogicalDevice()
{
  binding.queueFamilyIndices = FindQueueFamilies(binding.physicalDevice);

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set uniqueQueueFamilies = {binding.queueFamilyIndices.graphicsFamily.value(),
      binding.queueFamilyIndices.presentFamily.value()};

  float queuePriority = 1.0f;
  for (uint32_t queueFamily : uniqueQueueFamilies)
  {
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(queueCreateInfo);
  }

  VkPhysicalDeviceFeatures deviceFeatures{};

  VkDeviceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
  createInfo.pQueueCreateInfos = queueCreateInfos.data();
  createInfo.pEnabledFeatures = &deviceFeatures;

  const std::vector deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
  createInfo.ppEnabledExtensionNames = deviceExtensions.data();

  if (enableValidationLayers)
  {
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
  }
  else
  {
    createInfo.enabledLayerCount = 0;
  }

  if (vkCreateDevice(binding.physicalDevice, &createInfo, nullptr, &binding.device) !=
      VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create logical device");
  }

  vkGetDeviceQueue(binding.device, binding.queueFamilyIndices.graphicsFamily.value(), 0,
      &binding.graphicsQueue);
  vkGetDeviceQueue(binding.device, binding.queueFamilyIndices.presentFamily.value(), 0,
      &binding.presentQueue);
}

void Game::CreateSwapChain()
{
  MainBinding::QueueFamilyIndices indices = FindQueueFamilies(binding.physicalDevice);
  VkSurfaceCapabilitiesKHR        caps;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
      binding.physicalDevice, binding.surface, &caps);

  constexpr VkSurfaceFormatKHR surfaceFormat = {
      VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
  constexpr VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
  constexpr VkExtent2D       extent = {width, height};

  uint32_t imageCount = caps.minImageCount + 1;
  if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount)
  {
    imageCount = caps.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = binding.surface;
  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  createInfo.preTransform = caps.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;
  createInfo.oldSwapchain = VK_NULL_HANDLE;

  if (vkCreateSwapchainKHR(binding.device, &createInfo, nullptr, &binding.swapChain) !=
      VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create swap chain");
  }

  vkGetSwapchainImagesKHR(binding.device, binding.swapChain, &imageCount, nullptr);
  binding.swapChainImages.resize(imageCount);
  vkGetSwapchainImagesKHR(
      binding.device, binding.swapChain, &imageCount, binding.swapChainImages.data());

  binding.swapChainImageFormat = surfaceFormat.format;
  binding.swapChainExtent = extent;
}

void Game::CreateImageViews()
{
  binding.swapChainImageViews.resize(binding.swapChainImages.size());

  for (size_t i = 0; i < binding.swapChainImages.size(); i++)
  {
    VkImageViewCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = binding.swapChainImages[i];
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = binding.swapChainImageFormat;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(binding.device, &createInfo, nullptr,
            &binding.swapChainImageViews[i]) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create image views");
    }
  }
}

void Game::CreateRenderPass()
{
  VkAttachmentDescription colorAttachment{};
  colorAttachment.format = binding.swapChainImageFormat;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef{};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;

  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;

  if (vkCreateRenderPass(binding.device, &renderPassInfo, nullptr, &binding.renderPass) !=
      VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create Render pass");
  }
}

void Game::CreateFramebuffers()
{
  binding.swapChainFramebuffers.resize(binding.swapChainImageViews.size());

  for (size_t i = 0; i < binding.swapChainImageViews.size(); i++)
  {
    const VkImageView attachments[] = {binding.swapChainImageViews[i]};

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = binding.renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = binding.swapChainExtent.width;
    framebufferInfo.height = binding.swapChainExtent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(binding.device, &framebufferInfo, nullptr,
            &binding.swapChainFramebuffers[i]) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create Framebuffer");
    }
  }
}

void Game::CreateCommandPool()
{
  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = binding.queueFamilyIndices.graphicsFamily.value();

  if (vkCreateCommandPool(binding.device, &poolInfo, nullptr, &binding.commandPool) !=
      VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create command pool");
  }
}

void Game::CreateCommandBuffers()
{
  binding.commandBuffers.resize(1);
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = binding.commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;

  if (vkAllocateCommandBuffers(binding.device, &allocInfo, &binding.commandBuffers[0]) !=
      VK_SUCCESS)
  {
    throw std::runtime_error("Failed to allocate command buffers");
  }
}

void Game::CreateSyncObjects()
{
  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  if (vkCreateSemaphore(binding.device, &semaphoreInfo, nullptr,
          &binding.imageAvailableSemaphore) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create image available semaphore");
  }

  binding.renderFinishedSemaphores.resize(binding.swapChainImages.size());
  for (size_t i = 0; i < binding.swapChainImages.size(); i++)
  {
    if (vkCreateSemaphore(binding.device, &semaphoreInfo, nullptr,
            &binding.renderFinishedSemaphores[i]) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create render finished semaphore");
    }
  }

  if (vkCreateFence(binding.device, &fenceInfo, nullptr, &binding.inFlightFence) !=
      VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create synchronization objects for frame");
  }
}

void Game::DrawModels()
{
  unitModel->Draw(binding);
}

void Game::CreatePipelineLayout()
{
  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = 3 * sizeof(glm::mat4); // model, view, projection matrices

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 0;
  pipelineLayoutInfo.pSetLayouts = nullptr;
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

  if (vkCreatePipelineLayout(binding.device, &pipelineLayoutInfo, nullptr,
          &binding.pipelineLayout) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create pipeline layout");
  }
}

void Game::Draw()
{
  vkWaitForFences(binding.device, 1, &binding.inFlightFence, VK_TRUE, UINT64_MAX);

  uint32_t imageIndex;
  vkAcquireNextImageKHR(binding.device, binding.swapChain, UINT64_MAX,
      binding.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

  vkResetFences(binding.device, 1, &binding.inFlightFence);
  vkResetCommandBuffer(binding.commandBuffers[0], 0);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (vkBeginCommandBuffer(binding.commandBuffers[0], &beginInfo) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to begin recording command buffer");
  }

  unitModel->DrawCompute(binding);

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = binding.renderPass;
  renderPassInfo.framebuffer = binding.swapChainFramebuffers[imageIndex];
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = binding.swapChainExtent;

  constexpr VkClearValue clearColor = {{{0.0f, 1.0f, 0.0f, 1.0f}}};
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clearColor;

  vkCmdBeginRenderPass(
      binding.commandBuffers[0], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
  // Set viewport
  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(binding.swapChainExtent.width);
  viewport.height = static_cast<float>(binding.swapChainExtent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(binding.commandBuffers[0], 0, 1, &viewport);

  // Set scissor
  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = binding.swapChainExtent;
  vkCmdSetScissor(binding.commandBuffers[0], 0, 1, &scissor);
  UpdateModels();
  DrawModels();
  vkCmdEndRenderPass(binding.commandBuffers[0]);

  if (vkEndCommandBuffer(binding.commandBuffers[0]) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to record command buffer");
  }

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  const VkSemaphore              waitSemaphores[] = {binding.imageAvailableSemaphore};
  constexpr VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &binding.commandBuffers[0];

  const VkSemaphore signalSemaphores[] = {binding.renderFinishedSemaphores[imageIndex]};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  if (vkQueueSubmit(binding.graphicsQueue, 1, &submitInfo, binding.inFlightFence) !=
      VK_SUCCESS)
  {
    throw std::runtime_error("Failed to submit draw command buffer");
  }

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;

  const VkSwapchainKHR swapChains[] = {binding.swapChain};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = &imageIndex;
  vkQueuePresentKHR(binding.presentQueue, &presentInfo);
}

MainBinding::QueueFamilyIndices Game::FindQueueFamilies(VkPhysicalDevice device) const
{
  MainBinding::QueueFamilyIndices indices;

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(
      device, &queueFamilyCount, queueFamilies.data());

  int i = 0;
  for (const auto& queueFamily : queueFamilies)
  {
    if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
    {
      indices.graphicsFamily = i;
    }
    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, binding.surface, &presentSupport);
    if (presentSupport)
    {
      indices.presentFamily = i;
    }
    if (indices.isComplete())
    {
      break;
    }
    i++;
  }

  return indices;
}

bool Game::IsDeviceSuitable(const VkPhysicalDevice device) const
{
  const MainBinding::QueueFamilyIndices indices = FindQueueFamilies(device);

  return indices.isComplete();
}

bool Game::CheckValidationLayerSupport() const
{
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  for (const auto layerName : validationLayers)
  {
    bool layerFound = false;
    for (const auto& layerProperties : availableLayers)
    {
      if (std::strcmp(layerName, layerProperties.layerName) == 0)
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

std::vector<const char*> Game::GetRequiredExtensions() const
{
  uint32_t    glfwExtensionCount = 0;
  const auto  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

  if (enableValidationLayers)
  {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  return extensions;
}

} // namespace Rl::Game

int main()
{
  Rl::Main::Game& game = Rl::Main::Game::GetInstance();
  try
  {
    game.Run();
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
