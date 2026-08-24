#pragma once

#include <stdint.h>
#include <vulkan/vulkan.h>

#include "rlgame.base/cvulkan/cvulkan_platform.h"

struct R_CVulkan_Device;

/**
 * @brief Configuration parameters for image view creation
 */
struct R_CVulkan_ImageViewCreateInfo
{
                const struct R_CVulkan_Device* pDevice; /**< R_CVulkan device wrapper */
                VkImage                        image; /**< Image to create view for */
                VkImageViewType                viewType; /**< View type (1D, 2D, 3D, cube, etc.) */
                VkFormat                       format; /**< Image format */
                VkComponentMapping             components; /**< Component mapping for swizzling */
                VkImageSubresourceRange        subresourceRange; /**< Subresource range */
};

/**
 * @brief Safe wrapper for VkImageView
 */
struct R_CVulkan_ImageView
{
                VkImageView handle; /**< Raw Vulkan image view handle */
                VkDevice    device; /**< Associated device */
                VkImage     image; /**< Associated image */
                VkFormat    format; /**< View format */
                R_CVULKAN_DEBUG_FIELD
};

/**
 * @brief Initialize an image view
 * @param pImageView Pointer to image view to initialize
 * @param pCreateInfo Image view creation parameters
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkanError R_CVulkan_NewImageView (
    struct R_CVulkan_ImageView*                 pImageView,
    const struct R_CVulkan_ImageViewCreateInfo* pCreateInfo);

/**
 * @brief Deletes an image view and destroy the Vulkan object
 * @param imageView Pointer to image view to delete
 */
R_CVULKAN_API void R_CVulkan_DeleteImageView (struct R_CVulkan_ImageView* pImageView);

/**
 * @brief Get the raw Vulkan image view handle
 * @param imageView Pointer to image view
 * @return Vulkan image view handle, or VK_NULL_HANDLE if not initialized
 */
R_CVULKAN_API VkImageView R_CVulkan_ImageViewGetHandle (const struct R_CVulkan_ImageView* pImageView);

/**
 * @brief Get the associated device
 * @param imageView Pointer to image view
 * @return Vulkan device handle, or VK_NULL_HANDLE if not initialized
 */
R_CVULKAN_API VkDevice R_CVulkan_ImageViewGetDevice (const struct R_CVulkan_ImageView* pImageView);

/**
 * @brief Get the associated image
 * @param imageView Pointer to image view
 * @return Image handle, or VK_NULL_HANDLE if not initialized
 */
R_CVULKAN_API VkImage R_CVulkan_ImageViewGetImage (const struct R_CVulkan_ImageView* pImageView);

/**
 * @brief Get the view format
 * @param imageView Pointer to image view
 * @return View format, or VK_FORMAT_UNDEFINED if not initialized
 */
R_CVULKAN_API VkFormat R_CVulkan_ImageViewGetFormat (const struct R_CVulkan_ImageView* pImageView);

/**
 * @brief Check if the image view is initialized
 * @param imageView Pointer to image view
 * @return 1 if initialized, 0 otherwise
 */
R_CVULKAN_API int R_CVulkan_ImageViewIsInitialized (const struct R_CVulkan_ImageView* pImageView);
