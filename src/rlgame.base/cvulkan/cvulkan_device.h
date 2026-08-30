#pragma once

#include <stdint.h>
#include <vulkan/vulkan.h>

#include "rlgame.base/cvulkan/cvulkan_platform.h"
#include "rlgame.base/cvulkan/cvulkan_queue.h"
#include "rlgame.base/cstl/cstl_array.h"
#include "rlgame.base/cstl/cstl_string.h"

struct R_CSTL_Array;

struct R_CVulkan_Surface;
struct R_CVulkan_Instance;

/**
 * @brief Settingsuration parameters for device creation
 * The instance must be created separately using R_CVulkan_NewInstance before creating a device.
 */
struct R_CVulkan_DeviceCreateInfo
{
        const struct R_CVulkan_Instance* pInstance; /**< Vulkan instance (required) */
        const struct R_CVulkan_Surface*
            pSurface; /**< Optional surface for presentation (can be NULL in headless mode) */
};

/**
 * @brief Queue family indices for device selection
 */
struct R_CVulkan_QueueFamilyIndices
{
        uint32_t graphicsFamily;
        uint32_t presentFamily;
        uint32_t computeFamily;
        uint32_t transferFamily;
        bool     hasGraphicsFamily;
        bool     hasPresentFamily;
        bool     hasComputeFamily;
        bool     hasTransferFamily;
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
 * @brief Vulkan device wrapper containing physical device and logical device
 * The instance is managed separately by R_CVulkan_Instance.
 */
struct R_CVulkan_Device
{
        const struct R_CVulkan_Instance* pInstance; /**< Associated Vulkan instance */
        VkPhysicalDevice                 physicalDevice;
        VkDevice                         logicalDevice;
        VkSurfaceKHR                     surface;
        struct R_CVulkan_Queue           graphicsQueue;
        struct R_CVulkan_Queue           presentQueue;
};

/**
 * @brief Initialize a Vulkan device with the specified instance and surface
 * @param pDevice Pointer to device to initialize
 * @param pCreateInfo Device creation parameters (must include valid instance)
 * @return R_CVULKAN_OK on success, error code otherwise
 *
 * This function creates a logical Vulkan device from a physical device.
 * The instance must be created separately using R_CVulkan_NewInstance before calling this function.
 * The surface is optional but required for presentation (swapchain support).
 *
 * Common errors:
 * - R_CVULKAN_ERROR_NOT_INITIALIZED: Instance not initialized or surface not provided when required
 * - R_CVULKAN_ERROR_PHYSICAL_DEVICE_NOT_FOUND: No suitable physical device found
 * - R_CVULKAN_ERROR_DEVICE_CREATE_FAILED: Failed to create logical device
 */
R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_NewDevice (struct R_CVulkan_Device* pDevice, const struct R_CVulkan_DeviceCreateInfo* pCreateInfo);

/**
 * @brief Delete a Vulkan device and cleanup resources
 * @param pDevice Pointer to device to delete
 */
R_CVULKAN_API void R_CVulkan_DeleteDevice (struct R_CVulkan_Device* pDevice);

/**
 * @brief Get the associated Vulkan instance
 * @param pDevice Pointer to device
 * @return Pointer to Vulkan instance, or NULL if not initialized
 */
R_CVULKAN_API const struct R_CVulkan_Instance*
R_CVulkan_DeviceGetInstance (const struct R_CVulkan_Device* pDevice);

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

/**
 * @brief Query if a specific extension is supported on the physical device
 *
 * This function dynamically checks if a given Vulkan extension is available
 * on the physical device, allowing for flexible feature detection.
 *
 * @param pDevice Pointer to device
 * @param pExtensionName Null-terminated extension name to query
 * @param pIsSupported Output pointer to receive support status
 * @return R_CVULKAN_OK on success, error code on failure
 *
 * @note This function queries the physical device's available extensions
 * @note Common extension names: VK_KHR_DYNAMIC_RENDERING, VK_KHR_SWAPCHAIN, etc.
 */
R_CVULKAN_API enum R_CVulkan_Error R_CVulkan_DeviceQueryExtensionSupport (
    const struct R_CVulkan_Device* pDevice,
    const char*                    pExtensionName,
    bool*                          pIsSupported);

/**
 * @brief Query if a specific feature is supported on the physical device
 *
 * This function dynamically checks if a given Vulkan feature is available
 * on the physical device using the features2 chain.
 *
 * @param pDevice Pointer to device
 * @param pFeatureStructure Pointer to feature structure to query (must have sType set)
 * @return R_CVULKAN_OK on success, error code on failure
 *
 * @note The feature structure must have its sType member set appropriately
 * @note For example, use VkPhysicalDeviceDynamicRenderingFeaturesKHR for dynamic rendering
 */
R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_DeviceQueryFeatureSupport (const struct R_CVulkan_Device* pDevice, void* pFeatureStructure);
