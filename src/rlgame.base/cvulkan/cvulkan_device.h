#pragma once

#include <stdint.h>
#include <vulkan/vulkan.h>

#include "rlgame.base/cvulkan/cvulkan_common.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"
#include "rlgame.base/cvulkan/cvulkan_queue.h"

struct R_CSTL_Array;

/**
 * @brief Configuration parameters for device creation
 */
struct R_CVulkan_DeviceCreateInfo
{
                const char*  pApplicationName; /**< Application name (can be NULL) */
                bool         enableValidationLayers; /**< Whether to enable validation layers */
                bool         headlessMode; /**< Whether to run in headless mode (no surface) */
                VkSurfaceKHR surface; /**< Optional surface for presentation (can be NULL in headless mode) */
};

/**
 * @brief Queue family indices for device selection
 */
struct R_CVulkan_QueueFamilyIndices
{
                uint32_t graphicsFamily;
                uint32_t presentFamily;
                bool     hasGraphicsFamily;
                bool     hasPresentFamily;
};

/**
 * @brief Swapchain support details
 */
struct R_CVulkan_SwapChainSupport
{
                struct R_CSTL_Array*     pFormats;
                struct R_CSTL_Array*     pPresentModes;
                VkSurfaceCapabilitiesKHR capabilities;
};

/**
 * @brief Vulkan device wrapper containing instance, physical device, and logical device
 */
struct R_CVulkan_Device
{
                VkInstance               instance;
                VkPhysicalDevice         physicalDevice;
                VkDevice                 logicalDevice;
                VkSurfaceKHR             surface;
                struct R_CVulkan_Queue   graphicsQueue;
                struct R_CVulkan_Queue   presentQueue;
                VkDebugUtilsMessengerEXT debugMessenger;
                bool                     enableValidationLayers;
                bool                     headlessMode;
                R_CVULKAN_DEBUG_FIELD
};

/**
 * @brief Initialize a Vulkan device with instance and logical device
 * @param pDevice Pointer to device to initialize
 * @param pCreateInfo Device creation parameters
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkan_Error R_CVulkan_NewDevice (
    struct R_CVulkan_Device*             pDevice,
    const struct R_CVulkan_DeviceCreateInfo* pCreateInfo);

/**
 * @brief Delete a Vulkan device and cleanup resources
 * @param pDevice Pointer to device to delete
 */
R_CVULKAN_API void R_CVulkan_DeleteDevice (struct R_CVulkan_Device* pDevice);

/**
 * @brief Get the Vulkan instance handle
 * @param pDevice Pointer to device
 * @return Vulkan instance handle, or VK_NULL_HANDLE if not initialized
 */
R_CVULKAN_API VkInstance R_CVulkan_DeviceGetInstance (const struct R_CVulkan_Device* pDevice);

/**
 * @brief Get the physical device handle
 * @param pDevice Pointer to device
 * @return Vulkan physical device handle, or VK_NULL_HANDLE if not initialized
 */
R_CVULKAN_API VkPhysicalDevice R_CVulkan_DeviceGetPhysicalDevice (const struct R_CVulkan_Device* pDevice);

/**
 * @brief Get the logical device handle
 * @param pDevice Pointer to device
 * @return Vulkan logical device handle, or VK_NULL_HANDLE if not initialized
 */
R_CVULKAN_API VkDevice R_CVulkan_DeviceGetLogicalDevice (const struct R_CVulkan_Device* pDevice);

/**
 * @brief Get the graphics queue
 * @param pDevice Pointer to device
 * @return Pointer to graphics queue, or NULL if not initialized
 */
R_CVULKAN_API struct R_CVulkan_Queue*
R_CVulkan_DeviceGetGraphicsQueue (const struct R_CVulkan_Device* pDevice);

/**
 * @brief Get the present queue
 * @param pDevice Pointer to device
 * @return Pointer to present queue, or NULL if not initialized
 */
R_CVULKAN_API struct R_CVulkan_Queue*
R_CVulkan_DeviceGetPresentQueue (const struct R_CVulkan_Device* pDevice);

/**
 * @brief Get the surface handle
 * @param pDevice Pointer to device
 * @return Vulkan surface handle, or VK_NULL_HANDLE if not initialized
 */
R_CVULKAN_API VkSurfaceKHR R_CVulkan_DeviceGetSurface (const struct R_CVulkan_Device* pDevice);

/**
 * @brief Check if the device is initialized
 * @param pDevice Pointer to device
 * @return 1 if initialized, 0 otherwise
 */
R_CVULKAN_API int R_CVulkan_DeviceIsInitialized (const struct R_CVulkan_Device* pDevice);

/**
 * @brief Find queue family indices for a physical device
 * @param physicalDevice Vulkan physical device
 * @param surface Vulkan surface (can be VK_NULL_HANDLE in headless mode)
 * @param headlessMode Whether running in headless mode
 * @param pOutIndices Pointer to receive queue family indices
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkan_Error R_CVulkan_DeviceFindQueueFamilies (
    VkPhysicalDevice                     physicalDevice,
    VkSurfaceKHR                         surface,
    bool                                 headlessMode,
    struct R_CVulkan_QueueFamilyIndices* pOutIndices);

/**
 * @brief Check if queue family indices are complete
 * @param pIndices Pointer to queue family indices
 * @return 1 if complete, 0 otherwise
 */
R_CVULKAN_API int
R_CVulkan_QueueFamilyIndicesIsComplete (const struct R_CVulkan_QueueFamilyIndices* pIndices);

/**
 * @brief Check if dynamic rendering is supported
 * @param pDevice Pointer to device
 * @return 1 if supported, 0 otherwise
 */
R_CVULKAN_API int R_CVulkan_DeviceIsDynamicRenderingSupported (const struct R_CVulkan_Device* pDevice);
