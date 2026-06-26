export module Rl.Base.Binding;

import <optional>;
import <vulkan/vulkan.hpp>;

namespace Rl::Game {

/* Defines the main vulkan resources for the Game */
export struct MainBinding
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

  /* This should be initialized manually */
  MainBinding() = default;

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

}