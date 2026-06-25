import Rl.Base.Game;
import Rl.Base.Shader;
import Rl.Client.State.CameraState;

import <algorithm>;
import <cstdint>;
import <cstdlib>;
import <iostream>;
import <set>;
import <stdexcept>;
import <glm/fwd.hpp>;

namespace Rl::Game
{

using namespace Rl::Providers;

const std::vector validationLayers = {"VK_LAYER_KHRONOS_validation"};

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

Game::Game() : input(Input::InputReceiver::GetInstance())
{
}

Game::~Game()
{
  input.Unsubscribe(this);
  input.Stop();
  CleanupGraphics();
}

void Game::Run()
{
  InitWindow();
  InitGraphics();
  InitInputReceiverObserver();
  while (!glfwWindowShouldClose(vkWindow))
  {
    glfwPollEvents();
    DrawFrame();
  }
  vkDeviceWaitIdle(vkContext.device);
}

void Game::Tick()
{
}

void Game::CleanupGraphics()
{
  CleanupResources();
  vkDestroySemaphore(vkContext.device, vkContext.imageAvailableSemaphore, nullptr);
  for (const auto semaphore : vkContext.renderFinishedSemaphores)
  {
    vkDestroySemaphore(vkContext.device, semaphore, nullptr);
  }
  vkDestroyFence(vkContext.device, vkContext.inFlightFence, nullptr);
  vkDestroyCommandPool(vkContext.device, vkContext.commandPool, nullptr);
  vkDestroyPipelineLayout(vkContext.device, vkContext.pipelineLayout, nullptr);
  vkDestroyDescriptorSetLayout(vkContext.device, vkContext.descriptorSetLayout, nullptr);
  vkDestroyRenderPass(vkContext.device, vkContext.renderPass, nullptr);
  for (const auto framebuffer : vkContext.swapChainFramebuffers)
  {
    vkDestroyFramebuffer(vkContext.device, framebuffer, nullptr);
  }
  for (const auto imageView : vkContext.swapChainImageViews)
  {
    vkDestroyImageView(vkContext.device, imageView, nullptr);
  }
  vkDestroySwapchainKHR(vkContext.device, vkContext.swapChain, nullptr);
  vkDestroySurfaceKHR(vkContext.instance, vkContext.surface, nullptr);
  vkDestroyDevice(vkContext.device, nullptr);
  vkDestroyInstance(vkContext.instance, nullptr);
  glfwDestroyWindow(vkWindow);
  glfwTerminate();
}
void Game::CleanupResources()
{
  cameraModel->GetDrawable().OnDestroy(
      cameraModel->GetResource(), cameraModel->GetVulkanState(), vkContext);
  cameraModel.reset();
}

void Game::InitInputReceiverObserver()
{
  input.Subscribe(this);
  input.Start();
}

void Game::OnKeyEvent(const Input::KeyEvent& event)
{
  if (cameraModel)
  {
    cameraModel->GetObject().OnKeyEvent(event);
  }
}

void Game::OnMouseButtonEvent(const Input::MouseButtonEvent& event)
{
  if (cameraModel)
  {
    cameraModel->GetObject().OnMouseButtonEvent(event);
  }
}

void Game::OnMouseMoveEvent(const Input::MouseMoveEvent& event)
{
  if (cameraModel)
  {
    cameraModel->GetObject().OnMouseMoveEvent(event);
  }
}

void Game::OnMouseScrollEvent(const Input::MouseScrollEvent& event)
{
  if (cameraModel)
  {
    cameraModel->GetObject().OnMouseScrollEvent(event);
  }
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
  vkWindow = glfwCreateWindow(width, height, "RL", nullptr, nullptr);
  // Configure monitor and window
  GLFWmonitor*       monitor = glfwGetPrimaryMonitor();
  const GLFWvidmode* mode    = glfwGetVideoMode(monitor);
  glfwSetWindowPos(vkWindow, (mode->width - width) / 2, (mode->height - height) / 2);
  // Disable cursor and center it for better camera
  glfwSetInputMode(vkWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwSetCursorPos(vkWindow, width / 2.0, height / 2.0);
  // Set GLFW cursor position callback
  glfwSetCursorPosCallback(vkWindow,
      [](GLFWwindow* window, double xpos, double ypos)
      {
        Game&                 game = Game::GetInstance();
        Input::MouseMoveEvent event;
        event.x = xpos;
        event.y = ypos;
        game.OnMouseMoveEvent(event);
      });
}

void Game::GetDeltaTime()
{
}

void Game::InputHandle()
{
}

void Game::UpdateEntities()
{
}

void Game::UpdateCamera()
{
}

void Game::UpdatePhysics()
{
}

void Game::UpdateAudio()
{
}

void Game::UpdateUI()
{
  cameraModel->UpdateFromStateModel(vkContext);
  unitModel->UpdateFromStateModel(vkContext);
}

void Game::UpdateLogic()
{
}

void Game::UpdateRender()
{
}

Game& Game::GetInstance()
{
  static Game game;
  return game;
}

VulkanContext& Game::GetVulkanContext()
{
  return this->vkContext;
}

void Game::CreateInstance()
{
  if (enableValidationLayers && !CheckValidationLayerSupport())
  {
    throw std::runtime_error("Validation layers requested, but not available");
  }
  VkApplicationInfo appInfo{};
  appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName   = "RL";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName        = "No Engine";
  appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion         = VK_API_VERSION_1_0;

  VkInstanceCreateInfo createInfo{};
  createInfo.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;

  const auto extensions              = GetRequiredExtensions();
  createInfo.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();
  if (enableValidationLayers)
  {
    createInfo.enabledLayerCount   = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
  }
  else
  {
    createInfo.enabledLayerCount = 0;
  }

  if (vkCreateInstance(&createInfo, nullptr, &vkContext.instance) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create instance");
  }
}

void Game::CreateSurface()
{
  if (glfwCreateWindowSurface(vkContext.instance, vkWindow, nullptr, &vkContext.surface) !=
      VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create window surface");
  }
}

void Game::CreateCameraModel()
{
  cameraModel = std::make_unique<CameraModel>(vkContext);
  // Set camera aspect ratio to match window dimensions
  cameraModel->GetObject().SetAspectRatio(static_cast<float>(width) / static_cast<float>(height));
  // Move camera back from origin to see the unit
  World::AbstractCamera::Eye eyePosition{};
  eyePosition.x = 0.0;
  eyePosition.y = 0.0;
  eyePosition.z = 5.0;
  cameraModel->GetObject().SetEyePosition(eyePosition);
}

void Game::CreateUnitModel()
{
  unitModel                            = std::make_unique<UnitModel>(vkContext);
  unitModel->GetResource().cameraModel = cameraModel.get();
}

void Game::CreateResources()
{
  CreateCameraModel();
  CreateUnitModel();
}

void Game::PickPhysicalDevice()
{
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(vkContext.instance, &deviceCount, nullptr);

  if (deviceCount == 0)
  {
    throw std::runtime_error("Failed to find GPUs with Vulkan support");
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(vkContext.instance, &deviceCount, devices.data());

  for (const auto& device : devices)
  {
    if (IsDeviceSuitable(device))
    {
      vkContext.physicalDevice = device;
      break;
    }
  }

  if (vkContext.physicalDevice == VK_NULL_HANDLE)
  {
    throw std::runtime_error("Failed to find a suitable GPU");
  }
}

void Game::CreateLogicalDevice()
{
  vkContext.queueFamilyIndices = FindQueueFamilies(vkContext.physicalDevice);

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set uniqueQueueFamilies = {vkContext.queueFamilyIndices.graphicsFamily.value(),
      vkContext.queueFamilyIndices.presentFamily.value()};

  float queuePriority = 1.0f;
  for (uint32_t queueFamily : uniqueQueueFamilies)
  {
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount       = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(queueCreateInfo);
  }

  VkPhysicalDeviceFeatures deviceFeatures{};

  VkDeviceCreateInfo createInfo{};
  createInfo.sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
  createInfo.pQueueCreateInfos    = queueCreateInfos.data();
  createInfo.pEnabledFeatures     = &deviceFeatures;

  const std::vector deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  createInfo.enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size());
  createInfo.ppEnabledExtensionNames = deviceExtensions.data();

  if (enableValidationLayers)
  {
    createInfo.enabledLayerCount   = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
  }
  else
  {
    createInfo.enabledLayerCount = 0;
  }

  if (vkCreateDevice(vkContext.physicalDevice, &createInfo, nullptr, &vkContext.device) !=
      VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create logical device");
  }

  vkGetDeviceQueue(vkContext.device, vkContext.queueFamilyIndices.graphicsFamily.value(), 0,
      &vkContext.graphicsQueue);
  vkGetDeviceQueue(vkContext.device, vkContext.queueFamilyIndices.presentFamily.value(), 0,
      &vkContext.presentQueue);
}

void Game::CreateSwapChain()
{
  VulkanContext::QueueFamilyIndices indices = FindQueueFamilies(vkContext.physicalDevice);
  VkSurfaceCapabilitiesKHR          caps;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkContext.physicalDevice, vkContext.surface, &caps);

  constexpr VkSurfaceFormatKHR surfaceFormat = {
      VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
  constexpr VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
  constexpr VkExtent2D       extent      = {width, height};

  uint32_t imageCount = caps.minImageCount + 1;
  if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount)
  {
    imageCount = caps.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo{};
  createInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface          = vkContext.surface;
  createInfo.minImageCount    = imageCount;
  createInfo.imageFormat      = surfaceFormat.format;
  createInfo.imageColorSpace  = surfaceFormat.colorSpace;
  createInfo.imageExtent      = extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  createInfo.preTransform     = caps.currentTransform;
  createInfo.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode      = presentMode;
  createInfo.clipped          = VK_TRUE;
  createInfo.oldSwapchain     = VK_NULL_HANDLE;

  if (vkCreateSwapchainKHR(vkContext.device, &createInfo, nullptr, &vkContext.swapChain) !=
      VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create swap chain");
  }

  vkGetSwapchainImagesKHR(vkContext.device, vkContext.swapChain, &imageCount, nullptr);
  vkContext.swapChainImages.resize(imageCount);
  vkGetSwapchainImagesKHR(
      vkContext.device, vkContext.swapChain, &imageCount, vkContext.swapChainImages.data());

  vkContext.swapChainImageFormat = surfaceFormat.format;
  vkContext.swapChainExtent      = extent;
}

void Game::CreateImageViews()
{
  vkContext.swapChainImageViews.resize(vkContext.swapChainImages.size());

  for (size_t i = 0; i < vkContext.swapChainImages.size(); i++)
  {
    VkImageViewCreateInfo createInfo{};
    createInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image                           = vkContext.swapChainImages[i];
    createInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format                          = vkContext.swapChainImageFormat;
    createInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseMipLevel   = 0;
    createInfo.subresourceRange.levelCount     = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount     = 1;

    if (vkCreateImageView(vkContext.device, &createInfo, nullptr,
            &vkContext.swapChainImageViews[i]) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create image views");
    }
  }
}

void Game::CreateRenderPass()
{
  VkAttachmentDescription colorAttachment{};
  colorAttachment.format         = vkContext.swapChainImageFormat;
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

  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments    = &colorAttachment;
  renderPassInfo.subpassCount    = 1;
  renderPassInfo.pSubpasses      = &subpass;

  if (vkCreateRenderPass(vkContext.device, &renderPassInfo, nullptr, &vkContext.renderPass) !=
      VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create Render pass");
  }
}

void Game::CreateFramebuffers()
{
  vkContext.swapChainFramebuffers.resize(vkContext.swapChainImageViews.size());

  for (size_t i = 0; i < vkContext.swapChainImageViews.size(); i++)
  {
    const VkImageView attachments[] = {vkContext.swapChainImageViews[i]};

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass      = vkContext.renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments    = attachments;
    framebufferInfo.width           = vkContext.swapChainExtent.width;
    framebufferInfo.height          = vkContext.swapChainExtent.height;
    framebufferInfo.layers          = 1;

    if (vkCreateFramebuffer(vkContext.device, &framebufferInfo, nullptr,
            &vkContext.swapChainFramebuffers[i]) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create Framebuffer");
    }
  }
}

void Game::CreateCommandPool()
{
  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = vkContext.queueFamilyIndices.graphicsFamily.value();

  if (vkCreateCommandPool(vkContext.device, &poolInfo, nullptr, &vkContext.commandPool) !=
      VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create command pool");
  }
}

void Game::CreateCommandBuffers()
{
  vkContext.commandBuffers.resize(1);
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool        = vkContext.commandPool;
  allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;

  if (vkAllocateCommandBuffers(vkContext.device, &allocInfo, &vkContext.commandBuffers[0]) !=
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

  if (vkCreateSemaphore(vkContext.device, &semaphoreInfo, nullptr,
          &vkContext.imageAvailableSemaphore) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create image available semaphore");
  }

  // Create one render finished semaphore per swap chain image
  vkContext.renderFinishedSemaphores.resize(vkContext.swapChainImages.size());
  for (size_t i = 0; i < vkContext.swapChainImages.size(); i++)
  {
    if (vkCreateSemaphore(vkContext.device, &semaphoreInfo, nullptr,
            &vkContext.renderFinishedSemaphores[i]) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create render finished semaphore");
    }
  }

  if (vkCreateFence(vkContext.device, &fenceInfo, nullptr, &vkContext.inFlightFence) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create synchronization objects for frame");
  }
}

void Game::DrawUI()
{
  // Only draw unit model, camera matrices are pushed in unit renderer
  unitModel->DrawFromStateModel(vkContext);
}

void Game::CreatePipelineLayout()
{
  // Create pipeline layout with push constants for camera matrices (3 mat4 = 192 bytes)
  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  pushConstantRange.offset     = 0;
  pushConstantRange.size       = 3 * sizeof(glm::mat4); // model, view, projection matrices

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount         = 0;
  pipelineLayoutInfo.pSetLayouts            = nullptr;
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges    = &pushConstantRange;

  if (vkCreatePipelineLayout(
          vkContext.device, &pipelineLayoutInfo, nullptr, &vkContext.pipelineLayout) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create pipeline layout");
  }
}

void Game::DrawFrame()
{
  vkWaitForFences(vkContext.device, 1, &vkContext.inFlightFence, VK_TRUE, UINT64_MAX);

  uint32_t imageIndex;
  vkAcquireNextImageKHR(vkContext.device, vkContext.swapChain, UINT64_MAX,
      vkContext.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

  vkResetFences(vkContext.device, 1, &vkContext.inFlightFence);
  vkResetCommandBuffer(vkContext.commandBuffers[0], 0);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (vkBeginCommandBuffer(vkContext.commandBuffers[0], &beginInfo) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to begin recording command buffer");
  }

  // Dispatch compute shader for frustum culling (before render pass)
  unitModel->DrawComputeFromStateModel(vkContext);

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass        = vkContext.renderPass;
  renderPassInfo.framebuffer       = vkContext.swapChainFramebuffers[imageIndex];
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = vkContext.swapChainExtent;

  constexpr VkClearValue clearColor = {{{0.0f, 1.0f, 0.0f, 1.0f}}};
  renderPassInfo.clearValueCount    = 1;
  renderPassInfo.pClearValues       = &clearColor;

  vkCmdBeginRenderPass(vkContext.commandBuffers[0], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
  // Set viewport
  VkViewport viewport{};
  viewport.x        = 0.0f;
  viewport.y        = 0.0f;
  viewport.width    = static_cast<float>(vkContext.swapChainExtent.width);
  viewport.height   = static_cast<float>(vkContext.swapChainExtent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(vkContext.commandBuffers[0], 0, 1, &viewport);

  // Set scissor
  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = vkContext.swapChainExtent;
  vkCmdSetScissor(vkContext.commandBuffers[0], 0, 1, &scissor);
  UpdateUI();
  DrawUI();
  vkCmdEndRenderPass(vkContext.commandBuffers[0]);

  if (vkEndCommandBuffer(vkContext.commandBuffers[0]) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to record command buffer");
  }

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  const VkSemaphore              waitSemaphores[] = {vkContext.imageAvailableSemaphore};
  constexpr VkPipelineStageFlags waitStages[]     = {
      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores    = waitSemaphores;
  submitInfo.pWaitDstStageMask  = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers    = &vkContext.commandBuffers[0];

  const VkSemaphore signalSemaphores[] = {vkContext.renderFinishedSemaphores[imageIndex]};
  submitInfo.signalSemaphoreCount      = 1;
  submitInfo.pSignalSemaphores         = signalSemaphores;

  if (vkQueueSubmit(vkContext.graphicsQueue, 1, &submitInfo, vkContext.inFlightFence) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to submit draw command buffer");
  }

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores    = signalSemaphores;

  const VkSwapchainKHR swapChains[] = {vkContext.swapChain};
  presentInfo.swapchainCount        = 1;
  presentInfo.pSwapchains           = swapChains;
  presentInfo.pImageIndices         = &imageIndex;
  vkQueuePresentKHR(vkContext.presentQueue, &presentInfo);
}

VulkanContext::QueueFamilyIndices Game::FindQueueFamilies(VkPhysicalDevice device) const
{
  VulkanContext::QueueFamilyIndices indices;

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

  int i = 0;
  for (const auto& queueFamily : queueFamilies)
  {
    if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
    {
      indices.graphicsFamily = i;
    }
    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, vkContext.surface, &presentSupport);
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

bool Game::IsDeviceSuitable(VkPhysicalDevice device)
{
  VulkanContext::QueueFamilyIndices indices = FindQueueFamilies(device);

  return indices.isComplete();
}

bool Game::CheckValidationLayerSupport()
{
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

std::vector<const char*> Game::GetRequiredExtensions()
{
  uint32_t     glfwExtensionCount = 0;
  const char** glfwExtensions     = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  std::vector  extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

  if (enableValidationLayers)
  {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  return extensions;
}

} // namespace Rl::Game

int main()
{
  Rl::Game::Game& game = Rl::Game::Game::GetInstance();
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
