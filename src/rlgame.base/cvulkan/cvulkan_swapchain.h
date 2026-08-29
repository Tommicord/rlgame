#pragma once

#include <vulkan/vulkan.h>
#include <stdint.h>
#include <inttypes.h>

#include "rlgame.base/cvulkan/cvulkan_platform.h"

struct R_CVulkan_Device;
struct R_CVulkan_Surface;

/**
 * @file cvulkan_swapchain.h
 * @brief Vulkan swapchain wrapper for presentation
 *
 * This module provides a safe wrapper for VkSwapchainKHR, which represents
 * a chain of images for presentation to a surface. The swapchain manages
 * the set of images used for rendering and presentation.
 */

/**
 * @brief Configuration parameters for swapchain creation
 */
struct R_CVulkan_SwapchainCreateInfo
{
        const struct R_CVulkan_Device*  pDevice; /**< Vulkan device (required) */
        const struct R_CVulkan_Surface* pSurface; /**< Vulkan surface (required) */
        uint32_t                        imageCount; /**< Number of images in swapchain */
        VkSurfaceFormatKHR              surfaceFormat; /**< Surface format */
        VkPresentModeKHR                presentMode; /**< Present mode */
        VkExtent2D                      extent; /**< Extent (use {0,0} for surface extent) */
        VkImageUsageFlags
                 imageUsage; /**< Image usage flags (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT by default) */
        uint32_t arrayLayers; /**< Array layers (1 by default) */
        VkSurfaceTransformFlagBitsKHR
            preTransform; /**< Pre-transform (VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR by default) */
        VkCompositeAlphaFlagBitsKHR
                       compositeAlpha; /**< Composite alpha (VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR by default) */
        VkBool32       clipped; /**< Clipped (VK_TRUE by default) */
        VkSwapchainKHR oldSwapchain; /**< Old swapchain to replace (VK_NULL_HANDLE by default) */
};

/**
 * @brief Safe wrapper for VkSwapchainKHR
 */
struct R_CVulkan_Swapchain
{
        VkSwapchainKHR handle; /**< Raw Vulkan swapchain handle */
        VkDevice       device; /**< Associated device */
        VkFormat       imageFormat; /**< Format of swapchain images */
        VkExtent2D     extent; /**< Extent of swapchain images */
        uint32_t       imageCount; /**< Number of images in swapchain */
};

/**
 * @brief Initialize a swapchain
 * @param pSwapchain Pointer to swapchain to initialize
 * @param pCreateInfo Swapchain creation parameters (must include valid device and surface)
 * @return R_CVULKAN_OK on success, error code otherwise
 *
 * This function creates a Vulkan swapchain from a device and surface.
 * The device must be created separately using R_CVulkan_NewDevice before calling this function.
 *
 * Common errors:
 * - R_CVULKAN_ERROR_NOT_INITIALIZED: Device or surface not initialized
 * - R_CVULKAN_ERROR_SWAPCHAIN_CREATE_FAILED: Failed to create swapchain
 * - R_CVULKAN_ERROR_OUT_OF_MEMORY: Memory allocation failed
 */
R_CVULKAN_API enum R_CVulkan_Error R_CVulkan_NewSwapchain (
    struct R_CVulkan_Swapchain*                 pSwapchain,
    const struct R_CVulkan_SwapchainCreateInfo* pCreateInfo);

/**
 * @brief Delete a swapchain and destroy the Vulkan object
 * @param pSwapchain Pointer to swapchain to delete
 */
R_CVULKAN_API void R_CVulkan_DeleteSwapchain (struct R_CVulkan_Swapchain* pSwapchain);

/**
 * @brief Get the raw Vulkan swapchain handle
 * @param pSwapchain Pointer to swapchain
 * @return Vulkan swapchain handle, or VK_NULL_HANDLE if not initialized
 */
R_CVULKAN_API VkSwapchainKHR R_CVulkan_SwapchainGetHandle (const struct R_CVulkan_Swapchain* pSwapchain);

/**
 * @brief Get the associated device
 * @param pSwapchain Pointer to swapchain
 * @return Vulkan device handle, or VK_NULL_HANDLE if not initialized
 */
R_CVULKAN_API VkDevice R_CVulkan_SwapchainGetDevice (const struct R_CVulkan_Swapchain* pSwapchain);

/**
 * @brief Get the image format of the swapchain
 * @param pSwapchain Pointer to swapchain
 * @return Image format, or VK_FORMAT_UNDEFINED if not initialized
 */
R_CVULKAN_API VkFormat R_CVulkan_SwapchainGetImageFormat (const struct R_CVulkan_Swapchain* pSwapchain);

/**
 * @brief Get the extent of swapchain images
 * @param pSwapchain Pointer to swapchain
 * @return Extent of swapchain images, or {0,0} if not initialized
 */
R_CVULKAN_API VkExtent2D R_CVulkan_SwapchainGetExtent (const struct R_CVulkan_Swapchain* pSwapchain);

/**
 * @brief Get the number of images in the swapchain
 * @param pSwapchain Pointer to swapchain
 * @return Number of images, or 0 if not initialized
 */
R_CVULKAN_API uint32_t R_CVulkan_SwapchainGetImageCount (const struct R_CVulkan_Swapchain* pSwapchain);

/**
 * @brief Acquire the next image from the swapchain
 * @param pSwapchain Pointer to swapchain
 * @param timeout Timeout in nanoseconds (UINT64_MAX for infinite)
 * @param semaphore Semaphore to signal when image is ready (can be VK_NULL_HANDLE)
 * @param fence Fence to signal when image is ready (can be VK_NULL_HANDLE)
 * @param pImageIndex Pointer to receive the image index
 * @return R_CVULKAN_OK on success, error code otherwise
 *
 * Common errors:
 * - R_CVULKAN_ERROR_SWAPCHAIN_OUT_OF_DATE: Swapchain needs recreation
 * - R_CVULKAN_ERROR_SWAPCHAIN_SUBOPTIMAL: Swapchain is suboptimal
 */
R_CVULKAN_API enum R_CVulkan_Error R_CVulkan_SwapchainAcquireNextImage (
    struct R_CVulkan_Swapchain* pSwapchain,
    uint64_t                    timeout,
    VkSemaphore                 semaphore,
    VkFence                     fence,
    uint32_t*                   pImageIndex);