#pragma once

#include <stdint.h>
#include <vulkan/vulkan.h>

#include "rlgame.base/cvulkan/cvulkan_platform.h"

struct R_CVulkan_Device;

/**
 * @brief Settingsuration parameters for image view creation
 */
struct r_cvulkan_image_view_create_info
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
};

/**
 * @brief Initialize an image view
 * @param pImageView Pointer to image view to initialize
 * @param pCreateInfo Image view creation parameters
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkan_Error r_cvulkan_new_image_view (
    struct R_CVulkan_ImageView*                 pImageView,
    const struct r_cvulkan_image_view_create_info* pCreateInfo);

/**
 * @brief Deletes an image view and destroy the Vulkan object
 * @param imageView Pointer to image view to delete
 */
R_CVULKAN_API void r_cvulkan_delete_image_view (struct R_CVulkan_ImageView* pImageView);

/**
 * @brief Get the raw Vulkan image view handle
 * @param imageView Pointer to image view
 * @return Vulkan image view handle, or VK_NULL_HANDLE if not initialized
 */
R_CVULKAN_API VkImageView r_cvulkan_image_view_get_handle (const struct R_CVulkan_ImageView* pImageView);

/**
 * @brief Get the associated device
 * @param imageView Pointer to image view
 * @return Vulkan device handle, or VK_NULL_HANDLE if not initialized
 */
R_CVULKAN_API VkDevice r_cvulkan_image_view_get_device (const struct R_CVulkan_ImageView* pImageView);

/**
 * @brief Get the associated image
 * @param imageView Pointer to image view
 * @return Image handle, or VK_NULL_HANDLE if not initialized
 */
R_CVULKAN_API VkImage r_cvulkan_image_view_get_image (const struct R_CVulkan_ImageView* pImageView);

/**
 * @brief Get the view format
 * @param imageView Pointer to image view
 * @return View format, or VK_FORMAT_UNDEFINED if not initialized
 */
R_CVULKAN_API VkFormat r_cvulkan_image_view_get_format (const struct R_CVulkan_ImageView* pImageView);
