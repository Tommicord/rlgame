#pragma once

#include <vulkan/vulkan.h>
#include <stdint.h>

#include "rlgame.base/cvulkan/cvulkan_common.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"
#include "rlgame.base/cstl/cstl_string.h"

/**
 * @file cvulkan_instance.h
 * @brief Vulkan instance wrapper for managing Vulkan instances
 *
 * This module provides a safe wrapper for VkInstance, which represents
 * a Vulkan instance that holds the global state of the Vulkan library.
 * The instance must be created before any other Vulkan objects.
 */

/**
 * @brief Configuration parameters for instance creation
 */
struct R_CVulkan_InstanceCreateInfo
{
                const struct R_CSTL_String* pApplicationName; /**< Application name (can be NULL) */
                uint32_t                    applicationVersion; /**< Application version (default: 1,0,0) */
                const struct R_CSTL_String* pEngineName; /**< Engine name (can be NULL) */
                uint32_t                    engineVersion; /**< Engine version (default: 1,0,0) */
                uint32_t apiVersion; /**< Vulkan API version (default: VK_API_VERSION_1_1) */
                bool enableValidationLayers; /**< Enable validation layers (default: from R_CVULKAN_DEBUG) */
                bool enableHeadlessMode; /**< Enable headless mode (default: from R_CVULKAN_HEADLESS) */
};

/**
 * @brief Safe wrapper for VkInstance
 */
struct R_CVulkan_Instance
{
                VkInstance               handle; /**< Raw Vulkan instance handle */
                VkDebugUtilsMessengerEXT debugMessenger; /**< Debug messenger for validation layers */
                R_CVULKAN_DEBUG_FIELD
};

/**
 * @brief Create a Vulkan instance with the specified configuration
 * @param pInstance Pointer to instance to initialize
 * @param pCreateInfo Instance creation parameters
 * @return R_CVULKAN_OK on success, error code otherwise
 *
 * This function creates a Vulkan instance with the specified configuration.
 * The instance must be created before any other Vulkan objects (surfaces, devices, etc.).
 *
 * Common errors:
 * - R_CVULKAN_ERROR_LAYER_NOT_FOUND: Validation layers requested but not available
 * - R_CVULKAN_ERROR_EXTENSION_NOT_FOUND: Required extension not available
 * - R_CVULKAN_ERROR_INCOMPATIBLE_DRIVER: Vulkan driver version incompatible
 */
R_CVULKAN_API enum R_CVulkan_Error R_CVulkan_NewInstance (
    struct R_CVulkan_Instance*                 pInstance,
    const struct R_CVulkan_InstanceCreateInfo* pCreateInfo);

/**
 * @brief Delete a Vulkan instance and cleanup resources
 * @param pInstance Pointer to instance to delete
 *
 * This function destroys the Vulkan instance and all associated resources.
 * All Vulkan objects created from this instance must be destroyed before calling this function.
 */
R_CVULKAN_API void R_CVulkan_DeleteInstance (struct R_CVulkan_Instance* pInstance);

/**
 * @brief Get the raw Vulkan instance handle
 * @param pInstance Pointer to instance
 * @return Vulkan instance handle, or VK_NULL_HANDLE if not initialized
 */
R_CVULKAN_API VkInstance R_CVulkan_InstanceGetHandle (const struct R_CVulkan_Instance* pInstance);

/**
 * @brief Check if the instance is initialized
 * @param pInstance Pointer to instance
 * @return 1 if initialized, 0 otherwise
 */
R_CVULKAN_API int R_CVulkan_InstanceIsInitialized (const struct R_CVulkan_Instance* pInstance);
