#include "rl/Base/Game.h"
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <set>
#include <cstdint>
#include <algorithm>

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

GameLauncher::GameLauncher() {
}

GameLauncher::~GameLauncher() {
    CleanupGraphics();
}

void GameLauncher::Run() {
    InitWindow();
    InitGraphics();
    while (!glfwWindowShouldClose(vkWindow)) {
        glfwPollEvents();
        DrawFrame();
    }
    vkDeviceWaitIdle(vkContext.device);
}

void GameLauncher::Tick() {
}

void GameLauncher::Render() {
}

void GameLauncher::CleanupGraphics() {
    CleanupSwapChain();
    
    vkDestroySemaphore(vkContext.device, vkContext.renderFinishedSemaphore, nullptr);
    vkDestroySemaphore(vkContext.device, vkContext.imageAvailableSemaphore, nullptr);
    vkDestroyFence(vkContext.device, vkContext.inFlightFence, nullptr);
    
    vkDestroyCommandPool(vkContext.device, vkContext.commandPool, nullptr);
    
    vkDestroyDevice(vkContext.device, nullptr);
    vkDestroySurfaceKHR(vkContext.instance, vkContext.surface, nullptr);
    vkDestroyInstance(vkContext.instance, nullptr);
    
    glfwDestroyWindow(vkWindow);
    glfwTerminate();
}

void GameLauncher::InitGraphics() {
    CreateInstance();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
    CreateSwapChain();
    CreateImageViews();
    CreateRenderPass();
    CreateGraphicsPipeline();
    CreateFramebuffers();
    CreateCommandPool();
    CreateCommandBuffers();
    CreateSyncObjects();
}

void GameLauncher::InitWindow() {
    glfwInit();
    
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    
    vkWindow = glfwCreateWindow(width, height, "RL", nullptr, nullptr);
}

void GameLauncher::GetDeltaTime() {
}

void GameLauncher::InputHandle() {
}

void GameLauncher::UpdateEntities() {
}

void GameLauncher::UpdateCamera() {
}

void GameLauncher::UpdatePhysics() {
}

void GameLauncher::UpdateAudio() {
}

void GameLauncher::UpdateUI() {
}

void GameLauncher::UpdateLogic() {
}

void GameLauncher::UpdateRender() {
}

void GameLauncher::CreateInstance() {
    if (enableValidationLayers && !CheckValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }
    
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
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

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }
    
    if (vkCreateInstance(&createInfo, nullptr, &vkContext.instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }
}

void GameLauncher::SetupDebugMessenger() {
}

