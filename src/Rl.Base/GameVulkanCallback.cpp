#include "Rl.Base/GameVulkanCallback.h"

#include "Rl.Log/Log.h"
#include "Rl.Log/LogStackTrace.h"

#include <cstring>
#include <cstdint>
#include <vulkan/vulkan.hpp>
#include <vector>

namespace rl
{

VkDebugUtilsMessengerEXT GameVulkanCallback::setupDebugCallback(VkInstance instance)
{
        if (instance == VK_NULL_HANDLE)
        {
                return VK_NULL_HANDLE;
        }

        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
        createInfo.pUserData = nullptr;

        auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

        if (func == nullptr)
        {
                Log::error("Failed to load vkCreateDebugUtilsMessengerEXT");
                return VK_NULL_HANDLE;
        }

        VkDebugUtilsMessengerEXT debugMessenger;
        VkResult result = func(instance, &createInfo, nullptr, &debugMessenger);

        if (result != VK_SUCCESS)
        {
                Log::error("Failed to set up debug callback: %d", result);
                return VK_NULL_HANDLE;
        }

        return debugMessenger;
}

void GameVulkanCallback::destroyDebugCallback(VkInstance               instance,
                                              VkDebugUtilsMessengerEXT debugMessenger)
{
        if (instance == VK_NULL_HANDLE || debugMessenger == VK_NULL_HANDLE)
        {
                return;
        }
        auto debugMessengerCallback = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
        if (debugMessengerCallback != nullptr)
        {
                debugMessengerCallback(instance, debugMessenger, nullptr);
        }
}

VKAPI_ATTR VkBool32 VKAPI_CALL GameVulkanCallback::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT             messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void*                                       pUserData)
{
        (void)pUserData;
        (void)messageType;

        if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        {
                Log::error("Vulkan Validation Error: %s", pCallbackData->pMessage);
                
                std::vector<StackFrame> frames(logMaxStackFrames);
                int frameCount = LogStackTrace::capture(frames.data(), logMaxStackFrames);

                if (frameCount > 0)
                {
                        for (int i = 0; i < frameCount; i++)
                        {
                                LogStackTrace::demangle(&frames[i]);
                        }
                        std::vector<char> stackBuffer(LOG_BUFFER_SIZE);
                        LogStackTrace::formatStackTrace(stackBuffer.data(), sizeof(stackBuffer),
                                                        frames.data(), frameCount);
                        if (stackBuffer[0] != '\0')
                        {
                                Log::error("Stack trace:\n%s", stackBuffer.data());
                        }
                }
                
                if (pCallbackData->objectCount > 0)
                {
                        for (uint32_t i = 0; i < pCallbackData->objectCount; ++i)
                        {
                                const VkDebugUtilsObjectNameInfoEXT& obj = pCallbackData->pObjects[i];
                                Log::trace("  Object[%u]: Type=%u, Handle=0x%llx, Name='%s'", i,
                                          obj.objectType,
                                          obj.objectHandle,
                                          obj.pObjectName ? obj.pObjectName : "(null)");
                        }
                }
        }
        else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        {
                Log::warning("Vulkan Validation Warning: %s", pCallbackData->pMessage);
        }
        else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
        {
                Log::info("Vulkan Validation Info: %s", pCallbackData->pMessage);
        }
        else
        {
                Log::debug("Vulkan Validation Verbose: %s", pCallbackData->pMessage);
        }
        return VK_FALSE;
}

} // namespace rl
