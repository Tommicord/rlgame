#pragma once

#include <stdint.h>
#include <vulkan/vulkan.h>

#include "rlgame.base/cvulkan/cvulkan_common.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"

struct R_CVulkan_Device;

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
 * @param imageView Pointer to image view to initialize
 * @param device R_CVulkan device wrapper
 * @param image Image to create view from
 * @param viewType Image view type
 * @param format Format of the view
 * @param components Component mapping (use VK_COMPONENT_SWIZZLE_IDENTITY for default)
 * @param subresourceRange Subresource range to view
 * @return R_CVULKAN_OK on success, error code otherwise
 */
enum R_CVulkan_Error R_CVulkan_NewImage (
    struct R_CVulkan_ImageView*    imageView,
    const struct R_CVulkan_Device* device,
    VkImage                        image,
    VkImageViewType                viewType,
    VkFormat                       format,
    VkComponentMapping             components,
    VkImageSubresourceRange        subresourceRange);

/**
 * @brief Deletes an image view and destroy the Vulkan object
 * @param imageView Pointer to image view to delete
 */
void R_CVulkan_DeleteImageView (struct R_CVulkan_ImageView* imageView);

/**
 * @brief Get the raw Vulkan image view handle
 * @param imageView Pointer to image view
 * @return Vulkan image view handle, or VK_NULL_HANDLE if not initialized
 */
VkImageView R_CVulkan_ImageViewGetHandle (const struct R_CVulkan_ImageView* imageView);

/**
 * @brief Get the associated device
 * @param imageView Pointer to image view
 * @return Vulkan device handle, or VK_NULL_HANDLE if not initialized
 */
VkDevice R_CVulkan_ImageViewGetDevice (const struct R_CVulkan_ImageView* imageView);

/**
 * @brief Get the associated image
 * @param imageView Pointer to image view
 * @return Image handle, or VK_NULL_HANDLE if not initialized
 */
VkImage R_CVulkan_ImageViewGetImage (const struct R_CVulkan_ImageView* imageView);

/**
 * @brief Get the view format
 * @param imageView Pointer to image view
 * @return View format, or VK_FORMAT_UNDEFINED if not initialized
 */
VkFormat R_CVulkan_ImageViewGetFormat (const struct R_CVulkan_ImageView* imageView);

/**
 * @brief Check if the image view is initialized
 * @param imageView Pointer to image view
 * @return 1 if initialized, 0 otherwise
 */
int R_CVulkan_ImageViewIsInitialized (const struct R_CVulkan_ImageView* imageView);