void GameLauncher::CreateSurface() {
    if (glfwCreateWindowSurface(vkContext.instance, vkWindow, nullptr, &vkContext.surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

void GameLauncher::PickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vkContext.instance, &deviceCount, nullptr);
    
    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }
    
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(vkContext.instance, &deviceCount, devices.data());
    
    for (const auto& device : devices) {
        if (IsDeviceSuitable(device)) {
            vkContext.physicalDevice = device;
            break;
        }
    }
    
    if (vkContext.physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

void GameLauncher::CreateLogicalDevice() {
    vkContext.queueFamilyIndices = FindQueueFamilies(vkContext.physicalDevice);
    
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
        vkContext.queueFamilyIndices.graphicsFamily.value(),
        vkContext.queueFamilyIndices.presentFamily.value()
    };
    
    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
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
    
    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }
    
    if (vkCreateDevice(vkContext.physicalDevice, &createInfo, nullptr, &vkContext.device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }
    
    vkGetDeviceQueue(vkContext.device, vkContext.queueFamilyIndices.graphicsFamily.value(), 0, &vkContext.graphicsQueue);
    vkGetDeviceQueue(vkContext.device, vkContext.queueFamilyIndices.presentFamily.value(), 0, &vkContext.presentQueue);
}

void GameLauncher::CreateSwapChain() {
    VulkanContext::QueueFamilyIndices indices = FindQueueFamilies(vkContext.physicalDevice);
    
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkContext.physicalDevice, vkContext.surface, &caps);
    
    VkSurfaceFormatKHR surfaceFormat = {VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    VkExtent2D extent = { width, height };
    
    uint32_t imageCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount) {
        imageCount = caps.maxImageCount;
    }
    
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = vkContext.surface;
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
    
    if (vkCreateSwapchainKHR(vkContext.device, &createInfo, nullptr, &vkContext.swapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }
    
    vkGetSwapchainImagesKHR(vkContext.device, vkContext.swapChain, &imageCount, nullptr);
    vkContext.swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(vkContext.device, vkContext.swapChain, &imageCount, vkContext.swapChainImages.data());
    
    vkContext.swapChainImageFormat = surfaceFormat.format;
    vkContext.swapChainExtent = extent;
}

void GameLauncher::CreateImageViews() {
    vkContext.swapChainImageViews.resize(vkContext.swapChainImages.size());
    
    for (size_t i = 0; i < vkContext.swapChainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = vkContext.swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = vkContext.swapChainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        
        if (vkCreateImageView(vkContext.device, &createInfo, nullptr, &vkContext.swapChainImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image views!");
        }
    }
}

void GameLauncher::CreateRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = vkContext.swapChainImageFormat;
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
    
    if (vkCreateRenderPass(vkContext.device, &renderPassInfo, nullptr, &vkContext.renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}

void GameLauncher::CreateGraphicsPipeline() {
    const std::string vertShaderCode = R"(
        #version 450
        vec2 positions[3] = vec2[](
            vec2(0.0, -0.5),
            vec2(0.5, 0.5),
            vec2(-0.5, 0.5)
        );
        vec3 colors[3] = vec3[](
            vec3(1.0, 0.0, 0.0),
            vec3(0.0, 1.0, 0.0),
            vec3(0.0, 0.0, 1.0)
        );
        layout(location = 0) out vec3 fragColor;
        void main() {
            gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
            fragColor = colors[gl_VertexIndex];
        }
    )";
    
    const std::string fragShaderCode = R"(
        #version 450
        layout(location = 0) in vec3 fragColor;
        layout(location = 0) out vec4 outColor;
        void main() {
            outColor = vec4(fragColor, 1.0);
        }
    )";
    
    auto createShaderModule = [&](const std::string& code) -> VkShaderModule {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
        
        VkShaderModule shaderModule;
        if (vkCreateShaderModule(vkContext.device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }
        return shaderModule;
    };
    
    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);
    
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";
    
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";
    
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
    
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)vkContext.swapChainExtent.width;
    viewport.height = (float)vkContext.swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = vkContext.swapChainExtent;
    
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;
    
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    
    if (vkCreatePipelineLayout(vkContext.device, &pipelineLayoutInfo, nullptr, &vkContext.pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }
    
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = vkContext.pipelineLayout;
    pipelineInfo.renderPass = vkContext.renderPass;
    pipelineInfo.subpass = 0;
    
    if (vkCreateGraphicsPipelines(vkContext.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &vkContext.graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }
    
    vkDestroyShaderModule(vkContext.device, fragShaderModule, nullptr);
    vkDestroyShaderModule(vkContext.device, vertShaderModule, nullptr);
}

void GameLauncher::CreateFramebuffers() {
    vkContext.swapChainFramebuffers.resize(vkContext.swapChainImageViews.size());
    
    for (size_t i = 0; i < vkContext.swapChainImageViews.size(); i++) {
        VkImageView attachments[] = {
            vkContext.swapChainImageViews[i]
        };
        
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = vkContext.renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = vkContext.swapChainExtent.width;
        framebufferInfo.height = vkContext.swapChainExtent.height;
        framebufferInfo.layers = 1;
        
        if (vkCreateFramebuffer(vkContext.device, &framebufferInfo, nullptr, &vkContext.swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void GameLauncher::CreateCommandPool() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = vkContext.queueFamilyIndices.graphicsFamily.value();
    
    if (vkCreateCommandPool(vkContext.device, &poolInfo, nullptr, &vkContext.commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}

void GameLauncher::CreateCommandBuffers() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = vkContext.commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    
    if (vkAllocateCommandBuffers(vkContext.device, &allocInfo, &vkContext.commandBuffers[0]) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

void GameLauncher::CreateSyncObjects() {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    if (vkCreateSemaphore(vkContext.device, &semaphoreInfo, nullptr, &vkContext.imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(vkContext.device, &semaphoreInfo, nullptr, &vkContext.renderFinishedSemaphore) != VK_SUCCESS ||
        vkCreateFence(vkContext.device, &fenceInfo, nullptr, &vkContext.inFlightFence) != VK_SUCCESS) {
        throw std::runtime_error("failed to create synchronization objects for a frame!");
    }
}

void GameLauncher::DrawFrame() {
    vkWaitForFences(vkContext.device, 1, &vkContext.inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(vkContext.device, 1, &vkContext.inFlightFence);
    
    uint32_t imageIndex;
    vkAcquireNextImageKHR(vkContext.device, vkContext.swapChain, UINT64_MAX, vkContext.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
    
    vkResetCommandBuffer(vkContext.commandBuffers[0], /*VkCommandBufferResetFlagBits*/ 0);
    
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    
    if (vkBeginCommandBuffer(vkContext.commandBuffers[0], &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }
    
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = vkContext.renderPass;
    renderPassInfo.framebuffer = vkContext.swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = vkContext.swapChainExtent;
    
    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;
    
    vkCmdBeginRenderPass(vkContext.commandBuffers[0], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    vkCmdBindPipeline(vkContext.commandBuffers[0], VK_PIPELINE_BIND_POINT_GRAPHICS, vkContext.graphicsPipeline);
    
    vkCmdDraw(vkContext.commandBuffers[0], 3, 1, 0, 0);
    
    vkCmdEndRenderPass(vkContext.commandBuffers[0]);
    
    if (vkEndCommandBuffer(vkContext.commandBuffers[0]) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
    
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    
    VkSemaphore waitSemaphores[] = {vkContext.imageAvailableSemaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vkContext.commandBuffers[0];
    
    VkSemaphore signalSemaphores[] = {vkContext.renderFinishedSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    
    if (vkQueueSubmit(vkContext.graphicsQueue, 1, &submitInfo, vkContext.inFlightFence) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }
    
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    
    VkSwapchainKHR swapChains[] = {vkContext.swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    
    vkQueuePresentKHR(vkContext.presentQueue, &presentInfo);
}

void GameLauncher::RecreateSwapChain() {
}

void GameLauncher::CleanupSwapChain() {
    for (auto framebuffer : vkContext.swapChainFramebuffers) {
        vkDestroyFramebuffer(vkContext.device, framebuffer, nullptr);
    }
    
    for (auto imageView : vkContext.swapChainImageViews) {
        vkDestroyImageView(vkContext.device, imageView, nullptr);
    }
    
    vkDestroySwapchainKHR(vkContext.device, vkContext.swapChain, nullptr);
}

VulkanContext::QueueFamilyIndices GameLauncher::FindQueueFamilies(VkPhysicalDevice device) {
    VulkanContext::QueueFamilyIndices indices;
    
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
    
    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }
        
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, vkContext.surface, &presentSupport);
        
        if (presentSupport) {
            indices.presentFamily = i;
        }
        
        if (indices.isComplete()) {
            break;
        }
        
        i++;
    }
    
    return indices;
}

bool GameLauncher::IsDeviceSuitable(VkPhysicalDevice device) {
    VulkanContext::QueueFamilyIndices indices = FindQueueFamilies(device);
    
    return indices.isComplete();
}

bool GameLauncher::CheckValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
    
    for (const char* layerName : validationLayers) {
        bool layerFound = false;
        
        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }
        
        if (!layerFound) {
            return false;
        }
    }
    
    return true;
}

std::vector<const char*> GameLauncher::GetRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    
    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    
    return extensions;
}

int main() {
    GameLauncher game;
    
    try {
        game.Run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}