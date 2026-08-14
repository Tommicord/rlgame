#ifndef RL_BASE_GAME_VULKAN_CALLBACK_H
#define RL_BASE_GAME_VULKAN_CALLBACK_H

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <vulkan/vulkan.hpp>

namespace rl
{

/**
 * @brief Vulkan debug callback utilities for validation layer error tracking
 *
 * This class provides static methods to set up Vulkan debug callbacks that
 * log validation errors with stack traces using the Log system.
 */
class GameVulkanCallback
{
        public:
                /**
                 * @brief Sets up the Vulkan debug messenger callback
                 * @param instance The Vulkan instance to attach the callback to
                 * @return The debug messenger handle, or VK_NULL_HANDLE if setup fails
                 */
                static VkDebugUtilsMessengerEXT setupDebugCallback(VkInstance instance);

                /**
                 * @brief Destroys the Vulkan debug messenger callback
                 * @param instance The Vulkan instance
                 * @param debugMessenger The debug messenger handle to destroy
                 */
                static void destroyDebugCallback(VkInstance               instance,
                                                 VkDebugUtilsMessengerEXT debugMessenger);

                /**
                 * @brief Vulkan debug callback function
                 * @param messageSeverity Severity of the message
                 * @param messageType Type of the message
                 * @param pCallbackData Callback data containing the message
                 * @param pUserData User data passed to the callback
                 * @return VK_FALSE to not abort the call
                 */
                static VKAPI_ATTR VkBool32 VKAPI_CALL
                debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                              VkDebugUtilsMessageTypeFlagsEXT             messageType,
                              const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                              void*                                       pUserData);
};

} // namespace rl

#endif // RL_BASE_GAME_VULKAN_CALLBACK_H
